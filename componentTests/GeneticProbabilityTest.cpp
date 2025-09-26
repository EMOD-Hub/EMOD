
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "Environment.h"
#include "Log.h"
#include "GeneticProbability.h"
#include "VectorGene.h"
#include "JsonFullReader.h"
#include "JsonFullWriter.h"

using namespace Kernel;

SUITE( GeneticProbabilityTest )
{
    struct GeneticFixture
    {
        int m_SpeciesIndexGambiae;
        int m_SpeciesIndexFunestus;
        int m_SpeciesIndexFake;
        VectorGeneCollection m_GenesGambiae;
        VectorGeneCollection m_GenesFunestus;

        VectorGenome m_Genome_a1b1c1_a1b1c1;
        VectorGenome m_Genome_a1b1c1_a1b2c1;
        VectorGenome m_Genome_a1b1c1_a1b3c1;
        VectorGenome m_Genome_a1b1c1_a1b4c1;

        VectorGenome m_Genome_a2b1c1_a2b1c1;
        VectorGenome m_Genome_a1b2c1_a1b2c1;
        VectorGenome m_Genome_a2b2c1_a2b2c1;
        VectorGenome m_Genome_a2b2c1_a2b1c1;
        VectorGenome m_Genome_a1b1c1_a2b1c1;
        VectorGenome m_Genome_a2b1c1_a1b1c1;
        VectorGenome m_Genome_a1b2c1_a2b1c1;
        VectorGenome m_Genome_a1b1c1_a2b2c1;

        VectorGenome m_Genome_a2b3c1_a2b3c1;

        VectorGenome m_Genome_a1b2c1_a1b1c1;
        VectorGenome m_Genome_a1b2c1_a1b3c1;
        VectorGenome m_Genome_a1b2c1_a1b4c1;

        VectorGenome m_Genome_a1b3c1_a1b3c1;


        VectorGenome m_Genome_a1b1c3_a1b1c3;
        VectorGenome m_Genome_a1b2c3_a1b2c3;
        VectorGenome m_Genome_a2b1c3_a2b1c3;
        VectorGenome m_Genome_a2b2c3_a2b2c3;

        VectorGenome m_Genome_d1e1_d1e1;
        VectorGenome m_Genome_d1e2_d1e2;
        VectorGenome m_Genome_d2e1_d2e1;
        VectorGenome m_Genome_d2e2_d2e2;


        GeneticFixture()
            : m_SpeciesIndexGambiae( 0 )
            , m_SpeciesIndexFunestus( 1 )
            , m_SpeciesIndexFake( 2 )
            , m_GenesGambiae()
            , m_GenesFunestus()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            LoadGenes( "testdata/GeneticProbabilityTest/gambiae.json", m_GenesGambiae );
            LoadGenes( "testdata/GeneticProbabilityTest/funestus.json", m_GenesFunestus );
            CreateGenomes();
        }

        ~GeneticFixture()
        {
            Environment::Finalize();
        }

        void CreateGenomes()
        {
            m_Genome_a1b1c1_a1b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c1_a1b1c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b1c1_a1b1c1.SetLocus( 2, 0, 0 );
            m_Genome_a1b1c1_a1b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b1c1_a1b2c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c1_a1b2c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b1c1_a1b2c1.SetLocus( 2, 0, 1 );
            m_Genome_a1b1c1_a1b2c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b1c1_a1b3c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c1_a1b3c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b1c1_a1b3c1.SetLocus( 2, 0, 2 );
            m_Genome_a1b1c1_a1b3c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b1c1_a1b4c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c1_a1b4c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b1c1_a1b4c1.SetLocus( 2, 0, 3 );
            m_Genome_a1b1c1_a1b4c1.SetLocus( 3, 0, 0 );

            m_Genome_a2b1c1_a2b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a2b1c1_a2b1c1.SetLocus( 1, 1, 1 );
            m_Genome_a2b1c1_a2b1c1.SetLocus( 2, 0, 0 );
            m_Genome_a2b1c1_a2b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b2c1_a1b2c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b2c1_a1b2c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b2c1_a1b2c1.SetLocus( 2, 1, 1 );
            m_Genome_a1b2c1_a1b2c1.SetLocus( 3, 0, 0 );

            m_Genome_a2b2c1_a2b2c1.SetLocus( 0, 0, 0 );
            m_Genome_a2b2c1_a2b2c1.SetLocus( 1, 1, 1 );
            m_Genome_a2b2c1_a2b2c1.SetLocus( 2, 1, 1 );
            m_Genome_a2b2c1_a2b2c1.SetLocus( 3, 0, 0 );

            m_Genome_a2b2c1_a2b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a2b2c1_a2b1c1.SetLocus( 1, 1, 1 );
            m_Genome_a2b2c1_a2b1c1.SetLocus( 2, 1, 0 );
            m_Genome_a2b2c1_a2b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b1c1_a2b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c1_a2b1c1.SetLocus( 1, 0, 1 );
            m_Genome_a1b1c1_a2b1c1.SetLocus( 2, 0, 0 );
            m_Genome_a1b1c1_a2b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a2b1c1_a1b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a2b1c1_a1b1c1.SetLocus( 1, 1, 0 );
            m_Genome_a2b1c1_a1b1c1.SetLocus( 2, 0, 0 );
            m_Genome_a2b1c1_a1b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b2c1_a2b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b2c1_a2b1c1.SetLocus( 1, 0, 1 );
            m_Genome_a1b2c1_a2b1c1.SetLocus( 2, 1, 0 );
            m_Genome_a1b2c1_a2b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b1c1_a2b2c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c1_a2b2c1.SetLocus( 1, 0, 1 );
            m_Genome_a1b1c1_a2b2c1.SetLocus( 2, 0, 1 );
            m_Genome_a1b1c1_a2b2c1.SetLocus( 3, 0, 0 );

            m_Genome_a2b3c1_a2b3c1.SetLocus( 0, 0, 0 );
            m_Genome_a2b3c1_a2b3c1.SetLocus( 1, 1, 1 );
            m_Genome_a2b3c1_a2b3c1.SetLocus( 2, 2, 2 );
            m_Genome_a2b3c1_a2b3c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b2c1_a1b1c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b2c1_a1b1c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b2c1_a1b1c1.SetLocus( 2, 1, 0 );
            m_Genome_a1b2c1_a1b1c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b2c1_a1b3c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b2c1_a1b3c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b2c1_a1b3c1.SetLocus( 2, 1, 2 );
            m_Genome_a1b2c1_a1b3c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b2c1_a1b4c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b2c1_a1b4c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b2c1_a1b4c1.SetLocus( 2, 1, 3 );
            m_Genome_a1b2c1_a1b4c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b3c1_a1b3c1.SetLocus( 0, 0, 0 );
            m_Genome_a1b3c1_a1b3c1.SetLocus( 1, 0, 0 );
            m_Genome_a1b3c1_a1b3c1.SetLocus( 2, 2, 2 );
            m_Genome_a1b3c1_a1b3c1.SetLocus( 3, 0, 0 );

            m_Genome_a1b1c3_a1b1c3.SetLocus( 0, 0, 0 );
            m_Genome_a1b1c3_a1b1c3.SetLocus( 1, 0, 0 );
            m_Genome_a1b1c3_a1b1c3.SetLocus( 2, 0, 0 );
            m_Genome_a1b1c3_a1b1c3.SetLocus( 3, 2, 2 );

            m_Genome_a1b2c3_a1b2c3.SetLocus( 0, 0, 0 );
            m_Genome_a1b2c3_a1b2c3.SetLocus( 1, 0, 0 );
            m_Genome_a1b2c3_a1b2c3.SetLocus( 2, 1, 1 );
            m_Genome_a1b2c3_a1b2c3.SetLocus( 3, 2, 2 );

            m_Genome_a2b1c3_a2b1c3.SetLocus( 0, 0, 0 );
            m_Genome_a2b1c3_a2b1c3.SetLocus( 1, 1, 1 );
            m_Genome_a2b1c3_a2b1c3.SetLocus( 2, 0, 0 );
            m_Genome_a2b1c3_a2b1c3.SetLocus( 3, 2, 2 );

            m_Genome_a2b2c3_a2b2c3.SetLocus( 0, 0, 0 );
            m_Genome_a2b2c3_a2b2c3.SetLocus( 1, 1, 1 );
            m_Genome_a2b2c3_a2b2c3.SetLocus( 2, 2, 2 );
            m_Genome_a2b2c3_a2b2c3.SetLocus( 3, 2, 2 );

            m_Genome_d1e1_d1e1.SetLocus( 0, 0, 0 );
            m_Genome_d1e1_d1e1.SetLocus( 1, 0, 0 );
            m_Genome_d1e1_d1e1.SetLocus( 2, 0, 0 );

            m_Genome_d1e2_d1e2.SetLocus( 0, 0, 0 );
            m_Genome_d1e2_d1e2.SetLocus( 1, 0, 0 );
            m_Genome_d1e2_d1e2.SetLocus( 2, 1, 1 );

            m_Genome_d2e1_d2e1.SetLocus( 0, 0, 0 );
            m_Genome_d2e1_d2e1.SetLocus( 1, 1, 1 );
            m_Genome_d2e1_d2e1.SetLocus( 2, 0, 0 );

            m_Genome_d2e2_d2e2.SetLocus( 0, 0, 0 );
            m_Genome_d2e2_d2e2.SetLocus( 1, 1, 1 );
            m_Genome_d2e2_d2e2.SetLocus( 2, 1, 1 );
        }

        void LoadGenes( const char* pFilename, VectorGeneCollection& rGenes )
        {
            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( pFilename ) );
                rGenes.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
                rGenes.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                CHECK( false );
            }
        }

        void CreateAlleleComboInput( const VectorGeneCollection& rGenes,
                                     const std::string& g1a1,
                                     const std::string& g1a2,
                                     const std::string& g2a1,
                                     const std::string& g2a2,
                                     VectorGameteBitPair_t* pBitMask,
                                     std::vector<VectorGameteBitPair_t>* pPossibleGenomes )
        {
            release_assert( !g1a1.empty() && !g1a2.empty() );

            std::vector<std::vector<std::string>> combo_strings;
            std::vector<std::string> a1a2;
            a1a2.push_back( g1a1 );
            a1a2.push_back( g1a2 );
            combo_strings.push_back( a1a2 );
            if( !g2a1.empty() )
            {
                release_assert( !g2a1.empty() && !g2a2.empty() );
                std::vector<std::string> b1b1;
                b1b1.push_back( g2a1 );
                b1b1.push_back( g2a2 );
                combo_strings.push_back( b1b1 );
            }

            rGenes.ConvertAlleleCombinationsStrings( "TestAlleleCombo", combo_strings,
                                                     pBitMask, pPossibleGenomes );

        }

        GeneticProbability CreateProbability( VectorGeneCollection& rGenes,
                                              const std::string& rAlleleName1,
                                              const std::string& rAlleleName2,
                                              float resistance,
                                              float efficacy )
        {
            VectorGameteBitPair_t genome_bit_mask;
            std::vector<VectorGameteBitPair_t> possible_genomes;
            CreateAlleleComboInput( rGenes, rAlleleName1, rAlleleName2, "", "", &genome_bit_mask, &possible_genomes );
            AlleleCombo ac( 0, genome_bit_mask, possible_genomes );
            AlleleComboProbability acp( ac, resistance );
            GeneticProbability gp_resistance = 1.0f;
            gp_resistance.Add( acp );

            GeneticProbability prob = efficacy;
            prob *= gp_resistance;

            return prob;
        }
    };

    TEST_FIXTURE( GeneticFixture, TestAlleleComboOrderBug )
    {
        int species_index = 0;
        VectorGeneCollection genes_simple;
        LoadGenes( "testdata/GeneticProbabilityTest/simple.json", genes_simple );

        // -------------------------------------------------------------------------------------------
        // --- Pretend you are making a multi-insecticide intervention with blocking and killing
        // --- where one insecticide is (a0,*) and the other (a1,*).  Doing simple math with these
        // --- probabilities should result in a set of 3 ACPs - (a0,*), (a1,*), and (a0,a1) + (a1,a0).
        // --- We want them to stay in this order so that if you were to multiply a GP that had
        // --- similar ACPs, you would still get the same ACP's.  We don't want to see an extra
        // --- (a0,a1) + (a1,a0).
        // -------------------------------------------------------------------------------------------
        GeneticProbability carbamate_blocking  = CreateProbability( genes_simple, "a1", "*", 0.75f, 0.25f );
        GeneticProbability carbamate_killing   = CreateProbability( genes_simple, "a1", "*", 0.25f, 1.00f );
        GeneticProbability pyrethroid_blocking = CreateProbability( genes_simple, "a0", "*", 0.75f, 0.25f );
        GeneticProbability pyrethroid_killing  = CreateProbability( genes_simple, "a0", "*", 0.25f, 1.0f );

        GeneticProbability p_kill = 1.0f - (1.0f - carbamate_killing)*(1.0f - pyrethroid_killing);

        GeneticProbability p_block =  1.0f - (1.0f - pyrethroid_blocking)*(1.0f - carbamate_blocking);

        //printf( "p_kill =\n%s\n", p_kill.ToString( 0, genes_simple ).c_str() );
        //printf( "p_block =\n%s\n", p_block_housing.ToString( 0, genes_simple ).c_str() );

        GeneticProbability block_x_kill = p_block * p_kill;
        //printf( "block_x_kill =\n%s\n", block_x_kill.ToString( 0, genes_simple ).c_str() );

        CHECK_EQUAL( 3, block_x_kill.GetNumAlleleComboProbabilities() );

        GeneticProbability kill_x_block = p_block * p_kill;

        //printf( "kill_x_block =\n%s\n", kill_x_block.ToString( 0, genes_simple ).c_str() );
        CHECK_EQUAL( 3, kill_x_block.GetNumAlleleComboProbabilities() );

        CHECK( block_x_kill == kill_x_block );
    }

    TEST_FIXTURE( GeneticFixture, TestVectorDeposit )
    {
        // -----------------------------------------------------------------------------------------
        // --- This test attempts to simulate a vector deposits into StrainAwareTransmissionGroupsGP.
        // --- Once in SATGGP, we are going to pretend that this value is the "total contagion".
        // --- We then multiply this total contagion by the person's probability to acquire (PA)
        // --- which involves some resistance.  What we want is the PA to be combined with the
        // --- appropriate vector deposits.
        // --- NOTE:  The ACPs does increase in this example because the PA could be combined
        // ---        with more than the exact genomes.
        // -----------------------------------------------------------------------------------------

        // ----------------------------
        // --- Create vector Deposits
        // ----------------------------
        GeneticProbability vd_gp_a1a1( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1, 1.0f );
        GeneticProbability vd_gp_a1a2( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1, 2.0f );
        GeneticProbability vd_gp_a2a1( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1, 4.0f );
        GeneticProbability vd_gp_a2a2( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1, 8.0f );

        // ----------------------------------------------------------------------------------
        // --- Pretend that we are adding these deposits to StrainAwareTransmissionGroupsGP
        // --- All the same strain.
        // ----------------------------------------------------------------------------------
        GeneticProbability satggp_gp = 0.0f;
        satggp_gp += vd_gp_a1a1;
        CHECK_EQUAL( 1.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(   1, satggp_gp.GetNumAlleleComboProbabilities() );

        satggp_gp += vd_gp_a1a2;
        CHECK_EQUAL( 1.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 2.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(   2, satggp_gp.GetNumAlleleComboProbabilities() );

        satggp_gp += vd_gp_a2a1;
        CHECK_EQUAL( 1.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 2.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 4.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(   3, satggp_gp.GetNumAlleleComboProbabilities() );

        satggp_gp += vd_gp_a2a2;
        CHECK_EQUAL( 1.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 2.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 4.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 8.0, satggp_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(   4, satggp_gp.GetNumAlleleComboProbabilities() );

        GeneticProbability pa_a2a2_gp   = CreateProbability( m_GenesGambiae, "a2", "a2",  3.0f, 1.0f );
        GeneticProbability pa_a1a2_gp   = CreateProbability( m_GenesGambiae, "a1", "a2",  5.0f, 1.0f );
        GeneticProbability pa_a1star_gp = CreateProbability( m_GenesGambiae, "a1", "*",  10.0f, 1.0f );

        // ---------------------------------------------------------------------
        // --- Pretend we get the total contagion from the IContagionPopulation
        // ---------------------------------------------------------------------
        GeneticProbability tc_gp = satggp_gp;

        // -----------------------------------------------------------------------------------
        // --- Let's multiply the different Probabilities to Acquire times the total contagion
        // --- to verify it is getting multiplied times the correct deposits.
        // -----------------------------------------------------------------------------------
        tc_gp *= pa_a2a2_gp;
        CHECK_EQUAL(  1.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL(  2.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL(  4.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 24.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(    5, tc_gp.GetNumAlleleComboProbabilities() ); //added a2-*:a2-* - The other four are exact

        tc_gp *= pa_a1a2_gp;
        CHECK_EQUAL(  1.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 10.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 20.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 24.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(    6, tc_gp.GetNumAlleleComboProbabilities() ); // added a1-*:a2*

        // reverse order
        tc_gp = pa_a1star_gp * tc_gp;
        CHECK_EQUAL(  10.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 100.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 200.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL(  24.0, tc_gp.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL(     7, tc_gp.GetNumAlleleComboProbabilities() ); // added a1-*:*-*
    }

    TEST_FIXTURE( GeneticFixture, TestAlleleCombo )
    {
        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a1", "a2", "b1", "b1", &genome_bit_mask, &possible_genomes );

        AlleleCombo ac1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );
        CHECK_EQUAL( 1, ac1.GetRefCount() );

        AlleleCombo* p_ac2 = new AlleleCombo( ac1 );
        CHECK_EQUAL( 2, ac1.GetRefCount() );
        CHECK( ac1 == *p_ac2 );

        delete p_ac2;
        CHECK_EQUAL( 1, ac1.GetRefCount() );

        // ------------------------------------------------------------------
        // --- Create an 'if' block that the compiler will not optimize way.
        // --- Do an assignment so that we see the reference count increase
        // --- Then check after the block to see that the reference went awa.
        // ------------------------------------------------------------------
        if( ac1.GetRefCount() == 1 )
        {
            AlleleCombo inner_ac;
            inner_ac = ac1;
            CHECK_EQUAL( 2, ac1.GetRefCount() );
        }
        CHECK_EQUAL( 1, ac1.GetRefCount() );

        // ---------------------------------------------------------
        // --- Test that we can loose the reference by null objects
        // ---------------------------------------------------------
        AlleleCombo ac_null;
        CHECK_EQUAL( -1, ac_null.GetRefCount() ); // -1 implies null

        ac1 = ac_null;
        CHECK_EQUAL( -1, ac1.GetRefCount() );
        CHECK_EQUAL( -1, ac_null.GetRefCount() );
    }

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilityAssignment )
    {
        GeneticProbability gp1;
        gp1 = 0.75;
        CHECK_EQUAL( 0.75f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp2 = 0.25;
        CHECK_EQUAL( 0.25f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        gp2 = gp1;
        CHECK_EQUAL( 0.75f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.75f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.6f );

        GeneticProbability gp3 = acp_a2a2;

        CHECK_EQUAL( 0.0f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.6f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
    }

#if 1
    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilityAddScalar )
    {
        GeneticProbability gp1 = 0.25;
        CHECK_EQUAL( 0.25f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp2 = gp1 + 0.25;
        CHECK_EQUAL( 0.25f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.50f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp3 = 0.25 + gp1;
        CHECK_EQUAL( 0.25f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.50f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp4 = 0.25 + gp1 + 0.25;
        CHECK_EQUAL( 0.25f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.75f, gp4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.4f );

        GeneticProbability gpx1 = acp_a2a2;
        CHECK_EQUAL( 0.0f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.4f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );

        GeneticProbability gpx2 = gpx1 + 0.2f;
        CHECK_EQUAL( 0.0f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.4f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.2f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.6f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );

        GeneticProbability gpx3 = 0.2f + gpx1;
        CHECK_EQUAL( 0.0f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.4f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.2f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.6f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );

        GeneticProbability gpx4 = 0.2f + gpx1 + 0.3f;
        CHECK_CLOSE( 0.0f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        // ensure we are doing assignment and not copy constructor
        GeneticProbability gpx5;
        gpx5 = gpx1;
        CHECK_EQUAL( 0.0f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.4f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
    }

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilityAdd )
    {
        GeneticProbability gp1 = 0.1f;
        GeneticProbability gp2 = 0.2f;
        CHECK_EQUAL( 0.1f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.2f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        // --------------
        // --- EXAMPLE #1 - Default + Default
        // --------------
        GeneticProbability gp3 = gp1 + gp2;

        CHECK_EQUAL( 0.1f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.3f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        // ---------------
        // --- Create a2a2
        // ---------------
        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.2f );

        GeneticProbability gp_a2a2 = acp_a2a2;

        CHECK_EQUAL( 0.0f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.2f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.0f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.2f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        // --------------
        // --- EXAMPLE #2 - a2a2 + Default
        // --------------
        GeneticProbability gpx2 = gp_a2a2 + gp1;

        CHECK_EQUAL( 0.1f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.3f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.1f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.3f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gpx2 = gp1 + gp_a2a2;  // test commutative 

        CHECK_EQUAL( 0.1f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.3f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.1f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.3f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gpx2 = gp1 + gp_a2a2 + gp1;

        CHECK_EQUAL( 0.2f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.2f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        // --------------
        // --- EXAMPLE #3 - a2a2 + a2a2
        // --------------
        gpx2 = gp_a2a2 + gp_a2a2;

        CHECK_EQUAL( 0.0f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.0f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gpx2 = gp_a2a2 + gp1 + gp_a2a2;

        CHECK_EQUAL( 0.1f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.5f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.1f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.5f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        // ------------------------
        // --- Create a2-b1:a2-b1
        // ------------------------
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "b1", "b1", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2b1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2b1( a2b1, 0.35f );
        GeneticProbability gp_a2b1 = acp_a2b1;

        CHECK_EQUAL( 0.00f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.35f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.00f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.00f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gp_a2b1 = gp1 + gp_a2b1;

        CHECK_CLOSE( 0.10f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.45f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #6 - GP only has a specific combination and not its subsets
        // --------------
        GeneticProbability gp_a2b1_p_a2a2 = gp_a2b1 + gp_a2a2;

        CHECK_CLOSE( 0.10f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.65f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.30f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gp_a2b1_p_a2a2 = gp_a2a2 + gp_a2b1;  // test commutative 

        CHECK_CLOSE( 0.10f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.65f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.30f, gp_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // -----------------
        // --- Create b1:b1
        // -----------------
        CreateAlleleComboInput( m_GenesGambiae, "b1", "b1", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo b1b1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_b1b1( b1b1, 0.15f );
        GeneticProbability gp_b1b1 = acp_b1b1;

        CHECK_EQUAL( 0.15f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.15f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.00f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.00f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gp_b1b1 = gp1 + gp_b1b1;

        CHECK_CLOSE( 0.25f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.25f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        GeneticProbability gpx4 = gp_b1b1 + gp1;

        CHECK_CLOSE( 0.35f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.35f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.20f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.20f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #4 - a2a2 + b1b1 - see that we create a new combination of a2b1_a2b1
        // --------------
        GeneticProbability gp_a2b1_a2b1 = gp_b1b1 + gp_a2a2;

        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.45f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.30f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gp_a2b1_a2b1 = gp_a2a2 + gp_b1b1;  // test commutative 

        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.45f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.10f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.30f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #7 - Add (a2:a2, b1:b1, a2-b1:a2-b1) with (a2:a2)
        // --------------
        GeneticProbability gp_a2b1_a2b1_p_a2a2 = gp_a2b1_a2b1 + gp_a2a2;

        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); //**-b1-**
        CHECK_CLOSE( 0.65f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); //a2-b1-**
        CHECK_CLOSE( 0.10f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); //**-**-** - default
        CHECK_CLOSE( 0.50f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); //a2-**-**

        gp_a2b1_a2b1_p_a2a2 = gp_a2a2 + gp_a2b1_a2b1;  // test commutative 

        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); //**-b1-**
        CHECK_CLOSE( 0.65f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); //a2-b1-**
        CHECK_CLOSE( 0.10f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); //**-**-** - default
        CHECK_CLOSE( 0.50f, gp_a2b1_a2b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); //a2-**-**

        // --------------
        // --- EXAMPLE #7 - (again) Add (a2:a2, b1:b1, a2-b1:a2-b1) with (b1:b1)
        // --------------
        GeneticProbability gp_a2b1_a2b1_p_b1b1 = gp_a2b1_a2b1 + gp_b1b1;

        CHECK_CLOSE( 0.50f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.20f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.40f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gp_a2b1_a2b1_p_b1b1 = gp_b1b1 + gp_a2b1_a2b1;  // test commutative 
        CHECK_CLOSE( 0.50f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.20f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.40f, gp_a2b1_a2b1_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #8 - Add (a2:a2, b1:b1, a2-b1:a2-b1) with (a2:a2, b1:b1, a2-b1:a2-b1)
        // --------------
        GeneticProbability gp_a2b1_a2b1_2x = gp_a2b1_a2b1 + gp_a2b1_a2b1;

        CHECK_CLOSE( 0.50f, gp_a2b1_a2b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.90f, gp_a2b1_a2b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.20f, gp_a2b1_a2b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.60f, gp_a2b1_a2b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // ------------------
        // --- Create a1:a1
        // ------------------
        CreateAlleleComboInput( m_GenesGambiae, "a1", "a1", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a1a1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a1a1( a1a1, 0.001f );
        GeneticProbability gp_a1a1 = acp_a1a1;

        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.000f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.000f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gp_a1a1 = gp_a1a1 + 0.001f;

        CHECK_EQUAL( 0.002f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.002f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        // --------------
        // --- EXAMPLE #4 - (again) a1a1 + b1b1 - see that we create a new combination of a1b1_a1b1
        // --------------
        GeneticProbability gp_a1b1_a1b1 = gp_a1a1 + gp_b1b1;

        CHECK_EQUAL( 0.252f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.251f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.102f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.101f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );
        CHECK_EQUAL( 0.251f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.101f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ) );

        // --------------
        // --- EXAMPLE #10 - stack of a2-b1:a2-b1 with stack of a1-b1:a1-b1 (B's are same but A's are different)
        // --------------
        GeneticProbability gp_a1b1_a1b1_p_a2b1_a2b1 = gp_a1b1_a1b1 + gp_a2b1_a2b1;

        CHECK_CLOSE( 0.502f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.701f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.202f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.401f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.501f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.201f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ), FLT_EPSILON );

        gp_a1b1_a1b1_p_a2b1_a2b1 = gp_a2b1_a2b1 + gp_a1b1_a1b1;  // test commutative 

        CHECK_CLOSE( 0.502f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.701f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.202f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.401f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.501f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.201f, gp_a1b1_a1b1_p_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ), FLT_EPSILON );

        // ------------------
        // --- Create c3:c3
        // ------------------
        CreateAlleleComboInput( m_GenesGambiae, "c3", "c3", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo c3c3( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_c3c3( c3c3, 0.00001f );
        GeneticProbability gp_c3c3 = acp_c3c3;

        CHECK_EQUAL( 0.00001f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) );
        CHECK_EQUAL( 0.00001f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ) );
        CHECK_EQUAL( 0.00001f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ) );
        CHECK_EQUAL( 0.00000f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gp_c3c3 = gp_c3c3 + 0.00001f;

        CHECK_EQUAL( 0.00002f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) );
        CHECK_EQUAL( 0.00002f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ) );
        CHECK_EQUAL( 0.00002f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ) );
        CHECK_EQUAL( 0.00001f, gp_c3c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        // --------------
        // --- EXAMPLE #4 - (again) b1b1 + c3c3 - see that we create a new combination of b1c3_b1c3
        // --------------
        GeneticProbability gp_b1c3_b1c3 = gp_c3c3 + gp_b1b1;

        CHECK_EQUAL( 0.25002f, gp_b1c3_b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) );
        CHECK_EQUAL( 0.25001f, gp_b1c3_b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.10002f, gp_b1c3_b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ) );
        CHECK_EQUAL( 0.10001f, gp_b1c3_b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );

        // --------------
        // --- EXAMPLE #11 - Common B but different loci
        // --------------
        GeneticProbability gp_a2b1c3_a2b1c3 = gp_a2b1_a2b1 + gp_b1c3_b1c3;

        CHECK_CLOSE( 0.70002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ), FLT_EPSILON ); //a2-b1-c3
        CHECK_CLOSE( 0.50002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ), FLT_EPSILON ); //**-b1-c3
        CHECK_CLOSE( 0.40002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c3_a2b2c3 ), FLT_EPSILON ); //a2-**-c3
        CHECK_CLOSE( 0.70001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); //a2-b1-**
        CHECK_CLOSE( 0.20002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); //**-**-c3
        CHECK_CLOSE( 0.50001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); //**-b1-**
        CHECK_CLOSE( 0.40001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); //a2-**-**
        CHECK_CLOSE( 0.20001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); //**-**-** - default

        gp_a2b1c3_a2b1c3 = gp_b1c3_b1c3 + gp_a2b1_a2b1;  // test commutative 

        CHECK_CLOSE( 0.70002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ), FLT_EPSILON ); //a2-b1-c3
        CHECK_CLOSE( 0.50002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ), FLT_EPSILON ); //**-b1-c3
        CHECK_CLOSE( 0.40002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c3_a2b2c3 ), FLT_EPSILON ); //a2-**-c3
        CHECK_CLOSE( 0.70001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); //a2-b1-**
        CHECK_CLOSE( 0.20002f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); //**-**-c3
        CHECK_CLOSE( 0.50001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); //**-b1-**
        CHECK_CLOSE( 0.40001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); //a2-**-**
        CHECK_CLOSE( 0.20001f, gp_a2b1c3_a2b1c3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); //**-**-** - default

        // ------------------------------------------------------------------------------
        // --- Tests previously had probabilities that were homozygous for a given locus.
        // --- Now we need to add in some heterozygous.
        // ------------------------------------------------------------------------------
        CreateAlleleComboInput( m_GenesGambiae, "b1", "b2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo b1b2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_b1b2( b1b2, 0.001f );
        GeneticProbability gp_b1b2 = acp_b1b2;

        CHECK_EQUAL( 0.000f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) );
        CHECK_EQUAL( 0.000f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ) );
        CHECK_EQUAL( 0.000f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ) );
        CHECK_EQUAL( 0.000f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );
        CHECK_EQUAL( 0.001f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ) );
        CHECK_EQUAL( 0.001f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ) );

        gp_b1b2 = gp_b1b2 + 0.001f;

        CHECK_EQUAL( 0.001f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) ); // default
        CHECK_EQUAL( 0.001f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ) ); // default
        CHECK_EQUAL( 0.002f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ) ); // b1b2
        CHECK_EQUAL( 0.002f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ) ); // b1b2

        // -----------------
        // --- Add with self
        // -----------------
        gp_b1b2 = gp_b1b2 + gp_b1b2;

        CHECK_EQUAL( 0.002f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) ); // default
        CHECK_EQUAL( 0.002f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ) ); // default
        CHECK_EQUAL( 0.004f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ) ); // b1b2
        CHECK_EQUAL( 0.004f, gp_b1b2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ) ); // b1b2

        // --------------
        // --- Add b1b1
        // --------------
        GeneticProbability gp_b1b2_p_b1b1 = gp_b1b2 + gp_b1b1;

        CHECK_EQUAL( 0.252f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ) ); // b1b1
        CHECK_EQUAL( 0.252f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ) ); // b1b1
        CHECK_EQUAL( 0.104f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ) ); // b1b2
        CHECK_EQUAL( 0.104f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ) ); // b1b2
        CHECK_EQUAL( 0.102f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ) ); // b2b2 - default

        // --------------
        // --- Add
        // --------------
        gp_b1b2_p_b1b1 = gp_b1b2_p_b1b1 + gp_b1b2;

        CHECK_CLOSE( 0.254f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ), FLT_EPSILON ); // b1b1
        CHECK_CLOSE( 0.254f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ), FLT_EPSILON ); // b1b1
        CHECK_CLOSE( 0.108f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ), FLT_EPSILON ); // b1b2
        CHECK_CLOSE( 0.108f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ), FLT_EPSILON ); // b1b2
        CHECK_CLOSE( 0.104f, gp_b1b2_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); // b2b2 - default

        // --------------
        // --- Add 2 x gp_b1b2_p_b1b1 
        // --------------
        GeneticProbability gp_b1b2_p_b1b1_2x = gp_b1b2_p_b1b1 + gp_b1b2_p_b1b1;
        CHECK_CLOSE( 0.508f, gp_b1b2_p_b1b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c3_a1b1c3 ), FLT_EPSILON ); // b1b1
        CHECK_CLOSE( 0.508f, gp_b1b2_p_b1b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c3_a2b1c3 ), FLT_EPSILON ); // b1b1
        CHECK_CLOSE( 0.216f, gp_b1b2_p_b1b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ), FLT_EPSILON ); // b1b2
        CHECK_CLOSE( 0.216f, gp_b1b2_p_b1b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ), FLT_EPSILON ); // b1b2
        CHECK_CLOSE( 0.208f, gp_b1b2_p_b1b1_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); // b2b2 - default

        // --------------
        // --- Add b1b2_p_b1b1 + a2a2
        // --------------
        gp_a2a2 = gp_a2a2 + 0.01f;

        GeneticProbability gp_b1b2_p_b1b1_p_a2a2 = gp_b1b2_p_b1b1 + gp_a2a2;

        CHECK_CLOSE( 0.318f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b2:a2-b1
        CHECK_CLOSE( 0.464f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.118f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ), FLT_EPSILON ); // **-b1:**-b2
        CHECK_CLOSE( 0.264f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-b1
        CHECK_CLOSE( 0.314f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.114f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); // **-**:**-** - default

        gp_b1b2_p_b1b1_p_a2a2 = gp_a2a2 + gp_b1b2_p_b1b1;  // test commutative 

        CHECK_CLOSE( 0.318f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b2:a2-b1
        CHECK_CLOSE( 0.464f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.118f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ), FLT_EPSILON ); // **-b1:**-b2
        CHECK_CLOSE( 0.264f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-b1
        CHECK_CLOSE( 0.314f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.114f, gp_b1b2_p_b1b1_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); // **-**:**-** - default

        GeneticProbability gp_b1b2_p_b1b1_p_a2a2_2x = gp_b1b2_p_b1b1_p_a2a2 + gp_b1b2_p_b1b1_p_a2a2;

        CHECK_CLOSE( 0.636f, gp_b1b2_p_b1b1_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b2:a2-b1
        CHECK_CLOSE( 0.928f, gp_b1b2_p_b1b1_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.236f, gp_b1b2_p_b1b1_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b2c1 ), FLT_EPSILON ); // **-b1:**-b2
        CHECK_CLOSE( 0.528f, gp_b1b2_p_b1b1_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-b1
        CHECK_CLOSE( 0.628f, gp_b1b2_p_b1b1_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.228f, gp_b1b2_p_b1b1_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c3_a1b2c3 ), FLT_EPSILON ); // **-**:**-** - default

        // ------------------------------------------------------------------------------
        // --- Now try a combo that is concerned about just the presence of one allele.
        // ------------------------------------------------------------------------------
        CreateAlleleComboInput( m_GenesGambiae, "b1", "*", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo b1star( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_b1start( b1star, 0.001f );
        GeneticProbability gp_b1star = acp_b1start;

        CHECK_EQUAL( 0.001f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.001f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.001f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.001f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.000f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-**:**-**

        gp_b1star = gp_b1star + 0.001f;

        CHECK_EQUAL( 0.002f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.002f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.002f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.002f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.001f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-**:**-**

        // -----------------
        // --- Add with self
        // -----------------
        gp_b1star = gp_b1star + gp_b1star;

        CHECK_EQUAL( 0.004f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.004f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.004f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.004f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.002f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-**:**-**

        // -----------------
        // --- Add b1b1 to b1star - See that we add the b1start with the b1b
        // -----------------

        GeneticProbability gp_b1b1_p_b1star = gp_b1star + gp_b1b1;

        CHECK_EQUAL( 0.254f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-b1:b1-**
        CHECK_EQUAL( 0.104f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.104f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.104f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.102f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-**:**-**

        gp_b1b1_p_b1star = gp_b1b1 + gp_b1star;  // test commutative 

        CHECK_EQUAL( 0.254f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-b1:b1-**
        CHECK_EQUAL( 0.104f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.104f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.104f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.102f, gp_b1b1_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-**:**-**

        // ------------------------------------------------------------------------
        // --- See that we can take this special b1b1 & b1star and another b1star.
        // --- The new b1star should be added to the b1b1 and the b1star
        // ------------------------------------------------------------------------
        GeneticProbability gp_b1b1_p_b1star_p_b1star = gp_b1b1_p_b1star + gp_b1star;

        CHECK_CLOSE( 0.258f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.108f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.108f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.108f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.104f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        gp_b1b1_p_b1star_p_b1star = gp_b1star + gp_b1b1_p_b1star;  // test commutative 

        CHECK_CLOSE( 0.258f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.108f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.108f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.108f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.104f, gp_b1b1_p_b1star_p_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        // ----------------------------------
        // --- See that we can another b1b1
        // ----------------------------------
        GeneticProbability gp_b1b1_p_b1star_p_b1b1 = gp_b1b1_p_b1star + gp_b1b1;

        CHECK_CLOSE( 0.504f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.204f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.204f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.204f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.202f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        gp_b1b1_p_b1star_p_b1b1 = gp_b1b1 + gp_b1b1_p_b1star;  // test commutative 

        CHECK_CLOSE( 0.504f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.204f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.204f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.204f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.202f, gp_b1b1_p_b1star_p_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        // ---------------------------------------------------
        // --- See that we can add a new locus to this b1star
        // ---------------------------------------------------
        GeneticProbability gp_b1b1_p_b1star_p_a2a2 = gp_b1b1_p_b1star + gp_a2a2;

        CHECK_CLOSE( 0.464f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.314f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-**
        CHECK_CLOSE( 0.312f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.264f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.114f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.112f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        gp_b1b1_p_b1star_p_a2a2 = gp_a2a2 + gp_b1b1_p_b1star;  // test commutative 

        CHECK_CLOSE( 0.464f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.314f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-**
        CHECK_CLOSE( 0.312f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.264f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.114f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.112f, gp_b1b1_p_b1star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        GeneticProbability gp_b1b1_p_b1star_p_a2a2_2x = gp_b1b1_p_b1star_p_a2a2 + gp_b1b1_p_b1star_p_a2a2;

        CHECK_CLOSE( 0.928f, gp_b1b1_p_b1star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.628f, gp_b1b1_p_b1star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-**
        CHECK_CLOSE( 0.624f, gp_b1b1_p_b1star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.528f, gp_b1b1_p_b1star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.228f, gp_b1b1_p_b1star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.224f, gp_b1b1_p_b1star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**

        // ---------------------------------------------------------------------------------------------
        // --- Now try a comb that is concerned about the presence of another allele of the same locus
        // ---------------------------------------------------------------------------------------------
        CreateAlleleComboInput( m_GenesGambiae, "b2", "*", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo b2star( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_b2star( b2star, 0.0001f );
        GeneticProbability gp_b2star = acp_b2star;
        CHECK_EQUAL( 0.0001f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ) ); // **-b2:**-**
        CHECK_EQUAL( 0.0001f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-b2:**-**
        CHECK_EQUAL( 0.0001f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ) ); // **-b2:**-**
        CHECK_EQUAL( 0.0001f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ) ); // **-b2:**-**
        CHECK_EQUAL( 0.0000f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-**:**-**

        gp_b2star = gp_b2star + 0.0002f;

        CHECK_CLOSE( 0.0003f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0003f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0003f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0003f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0002f, gp_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-**:**-**

        // -------------------------------------------------------
        // --- Check that adding two "*" at the same locus works,
        // --- creating the proper subsets of similar values
        // -------------------------------------------------------
        GeneticProbability gp_b1star_b2star = gp_b1star + gp_b2star;

        CHECK_CLOSE( 0.0043f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0043f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0023f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0023f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0023f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0042f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0042f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0042f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0022f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b3c1_a1b3c1 ), FLT_EPSILON ); // **-**:**-**

        gp_b1star_b2star = gp_b2star + gp_b1star;  // test commutative 

        CHECK_CLOSE( 0.0043f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0043f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0023f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0023f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0023f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0042f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0042f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0042f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0022f, gp_b1star_b2star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b3c1_a1b3c1 ), FLT_EPSILON ); // **-**:**-**

        // ----------------------------------------------------------------------------
        // --- Try adding a new locus and see that we get combinations of a2's and b's
        // --- plus keep our b1star and b2star
        // ----------------------------------------------------------------------------
        //printf( "=== gp_b1star_b2star ===\n" );
        //printf( "%s\n", gp_b1star_b2star.ToString( m_SpeciesIndexGambiae, m_GenesGambiae ).c_str() );
        //printf( "=== gp_a2a2 ===\n" );
        //printf( "%s\n", gp_a2a2.ToString( m_SpeciesIndexGambiae, m_GenesGambiae ).c_str() );

        GeneticProbability gp_b1star_b2star_p_a2a2 = gp_b1star_b2star + gp_a2a2;

        //printf( "=== gp_b1star_b2star_p_a2a2 ===\n" );
        //printf( "%s\n", gp_b1star_b2star_p_a2a2.ToString( m_SpeciesIndexGambiae, m_GenesGambiae ).c_str() );

        CHECK_CLOSE( 0.2143f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b2:b1-a2
        CHECK_CLOSE( 0.2142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:**-a2
        CHECK_CLOSE( 0.2123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-b2:a2-**
        CHECK_CLOSE( 0.2122f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b3c1_a2b3c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.0143f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0143f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0122f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b3c1_a1b3c1 ), FLT_EPSILON ); // **-**:**-**

        gp_b1star_b2star_p_a2a2 = gp_a2a2 + gp_b1star_b2star;  // test commutative 

        CHECK_CLOSE( 0.2143f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b2:b1-a2
        CHECK_CLOSE( 0.2142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:**-a2
        CHECK_CLOSE( 0.2123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-b2:a2-**
        CHECK_CLOSE( 0.2122f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b3c1_a2b3c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.0143f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0143f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0123f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0142f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0122f, gp_b1star_b2star_p_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b3c1_a1b3c1 ), FLT_EPSILON ); // **-**:**-**

        GeneticProbability gp_b1star_b2star_p_a2a2_2x = gp_b1star_b2star_p_a2a2 + gp_b1star_b2star_p_a2a2;

        CHECK_CLOSE( 0.4286f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b1c1 ), FLT_EPSILON ); // a2-b2:b1-a2
        CHECK_CLOSE( 0.4284f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:**-a2
        CHECK_CLOSE( 0.4246f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-b2:a2-**
        CHECK_CLOSE( 0.4244f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b3c1_a2b3c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.0286f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0286f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON ); // **-b2:b1-**
        CHECK_CLOSE( 0.0246f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0246f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b3c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0246f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b4c1 ), FLT_EPSILON ); // **-b2:**-**
        CHECK_CLOSE( 0.0284f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0284f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0284f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.0244f, gp_b1star_b2star_p_a2a2_2x.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b3c1_a1b3c1 ), FLT_EPSILON ); // **-**:**-**

        // -----------------------------------------------------------
        // --- Now we want to test that we can handle multiple species
        // -----------------------------------------------------------
        CreateAlleleComboInput( m_GenesFunestus, "e2", "e2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo e2e2( m_SpeciesIndexFunestus, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_e2e2( e2e2, 0.2f );

        GeneticProbability gp_e2e2 = acp_e2e2;

        CHECK_EQUAL( 0.0f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d1e1_d1e1 ) );
        CHECK_EQUAL( 0.0f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d2e1_d2e1 ) );
        CHECK_EQUAL( 0.2f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d1e2_d1e2 ) );
        CHECK_EQUAL( 0.2f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d2e2_d2e2 ) );

        CHECK( m_Genome_a1b2c1_a1b2c1 == m_Genome_d1e2_d1e2 ); // genomes are the same but are for different species
        CHECK_EQUAL( 0.0f, gp_e2e2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // default because of species

        // -----------------------------------------------------------------------------
        // --- Test that adding a single value gets added to the default for all species
        // -----------------------------------------------------------------------------
        gp_e2e2 = gp_e2e2 + 0.11f;

        CHECK_CLOSE( 0.11f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d1e1_d1e1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.11f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d2e1_d2e1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.31f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d1e2_d1e2 ), FLT_EPSILON );
        CHECK_CLOSE( 0.31f, gp_e2e2.GetValue( m_SpeciesIndexFunestus, m_Genome_d2e2_d2e2 ), FLT_EPSILON );

        CHECK_CLOSE( 0.11f, gp_e2e2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // default because of species
        CHECK_CLOSE( 0.11f, gp_e2e2.GetValue( m_SpeciesIndexFake,    m_Genome_d2e2_d2e2     ), FLT_EPSILON ); // giving random genome
    }

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilitySubtractScalar )
    {
        GeneticProbability gp1 = 1.0f;
        CHECK_EQUAL( 1.0f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp2 = gp1 - 0.25;
        CHECK_EQUAL( 1.00f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.75f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp3 = 1.0f - gp2;
        CHECK_EQUAL( 1.00f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.75f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.25f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp4 = gp2 - gp3;
        CHECK_EQUAL( 1.00f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.75f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.25f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.50f, gp4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.4f );

        GeneticProbability gpx1 = acp_a2a2;
        gpx1 = gpx1 + 0.5f;
        CHECK_CLOSE( 0.5f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9f, gpx1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        GeneticProbability gpx2 = gpx1 - 0.2f;
        CHECK_CLOSE( 0.3f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.7f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        GeneticProbability gpx3 = 1.0f - gpx1;
        CHECK_CLOSE( 0.5f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
    }

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilitySubtract )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE: I added all of the combinations to this test so that all of the 
        // !!! calls to MathOperations() were called in GeneticMath().  This should
        // !!! ensure we have the right order of values in the subtraction operation.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        GeneticProbability gp1 = 0.9f;
        GeneticProbability gp2 = 0.1f;
        CHECK_EQUAL( 0.9f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.1f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        // --------------
        // --- EXAMPLE #1 - Default - Default
        // --------------
        GeneticProbability gp3 = gp1 - gp2;

        CHECK_CLOSE( 0.8f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );

        // ---------------
        // --- Create a2a2
        // ---------------
        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.2f );

        GeneticProbability gp_a2a2 = acp_a2a2;
        gp_a2a2 = gp_a2a2 + 0.5;

        CHECK_CLOSE( 0.5f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.7f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #2 - a2a2 - Default
        // --------------
        GeneticProbability gpx2 = gp_a2a2 - gp2;

        CHECK_CLOSE( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        GeneticProbability gpx3 = gp3 - gp_a2a2;

        CHECK_CLOSE( 0.3f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #3 - a2a2 + a2a2
        // --------------
        gpx2 = gp_a2a2 - gp_a2a2;

        CHECK_EQUAL( 0.0f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.0f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.0f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.0f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );

        // ------------------------
        // --- Create a2-b1:a2-b1
        // ------------------------
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "b1", "b1", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2b1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2b1( a2b1, 0.2f );
        GeneticProbability gp_a2b1 = acp_a2b1;

        CHECK_CLOSE( 0.0f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gp_a2b1 = gp1 - gp_a2b1;

        CHECK_CLOSE( 0.9f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.7f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9f, gp_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #6 - GP only has a specific combination and not its subsets
        // --------------
        GeneticProbability gp_a2b1_m_a2a2 = gp_a2b1 - gp_a2a2;

        CHECK_CLOSE( 0.4f, gp_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gp_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gp_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gp_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2f, gp_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // -----------------
        // --- Create b1:b1
        // -----------------
        CreateAlleleComboInput( m_GenesGambiae, "b1", "b1", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo b1b1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_b1b1( b1b1, 0.2f );
        GeneticProbability gp_b1b1 = acp_b1b1;
        gp_b1b1 = gp_b1b1 + 0.4f;

        CHECK_CLOSE( 0.6f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gp_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );

        GeneticProbability gpx4 = gp1 - gp_b1b1;

        CHECK_CLOSE( 0.3f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );

        GeneticProbability gpx5 = gp_b1b1 - gp2;

        CHECK_CLOSE( 0.5f, gpx5.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gpx5.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gpx5.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gpx5.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #4 - a2a2 + b1b1 - see that we create a new combination of a2b1_a2b1
        // --------------
        gp_a2a2 = gp_a2a2 + 0.2f;

        GeneticProbability gp_a2b1_a2b1 = gp_a2a2 - gp_b1b1;

        CHECK_CLOSE( 0.1f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gp_a2b1_a2b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #7 - Subtract (a2:a2, b1:b1, a2-b1:a2-b1) with (a2:a2)
        // --------------
        gp_a2a2 = gp_a2a2 - 0.65f;

        GeneticProbability gp_a2b1_a2b1_m_a2a2 = gp_a2b1_a2b1 - gp_a2a2;

        CHECK_CLOSE( 0.05f, gp_a2b1_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); //**-b1-**
        CHECK_CLOSE( 0.05f, gp_a2b1_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); //a2-b1-**
        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); //**-**-** - default
        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1_m_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); //a2-**-**

        // --------------
        // --- EXAMPLE #7 - (again) Add (a2:a2, b1:b1, a2-b1:a2-b1) with (b1:b1)
        // --------------
        AlleleComboProbability acp_b1b1_tmp( b1b1, 0.02f );
        GeneticProbability gp_b1b1_tmp = acp_b1b1_tmp;
        gp_b1b1 = gp_b1b1_tmp + 0.03f;

        GeneticProbability gp_a2b1_a2b1_m_b1b1 = gp_a2b1_a2b1 - gp_b1b1;

        CHECK_CLOSE( 0.05f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); //**-b1-**
        CHECK_CLOSE( 0.25f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); //a2-b1-**
        CHECK_CLOSE( 0.27f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); //**-**-** - default
        CHECK_CLOSE( 0.47f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); //a2-**-**

        // ----------------------------------------------------------------------
        // --- Add b1b1 back and see that we return the same values gp_a2b1_a2b1
        // ----------------------------------------------------------------------
        gp_a2b1_a2b1_m_b1b1 = gp_a2b1_a2b1_m_b1b1 + gp_b1b1;

        CHECK_CLOSE( 0.1f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5f, gp_a2b1_a2b1_m_b1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #8 - Subtract (a2:a2, b1:b1, a2-b1:a2-b1) with (a2:a2, b1:b1, a2-b1:a2-b1)
        // --------------
        GeneticProbability gp_a2b1_a2b1_zero = gp_a2b1_a2b1 - gp_a2b1_a2b1;

        CHECK_CLOSE( 0.0f, gp_a2b1_a2b1_zero.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1_a2b1_zero.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1_a2b1_zero.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0f, gp_a2b1_a2b1_zero.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // ------------------
        // --- Create a1:a1
        // ------------------
        CreateAlleleComboInput( m_GenesGambiae, "a1", "a1", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a1a1( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a1a1( a1a1, 0.001f );
        GeneticProbability gp_a1a1 = acp_a1a1;

        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.000f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.000f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        gp_a1a1 = gp_a1a1 + 0.001f;

        CHECK_EQUAL( 0.002f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ) );
        CHECK_EQUAL( 0.002f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) );
        CHECK_EQUAL( 0.001f, gp_a1a1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ) );

        // see that we create a new combination of a1b1_a1b1
        GeneticProbability gp_a1b1_a1b1 = gp_a1a1 + gp_b1b1;

        CHECK_CLOSE( 0.052f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.051f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.032f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.031f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.051f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.031f, gp_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #10 - stack of a2-b1:a2-b1 with stack of a1-b1:a1-b1 (B's are same but A's are different)
        // --------------
        GeneticProbability gp_a2b1_a2b1_m_a1b1_a1b1 = gp_a2b1_a2b1 - gp_a1b1_a1b1;

        CHECK_CLOSE( 0.048f, gp_a2b1_a2b1_m_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // a1-b1:a1-b1
        CHECK_CLOSE( 0.249f, gp_a2b1_a2b1_m_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON ); // a2-b1:a2-b1
        CHECK_CLOSE( 0.268f, gp_a2b1_a2b1_m_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // a1-**:a1-**
        CHECK_CLOSE( 0.469f, gp_a2b1_a2b1_m_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON ); // a2-**:a2-**
        CHECK_CLOSE( 0.049f, gp_a2b1_a2b1_m_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a2b1c1 ), FLT_EPSILON ); // **-b1:**-b1
        CHECK_CLOSE( 0.269f, gp_a2b1_a2b1_m_a1b1_a1b1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a2b1c1 ), FLT_EPSILON ); // **-**:**-** - default

        // ------------------------------------------------------------------------------
        // --- Now try a combo that is concerned about just the presence of one allele.
        // ------------------------------------------------------------------------------
        CreateAlleleComboInput( m_GenesGambiae, "b1", "*", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo b1star( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_b1start( b1star, 0.2f );
        GeneticProbability gp_b1star = acp_b1start;
        gp_b1star = gp_b1star + 0.2f;

        CHECK_EQUAL( 0.4f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.4f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.4f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.4f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ) ); // **-b1:**-**
        CHECK_EQUAL( 0.2f, gp_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ) ); // **-**:**-**

        // -----------------
        // --- Add b1b1 to b1star - See that we handle the b1star with the b1b
        // -----------------
        GeneticProbability gp_b1b1_m_b1star = gp_b1star - gp_b1b1;

        CHECK_CLOSE( 0.35f, gp_b1b1_m_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON ); // **-b1:b1-**
        CHECK_CLOSE( 0.37f, gp_b1b1_m_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b2c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.37f, gp_b1b1_m_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b3c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.37f, gp_b1b1_m_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b4c1 ), FLT_EPSILON ); // **-b1:**-**
        CHECK_CLOSE( 0.17f, gp_b1b1_m_b1star.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON ); // **-**:**-**
    }
#endif

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilityMultiplyScalar )
    {
        GeneticProbability gp1 = 0.25;
        CHECK_EQUAL( 0.25f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp2 = gp1 * 0.25;
        CHECK_EQUAL( 0.0625f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp3 = 0.25 * gp1;
        CHECK_EQUAL( 0.0625f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp4 = 0.25 * gp1 * 0.25;
        CHECK_CLOSE( 0.015625f, gp4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );

        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.4f );

        GeneticProbability gpx1 = acp_a2a2;
        GeneticProbability gpx2 = gpx1 + 0.2f;

        CHECK_CLOSE( 0.2f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        GeneticProbability gpx3 = 0.2f * gpx2;
        CHECK_CLOSE( 0.040f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.120f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        gpx3 = gpx2 * 0.2f;
        CHECK_CLOSE( 0.040f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.120f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        GeneticProbability gpx4 = 0.2f * gpx2 * 0.3f;
        CHECK_CLOSE( 0.012f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.036f, gpx4.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
    }

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilityMultiplication )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE:  Since addition and multiplication are communitve, we don't need to worry about
        // !!! testing all of the MathOperation calls in GeneticMath().  Seeing that some of the work
        // !!! should be good enough.  The addition tests should test everything else.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        GeneticProbability gp1 = 0.1f;
        GeneticProbability gp2 = 0.2f;
        CHECK_EQUAL( 0.1f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );
        CHECK_EQUAL( 0.2f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        // --------------
        // --- EXAMPLE #1 - Default + Default
        // --------------
        GeneticProbability gp3 = gp1 * gp2;

        CHECK_CLOSE( 0.02f, gp3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );

        // ---------------
        // --- Create a2a2
        // ---------------
        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.2f );

        GeneticProbability gp_a2a2 = acp_a2a2;
        gp_a2a2 = gp_a2a2 + 0.1f;

        CHECK_CLOSE( 0.1f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.3f, gp_a2a2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #2 - a2a2 * Default
        // --------------
        GeneticProbability gpx2 = gp_a2a2 * gp1;

        CHECK_CLOSE( 0.01f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.03f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.01f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.03f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gpx2 = gp1 * gp_a2a2;  // test commutative 

        CHECK_CLOSE( 0.01f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.03f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.01f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.03f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gpx2 = gp1 * gp_a2a2 * gp1;

        CHECK_CLOSE( 0.001f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.003f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.001f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.003f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        // --------------
        // --- EXAMPLE #3 - a2a2 * a2a2
        // --------------
        gpx2 = gp_a2a2 * gp_a2a2;

        CHECK_CLOSE( 0.01f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.09f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.01f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.09f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );

        gpx2 = gp_a2a2 * gp1 * gp_a2a2;

        CHECK_CLOSE( 0.001f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.009f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.001f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b2c1_a1b2c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.009f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b2c1_a2b2c1 ), FLT_EPSILON );
    }

    TEST_FIXTURE( GeneticFixture, TestGeneticProbabilityScalarDivision )
    {
        GeneticProbability gp1 = 0.2f;
        CHECK_EQUAL( 0.2f, gp1.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp2 = gp1 / 0.2f;
        CHECK_EQUAL( 1.0f, gp2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ) );

        GeneticProbability gp4 = gp1 / 0.4f;
        CHECK_CLOSE( 0.5f, gp4.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );

        VectorGameteBitPair_t genome_bit_mask;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        CreateAlleleComboInput( m_GenesGambiae, "a2", "a2", "", "", &genome_bit_mask, &possible_genomes );

        AlleleCombo a2a2( m_SpeciesIndexGambiae, genome_bit_mask, possible_genomes );

        AlleleComboProbability acp_a2a2( a2a2, 0.2f );

        GeneticProbability gpx1 = acp_a2a2;
        GeneticProbability gpx2 = gpx1 + 0.2f;

        CHECK_CLOSE( 0.2f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        GeneticProbability gpx3 = gpx2 / 0.8f;
        CHECK_CLOSE( 0.25f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.50f, gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        gpx3 = gpx2 * 0.8f;
        CHECK_CLOSE( 0.2f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a1b1c1_a1b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4f, gpx2.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );

        // -----------------------
        // --- Test serialization
        // -----------------------
        JsonFullWriter json_writer;
        IArchive* p_json_writer = &json_writer;
        p_json_writer->labelElement( "Test" )& gpx3;
        //PrintDebug( p_json_writer->GetBuffer() );

        GeneticProbability gp_read;
        JsonFullReader json_reader( p_json_writer->GetBuffer() );
        IArchive* p_json_reader = &json_reader;
        p_json_reader->labelElement( "Test" )& gp_read;

        CHECK_CLOSE( gpx3.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), gp_read.GetValue( m_SpeciesIndexGambiae, m_Genome_a2b1c1_a2b1c1 ), FLT_EPSILON );
        CHECK_CLOSE( gpx3.GetDefaultValue(), gp_read.GetDefaultValue(), FLT_EPSILON);

        // only changing default value to make sure everything else matches because otherwise fails due to floating point
        gpx3.SetDefaultValue( 0 );
        gp_read.SetDefaultValue( 0 );
        CHECK( gpx3 == gp_read );

    }
}
