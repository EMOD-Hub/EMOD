
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorSpeciesParameters.h"
#include "VectorParameters.h"
#include "SimulationConfig.h"
#include "MigrationInfoVector.h"
#include "INodeContextFake.h"

// maybe these shouldn't be protected in Simulation.h
typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
typedef nodeid_suid_map_t::value_type nodeid_suid_pair;

using namespace Kernel;

SUITE( VectorSpeciesParametersTest )
{
    struct VspFixture
    {
        SimulationConfig* m_pSimulationConfig ;

        VspFixture()
            : m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            m_pSimulationConfig->sim_type = SimType::VECTOR_SIM ;
            m_pSimulationConfig->demographics_initial = true ;
            Environment::setSimulationConfig( m_pSimulationConfig );
            EnvPtr->InputPaths.push_back( "." );

            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
        }

        ~VspFixture()
        {
            Environment::Finalize();
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename.c_str() ) );

            VectorSpeciesCollection collection;
            try
            {
                collection.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                collection.CheckConfiguration();

                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                if( msg.find( rExpMsg ) == string::npos )
                {
                    PrintDebug( rExpMsg );
                    PrintDebug( msg );
                    CHECK_LN( false, lineNumber );
                }
            }
        }
    };

    TEST_FIXTURE( VspFixture, TestBloodMealMortality )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/VectorSpeciesParametersTest/TestBloodMealMortality.json" );

        VectorSpeciesCollection collection;
        collection.ConfigureFromJsonAndKey( EnvPtr->Config, "Vector_Species_Params" );
        collection.CheckConfiguration();

        VectorGenome genome_0_a0b0_a0b0;
        genome_0_a0b0_a0b0.SetLocus( 0, 0, 0 );
        genome_0_a0b0_a0b0.SetLocus( 1, 0, 0 );
        genome_0_a0b0_a0b0.SetLocus( 2, 0, 0 );

        VectorGenome genome_0_a1b0_a1b1;
        genome_0_a1b0_a1b1.SetLocus( 0, 0, 0 );
        genome_0_a1b0_a1b1.SetLocus( 1, 1, 1 );
        genome_0_a1b0_a1b1.SetLocus( 2, 0, 1 );

        VectorGenome genome_0_a0b1_a1b1;
        genome_0_a0b1_a1b1.SetLocus( 0, 0, 0 );
        genome_0_a0b1_a1b1.SetLocus( 1, 0, 1 );
        genome_0_a0b1_a1b1.SetLocus( 2, 1, 1 );


        VectorGenome genome_1_c0_c0;
        genome_1_c0_c0.SetLocus( 0, 0, 0 );
        genome_1_c0_c0.SetLocus( 1, 0, 0 );

        VectorGenome genome_1_c1_c1;
        genome_1_c1_c1.SetLocus( 0, 0, 0 );
        genome_1_c1_c1.SetLocus( 1, 1, 1 );


        VectorGenome genome_2_d0_d0;
        genome_2_d0_d0.SetLocus( 0, 0, 0 );
        genome_2_d0_d0.SetLocus( 1, 0, 0 );

        VectorGenome genome_2_d1_d1;
        genome_2_d1_d1.SetLocus( 0, 0, 0 );
        genome_2_d1_d1.SetLocus( 1, 1, 1 );

        GeneticProbability bmm_0 = collection[0]->vsp_blood_meal_mortality;
        GeneticProbability bmm_1 = collection[1]->vsp_blood_meal_mortality;
        GeneticProbability bmm_2 = collection[2]->vsp_blood_meal_mortality;

        CHECK_CLOSE( 0.2, bmm_0.GetDefaultValue(), FLT_EPSILON );
        CHECK_CLOSE( 0.2, bmm_0.GetValue( 0, genome_0_a0b0_a0b0 ), FLT_EPSILON ); // not specified so default
        CHECK_CLOSE( 0.3, bmm_0.GetValue( 0, genome_0_a1b0_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, bmm_0.GetValue( 0, genome_0_a0b1_a1b1 ), FLT_EPSILON );

        CHECK_CLOSE( 0.5, bmm_1.GetDefaultValue(), FLT_EPSILON );
        CHECK_CLOSE( 0.5, bmm_1.GetValue( 1, genome_1_c0_c0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6, bmm_1.GetValue( 1, genome_1_c1_c1 ), FLT_EPSILON );

        CHECK_CLOSE( 0.7, bmm_2.GetDefaultValue(), FLT_EPSILON );
        CHECK_CLOSE( 0.7, bmm_2.GetValue( 2, genome_2_d0_d0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.8, bmm_2.GetValue( 2, genome_2_d1_d1 ), FLT_EPSILON );
    }

    TEST_FIXTURE( VspFixture, TestMigration )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/VectorSpeciesParametersTest/TestMigration.json" );

        VectorSpeciesCollection collection;
        collection.ConfigureFromJsonAndKey( EnvPtr->Config, "Vector_Species_Params" );
        collection.CheckConfiguration();

        CHECK_EQUAL( 2, collection.Size() );
        CHECK( collection[0]->p_migration_factory != nullptr );
        CHECK( collection[1]->p_migration_factory != nullptr );


        nodeid_suid_map_t nodeid_suid_map;
        for( uint32_t node_id = 1 ; node_id <= 26 ; node_id++ )
        {
            suids::suid node_suid ;
            node_suid.data = node_id ;
            nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
        }

        std::string idreference = "Household-Scenario-Small" ;

        INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;

        unique_ptr<IMigrationInfoVector> p_mi_species_1( collection[ 0 ]->p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                                          &nc_1,
                                                                                                                          nodeid_suid_map,
                                                                                                                          collection[0]->index,
                                                                                                                          &(collection[0]->genes)));

        const std::vector<suids::suid>& reachable_nodes = p_mi_species_1->GetReachableNodes();
        CHECK_EQUAL( 21, reachable_nodes.size() );
        CHECK_EQUAL(  3, reachable_nodes[ 0].data );
        CHECK_EQUAL(  4, reachable_nodes[ 1].data );
        CHECK_EQUAL(  5, reachable_nodes[ 2].data );
        CHECK_EQUAL(  8, reachable_nodes[ 3].data );
        CHECK_EQUAL(  9, reachable_nodes[ 4].data );
        CHECK_EQUAL( 10, reachable_nodes[ 5].data );
        CHECK_EQUAL( 11, reachable_nodes[ 6].data );
        CHECK_EQUAL( 12, reachable_nodes[ 7].data );
        CHECK_EQUAL( 13, reachable_nodes[ 8].data );
        CHECK_EQUAL( 14, reachable_nodes[ 9].data );
        CHECK_EQUAL( 15, reachable_nodes[10].data );
        CHECK_EQUAL( 16, reachable_nodes[11].data );
        CHECK_EQUAL( 17, reachable_nodes[12].data );
        CHECK_EQUAL( 18, reachable_nodes[13].data );
        CHECK_EQUAL( 19, reachable_nodes[14].data );
        CHECK_EQUAL( 20, reachable_nodes[15].data );
        CHECK_EQUAL( 21, reachable_nodes[16].data );
        CHECK_EQUAL( 22, reachable_nodes[17].data );
        CHECK_EQUAL( 23, reachable_nodes[18].data );
        CHECK_EQUAL( 24, reachable_nodes[19].data );
        CHECK_EQUAL( 25, reachable_nodes[20].data );

        unique_ptr<IMigrationInfoVector> p_mi_species_2( collection[ 1 ]->p_migration_factory->CreateMigrationInfoVector( idreference,
                                                                                                                          &nc_1,
                                                                                                                          nodeid_suid_map,
                                                                                                                          collection[1]->index,
                                                                                                                          &(collection[1]->genes) ) );

        const std::vector<suids::suid>& reachable_nodes2 = p_mi_species_2->GetReachableNodes();
        CHECK_EQUAL( 0, reachable_nodes2.size() );
    }

    TEST_FIXTURE( VspFixture, TestTooManySpecies )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestTooManySpecies.json",
                                       "7 (>6) species is not allowed in 'Vector_Species_Params'.\nPlease reduce the number of species you have to the maximum of 6" );
    }

    TEST_FIXTURE( VspFixture, TestDuplicateNames )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestDuplicateNames.json",
                                       "Duplicate vector species name.\nThe names of the species in 'Vector_Species_Params' must be unique.\nThe following names are defined:\nspecies_1\nspecies_2\nspecies_1" );
    }

    TEST_FIXTURE( VspFixture, TestEmptyName )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestEmptyName.json",
                                       "The 'Name' of the vector species must be defined and cannot be empty." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesDictionary )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesDictionary.json",
                                       "While trying to parse json data for param/key >>> Habitats <<< in otherwise valid json segment... \n{\n\t\"TEMPORARY_RAINFALL\" : 1.125e+10\n}\nCaught exception msg below: \nExpected ARRAY of OBJECTs" );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesElementNotObject )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesElementNotObject.json",
                                       "While trying to parse json data for param/key >>> Habitats[0] <<< in otherwise valid json segment... \n[\n\t1.125e+10\n]\nCaught exception msg below: \nExpected ARRAY of OBJECTs" );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesEmpty )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesEmpty.json",
                                       "'Habitats' for vector species = 'species_1' cannot be empty.\nPlease define at least one habitat." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesNoVectorHabitatType )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesNoVectorHabitatType.json",
                                       "'Habitat_Type' does not exist in 'Habitats[1]'.\nIt has the following JSON:\n{\n\t\"...Habitat_Type\" : \"CONSTANT\",\n\t\"Max_Larval_Capacity\" : 1.125e+10\n}" );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesEmptyElement )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesEmptyElement.json",
                                       "Found zero elements in JSON for 'Habitats[1]' in <testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesEmptyElement.json>." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesAllHabitats )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesAllHabitats.json",
                                       "Invalid 'Habitat_Type' = 'ALL_HABITATS' in 'Habitats[1]' in <testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesAllHabitats.json>." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesDuplicates )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesDuplicates.json",
                                       "Duplicate 'Habitat_Type' = 'TEMPORARY_RAINFALL'.\nOnly one habitat type per species is allowed." );
    }

    TEST_FIXTURE(VspFixture, TestMigrationParametersMissing)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/VectorSpeciesParametersTest/TestMigrationParametersMissing.json",
            "Parameter 'Vector_Migration_Food_Modifier of VectorSpeciesParameters' not found in input file 'testdata/VectorSpeciesParametersTest/TestMigrationParametersMissing.json'");
    }
}
