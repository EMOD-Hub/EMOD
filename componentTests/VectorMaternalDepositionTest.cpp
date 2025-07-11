
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorGeneDriver.h"
#include "VectorGene.h"
#include "VectorTraitModifiers.h"
#include "VectorMaternalDeposition.h"

using namespace Kernel;

SUITE( VectorMaternalDepositionTest )
{
    struct VectorMaternalDepositionFixture
    {
        VectorMaternalDepositionFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
        }

        ~VectorMaternalDepositionFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }
    };


    TEST_FIXTURE( VectorMaternalDepositionFixture, TestConfigure )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorMaternalDepositionTest/TestConfigure.json" ) );

        VectorGeneCollection gene_collection;
        VectorTraitModifiers trait_modifiers( &gene_collection );
        VectorGeneDriverCollection gene_drivers( &gene_collection, &trait_modifiers );
        VectorMaternalDepositionCollection maternal_deposition( &gene_collection, &gene_drivers );
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            gene_drivers.ConfigureFromJsonAndKey( p_config.get(), "Drivers" );
            gene_drivers.CheckConfiguration();

            maternal_deposition.ConfigureFromJsonAndKey( p_config.get(), "Maternal_Deposition" );
            maternal_deposition.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        // ------------------
        // -- Check Configuration
        // ------------------
        CHECK_EQUAL( 3, maternal_deposition.Size() );

        MaternalDeposition* md_0 = maternal_deposition[0];
        MaternalDeposition* md_1 = maternal_deposition[1];
        MaternalDeposition* md_2 = maternal_deposition[2];

        CHECK_EQUAL( 1,    md_0->GetCas9AlleleLocus() );
        CHECK_EQUAL( 1,    md_0->GetCas9AlleleIndex() );
        CHECK_EQUAL( 3,    md_0->GetAlleleToCutLocus() );
        CHECK_EQUAL( 0,    md_0->GetAlleleToCutIndex() );
        CHECK_EQUAL( 2,    md_0->GetCutToLikelihoods().Size() );
        CHECK_EQUAL( "c1", md_0->GetCutToLikelihoods()[0]->GetCopyToAlleleName() );
        CHECK_EQUAL( 0,    md_0->GetCutToLikelihoods()[0]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.7,  md_0->GetCutToLikelihoods()[0]->GetLikelihood(), FLT_EPSILON );
        CHECK_EQUAL( "c3", md_0->GetCutToLikelihoods()[1]->GetCopyToAlleleName() );
        CHECK_EQUAL( 2,    md_0->GetCutToLikelihoods()[1]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.3,  md_0->GetCutToLikelihoods()[1]->GetLikelihood(), FLT_EPSILON );


        CHECK_EQUAL( 1,    md_1->GetCas9AlleleLocus() );
        CHECK_EQUAL( 1,    md_1->GetCas9AlleleIndex() );
        CHECK_EQUAL( 1,    md_1->GetAlleleToCutLocus() );
        CHECK_EQUAL( 0,    md_1->GetAlleleToCutIndex() );
        CHECK_EQUAL( 2,    md_1->GetCutToLikelihoods().Size() );
        CHECK_EQUAL( "a1", md_1->GetCutToLikelihoods()[0]->GetCopyToAlleleName() );
        CHECK_EQUAL( 0,    md_1->GetCutToLikelihoods()[0]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.8,  md_1->GetCutToLikelihoods()[0]->GetLikelihood(), FLT_EPSILON );
        CHECK_EQUAL( "a3", md_1->GetCutToLikelihoods()[1]->GetCopyToAlleleName() );
        CHECK_EQUAL( 2,    md_1->GetCutToLikelihoods()[1]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.2,  md_1->GetCutToLikelihoods()[1]->GetLikelihood(), FLT_EPSILON );


        CHECK_EQUAL( 5,    md_2->GetCas9AlleleLocus() );
        CHECK_EQUAL( 1,    md_2->GetCas9AlleleIndex() );
        CHECK_EQUAL( 3,    md_2->GetAlleleToCutLocus() );
        CHECK_EQUAL( 0,    md_2->GetAlleleToCutIndex() );
        CHECK_EQUAL( 3,    md_2->GetCutToLikelihoods().Size() );
        CHECK_EQUAL( "c1", md_2->GetCutToLikelihoods()[0]->GetCopyToAlleleName() );
        CHECK_EQUAL( 0,    md_2->GetCutToLikelihoods()[0]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.8,  md_2->GetCutToLikelihoods()[0]->GetLikelihood(), FLT_EPSILON );
        CHECK_EQUAL( "c3", md_2->GetCutToLikelihoods()[1]->GetCopyToAlleleName() );
        CHECK_EQUAL( 2,    md_2->GetCutToLikelihoods()[1]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.15, md_2->GetCutToLikelihoods()[1]->GetLikelihood(), FLT_EPSILON );
        CHECK_EQUAL( "c4", md_2->GetCutToLikelihoods()[2]->GetCopyToAlleleName() );
        CHECK_EQUAL( 3,    md_2->GetCutToLikelihoods()[2]->GetCopyToAlleleIndex() );
        CHECK_CLOSE( 0.05, md_2->GetCutToLikelihoods()[2]->GetLikelihood(), FLT_EPSILON );

        // ------------------
        // -- Test MomCas9AlleleCount
        // ------------------

        VectorGenome genome_none; // X-a1-b2-c2-d2-e1:X-a1-b3-c1-d2-e2
        genome_none.SetLocus( 0, 0, 0 ); // X-X
        genome_none.SetLocus( 1, 0, 0 ); // a1-a1
        genome_none.SetLocus( 2, 1, 2 ); // b2-b3
        genome_none.SetLocus( 3, 1, 0 ); // c2-c1
        genome_none.SetLocus( 4, 1, 1 ); // d2-d2
        genome_none.SetLocus( 5, 0, 1 ); // e1-e2
        CHECK_EQUAL( 0, md_0->MomCas9AlleleCount( genome_none ) );
        CHECK_EQUAL( 0, md_1->MomCas9AlleleCount( genome_none ) );
        CHECK_EQUAL( 1, md_2->MomCas9AlleleCount( genome_none ) );

        VectorGenome genome_has_a2; //X-a2-b4-c2-d2-e1:X-a1-b3-c1-d2-e2
        genome_has_a2.SetLocus( 0, 0, 0 ); //  X-X
        genome_has_a2.SetLocus( 1, 1, 0 ); // a2-a1
        genome_has_a2.SetLocus( 2, 3, 2 ); // b4-b3
        genome_has_a2.SetLocus( 3, 1, 0 ); // c2-c1
        genome_has_a2.SetLocus( 4, 1, 1 ); // d2-d2
        genome_has_a2.SetLocus( 5, 1, 1 ); // e2-e2
        CHECK_EQUAL( 1, md_0->MomCas9AlleleCount( genome_has_a2 ) );
        CHECK_EQUAL( 1, md_1->MomCas9AlleleCount( genome_has_a2 ) );
        CHECK_EQUAL( 2, md_2->MomCas9AlleleCount( genome_has_a2 ) );

        VectorGenome genome_has_d1; // X-a1-b2-c2-d3-e2:X-a1-b3-c1-d1-e1
        genome_has_d1.SetLocus( 0, 0, 0 ); //  X-X
        genome_has_d1.SetLocus( 1, 0, 0 ); // a1-a1
        genome_has_d1.SetLocus( 2, 1, 2 ); // b2-b3
        genome_has_d1.SetLocus( 3, 1, 0 ); // c2-c1
        genome_has_d1.SetLocus( 4, 2, 0 ); // d3-d1
        genome_has_d1.SetLocus( 5, 1, 0 ); // e2-e1
        CHECK_EQUAL( 0, md_0->MomCas9AlleleCount( genome_has_d1 ) );
        CHECK_EQUAL( 0, md_1->MomCas9AlleleCount( genome_has_d1 ) );
        CHECK_EQUAL( 1, md_2->MomCas9AlleleCount( genome_has_d1 ) );

        VectorGenome genome_has_both;
        genome_has_both.SetLocus( 0, 0, 0 ); //  X-X
        genome_has_both.SetLocus( 1, 1, 1 ); // a2-a2
        genome_has_both.SetLocus( 2, 1, 2 ); // b2-b3
        genome_has_both.SetLocus( 3, 1, 0 ); // c2-c1
        genome_has_both.SetLocus( 4, 2, 0 ); // d3-d1
        genome_has_both.SetLocus( 5, 0, 0 ); // e1-e1
        CHECK_EQUAL( 2, md_0->MomCas9AlleleCount( genome_has_both ) );
        CHECK_EQUAL( 2, md_1->MomCas9AlleleCount( genome_has_both ) );
        CHECK_EQUAL( 0, md_2->MomCas9AlleleCount( genome_has_both ) );

        // -------------------------
        // --- Test DoMaternalDeposition 
        // ------------------------

        CHECK_EQUAL( "X-a2-b4-c2-d2-e2:X-a1-b3-c1-d2-e2", gene_collection.GetGenomeName( genome_has_a2 ) );
        VectorGeneDriver* p_driver_a2 = gene_drivers[0];

        GameteProbPairVector_t gametes;
        gametes.push_back( GameteProbPair( genome_has_a2.GetGamete( VectorGenomeGameteIndex::GAMETE_INDEX_DAD ), 1 ) );
        CHECK_EQUAL( 1, gametes.size() );
        GameteProbPairVector_t gametes2;
        gametes2.push_back( GameteProbPair( genome_has_a2.GetGamete( VectorGenomeGameteIndex::GAMETE_INDEX_DAD ), 1 ) );
        CHECK_EQUAL( 1, gametes2.size() );

        //md_0 x 1, md_1 x1, md_2 x2
        maternal_deposition.DoMaternalDeposition( genome_has_a2, gametes );

        //should be the same as above
        md_0->DoMaternalDeposition( 1, gametes2 );
        md_1->DoMaternalDeposition( 1, gametes2 );
        md_2->DoMaternalDeposition( 2, gametes2 );

        float total = 0;
        for ( int i = 0; i < gametes.size(); ++i )
        {
            CHECK( gametes[i].gamete==gametes2[i].gamete);
            CHECK( gametes[i].prob  ==gametes2[i].prob);
            total += gametes[i].prob;
        }

        // --- Check the results
        // making gametes into a genome to avoid adding GetGameteName() logic
        CHECK_EQUAL( 6, gametes.size() );
        CHECK_CLOSE( 1, total, FLT_EPSILON );
        CHECK_EQUAL( "X-a1-b3-c3-d2-e2:X-a1-b3-c3-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[0].gamete, gametes[0].gamete) ) );
        CHECK_CLOSE( 0.3912, gametes[0].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a3-b3-c3-d2-e2:X-a3-b3-c3-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[1].gamete, gametes[1].gamete ) ) );
        CHECK_CLOSE( 0.0978, gametes[1].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a1-b3-c4-d2-e2:X-a1-b3-c4-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[2].gamete, gametes[2].gamete ) ) );
        CHECK_CLOSE( 0.0504, gametes[2].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a3-b3-c4-d2-e2:X-a3-b3-c4-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[3].gamete, gametes[3].gamete ) ) );
        CHECK_CLOSE( 0.0126, gametes[3].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a1-b3-c1-d2-e2:X-a1-b3-c1-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[4].gamete, gametes[4].gamete ) ) );
        CHECK_CLOSE( 0.3584, gametes[4].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a3-b3-c1-d2-e2:X-a3-b3-c1-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[5].gamete, gametes[5].gamete ) ) );
        CHECK_CLOSE( 0.0896, gametes[5].prob, FLT_EPSILON );

        // --- Test DoMaternalDeposition with genome_has_d1, this means only md_2 will be applied once
        gametes.clear();
        gametes.push_back( GameteProbPair( genome_has_a2.GetGamete( VectorGenomeGameteIndex::GAMETE_INDEX_DAD ), 1 ) );
        CHECK_EQUAL( 1, gametes.size() );
        maternal_deposition.DoMaternalDeposition( genome_has_d1, gametes );

        CHECK_EQUAL( 3, gametes.size() );
        CHECK_EQUAL( "X-a1-b3-c1-d2-e2:X-a1-b3-c1-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[0].gamete, gametes[0].gamete ) ) );
        CHECK_CLOSE( 0.8, gametes[0].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a1-b3-c3-d2-e2:X-a1-b3-c3-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[1].gamete, gametes[1].gamete ) ) );
        CHECK_CLOSE( 0.15, gametes[1].prob, FLT_EPSILON );
        CHECK_EQUAL( "X-a1-b3-c4-d2-e2:X-a1-b3-c4-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[2].gamete, gametes[2].gamete ) ) );
        CHECK_CLOSE( 0.05, gametes[2].prob, FLT_EPSILON );

        // --- Test DoMaternalDeposition with genome_has_a2, but no available alleles to cut
        gametes.clear();
        gametes.push_back( GameteProbPair( genome_has_a2.GetGamete( VectorGenomeGameteIndex::GAMETE_INDEX_MOM ), 1 ) );
        CHECK_EQUAL( 1, gametes.size() );
        maternal_deposition.DoMaternalDeposition( genome_has_d1, gametes );

        CHECK_EQUAL( 1, gametes.size() );
        CHECK_EQUAL( "X-a2-b4-c2-d2-e2:X-a2-b4-c2-d2-e2", gene_collection.GetGenomeName( VectorGenome( gametes[0].gamete, gametes[0].gamete ) ) );
        CHECK_CLOSE( 1, gametes[0].prob, FLT_EPSILON );

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
            CHECK( true );

            // This is where the exception should be thrown
            VectorMaternalDepositionCollection maternal_deposition( &gene_collection, &gene_drivers );
            maternal_deposition.ConfigureFromJsonAndKey( p_config.get(), "Maternal_Deposition" );
            maternal_deposition.CheckConfiguration();

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

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestRedefinition )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestRedefinition.json",
                                       "At least two 'Maternal_Deposition' definitions with 'Cas9_gRNA_From' = 'a2' and 'Allele_To_Cut' = 'c1'" );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestAlleleToCutNotSameLocusCutToAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestAlleleToCutNotSameLocusCutToAllele.json",
                                       "For 'Cas9_gRNA_From'='a2' and 'Allele_To_Cut'='c1', the 'Cut_To_Allele'='a1' in 'Likelihood_Per_Cas9_gRNA_From' is invalid." );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestCutToLikelihoodsNotAddToOne )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestCutToLikelihoodsNotAddToOne.json",
                                       "Invalid 'Likelihood_Per_Cas9_gRNA_From' probabilities for 'Cas9_gRNA_From'='e2' and 'Allele_To_Cut'='c1'.\nThe sum of the probabilities is 1.05 but they must sum to 1.0." );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestDaisyChainSelfDrive )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestDaisyChainSelfDrive.json",
                                       "The 'Allele_To_Cut'='c1' is not a valid allele for the 'Cas9_gRNA_From'='c2'.\nFor 'DAISY_CHAIN' drive, the 'Driving_Allele' cannot cut the same locus (drive itself).\n" );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestMissingCas9FromParameter )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestMissingCas9FromParameter.json",
                                       "Parameter 'Cas9_gRNA_From of MaternalDeposition' not found in input file 'testdata/VectorMaternalDepositionTest/TestMissingCas9FromParameter.json'." );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestNoAlleleToCutInCutToAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestNoAlleleToCutInCutToAllele.json",
                                       "Missing allele in 'Likelihood_Per_Cas9_gRNA_From'" );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestNoDrivers )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestNoDrivers.json",
                                       "The 'Maternal_Deposition' cannot happen without gene drive and there are no 'Drivers' defined. Please define 'Drivers'." );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestNotValidDriver )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestNotValidDriver.json",
                                       "The 'Cas9_gRNA_From'='e1' is not one of the 'Driving_Allele' alleles defined in 'Drivers', but it should be." );
    }


    TEST_FIXTURE( VectorMaternalDepositionFixture, TestBadCutToLikelihoodAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestBadCutToLikelihoodAllele.json",
                                       "Constrained String (Cut_To_Allele) with specified value 'c8' invalid. Possible values are:" );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestNotAlleleToReplace )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestNotAlleleToReplace.json",
                                       "The 'Allele_To_Cut'='c3' is not a valid allele for the 'Cas9_gRNA_From'='e2'.\nThe 'Allele_To_Cut' must be one of the 'Allele_To_Replace' for 'Driving_Allele'='e2'.\n" );
    }

    TEST_FIXTURE( VectorMaternalDepositionFixture, TestCutToAlleleAlleleToCopy )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorMaternalDepositionTest/TestCutToAlleleAlleleToCopy.json",
            "Invalid 'Cut_To_Allele'='c2' in 'Likelihood_Per_Cas9_gRNA_From' for 'Cas9_gRNA_From'='a2'.\nA 'Cut_To_Allele' cannot be an 'Allele_To_Copy' for a 'Driving_Allele' ('a2')." );
    }
}

