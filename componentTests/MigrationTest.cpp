
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "IMigrationInfo.h"
#include "IMigrationInfoVector.h"
#include "SimulationConfig.h"
#include "INodeContextFake.h"
#include "IndividualHumanContextFake.h"
#include "RandomFake.h"
#include "IdmMpi.h"
#include "Instrumentation.h"
#include "VectorCohort.h"
#include "VectorContexts.h"

using namespace Kernel; 



// maybe these shouldn't be protected in Simulation.h
typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
typedef nodeid_suid_map_t::value_type nodeid_suid_pair;


SUITE(MigrationTest)
{
    static int m_NextId = 1 ;

    struct MigrationFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig ;
        RandomFake m_RandomFake ;

        MigrationFixture()
            : m_pSimulationConfig( new SimulationConfig() )
        {
            JsonConfigurable::ClearMissingParameters();

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            string configFilename("testdata/MigrationTest/config.json");
            string inputPath("testdata/MigrationTest");
            string outputPath("testdata/MigrationTest/output");
            string statePath("testdata/MigrationTest");
            string dllPath("testdata/MigrationTest");

            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, dllPath, false );

            m_pSimulationConfig->sim_type = SimType::VECTOR_SIM ;
            m_pSimulationConfig->demographics_initial = true ;
            Environment::setSimulationConfig( m_pSimulationConfig );

        }

        ~MigrationFixture()
        {
            delete m_pMpi;
            Environment::Finalize();
        }
    };

#if 1
    TEST_FIXTURE(MigrationFixture, TestBothGenders)
    {
        std::string config_filename = "testdata/MigrationTest/TestBothGenders_config.json";
        try
        {
            unique_ptr<Configuration> p_config_file( Environment::LoadConfigurationFile( config_filename.c_str() ) );

            unique_ptr<Configuration> p_config( Environment::CopyFromElement( (*p_config_file)["parameters"] ) );

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 4 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "ABC" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( p_config.get(), 
                                                                                                     idreference ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ---------------
            // --- Test Node 2
            // ---------------
            INodeContextFake nc_2( nodeid_suid_map.left.at(2) ) ;
            unique_ptr<IMigrationInfo> p_mi_2( p_mf->CreateMigrationInfo( &nc_2, nodeid_suid_map ) );

            CHECK( p_mi_2->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_2.size() );
            CHECK_EQUAL( 1, reachable_nodes_2[ 0].data );
            CHECK_EQUAL( 3, reachable_nodes_2[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_2[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_2.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 2] );

            // ================
            // === FROM NODE 2
            // ================
            m_RandomFake.SetUL( 2576980377 ); // 0.6

            IndividualHumanContextFake traveler( nullptr, &nc_2, nullptr, nullptr );

            // ------------------------------------------------------------------
            // --- Test that male < 15 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 10.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );

            // ------------------------------------------------------------------
            // --- Test that male = 30 will migrate to node 4
            // ------------------------------------------------------------------
            traveler.SetAge( 30.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 4, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 );

            // ------------------------------------------------------------------
            // --- Test that female = 60 will migrate to node 3
            // ------------------------------------------------------------------
            traveler.SetAge( 60.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that female = 90 will migrate to node 1
            // ------------------------------------------------------------------
            traveler.SetAge( 90.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            m_RandomFake.SetUL( 0 ); // 0.0 - need to change so we get the first in the list

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 1, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 15.9424, trip_time, 0.0001 ); //trip_time is different due to new random number

            // ------------------------------------------------------------------
            // --- Test that female = 120 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 120.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.0001 );

        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE( MigrationFixture, TestPerfVectorHumanMigration )
    {
        try
        {
            VectorGenome genome;
            genome.SetLocus( 0, 0, 0 );

            IVectorCohort* p_vc1 = VectorCohort::CreateCohort( 1,
                                                              VectorStateEnum::STATE_ADULT,
                                                              0.0,
                                                              0.0,
                                                              0.0,
                                                              1,
                                                              genome,
                                                              0 );

            std::string config_filename = "testdata/MigrationTest/TestBothGendersVectorPerf_config.json";
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( config_filename.c_str() ) );
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1; node_id <= 9; node_id++ )
            {
                suids::suid node_suid;
                node_suid.data = node_id;
                nodeid_suid_map.insert( nodeid_suid_pair( node_id, node_suid ) );
            }

            std::string idreference = "9-nodes";
            VectorSpeciesParameters vsp( 0 );
            vsp.Configure( p_config.get() );

            // ---------------
            // --- Test Node 5
            // ---------------
            INodeContextFake nc_5( nodeid_suid_map.left.at( 5 ) );
            unique_ptr<IMigrationInfoVector> p_mi_5( vsp.p_migration_factory->CreateMigrationInfoVector( idreference, 
                                                                                                         &nc_5,
                                                                                                         nodeid_suid_map,
                                                                                                         vsp.index,
                                                                                                         &vsp.genes ) );
            p_mi_5->GetFractionTraveling( p_vc1 );

            //m_RandomFake.SetUL( 2576980377 ); // 0.6
            IndividualHumanContextFake traveler( nullptr, &nc_5, nullptr, nullptr );

            traveler.SetAge(0);
            traveler.SetGender( Gender::FEMALE );

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;
            RANDOMBASE* prng = new PSEUDO_DES( 42 );

            int num_samples = 1000 * 365 * 5;
            Stopwatch watch;
            watch.Start();
            for(int i = 0; i < num_samples; i++ ) // 1k individual female vectors every day for five years
            {
                p_mi_5->PickMigrationStep( prng, &traveler, 1.0, destination, mig_type, trip_time, 1 );
            }
            watch.Stop();
            double seconds = watch.ResultNanoseconds() / 100000000.0;

            ostringstream msg;
            msg << "Duration PickMigrationStep (seconds) = " << seconds << endl;
            PrintDebug( msg.str() );

            const std::vector<suids::suid>& r_reachable_nodes = p_mi_5->GetReachableNodes(Gender::FEMALE );
            const std::vector<float              >&     r_cdf = p_mi_5->GetCumulativeDistributionFunction( Gender::FEMALE );
            float total_rate = p_mi_5->GetTotalRate(Gender::FEMALE);

            watch.Start();
            for( int i = 0; i < num_samples; i++ )
            {
                // time in days until we leave
                float time = float( prng->expdist( total_rate ) );
                // test if each vector will migrate this time step, if time <= 1, we leave today 
                if( time <= 1 )
                {
                    // picking destination node
                    int index = 0;
                    float desttemp = prng->e();
                    while( desttemp > r_cdf[index] )
                    {
                        index++;
                    }
                    suids::suid destination = r_reachable_nodes[index];
                }
            }
            watch.Stop();
            double seconds2 = watch.ResultNanoseconds() / 100000000.0;
            delete prng;
            prng = nullptr;
            ostringstream msg2;
            msg2 << "Duration simplified (seconds) = " << seconds2 << endl;
            PrintDebug( msg2.str() );

        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestEachGender)
    {
        std::string config_filename = "testdata/MigrationTest/TestEachGender_config.json";
        try
        {
            unique_ptr<Configuration> p_config_file( Environment::LoadConfigurationFile( config_filename.c_str() ) );

            unique_ptr<Configuration> p_config( Environment::CopyFromElement( (*p_config_file)["parameters"] ) );

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 4 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "ABC" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( p_config.get(),
                                                                                                     idreference ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ---------------
            // --- Test Node 1
            // ---------------
            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi_1( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( p_mi_1->IsHeterogeneityEnabled() );
   
            const std::vector<suids::suid>& reachable_nodes_1 = p_mi_1->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_1.size() );
            CHECK_EQUAL( 2, reachable_nodes_1[ 0].data );
            CHECK_EQUAL( 3, reachable_nodes_1[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_1[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_1 = p_mi_1->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_1.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_1[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_1[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_1[ 2] );

            // ---------------
            // --- Test Node 2
            // ---------------
            INodeContextFake nc_2( nodeid_suid_map.left.at(2) ) ;
            unique_ptr<IMigrationInfo> p_mi_2( p_mf->CreateMigrationInfo( &nc_2, nodeid_suid_map ) );

            CHECK( p_mi_2->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_2.size() );
            CHECK_EQUAL( 1, reachable_nodes_2[ 0].data );
            CHECK_EQUAL( 3, reachable_nodes_2[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_2[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_2.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 2] );

            // ---------------
            // --- Test Node 3
            // ---------------
            INodeContextFake nc_3( nodeid_suid_map.left.at(3) ) ;
            unique_ptr<IMigrationInfo> p_mi_3( p_mf->CreateMigrationInfo( &nc_3, nodeid_suid_map ) );

            CHECK( p_mi_3->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_3 = p_mi_3->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_3.size() );
            CHECK_EQUAL( 1, reachable_nodes_3[ 0].data );
            CHECK_EQUAL( 2, reachable_nodes_3[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_3[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_3 = p_mi_3->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_3.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_3[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_3[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_3[ 2] );

            // ---------------
            // --- Test Node 4
            // ---------------
            INodeContextFake nc_4( nodeid_suid_map.left.at(4) ) ;
            unique_ptr<IMigrationInfo> p_mi_4( p_mf->CreateMigrationInfo( &nc_4, nodeid_suid_map ) );

            CHECK( p_mi_4->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_4 = p_mi_4->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_4.size() );
            CHECK_EQUAL( 1, reachable_nodes_4[ 0].data );
            CHECK_EQUAL( 2, reachable_nodes_4[ 1].data );
            CHECK_EQUAL( 3, reachable_nodes_4[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_4 = p_mi_4->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_4.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_4[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_4[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_4[ 2] );

            // ================
            // === FROM NODE 2
            // ================
            m_RandomFake.SetUL( 2576980377 ); // 0.6

            IndividualHumanContextFake traveler( nullptr, &nc_2, nullptr, nullptr );

            // ------------------------------------------------------------------
            // --- Test that male < 15 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 10.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );

            // ------------------------------------------------------------------
            // --- Test that male = 30 will migrate to node 4
            // ------------------------------------------------------------------
            traveler.SetAge( 30.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 4, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 );

            // ------------------------------------------------------------------
            // --- Test that male = 60 will migrate to node 3
            // ------------------------------------------------------------------
            traveler.SetAge( 60.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that male = 90 will migrate to node 1
            // ------------------------------------------------------------------
            traveler.SetAge( 90.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            m_RandomFake.SetUL( 0 ); // 0.0 - need to change so we get the first in the list

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 1, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 15.9424, trip_time, 0.0001 ); //trip_time is different due to new random number

            // ------------------------------------------------------------------
            // --- Test that male = 120 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 120.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.0001 );

            // ======================================================================================
            // === Switch to female and verify that we are working with different data than for MALE
            // ======================================================================================
            m_RandomFake.SetUL( 2576980377 ); // 0.6 - reset 
            // ------------------------------------------------------------------
            // --- Test that female < 15 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 10.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );

            // ------------------------------------------------------------------
            // --- Test that female = 30 will migrate to node 3 instead of 4
            // ------------------------------------------------------------------
            traveler.SetAge( 30.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that female = 60 will migrate to node 3
            // ------------------------------------------------------------------
            traveler.SetAge( 60.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that female = 90 will migrate to node 1
            // ------------------------------------------------------------------
            traveler.SetAge( 90.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            m_RandomFake.SetUL( 0 ); // 0.0 - 

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 1, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 15.9424, trip_time, 0.0001 ); //trip_time is different due to new random number

            // ------------------------------------------------------------------
            // --- Test that female = 120 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 120.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.0001 );

        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestFixedRate)
    {
        try
        {
            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 26 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "Household-Scenario-Small" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( p_mi->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes = p_mi->GetReachableNodes();
            CHECK_EQUAL( 20, reachable_nodes.size() );
            CHECK_EQUAL(  2, reachable_nodes[ 0].data );
            CHECK_EQUAL(  6, reachable_nodes[ 1].data );
            CHECK_EQUAL(  3, reachable_nodes[ 2].data );
            CHECK_EQUAL(  4, reachable_nodes[ 3].data );
            CHECK_EQUAL(  5, reachable_nodes[ 4].data );
            CHECK_EQUAL(  8, reachable_nodes[ 5].data );
            CHECK_EQUAL(  9, reachable_nodes[ 6].data );
            CHECK_EQUAL( 10, reachable_nodes[ 7].data );
            CHECK_EQUAL( 11, reachable_nodes[ 8].data );
            CHECK_EQUAL( 12, reachable_nodes[ 9].data );
            CHECK_EQUAL( 13, reachable_nodes[10].data );
            CHECK_EQUAL( 15, reachable_nodes[11].data );
            CHECK_EQUAL( 17, reachable_nodes[12].data );
            CHECK_EQUAL( 18, reachable_nodes[13].data );
            CHECK_EQUAL( 19, reachable_nodes[14].data );
            CHECK_EQUAL( 20, reachable_nodes[15].data );
            CHECK_EQUAL( 23, reachable_nodes[16].data );
            CHECK_EQUAL( 24, reachable_nodes[17].data );
            CHECK_EQUAL( 25, reachable_nodes[18].data );
            CHECK_EQUAL( 26, reachable_nodes[19].data );

            const std::vector<MigrationType::Enum>& mig_type_list = p_mi->GetMigrationTypes();
            CHECK_EQUAL( 20, mig_type_list.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 1] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 2] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 3] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 4] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 5] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 6] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 7] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 8] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 9] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[10] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[11] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[12] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[13] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[14] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[15] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[16] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[17] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[18] );
            CHECK_EQUAL( MigrationType::SEA_MIGRATION,      mig_type_list[19] );

            // ------------------------------------------------------------------
            // --- Test picking destination that is in the middle of our options
            // ------------------------------------------------------------------
            m_RandomFake.SetUL( 2147486240 ); // 0.5

            IndividualHumanContextFake traveler( nullptr, &nc_1, nullptr, nullptr );
            traveler.SetAge( 20.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );
            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION ;
            float trip_time = -1.0 ;
            p_mi->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 10, destination.data );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type );
            CHECK_CLOSE( 4.33216, trip_time, 0.00001 );

            // -----------------------------------------------
            // --- Test picking our first possible destination
            // -----------------------------------------------
            m_RandomFake.SetUL( 0 ); // 0.0

            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION ;
            trip_time = -1.0 ;
            p_mi->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 2, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 99.63990, trip_time, 0.00001 );

            // -----------------------------------------------
            // --- Test picking our last possible destination
            // -----------------------------------------------
            m_RandomFake.SetUL( 0xFFFFFFFF ); // 1.0

            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION ;
            trip_time = -1.0 ;
            p_mi->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 26, destination.data );
            CHECK_EQUAL( MigrationType::SEA_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestDefault)
    {
        try
        {
            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 100 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            std::string idreference = "Default" ;
            if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
            {
                GET_CONFIGURABLE( SimulationConfig )->demographics_initial = false;
                GET_CONFIGURABLE( SimulationConfig )->default_torus_size = 10;
            }
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference ) );

            CHECK( !p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ----------
            // --- Node 1 - Upper Left
            // ----------
            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( !p_mi->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes = p_mi->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes.size() );
            CHECK_EQUAL( 100, reachable_nodes[ 0].data );
            CHECK_EQUAL(  91, reachable_nodes[ 1].data );
            CHECK_EQUAL(  92, reachable_nodes[ 2].data );
            CHECK_EQUAL(  10, reachable_nodes[ 3].data );
            CHECK_EQUAL(   2, reachable_nodes[ 4].data );
            CHECK_EQUAL(  20, reachable_nodes[ 5].data );
            CHECK_EQUAL(  11, reachable_nodes[ 6].data );
            CHECK_EQUAL(  12, reachable_nodes[ 7].data );

            const std::vector<MigrationType::Enum>& mig_type_list = p_mi->GetMigrationTypes();
            CHECK_EQUAL( 8, mig_type_list.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 2] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 3] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 4] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 5] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 6] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 7] );

            // ----------
            // --- Node 10 - Upper Right
            // ----------
            INodeContextFake nc_10( nodeid_suid_map.left.at(10) ) ;
            unique_ptr<IMigrationInfo> p_mi_10( p_mf->CreateMigrationInfo( &nc_10, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_10 = p_mi_10->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_10.size() );
            CHECK_EQUAL(  99, reachable_nodes_10[ 0].data );
            CHECK_EQUAL( 100, reachable_nodes_10[ 1].data );
            CHECK_EQUAL(  91, reachable_nodes_10[ 2].data );
            CHECK_EQUAL(   9, reachable_nodes_10[ 3].data );
            CHECK_EQUAL(   1, reachable_nodes_10[ 4].data );
            CHECK_EQUAL(  19, reachable_nodes_10[ 5].data );
            CHECK_EQUAL(  20, reachable_nodes_10[ 6].data );
            CHECK_EQUAL(  11, reachable_nodes_10[ 7].data );

            // ----------
            // --- Node 91 - Lower Left
            // ----------
            INodeContextFake nc_91( nodeid_suid_map.left.at(91) ) ;
            unique_ptr<IMigrationInfo> p_mi_91( p_mf->CreateMigrationInfo( &nc_91, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_91 = p_mi_91->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_91.size() );
            CHECK_EQUAL(  90, reachable_nodes_91[ 0].data );
            CHECK_EQUAL(  81, reachable_nodes_91[ 1].data );
            CHECK_EQUAL(  82, reachable_nodes_91[ 2].data );
            CHECK_EQUAL( 100, reachable_nodes_91[ 3].data );
            CHECK_EQUAL(  92, reachable_nodes_91[ 4].data );
            CHECK_EQUAL(  10, reachable_nodes_91[ 5].data );
            CHECK_EQUAL(   1, reachable_nodes_91[ 6].data );
            CHECK_EQUAL(   2, reachable_nodes_91[ 7].data );

            // ----------
            // --- Node 100 - Lower Right
            // ----------
            INodeContextFake nc_100( nodeid_suid_map.left.at(100) ) ;
            unique_ptr<IMigrationInfo> p_mi_100( p_mf->CreateMigrationInfo( &nc_100, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_100 = p_mi_100->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_100.size() );
            CHECK_EQUAL(  89, reachable_nodes_100[ 0].data );
            CHECK_EQUAL(  90, reachable_nodes_100[ 1].data );
            CHECK_EQUAL(  81, reachable_nodes_100[ 2].data );
            CHECK_EQUAL(  99, reachable_nodes_100[ 3].data );
            CHECK_EQUAL(  91, reachable_nodes_100[ 4].data );
            CHECK_EQUAL(   9, reachable_nodes_100[ 5].data );
            CHECK_EQUAL(  10, reachable_nodes_100[ 6].data );
            CHECK_EQUAL(   1, reachable_nodes_100[ 7].data );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestDefaultSize9)
    {
        try
        {
            int torus_size = 3 ;
            uint32_t num_nodes = torus_size*torus_size;

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= num_nodes ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            std::string idreference = "Default" ;
            if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
            {
                GET_CONFIGURABLE( SimulationConfig )->demographics_initial = false;
                GET_CONFIGURABLE( SimulationConfig )->default_torus_size = torus_size;
            }
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference ) );

            CHECK( !p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ----------
            // --- Node 1 - Upper Left
            // ----------
            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( !p_mi->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_1 = p_mi->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_1.size() );
            CHECK_EQUAL( 9, reachable_nodes_1[ 0].data );
            CHECK_EQUAL( 7, reachable_nodes_1[ 1].data );
            CHECK_EQUAL( 8, reachable_nodes_1[ 2].data );
            CHECK_EQUAL( 3, reachable_nodes_1[ 3].data );
            CHECK_EQUAL( 2, reachable_nodes_1[ 4].data );
            CHECK_EQUAL( 6, reachable_nodes_1[ 5].data );
            CHECK_EQUAL( 4, reachable_nodes_1[ 6].data );
            CHECK_EQUAL( 5, reachable_nodes_1[ 7].data );

            // ----------
            // --- Node 3 - Upper Right
            // ----------
            INodeContextFake nc_3( nodeid_suid_map.left.at(3) ) ;
            unique_ptr<IMigrationInfo> p_mi_3( p_mf->CreateMigrationInfo( &nc_3, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_3 = p_mi_3->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_3.size() );
            CHECK_EQUAL( 8, reachable_nodes_3[ 0].data );
            CHECK_EQUAL( 9, reachable_nodes_3[ 1].data );
            CHECK_EQUAL( 7, reachable_nodes_3[ 2].data );
            CHECK_EQUAL( 2, reachable_nodes_3[ 3].data );
            CHECK_EQUAL( 1, reachable_nodes_3[ 4].data );
            CHECK_EQUAL( 5, reachable_nodes_3[ 5].data );
            CHECK_EQUAL( 6, reachable_nodes_3[ 6].data );
            CHECK_EQUAL( 4, reachable_nodes_3[ 7].data );

            // ----------
            // --- Node 7 - Lower Left
            // ----------
            INodeContextFake nc_7( nodeid_suid_map.left.at(7) ) ;
            unique_ptr<IMigrationInfo> p_mi_7( p_mf->CreateMigrationInfo( &nc_7, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_7 = p_mi_7->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_7.size() );
            CHECK_EQUAL( 6, reachable_nodes_7[ 0].data );
            CHECK_EQUAL( 4, reachable_nodes_7[ 1].data );
            CHECK_EQUAL( 5, reachable_nodes_7[ 2].data );
            CHECK_EQUAL( 9, reachable_nodes_7[ 3].data );
            CHECK_EQUAL( 8, reachable_nodes_7[ 4].data );
            CHECK_EQUAL( 3, reachable_nodes_7[ 5].data );
            CHECK_EQUAL( 1, reachable_nodes_7[ 6].data );
            CHECK_EQUAL( 2, reachable_nodes_7[ 7].data );

            // ----------
            // --- Node 9 - Lower right
            // ----------
            INodeContextFake nc_9( nodeid_suid_map.left.at(9) ) ;
            unique_ptr<IMigrationInfo> p_mi_9( p_mf->CreateMigrationInfo( &nc_9, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_9.size() );
            CHECK_EQUAL( 5, reachable_nodes_9[ 0].data );
            CHECK_EQUAL( 6, reachable_nodes_9[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_9[ 2].data );
            CHECK_EQUAL( 8, reachable_nodes_9[ 3].data );
            CHECK_EQUAL( 7, reachable_nodes_9[ 4].data );
            CHECK_EQUAL( 2, reachable_nodes_9[ 5].data );
            CHECK_EQUAL( 3, reachable_nodes_9[ 6].data );
            CHECK_EQUAL( 1, reachable_nodes_9[ 7].data );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }
#endif

    TEST_FIXTURE(MigrationFixture, TestVectorMigrationInfo)
    {
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/MigrationTest/TestVectorMigrationInfo.json"));

            VectorGenome genome;
            genome.SetLocus( 0, 0, 0 );

            IVectorCohort* p_vc1 = VectorCohort::CreateCohort( 1,
                VectorStateEnum::STATE_ADULT,
                0.0,
                0.0,
                0.0,
                1,
                genome,
                0 );

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 26 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            std::string idreference = "Household-Scenario-Small" ;
            VectorSpeciesParameters vsp( 0 );
            vsp.Configure( p_config.get() );

            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfoVector> p_mi( vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                       &nc_1,
                                                                                                       nodeid_suid_map,
                                                                                                       vsp.index,
                                                                                                       &vsp.genes ) );
            p_mi->GetFractionTraveling( p_vc1 );

            const std::vector<suids::suid>& reachable_nodes = p_mi->GetReachableNodes();
            CHECK_EQUAL( 2, reachable_nodes.size() );
            CHECK_EQUAL( 2, reachable_nodes[ 0].data );
            CHECK_EQUAL( 6, reachable_nodes[ 1].data );


            const std::vector<MigrationType::Enum>& mig_type_list = p_mi->GetMigrationTypes();
            CHECK_EQUAL( 2, mig_type_list.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 1] );


            INodeContextFake nc_9( nodeid_suid_map.left.at(9) ) ;
            unique_ptr<IMigrationInfoVector> p_mi_9( vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                         &nc_9,
                                                                                                         nodeid_suid_map,
                                                                                                         vsp.index,
                                                                                                         &vsp.genes ) );
            p_mi_9->GetFractionTraveling( p_vc1 );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL( 7, reachable_nodes_9.size() );
            CHECK_EQUAL(  3, reachable_nodes_9[ 0].data );
            CHECK_EQUAL(  4, reachable_nodes_9[ 1].data );
            CHECK_EQUAL(  5, reachable_nodes_9[ 2].data );
            CHECK_EQUAL(  8, reachable_nodes_9[ 3].data );
            CHECK_EQUAL( 10, reachable_nodes_9[ 4].data );
            CHECK_EQUAL( 13, reachable_nodes_9[ 5].data );
            CHECK_EQUAL( 15, reachable_nodes_9[ 6].data );



            const std::vector<MigrationType::Enum>& mig_type_list_9 = p_mi_9->GetMigrationTypes();
            CHECK_EQUAL( 7, mig_type_list_9.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 2] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 3] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 4] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 5] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 6] );


            INodeContextFake nc_26( nodeid_suid_map.left.at(26) ) ;
            unique_ptr<IMigrationInfoVector> p_mi_26( vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                          &nc_26,
                                                                                                          nodeid_suid_map,
                                                                                                          vsp.index,
                                                                                                          &vsp.genes ) );
            p_mi_26->GetFractionTraveling( p_vc1 );

            CHECK( p_mi_26->GetReachableNodes().size() == 0 );
        }
        catch( DetailedException& re )
        {
            printf("%s\n",re.GetMsg());
            CHECK( false );
        }
    }

    TEST_FIXTURE( MigrationFixture, TestMigrationByGenes )
    {
        std::string config_filename = "testdata/MigrationTest/TestMigrationByGenesVector_config.json";


        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( config_filename.c_str() ) );

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1; node_id <= 9; node_id++ )
            {
                suids::suid node_suid;
                node_suid.data = node_id;
                nodeid_suid_map.insert( nodeid_suid_pair( node_id, node_suid ) );
            }

            VectorGenome genome;  //index 0 female
            genome.SetLocus( 0, 0, 0 );
            genome.SetLocus( 1, 1, 0 );
            genome.SetLocus( 2, 0, 1 );

            IVectorCohort* p_female_vector_cohort = VectorCohort::CreateCohort( 1,
                VectorStateEnum::STATE_ADULT,
                0.0,
                0.0,
                0.0,
                1,
                genome,
                0 );

            genome.SetLocus( 0, 0, 4 ); // index 3 male
            genome.SetLocus( 1, 1, 0 );
            genome.SetLocus( 2, 0, 1 );

            IVectorCohort* p_male_vector_cohort = VectorCohortMale::CreateCohort( 1,
                0,
                0,
                0,
                1,
                genome,
                0 );

            std::string idreference = "9-nodes";
            VectorSpeciesParameters vsp( 0 );
            vsp.Configure( p_config.get() );

            // ---------------
            // --- Test Node 5
            // ---------------
            INodeContextFake nc_5( nodeid_suid_map.left.at( 5 ) );
            unique_ptr<IMigrationInfoVector> p_mi_5( vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                         &nc_5,
                                                                                                         nodeid_suid_map,
                                                                                                         vsp.index,
                                                                                                         &vsp.genes ) );

            const std::vector<suids::suid>& reachable_nodes_5 = p_mi_5->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_5.size() );
            CHECK_EQUAL( 1, reachable_nodes_5[0].data );
            CHECK_EQUAL( 2, reachable_nodes_5[1].data );
            CHECK_EQUAL( 3, reachable_nodes_5[2].data );
            CHECK_EQUAL( 4, reachable_nodes_5[3].data );
            CHECK_EQUAL( 6, reachable_nodes_5[4].data );
            CHECK_EQUAL( 7, reachable_nodes_5[5].data );
            CHECK_EQUAL( 8, reachable_nodes_5[6].data );
            CHECK_EQUAL( 9, reachable_nodes_5[7].data );

            const std::vector<MigrationType::Enum>& mig_type_list_5 = p_mi_5->GetMigrationTypes();
            CHECK_EQUAL( 8, mig_type_list_5.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[2] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[3] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[4] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[5] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[6] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_5[7] );

            // ================
            // === FROM NODE 5
            // ================

            const std::vector<float>* fraction_traveling = p_mi_5->GetFractionTraveling( p_female_vector_cohort );
            CHECK_CLOSE( 0.086, fraction_traveling->at( 0 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling->at( 1 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling->at( 2 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling->at( 3 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling->at( 4 ), 0.001 );
            CHECK_CLOSE( 0.086, fraction_traveling->at( 5 ), 0.001 );
            CHECK_CLOSE( 0.086, fraction_traveling->at( 6 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling->at( 7 ), 0.001 );


            genome.SetLocus( 0, 0, 2 ); // index 0 female
            genome.SetLocus( 1, 1, 0 );
            genome.SetLocus( 2, 0, 1 );

            IVectorCohort* p_mvc1 = VectorCohortMale::CreateCohort( 1,
                0,
                0,
                0,
                1,
                genome,
                0 );

            const std::vector<float>* fraction_traveling2 = p_mi_5->GetFractionTraveling( p_mvc1 );
            CHECK_CLOSE( 0.086, fraction_traveling2->at( 0 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling2->at( 1 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling2->at( 2 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling2->at( 3 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling2->at( 4 ), 0.001 );
            CHECK_CLOSE( 0.086, fraction_traveling2->at( 5 ), 0.001 );
            CHECK_CLOSE( 0.086, fraction_traveling2->at( 6 ), 0.001 );
            CHECK_CLOSE( 0.0,   fraction_traveling2->at( 7 ), 0.001 );

            const std::vector<float>* fraction_traveling3 = p_mi_5->GetFractionTraveling( p_male_vector_cohort );
             CHECK_CLOSE( 0.0,   fraction_traveling3->at( 0 ), 0.001 );
             CHECK_CLOSE( 0.0,   fraction_traveling3->at( 1 ), 0.001 );
             CHECK_CLOSE( 0.0,   fraction_traveling3->at( 2 ), 0.001 );
             CHECK_CLOSE( 0.092, fraction_traveling3->at( 3 ), 0.001 );
             CHECK_CLOSE( 0.0,   fraction_traveling3->at( 4 ), 0.001 );
             CHECK_CLOSE( 0.0,   fraction_traveling3->at( 5 ), 0.001 );
             CHECK_CLOSE( 0.046, fraction_traveling3->at( 6 ), 0.001 );
             CHECK_CLOSE( 0.0,   fraction_traveling3->at( 7 ), 0.001 );
            
             // creates index 4 male
             genome.SetLocus( 0, 0, 4 );
             genome.SetLocus( 1, 1, 1 );
             genome.SetLocus( 2, 1, 1 );

             IVectorCohort* index_4_male = VectorCohortMale::CreateCohort( 1,
                 0,
                 0,
                 0,
                 1,
                 genome,
                 0 );

             const std::vector<float>* fraction_traveling4 = p_mi_5->GetFractionTraveling( index_4_male );
             CHECK_CLOSE( 0.0, fraction_traveling4->at( 0 ), 0.001 );
             CHECK_CLOSE( 0.0, fraction_traveling4->at( 1 ), 0.001 );
             CHECK_CLOSE( 0.0, fraction_traveling4->at( 2 ), 0.001 );
             CHECK_CLOSE( 0.0, fraction_traveling4->at( 3 ), 0.001 );
             CHECK_CLOSE( 0.090, fraction_traveling4->at( 4 ), 0.001 );
             CHECK_CLOSE( 0.0, fraction_traveling4->at( 5 ), 0.001 );
             CHECK_CLOSE( 0.009, fraction_traveling4->at( 6 ), 0.001 );
             CHECK_CLOSE( 0.090, fraction_traveling4->at( 7 ), 0.001 );

            m_RandomFake.SetUL( 2576980377 ); // 0.6

            IndividualHumanContextFake traveler( nullptr, &nc_5, nullptr, nullptr );

            // ------------------------------------------------------------------
            // --- Test that each index has correct rates
            // ------------------------------------------------------------------
            traveler.SetAge( 0 );
            traveler.SetGender( Gender::FEMALE );

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            genome.SetLocus( 0, 0, 0 );  //index 4 female
            genome.SetLocus( 1, 1, 1 );
            genome.SetLocus( 2, 1, 1 );

            IVectorCohort* index_4_female_cohort = VectorCohort::CreateCohort( 1,
                VectorStateEnum::STATE_ADULT,
                0.0,
                0.0,
                0.0,
                1,
                genome,
                0 );

            p_mi_5->GetFractionTraveling( index_4_female_cohort );
            p_mi_5->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );


            CHECK_EQUAL( 9, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 2.432, trip_time, 0.001 );

            genome.SetLocus( 0, 0, 0 );  //index 0 female
            genome.SetLocus( 1, 0, 0 );
            genome.SetLocus( 2, 0, 0 );

            IVectorCohort* index_0_female_cohort = VectorCohort::CreateCohort( 1,
                VectorStateEnum::STATE_ADULT,
                0.0,
                0.0,
                0.0,
                1,
                genome,
                0 );

            p_mi_5->GetFractionTraveling( index_0_female_cohort );

            p_mi_5->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 7, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 1.702, trip_time, 0.001 );

            genome.SetLocus( 0, 0, 0 ); // index 2 female
            genome.SetLocus( 1, 1, 1 );
            genome.SetLocus( 2, 1, 0 );

            IVectorCohort* index_2_female_cohort = VectorCohort::CreateCohort( 1,
                VectorStateEnum::STATE_ADULT,
                0.0,
                0.0,
                0.0,
                1,
                genome,
                0 );

            p_mi_5->GetFractionTraveling( index_2_female_cohort );
            p_mi_5->PickMigrationStep( &m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 8, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 2.554, trip_time, 0.001 );

        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestEachGenderVector)
    {
        std::string config_filename = "testdata/MigrationTest/TestEachGenderVector_config.json";
        try
        {
            VectorGenome genome;
            genome.SetLocus( 0, 0, 0 );

            IVectorCohort* p_female_vector_cohort = VectorCohort::CreateCohort( 1,
                VectorStateEnum::STATE_ADULT,
                0.0,
                0.0,
                0.0,
                1,
                genome,
                0 );

            genome.SetLocus( 0, 0, 4 );

            IVectorCohort* p_male_vector_cohort = VectorCohortMale::CreateCohort( 1,
                0,
                0,
                0,
                1,
                genome,
                0 );

            unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile(config_filename.c_str()));

            // Female vectors migrate from odd nodes to even nodes, male vectors migrate from even nodes to odd nodes
            // Vector migration does not depend on age, so we use age=0

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for (uint32_t node_id = 1; node_id <= 9; node_id++)
            {
                suids::suid node_suid;
                node_suid.data = node_id;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "MigrationTest";
            VectorSpeciesParameters vsp(0);
            vsp.Configure(p_config.get());

            // ---------------
            // --- Test Node 2
            // ---------------
            INodeContextFake nc_2(nodeid_suid_map.left.at(2));
            unique_ptr<IMigrationInfoVector> p_mi_2(vsp.p_migration_factory->CreateMigrationInfoVector(idreference,
                                                                                                       &nc_2,
                                                                                                       nodeid_suid_map,
                                                                                                       vsp.index,
                                                                                                       &vsp.genes ) );
            p_mi_2->GetFractionTraveling( p_female_vector_cohort );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL(3, reachable_nodes_2.size());
            CHECK_EQUAL(1, reachable_nodes_2[0].data);
            CHECK_EQUAL(3, reachable_nodes_2[1].data);
            CHECK_EQUAL(5, reachable_nodes_2[2].data);

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL(3, mig_type_list_2.size());
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[0]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[1]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[2]);

            // ================
            // === FROM NODE 2
            // ================

            const std::vector<float>* fraction_traveling = p_mi_2->GetFractionTraveling( p_male_vector_cohort );
            CHECK_CLOSE( 0.086, fraction_traveling->at( 0 ), 0.001 );
            CHECK_CLOSE( 0.086, fraction_traveling->at( 1 ), 0.001 );
            CHECK_CLOSE( 0.086, fraction_traveling->at( 2 ), 0.001 );

            const std::vector<float>* fraction_traveling2 = p_mi_2->GetFractionTraveling( p_female_vector_cohort );
            if( fraction_traveling2 != nullptr )
            {
                CHECK_EQUAL( true, false );
            }

            m_RandomFake.SetUL(2576980377); // 0.6

            IndividualHumanContextFake traveler(nullptr, &nc_2, nullptr, nullptr);

            // ------------------------------------------------------------------
            // --- Test that male vector (age 0) will migrate from node 2 to nodes 1, 3, 5
            // ------------------------------------------------------------------
            // PickMigrationStep does not work for makes because we do not keep their migration rCDS as they only migration by fraction  


            // ------------------------------------------------------------------
            // --- Test that female vector (age 0) will not migration from node 2
            // ------------------------------------------------------------------
            traveler.SetAge(0);
            traveler.SetGender(Gender::FEMALE);


            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep(&m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time);

            CHECK_EQUAL(0, destination.data);
            CHECK_EQUAL(MigrationType::NO_MIGRATION, mig_type);
            CHECK_CLOSE(0.0, trip_time, 0.0001);

            // ---------------
            // --- Test Node 9
            // ---------------
            INodeContextFake nc_9(nodeid_suid_map.left.at(9));
            unique_ptr<IMigrationInfoVector> p_mi_9(vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                        &nc_9,
                                                                                                        nodeid_suid_map,
                                                                                                        vsp.index,
                                                                                                        &vsp.genes ) );
            p_mi_9->GetFractionTraveling( p_female_vector_cohort );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL(2, reachable_nodes_9.size());
            CHECK_EQUAL(6, reachable_nodes_9[0].data);
            CHECK_EQUAL(8, reachable_nodes_9[1].data);


            const std::vector<MigrationType::Enum>& mig_type_list_9 = p_mi_9->GetMigrationTypes();
            CHECK_EQUAL(2, mig_type_list_9.size());
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[0]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[1]);

            const std::vector<float>* fraction_traveling1 = p_mi_9->GetFractionTraveling( p_male_vector_cohort );
            if( fraction_traveling1 != nullptr )
            {
                CHECK_EQUAL( true, false );
            }


            const std::vector<float>* fraction_traveling33 = p_mi_9->GetFractionTraveling( p_female_vector_cohort );
            CHECK_CLOSE( 0.090, fraction_traveling33->at( 0 ), 0.001 );
            CHECK_CLOSE( 0.090, fraction_traveling33->at( 1 ), 0.001 );



            // ================
            // === FROM NODE 9
            // ================
            m_RandomFake.SetUL(2576980377); // 0.6

            IndividualHumanContextFake traveler9(nullptr, &nc_9, nullptr, nullptr);

            // ------------------------------------------------------------------
            // --- Test that male vector (age 0) will not migration from node 9
            // ------------------------------------------------------------------
            // vector males do not migrate by PickMigrationStep

            // ------------------------------------------------------------------
            // --- Test that female vector (age 0) will migrate from node 9 to nodes 6, 8
            // ------------------------------------------------------------------
            traveler9.SetAge(0);
            traveler9.SetGender(Gender::FEMALE);
            p_mi_9->GetFractionTraveling( p_female_vector_cohort );

            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION;
            trip_time = -1.0;

            p_mi_9->PickMigrationStep(&m_RandomFake, &traveler9, 1.0, destination, mig_type, trip_time);

            CHECK_EQUAL(8, destination.data);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type);
            CHECK_CLOSE(2.5541, trip_time, 0.0001);

        }
        catch (DetailedException& re)
        {
            PrintDebug(re.GetMsg());
            CHECK(false);
        }
    }


    TEST_FIXTURE(MigrationFixture, TestBothGendersVector)
    {
        std::string config_filename = "testdata/MigrationTest/TestBothGendersVector_config.json";

        VectorGenome genome;
        genome.SetLocus( 0, 0, 0 );

        IVectorCohort* p_female_vector_cohort = VectorCohort::CreateCohort( 1,
            VectorStateEnum::STATE_ADULT,
            0.0,
            0.0,
            0.0,
            1,
            genome,
            0 );

        genome.SetLocus( 0, 0, 4 );

        IVectorCohort* p_male_vector_cohort = VectorCohortMale::CreateCohort( 1,
            0,
            0,
            0,
            1,
            genome,
            0 );

        try
        {
            unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile(config_filename.c_str()));

            // Female vectors migrate from odd nodes to even nodes, male vectors migrate from even nodes to odd nodes
            // Vector migration does not depend on age, so we use age=0

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for (uint32_t node_id = 1; node_id <= 9; node_id++)
            {
                suids::suid node_suid;
                node_suid.data = node_id;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "MigrationTest";
            VectorSpeciesParameters vsp(0);
            vsp.Configure(p_config.get());

            // ---------------
            // --- Test Node 2 , no migration from file for Node 2
            // ---------------
            INodeContextFake nc_2(nodeid_suid_map.left.at(2));
            unique_ptr<IMigrationInfoVector> p_mi_2(vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                        &nc_2,
                                                                                                        nodeid_suid_map,
                                                                                                        vsp.index,
                                                                                                        &vsp.genes ) );
            p_mi_2->GetFractionTraveling( p_female_vector_cohort );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL(6, reachable_nodes_2.size());
            CHECK_EQUAL(4, reachable_nodes_2[0].data);
            CHECK_EQUAL(5, reachable_nodes_2[1].data);
            CHECK_EQUAL(6, reachable_nodes_2[2].data);
            CHECK_EQUAL(7, reachable_nodes_2[3].data);
            CHECK_EQUAL(8, reachable_nodes_2[4].data);
            CHECK_EQUAL(9, reachable_nodes_2[5].data);

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL(6, mig_type_list_2.size());
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[0]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[1]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[2]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[3]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[4]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[5]);

            // ================
            // === FROM NODE 2
            // ================
            m_RandomFake.SetUL(2576980377); // 0.6

            IndividualHumanContextFake traveler(nullptr, &nc_2, nullptr, nullptr);

            // ------------------------------------------------------------------
            // --- Test that male vector (age 0) no migration from Node 2
            // ------------------------------------------------------------------
            const std::vector<float>* fraction_traveling = p_mi_2->GetFractionTraveling( p_female_vector_cohort );
            if( fraction_traveling != nullptr )
            {
                CHECK_EQUAL( true, false );
            }

            // ------------------------------------------------------------------
            // --- Test that female vector (age 0) no migration from Node 2
            // ------------------------------------------------------------------
            traveler.SetAge(0);
            traveler.SetGender(Gender::FEMALE);

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep(&m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time);

            CHECK_EQUAL(0, destination.data);
            CHECK_EQUAL(MigrationType::NO_MIGRATION, mig_type);
            CHECK_CLOSE(0.0, trip_time, 0.0001);

            // ---------------
            // --- Test Node 9 
            // ---------------
            INodeContextFake nc_9(nodeid_suid_map.left.at(9));
            unique_ptr<IMigrationInfoVector> p_mi_9(vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                        &nc_9,
                                                                                                        nodeid_suid_map,
                                                                                                        vsp.index,
                                                                                                        &vsp.genes ) );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL(6, reachable_nodes_9.size());
            CHECK_EQUAL(1, reachable_nodes_9[0].data);
            CHECK_EQUAL(2, reachable_nodes_9[1].data);
            CHECK_EQUAL(3, reachable_nodes_9[2].data);
            CHECK_EQUAL(4, reachable_nodes_9[3].data);
            CHECK_EQUAL(5, reachable_nodes_9[4].data);
            CHECK_EQUAL(6, reachable_nodes_9[5].data);


            const std::vector<MigrationType::Enum>& mig_type_list_9 = p_mi_9->GetMigrationTypes();
            CHECK_EQUAL(6, mig_type_list_9.size());
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[0]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[1]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[2]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[3]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[4]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[5]);

            // ================
            // === FROM NODE 9 migration only to Node 1
            // ================
            m_RandomFake.SetUL(2576980377); // 0.6

            IndividualHumanContextFake traveler9(nullptr, &nc_9, nullptr, nullptr);

            // ------------------------------------------------------------------
            // --- Test that male vector (age 0) will migrate from Node 9 to 1
            // ------------------------------------------------------------------
            const std::vector<float>* fraction_traveling1 = p_mi_9->GetFractionTraveling( p_female_vector_cohort );
            CHECK_CLOSE( 0.095, fraction_traveling1->at( 0 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 1 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 2 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 3 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 4 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 5 ), 0.001 );

            // ------------------------------------------------------------------
            // --- Test that female vector (age 0) will migrate from Node 9 to node 1
            // ------------------------------------------------------------------
            traveler9.SetAge(0);
            traveler9.SetGender(Gender::FEMALE);
            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION;
            trip_time = -1.0;

            p_mi_9->PickMigrationStep(&m_RandomFake, &traveler9, 1.0, destination, mig_type, trip_time);

            CHECK_EQUAL(1, destination.data);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type);
            CHECK_CLOSE(5.1082, trip_time, 0.0001);

        }
        catch (DetailedException& re)
        {
            PrintDebug(re.GetMsg());
            CHECK(false);
        }
    }


    TEST_FIXTURE(MigrationFixture, TestUpdateFemaleRateVector)
    {
        std::string config_filename = "testdata/MigrationTest/TestBothGendersVector_config.json";

        VectorGenome genome;
        genome.SetLocus( 0, 0, 0 );

        IVectorCohort* p_vc1 = VectorCohort::CreateCohort( 1,
            VectorStateEnum::STATE_ADULT,
            0.0,
            0.0,
            0.0,
            1,
            genome,
            0 );
        try
        {
            unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile(config_filename.c_str()));

            // Female vectors migrate from odd nodes to even nodes, male vectors migrate from even nodes to odd nodes
            // Vector migration does not depend on age, so we use age=0

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for (uint32_t node_id = 1; node_id <= 9; node_id++)
            {
                suids::suid node_suid;
                node_suid.data = node_id;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "MigrationTest";
            VectorSpeciesParameters vsp(0);
            vsp.Configure(p_config.get());

            // ---------------
            // --- Test Node 2 , no migration from file for Node 2
            // ---------------
            INodeContextFake nc_2(nodeid_suid_map.left.at(2));
            unique_ptr<IMigrationInfoVector> p_mi_2(vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                        &nc_2,
                                                                                                        nodeid_suid_map,
                                                                                                        vsp.index,
                                                                                                        &vsp.genes ) );

            p_mi_2->GetFractionTraveling( p_vc1 );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL(6, reachable_nodes_2.size());
            CHECK_EQUAL(4, reachable_nodes_2[0].data);
            CHECK_EQUAL(5, reachable_nodes_2[1].data);
            CHECK_EQUAL(6, reachable_nodes_2[2].data);
            CHECK_EQUAL(7, reachable_nodes_2[3].data);
            CHECK_EQUAL(8, reachable_nodes_2[4].data);
            CHECK_EQUAL(9, reachable_nodes_2[5].data);

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL(6, mig_type_list_2.size());
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[0]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[1]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[2]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[3]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[4]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_2[5]);

            // ================
            // === FROM NODE 2
            // ================
            m_RandomFake.SetUL(2576980377); // 0.6

            IndividualHumanContextFake traveler(nullptr, &nc_2, nullptr, nullptr);

            // ------------------------------------------------------------------
            // --- Test that male vector (age 0) no migration from Node 2
            // ------------------------------------------------------------------
            const std::vector<float>* fraction_traveling = p_mi_2->GetFractionTraveling( p_vc1 );
            if( fraction_traveling != nullptr )
            {
                CHECK_EQUAL( true, false );
            }
            // ------------------------------------------------------------------
            // --- Test that female vector (age 0) no migration from Node 2
            // ------------------------------------------------------------------
            traveler.SetAge(0);
            traveler.SetGender(Gender::FEMALE);

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep(&m_RandomFake, &traveler, 1.0, destination, mig_type, trip_time);

            CHECK_EQUAL(0, destination.data);
            CHECK_EQUAL(MigrationType::NO_MIGRATION, mig_type);
            CHECK_CLOSE(0.0, trip_time, 0.0001);

            // ---------------
            // --- Test Node 9 
            // ---------------
            INodeContextFake nc_9(nodeid_suid_map.left.at(9));
            unique_ptr<IMigrationInfoVector> p_mi_9(vsp.p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                        &nc_9,
                                                                                                        nodeid_suid_map,
                                                                                                        vsp.index,
                                                                                                        &vsp.genes ) );
            p_mi_9->GetFractionTraveling( p_vc1 );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL(6, reachable_nodes_9.size());
            CHECK_EQUAL(1, reachable_nodes_9[0].data);
            CHECK_EQUAL(2, reachable_nodes_9[1].data);
            CHECK_EQUAL(3, reachable_nodes_9[2].data);
            CHECK_EQUAL(4, reachable_nodes_9[3].data);
            CHECK_EQUAL(5, reachable_nodes_9[4].data);
            CHECK_EQUAL(6, reachable_nodes_9[5].data);


            const std::vector<MigrationType::Enum>& mig_type_list_9 = p_mi_9->GetMigrationTypes();
            CHECK_EQUAL(6, mig_type_list_9.size());
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[0]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[1]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[2]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[3]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[4]);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type_list_9[5]);

            // ================
            // === FROM NODE 9 migration only to Node 1
            // ================
            m_RandomFake.SetUL(2576980377); // 0.6

            IndividualHumanContextFake traveler9(nullptr, &nc_9, nullptr, nullptr);

            // ------------------------------------------------------------------
            // --- Test that male vector (age 0) will migrate from Node 9 to 1
            // ------------------------------------------------------------------
            const std::vector<float>* fraction_traveling1 = p_mi_9->GetFractionTraveling( p_vc1 );
            CHECK_CLOSE( 0.095, fraction_traveling1->at( 0 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 1 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 2 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 3 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 4 ), 0.001 );
            CHECK_CLOSE( 0.0, fraction_traveling1->at( 5 ), 0.001 );

            // ------------------------------------------------------------------
            // --- Test that female vector (age 0) will migrate from Node 9 to node 1
            // ------------------------------------------------------------------
            traveler9.SetAge(0);
            traveler9.SetGender(Gender::FEMALE);

            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION;
            trip_time = -1.0;

            p_mi_9->PickMigrationStep(&m_RandomFake, &traveler9, 1.0, destination, mig_type, trip_time);

            CHECK_EQUAL(1, destination.data);
            CHECK_EQUAL(MigrationType::LOCAL_MIGRATION, mig_type);
            CHECK_CLOSE(5.1082, trip_time, 0.0001);

        }
        catch (DetailedException& re)
        {
            PrintDebug(re.GetMsg());
            CHECK(false);
        }
    }




    void TestHelper_FactoryConfigureException( int lineNumber, 
                                               const std::string& rFilename, 
                                               const std::string& rIdReference,
                                               uint32_t numNodes,
                                               ExternalNodeId_t nodeId,
                                               const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config_file( Environment::LoadConfigurationFile( rFilename.c_str() ) );

            unique_ptr<Configuration> p_config( Environment::CopyFromElement( (*p_config_file)["parameters"] ) );

            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= numNodes ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( p_config.get(),
                                                                                                     rIdReference ) );

            suids::suid node_suid;
            node_suid.data = nodeId;
            INodeContextFake nc( node_suid ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc, nodeid_suid_map ) );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            bool passed = msg.find( rExpMsg ) != string::npos ;
            if( !passed )
            {
                PrintDebug( msg );
            }
            CHECK_LN( passed, lineNumber );
        }
    }


    TEST_FIXTURE(MigrationFixture, TestNoLocalFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoLocalFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Local_Migration' with value 1 is incompatible with variable or parameter 'Local_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoAirFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoAirFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Air_Migration' with value 1 is incompatible with variable or parameter 'Air_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoRegionalFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoRegionalFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Regional_Migration' with value 1 is incompatible with variable or parameter 'Regional_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoSeaFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoSeaFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Sea_Migration' with value 1 is incompatible with variable or parameter 'Sea_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoMetadataFile)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoMetadataFile_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Could not find file testdata/MigrationTest/5x5_Households_Local_MigrationXXX.bin.json" );
    }

    TEST_FIXTURE(MigrationFixture, TestBadIdReference)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/config.json", 
            "BadIdReference", 
            26,
            1,
            "Variable or parameter 'Expected IdReference' with value BadIdReference is incompatible with variable or parameter 'testdata/MigrationTest/5x5_Households_Local_Migration.bin.json[Metadata][IdReference]' with value Household-Scenario-Small. " );
    }

    TEST_FIXTURE(MigrationFixture, TestDatavalueCountTooSmall)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestDatavalueCountTooSmall_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Configuration variable 'DatavalueCount' with value 0 out of range: less than 1." );
    }

    TEST_FIXTURE(MigrationFixture, TestDatavalueCountTooBig)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestDatavalueCountTooBig_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Configuration variable 'DatavalueCount' with value 999 out of range: greater than 100." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidMigrationTypeA)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidMigrationTypeA_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Expected MigrationType' with value LOCAL_MIGRATION is incompatible with variable or parameter 'testdata/MigrationTest/TestInvalidMigrationTypeA.bin.json[Metadata][MigrationType]' with value SEA_MIGRATION. ");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidMigrationTypeB)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidMigrationTypeB_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Failed to find enum match for value 'XXX' and key 'MigrationType'.\nPossible values are:\nNO_MIGRATION\nLOCAL_MIGRATION\nAIR_MIGRATION\nREGIONAL_MIGRATION\nSEA_MIGRATION\nFAMILY_MIGRATION\nINTERVENTION_MIGRATION" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidGenderDataType)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidGenderDataType_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Failed to find enum match for value 'XXX' and key 'GenderDataType'.\nPossible values are:\nSAME_FOR_BOTH_GENDERS\nONE_FOR_EACH_GENDER\nVECTOR_MIGRATION_BY_GENETICS" );

    }

    TEST_FIXTURE( MigrationFixture, TestInvalidGenderDataTypeVectorOnly )
    {
        TestHelper_FactoryConfigureException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidGenderDataTypeVectorOnly_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "testdata/MigrationTest/TestInvalidGenderDataTypeVectorOnly.bin.json[Metadata][GenderDataType] cannot be 'VECTOR_MIGRATION_BY_GENETICS' for humans." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsNotArray)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsNotArray_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Expected FLOAT VECTOR/ARRAY" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsLessThanZero)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsLessThanZero_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Configuration variable 'AgesYears' with value -1 out of range: less than 0." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsGreaterThanMax)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsGreaterThanMax_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Configuration variable 'AgesYears' with value 999 out of range: greater than 125.");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsOrder)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsOrder_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "The values in 'AgesYears' must be unique and in ascending order.");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidInterpolationType)
    {
        TestHelper_FactoryConfigureException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidInterpolationType_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "Failed to find enum match for value 'XXX' and key 'InterpolationType'.\nPossible values are:\nLINEAR_INTERPOLATION\nPIECEWISE_CONSTANT" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidOffset)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidOffset_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'offsets_str.length() / 16' with value 26 is incompatible with variable or parameter 'num_nodes' with value 999. " );
    }

    TEST_FIXTURE(MigrationFixture, TestMetadataBadJsonC)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestMetadataBadJsonC_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestMetadataBadJsonC.bin.json[Metadata][NodeCount] must be defined.\nIt must indicate the number of 'from' nodes in the data." );
    }

    TEST_FIXTURE(MigrationFixture, TestLocalMigrationFileNotFound)
    {
        std::string exp_msg;
        exp_msg  = "Could not find file TestLocalMigrationFileNotFound.bin.\n";
        exp_msg += "Received the following system error messages while checking for the existence\n";
        exp_msg += "of the file at the following locations:\n";
        exp_msg += "testdata/MigrationTest/TestLocalMigrationFileNotFound.bin - 'No such file or directory'";

        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestLocalMigrationFileNotFound_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            exp_msg );
    }

    TEST_FIXTURE(MigrationFixture, TestWrongSize)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestWrongSize_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "I/O error while reading/writing. File name =  testdata/MigrationTest/TestWrongSize.bin.  Detected wrong size for migration data file.  Expected 2400 bytes, read 2496 bytes" );
    }

    TEST_FIXTURE(MigrationFixture, TestNodeNotFound)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/config.json", 
            "Household-Scenario-Small", 
            26,
            999,
            "Variable or parameter 'rNodeIdSuidMap.right.count(node_suid)' with value 0 is incompatible with variable or parameter 'node_suid' with value 999.");
    }

    TEST_FIXTURE(MigrationFixture, TestNodesInFileNotInScenario)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/config.json", 
            "Household-Scenario-Small", 
            5,
            1,
            "NodeId, 6, found in 5x5_Households_Local_Migration.bin, is not a node in the simulation." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgeDataSection)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgeDataSection_config.json", 
            "ABC", 
            4,
            2,
            "In file 'TestInvalidAgeDataSection.bin', the 'To' Node IDs are not the same for the Age Data sections for fromNodeId = 2" );
    }

    TEST_FIXTURE( MigrationFixture, TestInvalidOffsetValues )
    {
        TestHelper_FactoryConfigureException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidOffsetValues_config.json",
            "Household-Scenario-Small",
            26,
            26,
            "Invalid 'NodeOffsets' in testdata/MigrationTest/TestInvalidOffsetValues.bin.json.\nNode ID=26 has an offset of 0xbadbeef but the '.bin' file size is expected to be 2496(0x9c0)." );
    }

    void TestHelper_FactoryConfigureVectorException(int lineNumber,
        const std::string& rFilename,
        const std::string& rIdReference,
        uint32_t numNodes,
        ExternalNodeId_t nodeId,
        const std::string& rExpMsg)
    {
        try
        {
            unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile(rFilename.c_str()));

            nodeid_suid_map_t nodeid_suid_map;
            for (uint32_t node_id = 1; node_id <= numNodes; node_id++)
            {
                suids::suid node_suid;
                node_suid.data = node_id;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            VectorSpeciesParameters vsp(0);
            vsp.Configure(p_config.get());

            suids::suid node_suid;
            node_suid.data = nodeId;
            INodeContextFake nc(node_suid);
            unique_ptr<IMigrationInfo> p_mi(vsp.p_migration_factory->CreateMigrationInfoVector(rIdReference, &nc, nodeid_suid_map, vsp.index, &vsp.genes ) );

            CHECK_LN(false, lineNumber); // should not get here
        }
        catch (DetailedException& re)
        {
            std::string msg = re.GetMsg();
            bool passed = msg.find(rExpMsg) != string::npos;
            if (!passed)
            {
                PrintDebug(msg);
            }
            CHECK_LN(passed, lineNumber);
        }
    }



    TEST_FIXTURE(MigrationFixture, TestNoMetadataFileVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestNoMetadataFileVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "Could not find file testdata/MigrationTest/5x5_Households_Local_MigrationXXX.bin.json");
    }

    TEST_FIXTURE(MigrationFixture, TestBadIdReferenceVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestVectorMigrationInfo.json",
            "BadIdReference",
            26,
            1,
            "Variable or parameter 'Expected IdReference' with value BadIdReference is incompatible with variable or parameter 'testdata/MigrationTest/Local_Vector_Migration.bin.json[Metadata][IdReference]' with value Household-Scenario-Small." );
    }

    TEST_FIXTURE(MigrationFixture, TestDatavalueCountTooSmallVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestDatavalueCountTooSmallVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "Configuration variable 'DatavalueCount' with value 0 out of range: less than 1." );
    }

    TEST_FIXTURE(MigrationFixture, TestDatavalueCountTooBigVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestDatavalueCountTooBigVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "Configuration variable 'DatavalueCount' with value 999 out of range: greater than 100." );
    }


    TEST_FIXTURE(MigrationFixture, TestInvalidGenderDataTypeVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidGenderDataTypeVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "Failed to find enum match for value 'XXX' and key 'GenderDataType'.\nPossible values are:\nSAME_FOR_BOTH_GENDERS\nONE_FOR_EACH_GENDER\nVECTOR_MIGRATION_BY_GENETICS" );
    }


    TEST_FIXTURE(MigrationFixture, TestInvalidOffsetVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidOffsetVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "Variable or parameter 'offsets_str.length() / 16' with value 26 is incompatible with variable or parameter 'num_nodes' with value 999. ");
    }

    TEST_FIXTURE(MigrationFixture, TestMetadataBadJsonCVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestMetadataBadJsonCVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "testdata/MigrationTest/TestMetadataBadJsonC.bin.json[Metadata][NodeCount] must be defined.\nIt must indicate the number of 'from' nodes in the data." );
    }

    TEST_FIXTURE(MigrationFixture, TestWrongSizeVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestWrongSizeVector_config.json",
            "Household-Scenario-Small",
            26,
            1,
            "I/O error while reading/writing. File name =  testdata/MigrationTest/TestWrongSize.bin.  Detected wrong size for migration data file.  Expected 2400 bytes, read 2496 bytes");
    }

    TEST_FIXTURE(MigrationFixture, TestNodeNotFoundVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestVectorMigrationInfo.json",
            "Household-Scenario-Small",
            26,
            999,
            "Variable or parameter 'rNodeIdSuidMap.right.count(node_suid)' with value 0 is incompatible with variable or parameter 'node_suid' with value 999.");
    }

    TEST_FIXTURE(MigrationFixture, TestNodesInFileNotInScenarioVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestVectorMigrationInfo.json",
            "Household-Scenario-Small",
            5,
            1,
            "NodeId, 6, found in Local_Vector_Migration.bin, is not a node in the simulation.");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidOffsetValuesVector)
    {
        TestHelper_FactoryConfigureVectorException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidOffsetValuesVector_config.json",
            "Household-Scenario-Small",
            26,
            26,
            "Invalid 'NodeOffsets' in testdata/MigrationTest/TestInvalidOffsetValues.bin.json.\nNode ID=26 has an offset of 0xbadbeef but the '.bin' file size is expected to be 2496(0x9c0).");
    }
}
