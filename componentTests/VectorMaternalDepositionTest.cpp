
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorGeneDriver.h"
#include "VectorGene.h"
#include "VectorTraitModifiers.h"

using namespace Kernel;

SUITE( VectorGeneDriverTest )
{
    struct VectorGeneDriverFixture
    {
        VectorGeneDriverFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
        }

        ~VectorGeneDriverFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }
    };

    TEST_FIXTURE( VectorGeneDriverFixture, TestConfigure )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneDriverTest/TestConfigure.json" ) );

        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 6, gene_collection.Size() );
        CHECK_EQUAL( 1, gene_drivers.Size() );
        VectorGeneDriver* p_driver_a2 = gene_drivers[ 0 ];

        CHECK_EQUAL( VectorGeneDriverType::INTEGRAL_AUTONOMOUS, p_driver_a2->GetDriverType() );
        CHECK_EQUAL( 1, p_driver_a2->GetDriverLocusIndex() );
        CHECK_EQUAL( 1, p_driver_a2->GetDriverAlleleIndex() );
        CHECK_EQUAL( 3, p_driver_a2->GetNumLociDriven() );
        CHECK_EQUAL( 0, p_driver_a2->GetNumAlleleToCopy( 0 ) );
        CHECK_EQUAL( 2, p_driver_a2->GetNumAlleleToCopy( 1 ) );
        CHECK_EQUAL( 2, p_driver_a2->GetNumAlleleToCopy( 2 ) );
        CHECK_EQUAL( 2, p_driver_a2->GetNumAlleleToCopy( 3 ) );
        CHECK_EQUAL( 0, p_driver_a2->GetNumAlleleToCopy( 4 ) );
        CHECK_EQUAL( 0, p_driver_a2->GetNumAlleleToCopy( 5 ) );

        // ------------------
        // -- Test CanBeDriven
        // ------------------
        VectorGenome genome_none; // X-a1-b2-c2-d2-e1:X-a1-b3-c1-d2-e2
        genome_none.SetLocus( 0, 0, 0 ); // X-X
        genome_none.SetLocus( 1, 0, 0 ); // a1-a1
        genome_none.SetLocus( 2, 1, 2 ); // b2-b3
        genome_none.SetLocus( 3, 1, 0 ); // c2-c1
        genome_none.SetLocus( 4, 1, 1 ); // d2-d2
        genome_none.SetLocus( 5, 0, 1 ); // e1-e2
        CHECK( !p_driver_a2->CanBeDriven( genome_none ) );

        VectorGenome genome_has_a2; //X-a2-b4-c2-d2-e1:X-a1-b3-c1-d2-e2
        genome_has_a2.SetLocus( 0, 0, 0 ); //  X-X
        genome_has_a2.SetLocus( 1, 1, 0 ); // a2-a1
        genome_has_a2.SetLocus( 2, 3, 2 ); // b4-b3
        genome_has_a2.SetLocus( 3, 1, 0 ); // c2-c1
        genome_has_a2.SetLocus( 4, 1, 1 ); // d2-d2
        genome_has_a2.SetLocus( 5, 0, 1 ); // e1-e2
        CHECK(  p_driver_a2->CanBeDriven( genome_has_a2 ) );

        VectorGenome genome_has_d1; // X-a1-b2-c2-d3-e2:X-a1-b3-c1-d1-e1
        genome_has_d1.SetLocus( 0, 0, 0 ); //  X-X
        genome_has_d1.SetLocus( 1, 0, 0 ); // a1-a1
        genome_has_d1.SetLocus( 2, 1, 2 ); // b2-b3
        genome_has_d1.SetLocus( 3, 1, 0 ); // c2-c1
        genome_has_d1.SetLocus( 4, 2, 0 ); // d3-d1
        genome_has_d1.SetLocus( 5, 1, 0 ); // e2-e1
        CHECK( !p_driver_a2->CanBeDriven( genome_has_d1 ) );

        VectorGenome genome_has_both;
        genome_has_both.SetLocus( 0, 0, 0 ); //  X-X
        genome_has_both.SetLocus( 1, 1, 0 ); // a2-a1
        genome_has_both.SetLocus( 2, 1, 2 ); // b2-b3
        genome_has_both.SetLocus( 3, 1, 0 ); // c2-c1
        genome_has_both.SetLocus( 4, 2, 0 ); // d3-d1
        genome_has_both.SetLocus( 5, 0, 1 ); // e1-e2
        CHECK(  p_driver_a2->CanBeDriven( genome_has_both ) );

        // -------------------------
        // --- Test DriveGenes - a2
        // ------------------------
        CHECK_EQUAL( "X-a2-b4-c2-d2-e1:X-a1-b3-c1-d2-e2", gene_collection.GetGenomeName( genome_has_a2 ) );

        GenomeProbPairVector_t possible_genomes     = p_driver_a2->DriveGenes( genome_has_a2 );
        GenomeProbPairVector_t possible_genomes_all = gene_drivers.DriveGenes( genome_has_a2 );

        // There are two because a2 copies 90% of time and failes 10%.
        // c1 is replaced 100% of time by c3 - even if a2 does not copy.
        CHECK_EQUAL( 2, possible_genomes.size() );
        CHECK_EQUAL( 2, possible_genomes_all.size() );
        CHECK_EQUAL( "X-a2-b4-c2-d2-e1:X-a1-b3-c3-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-a2-b4-c2-d2-e1:X-a2-b3-c3-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_CLOSE( 0.1, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.9, possible_genomes[ 1 ].prob, FLT_EPSILON );

        genome_has_a2.SetLocus( 2, 1, 0 );
        CHECK_EQUAL( "X-a2-b2-c2-d2-e1:X-a1-b1-c1-d2-e2", gene_collection.GetGenomeName( genome_has_a2 ) );

        possible_genomes     = p_driver_a2->DriveGenes( genome_has_a2 );
        possible_genomes_all = gene_drivers.DriveGenes( genome_has_a2 );

        // There are two possible because a2 copies to a2 90% and a1 10% of time.
        // b1 is converted to b3 100% of the time and c1 is converted 100% of the time.
        CHECK_EQUAL( 2, possible_genomes.size() );
        CHECK_EQUAL( 2, possible_genomes_all.size() );
        CHECK_EQUAL( "X-a2-b2-c2-d2-e1:X-a1-b3-c3-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-a2-b2-c2-d2-e1:X-a2-b3-c3-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_CLOSE( 0.1, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.9, possible_genomes[ 1 ].prob, FLT_EPSILON );

        genome_has_a2.SetLocus( 2, 3, 4 );
        genome_has_a2.SetLocus( 3, 0, 0 );
        CHECK_EQUAL( "X-a2-b4-c1-d2-e1:X-a1-b5-c1-d2-e2", gene_collection.GetGenomeName( genome_has_a2 ) );

        possible_genomes     = p_driver_a2->DriveGenes( genome_has_a2 );
        possible_genomes_all = gene_drivers.DriveGenes( genome_has_a2 );

        CHECK_EQUAL( 2, possible_genomes.size() );
        CHECK_EQUAL( 2, possible_genomes_all.size() );
        CHECK_EQUAL( "X-a2-b4-c1-d2-e1:X-a1-b5-c1-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-a2-b4-c1-d2-e1:X-a2-b5-c1-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( genome_has_a2.GetBits(), possible_genomes[ 0 ].genome.GetBits() );
        CHECK_CLOSE( 0.1, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.9, possible_genomes[ 1 ].prob, FLT_EPSILON );


        genome_has_a2.SetLocus( 2, 3, 1 );
        CHECK_EQUAL( "X-a2-b4-c1-d2-e1:X-a1-b2-c1-d2-e2", gene_collection.GetGenomeName( genome_has_a2 ) );

        possible_genomes     = p_driver_a2->DriveGenes( genome_has_a2 );
        possible_genomes_all = gene_drivers.DriveGenes( genome_has_a2 );

        CHECK_EQUAL( 2, possible_genomes.size() );
        CHECK_EQUAL( 2, possible_genomes_all.size() );
        CHECK_EQUAL( "X-a2-b4-c1-d2-e1:X-a1-b2-c1-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-a2-b4-c1-d2-e1:X-a2-b2-c1-d2-e2", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( genome_has_a2.GetBits(), possible_genomes[ 0 ].genome.GetBits() );
        CHECK_CLOSE( 0.1, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.9, possible_genomes[ 1 ].prob, FLT_EPSILON );

        // ------------------------------------
        // --- Test HasDriverAndHeterozygous()
        // ------------------------------------
        CHECK(  gene_drivers.HasDriverAndHeterozygous( genome_has_a2 ) ); // has driver and heterozygous
        genome_has_a2.SetLocus( 1, 1, 1 ); // a2-a2
        CHECK( !gene_drivers.HasDriverAndHeterozygous( genome_has_a2 ) ); // has driver and homozygous
        genome_has_a2.SetLocus( 1, 1, 0 ); // a2-a1
        CHECK(  gene_drivers.HasDriverAndHeterozygous( genome_has_a2 ) ); // has driver and heterozygous
        genome_has_a2.SetLocus( 1, 2, 0 ); // a3-a1
        CHECK( !gene_drivers.HasDriverAndHeterozygous( genome_has_a2 ) ); // no driver and heterozygous
        genome_has_a2.SetLocus( 1, 2, 2 ); // a3-a3
        CHECK( !gene_drivers.HasDriverAndHeterozygous( genome_has_a2 ) ); // no driver and homozygous
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestClassic )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneDriverTest/TestClassic.json" ) );

        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 2, gene_collection.Size() );
        CHECK_EQUAL( 1, gene_drivers.Size() );
        VectorGeneDriver* p_driver_a1 = gene_drivers[ 0 ];

        // -----------------------------------------------------
        // --- Test all wild - driver should not do anything
        // -----------------------------------------------------
        VectorGenome genome_all_wild; //X-Aw:X-Aw
        genome_all_wild.SetLocus( 0, 0, 0 ); //  X-X
        genome_all_wild.SetLocus( 1, 2, 2 ); // Aw-Aw
        CHECK_EQUAL( "X-Aw:X-Aw", gene_collection.GetGenomeName( genome_all_wild ) );
        CHECK( !p_driver_a1->CanBeDriven( genome_all_wild ) );

        GenomeProbPairVector_t possible_genomes = gene_drivers.DriveGenes( genome_all_wild );

        CHECK_EQUAL( 1, possible_genomes.size() );
        CHECK_EQUAL( "X-Aw:X-Aw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_CLOSE( 1.0, possible_genomes[ 0 ].prob, FLT_EPSILON );

        // --------------------------
        // --- Driver & wild
        // --------------------------
        VectorGenome genome_driver_wild; //X-Ad:X-Aw
        genome_driver_wild.SetLocus( 0, 0, 0 ); //  X-X
        genome_driver_wild.SetLocus( 1, 0, 2 ); // Ad-Aw
        CHECK_EQUAL( "X-Ad:X-Aw", gene_collection.GetGenomeName( genome_driver_wild ) );
        CHECK( p_driver_a1->CanBeDriven( genome_driver_wild ) );

        possible_genomes = gene_drivers.DriveGenes( genome_driver_wild );

        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad:X-Ad", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad:X-Am", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad:X-Aw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_CLOSE( 0.80, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.05, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.15, possible_genomes[ 2 ].prob, FLT_EPSILON );

        // ------------------------------------------
        // --- Two Drivers
        // ------------------------------------------
        VectorGenome genome_two_drivers; //X-Ad:X-Ad
        genome_two_drivers.SetLocus( 0, 0, 0 ); //  X-X
        genome_two_drivers.SetLocus( 1, 0, 0 ); // Ad-Ad
        CHECK_EQUAL( "X-Ad:X-Ad", gene_collection.GetGenomeName( genome_two_drivers ) );
        CHECK( !p_driver_a1->CanBeDriven( genome_two_drivers ) );

        possible_genomes = gene_drivers.DriveGenes( genome_two_drivers );

        CHECK_EQUAL( 1, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad:X-Ad", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_CLOSE( 1.0, possible_genomes[ 0 ].prob, FLT_EPSILON );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestAutonomousOneDriverOneEffector )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneDriverTest/TestAutonomousOneDriverOneEffector.json" ) );

        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 3, gene_collection.Size() );
        CHECK_EQUAL( 1, gene_drivers.Size() );

        // ------------------------------------------------------------------
        // --- No drivers - no change
        // ------------------------------------------------------------------
        VectorGenome genome_all_wild; //X-Aw-Bw:X-Aw-Bw
        genome_all_wild.SetLocus( 0, 0, 0 ); //  X-X
        genome_all_wild.SetLocus( 1, 2, 2 ); // Aw-Aw
        genome_all_wild.SetLocus( 2, 2, 2 ); // Bw-Bw
        CHECK_EQUAL( "X-Aw-Bw:X-Aw-Bw", gene_collection.GetGenomeName( genome_all_wild ) );

        GenomeProbPairVector_t possible_genomes = gene_drivers.DriveGenes( genome_all_wild );
        CHECK_EQUAL( 1, possible_genomes.size() );
        CHECK_EQUAL( "X-Aw-Bw:X-Aw-Bw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );

        CHECK_CLOSE( 1.0, possible_genomes[ 0 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- No driver, but effector - no change
        // ------------------------------------------------------------------
        VectorGenome genome_one_effector; //X-Aw-Be:X-Aw-Bw
        genome_one_effector.SetLocus( 0, 0, 0 ); //  X-X
        genome_one_effector.SetLocus( 1, 2, 2 ); // Aw-Aw
        genome_one_effector.SetLocus( 2, 0, 2 ); // Be-Bw
        CHECK_EQUAL( "X-Aw-Be:X-Aw-Bw", gene_collection.GetGenomeName( genome_one_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_one_effector );
        CHECK_EQUAL( 1, possible_genomes.size() );
        CHECK_EQUAL( "X-Aw-Be:X-Aw-Bw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );

        CHECK_CLOSE( 1.0, possible_genomes[ 0 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- Driver only
        // ------------------------------------------------------------------
        VectorGenome genome_one_driver_only; //X-Ad-Bw:X-Aw-Bw
        genome_one_driver_only.SetLocus( 0, 0, 0 ); //  X-X
        genome_one_driver_only.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_one_driver_only.SetLocus( 2, 2, 2 ); // Bw-Bw
        CHECK_EQUAL( "X-Ad-Bw:X-Aw-Bw", gene_collection.GetGenomeName( genome_one_driver_only ) );

        possible_genomes = gene_drivers.DriveGenes( genome_one_driver_only );
        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bw:X-Ad-Bw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw:X-Am-Bw", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw:X-Aw-Bw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );

        CHECK_CLOSE( 0.85, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.05, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.10, possible_genomes[ 2 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- One Driver and effector
        // ------------------------------------------------------------------
        VectorGenome genome_one_driver_effector; //X-Ad-Be:X-Aw-Bw
        genome_one_driver_effector.SetLocus( 0, 0, 0 ); //  X-X
        genome_one_driver_effector.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_one_driver_effector.SetLocus( 2, 0, 2 ); // Be-Bw
        CHECK_EQUAL( "X-Ad-Be:X-Aw-Bw", gene_collection.GetGenomeName( genome_one_driver_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_one_driver_effector );
        CHECK_EQUAL( 9, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Be", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Bm", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Bw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Am-Be", gene_collection.GetGenomeName( possible_genomes[ 3 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Am-Bm", gene_collection.GetGenomeName( possible_genomes[ 4 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Am-Bw", gene_collection.GetGenomeName( possible_genomes[ 5 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Aw-Be", gene_collection.GetGenomeName( possible_genomes[ 6 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Aw-Bm", gene_collection.GetGenomeName( possible_genomes[ 7 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Aw-Bw", gene_collection.GetGenomeName( possible_genomes[ 8 ].genome ) );

        CHECK_CLOSE( 0.6800, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0595, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1105, possible_genomes[ 2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0400, possible_genomes[ 3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0035, possible_genomes[ 4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0065, possible_genomes[ 5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0800, possible_genomes[ 6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0070, possible_genomes[ 7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0130, possible_genomes[ 8 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- One Driver and effector, but can't drive effector due to mutation
        // ------------------------------------------------------------------
        VectorGenome genome_one_driver_effector_mutation; //X-Ad-Be:X-Aw-Bm
        genome_one_driver_effector_mutation.SetLocus( 0, 0, 0 ); //  X-X
        genome_one_driver_effector_mutation.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_one_driver_effector_mutation.SetLocus( 2, 0, 1 ); // Be-Bm
        CHECK_EQUAL( "X-Ad-Be:X-Aw-Bm", gene_collection.GetGenomeName( genome_one_driver_effector_mutation ) );

        possible_genomes = gene_drivers.DriveGenes( genome_one_driver_effector_mutation );
        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Bm", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Am-Bm", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Aw-Bm", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );

        CHECK_CLOSE( 0.85, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.05, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.10, possible_genomes[ 2 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- Two drivers and one effector
        // ------------------------------------------------------------------
        VectorGenome genome_two_drivers_effector; //X-Ad-Be:X-Ad-Bw
        genome_two_drivers_effector.SetLocus( 0, 0, 0 ); //  X-X
        genome_two_drivers_effector.SetLocus( 1, 0, 0 ); // Ad-Ad
        genome_two_drivers_effector.SetLocus( 2, 0, 2 ); // Be-Bw
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Bw", gene_collection.GetGenomeName( genome_two_drivers_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_two_drivers_effector );
        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Be", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Bm", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Be:X-Ad-Bw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );

        CHECK_CLOSE( 0.8000000, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0700000, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1300000, possible_genomes[ 2 ].prob, FLT_EPSILON );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestAutonomousTwoDriversOneEffector )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneDriverTest/TestAutonomousTwoDriversOneEffector.json" ) );

        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 4, gene_collection.Size() );
        CHECK_EQUAL( 2, gene_drivers.Size() );
        VectorGeneDriver* p_driver_a = gene_drivers[ 0 ];
        VectorGeneDriver* p_driver_b = gene_drivers[ 1 ];

        // ------------------------------------------------------------------
        // --- All wild, no drivers, no effectors
        // ------------------------------------------------------------------
        VectorGenome genome_all_wild; //X-Aw-Bw-Cw:Y-Aw-Bw-Cw
        genome_all_wild.SetLocus( 0, 0, 4 ); //  X-Y
        genome_all_wild.SetLocus( 1, 2, 2 ); // Aw-Aw
        genome_all_wild.SetLocus( 2, 2, 2 ); // Bw-Bw
        genome_all_wild.SetLocus( 3, 2, 2 ); // Cw-Cw
        CHECK_EQUAL( "X-Aw-Bw-Cw:Y-Aw-Bw-Cw", gene_collection.GetGenomeName( genome_all_wild ) );
        CHECK( !p_driver_a->CanBeDriven( genome_all_wild ) );
        CHECK( !p_driver_b->CanBeDriven( genome_all_wild ) );

        GenomeProbPairVector_t possible_genomes = gene_drivers.DriveGenes( genome_all_wild );
        CHECK_EQUAL( 1, possible_genomes.size() );
        CHECK_EQUAL( "X-Aw-Bw-Cw:Y-Aw-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[  0 ].genome ) );

        CHECK_CLOSE( 1.0, possible_genomes[  0 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- A driver only
        // ------------------------------------------------------------------
        VectorGenome genome_A_driver_only; //X-Ad-Bw-Cw:Y-Aw-Bw-Cw
        genome_A_driver_only.SetLocus( 0, 0, 4 ); //  X-Y
        genome_A_driver_only.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_A_driver_only.SetLocus( 2, 2, 2 ); // Bw-Bw
        genome_A_driver_only.SetLocus( 3, 2, 2 ); // Cw-Cw
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Aw-Bw-Cw", gene_collection.GetGenomeName( genome_A_driver_only ) );
        CHECK(  p_driver_a->CanBeDriven( genome_A_driver_only ) );
        CHECK( !p_driver_b->CanBeDriven( genome_A_driver_only ) );

        possible_genomes = gene_drivers.DriveGenes( genome_A_driver_only );
        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Ad-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Am-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Aw-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );

        CHECK_CLOSE( 0.85, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.05, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.10, possible_genomes[ 2 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- B driver only
        // ------------------------------------------------------------------
        VectorGenome genome_B_driver_only; //X-Aw-Bw-Cw:Y-Aw-Bd-Cw
        genome_B_driver_only.SetLocus( 0, 0, 4 ); //  X-Y
        genome_B_driver_only.SetLocus( 1, 2, 2 ); // Aw-Aw
        genome_B_driver_only.SetLocus( 2, 2, 0 ); // Bw-Bd
        genome_B_driver_only.SetLocus( 3, 2, 2 ); // Cw-Cw
        CHECK_EQUAL( "X-Aw-Bw-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( genome_B_driver_only ) );
        CHECK( !p_driver_a->CanBeDriven( genome_B_driver_only ) );
        CHECK(  p_driver_b->CanBeDriven( genome_B_driver_only ) );

        possible_genomes = gene_drivers.DriveGenes( genome_B_driver_only );
        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Aw-Bd-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bm-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bw-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );

        CHECK_CLOSE( 0.74, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.06, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.20, possible_genomes[ 2 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- A driver plus effector
        // ------------------------------------------------------------------
        VectorGenome genome_A_driver_effector; //X-Ad-Bw-Cw:Y-Aw-Bw-Ce
        genome_A_driver_effector.SetLocus( 0, 0, 4 ); //  X-Y
        genome_A_driver_effector.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_A_driver_effector.SetLocus( 2, 2, 2 ); // Bw-Bw
        genome_A_driver_effector.SetLocus( 3, 2, 0 ); // Cw-Ce
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Aw-Bw-Ce", gene_collection.GetGenomeName( genome_A_driver_effector ) );
        CHECK(  p_driver_a->CanBeDriven( genome_A_driver_effector ) );
        CHECK( !p_driver_b->CanBeDriven( genome_A_driver_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_A_driver_effector );
        
        CHECK_EQUAL( 9, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bw-Ce:Y-Ad-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cm:Y-Ad-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Ad-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Ce:Y-Am-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 3 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cm:Y-Am-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 4 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Am-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 5 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Ce:Y-Aw-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 6 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cm:Y-Aw-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 7 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Aw-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 8 ].genome ) );

        CHECK_CLOSE( 0.6800, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0595, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1105, possible_genomes[ 2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0400, possible_genomes[ 3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0035, possible_genomes[ 4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0065, possible_genomes[ 5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0800, possible_genomes[ 6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0070, possible_genomes[ 7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0130, possible_genomes[ 8 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- B driver plus effector
        // ------------------------------------------------------------------
        VectorGenome genome_B_driver_effector; //X-Aw-Bw-Cw:Y-Aw-Bd-Ce
        genome_B_driver_effector.SetLocus( 0, 0, 4 ); //  X-Y
        genome_B_driver_effector.SetLocus( 1, 2, 2 ); // Ad-Aw
        genome_B_driver_effector.SetLocus( 2, 2, 0 ); // Bw-Bd
        genome_B_driver_effector.SetLocus( 3, 2, 0 ); // Cw-Ce
        CHECK_EQUAL( "X-Aw-Bw-Cw:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( genome_B_driver_effector ) );
        CHECK( !p_driver_a->CanBeDriven( genome_B_driver_effector ) );
        CHECK(  p_driver_b->CanBeDriven( genome_B_driver_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_B_driver_effector );
        CHECK_EQUAL( 9, possible_genomes.size() );
        CHECK_EQUAL( "X-Aw-Bd-Ce:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bd-Cm:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bd-Cw:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bm-Ce:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 3 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bm-Cm:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 4 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bm-Cw:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 5 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bw-Ce:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 6 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bw-Cm:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 7 ].genome ) );
        CHECK_EQUAL( "X-Aw-Bw-Cw:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 8 ].genome ) );

        CHECK_CLOSE( 0.5920, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0518, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0962, possible_genomes[ 2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0480, possible_genomes[ 3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0042, possible_genomes[ 4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0078, possible_genomes[ 5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1600, possible_genomes[ 6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0140, possible_genomes[ 7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0260, possible_genomes[ 8 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- A & B drivers only
        // ------------------------------------------------------------------
        VectorGenome genome_AB_drivers_only; //X-Ad-Bw-Cw:Y-Aw-Bd-Cw
        genome_AB_drivers_only.SetLocus( 0, 0, 4 ); //  X-Y
        genome_AB_drivers_only.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_AB_drivers_only.SetLocus( 2, 2, 0 ); // Bw-Bd
        genome_AB_drivers_only.SetLocus( 3, 2, 2 ); // Cw-Cw
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( genome_AB_drivers_only ) );
        CHECK( p_driver_a->CanBeDriven( genome_AB_drivers_only ) );
        CHECK( p_driver_b->CanBeDriven( genome_AB_drivers_only ) );

        possible_genomes = gene_drivers.DriveGenes( genome_AB_drivers_only );
        CHECK_EQUAL( 9, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bd-Cw:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bm-Cw:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Cw:Y-Am-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 3 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bm-Cw:Y-Am-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 4 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Am-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 5 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 6 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bm-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 7 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bw-Cw:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 8 ].genome ) );

        CHECK_CLOSE( 0.6290, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0510, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1700, possible_genomes[ 2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0370, possible_genomes[ 3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0030, possible_genomes[ 4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0100, possible_genomes[ 5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0740, possible_genomes[ 6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0060, possible_genomes[ 7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0200, possible_genomes[ 8 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- A & B drivers plus effector
        // ------------------------------------------------------------------
        VectorGenome genome_AB_drivers_effector; //X-Ad-Bd-Ce:Y-Aw-Bw-Cw
        genome_AB_drivers_effector.SetLocus( 0, 0, 4 ); //  X-Y
        genome_AB_drivers_effector.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_AB_drivers_effector.SetLocus( 2, 0, 2 ); // Bd-Bw
        genome_AB_drivers_effector.SetLocus( 3, 0, 2 ); // Ce-Cw
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bw-Cw", gene_collection.GetGenomeName( genome_AB_drivers_effector ) );
        CHECK( p_driver_a->CanBeDriven( genome_AB_drivers_effector ) );
        CHECK( p_driver_b->CanBeDriven( genome_AB_drivers_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_AB_drivers_effector );
        CHECK_EQUAL( 27, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[  0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bm-Ce", gene_collection.GetGenomeName( possible_genomes[  1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[  2 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cm", gene_collection.GetGenomeName( possible_genomes[  3 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bm-Cm", gene_collection.GetGenomeName( possible_genomes[  4 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Cm", gene_collection.GetGenomeName( possible_genomes[  5 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[  6 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bm-Cw", gene_collection.GetGenomeName( possible_genomes[  7 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[  8 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[  9 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bm-Ce", gene_collection.GetGenomeName( possible_genomes[ 10 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 11 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bd-Cm", gene_collection.GetGenomeName( possible_genomes[ 12 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bm-Cm", gene_collection.GetGenomeName( possible_genomes[ 13 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bw-Cm", gene_collection.GetGenomeName( possible_genomes[ 14 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 15 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bm-Cw", gene_collection.GetGenomeName( possible_genomes[ 16 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Am-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[ 17 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 18 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bm-Ce", gene_collection.GetGenomeName( possible_genomes[ 19 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 20 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bd-Cm", gene_collection.GetGenomeName( possible_genomes[ 21 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bm-Cm", gene_collection.GetGenomeName( possible_genomes[ 22 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bw-Cm", gene_collection.GetGenomeName( possible_genomes[ 23 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 24 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bm-Cw", gene_collection.GetGenomeName( possible_genomes[ 25 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Aw-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[ 26 ].genome ) );

        CHECK_CLOSE( 0.5686160, possible_genomes[  0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0461040, possible_genomes[  1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1536800, possible_genomes[  2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0497539, possible_genomes[  3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0040341, possible_genomes[  4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0134470, possible_genomes[  5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0106301, possible_genomes[  6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0008619, possible_genomes[  7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0028730, possible_genomes[  8 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0334480, possible_genomes[  9 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0027120, possible_genomes[ 10 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0090400, possible_genomes[ 11 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0029267, possible_genomes[ 12 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0002373, possible_genomes[ 13 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0007910, possible_genomes[ 14 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0006253, possible_genomes[ 15 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0000507, possible_genomes[ 16 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0001690, possible_genomes[ 17 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0668960, possible_genomes[ 18 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0054240, possible_genomes[ 19 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0180800, possible_genomes[ 20 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0058534, possible_genomes[ 21 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0004746, possible_genomes[ 22 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0015820, possible_genomes[ 23 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0012506, possible_genomes[ 24 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0001014, possible_genomes[ 25 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0003380, possible_genomes[ 26 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- Two A's and One B plus Effector
        // ------------------------------------------------------------------
        VectorGenome genome_AAB_drivers_effector; //X-Ad-Bd-Ce:Y-Ad-Bw-Cw
        genome_AAB_drivers_effector.SetLocus( 0, 0, 4 ); //  X-Y
        genome_AAB_drivers_effector.SetLocus( 1, 0, 0 ); // Ad-Ad
        genome_AAB_drivers_effector.SetLocus( 2, 0, 2 ); // Bd-Bw
        genome_AAB_drivers_effector.SetLocus( 3, 0, 2 ); // Ce-Cw
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Cw", gene_collection.GetGenomeName( genome_AAB_drivers_effector ) );
        CHECK( p_driver_a->CanBeDriven( genome_AAB_drivers_effector ) );
        CHECK( p_driver_b->CanBeDriven( genome_AAB_drivers_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_AAB_drivers_effector );

        CHECK_EQUAL( 9, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bm-Ce", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Ce", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cm", gene_collection.GetGenomeName( possible_genomes[ 3 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bm-Cm", gene_collection.GetGenomeName( possible_genomes[ 4 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Cm", gene_collection.GetGenomeName( possible_genomes[ 5 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 6 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bm-Cw", gene_collection.GetGenomeName( possible_genomes[ 7 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bw-Cw", gene_collection.GetGenomeName( possible_genomes[ 8 ].genome ) );

        CHECK_CLOSE( 0.6689600, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0542400, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1808000, possible_genomes[ 2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0585340, possible_genomes[ 3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0047460, possible_genomes[ 4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0158200, possible_genomes[ 5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0125060, possible_genomes[ 6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0010140, possible_genomes[ 7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0033800, possible_genomes[ 8 ].prob, FLT_EPSILON );

        // ------------------------------------------------------------------
        // --- Two A's and Two B's plus Effector
        // ------------------------------------------------------------------
        VectorGenome genome_AABB_drivers_effector; //X-Ad-Bd-Ce:Y-Ad-Bd-Cw
        genome_AABB_drivers_effector.SetLocus( 0, 0, 4 ); //  X-Y
        genome_AABB_drivers_effector.SetLocus( 1, 0, 0 ); // Ad-Ad
        genome_AABB_drivers_effector.SetLocus( 2, 0, 0 ); // Bd-Bd
        genome_AABB_drivers_effector.SetLocus( 3, 0, 2 ); // Ce-Cw
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( genome_AABB_drivers_effector ) );
        CHECK( p_driver_a->CanBeDriven( genome_AABB_drivers_effector ) );
        CHECK( p_driver_b->CanBeDriven( genome_AABB_drivers_effector ) );

        possible_genomes = gene_drivers.DriveGenes( genome_AABB_drivers_effector );
        CHECK_EQUAL( 3, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Ce", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cm", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce:Y-Ad-Bd-Cw", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );

        CHECK_CLOSE( 0.9040000, possible_genomes[ 0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0791000, possible_genomes[ 1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0169000, possible_genomes[ 2 ].prob, FLT_EPSILON );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestAutonomousTwoDriversTwoEffectors )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneDriverTest/TestAutonomousTwoDriversTwoEffectors.json" ) );

        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 5, gene_collection.Size() );
        CHECK_EQUAL( 2, gene_drivers.Size() );
        VectorGeneDriver* p_driver_a = gene_drivers[ 0 ];
        VectorGeneDriver* p_driver_b = gene_drivers[ 1 ];

        // ------------------------------------------------------------------
        // --- Test both drivers and both effectors
        // ------------------------------------------------------------------
        VectorGenome genome_both_both; //X-Ad-Bd-Ce-De:Y-Aw-Bw-Cw-Dw
        genome_both_both.SetLocus( 0, 0, 4 ); //  X-Y
        genome_both_both.SetLocus( 1, 0, 2 ); // Ad-Aw
        genome_both_both.SetLocus( 2, 0, 2 ); // Bd-Bw
        genome_both_both.SetLocus( 3, 0, 2 ); // Ce-Cw
        genome_both_both.SetLocus( 4, 0, 2 ); // De-Dw
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cw-Dw", gene_collection.GetGenomeName( genome_both_both ) );
        CHECK( p_driver_a->CanBeDriven( genome_both_both ) );
        CHECK( p_driver_b->CanBeDriven( genome_both_both ) );

        GenomeProbPairVector_t possible_genomes = gene_drivers.DriveGenes( genome_both_both );

        CHECK_EQUAL( 81, possible_genomes.size() );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 0 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 1 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 2 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 3 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 4 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 5 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 6 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 7 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 8 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 9 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 10 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 11 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 12 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 13 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 14 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 15 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 16 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 17 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 18 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 19 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 20 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 21 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 22 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 23 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bd-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 24 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bm-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 25 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Ad-Bw-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 26 ].genome ) );

        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 27 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 28 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 29 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 30 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 31 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 32 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 33 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 34 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 35 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 36 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 37 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 38 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 39 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 40 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 41 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 42 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 43 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 44 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 45 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 46 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 47 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 48 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 49 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 50 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bd-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 51 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bm-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 52 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Am-Bw-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 53 ].genome ) );

        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 54 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 55 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Ce-De", gene_collection.GetGenomeName( possible_genomes[ 56 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 57 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 58 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Ce-Dm", gene_collection.GetGenomeName( possible_genomes[ 59 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 60 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 61 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Ce-Dw", gene_collection.GetGenomeName( possible_genomes[ 62 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 63 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 64 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cm-De", gene_collection.GetGenomeName( possible_genomes[ 65 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 66 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 67 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cm-Dm", gene_collection.GetGenomeName( possible_genomes[ 68 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 69 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 70 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cm-Dw", gene_collection.GetGenomeName( possible_genomes[ 71 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 72 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 73 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cw-De", gene_collection.GetGenomeName( possible_genomes[ 74 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 75 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 76 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cw-Dm", gene_collection.GetGenomeName( possible_genomes[ 77 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bd-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 78 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bm-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 79 ].genome ) );
        CHECK_EQUAL( "X-Ad-Bd-Ce-De:Y-Aw-Bw-Cw-Dw", gene_collection.GetGenomeName( possible_genomes[ 80 ].genome ) );

        CHECK_CLOSE( 0.4736571, possible_genomes[  0 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0384046, possible_genomes[  1 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.1280154, possible_genomes[  2 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0744318, possible_genomes[  3 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0060350, possible_genomes[  4 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0201167, possible_genomes[  5 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0205270, possible_genomes[  6 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0016644, possible_genomes[  7 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0055478, possible_genomes[  8 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0414450, possible_genomes[  9 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0033604, possible_genomes[ 10 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0112014, possible_genomes[ 11 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0065128, possible_genomes[ 12 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0005281, possible_genomes[ 13 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0017602, possible_genomes[ 14 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0017961, possible_genomes[ 15 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0001456, possible_genomes[ 16 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0004854, possible_genomes[ 17 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0088549, possible_genomes[ 18 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0007180, possible_genomes[ 19 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0023932, possible_genomes[ 20 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0013915, possible_genomes[ 21 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0001128, possible_genomes[ 22 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0003761, possible_genomes[ 23 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0003837, possible_genomes[ 24 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0000311, possible_genomes[ 25 ].prob, FLT_EPSILON );
        CHECK_CLOSE( 0.0001037, possible_genomes[ 26 ].prob, FLT_EPSILON );

        // too much work to calculate the rest
    }

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            VectorGeneCollection gene_collection;
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();
            CHECK( true );

            VectorTraitModifiers trait_modifiers( &gene_collection );
            VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( rExpMsg + "\n" );
                PrintDebug( msg + "\n" );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidDrivingAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidDrivingAllele.json",
                                       "Constrained String (Driving_Allele) with specified value 'aXXX' invalid. Possible values are:" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidDrivingAlleleAndAlleleToCopy )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidDrivingAlleleAndAlleleToCopy.json",
                                       "Invalid Gene Driver\nThe gene driver with 'Driving_Allele'='a1' must have exactly\none entry in 'Alleles_Driven' where 'Allele_To_Copy' is the same as the 'Driving_Allele'.\nThe entry in 'Alleles_Driven' for the same locus as the 'Driving_Allele' has\n'Allele_To_Copy'='a2'" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestAlleleDrivenEmpty )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestAlleleDrivenEmpty.json",
                                       "The gene driver with 'Driving_Allele'='a2' must have exactly one entry in 'Alleles_Driven' for this allele." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestAlleleDrivenTwoForSameLocus )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestAlleleDrivenTwoForSameLocus.json",
                                       "Invalid 'Alleles_Driven' for gene driver with 'Driving_Allele'='a2'.\nThere are at least two alleles to drive that effect the same gene/locus.\n'Allele_To_Copy'='a3' and 'Allele_To_Copy'='a2' are from the same gene/locus." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestAlleleDrivenDifferentLocus )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestAlleleDrivenDifferentLocus.json",
                                       "The 'Allele_To_Copy' (='a2') and the 'Allele_To_Replace' (='b1') are not from the same gene/locus.  They must effect the same gene/locus." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidAlleleToCopy )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidAlleleToCopy.json",
                                       "Constrained String (Allele_To_Copy) with specified value 'aXXX' invalid. Possible values are" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidAlleleToReplace )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidAlleleToReplace.json",
                                       "Constrained String (Allele_To_Replace) with specified value 'aXXX' invalid. Possible values are" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodEmpty )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodEmpty.json",
                                       "Invalid 'Copy_To_Likelihood' probabilities for 'Allele_To_Copy'='a2'.\nThe sum of the probabilities equals 0 but they must sum to 1.0." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodInvalidAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodInvalidAllele.json",
                                       "Constrained String (Copy_To_Allele) with specified value 'aXXX' invalid. Possible values are" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodInvalidAlleleDifferentLocus )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodInvalidAlleleDifferentLocus.json",
                                       "Invalid allele defined in 'Copy_To_Likelihood'.\nThe allele='b1' is invalid with 'Allele_To_Copy'='a2'.\nThe allele defined in 'Copy_To_Likelihood' must be of the same gene/locus as 'Allele_To_Copy' and 'Allele_To_Replace'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodProbabilityToHigh )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodProbabilityToHigh.json",
                                       "Configuration variable 'Likelihood' with value 99.99 out of range: greater than 1." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodProbabilityToLow )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodProbabilityToLow.json",
                                       "Configuration variable 'Likelihood' with value -22.22 out of range: less than 0." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodProbabilityDontSumToOne )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodProbabilityDontSumToOne.json",
                                       "Invalid 'Copy_To_Likelihood' probabilities for 'Allele_To_Copy'='a2'.\nThe sum of the probabilities equals 1.5 but they must sum to 1.0." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestCopyToLikelihoodAlleleToReplaceNotDefined )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestCopyToLikelihoodAlleleToReplaceNotDefined.json",
                                       "Missing allele in 'Copy_To_Likelihood'.\nThe 'Allele_To_Replace'='b1' must have an entry in the 'Copy_To_Likelihood' list.\nThe value represents failure to copy and can be zero." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidMultipleOfSameDriver )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidMultipleOfSameDriver.json",
                                       "Invalid Gene Driver Overlap\nThere are at least two drivers with 'Driving_Allele'='b1'.\nThere can be only one." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidClassicDrive )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidClassicDrive.json",
                                       "Invalid Classic Drive\nClassic drive with 'Driving_Allele' = 'b1' must have exactly one entry in in 'Alleles_Driven'.\nThis entry must have 'Allele_To_Copy' equal to 'Driving_Allele'" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidCircularDrivers )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidCircularDrivers.json",
                                       "Invalid Gene Drivers\nDrivers with 'Driving_Allele' 'b1' and 'b0' are circular.\nYou cannot have drivers that attempt to replace each other." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidCircularDrivers2 )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidCircularDrivers2.json",
                                       "Invalid Gene Drivers\nDriver with 'Driving_Allele' 'b1' has 'Copy_To_Likelihood' = 'b2'.\nDriver with 'Driving_Allele' 'b2' has 'Allele_To_Replace' = 'b1'.\nThis will cause a circular issue that the model does not support at this time." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidCircularDrivers3 )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidCircularDrivers3.json",
                                       "Invalid Gene Drivers\nDriver with 'Driving_Allele' 'b2' has 'Allele_To_Replace' = 'b1'.\nDriver with 'Driving_Allele' 'b1' has 'Copy_To_Likelihood' = 'b2'.\nThis will cause a circular issue that the model does not support at this time." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidCircularEffectors )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidCircularEffectors.json",
                                       "Invalid Gene Drivers\nDriver with 'Driving_Allele' 'a1' and effector 'c1' and\ndriver with 'Driving_Allele' 'b0' and effector 'c0'\nare circular.  You cannot have effectors that attempt to replace each other." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidAutonomousDriverOverlapOtherDriverIsEffector )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidAutonomousDriverOverlapOtherDriverIsEffector.json",
                                       "Invalid Gene Driver Overlap\nDrivers with 'Driving_Allele' 'a1' and 'c1' have\n'Alleles_Driven' that operate on the locus of the other driver.\n'INTEGRAL_AUTONOMOUS' drivers can be used together but their 'Alleles_Driven'\nmust have the same alleles, probabilities, AND cannot include the locus of another driver." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidAutonomousDriverOverlapSameLocusDifferentAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidAutonomousDriverOverlapSameLocusDifferentAllele.json",
                                       "Invalid Gene Driver Overlap\nDrivers with 'Driving_Allele' 'a1' and 'c1' have effectors in 'Alleles_Driven' that are not the same.\n'INTEGRAL_AUTONOMOUS' drivers can be used together but the effectors in 'Alleles_Driven'\nmust have the same alleles AND probabilities.  Only the drivers can be different." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidAutonomousDriverOverlapSameLocusDifferentProbabilities )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidAutonomousDriverOverlapSameLocusDifferentProbabilities.json",
                                       "Invalid Gene Driver Overlap\nDrivers with 'Driving_Allele' 'a1' and 'c1' have effectors in 'Alleles_Driven' that are not the same.\n'INTEGRAL_AUTONOMOUS' drivers can be used together but the effectors in 'Alleles_Driven'\nmust have the same alleles AND probabilities.  Only the drivers can be different." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidMixOfDriverTypes )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidMixOfDriverTypes.json",
                                       "Invalid Gene Drivers\nDriver with 'Driving_Allele' 'a1' has 'Driver_Type' = 'CLASSIC'.\nDriver with 'Driving_Allele' 'c1' has 'Driver_Type' = 'INTEGRAL_AUTONOMOUS'.\nOne cannot mix driver types." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidAutonomousThreeDriversThreeEffectors )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidAutonomousThreeDriversThreeEffectors.json",
                                       "Invalid Gene Driver Overlap\nDrivers with 'Driving_Allele' 'Ad' and 'Bd' have effectors in 'Alleles_Driven' that are not the same.\n'INTEGRAL_AUTONOMOUS' drivers can be used together but the effectors in 'Alleles_Driven'\nmust have the same alleles AND probabilities.  Only the drivers can be different." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredMissingDrivingAlleleParams )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredMissingDrivingAlleleParams.json",
                                       "Parameter 'Driving_Allele_Params of VectorGeneDriver' not found in input file 'testdata/VectorGeneDriverTest/TestInvalidXShredMissingDrivingAlleleParams.json" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredMissingShreddingAlleles )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredMissingShreddingAlleles.json",
                                       "Parameter 'Shredding_Alleles of VectorGeneDriver' not found in input file 'testdata/VectorGeneDriverTest/TestInvalidXShredMissingShreddingAlleles.json" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredDrivingAlleleNotInDrivingAlleleParams )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredDrivingAlleleNotInDrivingAlleleParams.json",
                                       "The gene driver with 'Driving_Allele'='Bd'\nmust have the 'Driving_Allele' equal to 'Allele_To_Copy' in the 'Driving_Allele_Params'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredInvalidAlleleToReplace )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredInvalidAlleleToReplace.json",
                                       "Missing allele in 'Copy_To_Likelihood'.\nThe 'Allele_To_Replace'='Aw' must have an entry in the 'Copy_To_Likelihood' list.\nThe value represents failure to copy and can be zero." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredNonGenderGenes )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredNonGenderGenes.json",
                                       "Constrained String (Allele_Required) with specified value 'Be' invalid. Possible values are: \nXm\nXw\nYw" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredSameAlleles )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredSameAlleles.json",
                                       "Invalid Shredding_Alleles\nThe alleles used in 'Shredding_Alleles' must all be different.\nThe read values are:\n'Allele_Required'    = 'Yw'\n'Allele_To_Shred'    = 'Yw'\n'Allele_To_Shred_To' = 'Yw'" );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredAlleleRequiredIsNotYChromosome )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredAlleleRequiredIsNotYChromosome.json",
                                       "Invalid Shredding_Alleles\nThe 'Allele_Required' (='Xw') must be an allele that is a Y-Chromosome when using 'X_SHRED'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredAlleleToShredIsYChromosome )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredAlleleToShredIsYChromosome.json",
                                       "Invalid Shredding_Alleles\nThe 'Allele_To_Shred' (='Ym') must be an allele that is NOT a Y-Chromosome when using 'X_SHRED'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidXShredAlleleToShredToIsYChromosome )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidXShredAlleleToShredToIsYChromosome.json",
                                       "Invalid Shredding_Alleles\nThe 'Allele_To_Shred_To' (='Ym') must be an allele that is NOT a Y-Chromosome when using 'X_SHRED'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidYShredAlleleRequiredIsYChromosome )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidYShredAlleleRequiredIsYChromosome.json",
                                       "Invalid Shredding_Alleles\nThe 'Allele_Required' (='Yw') must be an allele that is NOT a Y-Chromosome when using 'Y_SHRED'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidYShredAlleleToShredIsNotYChromosome )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidYShredAlleleToShredIsNotYChromosome.json",
                                       "Invalid Shredding_Alleles\nThe 'Allele_To_Shred' (='Xm') must be an allele that is a Y-Chromosome when using 'Y_SHRED'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidYShredAlleleToShredToIsNotYChromosome )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidYShredAlleleToShredToIsNotYChromosome.json",
                                       "Invalid Shredding_Alleles\nThe 'Allele_To_Shred_To' (='Xm') must be an allele that is a Y-Chromosome when using 'Y_SHRED'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidTwoXShredDrivesSameDrivingLocus )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidTwoXShredDrivesSameDrivingLocus.json",
                                       "Invalid Gene Driver Overlap\nDrivers with 'Driving_Allele' 'Ad' and 'Am' have\n'Driving_Allele_Params' that operate on the same locus.\n'X_SHRED' drivers can be used together but their 'Driving_Allele_Params'\nmust be alleles from a different gene/locus." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidTwoXShredDrivesDifferentShreddingAlleles )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidTwoXShredDrivesDifferentShreddingAlleles.json",
                                       "Invalid Gene Driver Overlap\nDrivers with 'Driving_Allele' 'Ad' and 'Bm' have alleles in 'Shredding_Alleles' that are not the same.\n'X_SHRED' drivers can be used together but the alleles in 'Shredding_Alleles'\nmust be the same AND have the same 'Allele_Shredding_Fraction'." );
    }

    TEST_FIXTURE( VectorGeneDriverFixture, TestInvalidDaisyChainCannotDriveItself )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneDriverTest/TestInvalidDaisyChainCannotDriveItself.json",
                                       "Invalid Daisy Chain Drive\nDaisy Chain drive with 'Driving_Allele' = 'Ct' cannot drive itself.\nThis drive cannot have a 'Copy_To_Allele' equal to the 'Driving_Allele'" );
    }
}

