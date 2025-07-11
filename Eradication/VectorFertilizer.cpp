
#include "stdafx.h"
#include "VectorFertilizer.h"
#include "VectorGene.h"
#include "VectorGeneDriver.h"
#include "VectorTraitModifiers.h"
#include "VectorMaternalDeposition.h"
#include "RANDOM.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"

SETUP_LOGGING( "VectorFertilizer" )

namespace Kernel
{
    VectorFertilizer::VectorFertilizer()
        : m_pGenes( nullptr )
        , m_pTraitModifiers( nullptr )
        , m_pGeneDrivers( nullptr )
        , m_pMaternalDepositions( nullptr )
    {
    }

    VectorFertilizer::~VectorFertilizer()
    {
    }

    void VectorFertilizer::Initialize( const VectorGeneCollection* pGenes,
                                       const VectorTraitModifiers* pTraitModifiers,
                                       const VectorGeneDriverCollection* pGeneDrivers,
                                       const VectorMaternalDepositionCollection* pMaternalDepositions )
    {
        m_pGenes = pGenes;
        m_pTraitModifiers = pTraitModifiers;
        m_pGeneDrivers = pGeneDrivers;
        m_pMaternalDepositions = pMaternalDepositions;
    }

    InitialGenomeData VectorFertilizer::DetermineInitialGenomeData( RANDOMBASE* pRNG, uint32_t total )
    {
        // ---------------------------------------------------------------
        // --- Determine the initial set of genomes and the number of each
        // --- This is based on the user defined initial frequencies.
        // ---------------------------------------------------------------
        GenomeCountPairVector_t initial_genomes = DetermineInitialGenomes( pRNG, total );

        // ----------------------------------------------------------
        // --- Break the initial genomes into sets of male and female
        // --- and count the total number of males
        // ----------------------------------------------------------
        InitialGenomeData initial_data;
        uint32_t total_males = 0;
        GenomeCountPairVector_t genomes_female;
        for( auto& r_genome_num : initial_genomes )
        {
            if( r_genome_num.genome.GetGender() == VectorGender::VECTOR_FEMALE )
            {
                genomes_female.push_back( r_genome_num );
            }
            else
            {
                initial_data.males.push_back( r_genome_num );
                total_males += r_genome_num.count;
            }
        }

        // -----------------------------------------------------------------------------------
        // --- We want to ensure that each male geneom can mate with each female genome.
        // --- Hence, each male genome is a percentage of the overall number of males.
        // --- These males get to mate with that percentage of females from each female genome.
        // -----------------------------------------------------------------------------------
        for( auto& r_male : initial_data.males )
        {
            float percentage_of_males = float( r_male.count ) / float( total_males );
            for( auto& r_female : genomes_female )
            {
                MatedGenomeCount mated;
                mated.male = r_male.genome;
                mated.female = r_female.genome;

                // -------------------------------------------------------------------
                // --- Since there can be situations where there is a low probability
                // --- of a particular combination, we use randomRound() so that some
                // --- runs will get some of that combination.
                // -------------------------------------------------------------------
                float count_f = percentage_of_males * float( r_female.count );
                mated.count = pRNG->randomRound( count_f );

                if( mated.count > 0 )
                {
                    initial_data.mated_females.push_back( mated );
                }
            }
        }

        return initial_data;
    }

    GenomeCountPairVector_t VectorFertilizer::DetermineInitialGenomes( RANDOMBASE* pRNG, uint32_t total )
    {
        // ------------------------------------------------
        // --- Create possible gametes for male and female
        // --- based on the initial frequencies.
        // ------------------------------------------------
        GameteProbPairVector_t gametes_female = CreateInitialGametes( true, true );
        GameteProbPairVector_t gametes_male   = CreateInitialGametes( false, true );

        GenomeProbPairVector_t possibilities = CreatePossibleGenomes( gametes_female, gametes_male );

        GenomeCountPairVector_t initial_genomes= DetermineNumberOfGenomes( pRNG, possibilities, total );
        return initial_genomes;
    }

    GenomeCountPairVector_t VectorFertilizer::DetermineFertilizedEggs( RANDOMBASE* pRNG,
                                                                       const VectorGenome& rFemale,
                                                                       const VectorGenome& rMale,
                                                                       uint32_t totalEggs )
    {
        release_assert( pRNG != nullptr );
        release_assert( m_pGenes != nullptr );
        release_assert( m_pTraitModifiers != nullptr );

        // -------------------------------------------------------------
        // --- Execute drivers that work from one chromosome to another.
        // --- When a vector gets a driver, it does not drive until the
        // --- vector is generating offspring.
        // -------------------------------------------------------------
        GenomeProbPairVector_t female_gppv = DriveGenes( rFemale );
        GenomeProbPairVector_t male_gppv   = DriveGenes( rMale );

        // ---------------------------------------------------------
        // --- Get the modifier that indicates if the male produces
        // --- more gametes with the X "gene" than the Y "gene
        // ---------------------------------------------------------
        float is_female_modifier = m_pTraitModifiers->GetModifier( VectorTrait::FEMALE_EGG_RATIO, rMale );

        // --------------------------------------------------------------
        // --- Generate the gametes (i.e. eggs and sperm) for the parents
        // --------------------------------------------------------------
        GameteProbPairVector_t gametes_female = CreateGametes( 1.0, female_gppv );
        GameteProbPairVector_t gametes_male   = CreateGametes( is_female_modifier, male_gppv );

        // -------------------------------------------------------------------------
        // --- Perform any germline mutations in the gametes (i.e. an allele mutates
        // --- into a different allele during the creation of the gametes.)
        // -------------------------------------------------------------------------
        GermlineMutation( gametes_female );
        GermlineMutation( gametes_male   );

        // -------------------------------------------------------------------------
        // --- Pre-calculating allele changes due to maternal deposition of Cas9
        // -------------------------------------------------------------------------

        DoMaternalDepostion( rFemale, gametes_female );
        DoMaternalDepostion( rFemale, gametes_male );

        // -------------------------------------------------------------------------
        // --- Now that we have the gametes from each parent and the probability of
        // --- that gamete generate the possible genomes (i.e. fertilized eggs).
        // -------------------------------------------------------------------------
        GenomeProbPairVector_t possibilities = CreatePossibleGenomes( gametes_female, gametes_male );

        AdjustForNonFertileEggs( possibilities );

        GenomeCountPairVector_t fertilized_eggs = DetermineNumberOfGenomes( pRNG, possibilities, totalEggs );
        return fertilized_eggs;
    }

    GenomeNamePairVector_t VectorFertilizer::CreateAllPossibleGenomesWithNames() const
    {
        GameteProbPairVector_t gametes_female = CreateInitialGametes( true, false );
        GameteProbPairVector_t gametes_male   = CreateInitialGametes( false, false );

        GenomeProbPairVector_t possibilities = CreatePossibleGenomes( gametes_female, gametes_male );

        GenomeNamePairVector_t genome_name_pair_vector;
        for( auto& r_gp : possibilities )
        { 
            GenomeNamePair gnp;
            gnp.genome = r_gp.genome;
            gnp.name   = m_pGenes->GetGenomeName( r_gp.genome );
            genome_name_pair_vector.push_back( gnp );
        }
        return genome_name_pair_vector;
    }

    GenomeProbPairVector_t VectorFertilizer::CreatePossibleGenomes( const GameteProbPairVector_t& rGametesFemale,
                                                                    const GameteProbPairVector_t& rGametesMale ) const
    {
        GenomeProbPairVector_t possibilities;
        possibilities.reserve( rGametesFemale.size() * rGametesMale.size() );
        for( auto female_entry : rGametesFemale )
        {
            for( auto male_entry : rGametesMale )
            {
                GenomeProbPair genome_prob;
                genome_prob.genome = VectorGenome( female_entry.gamete, male_entry.gamete );
                genome_prob.prob = female_entry.prob * male_entry.prob;
                if( genome_prob.prob > 0.0 )
                {
                    possibilities.push_back( genome_prob );
                }
            }
        }
        return possibilities;
    }

    GenomeCountPairVector_t VectorFertilizer::DetermineNumberOfGenomes( RANDOMBASE* pRNG,
                                                                        const GenomeProbPairVector_t& rPossibilities,
                                                                        uint32_t total )
    {
        // -------------------------------------------------------------
        // --- With a probability assigned to each genome, deternine
        // --- the number of eggs for each genome using the probability
        // --- assigned to that genome.  We use randomRound() so that on
        // --- average we get the correct numbers, especially when the
        // --- probability is less than 1. (randomRound(v) - if v=2.3,
        // --- then 70% it returns 2, and 30% it returns 3.)
        // -------------------------------------------------------------
        GenomeCountPairVector_t fertilized_eggs;
        fertilized_eggs.reserve( rPossibilities.size() );
        for( uint32_t i = 0; i < rPossibilities.size(); ++i )
        {
            float prob = rPossibilities[ i ].prob;
            float float_num_eggs = float( total ) * prob;
            uint32_t num_eggs = pRNG->randomRound( float_num_eggs );
            if( num_eggs > 0 )
            {
                fertilized_eggs.push_back( GenomeCountPair( rPossibilities[ i ].genome, num_eggs ) );
            }
        }

        return fertilized_eggs;
    }

    void CreatePossibleGenderAlleles( const VectorGene& rGenderGene,
                                      bool femaleOnly,
                                      bool useFrequencies,
                                      GameteProbPairVector_t& rPossibleGametes )
    {
        release_assert( rGenderGene.GetLocusIndex() == VectorGenome::GENDER_LOCUS_INDEX );

        // --------------------------------------------------------------------------------------
        // --- The gender alleles are different than the others due to the fact that a male
        // --- must have one allele that is a Y-chromosome type.  For example, in a non-gender
        // --- gene with two alleles, one would specify frequencies of a1=0.5 & a2=0.5 to get
        // --- 50% of the alleles to be a1 and 50% a2.  This would make you want to do the same
        // --- for X & Y (i.e. 50% Y imples 50% males), however, to get 50% males, you really want
        // --- X=0.75 and Y=0.25 because in a mating there are 3-X's and 1-Y out of the four allele.
        // --- NOTE: X=1.0 and Y=0.0 implies all females while X=0.5 and Y=0.5 implies all males.
        // --------------------------------------------------------------------------------------

        float female_total_freq = 0.0;
        float male_total_freq = 0.0;
        for( uint8_t allele_index = 0; allele_index < rGenderGene.GetNumAllele(); ++allele_index )
        {
            const VectorAllele* p_allele = rGenderGene.GetAllele( allele_index );
            if( p_allele != nullptr )
            {
                if( p_allele->IsFemale() )
                {
                    female_total_freq += p_allele->GetFrequency();
                }
                else
                {
                    male_total_freq += p_allele->GetFrequency();
                }
            }
        }
        float delta_freq = female_total_freq - male_total_freq;

        for( uint8_t allele_index = 0; allele_index < rGenderGene.GetNumAllele(); ++allele_index )
        {
            const VectorAllele* p_allele = rGenderGene.GetAllele( allele_index );
            if( (p_allele == nullptr) || (femaleOnly && p_allele->IsMale()) ) continue;

            GameteProbPair tmp_gamete_prob;
            tmp_gamete_prob.gamete.SetLocus( VectorGenome::GENDER_LOCUS_INDEX, p_allele->GetIndex() );
            tmp_gamete_prob.prob = 1.0;
            if( useFrequencies )
            {
                if( femaleOnly )
                {
                    // ------------------------------------------------------------------------
                    // --- The probability that a female contributes this allele to its gamete
                    // ------------------------------------------------------------------------
                    tmp_gamete_prob.prob = p_allele->GetFrequency() / female_total_freq;
                }
                else
                {
                    if( p_allele->IsFemale() )
                    {
                        // -------------------------------------------------------------------------
                        // --- The probability that the male vector contributes this 'X' chromosome
                        // -------------------------------------------------------------------------
                        tmp_gamete_prob.prob = delta_freq * p_allele->GetFrequency() / female_total_freq;
                    }
                    else
                    {
                        // -------------------------------------------------------------------------
                        // --- The probability that the male vector contributes this 'Y' chromosome
                        // -------------------------------------------------------------------------
                        tmp_gamete_prob.prob = 2.0 * p_allele->GetFrequency();
                    }
                }
            }
            rPossibleGametes.push_back( tmp_gamete_prob );
        }
    }

    GameteProbPairVector_t VectorFertilizer::CreateInitialGametes( bool femaleOnly, bool useFrequencies ) const
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Assume the gender "gene" is at locus / gene index zero
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        release_assert( VectorGenome::GENDER_LOCUS_INDEX == 0 );
        const VectorGene& r_gender_gene = *(*m_pGenes)[ VectorGenome::GENDER_LOCUS_INDEX ];

        GameteProbPairVector_t possible_gametes;
        CreatePossibleGenderAlleles( r_gender_gene, femaleOnly, useFrequencies, possible_gametes );
        
        for( uint8_t gene_index = 1; gene_index < m_pGenes->Size(); ++gene_index )
        {
            GameteProbPairVector_t tmp_possible_gametes = possible_gametes;
            possible_gametes.clear();

            const VectorGene& r_gene = *(*m_pGenes)[ gene_index ];
            uint8_t locus_index = r_gene.GetLocusIndex();

            for( uint8_t allele_index = 0; allele_index < r_gene.GetNumAllele(); ++allele_index )
            {
                const VectorAllele* p_allele = r_gene.GetAllele( allele_index );
                if( (p_allele != nullptr) && ((p_allele->GetFrequency() > 0.0) || !useFrequencies) )
                {
                    for( auto gamete_prob : tmp_possible_gametes )
                    {
                        gamete_prob.gamete.SetLocus( locus_index, p_allele->GetIndex() );
                        if( useFrequencies )
                        {
                            gamete_prob.prob *= p_allele->GetFrequency();
                        }
                        possible_gametes.push_back( gamete_prob );
                    }
                }
            }
        }
        return possible_gametes;
    }

    GameteProbPairVector_t VectorFertilizer::CreateGametes( float isFemaleModifier,
                                                            const GenomeProbPairVector_t& rGPPV )
    {
        GameteProbPairVector_t gam_ppv;
        for( auto gpp : rGPPV )
        {
            GameteProbPairVector_t tmp_gam_ppv = CreateGametes( isFemaleModifier, gpp.genome );
            for( auto gam_pp : tmp_gam_ppv )
            {
                gam_pp.prob *= gpp.prob;
                gam_ppv.push_back( gam_pp );
            }
        }
        return gam_ppv;
    }


    GameteProbPairVector_t VectorFertilizer::CreateGametes( float isFemaleModifier,
                                                            const VectorGenome& rGenome )
    {
        GameteProbPairVector_t gamete_prob_list;
        GameteProbPair initial_gamete_prob;

        initial_gamete_prob.prob = 1.0;

        if( rGenome.GetGender() == VectorGender::VECTOR_FEMALE )
        {
            // Wolbachia retains female type
            initial_gamete_prob.gamete.SetWolbachia( rGenome.GetWolbachia() );
        }

        gamete_prob_list.push_back( initial_gamete_prob );

        // -------------------------------------------------------------------
        // --- This assumes an even 50/50 probability that a gamete will have
        // --- one of the alleles of the parent.  Hence, if there are X genes,
        // --- then there should be 2^X combinantions/gametes.
        // -------------------------------------------------------------------
        GameteProbPairVector_t tmp_gamete_prob_list;

        for( int locus_index = 0; locus_index < m_pGenes->Size(); ++locus_index )
        {
            tmp_gamete_prob_list = gamete_prob_list;
            gamete_prob_list.clear();
            std::pair<uint8_t,uint8_t> allele_pair = rGenome.GetLocus( locus_index );
            for( auto gamete_prob : tmp_gamete_prob_list )
            {
                float ratio_a = 0.5;
                float ratio_b = 0.5;

                // ------------------------------------------------------------------------
                // --- If modifying the ratio of female to male eggs and we are looking at
                // --- an allele pair that has a Y gene/chromosome, then adjust the ratio.
                // --- 2=> all females, 1=> even, 0=> all males.
                // ------------------------------------------------------------------------
                if( (isFemaleModifier != 1.0) && (locus_index == 0) &&
                    (rGenome.GetGender() == VectorGender::VECTOR_MALE) )
                {
                    ratio_a = ratio_a*isFemaleModifier;
                    if( ratio_a > 1.0 )
                    {
                        ratio_a = 1.0;
                    }
                    ratio_b = 1.0 - ratio_a;
                }

                GameteProbPair vgp_a = gamete_prob;
                GameteProbPair vgp_b = gamete_prob;

                // homozygous
                if( allele_pair.first == allele_pair.second )
                {
                    vgp_a.gamete.SetLocus( locus_index, allele_pair.first );
                    vgp_a.prob *= (ratio_a + ratio_b);
                    vgp_b.prob = 0.0; // keeps 'b' from being added to list
                }
                else // heterozygous
                {
                    vgp_a.gamete.SetLocus( locus_index, allele_pair.first );
                    vgp_b.gamete.SetLocus( locus_index, allele_pair.second );
                    vgp_a.prob *= ratio_a;
                    vgp_b.prob *= ratio_b;
                }

                if( vgp_a.prob > 0.0 )
                {
                    gamete_prob_list.push_back( vgp_a );
                }
                if( vgp_b.prob > 0.0 )
                {
                    gamete_prob_list.push_back( vgp_b );
                }
            }
        }

        return gamete_prob_list;
    }

    void VectorFertilizer::GermlineMutation( GameteProbPairVector_t& rGametes )
    {
        for( uint8_t gene_index = 0; gene_index < m_pGenes->Size(); ++gene_index )
        {
            const VectorGene& r_gene = *(*m_pGenes)[ gene_index ];
            uint8_t locus_index = r_gene.GetLocusIndex();

            // so we can add to end not visit the added ones
            int num_gametes = rGametes.size();

            for( int gamete_index = 0 ; gamete_index < num_gametes ; ++gamete_index )
            {
                // -------------------------------------------------------------------
                // --- If an allele has mutations, add new gametes with the mutations
                // -------------------------------------------------------------------
                uint8_t old_allele_index = rGametes[ gamete_index ].gamete.GetLocus( locus_index );
                const VectorAllele* p_allele = r_gene.GetAllele( old_allele_index );
                if( p_allele == nullptr ) continue;

                float total_mutation_frequency = 0.0;
                for( const VectorAlleleMutation* p_mutation_data : p_allele->GetMutations() )
                {
                    GameteProbPair mutation = rGametes[ gamete_index ];
                    mutation.gamete.SetLocus( locus_index, p_mutation_data->GetAlleleIndexTo() );
                    mutation.prob *= p_mutation_data->GetFrequency();
                    rGametes.push_back( mutation );
                    total_mutation_frequency += p_mutation_data->GetFrequency();
                }

                // --------------------------------------------------------------------------
                // --- If the allele did have mutations, then we adjust the frequency
                // --- of the non-mutating possibility so that it is less likely than before.
                // --------------------------------------------------------------------------
                rGametes[ gamete_index ].prob *= (1.0 - total_mutation_frequency);
            }
        }
    }

    GenomeProbPairVector_t VectorFertilizer::DriveGenes( const VectorGenome& rGenome ) const
    {
        GenomeProbPairVector_t gppv;
        if( (m_pGeneDrivers == nullptr) || (m_pGeneDrivers->Size() == 0) )
        {
            gppv.push_back( GenomeProbPair( rGenome, 1.0 ) );
        }
        else
        {
            gppv = m_pGeneDrivers->DriveGenes( rGenome );
        }
        return gppv;
    }

    void VectorFertilizer::DoMaternalDepostion( const VectorGenome& rMomGenome, GameteProbPairVector_t& rGametes ) const
    {
        if( ( m_pGeneDrivers == nullptr ) || ( m_pGeneDrivers->Size() == 0 ) ||
            ( m_pMaternalDepositions == nullptr) || ( m_pMaternalDepositions->Size() == 0) )
        {
            return;
        }
        else
        {
            m_pMaternalDepositions->DoMaternalDeposition( rMomGenome, rGametes );
            return;
        }

    }



    void VectorFertilizer::AdjustForNonFertileEggs( GenomeProbPairVector_t& rPossibilities ) const
    {
        for( auto& r_gpp : rPossibilities )
        {
            float modifier = m_pTraitModifiers->GetModifier( VectorTrait::ADJUST_FERTILE_EGGS, r_gpp.genome );
            r_gpp.prob *= modifier;
        }
    }
}
