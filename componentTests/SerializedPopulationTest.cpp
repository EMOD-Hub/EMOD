
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "FileSystem.h"
#include "SerializedPopulation.h"
#include <iostream>
#include "ProgVersion.h"
#include "RapidJsonImpl.h"
#include "version_info.h"
#include <iomanip>


using namespace Kernel;

// Need lambda function because isspace is overloaded
auto is_a_space = [](const unsigned char c) 
{
    return std::isspace(c);
};

SUITE( SerializedPopulationTest )
{   
    TEST( TestHeader_Version5_ToString )
    {
        try
        {
            // Create Header
            SerializedState::Header header;
            header.version = 5;
            header.date = "Tue Aug 30 19:28:49 2022";
            header.compressed = false;;
            header.byte_count = 123456789;
            header.compression = "NONE";
            header.chunk_count = 987654321;             // actually 3, testing uint
            header.chunk_sizes = { 1, 234567891, 3 };   

            // compare generated header, remove spaces before comparing strings
            std::ostringstream json_header_temp;
            json_header_temp    << "{                                                                           "
                                << "    \"version\": 5,                                                         "
                                << "    \"author\" : \"IDM\",                                                   "
                                << "    \"tool\" : \"DTK\",                                                     "
                                << "    \"date\" : \"Tue Aug 30 19:28:49 2022\",                                "
                                << "    \"compression\" : \"NONE\",                                             "
                                << "    \"emod_info\" :                                                         "
                                << "    {                                                                       "
                                << "        \"emod_major_version\": 2,                                          "
                                << "        \"emod_minor_version\" : 3,                                         "
                                << "        \"emod_revision_number\" : 4,                                       "
                                << "        \"ser_pop_major_version\" : 5,                                      "
                                << "        \"ser_pop_minor_version\" : 6,                                      "
                                << "        \"ser_pop_patch_version\" : 7,                                      "
                                << "        \"emod_build_date\" : \"Wed Aug 31 19:28:49 2022\",                 "
                                << "        \"emod_builder_name\" : \"me\",                                     "
                                << "        \"emod_build_number\" : 0,                                          "
                                << "        \"emod_sccs_branch\" : \"serialization_improvements (1c3fc702f7)\", "
                                << "        \"emod_sccs_date\" : \"2022-07-15 06:46:28 -0700\"                  "
                                << "    },                                                                      "
                                << "    \"bytecount\" : 123456789,                                              "
                                << "    \"chunkcount\" : 987654321,                                             "
                                << "    \"chunksizes\": [1, 234567891, 3]                                       "
                                << "}                                                                           ";

            std::string expected_json_header = json_header_temp.str();
            expected_json_header.erase( std::remove_if( expected_json_header.begin(), expected_json_header.end(), is_a_space ), expected_json_header.end() );

            Kernel::RapidJsonObj rapjo;
            rapjo.Parse( json_header_temp.str().c_str() );
            header.emod_info = ProgDllVersion( rapjo.GetJsonObject( "emod_info" ) );

            std::string actual_header = header.ToString();
            actual_header.erase( std::remove_if( actual_header.begin(), actual_header.end(), is_a_space ), actual_header.end() );

            CHECK_EQUAL( expected_json_header, actual_header );
        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    };

    TEST( TestHeader_Version5_ReadHeader )
    {
        // test if header stays the same after writing and reading it
        try
        {
            // Create header, save header, read file into new header
            SerializedState::Header header_expected( "NONE", {1, 2, 3} );

            const char* filename = "TestHeader_Version5_ReadHeader.json";
            FILE* file_write = SerializedState::OpenFileForWriting( filename );
            string size_string;
            ConstructHeaderSize( header_expected, size_string );
            SerializedState::WriteHeaderSize( size_string, file_write );
            WriteHeader( header_expected, file_write );
            fflush( file_write );
            fclose( file_write );

            SerializedState::Header header_actual;
            FILE* file_read = SerializedState::OpenFileForReading( filename );
            SerializedState::ReadHeader( file_read, filename, header_actual );
            fclose( file_read );

            CHECK_EQUAL( header_expected.ToString(), header_actual.ToString() );

            CHECK( FileSystem::RemoveFile( std::string( filename ) ) );
        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    };

    TEST( TestHeader_Version5_getVersionComparisonString )
    {
        // test if error message contains all data
        try
        {
            json::Object emod_info_json;
            emod_info_json["emod_major_version"] = json::Uint64( 1234 );
            emod_info_json["emod_minor_version"] = json::Uint64( 2345 );
            emod_info_json["emod_revision_number"] = json::Uint64( 3456 );

            emod_info_json["ser_pop_major_version"] = json::Uint64( 4567 );
            emod_info_json["ser_pop_minor_version"] = json::Uint64( 5678 );
            emod_info_json["ser_pop_patch_version"] = json::Uint64( 6789 );

            emod_info_json["emod_build_date"] = json::String( "dummy_date" );
            emod_info_json["emod_builder_name"] = json::String( "dummy_name" );
            emod_info_json["emod_build_number"] = json::Uint64( 7890 );
            emod_info_json["emod_sccs_branch"] = json::String( "dummy_branch" );
            emod_info_json["emod_sccs_date"] = json::String( "dummy sccs" );

            ProgDllVersion emod_info_ser_pop( emod_info_json );
            ProgDllVersion emod_info_this;
            json::QuickInterpreter em_info = emod_info_json;

            std::stringstream emod_version;
            emod_version << em_info[ "emod_major_version" ].As<json::Uint64>()
                  << "." << em_info[ "emod_minor_version" ].As<json::Uint64>();
                  // -----------------------------------------------------
                  // --- DanB - 10/28/2024 - Part of removing revision and
                  // --- build number because they are always 1 and 0.
                  // -----------------------------------------------------
                  //<< "." << em_info["emod_revision_number"].As<json::Uint64>()
                  //<< "." << em_info["emod_build_number"].As<json::Uint64>();

            std::stringstream ser_pop_version;
            ser_pop_version << em_info["ser_pop_major_version"].As<json::Uint64>() << "."
                            << em_info["ser_pop_minor_version"].As<json::Uint64>() << "."
                            << em_info["ser_pop_patch_version"].As<json::Uint64>();
                                    
            string error_msg = emod_info_ser_pop.getVersionComparisonString( emod_info_this );

            std::vector<std::string> expected_messages{ "EMOD used to create population",
                "This version of EMOD",
                "Emod version:",
                "Build date:",
                "Builder name:",
                "Serial Pop version:",
                "Branch (commit):",
                "Commit Date:",
                emod_info_this.getVersion(),
                emod_version.str(),
                emod_info_this.getBuildDate(),
                em_info["emod_build_date"].As<json::String>(),
                emod_info_this.getBuilderName(),
                em_info["emod_builder_name"].As<json::String>(),
                emod_info_this.getSerPopVersion(),
                ser_pop_version.str(),
                emod_info_this.getSccsBranch(),
                em_info["emod_sccs_branch"].As<json::String>(),
                emod_info_this.getSccsDate(),
                em_info["emod_sccs_date"].As<json::String>()
            };

            for( std::string& expected_message : expected_messages )
            {
                int found_pos = error_msg.find( expected_message );
                CHECK( found_pos != std::string::npos );
            }
        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    };



    struct HeaderSetup {
        // Check exception for wrong major version
        json::Object emod_info_json;
        ProgDllVersion emod_info;
        HeaderSetup() {
            emod_info_json["emod_major_version"] = json::Uint64( 2 );
            emod_info_json["emod_minor_version"] = json::Uint64( 2 );
            emod_info_json["emod_revision_number"] = json::Uint64( 2 );

            emod_info_json["ser_pop_major_version"] = json::Uint64( emod_info.getSerPopMajorVersion() );
            emod_info_json["ser_pop_minor_version"] = json::Uint64( emod_info.getSerPopMinorVersion() );
            emod_info_json["ser_pop_patch_version"] = json::Uint64( emod_info.getSerPopPatchVersion() );

            emod_info_json["emod_build_date"] = json::String( "dummy_emod_build_date" );
            emod_info_json["emod_builder_name"] = json::String( "dummy_build_name" );
            emod_info_json["emod_sccs_branch"] = json::String( "dummy_sccs_branch (xyz123)" );
            emod_info_json["emod_sccs_date"] = json::String( "dummy_sccs_date" );
            emod_info_json["emod_build_number"] = json::Uint64( 123 );
        }
    };

    /*
     * Population saved with interface version     Loading with interface versionIs    loaded
     * 1.1.1                                       1.1.1                               Yes
     * 1.1.1                                       1.2.1                               Yes
     * 1.1.1                                       2.1.1                               No (Exception)
     * 1.2.1                                       1.1.1                               No (Exception)
     * 2.1.1                                       1.1.1                               Yes
     *
     * Increased major version means compatibility with newer interface versions is broken.
     * Increased minor version means compatibility with older interface versions is broken.
     */


    TEST_FIXTURE( HeaderSetup, TestHeader_Version5_ErrorMessage_LoadSerializedSimulation )
    {
        // test if LoadSerializedSimulation() forwards the error msg from getVersionComparisonString();
        try
        {           
            json::QuickInterpreter em_info = emod_info_json;
            assert(em_info["ser_pop_major_version"].As<json::Uint64>() >= 1);
            emod_info_json["ser_pop_major_version"] = json::Uint64( 0 ); // trigger version comparison in checkSerializationVersion()

            SerializedState::Header header( "NONE", { 1, 2, 3 } );
            header.emod_info = ProgDllVersion( emod_info_json );

            const char* filename = "TestHeader_Version5_ErrorMessage_LoadSerializedSimulation.json";
            FILE* file_write = SerializedState::OpenFileForWriting( filename );
            SerializedState::WriteMagicNumber( file_write );
            string size_string;
            SerializedState::ConstructHeaderSize( header, size_string );
            SerializedState::WriteHeaderSize( size_string, file_write );
            SerializedState::WriteHeader( header, file_write );
            fflush( file_write );
            fclose( file_write );

            try
            {
                Kernel::ISimulation* sim = SerializedState::LoadSerializedSimulation( filename );
                CHECK( false );
            }
            catch( Kernel::SerializationException& actual_ex )
            {
                ProgDllVersion emod_info_this;
                ProgDllVersion emod_info_ser_pop( emod_info_json );
                std::string expected_message = emod_info_ser_pop.getVersionComparisonString( emod_info_this );

                size_t found_pos = std::string( actual_ex.GetMsg() ).find( expected_message );
                CHECK( found_pos != std::string::npos );

                CHECK( FileSystem::RemoveFile( std::string( filename ) ) );
            }
        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    };

    TEST_FIXTURE( HeaderSetup, TestHeader_Version5_same_version )
    {
        // Test same major and minor version
        ProgDllVersion emod_info_same_version( emod_info_json );
        try
        {
            emod_info_same_version.checkSerializationVersion( "dummy_filename.json" );
            CHECK( true );
        }
        catch( Kernel::SerializationException& ser_ex )
        {
            PrintDebug( ser_ex.GetMsg() );
            CHECK( false );
        }
    };

    TEST_FIXTURE( HeaderSetup, TestHeader_Version5_CheckHeader_smaller_minor_version )
    {
        // The minor version of the loaded pop is greater than the version emod supports
        emod_info_json["ser_pop_minor_version"] = json::Uint64( emod_info.getSerPopMinorVersion() + 1 );
        ProgDllVersion emod_info_smaller_minor_version( emod_info_json );
        try
        {
            emod_info_smaller_minor_version.checkSerializationVersion( "dummy_filename.json" );
            CHECK( false );
        }
        catch( Kernel::SerializationException& actual_ex )
        {
            std::string expected_message = {"EMOD used to create population"};
            size_t found_pos = std::string( actual_ex.GetMsg() ).find( expected_message );
            CHECK( !(found_pos == std::string::npos) );
        }
    }

    TEST_FIXTURE( HeaderSetup, TestHeader_Version5_CheckHeader_smaller_major_version )
    {
        // The major version of the loaded pop is greater than the version emod supports
        emod_info_json["ser_pop_major_version"] = json::Uint64( emod_info.getSerPopMajorVersion() + 1 );
        ProgDllVersion emod_info_smaller_major_version( emod_info_json );        
        try
        {
            emod_info_smaller_major_version.checkSerializationVersion( "dummy_filename.json" );
            CHECK( true );
        }
        catch( Kernel::SerializationException& actual_ex )
        {
            PrintDebug( actual_ex.GetMsg() );
            CHECK( false );
        }
    }


    TEST_FIXTURE( HeaderSetup, TestHeader_Version5_CheckHeader_greater_minor_version )
    {
        // The minor version of the loaded pop is lower than the version emod supports
        emod_info_json["ser_pop_minor_version"] = json::Uint64( emod_info.getSerPopMinorVersion() - 1 );
        ProgDllVersion emod_info_greater_minor_version( emod_info_json );        
        try
        {
            emod_info_greater_minor_version.checkSerializationVersion( "dummy_filename.json" );
            CHECK( false );
        }
        catch( Kernel::SerializationException& actual_ex )
        {
            std::string expected_message = { "EMOD used to create population" };
            size_t found_pos = std::string( actual_ex.GetMsg() ).find( expected_message );
            CHECK( found_pos != std::string::npos );
        }
    }

    TEST_FIXTURE( HeaderSetup, TestHeader_Version5_CheckHeader_greater_major_version )
    {
        // The major version of the loaded pop is lower than the version emod supports
        emod_info_json["ser_pop_major_version"] = json::Uint64( emod_info.getSerPopMajorVersion() - 1 );
        ProgDllVersion emod_info_greater_major_version( emod_info_json );        
        try
        {
            emod_info_greater_major_version.checkSerializationVersion( "dummy_filename.json" );
            CHECK( false );
        }
        catch( Kernel::SerializationException& actual_ex )
        {
            std::string expected_message = {"EMOD used to create population"};
            size_t found_pos = std::string( actual_ex.GetMsg() ).find( expected_message );
            CHECK( found_pos != std::string::npos );
        }
    }

    TEST( ProgDllVersion_ctor_json )
    {
        try
        {
            ProgDllVersion emod_info_expected;

            // Check exception for wrong major version
            json::Object emod_info_json;
            emod_info_json["emod_major_version"] = json::Uint64( MAJOR_VERSION );
            emod_info_json["emod_minor_version"] = json::Uint64( MINOR_VERSION );
            emod_info_json["emod_revision_number"] = json::Uint64( REVISION_NUMBER );

            emod_info_json["ser_pop_major_version"] = json::Uint64( SER_POP_MAJOR_VERSION );
            emod_info_json["ser_pop_minor_version"] = json::Uint64( SER_POP_MINOR_VERSION );
            emod_info_json["ser_pop_patch_version"] = json::Uint64( SER_POP_PATCH_VERSION );

            emod_info_json["emod_build_date"] = json::String( emod_info_expected.getBuildDate() );
            emod_info_json["emod_builder_name"] = json::String( BUILDER_NAME );
            emod_info_json["emod_sccs_branch"] = json::String( SCCS_BRANCH );
            emod_info_json["emod_sccs_date"] = json::String( SCCS_DATE );
            emod_info_json["emod_build_number"] = json::Uint64( 0 );

            ProgDllVersion emod_info_from_json( emod_info_json );

            CHECK_EQUAL( emod_info_expected.toString(), emod_info_from_json.toString() );

        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST( ProgDllVersion_checkProgVersion )
    {
        try
        {
            json::Object emod_info_json;
            emod_info_json["emod_major_version"] = json::Uint64( 3 );
            emod_info_json["emod_minor_version"] = json::Uint64( 3 );
            emod_info_json["emod_revision_number"] = json::Uint64( 3 );

            emod_info_json["ser_pop_major_version"] = json::Uint64( 2 );
            emod_info_json["ser_pop_minor_version"] = json::Uint64( 2 );
            emod_info_json["ser_pop_patch_version"] = json::Uint64( 2 );

            emod_info_json["emod_build_date"] = json::String( "dummy_date" );
            emod_info_json["emod_builder_name"] = json::String( "dummy_name" );
            emod_info_json["emod_build_number"] = json::Uint64( 123 );
            emod_info_json["emod_sccs_branch"] = json::String( "dummy_branch");
            emod_info_json["emod_sccs_date"] = json::String( "dummy_sccs");

            ProgDllVersion emod_info_from_json( emod_info_json );

            //equal            
            uint8_t major = 3;
            uint8_t minor = 3;
            uint16_t revision = 3;   
            int ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( 0, ret );

            // prog version smaller than emod version
            major = 3;
            minor = 3;
            revision = 2;
            ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( -1, ret );

            major = 3;
            minor = 2;
            revision = 3;
            ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( -1, ret );

            major = 2;
            minor = 3;
            revision = 3;
            ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( -1, ret );

            // prog version greater than emod version
            major = 3;
            minor = 3;
            revision = 4;
            ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( 1, ret );

            major = 3;
            minor = 4;
            revision = 3;
            ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( 1, ret );

            major = 4;
            minor = 3;
            revision = 3;
            ret = emod_info_from_json.checkProgVersion( major, minor, revision );
            CHECK_EQUAL( 1, ret );
        }
        catch(DetailedException& detailed_ex)
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST( ProgDllVersion_ctor_JsonObjectAdaptor )
    {
        try
        {
            std::ostringstream emod_info_str;
            emod_info_str
                << "{                                                                       "
                << "    \"emod_major_version\": 1,                                          "
                << "    \"emod_minor_version\" : 2,                                         "
                << "    \"emod_revision_number\" : 3,                                       "
                << "    \"ser_pop_major_version\" : 12345,                                  "
                << "    \"ser_pop_minor_version\" : 23456,                                  "
                << "    \"ser_pop_patch_version\" : 3456,                                   "
                << "    \"emod_build_date\" : \"Wed Aug 31 19:28:49 2022\",                 "
                << "    \"emod_builder_name\" : \"me\",                                     "
                << "    \"emod_build_number\" : 0,                                          "
                << "    \"emod_sccs_branch\" : \"serialization_improvements (1c3fc702f7)\", "
                << "    \"emod_sccs_date\" : \"2022-07-15 06:46:28 -0700\"                  "
                << "}                                                                       ";

            std::string emod_info_expected = emod_info_str.str();
            emod_info_expected.erase( std::remove_if( emod_info_expected.begin(), emod_info_expected.end(), is_a_space ), emod_info_expected.end() );

            Kernel::RapidJsonObj rapjo;
            rapjo.Parse( emod_info_str.str().c_str() );
            ProgDllVersion emod_info( &rapjo );

            std::string emod_info_actual = emod_info.toString();
            emod_info_actual.erase( std::remove_if( emod_info_actual.begin(), emod_info_actual.end(), is_a_space ), emod_info_actual.end() );

            CHECK_EQUAL( emod_info_expected, emod_info_actual );

        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    }
    
    // header version 4
    TEST( TestHeader_Version4_ErrorMessage_major_LoadSerializedSimulation )
    {
        // test if a lower version of ser_pop_major_version in a version 4 header triggers an SerializationException
        // test only works if SER_POP_MAJOR_VERSION > version4_ser_pop_major_version
        try
        {
            SerializedState::Header header( "NONE", { 1, 2, 3 } );
            header.version = 4;
            header.emod_info = ProgDllVersion::getEmodInfoVersion4();

            const char* filename = "TestHeader_Version4_ErrorMessage_LoadSerializedSimulation.json";
            FILE* file_write = SerializedState::OpenFileForWriting( filename );
            SerializedState::WriteMagicNumber( file_write );
            string size_string;
            SerializedState::ConstructHeaderSize( header, size_string );
            SerializedState::WriteHeaderSize( size_string, file_write );
            SerializedState::WriteHeader( header, file_write );
            fflush( file_write );
            fclose( file_write );

            ProgDllVersion this_version;

            try
            {
                if( this_version.getSerPopMajorVersion() > 1 ) // testable only if current emod requires SPs with greater major version than standard header version 4
                {
                    Kernel::ISimulation* sim = SerializedState::LoadSerializedSimulation( filename );
                    CHECK( false );
                }
                CHECK( true );
                FileSystem::RemoveFile( filename );
            }
            catch( Kernel::SerializationException& actual_ex )
            {
                std::string expected_message = header.emod_info.getVersionComparisonString( this_version );
                size_t found_pos = std::string( actual_ex.GetMsg() ).find( expected_message );
                CHECK( found_pos != std::string::npos );
                FileSystem::RemoveFile( filename );
            }
        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST( TestHeader_SetEmodInfoVersion4 )
    {
        // test if LoadSerializedSimulation() forwards the error msg from getVersionComparisonString();
        try
        {
            SerializedState::Header header( "NONE", { 1, 2, 3 } );
            header.emod_info = ProgDllVersion::getEmodInfoVersion4();

            try
            {
                CHECK_EQUAL( header.emod_info.getSerPopMajorVersion(), 1 );
                CHECK_EQUAL( header.emod_info.getSerPopMinorVersion(), 0 );
                CHECK_EQUAL( header.emod_info.getSerPopPatchVersion(), 0 );
            }
            catch( Kernel::SerializationException& actual_ex )
            {
                PrintDebug( actual_ex.GetMsg() );
                CHECK( false );
            }
        }
        catch( DetailedException& detailed_ex )
        {
            PrintDebug( detailed_ex.GetMsg() );
            PrintDebug( detailed_ex.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST( Test_Header_throw_Exception_smaller_version_4 )
    {
        // create file with version 3 header 
        std::ostringstream json_header;
        json_header << "{                                    " << std::endl
            << "    \"version\": 3,                          " << std::endl
            << "    \"author\" : \"IDM\",                    " << std::endl
            << "    \"tool\" : \"DTK\",                      " << std::endl
            << "    \"date\" : \"Tue Aug 30 19:28:49 2022\", " << std::endl
            << "    \"engine\" : \"NONE\",                   " << std::endl
            << "    \"bytecount\" : 123456789,               " << std::endl
            << "    \"chunkcount\" : 3,                      " << std::endl
            << "    \"chunksizes\": [1, 2, 3]                " << std::endl
            << "}                                            " << std::endl;
        
        const char* filename = "Test_Header_throw_Exception_smaller_version_4.json";
        FILE* file = SerializedState::OpenFileForWriting( filename );
        SerializedState::WriteMagicNumber( file );
          
        std::string header_v3 = json_header.str();
        std::ostringstream temp;
        temp << setw( 12 ) << right << header_v3.size();
        SerializedState::WriteHeaderSize( temp.str(), file );

        fwrite( header_v3.c_str(), 1, header_v3.size(), file );
        fflush( file );
        fclose( file );

        // load created file
        try
        {
            Kernel::ISimulation* sim = SerializedState::LoadSerializedSimulation( filename );
            CHECK( false );
        }
        catch( Kernel::SerializationException& actual_ex )
        {
            PrintDebug( actual_ex.GetMsg() );
            std::string expected_message = { "The serialized population you are trying to load has a a header versions < 4. Serialized populations with a header version < 4 aren't supported anymore." };
            size_t found_pos = std::string( actual_ex.GetMsg() ).find( expected_message );
            CHECK( found_pos != std::string::npos );
        }   
        FileSystem::RemoveFile( filename );
    }
}
