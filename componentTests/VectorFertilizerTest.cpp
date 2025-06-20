
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorFertilizer.h"
#include "VectorGene.h"
#include "VectorTraitModifiers.h"
#include "VectorGeneDriver.h"
#include "RANDOM.h"

using namespace Kernel;

SUITE( VectorFertilizerTest )
{
    struct VectorFertilizerFixture
    {
        VectorFertilizerFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
        }

        ~VectorFertilizerFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }
    };

    void TestHelper_Initialize( const std::string& rFilename,
                                VectorGeneCollection* pGeneCollection,
                                VectorTraitModifiers* pTraitModifiers,
                                VectorGeneDriverCollection* pGeneDrivers )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

        try
        {
            pGeneCollection->ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            pGeneCollection->CheckConfiguration();

            pTraitModifiers->ConfigureFromJsonAndKey( p_config.get(), "Gene_To_Trait_Modifiers" );
            pTraitModifiers->CheckConfiguration();

            if( pGeneDrivers != nullptr )
            {
                pGeneDrivers->ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
                pGeneDrivers->CheckConfiguration();
            }
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestCreateAllPossibleGenomesWithNames )
    {
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestCreateAllPossibleGenomesWithNames.json",
                               &gene_collection,
                               &trait_modifiers,
                               nullptr );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, nullptr, nullptr );

        CHECK_EQUAL( 3, gene_collection.Size() );
        CHECK_EQUAL( 0, trait_modifiers.Size() );

        GenomeNamePairVector_t gnp_vector = fertilizer.CreateAllPossibleGenomesWithNames();
        CHECK_EQUAL( 72, gnp_vector.size() );
        CHECK_EQUAL( "X-a1-b1:X-a1-b1", gnp_vector[  0 ].name );
        CHECK_EQUAL( "X-a1-b1:Y-a1-b1", gnp_vector[  1 ].name );
        CHECK_EQUAL( "X-a1-b1:X-a2-b1", gnp_vector[  2 ].name );
        CHECK_EQUAL( "X-a1-b1:Y-a2-b1", gnp_vector[  3 ].name );
        CHECK_EQUAL( "X-a1-b1:X-a3-b1", gnp_vector[  4 ].name );
        CHECK_EQUAL( "X-a1-b1:Y-a3-b1", gnp_vector[  5 ].name );
        CHECK_EQUAL( "X-a1-b1:X-a1-b2", gnp_vector[  6 ].name );
        CHECK_EQUAL( "X-a1-b1:Y-a1-b2", gnp_vector[  7 ].name );
        CHECK_EQUAL( "X-a1-b1:X-a2-b2", gnp_vector[  8 ].name );
        CHECK_EQUAL( "X-a1-b1:Y-a2-b2", gnp_vector[  9 ].name );
        CHECK_EQUAL( "X-a1-b1:X-a3-b2", gnp_vector[ 10 ].name );
        CHECK_EQUAL( "X-a1-b1:Y-a3-b2", gnp_vector[ 11 ].name );
        CHECK_EQUAL( "X-a2-b1:X-a1-b1", gnp_vector[ 12 ].name );
        CHECK_EQUAL( "X-a2-b1:Y-a1-b1", gnp_vector[ 13 ].name );
        CHECK_EQUAL( "X-a2-b1:X-a2-b1", gnp_vector[ 14 ].name );
        CHECK_EQUAL( "X-a2-b1:Y-a2-b1", gnp_vector[ 15 ].name );
        CHECK_EQUAL( "X-a2-b1:X-a3-b1", gnp_vector[ 16 ].name );
        CHECK_EQUAL( "X-a2-b1:Y-a3-b1", gnp_vector[ 17 ].name );
        CHECK_EQUAL( "X-a2-b1:X-a1-b2", gnp_vector[ 18 ].name );
        CHECK_EQUAL( "X-a2-b1:Y-a1-b2", gnp_vector[ 19 ].name );
        CHECK_EQUAL( "X-a2-b1:X-a2-b2", gnp_vector[ 20 ].name );
        CHECK_EQUAL( "X-a2-b1:Y-a2-b2", gnp_vector[ 21 ].name );
        CHECK_EQUAL( "X-a2-b1:X-a3-b2", gnp_vector[ 22 ].name );
        CHECK_EQUAL( "X-a2-b1:Y-a3-b2", gnp_vector[ 23 ].name );
        CHECK_EQUAL( "X-a3-b1:X-a1-b1", gnp_vector[ 24 ].name );
        CHECK_EQUAL( "X-a3-b1:Y-a1-b1", gnp_vector[ 25 ].name );
        CHECK_EQUAL( "X-a3-b1:X-a2-b1", gnp_vector[ 26 ].name );
        CHECK_EQUAL( "X-a3-b1:Y-a2-b1", gnp_vector[ 27 ].name );
        CHECK_EQUAL( "X-a3-b1:X-a3-b1", gnp_vector[ 28 ].name );
        CHECK_EQUAL( "X-a3-b1:Y-a3-b1", gnp_vector[ 29 ].name );
        CHECK_EQUAL( "X-a3-b1:X-a1-b2", gnp_vector[ 30 ].name );
        CHECK_EQUAL( "X-a3-b1:Y-a1-b2", gnp_vector[ 31 ].name );
        CHECK_EQUAL( "X-a3-b1:X-a2-b2", gnp_vector[ 32 ].name );
        CHECK_EQUAL( "X-a3-b1:Y-a2-b2", gnp_vector[ 33 ].name );
        CHECK_EQUAL( "X-a3-b1:X-a3-b2", gnp_vector[ 34 ].name );
        CHECK_EQUAL( "X-a3-b1:Y-a3-b2", gnp_vector[ 35 ].name );
        CHECK_EQUAL( "X-a1-b2:X-a1-b1", gnp_vector[ 36 ].name );
        CHECK_EQUAL( "X-a1-b2:Y-a1-b1", gnp_vector[ 37 ].name );
        CHECK_EQUAL( "X-a1-b2:X-a2-b1", gnp_vector[ 38 ].name );
        CHECK_EQUAL( "X-a1-b2:Y-a2-b1", gnp_vector[ 39 ].name );
        CHECK_EQUAL( "X-a1-b2:X-a3-b1", gnp_vector[ 40 ].name );
        CHECK_EQUAL( "X-a1-b2:Y-a3-b1", gnp_vector[ 41 ].name );
        CHECK_EQUAL( "X-a1-b2:X-a1-b2", gnp_vector[ 42 ].name );
        CHECK_EQUAL( "X-a1-b2:Y-a1-b2", gnp_vector[ 43 ].name );
        CHECK_EQUAL( "X-a1-b2:X-a2-b2", gnp_vector[ 44 ].name );
        CHECK_EQUAL( "X-a1-b2:Y-a2-b2", gnp_vector[ 45 ].name );
        CHECK_EQUAL( "X-a1-b2:X-a3-b2", gnp_vector[ 46 ].name );
        CHECK_EQUAL( "X-a1-b2:Y-a3-b2", gnp_vector[ 47 ].name );

        CHECK_EQUAL( "X-a2-b2:X-a1-b1", gnp_vector[ 48 ].name );
        CHECK_EQUAL( "X-a2-b2:Y-a1-b1", gnp_vector[ 49 ].name );
        CHECK_EQUAL( "X-a2-b2:X-a2-b1", gnp_vector[ 50 ].name );
        CHECK_EQUAL( "X-a2-b2:Y-a2-b1", gnp_vector[ 51 ].name );
        CHECK_EQUAL( "X-a2-b2:X-a3-b1", gnp_vector[ 52 ].name );
        CHECK_EQUAL( "X-a2-b2:Y-a3-b1", gnp_vector[ 53 ].name );
        CHECK_EQUAL( "X-a2-b2:X-a1-b2", gnp_vector[ 54 ].name );
        CHECK_EQUAL( "X-a2-b2:Y-a1-b2", gnp_vector[ 55 ].name );
        CHECK_EQUAL( "X-a2-b2:X-a2-b2", gnp_vector[ 56 ].name );
        CHECK_EQUAL( "X-a2-b2:Y-a2-b2", gnp_vector[ 57 ].name );
        CHECK_EQUAL( "X-a2-b2:X-a3-b2", gnp_vector[ 58 ].name );
        CHECK_EQUAL( "X-a2-b2:Y-a3-b2", gnp_vector[ 59 ].name );

        CHECK_EQUAL( "X-a3-b2:X-a1-b1", gnp_vector[ 60 ].name );
        CHECK_EQUAL( "X-a3-b2:Y-a1-b1", gnp_vector[ 61 ].name );
        CHECK_EQUAL( "X-a3-b2:X-a2-b1", gnp_vector[ 62 ].name );
        CHECK_EQUAL( "X-a3-b2:Y-a2-b1", gnp_vector[ 63 ].name );
        CHECK_EQUAL( "X-a3-b2:X-a3-b1", gnp_vector[ 64 ].name );
        CHECK_EQUAL( "X-a3-b2:Y-a3-b1", gnp_vector[ 65 ].name );
        CHECK_EQUAL( "X-a3-b2:X-a1-b2", gnp_vector[ 66 ].name );
        CHECK_EQUAL( "X-a3-b2:Y-a1-b2", gnp_vector[ 67 ].name );
        CHECK_EQUAL( "X-a3-b2:X-a2-b2", gnp_vector[ 68 ].name );
        CHECK_EQUAL( "X-a3-b2:Y-a2-b2", gnp_vector[ 69 ].name );
        CHECK_EQUAL( "X-a3-b2:X-a3-b2", gnp_vector[ 70 ].name );
        CHECK_EQUAL( "X-a3-b2:Y-a3-b2", gnp_vector[ 71 ].name );
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestDetermineInitialGenomes )
    {
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestDetermineInitialGenomes.json",
                               &gene_collection,
                               &trait_modifiers,
                               nullptr );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, nullptr, nullptr );

        CHECK_EQUAL( 2, gene_collection.Size() );
        CHECK_EQUAL( 1, trait_modifiers.Size() );

        PSEUDO_DES rng( 0 );
        GenomeCountPairVector_t genome_count_list = fertilizer.DetermineInitialGenomes( &rng, 10000 );

        CHECK_EQUAL( 50, genome_count_list.size() );

        uint32_t total_count = 0;
        uint32_t num_females = 0;
        uint32_t num_males = 0;
        uint32_t num_a1 = 0;
        uint32_t num_a2 = 0;
        uint32_t num_a3 = 0;
        uint32_t num_a4 = 0;
        uint32_t num_a5 = 0;
        for( auto gc : genome_count_list )
        {
            total_count += gc.count;
            if( gc.genome.GetGender() == VectorGender::VECTOR_FEMALE )
            {
                num_females += gc.count;
            }
            else
            {
                num_males += gc.count;
            }

            std::pair<uint8_t,uint8_t> allele_pair = gc.genome.GetLocus( 1 );
            if( allele_pair.first  == 0 ) num_a1 += gc.count;
            if( allele_pair.second == 0 ) num_a1 += gc.count;
            if( allele_pair.first  == 1 ) num_a2 += gc.count;
            if( allele_pair.second == 1 ) num_a2 += gc.count;
            if( allele_pair.first  == 2 ) num_a3 += gc.count;
            if( allele_pair.second == 2 ) num_a3 += gc.count;
            if( allele_pair.first  == 3 ) num_a4 += gc.count;
            if( allele_pair.second == 3 ) num_a4 += gc.count;
            if( allele_pair.first  == 4 ) num_a5 += gc.count;
            if( allele_pair.second == 4 ) num_a5 += gc.count;
        }
        CHECK_EQUAL( 10002, total_count ); // not 10000 because of randomRound()
        CHECK_EQUAL( 6001, num_females );
        CHECK_EQUAL( 4001, num_males );

        // Number of gametes that contain aX (not num genomes/vectors)
        CHECK_EQUAL( 8000, num_a1 ); // ~ 0.40 of 20000
        CHECK_EQUAL( 6000, num_a2 ); // ~ 0.30 of 20000
        CHECK_EQUAL( 4000, num_a3 ); // ~ 0.20 of 20000
        CHECK_EQUAL( 1202, num_a4 ); // ~ 0.06 of 20000
        CHECK_EQUAL(  802, num_a5 ); // ~ 0.04 of 20000
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestDetermineInitialGenomeData )
    {
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestDetermineInitialGenomeData.json",
                               &gene_collection,
                               &trait_modifiers,
                               nullptr );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, nullptr, nullptr );

        CHECK_EQUAL( 2, gene_collection.Size() );
        CHECK_EQUAL( 1, trait_modifiers.Size() );

        PSEUDO_DES rng( 0 );
        InitialGenomeData initial_data = fertilizer.DetermineInitialGenomeData( &rng, 10000 );

        CHECK_EQUAL( 4, initial_data.males.size() );
        CHECK_EQUAL( 16, initial_data.mated_females.size() );

        CHECK_EQUAL( 3937, initial_data.mated_females[  0 ].count );
        CHECK_EQUAL(  437, initial_data.mated_females[  1 ].count );
        CHECK_EQUAL(  437, initial_data.mated_females[  2 ].count );
        CHECK_EQUAL(   49, initial_data.mated_females[  3 ].count );
        CHECK_EQUAL(  438, initial_data.mated_females[  4 ].count );
        CHECK_EQUAL(   49, initial_data.mated_females[  5 ].count );
        CHECK_EQUAL(   49, initial_data.mated_females[  6 ].count );
        CHECK_EQUAL(    6, initial_data.mated_females[  7 ].count );
        CHECK_EQUAL(  437, initial_data.mated_females[  8 ].count );
        CHECK_EQUAL(   48, initial_data.mated_females[  9 ].count );
        CHECK_EQUAL(   49, initial_data.mated_females[ 10 ].count );
        CHECK_EQUAL(    5, initial_data.mated_females[ 11 ].count );
        CHECK_EQUAL(   49, initial_data.mated_females[ 12 ].count );
        CHECK_EQUAL(    5, initial_data.mated_females[ 13 ].count );
        CHECK_EQUAL(    6, initial_data.mated_females[ 14 ].count );
        CHECK_EQUAL(    1, initial_data.mated_females[ 15 ].count );

        uint32_t total_females = 0;
        for( auto& r_mated : initial_data.mated_females )
        {
            total_females += r_mated.count;
        }
        CHECK_EQUAL( 6002, total_females );

        uint32_t total_males = 0;
        for( auto& r_males : initial_data.males )
        {
            total_males += r_males.count;
        }
        CHECK_EQUAL( 4000, total_males );
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestDetermineFertilizedEggs )
    {
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestDetermineFertilizedEggs.json",
                               &gene_collection,
                               &trait_modifiers,
                               nullptr );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, nullptr, nullptr );

        CHECK_EQUAL( 4, gene_collection.Size() );
        CHECK_EQUAL( 1, trait_modifiers.Size() );

        VectorGenome genome_female;
        genome_female.SetLocus( 0, 0, 0 );
        genome_female.SetLocus( 1, 0, 1 );
        genome_female.SetLocus( 2, 0, 1 );
        genome_female.SetLocus( 3, 0, 1 );

        VectorGenome genome_male;
        genome_male.SetLocus( 0, 0, 4 );
        genome_male.SetLocus( 1, 0, 1 );
        genome_male.SetLocus( 2, 0, 1 );
        genome_male.SetLocus( 3, 1, 1 );

        PSEUDO_DES rng( 0 );
        GenomeCountPairVector_t genome_count_list = fertilizer.DetermineFertilizedEggs( &rng, genome_female, genome_male, 200 );

        // you might expect 96 combinations, but some of them would have zero eggs
        // so they are not inclded.
        CHECK_EQUAL( 85, genome_count_list.size() );

        uint32_t total_count = 0;
        uint32_t num_females = 0;
        uint32_t num_males = 0;
        uint32_t num_C = 0;
        uint32_t num_c = 0;
        uint32_t num_C2 = 0; //mutation
        for( auto gc : genome_count_list )
        {
            total_count += gc.count;
            if( gc.genome.GetGender() == VectorGender::VECTOR_FEMALE )
            {
                num_females += gc.count;
            }
            else
            {
                num_males += gc.count;
            }

            std::pair<uint8_t, uint8_t> ap_3 = gc.genome.GetLocus( 3 );
            if( ap_3.first  == 0 ) num_C  += gc.count;
            if( ap_3.first  == 1 ) num_c  += gc.count;
            if( ap_3.first  == 2 ) num_C2 += gc.count;
        }

        CHECK_EQUAL( 201, total_count ); // not 200 because of randomRound()
        CHECK_EQUAL( 100, num_females );
        CHECK_EQUAL( 101, num_males );

        // ------------------------------------------------------------
        // --- Check mutations.
        // --- About half of the eggs should have the 'c' allele
        // --- while 80% of the other half should have 'C' and 20% 'C2'
        // --- since the probability of mutating is 20%.
        // ------------------------------------------------------------
        CHECK_EQUAL(  81, num_C );
        CHECK_EQUAL(  99, num_c );
        CHECK_EQUAL(  21, num_C2 ); // prediction was 20 but randomRound() caused higher
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestDetermineFertilizedEggsTraitEggRatio )
    {
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestDetermineFertilizedEggsTraitEggRatio.json",
                               &gene_collection,
                               &trait_modifiers,
                               nullptr );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, nullptr, nullptr );

        CHECK_EQUAL( 4, gene_collection.Size() );
        CHECK_EQUAL( 1, trait_modifiers.Size() );

        VectorGenome genome_female;
        genome_female.SetLocus( 0, 0, 0 );
        genome_female.SetLocus( 1, 0, 1 );
        genome_female.SetLocus( 2, 0, 1 );
        genome_female.SetLocus( 3, 0, 1 );

        VectorGenome genome_male;
        genome_male.SetLocus( 0, 0, 4 );
        genome_male.SetLocus( 1, 0, 1 );
        genome_male.SetLocus( 2, 0, 1 );
        genome_male.SetLocus( 3, 1, 1 );

        PSEUDO_DES rng( 0 );
        GenomeCountPairVector_t genome_count_list = fertilizer.DetermineFertilizedEggs( &rng, genome_female, genome_male, 200 );

        CHECK_EQUAL( 64, genome_count_list.size() );

        uint32_t total_count = 0;
        uint32_t num_females = 0;
        uint32_t num_males = 0;
        for( auto gc : genome_count_list )
        {
            total_count += gc.count;
            if( gc.genome.GetGender() == VectorGender::VECTOR_FEMALE )
            {
                num_females += gc.count;
            }
            else
            {
                num_males += gc.count;
            }
        }

        // Numbers aren't exact due to randomRound()
        CHECK_EQUAL( 202, total_count );
        CHECK_EQUAL(  50, num_females );
        CHECK_EQUAL( 152, num_males );
    }

    void CheckGenderAndType( const GenomeCountPairVector_t& rGenomeCountList,
                             uint32_t expected_total_count,
                             uint32_t expected_num_females,
                             uint32_t expected_num_males,
                             uint32_t expected_num_A,
                             uint32_t expected_num_a )
    {
        uint32_t actual_total_count = 0;
        uint32_t actual_num_females = 0;
        uint32_t actual_num_males = 0;
        uint32_t actual_num_A = 0;
        uint32_t actual_num_a = 0;
        for( auto gc : rGenomeCountList )
        {
            actual_total_count += gc.count;
            if( gc.genome.GetGender() == VectorGender::VECTOR_FEMALE )
            {
                actual_num_females += gc.count;
            }
            else
            {
                actual_num_males += gc.count;
            }

            std::pair<uint8_t, uint8_t> ap_1 = gc.genome.GetLocus( 1 );
            if( ap_1.second == 0 ) actual_num_A += gc.count;
            if( ap_1.second == 1 ) actual_num_a += gc.count;
        }
        CHECK_EQUAL( expected_total_count, actual_total_count );
        CHECK_EQUAL( expected_num_females, actual_num_females );
        CHECK_EQUAL( expected_num_males  , actual_num_males   );
        CHECK_EQUAL( expected_num_A      , actual_num_A       );
        CHECK_EQUAL( expected_num_a      , actual_num_a       );
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestDriveGenesExample )
    {
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestDriveGenesExample.json",
                               &gene_collection,
                               &trait_modifiers,
                               &gene_drivers );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, &gene_drivers, nullptr );

        CHECK_EQUAL( 2, gene_collection.Size() );
        CHECK_EQUAL( 0, trait_modifiers.Size() );
        CHECK_EQUAL( 1, gene_drivers.Size() );

        VectorGenome genome_female; // X-Ad:X-Aw
        genome_female.SetLocus( 0, 0, 0 );
        genome_female.SetLocus( 1, 0, 2 );
        CHECK_EQUAL( "X-Ad:X-Aw", gene_collection.GetGenomeName( genome_female ) );

        VectorGenome genome_male; // X-Aw:Y-Aw
        genome_male.SetLocus( 0, 0, 4 );
        genome_male.SetLocus( 1, 2, 2 );
        CHECK_EQUAL( "X-Aw:Y-Aw", gene_collection.GetGenomeName( genome_male ) );

        PSEUDO_DES rng( 0 );
        GenomeCountPairVector_t genome_count_list = fertilizer.DetermineFertilizedEggs( &rng, genome_female, genome_male, 200 );

        CHECK_EQUAL( 10, genome_count_list.size() );
        CHECK_EQUAL( "X-Ad:X-Aw", gene_collection.GetGenomeName( genome_count_list[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad:Y-Aw", gene_collection.GetGenomeName( genome_count_list[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad:X-Aw", gene_collection.GetGenomeName( genome_count_list[ 2 ].genome ) );
        CHECK_EQUAL( "X-Ad:Y-Aw", gene_collection.GetGenomeName( genome_count_list[ 3 ].genome ) );
        CHECK_EQUAL( "X-Ar:X-Aw", gene_collection.GetGenomeName( genome_count_list[ 4 ].genome ) );
        CHECK_EQUAL( "X-Ar:Y-Aw", gene_collection.GetGenomeName( genome_count_list[ 5 ].genome ) );
        CHECK_EQUAL( "X-Ad:X-Aw", gene_collection.GetGenomeName( genome_count_list[ 6 ].genome ) );
        CHECK_EQUAL( "X-Ad:Y-Aw", gene_collection.GetGenomeName( genome_count_list[ 7 ].genome ) );
        CHECK_EQUAL( "X-Aw:X-Aw", gene_collection.GetGenomeName( genome_count_list[ 8 ].genome ) );
        CHECK_EQUAL( "X-Aw:Y-Aw", gene_collection.GetGenomeName( genome_count_list[ 9 ].genome ) );
        CHECK_EQUAL( 70, genome_count_list[ 0 ].count );
        CHECK_EQUAL( 70, genome_count_list[ 1 ].count );
        CHECK_EQUAL(  5, genome_count_list[ 2 ].count );
        CHECK_EQUAL(  5, genome_count_list[ 3 ].count );
        CHECK_EQUAL(  5, genome_count_list[ 4 ].count );
        CHECK_EQUAL(  5, genome_count_list[ 5 ].count );
        CHECK_EQUAL( 10, genome_count_list[ 6 ].count );
        CHECK_EQUAL( 10, genome_count_list[ 7 ].count );
        CHECK_EQUAL( 10, genome_count_list[ 8 ].count );
        CHECK_EQUAL( 10, genome_count_list[ 9 ].count );
    }

    TEST_FIXTURE( VectorFertilizerFixture, TestXShred )
    {
        // --------------------------------------------------------------------------------------------
        // --- This test is to show that the following gene drive can be implemented
        // --- in EMOD and how it works.
        // --- "A male-biased sex-distorter gene drive for the human malaria vector Anopheles gambiae"
        // --- https://www.nature.com/articles/s41587-020-0508-1/
        // --- "The SDGD X - shredder only affects the sex ratio of the progeny if it is in males.
        // --- It destroys the X chromosome while males are making their sperm, resulting
        // --- in mostly Y - bearing sperm.
        // --------------------------------------------------------------------------------------------
        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );

        TestHelper_Initialize( "testdata/VectorFertilizerTest/TestXShred.json",
                               &gene_collection,
                               &trait_modifiers,
                               &gene_drivers );

        VectorFertilizer fertilizer;
        fertilizer.Initialize( &gene_collection, &trait_modifiers, &gene_drivers, nullptr );

        CHECK_EQUAL( 2, gene_collection.Size() );
        CHECK_EQUAL( 1, trait_modifiers.Size() );
        CHECK_EQUAL( 1, gene_drivers.Size() );

        // ----------------------------------------------------------------
        // --- Test how the driver, Ad, is driven in the female gametes
        // --- The X chromosome will remain unchanged,but the Aw should be
        // --- replaced with the driver, Ad.  You'll see this in that the
        // --- female gametes are all Xw-Ad.  The male gametes should follow
        // --- normal mandelian inheritance.
        // ----------------------------------------------------------------
        VectorGenome female_genome_has_Ad; //Xw-Ad:Xw-Aw
        female_genome_has_Ad.SetLocus( 0, 1, 1 ); // Xw-Xw
        female_genome_has_Ad.SetLocus( 1, 0, 2 ); // Ad-Aw
        CHECK_EQUAL( "Xw-Ad:Xw-Aw", gene_collection.GetGenomeName( female_genome_has_Ad ) );

        VectorGenome male_genome_no_Ad; // Xw-Aw:Yw-Aw
        male_genome_no_Ad.SetLocus( 0, 1, 4 ); // Xw-Yw
        male_genome_no_Ad.SetLocus( 1, 2, 2 ); // Aw-Aw
        CHECK_EQUAL( "Xw-Aw:Yw-Aw", gene_collection.GetGenomeName( male_genome_no_Ad ) );

        PSEUDO_DES rng( 0 );
        GenomeCountPairVector_t genome_count_list = fertilizer.DetermineFertilizedEggs( &rng, female_genome_has_Ad, male_genome_no_Ad, 200 );

        CHECK_EQUAL( 2, genome_count_list.size() );
        CHECK_EQUAL( "Xw-Ad:Xw-Aw", gene_collection.GetGenomeName( genome_count_list[ 0 ].genome ) );
        CHECK_EQUAL( "Xw-Ad:Yw-Aw", gene_collection.GetGenomeName( genome_count_list[ 1 ].genome ) );
        CHECK_EQUAL( 100, genome_count_list[ 0 ].count );
        CHECK_EQUAL( 100, genome_count_list[ 1 ].count );

        // --------------------------------------------------------------------------
        // --- Test how the driver, Ad, is driven in the male gametes
        // --- when the male genome has a gender gene that can be modified.
        // --- The paper says that the gametes that would have gotten the
        // --- X chromosome (Xw) would be invalid.  To model this, we will
        // --- have those gametes get an Xm gender gene allele and then use the trait
        // --- modifiers to kill all eggs that get that allele. We should get
        // --- 50% with Yw and 50% with Xm.  The Ad driver should be copied as well so
        // --- the male gametes should be Yw-Ad & Xm-Ad.  
        // --------------------------------------------------------------------------
        VectorGenome male_genome_has_Ad; //Xw-Aw:Yw-Ad
        male_genome_has_Ad.SetLocus( 0, 1, 4 ); //  Xw-Yw
        male_genome_has_Ad.SetLocus( 1, 2, 0 ); // Aw-Ad
        CHECK_EQUAL( "Xw-Aw:Yw-Ad", gene_collection.GetGenomeName( male_genome_has_Ad ) );

        VectorGenome female_genome_no_Ad;
        female_genome_no_Ad.SetLocus( 0, 1, 1 ); //  Xw-Xw
        female_genome_no_Ad.SetLocus( 1, 2, 2 ); // Aw-Aw
        CHECK_EQUAL( "Xw-Aw:Xw-Aw", gene_collection.GetGenomeName( female_genome_no_Ad ) );

        genome_count_list = fertilizer.DetermineFertilizedEggs( &rng, female_genome_no_Ad, male_genome_has_Ad, 200 );

        // The trait modifier for Xm killed the eggs so only half are left
        CHECK_EQUAL( 1, genome_count_list.size() );
        CHECK_EQUAL( "Xw-Aw:Yw-Ad", gene_collection.GetGenomeName( genome_count_list[ 0 ].genome ) );
        CHECK_EQUAL( 100, genome_count_list[ 0 ].count );

        // --------------------------------------------------------------------------
        // --- Test how the driver, Ad, is driven in the male gametes
        // --- but this time have the driver be in the gamete from the mom.
        // --- I think we should get the same result
        // --------------------------------------------------------------------------
        VectorGenome male_genome_has_Ad_in_mom; //Xw-Ad:Yw-Aw
        male_genome_has_Ad_in_mom.SetLocus( 0, 1, 4 ); // Xw-Yw
        male_genome_has_Ad_in_mom.SetLocus( 1, 0, 2 ); // Ad-Aw
        CHECK_EQUAL( "Xw-Ad:Yw-Aw", gene_collection.GetGenomeName( male_genome_has_Ad_in_mom ) );

        genome_count_list = fertilizer.DetermineFertilizedEggs( &rng, female_genome_no_Ad, male_genome_has_Ad_in_mom, 200 );

        CHECK_EQUAL( 1, genome_count_list.size() );
        CHECK_EQUAL( "Xw-Aw:Yw-Ad", gene_collection.GetGenomeName( genome_count_list[ 0 ].genome ) );
        CHECK_EQUAL( 100, genome_count_list[ 0 ].count );
    }
}