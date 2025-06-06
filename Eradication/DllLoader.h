
#pragma once

#include <vector>
#include <unordered_map>
#include <list>
#include <string>

#include "ISimulation.h"
#include "IReport.h"
#include "InterventionFactory.h"
#include "DllDefs.h"

#ifdef __GNUC__
#include <stdint.h>
#endif

using namespace std;

typedef Kernel::ISimulation* (*createSim)(const Environment *);
typedef const char* (*getSchema)();
#ifdef WIN32
typedef std::wstring emodulewstr;
#else
typedef std::string emodulewstr;
#endif

class DllLoader
{

public:

    DllLoader(const char* sSimType = nullptr);

    virtual ~DllLoader ();

    bool LoadReportDlls( Kernel::support_spec_map_t& reportInstantiators );

    bool StringEquals(const emodulewstr& wStr, const char* cStr);
#if defined(WIN32)
    bool StringEquals(const TCHAR* tStr, const char* cStr);
    bool CheckEModuleVersion(HMODULE hEMod, char* emodVer=nullptr);
#else
    bool StringEquals(const char* tStr, const char* cStr);
    bool CheckEModuleVersion(void* hEMod, char* emodVer=nullptr);
#endif
    bool GetDllsVersion(const char* dllPath, emodulewstr& wsPluginDir,list<string>& dllNames, list<string>& dllVersions);
    emodulewstr GetFullDllPath(emodulewstr& pluginDir, const char* dllPath = nullptr);

    string GetEModulePath(const char* emoduleDir);
    bool GetEModulesVersion(const char* dllPath, list<string>& dllNames, list<string>& dllVersions);

protected:
#if defined(WIN32)
    bool GetSimTypes( const TCHAR* pFilename, HMODULE repDll );
    bool GetReportInstantiator( const TCHAR* pFilename, HMODULE repDll, Kernel::instantiator_function_t* pRIF );
    bool GetType( const TCHAR* pFilename, HMODULE repDll, std::string& rClassName );
#else
    bool GetSimTypes( const char* pFilename, void* repDll );
    bool GetReportInstantiator( const char* pFilename, void* repDll, Kernel::instantiator_function_t* pRIF );
    bool GetType( const char* pFilename, void* repDll, std::string& rClassName );
#endif
    
    bool MatchSimType(char* simTypes[]);
    bool IsValidVersion(const char* emodVer);
    void LogSimTypes(char* simTypes[]);
    void ReadEmodulesJson( const std::string& key, std::list< emodulewstr > &dll_dirs );
    std::map< std::string, getSchema> getSchemaFuncPtrMap;
    std::map< std::string, std::string > dll2VersionStringMap;

private:
 
    char* m_sSimType;
    char* m_sSimTypeAll[SIMTYPES_MAXNUM];
};
