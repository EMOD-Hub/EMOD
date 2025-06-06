
/////////////////////////////////////////////////////////////////////////////
// Dll export and loading platform implementation
//
#include "stdafx.h"

#include <string.h>
#include <iostream>
#include "DllLoader.h"
#include "Debug.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Log.h"
#include "ProgVersion.h"
#include "Exceptions.h"
#include "CajunIncludes.h"
#ifdef __GNUC__
#include <dlfcn.h>
#include <dirent.h>
#include <stdlib.h>
#endif

typedef char* (*gver)(char*,const Environment * pEnv);

SETUP_LOGGING( "DllLoader" )

#pragma warning(disable : 4996)

/////////////////////////////////////////////////////////////
// Important Note:
// GetEModuleVersion has to be the first exported function to call
// to make sure the environment instance is set from main exe.
/////////////////////////////////////////////////////////////////

// This code now supports loading linux shared objects for reproters once again in the same way as on windows.
// Note that this is not yet true of interventions or sim types -- though that was true in the original version.
// There has been little attempt to optimize this for code reuse between os-es. And that may not be desirable 
// from a readability point of view. If one were to make any change, it might be to optimize around error handling.
// The biggest complaint about non-monolithic code is confusion about when it doesn't work. That can be ameliorated
// in large part but top-notch error handling.
DllLoader::DllLoader(const char* sSimType)
{
    m_sSimType = nullptr;
    memset(m_sSimTypeAll, 0, SIMTYPES_MAXNUM*sizeof(char *));
    if (sSimType)
    {
        m_sSimType = new char[strlen(sSimType)+1];
        strcpy(m_sSimType, sSimType);
    }
}

DllLoader::~DllLoader ()
{
    if (m_sSimType)
    {
        delete m_sSimType;
        m_sSimType = nullptr;
    }
    int i=0;
    while (m_sSimTypeAll[i] != nullptr && i<SIMTYPES_MAXNUM )
    {
        delete m_sSimTypeAll[i];
        m_sSimTypeAll[i] = nullptr;
        i++;
    }
}

void
DllLoader::ReadEmodulesJson(
    const std::string& key,
    std::list< emodulewstr > &dll_dirs
)
{
    if( !FileSystem::FileExists( "emodules_map.json" ) )
    {
        // actually, might be ok if just using --dll_dir, not settled yet
        LOG_INFO( "ReadEmodulesJson: no file, returning.\n" );
        return;
    }

    LOG_DEBUG( "open emodules_map.json\n" );
    std::ifstream emodules_json_raw;
    FileSystem::OpenFileForReading( emodules_json_raw, "emodules_map.json" );

    try
    {
        json::Object emodules;
        json::Reader::Read(emodules, emodules_json_raw);
        const json::Array &disease_dll_array = json::json_cast<const json::Array&>( emodules[ key ] );
        for( unsigned int idx = 0; idx < disease_dll_array.Size(); ++idx )
        {
            LOG_DEBUG_F( "Getting dll_path at idx %d\n", idx );
            const std::string &dll_path = json::json_cast<const json::String&>(disease_dll_array[idx]);
            LOG_DEBUG_F( "Found dll_path %s\n", dll_path.c_str() );

            emodulewstr str2(std::string( dll_path ).length(), L' ');
            std::copy(dll_path.begin(), dll_path.end(), str2.begin());
            dll_dirs.push_back( str2 );

            //LOG_INFO_F( "Stored dll_path %S as wstring in list.\n", str2.c_str() );
        }
        LOG_INFO( "Stored all dll_paths.\n" );
    }
    catch( const json::Exception &e )
    {
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
    }
    emodules_json_raw.close();
}

bool DllLoader::LoadReportDlls( Kernel::support_spec_map_t& reportInstantiators )
{
    bool bRet = false;
    if (!EnvPtr) return bRet;


    std::list< emodulewstr > report_dll_dirs;
    ReadEmodulesJson( REPORTER_EMODULES, report_dll_dirs );


#ifdef WIN32
    emodulewstr dllDir( L" " );
    std::wstring reporter_plugin_str = REPORTER_PLUGINS;
#else
    std::string dllDir( " " );
    std::string reporter_plugin_str = REPORTER_PLUGINS;
#endif
    bool fully_qualified_dll_path = true;
    if( report_dll_dirs.size() == 0 )
    {
        LOG_DEBUG( "Getting REPORTER_PLUGINS dir because no paths from emodules_map.json\n" );
        emodulewstr rpname_as_wstr;
        rpname_as_wstr.assign(reporter_plugin_str.begin(), reporter_plugin_str.end());
        dllDir = GetFullDllPath( rpname_as_wstr ); 

#ifdef WIN32
        emodulewstr &dllDirStar = FileSystem::Concat( dllDir, emodulewstr(L"*") );
#else
        emodulewstr dllDirStar = FileSystem::Concat( dllDir, emodulewstr("") );
#endif
        report_dll_dirs.push_back( dllDirStar );
        fully_qualified_dll_path = false;
    }
    else
    {
        LOG_DEBUG( "Did not use --dll-path since emodules_map.json file was found.\n" );
    }

#ifdef WIN32
    WIN32_FIND_DATA ffd;

    for (auto& dllDirStar : report_dll_dirs)
    {
        LOG_DEBUG_F("Searching reporter plugin dir: %S\n", dllDirStar.c_str());

        HANDLE hFind = FindFirstFile(dllDirStar.c_str(), &ffd);
        do
        {
            if ( ffd.dwFileAttributes > 0 && ffd.cFileName[0] != '\0' && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
            {
                emodulewstr dllPath = dllDirStar;
                LOG_DEBUG_F("Found dll: %S\n", dllPath.c_str());
                if( fully_qualified_dll_path == false )
                {
                    LOG_DEBUG( "We are using reg-ex dll searching, append dir path to filename found.\n" );
                    dllPath = FileSystem::Concat( dllDir, emodulewstr(ffd.cFileName) );
                }

                if( dllPath.find( L".dll" ) == std::string::npos || // must end in .dll
                    dllPath.find( L".dll" ) != dllPath.length()-4 )
                {
                    LOG_DEBUG_F( "%S is not a DLL\n", dllPath );
                    continue;
                }

                LOG_INFO_F( "Calling LoadLibrary on reporter emodule %S\n", dllPath.c_str() );
                HMODULE repDll = LoadLibrary( dllPath.c_str() );

                if( repDll == nullptr )
                {
                    LOG_WARN_F( "Failed to load dll %S\n", ffd.cFileName );
                }
                else
                {

                    LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
                    char emodVersion[64];
                    if (!CheckEModuleVersion(repDll, emodVersion))
                    {
                        LOG_WARN_F("The version of EModule %S is lower than current application!\n", ffd.cFileName);

                        // For now, load report dll anyway
                        //continue;
                    }

                    bool success = GetSimTypes( ffd.cFileName, repDll );
                    if( !success ) continue ;

                    Kernel::instantiator_function_t rif = nullptr ;
                    success = GetReportInstantiator( ffd.cFileName, repDll, &rif );
                    if( !success ) continue ;

                    if( rif != nullptr )
                    {
                        std::string class_name ;
                        success = GetType( ffd.cFileName, repDll, class_name );
                        if( !success ) continue ;

                        reportInstantiators[ class_name ] = rif ;
                        bRet = true ;
                    }
                }
            }
            else
            {
                LOG_DEBUG("That was a directory not an EModule or . or .. so ignore.\n");
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
    }
#else
    // Scan for intervention dlls/shared libraries and Register them with us.
    void * newSimDlHandle = nullptr;
    int (*RegisterDotsoIntervention)( Kernel::InterventionFactory* );
    DIR * dp = nullptr;
    struct dirent *dirp = nullptr;
    for (auto& dllDirStar : report_dll_dirs)
    {
        //std::cout << "Processing " << dllDirStar << std::endl;
        auto report_dir = dllDirStar.c_str();
        LOG_INFO_F( "Considering report 'directory' (may be a file): %s.\n", report_dir );
        std::string filename_str = "";
        if( FileSystem::FileExists( report_dir ) )
        {
            filename_str = report_dir;
        }
        else if( ( dp = opendir( report_dir ) ) == nullptr )
        {
            LOG_WARN_F("Failed to open reporter_plugins directory %s.\n", report_dir );
            return;
        }
        char * filename = nullptr;
        do
        {
            if( dirp != nullptr )
            {
                filename = dirp->d_name;
            }
            else
            {
                filename = filename_str.c_str();
            }
            if( std::string( filename ) == "." || std::string( filename ) == ".." )
            {
                continue;
            }
            LOG_INFO_F( "Still considering %s\n", filename );
            // filename seems to works for emodules_map.json, concat for dllpath
            std::string fullDllPath = filename;
            if( !FileSystem::FileExists( fullDllPath  ) )
            {
                fullDllPath = FileSystem::Concat( std::string( report_dir ), std::string( filename ) );
            }
            newSimDlHandle = dlopen( fullDllPath.c_str(), RTLD_LAZY );
            if( newSimDlHandle )
            {
                LOG_DEBUG_F( "Got 'handle' for shared lib.\n" );
                char emodVersion[64];
                if (!CheckEModuleVersion(newSimDlHandle, emodVersion))
                {
                    LOG_WARN_F("The version of EModule %S is lower than current application!\n", filename);

                    // For now, load report dll anyway
                    //continue;
                } 
                LOG_INFO_F( "Checked EModule version.\n" );
                bool success = GetSimTypes( filename, newSimDlHandle );
                LOG_DEBUG_F( "success = %d.\n", success );
                if( !success )
                {
                    continue;
                }

                Kernel::instantiator_function_t rif = nullptr;
                success = GetReportInstantiator( filename, newSimDlHandle, &rif );
                LOG_DEBUG_F( "success = %d.\n", success );
                if( !success )
                {
                    continue;
                }

                if( rif != nullptr )
                {
                    std::string class_name ;
                    success = GetType( filename, newSimDlHandle, class_name );
                    LOG_DEBUG_F( "success = %d.\n", success );
                    if( !success )
                    {
                        continue;
                    }

                    reportInstantiators[ class_name ] = rif;
                    bRet = true;
                }
            }
            else
            {
                LOG_WARN_F("dlopen failed on %s with error %s\n", fullDllPath.c_str(), dlerror());
            }
            if( dp == 0x0 || dp == nullptr )
            {
                // This should NOT be necessary but I'm not getting while while condition is executing readdir on nullptr.
                std::cout << "dp is null." << std::endl; // apparently this line is critical???!!!
                break;
            }
        } while( dp != nullptr && ( dirp = readdir( dp ) ) != nullptr ); // first condition is for emodules_map, second is for dll-path; but 1st cond being true was still causing second to be checked
    }
#endif

    LOG_DEBUG_F( "bRet = %d.\n", bRet );
    return bRet;
}


#if defined(WIN32)
bool DllLoader::GetSimTypes( const TCHAR* pFilename, HMODULE repDll )
{
    bool success = true ;
    LOG_INFO_F( "Calling GetProcAddress for GetSupportedSimTypes on %S\n", pFilename );
    typedef void (*gst)(char* simType[]);
    gst _gst = (gst)GetProcAddress( repDll, "GetSupportedSimTypes" );
    if( _gst != nullptr )
    {
        (_gst)(m_sSimTypeAll);
        if (!MatchSimType(m_sSimTypeAll)) 
        {
            LogSimTypes(m_sSimTypeAll);
            LOG_WARN_F( "EModule %S does not support current disease SimType %s \n", pFilename, m_sSimType);
            success = false;
        }
    }
    else
    {
        LOG_INFO_F ("GetProcAddress failed for GetSupportedSimTypes: %d.\n", GetLastError() );
        success = false;
    }
    return success ;
}

bool DllLoader::GetReportInstantiator( const TCHAR* pFilename, 
                                       HMODULE repDll, 
                                       Kernel::instantiator_function_t* pRIF )
{
    bool success = false ;
    LOG_DEBUG_F("Calling GetProcAddress for GetReportInstantiator on %S\n", pFilename);
    typedef void (*gri)(Kernel::instantiator_function_t* pif);
    gri gri_func = (gri)GetProcAddress( repDll, "GetReportInstantiator" );
    if( gri_func != nullptr )
    {
        gri_func( pRIF );
        if( *pRIF != nullptr )
        {
            success = true ;
        }
        else
        {
            LOG_WARN_F( "Failed to get Report Instantiator on %S.\n", pFilename );
        }
    }
    else
    {
        LOG_WARN_F( "GetReportInstantiator not supported in %S.\n", pFilename );
    }
    return success ;
}

bool DllLoader::GetType( const TCHAR* pFilename, 
                         HMODULE repDll, 
                         std::string& rClassName )
{
    bool success = true ;

    typedef char* (*gtp)();
    gtp gtp_func = (gtp)GetProcAddress( repDll, "GetType" );
    if( gtp_func != nullptr )
    {
        char* p_class_name = gtp_func();
        if( p_class_name != nullptr )
        {
            rClassName = std::string( p_class_name );
            LOG_INFO_F( "Found Report DLL = %s\n", p_class_name );
        }
        else
        {
            LOG_WARN_F( "GetProcAddr failed for GetType on %S.\n", pFilename );
            success = false ;
        }
    }
    else
    {
        LOG_WARN_F( "GetType not supported in %S.\n", pFilename );
        success = false ;
    }
    return success ;
}
#else
bool DllLoader::GetSimTypes( const char* pFilename, void* repDll )
{
    bool success = true ;
    //LOG_INFO_F( "Calling dlsym for GetSupportedSimTypes on %s\n", pFilename );
    typedef void (*gst)(char* simType[]);
    auto extern_fc = dlsym( repDll, "GetSupportedSimTypes" );
    gst _gst = (gst)extern_fc;
    release_assert( _gst );
    if( _gst != nullptr )
    {
        (*_gst)(m_sSimTypeAll);
        if (!MatchSimType(m_sSimTypeAll)) 
        {
            LogSimTypes(m_sSimTypeAll);
            LOG_WARN_F( "EModule %s does not support current disease SimType %s \n", pFilename, m_sSimType);
            success = false;
        }
    }
    else
    {
        //LOG_INFO_F ("dlsym failed for GetSupportedSimTypes.\n" );
        success = false;
    }
    return success ;
}

bool DllLoader::GetReportInstantiator( const char* pFilename, 
                                       void* repDll, 
                                       Kernel::instantiator_function_t* pRIF )
{
    bool success = false ;
    //LOG_DEBUG_F("Calling dlsym for GetReportInstantiator on %S\n", pFilename);
    typedef void (*gri)(Kernel::instantiator_function_t*);
    auto gri_func = (gri)dlsym( repDll, "GetReportInstantiator" );
    if( gri_func != nullptr )
    {
        (*gri_func)( pRIF );
        if( *pRIF != nullptr )
        {
            success = true;
        }
        else
        {
            LOG_WARN_F( "Failed to get Report Instantiator on %S.\n", pFilename );
        }
    }
    else
    {
        LOG_WARN_F( "GetReportInstantiator not supported in %S.\n", pFilename );
    }
    return success ;
}

bool DllLoader::GetType( const char* pFilename, 
                         void* repDll, 
                         std::string& rClassName )
{
    bool success = true ;

    typedef char* (*gtp)();
    gtp gtp_func = (gtp)dlsym( repDll, "GetType" );
    if( gtp_func != nullptr )
    {
        char* p_class_name = (*gtp_func)();
        if( p_class_name != nullptr )
        {
            rClassName = std::string( p_class_name );
            //LOG_INFO_F( "Found Report DLL = %s\n", p_class_name );
        }
        else
        {
            LOG_WARN_F( "GetProcAddr failed for GetType on %S.\n", pFilename );
            success = false;
        }
    }
    else
    {
        LOG_WARN_F( "GetType not supported in %S.\n", pFilename );
        success = false;
    }
    return success;
}

#endif

bool DllLoader::IsValidVersion(const char* emodVer)
{
    bool bValid = false;
    if (!emodVer)
    {
        // if emodule has null version, something is wrong
        return bValid;
    }

    ProgDllVersion pv;
    if (pv.checkProgVersion(emodVer) >= 0)
    {
        bValid = true;
    }
    else
    {
        LOG_INFO_F("The application has version %s while the emodule has version %s\n", pv.getVersion(), emodVer);
    }

    return bValid;
}

bool DllLoader::MatchSimType(char* simTypes[])
{
    bool bMatched = true;
    if (!m_sSimType)
    {
        // The Dll loader doesn't care about the SimType, so return bMatched=true
        return bMatched;
    }

    int i=0;
    bMatched = false;
    while (simTypes[i] != nullptr && i < SIMTYPES_MAXNUM)
    {
        if( (simTypes[i][0] == '*') || (strcmp(simTypes[i], m_sSimType) == 0) )
        {
            bMatched = true;
            break;
        }
        i++;
    }

    return bMatched;
}

void DllLoader::LogSimTypes(char* simTypes[])
{
    int i=0;
    while (simTypes[i] != nullptr && i < SIMTYPES_MAXNUM)
    {
        LOG_INFO_F("SimType: %s \n", simTypes[i]);
        i++;
    }

}

std::string DllLoader::GetEModulePath(const char* emoduleDir)
{
    std::string sDllRootPath = ".";
    if (EnvPtr)
    {
        sDllRootPath = EnvPtr->DllPath;
    }
    return FileSystem::Concat( sDllRootPath, std::string(emoduleDir) );
}


bool DllLoader::GetEModulesVersion(const char* dllPath, list<string>& dllNames, list<string>& dllVersions)
{

    if (!dllPath || dllPath[0] == '\0')
    {
        LOG_DEBUG("The EModule root path is not given, so nothing to get version from.\n");
        return false;
    }

#ifdef WIN32

   return GetDllsVersion(dllPath, emodulewstr( DISEASE_PLUGINS ), dllNames, dllVersions)
          && GetDllsVersion(dllPath, emodulewstr( REPORTER_PLUGINS ), dllNames, dllVersions)
          && GetDllsVersion(dllPath, emodulewstr( INTERVENTION_PLUGINS ), dllNames, dllVersions);
#else
#warning "Linux shared library for disease not implemented yet at load."
#endif
}

bool DllLoader::StringEquals(const emodulewstr& wStr, const char* cStr) 
{ 
    std::string str(cStr); 

    if (wStr.size() < str.size()) 
    {
        return false; 
    }

    return std::equal(str.begin(), str.end(), wStr.begin()); 
}

#ifdef WIN32
bool DllLoader::StringEquals(const TCHAR* tStr, const char* cStr) 
{ 
    emodulewstr wStr(tStr);
    return StringEquals(wStr, cStr);
}

emodulewstr DllLoader::GetFullDllPath(emodulewstr& pluginDir, const char* dllPath)
{
    LOG_DEBUG( "GetFullDllPath\n" );
    std::string sDllRootPath = "";
    if (dllPath)
    {
        LOG_INFO( "dllPath passed in\n" );
        sDllRootPath = dllPath;
    }
    else if (EnvPtr)
    {
        LOG_INFO( "dllPath not passed in, getting from EnvPtr\n" );
        release_assert( EnvPtr );
        sDllRootPath = EnvPtr->DllPath;
    }

    LOG_DEBUG( "Trying to copy from string to wstring.\n" );
    emodulewstr wsDllRootPath;

    wsDllRootPath.assign(sDllRootPath.begin(), sDllRootPath.end());
    LOG_INFO_F("DLL ws root path: %S\n", wsDllRootPath.c_str());
    return FileSystem::Concat( wsDllRootPath, pluginDir );
}

bool DllLoader::CheckEModuleVersion(HMODULE hEMod, char* emodVer)
{
    bool bRet = false;
    gver _gver = (gver)GetProcAddress( hEMod, "GetEModuleVersion" );
    if( _gver != nullptr )
    {
        char emodVersion[64];
        (_gver)(emodVersion,EnvPtr);
        bRet = IsValidVersion(emodVersion);
        if (emodVer)
        {
            strcpy(emodVer, emodVersion);
        }
    }
    else
    {
        LOG_WARN("GetProcAddr failed for GetEModuleVersion.\n");
    }
    return bRet;
}
#else
bool DllLoader::StringEquals(const char* tStr, const char* cStr) 
{ 
    emodulewstr wStr(tStr);
    return StringEquals(wStr, cStr);
}

std::string DllLoader::GetFullDllPath(std::string& pluginDir, const char* dllPath)
{
    LOG_DEBUG( "GetFullDllPath\n" );
    std::string sDllRootPath = "";
    if (dllPath)
    {
        LOG_DEBUG( "dllPath passed in\n" );
        sDllRootPath = dllPath;
    }
    else if (EnvPtr)
    {
        LOG_DEBUG( "dllPath not passed in, getting from EnvPtr\n" );
        release_assert( EnvPtr );
        sDllRootPath = EnvPtr->DllPath;
    }
    LOG_DEBUG( "Trying to copy from string to string.\n" );
    std::string wsDllRootPath;
    wsDllRootPath.assign(sDllRootPath.begin(), sDllRootPath.end());
    LOG_DEBUG_F("DLL ws root path: %S\n", wsDllRootPath.c_str());
    return FileSystem::Concat( wsDllRootPath, pluginDir );
}

bool DllLoader::CheckEModuleVersion(void* hEMod, char* emodVer)
{
    //LOG_INFO_F( "%s.\n", __FUNCTION__ );
    bool bRet = false;

    //auto extern_fc = dlsym( hEMod, "GetEModuleVersion" );
    LOG_DEBUG_F( "dlsym (GetEModuleVersion) worked. Now cast.\n", __FUNCTION__ );
    //auto _gver = (gver)extern_fc;
    auto _gver = (gver)dlsym( hEMod, "GetEModuleVersion" );
    if( _gver != nullptr )
    {
        char emodVersion[64];
        (*_gver)(emodVersion,EnvPtr);
        LOG_DEBUG_F( "emodVersion = %s.\n", emodVersion );
        bRet = IsValidVersion(emodVersion);
        LOG_DEBUG_F( "bRet = %d.\n", bRet );
        if (emodVer)
        {
            strcpy(emodVer, emodVersion);
        }
    }
    else
    {
        LOG_WARN("dlsym failed for GetEModuleVersion.\n");
    }
    return bRet;
}
#endif


#if defined(WIN32)
bool DllLoader::GetDllsVersion(const char* dllPath, emodulewstr& wsPluginDir,list<string>& dllNames, list<string>& dllVersions)
{    

    // Look through disease dll directory, do LoadLibrary on each .dll,
    // do GetProcAddress for get
    bool bRet = true;

    emodulewstr dllDir = GetFullDllPath(wsPluginDir, dllPath); 
    emodulewstr dllDirStar = FileSystem::Concat( dllDir, emodulewstr(L"*") );
    
    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(dllDirStar.c_str(), &ffd);
    do
    {
        if ( ffd.dwFileAttributes > 0 && ffd.cFileName[0] != '\0' && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
        {
            emodulewstr dllPath = FileSystem::Concat( dllDir, emodulewstr(ffd.cFileName) );
                
            // must end in .dll
            if( dllPath.find( L".dll" ) == std::string::npos ||
                dllPath.find( L".dll" ) != dllPath.length()-4 )
            {
                LOG_INFO_F("Not a dll ( %S) \n", ffd.cFileName);
                continue;
            }
                

            LOG_INFO_F("Calling LoadLibrary for %S\n", dllPath.c_str());
            HMODULE ecDll = LoadLibrary( dllPath.c_str() );
            if( ecDll == nullptr )
            {
                LOG_WARN_F("Failed to load dll %S\n",ffd.cFileName);
            }
            else
            {

                LOG_INFO("Calling GetProcAddress for GetEModuleVersion\n");
                char emodVersion[64];
                if (!CheckEModuleVersion(ecDll, emodVersion))
                {
                    LOG_WARN_F("The version of EModule %S is lower than current application\n", ffd.cFileName);
                }
                dllVersions.push_back(emodVersion);
                                   
                // Convert to a char*
                size_t wcsize = wcslen(ffd.cFileName) + 1;
                const size_t newsize = 256;
                size_t convertedChars = 0;
                char namestring[newsize];
                wcstombs_s(&convertedChars, namestring, wcsize, ffd.cFileName, _TRUNCATE);
                dllNames.push_back(namestring);

            }
        }
        else
        {
            LOG_DEBUG("That was a directory not an EModule or . or .. so ignore.\n");
        }
    }
    while (FindNextFile(hFind, &ffd) != 0);

    return bRet;

}

#endif // End of WIN32
