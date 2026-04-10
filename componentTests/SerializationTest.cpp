
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "Environment.h"
#include "SimulationConfig.h"
#include "JsonFullReader.h"
#include "JsonFullWriter.h"
#include "BinaryArchiveReader.h"
#include "BinaryArchiveWriter.h"
#include "JsonObjectDemog.h"
#include "Properties.h"

#include "FileSystem.h"
#include "Individual.h"

#include "Diagnostics.h"
#include "RandomFake.h"
#include "MalariaDrugTypeParameters.h"
#include "VectorParameters.h"
#include "SusceptibilityMalaria.h"

using namespace Kernel; 

SUITE(SerializationTest)
{
    static int m_NextId = 1 ;

    struct SerializationFixture
    {
        SimulationConfig* m_pSimulationConfig ;

        SerializationFixture()
        :  m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger() );
            Environment::setSimulationConfig( m_pSimulationConfig );

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/SerializationTest/VSP.json" ) );
            m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
            m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();

            EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/SerializationTest/Drugs.json" );
            MalariaDrugTypeCollection::GetInstanceNonConst()->ConfigureFromJsonAndKey( EnvPtr->Config, "Malaria_Drug_Params" );
            MalariaDrugTypeCollection::GetInstanceNonConst()->CheckConfiguration();

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string,float> ip_values ;
            ip_values.insert( std::make_pair( "LOW",  0.9f ) );
            ip_values.insert( std::make_pair( "HIGH", 0.1f ) );

            IPFactory::GetInstance()->AddIP( 1, "Risk", ip_values );

            SusceptibilityMalariaConfig::falciparumMSPVars      = DEFAULT_MSP_VARIANTS;
            SusceptibilityMalariaConfig::falciparumNonSpecTypes = DEFAULT_NONSPECIFIC_TYPES;
            SusceptibilityMalariaConfig::falciparumPfEMP1Vars   = DEFAULT_PFEMP1_VARIANTS;
        }

        ~SerializationFixture()
        {
            MalariaDrugTypeCollection::DeleteInstance();
            delete m_pSimulationConfig;
            m_pSimulationConfig = nullptr;
            IPFactory::DeleteFactory();
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(SerializationFixture, TestSerializeDeserialize)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        EventTriggerFactory::DeleteInstance();

        json::Object fakeConfigJson;
        Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
        EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );

        try
        {
            JsonObjectDemog json ;
            json.ParseFile( "testdata/SerializationTest/SerializationTest.json" );
            CHECK( json.Contains( "Custom_Individual_Events" ) );
            CHECK( json["Custom_Individual_Events"].IsArray() );

            for( int i = 0 ; i < json["Custom_Individual_Events"].size() ; i++ )
            {
                EventTriggerFactory::GetInstance()->CreateUserEventTrigger( json["Custom_Individual_Events"][i].AsString() );
            }

            std::map<std::string, float> ip_values_state ;
            CHECK( json.Contains( "Valid_Intervention_Statuses" ) );
            CHECK( json["Valid_Intervention_Statuses"].IsArray() );
            for( int i = 0 ; i < json["Valid_Intervention_Statuses"].size() ; i++ )
            {
                float percent_dist = 0.0;
                if( (i + 1) == json[ "Valid_Intervention_Statuses" ].size() )
                {
                    percent_dist = 1.0;
                }
                ip_values_state.insert( std::make_pair( json[ "Valid_Intervention_Statuses" ][ i ].AsString(), percent_dist ) );
            }
            IPFactory::GetInstance()->AddIP( 1, "InterventionStatus", ip_values_state );

            CHECK( json.Contains( "Objects" ) );
            CHECK( json["Objects"].IsArray() );

            for( int i = 0 ; i < json["Objects"].size() ; i++ )
            {
                string class_name = json["Objects"][i]["Test"]["__class__"].AsString();
                if (class_name == "SusceptibilityMalaria")
                {
                    // This class has a pointer to an EventTriggerFactory which is not serialized and will cause the test to fail.
                    bool here = true;
                }
                printf("testing  %s\n",class_name.c_str());

                string expected_json_str = json["Objects"][i].ToString();

                ISerializable* p_read_obj = nullptr;
                JsonFullReader json_reader( expected_json_str.c_str() );
                IArchive* p_json_reader = &json_reader;
                p_json_reader->labelElement("Test") & p_read_obj;

                BinaryArchiveWriter binary_writer;
                IArchive* p_binary_writer = &binary_writer;
                p_binary_writer->labelElement("Test") & p_read_obj;

                ISerializable* p_read_obj_2 = nullptr;
                BinaryArchiveReader binary_reader( p_binary_writer->GetBuffer(), p_binary_writer->GetBufferSize() );
                IArchive* p_binary_reader = &binary_reader;
                p_binary_reader->labelElement("Test") & p_read_obj_2;

                JsonFullWriter json_writer;
                IArchive* p_json_writer = &json_writer;
                p_json_writer->labelElement("Test") & p_read_obj_2;

                std::string actual_json_str = p_json_writer->GetBuffer();

                CHECK_EQUAL( expected_json_str, actual_json_str );
            }
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            PrintDebug( re.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(SerializationFixture, TestGenericSerialization)
    {
        // Open and read JSON
        std::string* json = FileSystem::ReadFile( "testdata/SerializationTest/genericIndividual.json" );

        // Instantiate from JSON
        IndividualHuman* individual = nullptr;
        ISerializable* source = (ISerializable*)individual;
        IArchive* reader = dynamic_cast<IArchive*>(new JsonFullReader( json->c_str() ));
        (*reader).labelElement("individual") & source;

        // Serialize to binary
        IArchive* binary_writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
        (*binary_writer) & source;

        // Deserialize from binary
        const char* buffer = binary_writer->GetBuffer();
        size_t count = binary_writer->GetBufferSize();
        IArchive* binary_reader = dynamic_cast<IArchive*>(new BinaryArchiveReader( buffer, count ));
        ISerializable* destination = nullptr;
        (*binary_reader) & destination;
        IndividualHuman* compare = (IndividualHuman*)destination;

        // Compare
        IArchive* writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
        (*writer) & compare;

        const char* actual = writer->GetBuffer();
        size_t actual_count = writer->GetBufferSize();

        CHECK_EQUAL( count, actual_count );
        CHECK_ARRAY_EQUAL( buffer, actual, count );

        delete writer;

        compare = nullptr;
        delete destination;
        delete binary_reader;
        delete source;
        individual = nullptr;
        delete binary_writer;
        delete reader;
        delete json;
    }
}
