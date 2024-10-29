#include "stdafx.h"

#include <iostream>
#include <sstream>
#include <string.h>
#include "CajunIncludes.h"
#include "ProgVersion.h"
#include "JsonObject.h"
#include "Exceptions.h"
#include <iomanip>


// See the definition of version info
#include "version_info.h"
#include "Log.h"

SETUP_LOGGING( "ProgVersion" )

// for unsafe usage of sprintf
#pragma warning(disable: 4996)

ProgDllVersion::ProgDllVersion()
{
    m_nMajor = MAJOR_VERSION;
    m_nMinor = MINOR_VERSION;
    m_nRevision = REVISION_NUMBER;

    m_nSerPopMajor = SER_POP_MAJOR_VERSION;
    m_nSerPopMinor = SER_POP_MINOR_VERSION;
    m_nSerPopPatch = SER_POP_PATCH_VERSION;

    m_nBuild = BUILD_NUMBER;
    strncpy( m_sBuildDate, BUILD_DATE, VER_LEN );
    strncpy( m_builderName, BUILDER_NAME, VER_LEN );
    strncpy( m_sSccsBranch, SCCS_BRANCH, VER_LEN );
    strncpy( m_sSccsDate, SCCS_DATE, VER_LEN );
    for( int i=0; i<VER_LEN; i++ )
    {
        if( m_sSccsDate[i] == '_' )
            m_sSccsDate[i] = ' ';
    }

    updateVersionString();
}


ProgDllVersion::ProgDllVersion( Kernel::IJsonObjectAdapter* emod_info )
{
    m_nMajor = emod_info->GetUint( "emod_major_version" );
    m_nMinor = emod_info->GetUint( "emod_minor_version" );
    m_nRevision = emod_info->GetUint( "emod_revision_number" );

    m_nSerPopMajor = emod_info->GetUint( "ser_pop_major_version" );
    m_nSerPopMinor = emod_info->GetUint( "ser_pop_minor_version" );
    m_nSerPopPatch = emod_info->GetUint( "ser_pop_patch_version" );

    std::string build_date = emod_info->GetString( "emod_build_date" );
    std::string builder_name = emod_info->GetString( "emod_builder_name" );
    std::string sccs_branch = emod_info->GetString( "emod_sccs_branch" );
    std::string sccs_date = emod_info->GetString( "emod_sccs_date" );

    build_date.copy( m_sBuildDate, build_date.length() );
    m_sBuildDate[build_date.length()] = '\0';
    builder_name.copy( m_builderName, builder_name.length() );
    m_builderName[builder_name.length()] = '\0';
    sccs_branch.copy( m_sSccsBranch, sccs_branch.length() );
    m_sSccsBranch[sccs_branch.length()] = '\0';
    sccs_date.copy( m_sSccsDate, sccs_date.length() );
    m_sSccsDate[sccs_date.length()] = '\0';

    m_nBuild = emod_info->GetUint( "emod_build_number" );
    for(int i = 0; i < VER_LEN; i++)
    {
        if(m_sSccsDate[i] == '_')
            m_sSccsDate[i] = ' ';
    }

    updateVersionString();
}

ProgDllVersion::ProgDllVersion( json::Object& emod_info )
{
    json::QuickInterpreter em_info = emod_info;

    m_nMajor = em_info["emod_major_version"].As<json::Uint64>();
    m_nMinor = em_info["emod_minor_version"].As<json::Uint64>();
    m_nRevision = em_info["emod_revision_number"].As<json::Uint64>();

    m_nSerPopMajor = em_info["ser_pop_major_version"].As<json::Uint64>();
    m_nSerPopMinor = em_info["ser_pop_minor_version"].As<json::Uint64>();
    m_nSerPopPatch = em_info["ser_pop_patch_version"].As<json::Uint64>();

    std::string build_date = em_info["emod_build_date"].As<json::String>();
    std::string builder_name = em_info["emod_builder_name"].As<json::String>();
    std::string sccs_branch = em_info["emod_sccs_branch"].As<json::String>();
    std::string sccs_date = em_info["emod_sccs_date"].As<json::String>();
    
    build_date.copy( m_sBuildDate, build_date.length() );
    m_sBuildDate[build_date.length()] = '\0';
    builder_name.copy( m_builderName, builder_name.length() );
    m_builderName[builder_name.length()] = '\0';
    sccs_branch.copy( m_sSccsBranch, sccs_branch.length() );
    m_sSccsBranch[sccs_branch.length()] = '\0';
    sccs_date.copy( m_sSccsDate, sccs_date.length() );
    m_sSccsDate[sccs_date.length()] = '\0';

    m_nBuild = em_info["emod_build_number"].As<json::Uint64>();
    for(int i = 0; i < VER_LEN; i++)
    {
        if(m_sSccsDate[i] == '_')
            m_sSccsDate[i] = ' ';
    }

    updateVersionString();
}


void ProgDllVersion::updateVersionString()
{
    // ----------------------------------------------------------------------------------
    // --- DanB - commenting out the revision and build number because we aren't really
    // --- setting them right now.  The revision used to be the number of commits in the
    // --- branch, but now on Jenkins, we only get the code for the last revision.  This
    // --- means it is always "1".  I don't know what happened to build number it appears
    // --- to have been zero for a while.
    // ----------------------------------------------------------------------------------
    //sprintf( m_sVersion, "%Iu.%Iu.%Iu.%Iu", m_nMajor, m_nMinor, m_nRevision, m_nBuild );

    sprintf( m_sVersion, "%Iu.%Iu", m_nMajor, m_nMinor );
}

std::string ProgDllVersion::getSerPopVersion() const
{
    std::stringstream version;
    version << m_nSerPopMajor << "." << m_nSerPopMinor << "." << m_nSerPopPatch;
    return version.str();
}

std::string ProgDllVersion::getVersionComparisonString( const ProgDllVersion& pv ) const
{
    std::stringstream version_comparisson;
    version_comparisson << std::setw( 60 )   << "EMOD used to create population"      << std::setw( 50 ) << "This version of EMOD" << std::endl
        << "Emod version:"       << "\t\t" << std::setw( 40 ) << getVersion()       << std::setw( 50 ) << pv.getVersion()        << std::endl
        << "Build date:"         << "\t\t" << std::setw( 40 ) << getBuildDate()     << std::setw( 50 ) << pv.getBuildDate()      << std::endl
        << "Builder name:"       << "\t\t" << std::setw( 40 ) << getBuilderName()   << std::setw( 50 ) << pv.getBuilderName()    << std::endl
        << "Serial Pop version:" << "\t"   << std::setw( 40 ) << getSerPopVersion() << std::setw( 50 ) << pv.getSerPopVersion()  << std::endl
        << "Branch (commit):"    << "\t"   << std::setw( 40 ) << getSccsBranch()    << std::setw( 50 ) << pv.getSccsBranch()     << std::endl
        << "Commit Date:"        << "\t\t" << std::setw( 40 ) << getSccsDate()      << std::setw( 50 ) << pv.getSccsDate()       << std::endl;
    return version_comparisson.str();
}


int ProgDllVersion::checkProgVersion(uint8_t nMajor, uint8_t nMinor, uint16_t nRevision) const
{
    int ret = 0;
    
    uint32_t num = ProgDllVersion::combineVersion( nMajor, nMinor, nRevision );
    uint32_t nVersion = ProgDllVersion::combineVersion( uint8_t( m_nMajor ), uint8_t( m_nMinor ), uint16_t( m_nRevision ) );

    if (num < nVersion) 
        ret = -1;
    else if (num > nVersion) 
        ret = 1;
     return ret;
}

int ProgDllVersion::checkProgVersion(const char* sVersion) const
{
    // -2 for error
    int ret = -2;
    uint8_t maj, min;
    uint16_t rev;
    if (parseProgVersion(sVersion, maj, min, rev))
    {
        ret = checkProgVersion(maj, min, rev);
    }
    return ret;
}

uint32_t ProgDllVersion::combineVersion( uint8_t nMajor, uint8_t nMinor, uint16_t nRevision )
{
    return (((nMajor) << 24) | ((nMinor) << 16) | (nRevision) << 8);
}

std::string ProgDllVersion::toString() const
{
    std::stringstream os;
    json::Writer::Write( toJson(), os );
    return os.str();
}

json::Object ProgDllVersion::toJson() const
{
    json::Object temp_stream;
    temp_stream["emod_major_version"] = json::Uint64( m_nMajor );
    temp_stream["emod_minor_version"] = json::Uint64( m_nMinor );
    temp_stream["emod_revision_number"] = json::Uint64( m_nRevision );

    temp_stream["ser_pop_major_version"] = json::Uint64( m_nSerPopMajor );
    temp_stream["ser_pop_minor_version"] = json::Uint64( m_nSerPopMinor );
    temp_stream["ser_pop_patch_version"] = json::Uint64( m_nSerPopPatch );

    temp_stream["emod_build_date"] = json::String( m_sBuildDate );
    temp_stream["emod_builder_name"] = json::String( m_builderName );
    temp_stream["emod_build_number"] = json::Uint64( m_nBuild );
    temp_stream["emod_sccs_branch"] = json::String( m_sSccsBranch );
    temp_stream["emod_sccs_date"] = json::String( m_sSccsDate );

    return temp_stream;
}

void ProgDllVersion::checkSerializationVersion( const char* const& filename )
{
    /*
     * Population saved with interface version     Loading with interface versionIs    loaded
     * 1.1.1                                       1.1.1                               Yes
     * 1.1.1                                       1.2.1                               Yes
     * 1.1.1                                       2.1.1                               No
     * 1.2.1                                       1.1.1                               No
     * 2.1.1                                       1.1.1                               Yes
     *
     * Increased major version means compatibility with newer interface versions is broken.
     * Increased minor version means compatibility with older interface versions is broken.
     */
    
    ProgDllVersion code_version;
    ProgDllVersion* file_version = this;

    if( file_version->getSerPopMajorVersion() < code_version.getSerPopMajorVersion() ||
        file_version->getSerPopMinorVersion() > code_version.getSerPopMinorVersion())
    {
        ostringstream msg;
        msg << "ERROR while reading serialized file '" << filename <<  "' - incompatible version comparison." << std::endl
            << std::endl << getVersionComparisonString( code_version ).c_str() << std::endl;        
        throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }
}

bool ProgDllVersion::parseProgVersion(const char* sVersion, uint8_t& maj, uint8_t& min, uint16_t& rev) const
{
    maj = 0;
    min = 0;
    rev = 0;
    
    char sTemp[VER_LEN];
    strncpy(sTemp, sVersion, VER_LEN);
    char* str = strchr(sTemp, '.');
    if (!str) return false;
    
    str[0] = '\0';
    maj = atoi(sTemp);
    
    char* str1 = strchr(str+1, '.');
    if (!str1) return false;
    
    str1[0] = '\0';
    min = atoi(str+1);
    
    char* str2 = strchr(str1+1, '.');
    if (!str2) return false;
    
    str2[0] = '\0';
    rev = atoi(str1+1);
    
    return true;
}

ProgDllVersion ProgDllVersion::getEmodInfoVersion4()
{
    json::Object emod_info_json;
    emod_info_json["emod_major_version"] = json::Uint64( 0 );
    emod_info_json["emod_minor_version"] = json::Uint64( 0 );
    emod_info_json["emod_revision_number"] = json::Uint64( 0 );

    emod_info_json["ser_pop_major_version"] = json::Uint64( 1 );
    emod_info_json["ser_pop_minor_version"] = json::Uint64( 0 );
    emod_info_json["ser_pop_patch_version"] = json::Uint64( 0 );

    emod_info_json["emod_build_date"] = json::String( "unknown" );
    emod_info_json["emod_builder_name"] = json::String( "unknown" );
    emod_info_json["emod_build_number"] = json::Uint64( 0 );
    emod_info_json["emod_sccs_branch"] = json::String( "unknown" );
    emod_info_json["emod_sccs_date"] = json::String( "unknown" );

    return ProgDllVersion( emod_info_json );
}



#pragma warning (default: 4996)
