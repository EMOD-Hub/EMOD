
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"

#include "FileSystem.h"
#include "NodeDemographics.h"
#include "SimulationConfig.h"
#include "INodeContextFake.h"
#include "IdmMpi.h"
#include "Instrumentation.h"
#include "RANDOM.h"

#include <string>
#include <vector>
#include <memory> // unique_ptr

#ifdef WIN32
#include <sys/stat.h> // _S_IREAD
#include <io.h> //_chmod
#endif

using namespace Kernel;
using namespace std;


SUITE(NodeDemographicsTest)
{
    typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
    typedef nodeid_suid_map_t::value_type nodeid_suid_pair;

    struct NodeDemographicsFactoryFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* pSimConfig ;

        suids::suid next_suid;

        NodeDemographicsFactoryFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;

            next_suid.data = 1;

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            string configFilename("testdata/NodeDemographicsTest/config.json");
            string inputPath("testdata/NodeDemographicsTest");
            string outputPath("testdata/NodeDemographicsTest/output");
            string statePath("testdata/NodeDemographicsTest");
            string dllPath("testdata/NodeDemographicsTest");
            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);

            pSimConfig = SimulationConfigFactory::CreateInstance(Environment::getInstance()->Config);
            if (pSimConfig)
            {
                Environment::setSimulationConfig(pSimConfig);
            }
        }

        ~NodeDemographicsFactoryFixture()
        {
            delete m_pMpi;
            delete pSimConfig;
            Environment::Finalize();
            NodeDemographicsFactory::SetDemographicsFileList( std::vector<std::string>() ) ;
        }

        suids::suid GetNextNodeSuid()
        {
            suids::suid next = next_suid;
            next_suid.data++;
            return next;
        }

        void TestHelper_FormatException( int lineNumber, const std::string& rDemoFilenames, const std::string& rExpMsg )
        {
            NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet(rDemoFilenames) ) ;

            nodeid_suid_map_t node_id_suid_map;
            try
            {
                NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 );
                CHECK_LN( false, lineNumber ); // shouldn't get here
            }
            catch( DetailedException& e )
            {
                // ----------------------------------------------------------------------------------------------------
                // --- NOTE: This check ensures that the expected message is contained in the message returned.
                // --- We do not check the whole message in order reduce the likelihood that the message will change
                // --- and break the test.  The whole message contains file and line number.  By checking for contains
                // --- the tests does not file just because someone added code and the line number changed.
                // --- NOTE: CHECK_LN() - Allows us to report the line number of the calling test method as failed versus
                // --- just reporting this line and not know which test failed.
                // ----------------------------------------------------------------------------------------------------
                std::string msg = e.GetMsg();
                bool passed = msg.find( rExpMsg ) != string::npos ;
                if( !passed )
                {
                    PrintDebug( msg );
                }
                CHECK_LN( passed, lineNumber );
            }
        }

        void TestHelper_MissingAttributes_FormatException( int lineNumber, const std::string& rDemoFilenames, const std::string& rExpMsg )
        {
            NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet(rDemoFilenames) ) ;

            nodeid_suid_map_t node_id_suid_map;
            unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory( &node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

            vector<uint32_t> nodeIDs = factory->GetNodeIDs();
            for (uint32_t node_id : nodeIDs)
            {
                suids::suid node_suid = GetNextNodeSuid();
                node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            INodeContextFake ncf ;
        
            try
            {
                unique_ptr<NodeDemographics> demographics( factory->CreateNodeDemographics( &ncf ) );

                (*demographics)["NodeAttributes"]["Latitude"].AsDouble();
                CHECK_LN( false, lineNumber );
            }
            catch( NodeDemographicsFormatErrorException& e )
            {
                // ----------------------------------------------------------------
                // --- See exception handling comment in TestHelper_FormatException
                // ----------------------------------------------------------------
                std::string msg = e.GetMsg();
                bool passed = msg.find( rExpMsg ) != string::npos ;
                if( !passed )
                {
                    PrintDebug( msg );
                }
                CHECK_LN( passed, lineNumber );
            }
        }

        void TestHelper_DistributionFormatException( int lineNumber, 
                                                     const std::string& rDemoFilenames, 
                                                     const std::string& rDistName, 
                                                     const std::vector<std::string>& rAxisNames,
                                                     const std::string& rExpMsg )
        {
            NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet(rDemoFilenames) ) ;

            nodeid_suid_map_t node_id_suid_map;
            unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );

            vector<uint32_t> nodeIDs = factory->GetNodeIDs();
            for (uint32_t node_id : nodeIDs)
            {
                suids::suid node_suid = GetNextNodeSuid();
                node_id_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            INodeContextFake ncf ;
        
            unique_ptr<NodeDemographics> p_node_demo( factory->CreateNodeDemographics(&ncf) );

            try
            {
                unique_ptr<NodeDemographicsDistribution> p_ndd ( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo)["IndividualAttributes"][rDistName], rAxisNames ) );
                CHECK_LN( false, lineNumber ); // shouldn't get here
            }
            catch( DetailedException& re )
            {
                // ----------------------------------------------------------------
                // --- See exception handling comment in TestHelper_FormatException
                // ----------------------------------------------------------------
                std::string msg = re.GetMsg();
                bool passed = msg.find( rExpMsg ) != string::npos ;
                if( !passed )
                {
                    std::string exp_msg = std::string( "Expected: " ) + rExpMsg + std::string("\n");
                    std::string act_msg = std::string( "Actual..: " ) + re.GetMsg();
                    PrintDebug( exp_msg );
                    PrintDebug( act_msg );
                }
                CHECK_LN( passed, lineNumber );
            }
        }


    };

#ifndef INCLUDED  // Use this to control what tests are run during development

    TEST(LegacyDemographicsFilename)
    {
        IdmMpi::MessageInterface* pMpi = IdmMpi::MessageInterface::CreateNull();
        Environment::Finalize();
        Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
        string configFilename("testdata/NodeDemographicsTest/config_legacy.json");
        string p("testdata/NodeDemographicsTest");
        Environment::Initialize(pMpi,configFilename, p, p, /*p, */p, false);
        SimulationConfig* pSimConfig = SimulationConfigFactory::CreateInstance(Environment::getInstance()->Config);
        CHECK(pSimConfig);
        Environment::setSimulationConfig(pSimConfig);
        nodeid_suid_map_t node_id_suid_map;
        NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 );
        CHECK_EQUAL(NodeDemographicsFactory::GetDemographicsFileList().front(),"demographics.compiled.json");
        Environment::Finalize();
        delete pMpi;
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, Create)
    {
#ifdef WIN32
        // ---------------------------------------------------------------------------------------------------------------------------------
        // --- This is to test a bug where I accidentally changed the code that opens the demographics file from using ifstream to fstream.
        // --- If the file is read-only and the code uses fstream, the file will fail to open.  This makes sure that is does not happen again.
        // ---------------------------------------------------------------------------------------------------------------------------------
        std::string demo_filename = "testdata/NodeDemographicsTest/demographics.compiled.json";
        _chmod(demo_filename.c_str(), _S_IREAD );
#endif
        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& ids = factory->GetNodeIDs();
        CHECK_EQUAL(1, ids.size());
        CHECK_EQUAL(1, ids[0]);
        string idReference = factory->GetIdReference();
        CHECK_EQUAL("Gridded world grump2.5arcmin", idReference);
        unique_ptr<DemographicsContext> context( factory->CreateDemographicsContext() );
        CHECK(context != nullptr);

#ifdef WIN32
        _chmod(demo_filename.c_str(), _S_IREAD | _S_IWRITE );
#endif
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, CreateNodeDemographics)
    {
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

        int32_t initial_population = (*demographics)["NodeAttributes"]["InitialPopulation"].AsDouble();
        string axis_age = (*demographics)["IndividualAttributes"]["MortalityDistribution"]["AxisNames"][1].AsString();

        CHECK_EQUAL( 10000, initial_population );
        CHECK_EQUAL( string("age"), axis_age );

        CHECK(demographics->Contains("NodeAttributes"));
        CHECK((*demographics)["NodeAttributes"].Contains("InitialPopulation"));
        CHECK(demographics->Contains("IndividualAttributes"));
        CHECK((*demographics)["IndividualAttributes"].Contains("MortalityDistribution"));
        CHECK((*demographics)["IndividualAttributes"]["MortalityDistribution"].Contains("AxisNames"));
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestFileNotFound)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("unknown_demographics.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        try
        {
            NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 );
            CHECK( false ); // shouldn't get here
        }
        catch( FileNotFoundException& fnfe )
        {
            std::string exp_msg;
            exp_msg  = "Could not find file unknown_demographics.json.\n";
            exp_msg += "Received the following system error messages while checking for the existence\n";
            exp_msg += "of the file at the following locations:\n";
            exp_msg += "testdata/NodeDemographicsTest/unknown_demographics.json - 'No such file or directory'";

            std::string msg = fnfe.GetMsg();
            bool passed = msg.find( exp_msg ) != string::npos;
            if( !passed )
            {
                PrintDebug( exp_msg );
                PrintDebug( msg );
                CHECK( passed );
            }
        }
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestNoMetadata)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestNoMetadata.json", 
                                              "Failed to parse incoming text. Name of an object member must be a string" );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestNoNodeCount)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestNoNodeCount.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestNoNodeCount.json).  Missing the 'Metadata.NodeCount' object." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestZeroNodecount)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestZeroNodeCount.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestZeroNodeCount.json).  'NodeCount' = 0.  It must be positive." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestMissingIdReference)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestMissingIdReference.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestMissingIdReference.json).  The 'IdReference' attribute is missing in the 'Metadata' group." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestOnlyMetadata)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestOnlyMetadata.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestOnlyMetadata.json).  It is the base layer demographics file.  It must have nodes defined in order to be a base layer.  The file is assumed to be uncompiled." );
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestEmptyNodesArray)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestEmptyNodesArray.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestEmptyNodesArray.json).  It is the base layer demographics file.  It must have nodes defined in order to be a base layer.  The file is assumed to be uncompiled." );
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestMissingNodeId)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestMissingNodeId.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestMissingNodeId.json).  It is missing the 'NodeID' attribute for node number 1." );
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestOnlyNodeId)
    {
        TestHelper_MissingAttributes_FormatException( __LINE__, "demographics_TestOnlyNodeId.json",
                                                                "Format error encountered loading demographics file (UNKNOWN).  Demographics attribute 'NodeAttributes' not present for any node in any demographics layer for NodeID=1.\nWas some config.json parameter changed without the demographics layer(s) specified containing the necessary parameters?");
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestNodeOffsetNoStringTable)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestNodeOffsetNoStringTable.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestNodeOffsetNoStringTable.json).  Invalid compiled file.  The file contains 'NodeOffsets' but does not have 'StringTable'." );
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestStringTableBadFormat)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestStringTableBadFormat.json", 
                                              "Failed to parse incoming text. Expect a value here. at character=991 / line number=0" );
    }
    
#ifdef USE_NODEOFFSET
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestBadIdInNodeOffset)
    {
        TestHelper_MissingAttributes_FormatException( __LINE__, "demographics_TestBadIdInNodeOffset.json",
                                                                "Format error encountered loading demographics file (UNKNOWN).  NodeID for lookup (2457) does not equal the NodeID (1) found in the data.  Is NodeOffset messed up?");
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestBadOffsetInNodeOffset)
    {
        TestHelper_MissingAttributes_FormatException( __LINE__, "demographics_TestBadOffsetInNodeOffset.json",
                                                                "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestBadOffsetInNodeOffset.json).  NodeID for lookup (1) does not have data at offset 4095.  Is NodeOffset messed up?");
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestBadNodeOffsetNot16)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestBadNodeOffsetNot16.json",
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestBadNodeOffsetNot16.json).  Length of 'NodeOffsets' isn't consistent with 'NodeCount' attribute." );
    }
    
    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestBadNodeOffsetNc2) //offset is for two nodes while nodecount=1
    {
        TestHelper_FormatException( __LINE__, "demographics_TestBadNodeOffsetNc2.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestBadNodeOffsetNc2.json).  Length of 'NodeOffsets' isn't consistent with 'NodeCount' attribute." );
    }
#endif

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestMisingNodeAttributes)
    {
        TestHelper_MissingAttributes_FormatException( __LINE__, "demographics_TestMisingNodeAttributes.json",
                                                                "Format error encountered loading demographics file (UNKNOWN).  Demographics attribute 'NodeAttributes' not present for any node in any demographics layer for NodeID=340461476.\nWas some config.json parameter changed without the demographics layer(s) specified containing the necessary parameters?");
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestMisingNodeAttributesFromOne)
    {
        TestHelper_MissingAttributes_FormatException( __LINE__, "demographics_TestMisingNodeAttributesFromOne.json",
                                                                "Format error encountered loading demographics file (UNKNOWN).  NodeID 340461476's 'Node' object doesn't contain expected demographics attribute 'NodeAttributes'.");
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestMisingLatitude)
    {
        TestHelper_MissingAttributes_FormatException( __LINE__, "demographics_TestMisingLatitude.json",
                                                                "Format error encountered loading demographics file (UNKNOWN).  NodeID 340461476's 'NodeAttributes' object doesn't contain expected demographics attribute 'Latitude'.");
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestTwoFileGood)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_TestTwoFileGood_base.json;demographics_TestTwoFileGood_overlay.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& ids = factory->GetNodeIDs();
        CHECK_EQUAL(2, ids.size());
        CHECK_EQUAL(340461476, ids[0]);
        CHECK_EQUAL(340461477, ids[1]);
        string idReference = factory->GetIdReference();
        CHECK_EQUAL("Gridded world grump2.5arcmin", idReference);
        unique_ptr<DemographicsContext> context( factory->CreateDemographicsContext() );
        CHECK(context != nullptr);
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestTwoFileGoodDifferentDirectories)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_TestTwoFileGood_base.json;another_folder/demographics_TestTwoFileGood_overlay.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL(2, nodeIDs.size());
        CHECK_EQUAL(340461476, nodeIDs[0]);
        CHECK_EQUAL(340461477, nodeIDs[1]);

        suids::suid node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[0], node_suid ) );
        INodeContextFake ncf_1( node_suid );

        node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[1], node_suid ) );
        INodeContextFake ncf_2( node_suid );

        unique_ptr<NodeDemographics> p_node_demo_1( factory->CreateNodeDemographics(&ncf_1) );
        unique_ptr<NodeDemographics> p_node_demo_2( factory->CreateNodeDemographics(&ncf_2) );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestTwoFileGoodDefaultOverlayOneNode)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_TestTwoFileGoodDefaultOverlayOneNode_base.json;demographics_TestTwoFileGoodDefaultOverlayOneNode_overlay.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL(2, nodeIDs.size());
        CHECK_EQUAL(340461476, nodeIDs[0]);
        CHECK_EQUAL(340461477, nodeIDs[1]);

        suids::suid node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[0], node_suid ) );
        INodeContextFake ncf_1( node_suid );

        node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[1], node_suid ) );
        INodeContextFake ncf_2( node_suid );

        unique_ptr<NodeDemographics> p_node_demo_1( factory->CreateNodeDemographics(&ncf_1) );
        unique_ptr<NodeDemographics> p_node_demo_2( factory->CreateNodeDemographics(&ncf_2) );

        // -------------------------------------------------
        // --- Check properties that are unique for the node
        // -------------------------------------------------

        double  lat_1 = (*p_node_demo_1)["NodeAttributes"]["Latitude"         ].AsDouble();
        double  lon_1 = (*p_node_demo_1)["NodeAttributes"]["Longitude"        ].AsDouble();
        double  alt_1 = (*p_node_demo_1)["NodeAttributes"]["Altitude"         ].AsDouble();
        int32_t pop_1 = (*p_node_demo_1)["NodeAttributes"]["InitialPopulation"].AsInt();

        CHECK_EQUAL( -8.5, lat_1 );
        CHECK_EQUAL( 36.5, lon_1 );
        CHECK_EQUAL(  1.0, alt_1 );
        CHECK_EQUAL( 1000, pop_1 );

        double  lat_2 = (*p_node_demo_2)["NodeAttributes"]["Latitude"         ].AsDouble();
        double  lon_2 = (*p_node_demo_2)["NodeAttributes"]["Longitude"        ].AsDouble();
        double  alt_2 = (*p_node_demo_2)["NodeAttributes"]["Altitude"         ].AsDouble();
        int32_t pop_2 = (*p_node_demo_2)["NodeAttributes"]["InitialPopulation"].AsInt();

        CHECK_EQUAL( -9.5, lat_2 );
        CHECK_EQUAL( 37.5, lon_2 );
        CHECK_EQUAL(  5.0, alt_2 );
        CHECK_EQUAL(  999, pop_2 );

        // -----------------------------------------------------------
        // --- Check the property that is from the overlay's Default
        // --- This should have been applied to all nodes.
        // -----------------------------------------------------------
        double br_1 = (*p_node_demo_1)["NodeAttributes"]["BirthRate"].AsDouble();
        double br_2 = (*p_node_demo_2)["NodeAttributes"]["BirthRate"].AsDouble();

        CHECK_CLOSE( 0.001, br_1, 0.001 );
        CHECK_CLOSE( 0.777, br_2, 0.001 );

        bool has_ia_1 = (*p_node_demo_1).Contains( "IndividualAttributes" );
        bool has_ia_2 = (*p_node_demo_2).Contains( "IndividualAttributes" );

        CHECK( !has_ia_1 );
        CHECK(  has_ia_2 );

        int32_t adf_2 = (*p_node_demo_2)["IndividualAttributes"]["AgeDistributionFlag" ].AsInt();
        double  ad1_2 = (*p_node_demo_2)["IndividualAttributes"]["AgeDistribution1"    ].AsDouble();
        double  ad2_2 = (*p_node_demo_2)["IndividualAttributes"]["AgeDistribution2"    ].AsDouble();

        CHECK_EQUAL(   3, adf_2 );
        CHECK_EQUAL( 0.8, ad1_2 );
        CHECK_EQUAL( 0.1, ad2_2 );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestTwoFileGoodDefaultOverlayAllNodes)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_TestTwoFileGoodDefaultOverlayAllNodes_base.json;demographics_TestTwoFileGoodDefaultOverlayAllNodes_overlay.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL(2, nodeIDs.size());
        CHECK_EQUAL(340461476, nodeIDs[0]);
        CHECK_EQUAL(340461477, nodeIDs[1]);

        suids::suid node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[0], node_suid ) );
        INodeContextFake ncf_1( node_suid );

        node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[1], node_suid ) );
        INodeContextFake ncf_2( node_suid );

        unique_ptr<NodeDemographics> p_node_demo_1( factory->CreateNodeDemographics(&ncf_1) );
        unique_ptr<NodeDemographics> p_node_demo_2( factory->CreateNodeDemographics(&ncf_2) );

        // -------------------------------------------------
        // --- Check properties that are unique for the node
        // -------------------------------------------------
        int32_t node_pop_1 = (*p_node_demo_1)["NodeAttributes"]["InitialPopulation"].AsInt();
        int32_t node_pop_2 = (*p_node_demo_2)["NodeAttributes"]["InitialPopulation"].AsInt();

        CHECK_EQUAL( 1000, node_pop_1 );
        CHECK_EQUAL(  999, node_pop_2 );

        // -----------------------------------------------------------
        // --- Check the property that is from the overlay's Default
        // --- This should have been applied to all nodes.
        // -----------------------------------------------------------
        double node_br_1 = (*p_node_demo_1)["NodeAttributes"]["BirthRate"].AsDouble();
        double node_br_2 = (*p_node_demo_2)["NodeAttributes"]["BirthRate"].AsDouble();

        CHECK_CLOSE( 0.777, node_br_1, 0.001 );
        CHECK_CLOSE( 0.777, node_br_2, 0.001 );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestTwoFileDiffIdRef)
    {
        // Use lower case for IdReference values since the DTK converts all IdReference values to... lower case.
        TestHelper_FormatException( __LINE__, "demographics_TestTwoFileDiffIdRef_base.json;demographics_TestTwoFileDiffIdRef_overlay.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestTwoFileDiffIdRef_overlay.json).  IdReference (=Different ID Ref) doesn't match base layer (=Gridded world grump2.5arcmin)." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestTwoFileDiffNodeIds)
    {
        TestHelper_FormatException( __LINE__, "demographics_TestTwoFileDiffNodeIds_base.json;demographics_TestTwoFileDiffNodeIds_overlay.json", 
                                              "Format error encountered loading demographics file (testdata/NodeDemographicsTest/demographics_TestTwoFileDiffNodeIds_overlay.json).  Node number 1 with 'NodeID' = 99999 does not exist in the base layer.  The nodes in the overlay layers must exist in the base layer." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistGood)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_TestDistGood.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL(1, nodeIDs.size());
        CHECK_EQUAL(1, nodeIDs[0]);

        suids::suid node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( nodeIDs[0], node_suid ) );
        INodeContextFake ncf( node_suid );

        unique_ptr<NodeDemographics> p_node_demo( factory->CreateNodeDemographics(&ncf) );

        unique_ptr<NodeDemographicsDistribution> p_ndd_age    ( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo)["IndividualAttributes"]["AgeDistribution"           ] ) );
        unique_ptr<NodeDemographicsDistribution> p_ndd_suscep ( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo)["IndividualAttributes"]["SusceptibilityDistribution"] ) );
        unique_ptr<NodeDemographicsDistribution> p_ndd_fert   ( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo)["IndividualAttributes"]["FertilityDistribution"     ], "urban",  "age") );
        unique_ptr<NodeDemographicsDistribution> p_ndd_mort   ( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo)["IndividualAttributes"]["MortalityDistribution"     ], "gender", "age") );
    }

    TEST_FIXTURE( NodeDemographicsFactoryFixture, TestDistSuscepMissingDistValues )
    {
        std::vector<std::string> axis_names;
        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistSuscepMissingDistValues.json", "SusceptibilityDistribution", axis_names,
                                                "SusceptibilityDistribution for NodeID=1 is missing a 'DistributionValues' element." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistAgeDistNumResultValuesNotMatchDistValuesA)
    {
        std::vector<std::string> axis_names ;
        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistAgeDistNumResultValuesNotMatchDistValuesA.json", "AgeDistribution", axis_names,
                                                           "AgeDistribution for NodeID=1 has invalid 'DistributionValues' array.  DistributionValues has 20 elements when it should have 15." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistAgeDistNumResultValuesNotMatchDistValuesB)
    {
        std::vector<std::string> axis_names ;
        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistAgeDistNumResultValuesNotMatchDistValuesB.json", "AgeDistribution", axis_names,
                                                           "AgeDistribution for NodeID=1 has invalid 'DistributionValues' array.  DistributionValues has 15 elements when it should have 20." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, demographics_TestDistAgeDisttNumResultValuesNotMatchDistValuesC_DoubleArray)
    {
        std::vector<std::string> axis_names ;
        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistAgeDisttNumResultValuesNotMatchDistValuesC_DoubleArray.json", "AgeDistribution", axis_names,
                                                "ERROR:  Unexpected type while parsing 'ResultValues'\nin distribution 'AgeDistribution' for nodeID = 1.\nExpected an array of floats but detected an array of arrays.)" );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidNumAxisNames)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidNumAxisNames.json", "FertilityDistribution", axis_names,
                                                          "FertilityDistribution for NodeID=1 has 'AxisNames' with: 'urban', 'age', 'gender'.  Axis names must be the following and in the given order: 'urban', 'age'.");
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidAxisNames)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidAxisNames.json", "FertilityDistribution", axis_names,
                                                           "FertilityDistribution for NodeID=1 has 'AxisNames' with: 'urban', 'XXX'.  Axis names must be the following and in the given order: 'urban', 'age'." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidAxisScaleFactors)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidAxisScaleFactors.json", "FertilityDistribution", axis_names,
                                                           "FertilityDistribution for NodeID=1 has 'AxisScaleFactors' with 1 factor(s).  It must have one factor for each axis." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidPopGroupsA)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidPopGroupsA.json", "FertilityDistribution", axis_names,
                                                           "FertilityDistribution for NodeID=1 has 'PopulationGroups' with 1 array(s).  It must have one array for each axis." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidPopGroupsB)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidPopGroupsB.json", "FertilityDistribution", axis_names,
                                                           "FertilityDistribution for NodeID=1 has invalid 'ResultValues' array.  ResultValues has 2 elements when it should have 9.  The number of elements must match that in 'PopulationGroups'." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidResultsValueA)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidResultsValueA.json", "FertilityDistribution", axis_names,
                                                           "FertilityDistribution for NodeID=1 has invalid 'ResultValues' array.  ResultValues has 3 elements when it should have 2.  The number of elements must match that in 'PopulationGroups'." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistFertDistInvalidResultsValueB)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "urban" );
        axis_names.push_back( "age" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistFertDistInvalidResultsValueB.json", "FertilityDistribution", axis_names,
                                                           "FertilityDistribution for NodeID=1 has invalid 'ResultValues' array.  ResultValues[1] has 4 elements when it should have 9." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistHumoralResultValuesDontMatchDistValuesA)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "age" );
        axis_names.push_back( "mucosal_memory" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistHumoralResultValuesDontMatchDistValuesA.json", "humoral_memory_distribution1", axis_names,
                                                           "humoral_memory_distribution1 for NodeID=1 has invalid 'DistributionValues' array.  DistributionValues has 1 elements when it should have 2.  The outer number of arrays must match that in 'PopulationGroups' with the inner most arrays having the same number of elements as the 'ResultValues'." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistHumoralResultValuesDontMatchDistValuesB)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "age" );
        axis_names.push_back( "mucosal_memory" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistHumoralResultValuesDontMatchDistValuesB.json", "humoral_memory_distribution1", axis_names,
                                                           "humoral_memory_distribution1 for NodeID=1 has invalid 'DistributionValues' array.  DistributionValues[1] has 2 elements when it should have 4.  The outer number of arrays must match that in 'PopulationGroups' with the inner most arrays having the same number of elements as the 'ResultValues'." );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistHumoralResultValuesDontMatchDistValuesC)
    {
        std::vector<std::string> axis_names ;
        axis_names.push_back( "age" );
        axis_names.push_back( "mucosal_memory" );

        TestHelper_DistributionFormatException( __LINE__, "demographics_TestDistHumoralResultValuesDontMatchDistValuesC.json", "humoral_memory_distribution1", axis_names,
                                                           "humoral_memory_distribution1 for NodeID=1 has invalid 'DistributionValues' array.  DistributionValues[1][2] has 2 elements when it should have 3.  The outer number of arrays must match that in 'PopulationGroups' with the inner most arrays having the same number of elements as the 'ResultValues'." );
    }

    TEST(TestNodeDemographicsEquality)
    {
        std::string dist_str_1 = "" ;
        dist_str_1 += "{" ;
        dist_str_1 += "\"AxisNames\": [ \"gender\", \"age\" ],";
        dist_str_1 += "\"AxisScaleFactors\": [ 1, 365 ],";
        dist_str_1 += "\"PopulationGroups\": [";
        dist_str_1 += "    [ 0, 1 ]," ;
        dist_str_1 += "    [ 0, 100, 2000 ]";
        dist_str_1 += "],";
        dist_str_1 += "\"ResultScaleFactor\": 0.1,";
        dist_str_1 += "\"ResultValues\": [";
        dist_str_1 += "    [ 0, 20.0, 400.0 ],";
        dist_str_1 += "    [ 0, 30.0, 500.0 ]";
        dist_str_1 += "]" ;
        dist_str_1 += "}" ;

        JsonObjectDemog dist_json_1;
        dist_json_1.Parse( dist_str_1.c_str() );

        std::string dist_str_2 = "" ;
        dist_str_2 += "{" ;
        dist_str_2 += "\"AxisNames\": [ \"gender\", \"age\" ],";
        dist_str_2 += "\"AxisScaleFactors\": [ 1, 365 ],";
        dist_str_2 += "\"PopulationGroups\": [";
        dist_str_2 += "    [ 0, 1 ]," ;
        dist_str_2 += "    [ 0, 100, 2000 ]";
        dist_str_2 += "],";
        dist_str_2 += "\"ResultScaleFactor\": 0.1,";
        dist_str_2 += "\"rv\": [";                    //diffferent label
        dist_str_2 += "    [ 0, 20.0, 400.0 ],"; 
        dist_str_2 += "    [ 0, 30.0, 500.0 ]";
        dist_str_2 += "]" ;
        dist_str_2 += "}" ;

        JsonObjectDemog dist_json_2;
        dist_json_2.Parse( dist_str_2.c_str() );

        std::map<std::string, std::string> string_table_1 ;
        string_table_1["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table_1["AxisNames"          ] = "AxisNames" ;
        string_table_1["AxisUnits"          ] = "AxisUnits" ;
        string_table_1["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table_1["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table_1["PopulationGroups"   ] = "PopulationGroups" ;
        string_table_1["ResultUnits"        ] = "ResultUnits" ;
        string_table_1["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table_1["ResultValues"       ] = "ResultValues" ;
        string_table_1["DistributionValues" ] = "DistributionValues" ;

        std::map<std::string, std::string> string_table_2 ;
        string_table_2["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table_2["AxisNames"          ] = "AxisNames" ;
        string_table_2["AxisUnits"          ] = "AxisUnits" ;
        string_table_2["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table_2["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table_2["PopulationGroups"   ] = "PopulationGroups" ;
        string_table_2["ResultUnits"        ] = "ResultUnits" ;
        string_table_2["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table_2["ResultValues"       ] = "rv" ;
        string_table_2["DistributionValues" ] = "DistributionValues" ;

        suids::suid node_suid_1 ;
        node_suid_1.data = 1 ;
        suids::suid node_suid_2 ;
        node_suid_2.data = 2 ;

        INodeContextFake parent_1( node_suid_1 );
        INodeContextFake parent_2( node_suid_2 );

        unique_ptr<NodeDemographics> p_nd_a(  NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_1, &parent_1,  1, "MortalityDistribution", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_a2( NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_1, &parent_1,  1, "MortalityDistribution", "IndividualAttribute") );

        CHECK( *p_nd_a  == *p_nd_a  );
        CHECK( *p_nd_a  == *p_nd_a2 );
        CHECK( *p_nd_a2 == *p_nd_a  );

        unique_ptr<NodeDemographics> p_nd_b( NodeDemographicsFactory::CreateNodeDemographics( dist_json_2, &string_table_1, &parent_1,  1, "MortalityDistribution", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_c( NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_2, &parent_1,  1, "MortalityDistribution", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_d( NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_1, &parent_2,  1, "MortalityDistribution", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_e( NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_1, &parent_1,  2, "MortalityDistribution", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_f( NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_1, &parent_1,  1, "222222222222222222222", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_g( NodeDemographicsFactory::CreateNodeDemographics( dist_json_1, &string_table_1, &parent_1,  1, "MortalityDistribution", "2222222222222222222") );

        CHECK( *p_nd_a != *p_nd_b );
        CHECK( *p_nd_a != *p_nd_c );
        CHECK( *p_nd_a != *p_nd_d );
        CHECK( *p_nd_a != *p_nd_e );
        CHECK( *p_nd_a != *p_nd_f );
        CHECK( *p_nd_a != *p_nd_g );

        CHECK( *p_nd_b != *p_nd_a );
        CHECK( *p_nd_c != *p_nd_a );
        CHECK( *p_nd_d != *p_nd_a );
        CHECK( *p_nd_e != *p_nd_a );
        CHECK( *p_nd_f != *p_nd_a );
        CHECK( *p_nd_g != *p_nd_a );
    }

    TEST(TestNodeDemographicsDistributionEquality)
    {
        std::string dist_str = "" ;
        dist_str += "{" ;
        dist_str += "\"AxisNames\": [ \"gender\", \"age\" ],";
        dist_str += "\"AxisScaleFactors\": [ 1, 365 ],";
        dist_str += "\"PopulationGroups\": [";
        dist_str += "    [ 0, 1 ]," ;
        dist_str += "    [ 0, 100, 2000 ]";
        dist_str += "],";
        dist_str += "\"ResultScaleFactor\": 0.1,";
        dist_str += "\"ResultValues\": [";
        dist_str += "    [ 0, 20.0, 400.0 ],";
        dist_str += "    [ 0, 30.0, 500.0 ]";
        dist_str += "]" ;
        dist_str += "}" ;

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        suids::suid node_suid ;
        node_suid.data = 1 ;

        INodeContextFake parent( node_suid );

        unique_ptr<NodeDemographics> p_nd_1( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent,  1, "MortalityDistribution", "IndividualAttribute") );
        unique_ptr<NodeDemographics> p_nd_2( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent,  2, "MortalityDistribution", "IndividualAttribute") );

        std::vector<int> num_pop_groups_1;
        num_pop_groups_1.push_back( 2 );
        num_pop_groups_1.push_back( 3 );

        std::vector<int> num_pop_groups_2;
        num_pop_groups_2.push_back( 7 );
        num_pop_groups_2.push_back( 3 );

        std::vector<double> x_axis_1 ;
        x_axis_1.push_back( 0.0 );
        x_axis_1.push_back( 1.0 );

        std::vector<double> x_axis_2 ;
        x_axis_2.push_back( 7.0 );
        x_axis_2.push_back( 1.0 );

        std::vector<double> y_axis_1 ;
        y_axis_1.push_back(      0.0 );
        y_axis_1.push_back(  36500.0 );
        y_axis_1.push_back( 730000.0 );

        std::vector<double> y_axis_2 ;
        y_axis_2.push_back(      0.0 );
        y_axis_2.push_back(  36500.0 );
        y_axis_2.push_back( 777777.0 );

        std::vector< std::vector<double> > pop_groups_1;
        pop_groups_1.push_back( x_axis_1 );
        pop_groups_1.push_back( y_axis_1 );

        std::vector< std::vector<double> > pop_groups_2;
        pop_groups_2.push_back( x_axis_2 );
        pop_groups_2.push_back( y_axis_2 );

        std::vector<double> result_values_1 ;
        result_values_1.push_back(  0.0 );
        result_values_1.push_back(  2.0 );
        result_values_1.push_back( 40.0 );
        result_values_1.push_back(  0.0 );
        result_values_1.push_back(  3.0 );
        result_values_1.push_back( 50.0 );

        std::vector<double> result_values_2 ;
        result_values_2.push_back(   0.0 );
        result_values_2.push_back(   2.0 );
        result_values_2.push_back(  40.0 );
        result_values_2.push_back(   0.0 );
        result_values_2.push_back( 777.0 );
        result_values_2.push_back(  50.0 );

        std::vector<double> dv_1 ;
        dv_1.push_back( 1.0 );
        dv_1.push_back( 2.0 );

        std::vector<double> dv_2 ;
        dv_2.push_back( 77.0 );
        dv_2.push_back(  2.0 );

        std::vector< std::vector<double> > dist_values_1 ;
        dist_values_1.push_back( dv_1 );

        std::vector< std::vector<double> > dist_values_2 ;
        dist_values_1.push_back( dv_2 );

        unique_ptr<NodeDemographicsDistribution> p_dist_a(  NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 2, num_pop_groups_1, pop_groups_1, result_values_1, dist_values_1 ) );
        unique_ptr<NodeDemographicsDistribution> p_dist_a2( NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 2, num_pop_groups_1, pop_groups_1, result_values_1, dist_values_1 ) );

        CHECK( *p_dist_a  == *p_dist_a  );
        CHECK( *p_dist_a  == *p_dist_a2 );
        CHECK( *p_dist_a2 == *p_dist_a  );

        unique_ptr<NodeDemographicsDistribution> p_dist_b(  NodeDemographicsDistribution::CreateDistribution( *p_nd_2, 2, num_pop_groups_1, pop_groups_1, result_values_1, dist_values_1 ) );
        unique_ptr<NodeDemographicsDistribution> p_dist_c(  NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 7, num_pop_groups_1, pop_groups_1, result_values_1, dist_values_1 ) );
        unique_ptr<NodeDemographicsDistribution> p_dist_d(  NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 2, num_pop_groups_2, pop_groups_1, result_values_1, dist_values_1 ) );
        unique_ptr<NodeDemographicsDistribution> p_dist_e(  NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 2, num_pop_groups_1, pop_groups_2, result_values_1, dist_values_1 ) );
        unique_ptr<NodeDemographicsDistribution> p_dist_f(  NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 2, num_pop_groups_1, pop_groups_1, result_values_2, dist_values_1 ) );
        unique_ptr<NodeDemographicsDistribution> p_dist_g(  NodeDemographicsDistribution::CreateDistribution( *p_nd_1, 2, num_pop_groups_1, pop_groups_1, result_values_1, dist_values_2 ) );

        CHECK( *p_dist_a  != *p_dist_b  );
        CHECK( *p_dist_a  != *p_dist_c  );
        CHECK( *p_dist_a  != *p_dist_d  );
        CHECK( *p_dist_a  != *p_dist_e  );
        CHECK( *p_dist_a  != *p_dist_f  );
        CHECK( *p_dist_a  != *p_dist_g  );

        CHECK( *p_dist_b  != *p_dist_a  );
        CHECK( *p_dist_c  != *p_dist_a  );
        CHECK( *p_dist_d  != *p_dist_a  );
        CHECK( *p_dist_e  != *p_dist_a  );
        CHECK( *p_dist_f  != *p_dist_a  );
        CHECK( *p_dist_g  != *p_dist_a  );
    }

    TEST(TestDistDrawResultValue)
    {
        std::string dist_str = "" ;
        dist_str += "{" ;
        dist_str += "\"AxisNames\": [ \"gender\", \"age\" ],";
        dist_str += "\"AxisScaleFactors\": [ 1, 365 ],";
        dist_str += "\"PopulationGroups\": [";
        dist_str += "    [ 0, 1 ]," ;
        dist_str += "    [ 0, 10, 20 ]";
        dist_str += "],";
        dist_str += "\"ResultScaleFactor\": 0.1,";
        dist_str += "\"ResultValues\": [";
        dist_str += "    [ 0, 2.0, 40.0 ],";
        dist_str += "    [ 0, 3.0, 50.0 ]";
        dist_str += "]" ;
        dist_str += "}" ;

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        INodeContextFake parent ;

        unique_ptr<NodeDemographics> p_nd( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent, 1, "MortalityDistribution", "") );

        unique_ptr<NodeDemographicsDistribution> p_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, "gender", "age" ) );

        // -------------------------------------
        // --- Test that data was read correctly
        // -------------------------------------
        std::vector<int> num_pop_groups;
        num_pop_groups.push_back( 2 );
        num_pop_groups.push_back( 3 );

        std::vector<double> x_axis ;
        x_axis.push_back( 0.0 * 1.0 ); // * 1.0 becasuse that is the 1 from the AxisScaleFactors
        x_axis.push_back( 1.0 * 1.0 );

        // Notice how the AxisScaleFactors have been applied
        std::vector<double> y_axis ;
        y_axis.push_back(  0.0 * 365.0 ); // * 1.0 becasuse that is the 365 from the AxisScaleFactors
        y_axis.push_back( 10.0 * 365.0 );
        y_axis.push_back( 20.0 * 365.0 );

        std::vector< std::vector<double> > pop_groups;
        pop_groups.push_back( x_axis );
        pop_groups.push_back( y_axis );

        // Notice how the ResultScaleFactor has been applied
        std::vector<double> result_values ;
        result_values.push_back(  0.0 * 0.1 ); // * 0.1 becasuse that is the ResultScaleFactor
        result_values.push_back(  0.0 * 0.1 );
        result_values.push_back(  2.0 * 0.1 );
        result_values.push_back(  3.0 * 0.1 );
        result_values.push_back( 40.0 * 0.1 );
        result_values.push_back( 50.0 * 0.1 );

        std::vector< std::vector<double> > dist_values ;

        unique_ptr<NodeDemographicsDistribution> p_exp_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, 2, num_pop_groups, pop_groups, result_values, dist_values ) );

        CHECK( *p_exp_dist == *p_dist );

        // --------------------------------------------------------------
        // --- Test that DrawResultValue() returns the expected results
        // --------------------------------------------------------------
        CHECK_CLOSE( 0.000, p_exp_dist->DrawResultValue( 0.0,    0.0 ), 0.00001 );
        CHECK_CLOSE( 0.000, p_exp_dist->DrawResultValue( 1.0,    0.0 ), 0.00001 );
        CHECK_CLOSE( 0.200, p_exp_dist->DrawResultValue( 0.0, 3650.0 ), 0.00001 );
        CHECK_CLOSE( 0.250, p_exp_dist->DrawResultValue( 0.5, 3650.0 ), 0.00001 );
        CHECK_CLOSE( 0.300, p_exp_dist->DrawResultValue( 1.0, 3650.0 ), 0.00001 );
        CHECK_CLOSE( 4.000, p_exp_dist->DrawResultValue( 0.0, 7300.0 ), 0.00001 );
        CHECK_CLOSE( 4.500, p_exp_dist->DrawResultValue( 0.5, 7300.0 ), 0.00001 );
        CHECK_CLOSE( 5.000, p_exp_dist->DrawResultValue( 1.0, 7300.0 ), 0.00001 );

        CHECK_CLOSE( 2.100, p_exp_dist->DrawResultValue( 0.0, 5475.0 ), 0.00001 );
        CHECK_CLOSE( 2.375, p_exp_dist->DrawResultValue( 0.5, 5475.0 ), 0.00001 );
        CHECK_CLOSE( 2.650, p_exp_dist->DrawResultValue( 1.0, 5475.0 ), 0.00001 );
    }

    TEST(TestDistDrawFromDistribution)
    {
        std::string dist_str = "" ;
        dist_str += "{ ";
        dist_str += "    \"AxisNames\": [ \"age\", \"mucosal_memory\" ],";
        dist_str += "    \"AxisScaleFactors\": [ 365, 1 ],";
        dist_str += "    \"PopulationGroups\": ";
        dist_str += "    [";
        dist_str += "        [ 0, 5 ], ";
        dist_str += "        [ 0, 3, 10, 12 ]";
        dist_str += "    ],";
        dist_str += "    \"ResultScaleFactor\": 1,";
        dist_str += "    \"ResultValues\": [ 0, 3, 12 ],";
        dist_str += "    \"DistributionValues\": ";
        dist_str += "    [";
        dist_str += "        [";
        dist_str += "            [ 0.1, 0.5, 0.9 ], ";
        dist_str += "            [ 0.2, 0.6, 1.0 ], ";
        dist_str += "            [ 0.3, 0.7, 1.1 ], ";
        dist_str += "            [ 0.4, 0.8, 1.2 ]  ";
        dist_str += "        ],";
        dist_str += "        [ ";
        dist_str += "            [ 0.11, 0.51, 0.91 ], ";
        dist_str += "            [ 0.21, 0.61, 1.01 ], ";
        dist_str += "            [ 0.31, 0.71, 1.11 ], ";
        dist_str += "            [ 0.41, 0.81, 1.21 ]  ";
        dist_str += "        ]";
        dist_str += "    ]";
        dist_str += "}";

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        INodeContextFake parent ;

        unique_ptr<NodeDemographics> p_nd( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent, 1, "humoral_memory_distribution1", "") );

        unique_ptr<NodeDemographicsDistribution> p_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, "age", "mucosal_memory" ) );

        // -------------------------------------
        // --- Test that data was read correctly
        // -------------------------------------
        std::vector<int> num_pop_groups;
        num_pop_groups.push_back( 2 );
        num_pop_groups.push_back( 4 );

        std::vector<double> x_axis ;
        x_axis.push_back(    0.0 );
        x_axis.push_back( 1825.0 ); // AxisScaleFactor is applied

        std::vector<double> y_axis ;
        y_axis.push_back(  0.0 );
        y_axis.push_back(  3.0 );
        y_axis.push_back( 10.0 );
        y_axis.push_back( 12.0 );

        std::vector< std::vector<double> > pop_groups;
        pop_groups.push_back( x_axis );
        pop_groups.push_back( y_axis );

        std::vector<double> result_values ;
        result_values.push_back(  0.0 );
        result_values.push_back(  3.0 );
        result_values.push_back( 12.0 );

        std::vector<double> dv_1, dv_2, dv_3, dv_4, dv_5, dv_6, dv_7, dv_8 ;
        dv_1.push_back( 0.10 );
        dv_1.push_back( 0.50 );
        dv_1.push_back( 0.90 );
        dv_2.push_back( 0.20 );
        dv_2.push_back( 0.60 );
        dv_2.push_back( 1.00 );
        dv_3.push_back( 0.30 );
        dv_3.push_back( 0.70 );
        dv_3.push_back( 1.10 );
        dv_4.push_back( 0.40 );
        dv_4.push_back( 0.80 );
        dv_4.push_back( 1.20 );
        dv_5.push_back( 0.11 );
        dv_5.push_back( 0.51 );
        dv_5.push_back( 0.91 );
        dv_6.push_back( 0.21 );
        dv_6.push_back( 0.61 );
        dv_6.push_back( 1.01 );
        dv_7.push_back( 0.31 );
        dv_7.push_back( 0.71 );
        dv_7.push_back( 1.11 );
        dv_8.push_back( 0.41 );
        dv_8.push_back( 0.81 );
        dv_8.push_back( 1.21 );

        // The order here is due to the flattenDist() routine
        std::vector< std::vector<double> > dist_values ;
        dist_values.push_back( dv_1 );
        dist_values.push_back( dv_5 );
        dist_values.push_back( dv_2 );
        dist_values.push_back( dv_6 );
        dist_values.push_back( dv_3 );
        dist_values.push_back( dv_7 );
        dist_values.push_back( dv_4 );
        dist_values.push_back( dv_8 );

        unique_ptr<NodeDemographicsDistribution> p_exp_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, 2, num_pop_groups, pop_groups, result_values, dist_values ) );

        CHECK( *p_exp_dist == *p_dist );

        // --------------------------------------------------------------
        // --- Test that DrawFromDistribution() returns the expected results
        // --------------------------------------------------------------
        float val_2 = p_dist->DrawFromDistribution( 0.25f, 365.0f, 5.0f );
        CHECK_CLOSE( 0.2571428, val_2, 0.0000001 );
    }

    TEST(TestAgeDistribution)
    {
        std::string dist_str = "" ;
        dist_str += "{";
        dist_str += "     \"NumDistributionAxes\": 0,";
        dist_str += "     \"ResultUnits\": \"years\",";
        dist_str += "     \"ResultScaleFactor\": 365,";
        dist_str += "     \"AxisScaleFactors\": 1,";
        dist_str += "     \"ResultValues\"      : [    1,    2,   10,   20,   30,   40,   50,   60,   70,  80,  90 ],";
        dist_str += "     \"DistributionValues\": [ 0.10, 0.20, 0.30, 0.40, 0.45, 0.55, 0.70, 0.75, 0.90,   1,   1 ] ";
        dist_str += "}";

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        INodeContextFake parent ;

        unique_ptr<NodeDemographics> p_nd( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent, 1, "AgeDistribution", "") );

        unique_ptr<NodeDemographicsDistribution> p_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd ) );

        PSEUDO_DES rng( 42 );

        std::vector<float> hist( 13, 0.0 );
        std::vector<float> bin_maxes = { 0.1f, 1.0f, 2.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f };
        CHECK_EQUAL( hist.size(), bin_maxes.size() );

        int num_draws = 10000000;
        for( int i = 0; i < num_draws; ++i )
        {
            float val = p_dist->DrawFromDistribution( rng.e() )/365.0;
            bool found = false;
            for( int j = 0; !found && (j < bin_maxes.size()); ++j )
            {
                if( val < bin_maxes[ j ] )
                {
                    hist[ j ] += 1.0;
                    found = true;
                }
            }

        }

        for( int i = 0; i < hist.size(); ++i )
        {
            //printf( "%f-%f-%f\n", bin_maxes[ i ], hist[ i ], hist[ i ]/float(num_draws) );
            hist[ i ] = hist[ i ] / float( num_draws );
        }

        // ---------------------------------------------------------------------------------------
        // --- Notice   how the first  value in ResultValues is 1 with a DistributionValue of 0.1.
        // --- Next, notice the second value in ResultValues is 2 with a DistributionValue of 0.1.
        // --- See how we have 0 peple in the less than 0.1 and 1.0 bins.  The first ResultVale=1
        // --- makes this the max AND the min of the first bin.  However, notice how the fraction
        // --- of people less than 2 is 0.2 which means that 20% of people are 1-2.
        // ---
        // --- On the opposite end, notice how ResultValue = 80 and DistributionValue of 1 means
        // --- that there are no people greater than 80.
        // ---
        // --- Therefore, ResultValues are maximums and the first value is a maximum AND a minimum.
        // --- This means that users should always have a first values of ResultValue = 0 and
        // --- DistributionValue = 0.0
        // ---------------------------------------------------------------------------------------
        //                                            // bin_maxes ResultValues DistributionValues
        CHECK_CLOSE( 0.000000, hist[  0 ], 0.000001); //    0.1         
        CHECK_CLOSE( 0.000000, hist[  1 ], 0.000001); //    1.0          1           0.10
        CHECK_CLOSE( 0.199738, hist[  2 ], 0.000001); //    2.0          2           0.20
        CHECK_CLOSE( 0.099975, hist[  3 ], 0.000001); //   10.0         10           0.30
        CHECK_CLOSE( 0.100082, hist[  4 ], 0.000001); //   20.0         20           0.40
        CHECK_CLOSE( 0.049996, hist[  5 ], 0.000001); //   30.0         30           0.45
        CHECK_CLOSE( 0.099941, hist[  6 ], 0.000001); //   40.0         40           0.55
        CHECK_CLOSE( 0.150045, hist[  7 ], 0.000001); //   50.0         50           0.70
        CHECK_CLOSE( 0.050097, hist[  8 ], 0.000001); //   60.0         60           0.75
        CHECK_CLOSE( 0.150094, hist[  9 ], 0.000001); //   70.0         70           0.90
        CHECK_CLOSE( 0.100031, hist[ 10 ], 0.000001); //   80.0         80           1
        CHECK_CLOSE( 0.000000, hist[ 11 ], 0.000001); //   90.0         90           1
        CHECK_CLOSE( 0.000000, hist[ 12 ], 0.000001); //  100.0    
    }

    TEST(TestFertilityDistribution)
    {
        std::string dist_str = "" ;
        dist_str += "{\n";
        dist_str += "     \"AxisNames\": [\"age\", \"year\"],\n";
        dist_str += "     \"AxisUnits\": [\"years\", \"simulation_year\"],\n";
        dist_str += "     \"AxisScaleFactors\": [365, 1],\n";
        dist_str += "     \"NumDistributionAxes\": 2,\n";
        dist_str += "     \"ResultUnits\": \"annual births per 1000 individuals\",\n";
        dist_str += "     \"ResultScaleFactor\": 2.7397260273972604e-06,\n";
        dist_str += "     \"PopulationGroups\": [\n";
        dist_str += "           [15.0, 24.999, 25.0, 34.999, 35.0, 44.999, 45.0, 125.0],\n";
        dist_str += "           [2010.0, 2014.999, 2015.0, 2019.999, 2020.0, 2024.999, 2025.0, 2029.999, 2030.0, 2034.999, 2035.0, 2039.999]\n";
        dist_str += "      ],\n";
        dist_str += "     \"ResultValues\" : [\n";
        dist_str += "           [103.3, 103.3,  77.5,  77.5,  65.5,  65.5,  55.5,  55.5,  47.3,  47.3,  40.5,  40.5], \n";
        dist_str += "           [103.3, 103.3,  77.5,  77.5,  65.5,  65.5,  55.5,  55.5,  47.3,  47.3,  40.5,  40.5], \n";
        dist_str += "           [265.0, 265.0, 278.7, 278.7, 275.4, 275.4, 271.0, 271.0, 265.7, 265.7, 260.0, 260.0], \n";
        dist_str += "           [265.0, 265.0, 278.7, 278.7, 275.4, 275.4, 271.0, 271.0, 265.7, 265.7, 260.0, 260.0], \n";
        dist_str += "           [152.4, 152.4, 129.2, 129.2, 115.9, 115.9, 104.4, 104.4,  94.5,  94.5,  86.0,  86.0], \n";
        dist_str += "           [152.4, 152.4, 129.2, 129.2, 115.9, 115.9, 104.4, 104.4,  94.5,  94.5,  86.0,  86.0], \n";
        dist_str += "           [ 19.9,  19.9,  14.6,  14.6,  12.1,  12.1,  10.1,  10.1,   8.5,   8.5 ,  7.1,   7.1], \n";
        dist_str += "           [ 19.9,  19.9,  14.6,  14.6,  12.1,  12.1,  10.1,  10.1,   8.5,   8.5 ,  7.1,   7.1]  \n";
        dist_str += "      ]\n";
        dist_str += "}\n";
        printf( "%s\n", dist_str.c_str() );

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        INodeContextFake parent ;

        unique_ptr<NodeDemographics> p_nd( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent, 1, "AgeDistribution", "") );

        unique_ptr<NodeDemographicsDistribution> p_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, "age", "year" ) );

        CHECK_CLOSE( 103.3/365.0/1000.0, p_dist->DrawResultValue( 15.000 * 365.0, 2010.000 ), 0.000001);
        CHECK_CLOSE( 103.3/365.0/1000.0, p_dist->DrawResultValue( 24.999 * 365.0, 2010.000 ), 0.000001);
        CHECK_CLOSE( 103.3/365.0/1000.0, p_dist->DrawResultValue( 24.999 * 365.0, 2014.999 ), 0.000001);
        CHECK_CLOSE( 103.3/365.0/1000.0, p_dist->DrawResultValue( 15.000 * 365.0, 2014.999 ), 0.000001);
        CHECK_CLOSE( 103.3/365.0/1000.0, p_dist->DrawResultValue( 20.000 * 365.0, 2012.500 ), 0.000001);

        CHECK_CLOSE(  77.5/365.0/1000.0, p_dist->DrawResultValue( 15.000 * 365.0, 2015.000 ), 0.000001);
        CHECK_CLOSE(  77.5/365.0/1000.0, p_dist->DrawResultValue( 24.999 * 365.0, 2015.000 ), 0.000001);
        CHECK_CLOSE(  77.5/365.0/1000.0, p_dist->DrawResultValue( 24.999 * 365.0, 2019.999 ), 0.000001);
        CHECK_CLOSE(  77.5/365.0/1000.0, p_dist->DrawResultValue( 15.000 * 365.0, 2019.999 ), 0.000001);
        CHECK_CLOSE(  77.5/365.0/1000.0, p_dist->DrawResultValue( 20.000 * 365.0, 2017.500 ), 0.000001);

        CHECK_CLOSE(  65.5/365.0/1000.0, p_dist->DrawResultValue( 15.000 * 365.0, 2020.000 ), 0.000001);
        CHECK_CLOSE(  65.5/365.0/1000.0, p_dist->DrawResultValue( 24.999 * 365.0, 2020.000 ), 0.000001);
        CHECK_CLOSE(  65.5/365.0/1000.0, p_dist->DrawResultValue( 24.999 * 365.0, 2024.999 ), 0.000001);
        CHECK_CLOSE(  65.5/365.0/1000.0, p_dist->DrawResultValue( 15.000 * 365.0, 2024.999 ), 0.000001);
        CHECK_CLOSE(  65.5/365.0/1000.0, p_dist->DrawResultValue( 20.000 * 365.0, 2022.500 ), 0.000001);

        CHECK_CLOSE( 275.4/365.0/1000.0, p_dist->DrawResultValue( 25.000 * 365.0, 2020.000 ), 0.000001);
        CHECK_CLOSE( 275.4/365.0/1000.0, p_dist->DrawResultValue( 34.999 * 365.0, 2020.000 ), 0.000001);
        CHECK_CLOSE( 275.4/365.0/1000.0, p_dist->DrawResultValue( 34.999 * 365.0, 2024.999 ), 0.000001);
        CHECK_CLOSE( 275.4/365.0/1000.0, p_dist->DrawResultValue( 25.000 * 365.0, 2024.999 ), 0.000001);
        CHECK_CLOSE( 275.4/365.0/1000.0, p_dist->DrawResultValue( 30.000 * 365.0, 2022.500 ), 0.000001);

        CHECK_CLOSE( 115.9/365.0/1000.0, p_dist->DrawResultValue( 35.000 * 365.0, 2020.000 ), 0.000001);
        CHECK_CLOSE( 115.9/365.0/1000.0, p_dist->DrawResultValue( 44.999 * 365.0, 2020.000 ), 0.000001);
        CHECK_CLOSE( 115.9/365.0/1000.0, p_dist->DrawResultValue( 44.999 * 365.0, 2024.999 ), 0.000001);
        CHECK_CLOSE( 115.9/365.0/1000.0, p_dist->DrawResultValue( 35.000 * 365.0, 2024.999 ), 0.000001);
        CHECK_CLOSE( 115.9/365.0/1000.0, p_dist->DrawResultValue( 40.000 * 365.0, 2022.500 ), 0.000001);

        CHECK_CLOSE(  86.0/365.0/1000.0, p_dist->DrawResultValue( 35.000 * 365.0, 2035.000 ), 0.000001);
        CHECK_CLOSE(  86.0/365.0/1000.0, p_dist->DrawResultValue( 44.999 * 365.0, 2035.000 ), 0.000001);
        CHECK_CLOSE(  86.0/365.0/1000.0, p_dist->DrawResultValue( 44.999 * 365.0, 2039.999 ), 0.000001);
        CHECK_CLOSE(  86.0/365.0/1000.0, p_dist->DrawResultValue( 35.000 * 365.0, 2039.999 ), 0.000001);
        CHECK_CLOSE(  86.0/365.0/1000.0, p_dist->DrawResultValue( 40.000 * 365.0, 2037.500 ), 0.000001);

        CHECK_CLOSE(   7.1/365.0/1000.0, p_dist->DrawResultValue( 45.000 * 365.0, 2035.000 ), 0.000001);
        CHECK_CLOSE(   7.1/365.0/1000.0, p_dist->DrawResultValue( 99.999 * 365.0, 2035.000 ), 0.000001);
        CHECK_CLOSE(   7.1/365.0/1000.0, p_dist->DrawResultValue( 99.999 * 365.0, 2039.999 ), 0.000001);
        CHECK_CLOSE(   7.1/365.0/1000.0, p_dist->DrawResultValue( 45.000 * 365.0, 2039.999 ), 0.000001);
        CHECK_CLOSE(   7.1/365.0/1000.0, p_dist->DrawResultValue( 70.000 * 365.0, 2037.500 ), 0.000001);

        // ----------------------
        // --- Year outside range
        // ----------------------
        CHECK_CLOSE(  86.0/365.0/1000.0, p_dist->DrawResultValue( 40.000 * 365.0, 2050.000 ), 0.000001);

        // ----------------------
        // --- Age outside range
        // ----------------------
        CHECK_CLOSE(   7.1/365.0/1000.0, p_dist->DrawResultValue(130.000 * 365.0, 2037.500 ), 0.000001);
    }



#endif //INCLUDED

    TEST(TestDistFake)
    {
        std::string dist_str = "" ;
        dist_str += "{ ";
        dist_str += "    \"NumDistributionAxes\": 1," ;
        dist_str += "    \"AxisNames\": [ \"age\" ]," ;
        dist_str += "    \"AxisUnits\": [ \"years\" ],";
        dist_str += "    \"AxisScaleFactors\": [ 365 ]," ;
        dist_str += "    \"NumPopulationGroups\": [ 2 ]," ;
        dist_str += "    \"PopulationGroups\": " ;
        dist_str += "     [" ;
        dist_str += "        [ 0, 5 ]";
        dist_str += "     ]," ;
        dist_str += "    \"ResultUnits\": \"time\",";
        dist_str += "    \"ResultScaleFactor\": 1, ";
        dist_str += "    \"ResultValues\": " ;
        dist_str += "    [";
        dist_str += "        5, 6" ;
        dist_str += "    ],";
        dist_str += "    \"DistributionValues\": " ;
        dist_str += "    [" ;
        dist_str += "        [ 0.7, 0.8 ], " ;
        dist_str += "        [ 0.9, 1.0 ] ";
        dist_str += "    ]" ;
        dist_str += "}";

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        INodeContextFake parent ;

        unique_ptr<NodeDemographics> p_nd( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent, 1, "fake_time_since_infection_distribution", "") );

        unique_ptr<NodeDemographicsDistribution> p_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, "age" ) );

        float val_a2 = p_dist->DrawFromDistribution( 0.70f,         0.5f  ); // half-day old
        float val_b2 = p_dist->DrawFromDistribution( 0.70f,  1.0f*365.0f  );
        float val_c2 = p_dist->DrawFromDistribution( 0.70f,  2.5f*365.0f  );
        float val_d2 = p_dist->DrawFromDistribution( 0.70f,  5.0f*365.0f  );
        float val_e2 = p_dist->DrawFromDistribution( 0.70f,  7.5f*365.0f  );
        float val_f2 = p_dist->DrawFromDistribution( 0.70f, 10.0f*365.0f  );

        // Any random number <= 0.7 gets 5.0
        CHECK_EQUAL( 5.0f, val_a2 );
        CHECK_EQUAL( 5.0f, val_b2 );
        CHECK_EQUAL( 5.0f, val_c2 );
        CHECK_EQUAL( 5.0f, val_d2 );
        CHECK_EQUAL( 5.0f, val_e2 );
        CHECK_EQUAL( 5.0f, val_f2 );

        float val_z3 = p_dist->DrawFromDistribution( 0.75f,         0.0f  );
        float val_a3 = p_dist->DrawFromDistribution( 0.75f,         0.5f  ); // half-day old
        float val_b3 = p_dist->DrawFromDistribution( 0.75f,  1.0f*365.0f  );
        float val_c3 = p_dist->DrawFromDistribution( 0.75f,  2.5f*365.0f  );
        float val_d3 = p_dist->DrawFromDistribution( 0.75f,  5.0f*365.0f  );
        float val_e3 = p_dist->DrawFromDistribution( 0.75f,  7.5f*365.0f  );
        float val_f3 = p_dist->DrawFromDistribution( 0.75f, 10.0f*365.0f  );

        CHECK_EQUAL( 5.50f, val_z3 );
        CHECK_CLOSE( 5.50f, val_a3, 0.001 );
        CHECK_EQUAL( 5.40f, val_b3 );
        CHECK_EQUAL( 5.25f, val_c3 );
        CHECK_EQUAL( 5.00f, val_d3 );
        CHECK_EQUAL( 5.00f, val_e3 );
        CHECK_EQUAL( 5.00f, val_f3 );

        float val_z4 = p_dist->DrawFromDistribution( 0.80f,         0.0f  );
        float val_a4 = p_dist->DrawFromDistribution( 0.80f,         0.5f  ); // half-day old
        float val_b4 = p_dist->DrawFromDistribution( 0.80f,  1.0f*365.0f  );
        float val_c4 = p_dist->DrawFromDistribution( 0.80f,  2.5f*365.0f  );
        float val_d4 = p_dist->DrawFromDistribution( 0.80f,  5.0f*365.0f  );
        float val_e4 = p_dist->DrawFromDistribution( 0.80f,  7.5f*365.0f  );
        float val_f4 = p_dist->DrawFromDistribution( 0.80f, 10.0f*365.0f  );

        CHECK_EQUAL( 6.0f, val_z4 );
        CHECK_CLOSE( 6.0f, val_a4, 0.001 );
        CHECK_CLOSE( 5.8f, val_b4, 0.001 );
        CHECK_EQUAL( 5.5f, val_c4 );
        CHECK_EQUAL( 5.0f, val_d4 );
        CHECK_EQUAL( 5.0f, val_e4 );
        CHECK_EQUAL( 5.0f, val_f4 );

        float val_z5 = p_dist->DrawFromDistribution( 0.85f,         0.0f  );
        float val_a5 = p_dist->DrawFromDistribution( 0.85f,         0.5f  ); // half-day old
        float val_b5 = p_dist->DrawFromDistribution( 0.85f,  1.0f*365.0f  );
        float val_c5 = p_dist->DrawFromDistribution( 0.85f,  2.5f*365.0f  );
        float val_d5 = p_dist->DrawFromDistribution( 0.85f,  5.0f*365.0f  );
        float val_e5 = p_dist->DrawFromDistribution( 0.85f,  7.5f*365.0f  );
        float val_f5 = p_dist->DrawFromDistribution( 0.85f, 10.0f*365.0f  );

        CHECK_EQUAL( 6.0f, val_z5 );
        CHECK_CLOSE( 6.0f, val_a5, 0.001 );
        CHECK_CLOSE( 5.8f, val_b5, 0.001 );
        CHECK_EQUAL( 5.5f, val_c5 );
        CHECK_EQUAL( 5.0f, val_d5 );
        CHECK_EQUAL( 5.0f, val_e5 );
        CHECK_EQUAL( 5.0f, val_f5 );

        float val_z6 = p_dist->DrawFromDistribution( 0.90f,         0.0f  );
        float val_a6 = p_dist->DrawFromDistribution( 0.90f,         0.5f  ); // half-day old
        float val_b6 = p_dist->DrawFromDistribution( 0.90f,  1.0f*365.0f  );
        float val_c6 = p_dist->DrawFromDistribution( 0.90f,  2.5f*365.0f  );
        float val_d6 = p_dist->DrawFromDistribution( 0.90f,  5.0f*365.0f  );
        float val_e6 = p_dist->DrawFromDistribution( 0.90f,  7.5f*365.0f  );
        float val_f6 = p_dist->DrawFromDistribution( 0.90f, 10.0f*365.0f  );

        CHECK_EQUAL( 6.0f, val_z6 );
        CHECK_CLOSE( 6.0f, val_a6, 0.001 );
        CHECK_CLOSE( 5.8f, val_b6, 0.001 );
        CHECK_EQUAL( 5.5f, val_c6 );
        CHECK_EQUAL( 5.0f, val_d6 );
        CHECK_EQUAL( 5.0f, val_e6 );
        CHECK_EQUAL( 5.0f, val_f6 );

        float val_z7 = p_dist->DrawFromDistribution( 0.95f,         0.0f  );
        float val_a7 = p_dist->DrawFromDistribution( 0.95f,         0.5f  ); // half-day old
        float val_b7 = p_dist->DrawFromDistribution( 0.95f,  1.0f*365.0f  );
        float val_c7 = p_dist->DrawFromDistribution( 0.95f,  2.5f*365.0f  );
        float val_d7 = p_dist->DrawFromDistribution( 0.95f,  5.0f*365.0f  );
        float val_e7 = p_dist->DrawFromDistribution( 0.95f,  7.5f*365.0f  );
        float val_f7 = p_dist->DrawFromDistribution( 0.95f, 10.0f*365.0f  );

        CHECK_EQUAL( 6.00f, val_z7 );
        CHECK_CLOSE( 6.00f, val_a7, 0.001 );
        CHECK_CLOSE( 5.90f, val_b7, 0.001 );
        CHECK_EQUAL( 5.75f, val_c7 );
        CHECK_EQUAL( 5.50f, val_d7 );
        CHECK_EQUAL( 5.50f, val_e7 );
        CHECK_EQUAL( 5.50f, val_f7 );

        float val_z8 = p_dist->DrawFromDistribution( 1.0f,         0.0f  );
        float val_a8 = p_dist->DrawFromDistribution( 1.0f,         0.5f  ); // half-day old
        float val_b8 = p_dist->DrawFromDistribution( 1.0f,  1.0f*365.0f  );
        float val_c8 = p_dist->DrawFromDistribution( 1.0f,  2.5f*365.0f  );
        float val_d8 = p_dist->DrawFromDistribution( 1.0f,  5.0f*365.0f  );
        float val_e8 = p_dist->DrawFromDistribution( 1.0f,  7.5f*365.0f  );
        float val_f8 = p_dist->DrawFromDistribution( 1.0f, 10.0f*365.0f  );

        CHECK_EQUAL( 6.0f, val_z8 );
        CHECK_EQUAL( 6.0f, val_a8 );
        CHECK_EQUAL( 6.0f, val_b8 );
        CHECK_EQUAL( 6.0f, val_c8 );
        CHECK_EQUAL( 6.0f, val_d8 );
        CHECK_EQUAL( 6.0f, val_e8 );
        CHECK_EQUAL( 6.0f, val_f8 );
    }

    TEST(TestDistSupportsTwoAxisButOneDefined)
    {
        std::string dist_str = "" ;
        dist_str += "{ ";
        dist_str += "    \"AxisNames\": [ \"age\" ],";
        dist_str += "    \"AxisScaleFactors\": [ 365 ],";
        dist_str += "    \"PopulationGroups\": ";
        dist_str += "    [";
        dist_str += "        [ 0, 5 ] ";
        dist_str += "    ],";
        dist_str += "    \"ResultScaleFactor\": 1,";
        dist_str += "    \"ResultValues\": [ 0, 3, 12 ],";
        dist_str += "    \"DistributionValues\": ";
        dist_str += "    [";
        dist_str += "        [ 0.1, 0.5, 0.9 ], ";
        dist_str += "        [ 0.2, 0.6, 1.0 ] ";
        dist_str += "    ]";
        dist_str += "}";

        JsonObjectDemog dist_json;
        dist_json.Parse( dist_str.c_str() );

        std::map<std::string, std::string> string_table ;
        string_table["NumDistributionAxes"] = "NumDistributionAxes" ;
        string_table["AxisNames"          ] = "AxisNames" ;
        string_table["AxisUnits"          ] = "AxisUnits" ;
        string_table["AxisScaleFactors"   ] = "AxisScaleFactors" ;
        string_table["NumPopulationGroups"] = "NumPopulationGroups" ;
        string_table["PopulationGroups"   ] = "PopulationGroups" ;
        string_table["ResultUnits"        ] = "ResultUnits" ;
        string_table["ResultScaleFactor"  ] = "ResultScaleFactor" ;
        string_table["ResultValues"       ] = "ResultValues" ;
        string_table["DistributionValues" ] = "DistributionValues" ;

        INodeContextFake parent ;

        unique_ptr<NodeDemographics> p_nd( NodeDemographicsFactory::CreateNodeDemographics( dist_json, &string_table, &parent, 1, "humoral_memory_distribution1", "") );

        unique_ptr<NodeDemographicsDistribution> p_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, "age", "mucosal_memory" ) );

        // -------------------------------------
        // --- Test that data was read correctly
        // -------------------------------------
        std::vector<int> num_pop_groups;
        num_pop_groups.push_back( 2 );

        std::vector<double> x_axis ;
        x_axis.push_back(    0.0 );
        x_axis.push_back( 1825.0 ); // AxisScaleFactor is applied

        std::vector< std::vector<double> > pop_groups;
        pop_groups.push_back( x_axis );

        std::vector<double> result_values ;
        result_values.push_back(  0.0 );
        result_values.push_back(  3.0 );
        result_values.push_back( 12.0 );

        std::vector<double> dv_1, dv_2 ;
        dv_1.push_back( 0.10 );
        dv_1.push_back( 0.50 );
        dv_1.push_back( 0.90 );
        dv_2.push_back( 0.20 );
        dv_2.push_back( 0.60 );
        dv_2.push_back( 1.00 );

        // The order here is due to the flattenDist() routine
        std::vector< std::vector<double> > dist_values ;
        dist_values.push_back( dv_1 );
        dist_values.push_back( dv_2 );

        unique_ptr<NodeDemographicsDistribution> p_exp_dist( NodeDemographicsDistribution::CreateDistribution( *p_nd, 1, num_pop_groups, pop_groups, result_values, dist_values ) );

        CHECK( *p_exp_dist == *p_dist );

        // -----------------------------------------------------------------------
        // --- Test that DrawFromDistribution() returns the expected results
        // !!! NOTE: We have one axis defined but we are still providing the method
        // !!!       both arguments.  The method should ignore the second
        // !!!       argument and just use age for only axis defined in the data..
        // -----------------------------------------------------------------------
        float val_2 = p_dist->DrawFromDistribution( 0.25f, 365.0f, 5.0f );
        CHECK_CLOSE( 0.975, val_2, 0.0000001 );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDefaultDemographics)
    {
        // ------------------------------------------------------------------
        // --- Verify that with the Default Demographics selected that there
        // --- are 100 nodes with 1,000 initial people in each node.
        // ------------------------------------------------------------------
        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, false, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL(100, nodeIDs.size());

        // -----------------------------------------------------------------------------------
        // --- When creating the default demographics, the factory will output a json file in
        // --- the output directory containing a representation of the data used.
        // -----------------------------------------------------------------------------------
        std::string default_fn = "testdata/NodeDemographicsTest/output/DefaultDemographics.json" ;
        CHECK( FileSystem::FileExists( default_fn ) );

        JsonObjectDemog default_demog_json ;
        default_demog_json.ParseFile( default_fn.c_str() );

        int num_nodes = default_demog_json["Metadata"]["NodeCount"].AsInt();
        CHECK_EQUAL( 100, num_nodes );

        for( int i = 0 ; i < nodeIDs.size() ; i++ )
        {
            uint32_t data_node_id = nodeIDs[i] ;
            uint32_t file_node_id = default_demog_json["Nodes"][IndexType(i)]["NodeID"].AsUint();
            CHECK_EQUAL( data_node_id, file_node_id );

            suids::suid node_suid = GetNextNodeSuid();
            node_id_suid_map.insert( nodeid_suid_pair( data_node_id, node_suid ) );
            INodeContextFake ncf( node_suid );

            unique_ptr<NodeDemographics> p_node_demo( factory->CreateNodeDemographics(&ncf) );

            int32_t data_initial_population = (*p_node_demo)["NodeAttributes"]["InitialPopulation"].AsInt();
            CHECK_EQUAL( 1000, data_initial_population );

            double data_lat = (*p_node_demo)["NodeAttributes"]["Latitude"].AsDouble();
            double file_lat = default_demog_json["Nodes"][IndexType(i)]["NodeAttributes"]["Latitude"].AsDouble();

            CHECK_EQUAL( data_lat, file_lat );
        }
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestReadLargeFile)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("demographics_TestReadLargeFile.compiled.json") ) ;

        nodeid_suid_map_t node_id_suid_map;

        Stopwatch watch ;
        watch.Start();
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        watch.Stop();
        double ms = watch.ResultNanoseconds() / 1000000.0;
        
        ostringstream msg ;
        msg << "Duration (ms) = " << ms << endl ;
        PrintDebug( msg.str() );

#ifdef _DEBUG
        CHECK( ms < 5000 );
#else
        CHECK( ms < 2000 );
#endif

        const vector<uint32_t>& ids = factory->GetNodeIDs();
        CHECK_EQUAL(30145, ids.size());
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestAgeAndAccess)
    {
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("hint_ageandaccess_demographics.compiled.json;hint_ageandaccess_overlay.compiled.json") ) ;

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL(1, nodeIDs.size());
        CHECK_EQUAL(340461476, nodeIDs[0]);

        suids::suid node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( 340461476, node_suid ) );
        INodeContextFake ncf_a( node_suid );

        unique_ptr<NodeDemographics> p_node_demo_a( factory->CreateNodeDemographics(&ncf_a) );

        // -------------------------------------------------
        // --- Check properties that are unique for the node
        // -------------------------------------------------

        double lat_a = (*p_node_demo_a)["NodeAttributes"]["Latitude"           ].AsDouble();
        double lon_a = (*p_node_demo_a)["NodeAttributes"]["Longitude"          ].AsDouble();
        int    pop_a = (*p_node_demo_a)["NodeAttributes"]["InitialPopulation"  ].AsInt();
        int    adf_a = (*p_node_demo_a)["IndividualAttributes"]["AgeDistributionFlag"].AsInt();
        int    pdf_a = (*p_node_demo_a)["IndividualAttributes"]["PrevalenceDistributionFlag"].AsInt();
        string prop_1  = (*p_node_demo_a)["IndividualProperties"][0]["Property"].AsString();
        string prop_2  = (*p_node_demo_a)["IndividualProperties"][1]["Property"].AsString();

        CHECK_CLOSE( -8.5,            lat_a, FLT_EPSILON );
        CHECK_CLOSE( 36.5,            lon_a, FLT_EPSILON );
        CHECK_EQUAL( 1000,            pop_a );
        CHECK_EQUAL( 3,               adf_a );
        CHECK_EQUAL( 0,               pdf_a );
        CHECK_EQUAL( "Accessibility", prop_1 );
        CHECK_EQUAL( "Age_Bin",       prop_2 );
    }

    TEST_FIXTURE( NodeDemographicsFactoryFixture, TestIndividualPropertyOverlays )
    {
        std::vector<std::string> demog_filenames;
        demog_filenames.push_back( "demographics_TestIndividualPropertyOverlays_Base.json" );
        demog_filenames.push_back( "demographics_TestIndividualPropertyOverlays_DefaultOverlay.json" );
        demog_filenames.push_back( "demographics_TestIndividualPropertyOverlays_NodeOverlay.json" );

        NodeDemographicsFactory::SetDemographicsFileList( demog_filenames );

        nodeid_suid_map_t node_id_suid_map;
        unique_ptr<NodeDemographicsFactory> factory( NodeDemographicsFactory::CreateNodeDemographicsFactory( &node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs = factory->GetNodeIDs();
        CHECK_EQUAL( 2, nodeIDs.size() );
        CHECK_EQUAL( 1, nodeIDs[ 0 ] );
        CHECK_EQUAL( 2, nodeIDs[ 1 ] );

        suids::suid node_suid_1 = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( 1, node_suid_1 ) );
        INodeContextFake ncf_1( node_suid_1 );
        unique_ptr<NodeDemographics> p_node_demo_1( factory->CreateNodeDemographics( &ncf_1 ) );

        suids::suid node_suid_2 = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( 2, node_suid_2 ) );
        INodeContextFake ncf_2( node_suid_2 );
        unique_ptr<NodeDemographics> p_node_demo_2( factory->CreateNodeDemographics( &ncf_2 ) );

        // -------------------------------------------------
        // --- Check properties that are unique for the node
        // -------------------------------------------------

        int    pop_1 = (*p_node_demo_1)[ "NodeAttributes" ][ "InitialPopulation" ].AsInt();
        int    pop_2 = (*p_node_demo_2)[ "NodeAttributes" ][ "InitialPopulation" ].AsInt();

        CHECK_EQUAL( 12345, pop_1 );
        CHECK_EQUAL( 67890, pop_2 );

        string prop_1_0 = (*p_node_demo_1)[ "IndividualProperties" ][ 0 ][ "Property" ].AsString();
        string prop_2_0 = (*p_node_demo_2)[ "IndividualProperties" ][ 0 ][ "Property" ].AsString();

        CHECK_EQUAL( "Accessibility", prop_1_0 );
        CHECK_EQUAL( "Accessibility", prop_2_0 );

        string prop_1_0_val_0 = (*p_node_demo_1)[ "IndividualProperties" ][ 0 ][ "Values" ][ 0 ].AsString();
        string prop_1_0_val_1 = (*p_node_demo_1)[ "IndividualProperties" ][ 0 ][ "Values" ][ 1 ].AsString();
        string prop_2_0_val_0 = (*p_node_demo_2)[ "IndividualProperties" ][ 0 ][ "Values" ][ 0 ].AsString();
        string prop_2_0_val_1 = (*p_node_demo_2)[ "IndividualProperties" ][ 0 ][ "Values" ][ 1 ].AsString();

        CHECK_EQUAL( "YES", prop_1_0_val_0 );
        CHECK_EQUAL( "NO",  prop_1_0_val_1 );
        CHECK_EQUAL( "YES", prop_2_0_val_0 );
        CHECK_EQUAL( "NO",  prop_2_0_val_1 );

        double prop_1_0_dis_0 = (*p_node_demo_1)[ "IndividualProperties" ][ 0 ][ "Initial_Distribution" ][ 0 ].AsDouble();
        double prop_1_0_dis_1 = (*p_node_demo_1)[ "IndividualProperties" ][ 0 ][ "Initial_Distribution" ][ 1 ].AsDouble();
        double prop_2_0_dis_0 = (*p_node_demo_2)[ "IndividualProperties" ][ 0 ][ "Initial_Distribution" ][ 0 ].AsDouble();
        double prop_2_0_dis_1 = (*p_node_demo_2)[ "IndividualProperties" ][ 0 ][ "Initial_Distribution" ][ 1 ].AsDouble();

        //CHECK_CLOSE( 0.7, prop_1_0_dis_0, 0.0001 );
        //CHECK_CLOSE( 0.3, prop_1_0_dis_1, 0.0001 );
        //CHECK_CLOSE( 0.8, prop_2_0_dis_0, 0.0001 );
        //CHECK_CLOSE( 0.2, prop_2_0_dis_1, 0.0001 );

        //CHECK_CLOSE( 0.1, prop_1_0_dis_0, 0.0001 );
        //CHECK_CLOSE( 0.9, prop_1_0_dis_1, 0.0001 );
        //CHECK_CLOSE( 0.1, prop_2_0_dis_0, 0.0001 );
        //CHECK_CLOSE( 0.9, prop_2_0_dis_1, 0.0001 );

        CHECK_CLOSE( 0.1, prop_1_0_dis_0, 0.0001 );
        CHECK_CLOSE( 0.9, prop_1_0_dis_1, 0.0001 );
        CHECK_CLOSE( 0.4, prop_2_0_dis_0, 0.0001 );
        CHECK_CLOSE( 0.6, prop_2_0_dis_1, 0.0001 );
    }

    TEST_FIXTURE(NodeDemographicsFactoryFixture, TestDistributionsInOverlays)
    {
        nodeid_suid_map_t node_id_suid_map;
        suids::suid node_suid = GetNextNodeSuid();
        node_id_suid_map.insert( nodeid_suid_pair( 340461476, node_suid ) );
        INodeContextFake ncf( node_suid );

        // ------------------------------------------------------------
        // --- Test that we get the data out of the base file correctly
        // ------------------------------------------------------------
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("Namawala_single_node_demographics.json") ) ;

        unique_ptr<NodeDemographicsFactory> factory_a( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs_a = factory_a->GetNodeIDs();
        CHECK_EQUAL(1, nodeIDs_a.size());
        CHECK_EQUAL(340461476, nodeIDs_a[0]);

        unique_ptr<NodeDemographics> p_node_demo_a( factory_a->CreateNodeDemographics(&ncf) );
        unique_ptr<NodeDemographicsDistribution> p_ndd_mort_a( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo_a)["IndividualAttributes"]["MortalityDistribution"], "gender", "age") );
        const std::vector< std::vector<double> >& r_pop_groups_a = p_ndd_mort_a->GetPopGroups();
        CHECK_EQUAL( 2, r_pop_groups_a.size() );
        CHECK_EQUAL( 2, r_pop_groups_a[0].size() );
        CHECK_EQUAL( 3, r_pop_groups_a[1].size() );

        // ----------------------------------------------------------------------------------------
        // --- Test that adding the overlay overrides the distribution found in the first/base file
        // ----------------------------------------------------------------------------------------
        NodeDemographicsFactory::SetDemographicsFileList( NodeDemographicsFactory::ConvertLegacyStringToSet("Namawala_single_node_demographics.json;Namawala_single_node_demographics_complex_mortality.json") ) ;

        unique_ptr<NodeDemographicsFactory> factory_b( NodeDemographicsFactory::CreateNodeDemographicsFactory(&node_id_suid_map, Environment::getInstance()->Config, true, 10, 1000 ) );
        const vector<uint32_t>& nodeIDs_b = factory_b->GetNodeIDs();
        CHECK_EQUAL(1, nodeIDs_b.size());
        CHECK_EQUAL(340461476, nodeIDs_b[0]);

        unique_ptr<NodeDemographics> p_node_demo_b( factory_b->CreateNodeDemographics(&ncf) );
        unique_ptr<NodeDemographicsDistribution> p_ndd_mort_b( NodeDemographicsDistribution::CreateDistribution( (*p_node_demo_b)["IndividualAttributes"]["MortalityDistribution"], "gender", "age") );
        const std::vector< std::vector<double> >& r_pop_groups_b = p_ndd_mort_b->GetPopGroups();
        CHECK_EQUAL( 2, r_pop_groups_b.size() );
        CHECK_EQUAL( 2, r_pop_groups_b[0].size() );
        CHECK_EQUAL( 5, r_pop_groups_b[1].size() );
    }
}
