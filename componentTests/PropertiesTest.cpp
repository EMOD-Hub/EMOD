
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "INodeContextFake.h"

#include "Properties.h"
#include "Environment.h"
#include "Log.h"
#include "SimulationConfig.h"
#include "NodeDemographics.h"
#include "FileSystem.h"
#include "IdmMpi.h"

using namespace std; 
using namespace Kernel; 

SUITE(PropertiesTest)
{
    typedef boost::bimap<uint32_t, suids::suid> nodeid_suid_map_t;
    typedef nodeid_suid_map_t::value_type nodeid_suid_pair;

    struct PropertiesTestFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* pSimConfig ;
        suids::suid next_suid;

        PropertiesTestFixture()
            : pSimConfig(nullptr)
            , next_suid(suids::nil_suid())
        {
            next_suid.data++;

            Environment::setLogger(new SimpleLogger());
            int argc      = 1;
            char* exeName = "componentTests.exe";
            char** argv   = &exeName;
            string configFilename("testdata/PropertiesTest/config.json");
            string inputPath(".;testdata/PropertiesTest");
            string outputPath("testdata/PropertiesTest/output");
            string statePath("testdata/PropertiesTest");
            string dllPath("testdata/PropertiesTest");

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);

            pSimConfig = SimulationConfigFactory::CreateInstance(Environment::getInstance()->Config);
            if (pSimConfig)
            {
                Environment::setSimulationConfig(pSimConfig);
            }
            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();
        }

        ~PropertiesTestFixture()
        {
            IPFactory::DeleteFactory();
            delete pSimConfig ;
            Environment::Finalize();
            NodeDemographicsFactory::SetDemographicsFileList( std::vector<std::string>() ) ;
        }

        suids::suid GetNextNodeSuid()
        {
            suids::suid next = next_suid;
            next_suid.data++;
            return next;
        }

        void TestReadingError( bool readTwo, int lineNumber, const std::string& rDemogFilename, const std::string& rExpMsg )
        {
            // --------------------
            // --- Initialize test
            // --------------------
            pSimConfig->demographics_initial = true ;
            NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet( rDemogFilename ) ) ;

            nodeid_suid_map_t node_id_suid_map;
            unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

            vector<uint32_t> nodeIDs = factory->GetNodeIDs();
            for (uint32_t node_id : nodeIDs)
            {
                suids::suid node_suid = GetNextNodeSuid();
                node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            INodeContext* nodeContext_1 = new INodeContextFake( 1 );
            unique_ptr<NodeDemographics> demographics_1( factory->CreateNodeDemographics( nodeContext_1 ) );

            try
            {
                IPFactory::GetInstance()->Initialize( 1, demographics_1->GetJsonObject() );
                if( readTwo )
                {
                    INodeContext* nodeContext_2 = new INodeContextFake( 2 );
                    unique_ptr<NodeDemographics> demographics_2( factory->CreateNodeDemographics( nodeContext_2 ) );

                    IPFactory::GetInstance()->Initialize( 2, demographics_2->GetJsonObject() );
                }
                CHECK_LN( false, lineNumber );
            }
            catch( DetailedException& e )
            {
                std::string msg = e.GetMsg();
                bool passed = msg.find( rExpMsg ) != std::string::npos;
                if( !passed )
                {
                    PrintDebug( rExpMsg );
                    PrintDebug( msg );
                }
                CHECK_LN( passed, lineNumber );
            }
        }

        void ReplaceCarriageReturn( std::string* p_str )
        {
            while( p_str->find( "\r" ) != string::npos )
            {
                p_str->erase( p_str->find( "\r" ), 1 );
            }
        }
    };

    TEST_FIXTURE(PropertiesTestFixture, TestRead)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        try
        {
            INodeContext* nodeContext = new INodeContextFake();
            unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

            IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

            std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
            CHECK_EQUAL( 2, ip_list.size() );
            CHECK_EQUAL( std::string("Accessibility"), ip_list[0]->GetKeyAsString() );
            CHECK_EQUAL( std::string("Risk"         ), ip_list[1]->GetKeyAsString() );

            IPKeyValueContainer ip_values_1 = ip_list[0]->GetValues<IPKeyValueContainer>();
            CHECK_EQUAL( 2, ip_values_1.Size() );
            CHECK( ip_values_1.Contains( "Accessibility:VaccineTake" ) );
            CHECK( ip_values_1.Contains( "Accessibility:VaccineRefuse" ) );

            IPKeyValueIterator it = ip_values_1.begin();
            CHECK_CLOSE( 0.85, (*it).GetInitialDistribution( 1 ), 0.001 );
            ++it;
            CHECK_CLOSE( 0.15, (*it).GetInitialDistribution( 1 ), 0.001 );
            ++it;
            CHECK( it == ip_values_1.end() );

            CHECK( ip_list[0]->GetIntraNodeTransmission( 1 ).HasMatrix() );
            CHECK_EQUAL( std::string("contact"), ip_list[0]->GetIntraNodeTransmission( 1 ).GetRouteName() );
            CHECK_EQUAL( 2,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix().size() );
            CHECK_EQUAL( 2,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0].size() );
            CHECK_EQUAL( 1.1f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0][0] );
            CHECK_EQUAL( 0.3f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0][1] );
            CHECK_EQUAL( 2,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1].size() );
            CHECK_EQUAL( 0.3f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1][0] );
            CHECK_EQUAL( 5.0f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1][1] );

            IPKeyValueContainer ip_values_2 = ip_list[1]->GetValues<IPKeyValueContainer>();
            CHECK_EQUAL( 3, ip_values_2.Size() );
            CHECK(  ip_values_2.Contains( "Risk:HIGH"   ) );
            CHECK(  ip_values_2.Contains( "Risk:MEDIUM" ) );
            CHECK(  ip_values_2.Contains( "Risk:LOW"    ) );
            CHECK( !ip_values_2.Contains( "Risk:XXX"    ) );

            CHECK( !ip_list[1]->GetIntraNodeTransmission( 1 ).HasMatrix() );
            CHECK_EQUAL( std::string("contact"), ip_list[0]->GetIntraNodeTransmission( 1 ).GetRouteName() );
            CHECK_EQUAL( 0, ip_list[1]->GetIntraNodeTransmission( 1 ).GetMatrix().size() );

            it = ip_values_2.begin();
            CHECK_CLOSE( 0.1, (*it).GetInitialDistribution( 1 ), 0.001 );
            ++it;
            CHECK_CLOSE( 0.4, (*it).GetInitialDistribution( 1 ), 0.001 );
            ++it;
            CHECK_CLOSE( 0.5, (*it).GetInitialDistribution( 1 ), 0.001 );
            ++it;
            CHECK( it == ip_values_2.end() );
        }
        catch( DetailedException& e )
        {
            PrintDebug( e.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(PropertiesTestFixture, TestReadAgeBins)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        pSimConfig->demographics_initial = true ;
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_age_bins.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        try
        {
            INodeContext* nodeContext = new INodeContextFake();
            unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

            IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

            std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
            CHECK_EQUAL( 1, ip_list.size() );
            CHECK_EQUAL( std::string("Age_Bin"), ip_list[0]->GetKeyAsString() );

            IPKeyValueContainer ip_values_1 = ip_list[0]->GetValues<IPKeyValueContainer>();
            CHECK_EQUAL( 3, ip_values_1.Size() );
            CHECK( ip_values_1.Contains( "Age_Bin:Age_Bin_Property_From_0_To_5"    ) );
            CHECK( ip_values_1.Contains( "Age_Bin:Age_Bin_Property_From_5_To_13"   ) );
            CHECK( ip_values_1.Contains( "Age_Bin:Age_Bin_Property_From_13_To_125" ) );


            IPKeyValueIterator it = ip_values_1.begin();
            CHECK_EQUAL( "Age_Bin_Property_From_0_To_5", (*it).GetValueAsString() );
            ++it;
            CHECK_EQUAL( "Age_Bin_Property_From_5_To_13", (*it).GetValueAsString() );
            ++it;
            CHECK_EQUAL( "Age_Bin_Property_From_13_To_125", (*it).GetValueAsString() );
            ++it;
            CHECK( it == ip_values_1.end() );

            std::string container_str = ip_values_1.ToString();
            CHECK_EQUAL( "Age_Bin:Age_Bin_Property_From_0_To_5,Age_Bin:Age_Bin_Property_From_5_To_13,Age_Bin:Age_Bin_Property_From_13_To_125", container_str );

            std::string values_str = ip_values_1.GetValuesToString();
            CHECK_EQUAL( "Age_Bin_Property_From_0_To_5, Age_Bin_Property_From_5_To_13, Age_Bin_Property_From_13_To_125", values_str );

            std::set<std::string> value_set = ip_values_1.GetValuesToStringSet();
            auto value_set_it = value_set.begin();
            CHECK_EQUAL( "Age_Bin_Property_From_0_To_5"   , *value_set_it );
            ++value_set_it;
            CHECK_EQUAL( "Age_Bin_Property_From_13_To_125", *value_set_it );
            ++value_set_it;
            CHECK_EQUAL( "Age_Bin_Property_From_5_To_13", *value_set_it );
            ++value_set_it;
            CHECK( value_set_it == value_set.end() );

            CHECK( ip_list[0]->GetIntraNodeTransmission( 1 ).HasMatrix() );
            CHECK_EQUAL( std::string("contact"), ip_list[0]->GetIntraNodeTransmission( 1 ).GetRouteName() );
            CHECK_EQUAL( 3,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix().size() );
            CHECK_EQUAL( 3,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0].size() );
            CHECK_EQUAL( 1.4f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0][0] );
            CHECK_EQUAL( 1.0f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0][1] );
            CHECK_EQUAL( 1.0f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[0][2] );
            CHECK_EQUAL( 3,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1].size() );
            CHECK_EQUAL( 1.0f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1][0] );
            CHECK_EQUAL( 2.5f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1][1] );
            CHECK_EQUAL( 0.7f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[1][2] );
            CHECK_EQUAL( 3,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[2].size() );
            CHECK_EQUAL( 1.0f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[2][0] );
            CHECK_EQUAL( 0.7f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[2][1] );
            CHECK_EQUAL( 1.0f, ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix()[2][2] );

            std::string act_filename = "testdata/PropertiesTest/output/TestReadAgeBins-Transitions-Actual.json" ;
            std::string exp_filename = "testdata/PropertiesTest/output/TestReadAgeBins-Transitions-Expected.json" ;

            // clean up before testing begnins
            CHECK( FileSystem::FileExists( exp_filename ) );

            if( FileSystem::FileExists( act_filename ) )
            {
                FileSystem::RemoveFile( act_filename );
            }

            IPFactory::GetInstance()->WriteTransitionsFile();

            CHECK( FileSystem::FileExists( "testdata/PropertiesTest/output/transitions.json" ) );

            int err = std::rename( "testdata/PropertiesTest/output/transitions.json", act_filename.c_str() );
            CHECK_EQUAL(0,err);

            CHECK( FileSystem::FileExists( act_filename ) );

            unique_ptr<string> p_act_contents( FileSystem::ReadFile( act_filename.c_str() ) );
            unique_ptr<string> p_exp_contents( FileSystem::ReadFile( exp_filename.c_str() ) );

            ReplaceCarriageReturn( p_act_contents.get() );
            ReplaceCarriageReturn( p_exp_contents.get() );

            CHECK( *p_exp_contents == *p_act_contents );

            // clean up
            if( FileSystem::FileExists( act_filename ) )
            {
                FileSystem::RemoveFile( act_filename );
            }
        }
        catch( DetailedException& e )
        {
            PrintDebug( e.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(PropertiesTestFixture, TestReadTransitions)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        pSimConfig->demographics_initial = true ;
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_transitions.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        try
        {
            INodeContext* nodeContext = new INodeContextFake();
            unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

            IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

            std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
            CHECK_EQUAL( 1, ip_list.size() );
            CHECK_EQUAL( std::string("QualityOfCare"), ip_list[0]->GetKeyAsString() );

            IPKeyValueContainer ip_values = ip_list[0]->GetValues<IPKeyValueContainer>();
            CHECK_EQUAL( 3, ip_values.Size() );
            CHECK( ip_values.Contains( "QualityOfCare:Good" ) );
            CHECK( ip_values.Contains( "QualityOfCare:OK"   ) );
            CHECK( ip_values.Contains( "QualityOfCare:Bad"  ) );

            CHECK( !ip_list[0]->GetIntraNodeTransmission( 1 ).HasMatrix() );
            CHECK_EQUAL( std::string("contact"), ip_list[0]->GetIntraNodeTransmission( 1 ).GetRouteName() );
            CHECK_EQUAL( 0,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix().size() );

            std::string act_filename = "testdata/PropertiesTest/output/TestReadTransitions-Transitions-Actual.json" ;
            std::string exp_filename = "testdata/PropertiesTest/output/TestReadTransitions-Transitions-Expected.json" ;

            // clean up before testing begnins
            CHECK( FileSystem::FileExists( exp_filename ) );

            if( FileSystem::FileExists( act_filename ) )
            {
                FileSystem::RemoveFile( act_filename );
            }

            IPFactory::GetInstance()->WriteTransitionsFile();

            CHECK( FileSystem::FileExists( "testdata/PropertiesTest/output/transitions.json" ) );

            int err = std::rename( "testdata/PropertiesTest/output/transitions.json", act_filename.c_str() );
            CHECK_EQUAL(0,err);

            CHECK( FileSystem::FileExists( act_filename ) );

            unique_ptr<string> p_act_contents( FileSystem::ReadFile( act_filename.c_str() ) );
            unique_ptr<string> p_exp_contents( FileSystem::ReadFile( exp_filename.c_str() ) );

            ReplaceCarriageReturn( p_act_contents.get() );
            ReplaceCarriageReturn( p_exp_contents.get() );

            CHECK( *p_exp_contents == *p_act_contents );

            // clean up
            if( FileSystem::FileExists( act_filename ) )
            {
                FileSystem::RemoveFile( act_filename );
            }
        }
        catch( DetailedException& e )
        {
            PrintDebug( e.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(PropertiesTestFixture, TestReadTransitionsSchool)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        pSimConfig->demographics_initial = true ;
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_transitions_school.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        try
        {
            INodeContext* nodeContext = new INodeContextFake();
            unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

            IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

            std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
            CHECK_EQUAL( 1, ip_list.size() );
            CHECK_EQUAL( std::string("Place"), ip_list[0]->GetKeyAsString() );

            IPKeyValueContainer ip_values = ip_list[0]->GetValues<IPKeyValueContainer>();
            CHECK_EQUAL( 5, ip_values.Size() );
            CHECK( ip_values.Contains( "Place:Community" ) );
            CHECK( ip_values.Contains( "Place:School"   ) );
            CHECK( ip_values.Contains( "Place:Work"  ) );
            CHECK( ip_values.Contains( "Place:SchoolBreak_Interaction"  ) );
            CHECK( ip_values.Contains( "Place:SchoolBreak_NonInteraction"  ) );

            CHECK( ip_list[0]->GetIntraNodeTransmission( 1 ).HasMatrix() );
            CHECK_EQUAL( std::string("contact"), ip_list[0]->GetIntraNodeTransmission( 1 ).GetRouteName() );
            CHECK_EQUAL( 5,    ip_list[0]->GetIntraNodeTransmission( 1 ).GetMatrix().size() );

            std::string act_filename = "testdata/PropertiesTest/output/TestReadTransitionsSchool-Transitions-Actual.json" ;
            std::string exp_filename = "testdata/PropertiesTest/output/TestReadTransitionsSchool-Transitions-Expected.json" ;

            // clean up before testing begnins
            CHECK( FileSystem::FileExists( exp_filename ) );

            if( FileSystem::FileExists( act_filename ) )
            {
                FileSystem::RemoveFile( act_filename );
            }

            IPFactory::GetInstance()->WriteTransitionsFile();

            CHECK( FileSystem::FileExists( "testdata/PropertiesTest/output/transitions.json" ) );

            int err = std::rename( "testdata/PropertiesTest/output/transitions.json", act_filename.c_str() );
            CHECK_EQUAL(0,err);

            CHECK( FileSystem::FileExists( act_filename ) );

            unique_ptr<string> p_act_contents( FileSystem::ReadFile( act_filename.c_str() ) );
            unique_ptr<string> p_exp_contents( FileSystem::ReadFile( exp_filename.c_str() ) );

            ReplaceCarriageReturn( p_act_contents.get() );
            ReplaceCarriageReturn( p_exp_contents.get() );

            CHECK( *p_exp_contents == *p_act_contents );

            // clean up
            if( FileSystem::FileExists( act_filename ) )
            {
                FileSystem::RemoveFile( act_filename );
            }
        }
        catch( DetailedException& e )
        {
            PrintDebug( e.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE( PropertiesTestFixture, TestTwoNodes )
    {
        // --------------------
        // --- Initialize test
        // --------------------
        pSimConfig->demographics_initial = true ;
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet( "testdata/PropertiesTest/demographics_TestTwoNodes.json" ) ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory( &node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for( uint32_t node_id : nodeIDs )
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert( nodeid_suid_pair( node_id, node_suid ) );
        }

        INodeContext* nodeContext_1 = new INodeContextFake( 1 );
        unique_ptr<NodeDemographics> demographics_1( factory->CreateNodeDemographics( nodeContext_1 ) );

        INodeContext* nodeContext_2 = new INodeContextFake( 2 );
        unique_ptr<NodeDemographics> demographics_2( factory->CreateNodeDemographics( nodeContext_2 ) );

        IPFactory::GetInstance()->Initialize( 1, demographics_1->GetJsonObject() );
        IPFactory::GetInstance()->Initialize( 2, demographics_2->GetJsonObject() );

        IPKeyValue kv_access_no  = IPFactory::GetInstance()->GetIP( "Accessibility" )->GetValues<IPKeyValueContainer>().Get( "Accessibility:NO"  );
        IPKeyValue kv_access_yes = IPFactory::GetInstance()->GetIP( "Accessibility" )->GetValues<IPKeyValueContainer>().Get( "Accessibility:YES" );

        CHECK_CLOSE( 0.85, kv_access_no.GetInitialDistribution(  1 ), 0.00001 );
        CHECK_CLOSE( 0.15, kv_access_yes.GetInitialDistribution( 1 ), 0.00001 );
        CHECK_CLOSE( 0.60, kv_access_no.GetInitialDistribution(  2 ), 0.00001 );
        CHECK_CLOSE( 0.40, kv_access_yes.GetInitialDistribution( 2 ), 0.00001 );

        IPKeyValue kv_risk_high = IPFactory::GetInstance()->GetIP( "Risk" )->GetValues<IPKeyValueContainer>().Get( "Risk:HIGH" );
        IPKeyValue kv_risk_med  = IPFactory::GetInstance()->GetIP( "Risk" )->GetValues<IPKeyValueContainer>().Get( "Risk:MED"  );
        IPKeyValue kv_risk_low  = IPFactory::GetInstance()->GetIP( "Risk" )->GetValues<IPKeyValueContainer>().Get( "Risk:LOW"  );

        CHECK_CLOSE( 0.1, kv_risk_high.GetInitialDistribution( 1 ), 0.00001 );
        CHECK_CLOSE( 0.4, kv_risk_med.GetInitialDistribution(  1 ), 0.00001 );
        CHECK_CLOSE( 0.5, kv_risk_low.GetInitialDistribution(  1 ), 0.00001 );
        CHECK_CLOSE( 0.0, kv_risk_high.GetInitialDistribution( 2 ), 0.00001 );
        CHECK_CLOSE( 0.2, kv_risk_med.GetInitialDistribution(  2 ), 0.00001 );
        CHECK_CLOSE( 0.8, kv_risk_low.GetInitialDistribution(  2 ), 0.00001 );

        IndividualProperty* p_ip_access = IPFactory::GetInstance()->GetIP( "Accessibility" );
        IndividualProperty* p_ip_risk   = IPFactory::GetInstance()->GetIP( "Risk" );

        const auto& r_access_trans_1 = p_ip_access->GetIntraNodeTransmission( 1 );
        CHECK( r_access_trans_1.HasMatrix() );
        CHECK_EQUAL( std::string( "contact" ), r_access_trans_1.GetRouteName() );
        CHECK_EQUAL( 2, r_access_trans_1.GetMatrix().size() );
        CHECK_EQUAL( 2, r_access_trans_1.GetMatrix()[ 0 ].size() );
        CHECK_EQUAL( 2, r_access_trans_1.GetMatrix()[ 1 ].size() );
        CHECK_EQUAL( 1.1f, r_access_trans_1.GetMatrix()[ 0 ][ 0 ] );
        CHECK_EQUAL( 1.2f, r_access_trans_1.GetMatrix()[ 0 ][ 1 ] );
        CHECK_EQUAL( 1.3f, r_access_trans_1.GetMatrix()[ 1 ][ 0 ] );
        CHECK_EQUAL( 1.4f, r_access_trans_1.GetMatrix()[ 1 ][ 1 ] );

        const auto& r_access_trans_2 = p_ip_access->GetIntraNodeTransmission( 2 );
        CHECK( r_access_trans_2.HasMatrix() );
        CHECK_EQUAL( std::string( "contact" ), r_access_trans_2.GetRouteName() );
        CHECK_EQUAL( 2, r_access_trans_2.GetMatrix().size() );
        CHECK_EQUAL( 2, r_access_trans_2.GetMatrix()[ 0 ].size() );
        CHECK_EQUAL( 2, r_access_trans_2.GetMatrix()[ 1 ].size() );
        CHECK_EQUAL( 2.1f, r_access_trans_2.GetMatrix()[ 0 ][ 0 ] );
        CHECK_EQUAL( 2.2f, r_access_trans_2.GetMatrix()[ 0 ][ 1 ] );
        CHECK_EQUAL( 2.3f, r_access_trans_2.GetMatrix()[ 1 ][ 0 ] );
        CHECK_EQUAL( 2.4f, r_access_trans_2.GetMatrix()[ 1 ][ 1 ] );

        const auto& r_risk_trans_1 = p_ip_risk->GetIntraNodeTransmission( 1 );
        CHECK( !r_risk_trans_1.HasMatrix() );

        const auto& r_risk_trans_2 = p_ip_risk->GetIntraNodeTransmission( 2 );
        CHECK( r_risk_trans_2.HasMatrix() );
        CHECK_EQUAL( std::string( "contact" ), r_access_trans_2.GetRouteName() );
        CHECK_EQUAL( 3, r_risk_trans_2.GetMatrix().size() );
        CHECK_EQUAL( 3, r_risk_trans_2.GetMatrix()[ 0 ].size() );
        CHECK_EQUAL( 3, r_risk_trans_2.GetMatrix()[ 1 ].size() );
        CHECK_EQUAL( 3, r_risk_trans_2.GetMatrix()[ 2 ].size() );
        CHECK_EQUAL( 2.1f, r_risk_trans_2.GetMatrix()[ 0 ][ 0 ] );
        CHECK_EQUAL( 2.2f, r_risk_trans_2.GetMatrix()[ 0 ][ 1 ] );
        CHECK_EQUAL( 2.3f, r_risk_trans_2.GetMatrix()[ 0 ][ 2 ] );
        CHECK_EQUAL( 2.4f, r_risk_trans_2.GetMatrix()[ 1 ][ 0 ] );
        CHECK_EQUAL( 2.5f, r_risk_trans_2.GetMatrix()[ 1 ][ 1 ] );
        CHECK_EQUAL( 2.6f, r_risk_trans_2.GetMatrix()[ 1 ][ 2 ] );
        CHECK_EQUAL( 2.7f, r_risk_trans_2.GetMatrix()[ 2 ][ 0 ] );
        CHECK_EQUAL( 2.8f, r_risk_trans_2.GetMatrix()[ 2 ][ 1 ] );
        CHECK_EQUAL( 2.9f, r_risk_trans_2.GetMatrix()[ 2 ][ 2 ] );
    }

    TEST_FIXTURE(PropertiesTestFixture, TestUninitialized)
    {
        try
        {
            IPKey key ;
            CHECK( !key.IsValid() );
            key.ToString();
            CHECK( false );
        }
        catch( NullPointerException& )
        {
            CHECK( true );
        }

        IPKeyValue kv ;
        CHECK( !kv.IsValid() );
        try
        {
            kv.ToString();
            CHECK( false );
        }
        catch( NullPointerException& )
        {
            CHECK( true );
        }

        try
        {
            kv.GetKey<IPKey>();
            CHECK( false );
        }
        catch( NullPointerException& )
        {
            CHECK( true );
        }

        try
        {
            kv.GetValueAsString();
            CHECK( false );
        }
        catch( NullPointerException& )
        {
            CHECK( true );
        }

        try
        {
            kv.GetInitialDistribution( 1 );
            CHECK( false );
        }
        catch( NullPointerException& )
        {
            CHECK( true );
        }
    }

    TEST_FIXTURE(PropertiesTestFixture, TestIPKeyValueContainerErrors)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PropertiesTest/transitions_config.json" ) );
        unique_ptr<Configuration> p_parameters( Environment::CopyFromElement( (*p_config)[ "parameters" ] ) );

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, p_parameters.get(), true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        INodeContext* nodeContext = new INodeContextFake();
        unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

        IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

        std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
        CHECK_EQUAL( 1, ip_list.size() );
        CHECK_EQUAL( std::string("QualityOfCare"), ip_list[0]->GetKeyAsString() );

        IPKeyValueContainer ip_values = ip_list[0]->GetValues<IPKeyValueContainer>();
        CHECK_EQUAL( 3, ip_values.Size() );
        CHECK( ip_values.Contains( "QualityOfCare:Good" ) );
        CHECK( ip_values.Contains( "QualityOfCare:OK"   ) );
        CHECK( ip_values.Contains( "QualityOfCare:Bad"  ) );

        try
        {
            ip_values.Get( ip_list[0]->GetKey<IPKey>() );
            CHECK( false );
        }
        catch( DetailedException& e )
        {
            std::string msg = e.GetMsg();
            if( msg.find( "Illegal use of KeyValueContainer::Get( const Key& rKey ).  Should not be used on containers that have multiple values for one key." ) == string::npos )
            {
                PrintDebug( e.GetMsg() );
                CHECK( false );
            }
        }

        try
        {
            IPKeyValue good( "QualityOfCare:Good" );
            ip_values.Set( good );
            CHECK( false );
        }
        catch( DetailedException& e )
        {
            std::string msg = e.GetMsg();
            if( msg.find( "Illegal use of IPKeyValueContainer::Set( const IPKeyValue& rKeyValue ).  Should not be used on containers that have multiple values for one key" ) == string::npos )
            {
                PrintDebug( e.GetMsg() );
                CHECK( false );
            }
        }
    }

    TEST_FIXTURE(PropertiesTestFixture, TestIPFactoryErrors)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PropertiesTest/transitions_config.json" ) );
        unique_ptr<Configuration> p_parameters( Environment::CopyFromElement( (*p_config)[ "parameters" ] ) );

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, p_parameters.get(), true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        INodeContext* nodeContext = new INodeContextFake();
        unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

        IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

        std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
        CHECK_EQUAL( 1, ip_list.size() );
        CHECK_EQUAL( std::string("QualityOfCare"), ip_list[0]->GetKeyAsString() );

        IPKeyValueContainer ip_values = ip_list[0]->GetValues<IPKeyValueContainer>();
        CHECK_EQUAL( 3, ip_values.Size() );
        CHECK( ip_values.Contains( "QualityOfCare:Good" ) );
        CHECK( ip_values.Contains( "QualityOfCare:OK"   ) );
        CHECK( ip_values.Contains( "QualityOfCare:Bad"  ) );

        // Test for spaces around colon
        std::string kv_str_spaces = "QualityOfCare : Bad";
        std::string key_str_spaces;
        std::string value_str_spaces;
        IPFactory::ParseKeyValueString( kv_str_spaces, key_str_spaces, value_str_spaces );
        CHECK_EQUAL( "QualityOfCare", key_str_spaces );
        CHECK_EQUAL( "Bad", value_str_spaces );

        // Test for more spaces
        std::string kv_str_spaces2 = " QualityOfCare : Bad ";
        std::string key_str_spaces2;
        std::string value_str_spaces2;
        IPFactory::ParseKeyValueString( kv_str_spaces2, key_str_spaces2, value_str_spaces2 );
        CHECK_EQUAL( "QualityOfCare", key_str_spaces2 );
        CHECK_EQUAL( "Bad", value_str_spaces2 );

        try
        {
            std::string key_value_str = "invalid_key-value";
            std::string key_str;
            std::string value_str;
            IPFactory::ParseKeyValueString( key_value_str, key_str, value_str );
            CHECK( false );
        }
        catch( DetailedException& e )
        {
            std::string msg = e.GetMsg();
            if( msg.find( "Invalid IndividualProperties Key-Value string = 'invalid_key-value'.  Format is 'key:value'." ) == string::npos )
            {
                PrintDebug( e.GetMsg() );
                CHECK( false );
            }
        }

        try
        {
            IPFactory::GetInstance()->GetIP( "Risk", "ParamNameXXX", true );
            CHECK( false );
        }
        catch( DetailedException& e )
        {
            std::string msg = e.GetMsg();
            if( msg.find( "Could not find the IndividualProperty key = 'Risk' for parameter 'ParamNameXXX'.  Known keys are: QualityOfCare" ) == string::npos )
            {
                PrintDebug( e.GetMsg() );
                CHECK( false );
            }
        }

        try
        {
            std::string key_str = "QualityOfCare";
            std::map<std::string,float> value_dist_map;
            IPFactory::GetInstance()->AddIP( 1, key_str, value_dist_map );
            CHECK( false );
        }
        catch( DetailedException& e )
        {
            std::string msg = e.GetMsg();
            if( msg.find( "Found existing Property key = 'QualityOfCare'.  Can't create duplicate key.  Known keys are: QualityOfCare" ) == string::npos )
            {
                PrintDebug( e.GetMsg() );
                CHECK( false );
            }
        }
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidTransitionType)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidTransitionType.json",
                          "Invalid Individual_Property Transitions value for Type = XXX.  Known values are: At_Timestep and At_Age" );
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeRestriction)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeRestriction.json",
                          "Variable or parameter 'demographics[Age_In_Years_Restriction][Min]' with value 40 is incompatible with variable or parameter 'demographics[Age_In_Years_Restriction][Max]' with value 19. In the Demographics for IndividualProperties:Property=QualityOfCare, Max age must be greater than Min age.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidCoverage)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidCoverage.json",
                          "Variable 'Demographics[IndividualProperties][Property=QualityOfCare][Coverage]' had value 999 which was inconsistent with range limit 1");
                           
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidProbability)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidProbability.json",
                          "Variable 'Demographics[IndividualProperties][Property=QualityOfCare][Probability_Per_Timestep]' had value -1 which was inconsistent with range limit 0");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestMissingTo)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestMissingTo.json",
                          "Parameter 'To' not found in input file 'demographics file'");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestMissingStart)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestMissingStart.json",
                          "Parameter 'Start' not found in input file 'demographics file'");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidTransmissionMatrixA)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidTransmissionMatrixA.json",
                          "Invalid Transmission Matrix for property 'Age_Bin'.  It has 2 rows when it should have 3.  It should be square with one row/col per value for the property.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidTransmissionMatrixB)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidTransmissionMatrixB.json",
                          "Invalid Transmission Matrix for property 'Age_Bin'.  Row 1 has 2 columns when it should have 3.  It should be square with one row/col per value for the property.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestMissingValues)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestMissingValues.json",
                          "Failed to find Values in map demographics[IndividualProperties][0]");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestMissingInitialDistribution)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestMissingInitialDistribution.json",
                          "Failed to find Initial_Distribution in map demographics[IndividualProperties][0]");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidNumInitialDistribution)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidNumInitialDistribution.json",
                          "Number of Values in Values (2) needs to be the same as number of values in Initial_Distribution (3).");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestDuplicateValues)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestDuplicateValues.json",
                          "demographics[IndividualProperties][0] with property=Risk has a duplicate value = HIGH");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestZeroValues)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestZeroValues.json",
                          "demographics[IndividualProperties][0][Values] (property=Risk) cannot have zero values.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidInitialDistribution)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidInitialDistribution.json",
                          "The values in demographics[IndividualProperties][1][Initial_Distribution] (property=Risk) add up to 1.45.  They must add up to 1.0");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestMissingAgeBinEdge)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestMissingAgeBinEdge.json",
                          "Failed to find Age_Bin_Edges_In_Years in map demographics[IndividualProperties][0] (property = Age_Bin)");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeBinEdgeA)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeBinEdgeA.json",
                          "demographics[IndividualProperties][0][Age_Bin_Edges_In_Years] must have at least two values: 0 must be first and -1 is last.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeBinEdgeB)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeBinEdgeB.json",
                          "demographics[IndividualProperties][0][Age_Bin_Edges_In_Years] must have at least two values: 0 must be first and -1 is last.  The first value cannot be 10");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeBinEdgeC)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeBinEdgeC.json",
                          "demographics[IndividualProperties][0][Age_Bin_Edges_In_Years] must have at least two values: 0 must be first and -1 is last.  The last value cannot be 10");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeBinEdgeD)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeBinEdgeD.json",
                          "demographics[IndividualProperties][0][Age_Bin_Edges_In_Years] must be in increasing order with the last value = -1.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeBinEdgeE)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeBinEdgeE.json",
                          "demographics[IndividualProperties][0][Age_Bin_Edges_In_Years] must be in increasing order with the last value = -1.");
    }

    TEST_FIXTURE(PropertiesTestFixture, TestInvalidAgeBinTransition)
    {
        TestReadingError( false, __LINE__, "testdata/PropertiesTest/demog_TestInvalidAgeBinTransition.json",
                          "demographics[IndividualProperties][0][Transitions] has more than zero entries.  They are not allowed with property=Age_Bin");
    }

    TEST_FIXTURE( PropertiesTestFixture, TestNotSameNumberOfIPs )
    {
        TestReadingError( true, __LINE__, "testdata/PropertiesTest/demog_TestNotSameNumberOfIPs.json",
            "IndividualProperties were first initialized for nodeID=1 and it had 2 properties.\nnodeID=2 has 1 properties.  All nodes must have the same keys and values." );
    }

    TEST_FIXTURE( PropertiesTestFixture, TestDifferentKey )
    {
        TestReadingError( true, __LINE__, "testdata/PropertiesTest/demog_TestDifferentKey.json",
            "IndividualProperties were first initialized for node 1.\nnodeID=2 has 'RiskyBusiness' which is not in the first node.\nAll nodes must have the same keys and values (and in the same order)." );
    }

    TEST_FIXTURE( PropertiesTestFixture, TestNotSameNumberOfValues )
    {
        TestReadingError( true, __LINE__, "testdata/PropertiesTest/demog_TestNotSameNumberOfValues.json",
            "demographics[IndividualProperties][1][Values] for key=Risk and nodeId=2 has 4 values.\nThe previous node(s) had 3 values.  All nodes must have the same keys and values." );
    }

    TEST_FIXTURE( PropertiesTestFixture, TestDifferentValue )
    {
        TestReadingError( true, __LINE__, "testdata/PropertiesTest/demog_TestDifferentValue.json",
            "demographics[IndividualProperties][1] with property=Risk for NodeId=2 has value=MED.\nPrevious node(s) do not have this value.  All nodes must have the same keys and values." );
    }

    TEST_FIXTURE( PropertiesTestFixture, TestDuplicateKeys )
    {
        TestReadingError( true, __LINE__, "testdata/PropertiesTest/demog_TestDuplicateKeys.json",
                          "Found existing Property key = 'Risk'.  Can't create duplicate key.  Known keys are: Risk, Accessibility" );
    }

    TEST_FIXTURE(PropertiesTestFixture, TestGetAllPossibleKeyValueCombinations)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        pSimConfig->demographics_initial = true ;
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet( "testdata/PropertiesTest/demog_TestGetAllPossibleKeyValueCombinations.json" ) ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

        vector<uint32_t> nodeIDs = factory->GetNodeIDs();
        for (uint32_t node_id : nodeIDs)
        {
            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        INodeContext* nodeContext = new INodeContextFake();
        unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics(nodeContext) );

        IPFactory::GetInstance()->Initialize( 1, demographics->GetJsonObject() );

        std::vector<IndividualProperty*> ip_list = IPFactory::GetInstance()->GetIPList();
        CHECK_EQUAL( 4, ip_list.size() );
        CHECK_EQUAL( std::string("Accessibility"), ip_list[0]->GetKeyAsString() );
        CHECK_EQUAL( std::string("HasActiveTB"  ), ip_list[1]->GetKeyAsString() );
        CHECK_EQUAL( std::string("QualityOfCare"), ip_list[2]->GetKeyAsString() );
        CHECK_EQUAL( std::string("Risk"         ), ip_list[3]->GetKeyAsString() );

        std::vector<std::string> combos = IPFactory::GetInstance()->GetAllPossibleKeyValueCombinations<IPKeyValueContainer>();
        //std::stringstream ss;
        //for( auto combo : combos )
        //{
        //    ss << combo << "\n";
        //}
        //PrintDebug(ss.str());

        CHECK_EQUAL( 48, combos.size() );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:E,Risk:H", combos[ 0] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:E,Risk:H", combos[ 1] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:E,Risk:H", combos[ 2] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:E,Risk:H", combos[ 3] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:G,Risk:H", combos[ 4] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:G,Risk:H", combos[ 5] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:G,Risk:H", combos[ 6] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:G,Risk:H", combos[ 7] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:O,Risk:H", combos[ 8] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:O,Risk:H", combos[ 9] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:O,Risk:H", combos[10] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:O,Risk:H", combos[11] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:B,Risk:H", combos[12] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:B,Risk:H", combos[13] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:B,Risk:H", combos[14] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:B,Risk:H", combos[15] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:E,Risk:M", combos[16] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:E,Risk:M", combos[17] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:E,Risk:M", combos[18] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:E,Risk:M", combos[19] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:G,Risk:M", combos[20] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:G,Risk:M", combos[21] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:G,Risk:M", combos[22] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:G,Risk:M", combos[23] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:O,Risk:M", combos[24] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:O,Risk:M", combos[25] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:O,Risk:M", combos[26] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:O,Risk:M", combos[27] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:B,Risk:M", combos[28] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:B,Risk:M", combos[29] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:B,Risk:M", combos[30] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:B,Risk:M", combos[31] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:E,Risk:L", combos[32] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:E,Risk:L", combos[33] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:E,Risk:L", combos[34] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:E,Risk:L", combos[35] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:G,Risk:L", combos[36] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:G,Risk:L", combos[37] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:G,Risk:L", combos[38] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:G,Risk:L", combos[39] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:O,Risk:L", combos[40] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:O,Risk:L", combos[41] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:O,Risk:L", combos[42] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:O,Risk:L", combos[43] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:F,QualityOfCare:B,Risk:L", combos[44] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:F,QualityOfCare:B,Risk:L", combos[45] );
        CHECK_EQUAL( "Accessibility:N,HasActiveTB:T,QualityOfCare:B,Risk:L", combos[46] );
        CHECK_EQUAL( "Accessibility:Y,HasActiveTB:T,QualityOfCare:B,Risk:L", combos[47] );
    }
}
