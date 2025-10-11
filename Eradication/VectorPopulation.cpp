
#include "stdafx.h"
#include "VectorPopulation.h"

#include "Debug.h"
#include "Exceptions.h"
#include "Log.h"
#include "INodeContext.h"
#include "ISimulationContext.h"
#include "Climate.h"
#include "SimulationConfig.h"
#include "TransmissionGroupMembership.h"
#include "Vector.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "VectorCohort.h"
#include "StrainIdentity.h"
#include "IMigrationInfoVector.h"
#include "RANDOM.h"
#include <numeric>
#include "IdmDateTime.h"

SETUP_LOGGING( "VectorPopulation" )

namespace Kernel
{
#ifndef WIN32
    #define min(a,b) ( (a<b) ? a : b )
#endif
    // QI stuff
    BEGIN_QUERY_INTERFACE_BODY(VectorPopulation)
        HANDLE_INTERFACE(IVectorPopulation)
        HANDLE_INTERFACE(IInfectable)
        HANDLE_INTERFACE(IVectorPopulationReporting)
        HANDLE_ISUPPORTS_VIA(IVectorPopulation)
    END_QUERY_INTERFACE_BODY(VectorPopulation)

    // ------------------------------------------------------------------------------
    // --- Table for storing mortailityFromAge() calculations for each possible age.
    // --- Age should be a factor of dt and we are assuming that dt is an integer day
    // ------------------------------------------------------------------------------
    std::vector<float> VectorPopulation::m_MortalityTable;

    VectorPopulation::VectorPopulation()
        : m_larval_habitats( nullptr )
        , genome_counts()
        , state_counts()
        , wolbachia_counts()
        , microsporidia_counts()
        , m_AgeAtDeathByState()
        , m_AgeAtDeathByGenomeByState()
        , m_ProgressedLarvaeToImmatureCount( 0 )
        , m_ProgressedLarvaeToImmatureSumDur( 0.0 )
        , num_gestating_queue()
        , num_gestating_begin( 0 )
        , num_gestating_end( 0 )
        , num_looking_to_feed( 0 )
        , num_fed_counter( 0 )
        , num_attempt_feed_indoor( 0 )
        , num_attempt_feed_outdoor( 0 )
        , num_attempt_but_not_feed( 0 )
        , neweggs(0)
        , new_adults(0)
        , unmated_adults(0)
        , dead_mosquitoes_before(0)
        , dead_mosquitoes_indoor(0)
        , dead_mosquitoes_outdoor(0)
        , num_infs_per_state_counts()
        , dryheatmortality(0.0f)
        , infectiouscorrection(0.0f)
        , infected_progress_this_timestep(0.0f)
        , indoorinfectiousbites(0.0f)
        , outdoorinfectiousbites(0.0f)
        , indoorbites(0.0f)
        , outdoorbites(0.0f)
        , m_EIR_by_pool()
        , m_HBR_by_pool()
        , gender_mating_eggs()
        , m_EggCohortVectorOfMaps()
        , m_ContagionToDepositIndoor()
        , m_ContagionToDepositOutdoor()
        , EggQueues()
        , LarvaQueues()
        , ImmatureQueues()
        , pAdultQueues( new VectorCohortCollectionStdMapWithAging() )
        , InfectedQueues()
        , InfectiousQueues()
        , MaleQueues()
        , m_ReleasedInfectious()
        , m_ReleasedAdults()
        , m_ReleasedMales()
        , m_context(nullptr)
        , m_pNodeVector(nullptr)
        , m_vector_params(nullptr)
        , m_species_params(nullptr)
        , m_probabilities(nullptr)
        , m_VectorMortality(true)
        , m_LocalMatureMortalityProbabilityTable()
        , m_DefaultLocalMatureMortalityProbability()
        , m_Fertilizer()
        , m_SpeciesIndex(-1)
        , m_UnmatedMaleTotal(0)
        , m_MaleMatingCDF()
        , m_IsSortingVectors(false)
        , m_NeedToRefreshTheMatingCDF(false)
        , m_ImmigratingInfectious()
        , m_ImmigratingInfected()
        , m_ImmigratingAdult()
        , m_ImmigratingMale()
        , m_pMigrationInfoVector(nullptr)
    {
        if( m_MortalityTable.size() == 0 )
        {
            for( uint32_t i = 0; i < VectorCohort::I_MAX_AGE; ++i )
            {
                float t_age = float( i );
                float mortality = mortalityFromAge( t_age );
                m_MortalityTable.push_back( mortality );
            }
        }
        // Initialize table for each gender
        int num_genders = 2;
        for( int i = 0 ; i < num_genders; ++i )
        {
            m_LocalMatureMortalityProbabilityTable.push_back( std::vector<std::vector<std::vector<float>>>() );
            m_LocalMatureMortalityProbabilityTable[i].push_back( std::vector<std::vector<float>>() ); // WOLBACHIA FREE
            m_LocalMatureMortalityProbabilityTable[i].push_back( std::vector<std::vector<float>>() ); // has wolbachia
            // allocate space in UpdateLocalMatureMortalityProbability() after m_species_params is set
            m_DefaultLocalMatureMortalityProbability.push_back( 1.0 );
        }

        release_assert( m_DefaultLocalMatureMortalityProbability.size() == num_genders );
        release_assert( m_LocalMatureMortalityProbabilityTable.size() == num_genders );
        release_assert( m_LocalMatureMortalityProbabilityTable[ 0 ].size() == 2 ); //for female - 2 wolbachia entries - free and has
        release_assert( m_LocalMatureMortalityProbabilityTable[ 1 ].size() == 2 ); //for mmale - 2 wolbachia entries - free and has

        genome_counts.resize( VectorStateEnum::pairs::count(), GenomeCountMap_t() );
        state_counts.resize( VectorStateEnum::pairs::count() , 0 );
        wolbachia_counts.resize( VectorWolbachia::pairs::count(), 0 );

        m_AgeAtDeathByState.resize( VectorStateEnum::pairs::count(), AgeAtDeath() );
        m_AgeAtDeathByGenomeByState.resize( VectorStateEnum::pairs::count(), AgeAtDeathGenomeMap_t() );

        m_EggCohortVectorOfMaps.resize( VectorHabitatType::pairs::count(), EggCohortMap_t() );

        num_infs_per_state_counts.resize( VectorStateEnum::pairs::count(), 0 );
    }

    void VectorPopulation::Initialize( INodeContext *context,
                                       int speciesIndex,
                                       uint32_t adults )
    {
        m_SpeciesIndex = speciesIndex;

        SetContextTo(context);

        // Correct diepostfeeding and successfulfeed for infectious, which have infectioushfmortmod
        // adjusted for increased feeding mortality due to longer probing
        // Wekesa, J. W., R. S. Copeland, et al. (1992). "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya." Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000). "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l." Parasitology 120(04): 329-333.
        infectiouscorrection = 0;
        if (params()->human_feeding_mortality * species()->infectioushfmortmod < 1.0)
        {
            infectiouscorrection = float((1.0 - params()->human_feeding_mortality * species()->infectioushfmortmod) / (1.0 - params()->human_feeding_mortality));
        }

        // Set up initial populations of adult/infectious/male mosquitoes
        InitializeVectorQueues( adults );
    }

    void VectorPopulation::InitializeVectorQueues( uint32_t numFemales )
    {
        if( numFemales > 0 )
        {
            // -----------------------------------------------------------------------
            // --- We multiply by two because the input value is the number of females
            // --- and we want essentially the same number of males as females.
            // -----------------------------------------------------------------------
            InitialGenomeData initial_genome_data = m_Fertilizer.DetermineInitialGenomeData( m_context->GetRng(),
                                                                                             2 * numFemales );

            for( auto& r_mated : initial_genome_data.mated_females )
            {
                if( r_mated.count > 0 )
                {
                    AddInitialFemaleCohort( r_mated.female, r_mated.male, r_mated.count );
                }
            }

            for( auto& r_male : initial_genome_data.males )
            {
                if( r_male.count > 0 )
                {
                    AddInitialMaleCohort( r_male.genome, r_male.count );
                }
            }
        }
    }

    void VectorPopulation::AddInitialCohort( VectorStateEnum::Enum state,
                                             const VectorGenome& rGenomeFemale,
                                             const VectorGenome& rGenomeMate,
                                             uint32_t num )
    {
        uint32_t days_between_feeds = m_context->GetRng()->randomRound( GetFeedingCycleDuration() );
        float percent_feed_per_day = 1.0f / float( days_between_feeds );

        std::vector<float> percentages( days_between_feeds, percent_feed_per_day );
        std::vector<uint64_t> num_fed_on_each_day = m_context->GetRng()->multinomial_approx( num, percentages );
        uint32_t num_remaining = num;
        for( uint32_t day = 1; day <= days_between_feeds; ++day )
        {
            uint32_t feed_on_day = num_remaining;
            if( day < days_between_feeds )
            {
                feed_on_day = num_fed_on_each_day[ day-1 ];
            }
            release_assert( num_remaining >= feed_on_day );
            num_remaining -= feed_on_day;

            float age = 0.0;
            if( params()->vector_aging )
            {
                age = float( (days_between_feeds + 1) - day );
            }

            if( state == VectorStateEnum::STATE_ADULT )
            {
                // progress is 1 since males/females should be at 1 from progressing from Immature to adult
                VectorCohort* pvc = VectorCohort::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                state,
                                                                age,
                                                                1.0,
                                                                0.0,
                                                                feed_on_day,
                                                                rGenomeFemale,
                                                                m_SpeciesIndex );

                queueIncrementTotalPopulation(pvc);
                // -------------------------------------------------------------------------------------------------
                // --- Initialize the cohort's gestation queue so that they start off laying eggs
                // --- This should be similar to how VectorPopulationIndividual has individual vectors starting off 
                // --- We assign days [1, days_between_feeds] (vs [0,days_between_feeds-1] because
                // --- we want to simulate that the eggs were added at the end of feeding.
                // --- For example, if day=1, then the cohort will birth eggs on the first update.
                // --- If day = days_between_feeds, the cohort will birth eggs on the "days_between_feeds'th" day.
                // ------------------------------------------------------------------------------------------------
                pvc->AddNewGestating( day, feed_on_day );
                pvc->SetMateGenome( rGenomeMate );
                pAdultQueues->add( pvc, 0.0, true );
            }
            else
            {
                // progress is 1 since males/females should be at 1 from progressing from Immature to adult
                VectorCohortMale* pvc = VectorCohortMale::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                        age,
                                                                        1.0,
                                                                        0.0,
                                                                        feed_on_day,
                                                                        rGenomeFemale,
                                                                        m_SpeciesIndex );
                queueIncrementTotalPopulation( pvc );
                m_NeedToRefreshTheMatingCDF = true; // anytime MaleQueues is modified, we need to refresh before mating
                MaleQueues.add( pvc, 0.0, true );
            }
        }
    }

    void VectorPopulation::AddInitialFemaleCohort( const VectorGenome& rGenomeFemale,
                                                   const VectorGenome& rGenomeMate,
                                                   uint32_t num )
    {
        AddInitialCohort( VectorStateEnum::STATE_ADULT, rGenomeFemale, rGenomeMate, num );
    }

    void VectorPopulation::AddInitialMaleCohort( const VectorGenome& rGenome, uint32_t num )
    {
        VectorGenome genome_empty;
        AddInitialCohort( VectorStateEnum::STATE_MALE, rGenome, genome_empty, num );
    }

    VectorPopulation *VectorPopulation::CreatePopulation( INodeContext *context,
                                                          int speciesIndex,
                                                          uint32_t adults )
    {
        VectorPopulation *newpopulation = _new_ VectorPopulation();
        release_assert( newpopulation );
        newpopulation->Initialize( context, speciesIndex, adults );
        return newpopulation;
    }

    void VectorPopulation::SetupLarvalHabitat( INodeContext *context )
    {
        // Query for vector node context
        IVectorNodeContext* ivnc = nullptr;
        if (s_OK !=  context->QueryInterface(GET_IID(IVectorNodeContext), (void**)&ivnc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorNodeContext", "INodeContext" );
        }

        // Create a larval habitat for each type specified in configuration file for this species
        for( IVectorHabitat* p_habitat_orig : species()->habitat_params.GetHabitats() )
        {
            IVectorHabitat* p_habitat = p_habitat_orig->Clone();

            float max_larval_capacity = p_habitat->GetMaximumLarvalCapacity()
                                      * params()->x_tempLarvalHabitat
                                      * ivnc->GetLarvalHabitatMultiplier( p_habitat->GetVectorHabitatType(), m_species_params->name );
            p_habitat->SetMaximumLarvalCapacity( max_larval_capacity );

            ivnc->AddHabitat( m_species_params->name, p_habitat );
        }
    }

    VectorPopulation::~VectorPopulation()
    {
        for( auto egg_map : m_EggCohortVectorOfMaps )
        {
            for( auto entry : egg_map )
            {
                delete entry.second;
            }
        }
        m_EggCohortVectorOfMaps.clear();

        for( auto p_vc : EggQueues )
        {
            delete p_vc;
        }
        EggQueues.clear();

        LarvaQueues.clear();
        ImmatureQueues.clear();
        MaleQueues.clear();
        pAdultQueues->clear();
        InfectedQueues.clear();
        InfectiousQueues.clear();

        for( auto p_vc : m_ReleasedInfectious )
        {
            delete p_vc;
        }
        for( auto p_vc : m_ReleasedAdults )
        {
            delete p_vc;
        }
        for( auto p_vc : m_ReleasedMales )
        {
            delete p_vc;
        }
        m_ReleasedInfectious.clear();
        m_ReleasedAdults.clear();
        m_ReleasedMales.clear();

        for( auto p_vc : m_ImmigratingInfectious )
        {
            delete p_vc;
        }
        for( auto p_vc : m_ImmigratingInfected )
        {
            delete p_vc;
        }
        for( auto p_vc : m_ImmigratingAdult )
        {
            delete p_vc;
        }
        for (auto p_vc : m_ImmigratingMale)
        {
            delete p_vc;
        }
        m_ImmigratingInfectious.clear();
        m_ImmigratingInfected.clear();
        m_ImmigratingAdult.clear();
        m_ImmigratingMale.clear();
    }

    void VectorPopulation::UpdateLocalMatureMortalityProbability( float dt )
    {
        if( m_LocalMatureMortalityProbabilityTable[ VectorGender::VECTOR_FEMALE ][0].size() == 0 )
        {
            for( int g = 0; g < 2; ++g ) // g=0=female, g=1=male
            {
                for( int w = 0; w < 2; ++w ) // w=0=no wolbachia, w=1=has wolbachia
                {
                    for( int m = 0; m < species()->microsporidia_strains.Size(); ++m )
                    {
                        // add an array for ages for each strain
                        m_LocalMatureMortalityProbabilityTable[ g ][ w ].push_back( std::vector<float>() );
                    }
                }
            }
        }

        UpdateLocalMatureMortalityProbabilityTable( dt,
                                                    species()->adultmortality,
                                                    species()->microsporidia_strains.GetMortalityModifierListFemale(),
                                                    &m_LocalMatureMortalityProbabilityTable[ VectorGender::VECTOR_FEMALE ],
                                                    &m_DefaultLocalMatureMortalityProbability[ VectorGender::VECTOR_FEMALE ] );

        UpdateLocalMatureMortalityProbabilityTable( dt,
                                                    species()->malemortality,
                                                    species()->microsporidia_strains.GetMortalityModifierListMale(),
                                                    &m_LocalMatureMortalityProbabilityTable[ VectorGender::VECTOR_MALE ],
                                                    &m_DefaultLocalMatureMortalityProbability[ VectorGender::VECTOR_MALE ] );

        int num_microsporidia_strains = species()->microsporidia_strains.Size();
        release_assert( m_LocalMatureMortalityProbabilityTable[ 0 ][ 0 ].size() == num_microsporidia_strains); //for female and wolbachia free - 2 microsporidia strains
        release_assert( m_LocalMatureMortalityProbabilityTable[ 0 ][ 1 ].size() == num_microsporidia_strains); //for female and has wolbachia  - 2 microsporidia strains
        release_assert( m_LocalMatureMortalityProbabilityTable[ 1 ][ 0 ].size() == num_microsporidia_strains); //for male and wolbachia free - 2 microsporidia strains
        release_assert( m_LocalMatureMortalityProbabilityTable[ 1 ][ 1 ].size() == num_microsporidia_strains); //for male and has wolbachia  - 2 microsporidia strains
    }

    void VectorPopulation::UpdateLocalMatureMortalityProbabilityTable( float dt,
                                                                       float matureMortality,
                                                                       const std::vector<float>& microsporidiaModifier,
                                                                       std::vector<std::vector<std::vector<float>>>* pProbabilityTable,
                                                                       float* pDefaultProbability )
    {
        int num_microsporidia_strains = species()->microsporidia_strains.Size();
        release_assert(  (*pProbabilityTable).size() == 2 ); // pointer is for one gender, so 2 for wolbachie free and has
        release_assert(  (*pProbabilityTable)[ 0 ].size() == num_microsporidia_strains ); // for wolbachia free, 2 for microsporidia strains
        release_assert(  (*pProbabilityTable)[ 1 ].size() == num_microsporidia_strains ); // for has wolbachia, 2 for microsporidia strains
        for( int w = 0; w < 2; ++w )
        {
            for( int ms = 0; ms < num_microsporidia_strains; ++ms )
            {
                (*pProbabilityTable)[ w ][ ms ].clear(); // clear the ages
            }
        }

        float wolb_free = 1.0;
        float wolb_mod  = params()->WolbachiaMortalityModification;
        float ms_free   = 1.0;

        // if we are not aging vectors, then "from_age" below is always zero.
        uint32_t num_ages = params()->vector_aging ? m_MortalityTable.size() : 1;

        // calculate probability for immature vectors
        float local_mortality = matureMortality + dryheatmortality;
        *pDefaultProbability = float( EXPCDF( -dt * local_mortality * wolb_free * ms_free ) );

        for( int ms = 0; ms < microsporidiaModifier.size(); ++ms )
        {
            float ms_mod = microsporidiaModifier[ ms ];
            for( uint32_t i_age = 0 ; i_age < num_ages; ++i_age )
            {
                float from_age = 0.0;
                if( params()->vector_aging )
                {
                    from_age = m_MortalityTable[ i_age ];
                }

                float p_local_mortality_wfree_ms = float( EXPCDF( -dt * (local_mortality + from_age) * wolb_free * ms_mod ) );
                float p_local_mortality_wmod_ms  = float( EXPCDF( -dt * (local_mortality + from_age) * wolb_mod  * ms_mod  ) );

                (*pProbabilityTable)[ 0 ][ ms ].push_back( p_local_mortality_wfree_ms );
                (*pProbabilityTable)[ 1 ][ ms ].push_back( p_local_mortality_wmod_ms  );
            }
        }
    }


    float VectorPopulation::GetLocalMatureMortalityProbability( float dt, IVectorCohort* pvc ) const
    {
        release_assert( pvc != nullptr );

        int i_gender = int( pvc->GetGenome().GetGender() );
        VectorStateEnum::Enum state = pvc->GetState();

        float p_local_mortality = m_DefaultLocalMatureMortalityProbability[ i_gender ];
        if( (state == VectorStateEnum::STATE_MALE      ) || 
            (state == VectorStateEnum::STATE_ADULT     ) || 
            (state == VectorStateEnum::STATE_INFECTED  ) || 
            (state == VectorStateEnum::STATE_INFECTIOUS) )
        {
            // --------------------------------------------------------------------------------
            // --- We don't use the WolbachiaMortalityModification on Males and Immature vectors 
            // --------------------------------------------------------------------------------
            uint32_t i_wolb = ((state == VectorStateEnum::STATE_ADULT) && pvc->HasWolbachia()) ? 1 : 0;
            uint32_t i_ms   = pvc->GetGenome().GetMicrosporidiaStrainIndex();
            uint32_t i_age  = (params()->vector_aging  ) ? uint32_t( pvc->GetAge() ) : 0;

            if( i_age >= VectorCohort::I_MAX_AGE )
            {
                // When I_MAX_AGE is large (i.e. > 100 ), then mortalityFromAge() approaches zero
                // so the last value should not be different from a value calculated at a larger age.
               i_age = VectorCohort::I_MAX_AGE-1;
            }

            p_local_mortality = m_LocalMatureMortalityProbabilityTable[ i_gender ][ i_wolb ][ i_ms ][ i_age ];
        }
        float modifier = species()->trait_modifiers.GetModifier( VectorTrait::MORTALITY, pvc->GetGenome() );
        p_local_mortality *= modifier;
        return p_local_mortality;
    }

    void VectorPopulation::UpdateAge( IVectorCohort* pvc, float dt )
    {
        if( pvc != nullptr )
        {
            pvc->IncreaseAge( dt );
        }
    }

    void VectorPopulation::VerifyCounts( VectorStateEnum::Enum state,
                                         const VectorCohortCollectionAbstract& rCollection,
                                         uint32_t expCount,
                                         const GenomeCountMap_t& rExpGenomeCount )
    {
        std::string state_name = VectorStateEnum::pairs::lookup_key( state );
        uint32_t actual_count = 0;
        GenomeCountMap_t actual_genome_count;
        for( auto pvc : rCollection )
        {
            if( pvc->GetState() != state ) continue;
            actual_count += pvc->GetPopulation();
            actual_genome_count[ pvc->GetGenome().GetBits() ] += pvc->GetPopulation();
        }
        if( actual_count != expCount )
        {
            std::stringstream ss;
            ss << state_name << "  expCount(=" << expCount << ") != actual_count(=" << actual_count << ")" ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        uint32_t exp_count_from_genomes = 0;
        for( auto entry : rExpGenomeCount )
        {
            exp_count_from_genomes += entry.second;
            if( actual_genome_count[ entry.first ] != entry.second )
            {
                std::string genome_name = m_species_params->genes.GetGenomeName( VectorGenome(entry.first) );
                std::stringstream ss;
                ss << state_name << "  genome='" << genome_name << "'   expCount(=" << entry.second << ") != actual_count(=" << actual_genome_count[ entry.first ] << ")";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        if( exp_count_from_genomes != expCount )
        {
            std::stringstream ss;
            ss << state_name << "  expCount(=" << expCount << ") != exp_count_from_genomes(=" << exp_count_from_genomes << ")";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void VectorPopulation::VerifyAllCounts()
    {
#if 0
        VerifyCounts( VectorStateEnum::STATE_MALE,       MaleQueues,       state_counts[ VectorStateEnum::STATE_MALE       ], genome_counts[ VectorStateEnum::STATE_MALE       ] );
        VerifyCounts( VectorStateEnum::STATE_IMMATURE,   ImmatureQueues,   state_counts[ VectorStateEnum::STATE_IMMATURE   ], genome_counts[ VectorStateEnum::STATE_IMMATURE   ] );
        VerifyCounts( VectorStateEnum::STATE_LARVA,      LarvaQueues,      state_counts[ VectorStateEnum::STATE_LARVA      ], genome_counts[ VectorStateEnum::STATE_LARVA      ] );
        VerifyCounts( VectorStateEnum::STATE_ADULT,      *pAdultQueues,      state_counts[ VectorStateEnum::STATE_ADULT      ], genome_counts[ VectorStateEnum::STATE_ADULT      ] );
        if( (params()->vector_sampling_type == VectorSamplingType::TRACK_ALL_VECTORS) ||
            (params()->vector_sampling_type == VectorSamplingType::SAMPLE_IND_VECTORS) )
        {
            VerifyCounts( VectorStateEnum::STATE_INFECTED,   *pAdultQueues, state_counts[ VectorStateEnum::STATE_INFECTED   ], genome_counts[ VectorStateEnum::STATE_INFECTED   ] );
            VerifyCounts( VectorStateEnum::STATE_INFECTIOUS, *pAdultQueues, state_counts[ VectorStateEnum::STATE_INFECTIOUS ], genome_counts[ VectorStateEnum::STATE_INFECTIOUS ] );
        }
        else
        {
            VerifyCounts( VectorStateEnum::STATE_INFECTED,   InfectedQueues,   state_counts[ VectorStateEnum::STATE_INFECTED   ], genome_counts[ VectorStateEnum::STATE_INFECTED   ] );
            VerifyCounts( VectorStateEnum::STATE_INFECTIOUS, InfectiousQueues, state_counts[ VectorStateEnum::STATE_INFECTIOUS ], genome_counts[ VectorStateEnum::STATE_INFECTIOUS ] );
        }
#endif

    }

    // 
    void VectorPopulation::visitVectors( vector_cohort_visit_function_t  func, VectorGender::Enum gender = VectorGender::VECTOR_BOTH_GENDERS)
    {
        if (gender == VectorGender::VECTOR_BOTH_GENDERS || gender == VectorGender::VECTOR_FEMALE)
        {
            // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
            for (auto it = pAdultQueues->begin(); it != pAdultQueues->end(); ++it)
            {
                IVectorCohort* cohort = *it;
                func(cohort);
            }

            // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
            for (auto it = InfectedQueues.begin(); it != InfectedQueues.end(); ++it)
            {
                IVectorCohort* cohort = *it;
                func(cohort);
            }

            // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
            for (auto it = InfectiousQueues.begin(); it != InfectiousQueues.end(); ++it)
            {
                IVectorCohort* cohort = *it;
                func(cohort);
            }
        }
        if (gender == VectorGender::VECTOR_BOTH_GENDERS || gender == VectorGender::VECTOR_MALE)
        {
            // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
            for (auto it = MaleQueues.begin(); it != MaleQueues.end(); ++it)
            {
                IVectorCohort* cohort = *it;
                func(cohort);
            }
        }
    }


    void VectorPopulation::UpdateVectorPopulation( float dt )
    {
        // Reset EIR/HBR reporting
        m_EIR_by_pool = std::make_pair(0.0f, 0.0f);
        m_HBR_by_pool = std::make_pair(0.0f, 0.0f);

        // Reset counters
        release_assert( genome_counts.size() == m_AgeAtDeathByGenomeByState.size() );
        release_assert( genome_counts.size() == m_AgeAtDeathByState.size() );
        for( int i = 0; i < genome_counts.size(); ++i )
        {
            genome_counts[ i ].clear();
            m_AgeAtDeathByGenomeByState[ i ].clear();
            m_AgeAtDeathByState[ i ].clear();
        }
        m_ProgressedLarvaeToImmatureCount = 0;
        m_ProgressedLarvaeToImmatureSumDur = 0.0;

        memset( state_counts.data(),              0, state_counts             .size()*sizeof(state_counts             [0]) );
        memset( num_infs_per_state_counts.data(), 0, num_infs_per_state_counts.size()*sizeof(num_infs_per_state_counts[0]) );
        memset( wolbachia_counts.data(),          0, wolbachia_counts         .size()*sizeof(wolbachia_counts         [0]) );

        for( int strain_index = 0; strain_index < m_species_params->microsporidia_strains.Size(); ++strain_index )
        {
            memset( microsporidia_counts[ strain_index ].data(), 0, microsporidia_counts[ strain_index ].size() * sizeof(uint32_t));
        }

        num_gestating_queue.clear();
        num_gestating_begin     = 0;
        num_gestating_end       = 0;
        num_looking_to_feed     = 0;
        num_fed_counter         = 0;
        num_attempt_feed_indoor = 0;
        num_attempt_feed_outdoor= 0;
        num_attempt_but_not_feed= 0;
        neweggs                 = 0;
        new_adults              = 0;
        unmated_adults          = 0;
        indoorinfectiousbites   = 0;
        outdoorinfectiousbites  = 0;
        indoorbites             = 0;
        outdoorbites            = 0;
        dead_mosquitoes_before  = 0;
        dead_mosquitoes_indoor  = 0;
        dead_mosquitoes_outdoor = 0;

        gender_mating_eggs.clear();

        float temperature = m_context->GetLocalWeather()->airtemperature();
        infected_progress_this_timestep = (species()->infectedarrhenius1 * exp( -species()->infectedarrhenius2 / (temperature + float( CELSIUS_TO_KELVIN )) )) * dt;

        // Over-ride some lifecycle probabilities with species-specific values
        Update_Lifecycle_Probabilities(dt);

        // Update vector-cohort lists
        AddReleasedVectorsToQueues();
        if ( m_NeedToRefreshTheMatingCDF )
        {
            // --------------------------------------------------------------------
            // --- Updates/refreshes m_MaleMatingCDF and m_UnmatedMaleTotal using the unmated_counts
            // --- from MaleQueues: males that stayed, immigrated males, and newly released males or de-serialized male population.
            // --- Does not change number of unmated males, just re-counts them into the CDF. 
            // --------------------------------------------------------------------
            BuildMaleMatingCDF(false);
        }
        Update_Infectious_Queue(dt);
        Update_Infected_Queue(dt);
        Update_Adult_Queue(dt);
        Update_Male_Queue(dt);
        Update_Immature_Queue(dt);
        Update_Larval_Queue(dt);
        Update_Egg_Hatching( dt );
        Update_Egg_Laying(dt);

        DepositContagion();

        VerifyAllCounts(); // enable/disable in the method

        // Now calculate quantities for reporting:
        float eff_pop = m_context->GetStatPop(); //probs()->effective_host_population; // actual bites divided by actual pop (not risk-weighted pop for feeding probs)
        if( eff_pop > 0 )
        {
            // (1) infectious bites (EIR)
            m_EIR_by_pool.first  = float(indoorinfectiousbites / eff_pop);
            m_EIR_by_pool.second = float(outdoorinfectiousbites / eff_pop);
            // (2) total human bites (HBR)
            m_HBR_by_pool.first  = float(indoorbites / eff_pop);
            m_HBR_by_pool.second = float(outdoorbites / eff_pop);
        }
        else
        {
            LOG_DEBUG_F("The effective human population at node %lu is zero, so EIR and HBR are not being normalized in VectorPopulation::UpdateVectorPopulation.\n", m_context->GetSuid().data );
        }
    }

    float VectorPopulation::GetInfectedProgress( IVectorCohort* pCohort )
    {
        float modifier = 1.0;
        if( pCohort->GetState() == VectorStateEnum::STATE_INFECTED )
        {
            modifier = this->m_species_params->trait_modifiers.GetModifier( VectorTrait::INFECTED_PROGRESS, pCohort->GetGenome() );
        }
        return infected_progress_this_timestep * modifier;
    }

    void VectorPopulation::Update_Lifecycle_Probabilities( float dt )
    {
        // Update adult transition probabilities:
        // calculated for each queue entry up to human indoor and outdoor feeding attempts
        probs()->FinalizeTransitionProbabilites( species()->anthropophily, species()->indoor_feeding, species()->vsp_blood_meal_mortality ); 

        // Update mortality from heat
        dryheatmortality = 0.0;
        if( m_VectorMortality )
        {
            float temperature = m_context->GetLocalWeather()->airtemperature();
            dryheatmortality  = dryHeatMortality(temperature);
        }
        UpdateLocalMatureMortalityProbability( dt );
    }

    void VectorPopulation::Update_Infectious_Queue( float dt )
    {
        InfectiousQueues.updateAge( dt );

        // Use the verbose "foreach" construct here because empty infectious cohorts (e.g. old vectors) will be removed
        for( auto it = InfectiousQueues.begin(); it != InfectiousQueues.end(); ++it )
        {
            IVectorCohort* cohort = *it;

            bool prev_has_microsporidia = cohort->HasMicrosporidia();
            if( !cohort->HasMated() )
            {
                AddAdultsAndMate( cohort, InfectiousQueues, false );
            }

            if( cohort->GetPopulation() > 0 )
            {
                ProcessFeedingCycle( dt, cohort );
            }

            cohort->Update( m_context->GetRng(), dt, species()->trait_modifiers, 0.0, prev_has_microsporidia );

            queueIncrementTotalPopulation( cohort );// update INFECTIOUS counters

            if( cohort->GetPopulation() <= 0 )
            {
                InfectiousQueues.remove( it );
                delete cohort;
            }
        }
        InfectiousQueues.compact();
    }

    void VectorPopulation::Update_Infected_Queue( float dt )
    {
        // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
        for( auto it = InfectedQueues.begin(); it != InfectedQueues.end(); ++it )
        {
            IVectorCohort* cohort = *it;

            UpdateAge( cohort, dt );

            bool prev_has_microsporidia = cohort->HasMicrosporidia();
            if( !cohort->HasMated() )
            {
                AddAdultsAndMate( cohort, InfectedQueues, false );
            }

            if( cohort->GetPopulation() > 0 )
            {
                ProcessFeedingCycle( dt, cohort );
            }

            cohort->Update( m_context->GetRng(), dt, species()->trait_modifiers, GetInfectedProgress( cohort ), prev_has_microsporidia );

            queueIncrementTotalPopulation( cohort ); // update counters

            // done with this queue if it is fully progressed or is empty
            if( (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) || (cohort->GetPopulation() <= 0) )
            {
                InfectedQueues.remove( it );

                // infected queue completion, moving to infectious
                if (cohort->GetPopulation() > 0)
                {
                    InfectiousQueues.add( cohort, 0.0, true );
                }
                else
                {
                    delete cohort;
                }
            }
        }
        InfectedQueues.compact();
    }

    void VectorPopulation::UpdateGestatingCount( const IVectorCohort* cohort )
    {
        if( cohort->GetPopulation() > 0 )
        {
            num_gestating_end += cohort->GetNumGestating();
            cohort->ReportOnGestatingQueue( num_gestating_queue );
        }
    }

    void VectorPopulation::Update_Adult_Queue( float dt )
    {
        pAdultQueues->updateAge( dt );

        // Use the verbose "foreach" construct here because empty adult cohorts (e.g. old vectors) will be removed
        for( auto it = pAdultQueues->begin(); it != pAdultQueues->end(); ++it )
        {
            IVectorCohort* cohort = *it;

            bool prev_has_microsporidia = cohort->HasMicrosporidia();
            if( !cohort->HasMated() )
            {
                AddAdultsAndMate( cohort, *pAdultQueues, false );
            }

            uint32_t newinfected = 0;
            if( cohort->GetPopulation() > 0 )
            {
                 newinfected = ProcessFeedingCycle( dt, cohort );
            }

            cohort->Update( m_context->GetRng(), dt,species()->trait_modifiers, 0.0, prev_has_microsporidia );

            if (newinfected > 0)
            {
                // Move newly infected vectors into another cohort in the infected queue
                // Since they are infected, they have just fed and need to start gestating
                IVectorCohort* infected_cohort = VectorCohort::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                             VectorStateEnum::STATE_INFECTED,
                                                                             cohort->GetAge(),
                                                                             0.0, //reset progress
                                                                             cohort->GetDurationOfMicrosporidia(),
                                                                             newinfected,
                                                                             cohort->GetGenome(),
                                                                             m_SpeciesIndex );
                infected_cohort->SetMateGenome( cohort->GetMateGenome() );
                StartGestating( newinfected, infected_cohort );
                queueIncrementTotalPopulation( infected_cohort ); // update INFECTED counters
                InfectedQueues.add( infected_cohort, GetInfectedProgress( cohort ), true );
            }

            queueIncrementTotalPopulation( cohort ); // update ADULT counters

            if( cohort->GetPopulation() <= 0 )
            {
                pAdultQueues->remove( it );
                delete cohort;
            }
        }
        pAdultQueues->compact();
    }

    float VectorPopulation::AdjustForConditionalProbability( float& rCumulative, float probability )
    {
        if( rCumulative >= 1.0 )
        {
            return 1.0;
        }

        //adjust for conditional probability
        float adjusted = probability / (1.0f - rCumulative);
        if( adjusted > 1.0 )
        {
            adjusted = 1.0;
        }

        rCumulative += probability;
        if( rCumulative > 1.0 )
        {
            rCumulative = 1.0;
        }

        return adjusted;
    }

    void VectorPopulation::UpdateSugarKilling( IVectorCohort* cohort, VectorPopulation::FeedingProbabilities& rFeedProbs )
    {
        // Now if sugar feeding exists every day or after each feed, and mortality is associated, then set probabilities
        if( (species()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED) ||
            (species()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY ) )
        {
            float sugar_trap_killing = probs()->sugarTrapKilling.GetValue( m_SpeciesIndex, cohort->GetGenome() );

            // adjust for the vectors that ARE feeding today
            rFeedProbs.die_sugar_feeding = (1.0 - rFeedProbs.die_local_mortality) * sugar_trap_killing;

            // now adjust the rest of the feeding branch probabilities to account for
            // the vectors that died during sugar feeding
            rFeedProbs.die_before_human_feeding        *= (1.0f - sugar_trap_killing );
            rFeedProbs.successful_feed_animal          *= (1.0f - sugar_trap_killing );
            rFeedProbs.successful_feed_artifical_diet  *= (1.0f - sugar_trap_killing );
            rFeedProbs.successful_feed_attempt_indoor  *= (1.0f - sugar_trap_killing );
            rFeedProbs.successful_feed_attempt_outdoor *= (1.0f - sugar_trap_killing );
            rFeedProbs.survive_without_feeding         *= (1.0f - sugar_trap_killing );
        }

        // correct for sugar feeding
        if( species()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY )
        {
            float sugar_trap_killing = probs()->sugarTrapKilling.GetValue( m_SpeciesIndex, cohort->GetGenome() );

            // adjust killing for the vectors that are not feeding today
            rFeedProbs.die_without_attempting_to_feed += (1.0 - rFeedProbs.die_without_attempting_to_feed) * sugar_trap_killing;  // add in sugarTrap to kill rate
        }
    }

    void VectorPopulation::AdjustForCumulativeProbability( VectorPopulation::FeedingProbabilities& rFeedProbs )
    {
        //--------------------------------------------------------------------------------------------------
        // --- We use AdjustForConditionalProbability() because we are trying to find the probability
        // --- that X vectors did something from a subset that did something else.  For example,
        // --- we want the probabity that if a vector did not die, then did it have a successful animal feed.
        // --- Then, if it did not die and did not have a successful animal feed, did it have a successful
        // --- artificial diet?
        //--------------------------------------------------------------------------------------------------

        // -----------------------
        // --- Non-Feeding Branch
        // -----------------------
        float cum_prob = rFeedProbs.die_local_mortality;
        rFeedProbs.die_without_attempting_to_feed = AdjustForConditionalProbability( cum_prob, rFeedProbs.die_without_attempting_to_feed );

        // -----------------------
        // --- Feeding Branch
        // -----------------------
        cum_prob  = rFeedProbs.die_local_mortality;
        rFeedProbs.die_sugar_feeding               = AdjustForConditionalProbability( cum_prob, rFeedProbs.die_sugar_feeding               );
        rFeedProbs.die_before_human_feeding        = AdjustForConditionalProbability( cum_prob, rFeedProbs.die_before_human_feeding        );
        rFeedProbs.successful_feed_animal          = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_animal          );
        rFeedProbs.successful_feed_artifical_diet  = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_artifical_diet  );
        rFeedProbs.successful_feed_attempt_indoor  = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_attempt_indoor  );
        rFeedProbs.successful_feed_attempt_outdoor = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_attempt_outdoor );
        rFeedProbs.survive_without_feeding         = AdjustForConditionalProbability( cum_prob, rFeedProbs.survive_without_feeding         );

        // -------------------------------------------------------------
        // --- Attempt Indoor Feeding Branch - subset of Feeding Branch
        // -------------------------------------------------------------
        cum_prob = rFeedProbs.indoor.successful_feed_ad;
        rFeedProbs.indoor.die_before_feeding    = AdjustForConditionalProbability( cum_prob, rFeedProbs.indoor.die_before_feeding    );
        rFeedProbs.indoor.not_available         = AdjustForConditionalProbability( cum_prob, rFeedProbs.indoor.not_available         );
        rFeedProbs.indoor.die_during_feeding    = AdjustForConditionalProbability( cum_prob, rFeedProbs.indoor.die_during_feeding    );
        rFeedProbs.indoor.die_after_feeding     = AdjustForConditionalProbability( cum_prob, rFeedProbs.indoor.die_after_feeding     );
        rFeedProbs.indoor.successful_feed_human = AdjustForConditionalProbability( cum_prob, rFeedProbs.indoor.successful_feed_human );

        // -------------------------------------------------------------
        // --- Attempt Outdoor Feeding Branch - subset of Feeding Branch
        // -------------------------------------------------------------
        cum_prob = rFeedProbs.outdoor.die_before_feeding;
        rFeedProbs.outdoor.not_available         = AdjustForConditionalProbability( cum_prob, rFeedProbs.outdoor.not_available         );
        rFeedProbs.outdoor.die_during_feeding    = AdjustForConditionalProbability( cum_prob, rFeedProbs.outdoor.die_during_feeding    );
        rFeedProbs.outdoor.die_after_feeding     = AdjustForConditionalProbability( cum_prob, rFeedProbs.outdoor.die_after_feeding     );
        rFeedProbs.outdoor.successful_feed_human = AdjustForConditionalProbability( cum_prob, rFeedProbs.outdoor.successful_feed_human );
    }

    VectorPopulation::FeedingProbabilities VectorPopulation::CalculateFeedingProbabilities( float dt, IVectorCohort* cohort )
    {
        // Adjust human-feeding mortality for longer-probing infectious vectors
        // Wekesa, J. W., R. S. Copeland, et al. (1992). 
        //     "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya."
        //     Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000).
        //     "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l."
        //     Parasitology 120(04): 329-333.
        float x_infectioushfmortmod  = 1.0f;
        float x_infectiouscorrection = 1.0f;
        if( cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
        {
            x_infectioushfmortmod  = species()->infectioushfmortmod;
            x_infectiouscorrection = infectiouscorrection;
        }

        const VectorGenome& r_genome = cohort->GetGenome();

        // calculate local mortality / survivability
        float p_local_mortality = GetLocalMatureMortalityProbability( dt, cohort );
        float p_local_survivability = 1.0f - p_local_mortality;

        float feed_attempt_outdoor    = probs()->outdoorattempttohumanfeed.GetValue(    m_SpeciesIndex, r_genome );
        float survive_without_feeding = probs()->survivewithoutsuccessfulfeed.GetValue( m_SpeciesIndex, r_genome );

        FeedingProbabilities feed_probs;

        feed_probs.die_local_mortality             = p_local_mortality;
        feed_probs.die_without_attempting_to_feed  = p_local_survivability * probs()->diewithoutattemptingfeed.GetValue( m_SpeciesIndex, r_genome );
        feed_probs.die_sugar_feeding               = 0.0f;
        feed_probs.die_before_human_feeding        = p_local_survivability * probs()->diebeforeattempttohumanfeed.GetValue(  m_SpeciesIndex, r_genome );
        feed_probs.successful_feed_animal          = p_local_survivability * probs()->successfulfeed_animal.GetValue(        m_SpeciesIndex, r_genome );
        feed_probs.successful_feed_artifical_diet  = p_local_survivability * probs()->successfulfeed_AD.GetValue(            m_SpeciesIndex, r_genome );
        feed_probs.successful_feed_attempt_indoor  = p_local_survivability * probs()->indoorattempttohumanfeed.GetValue(     m_SpeciesIndex, r_genome );
        feed_probs.successful_feed_attempt_outdoor = p_local_survivability * feed_attempt_outdoor;
        feed_probs.survive_without_feeding         = p_local_survivability * survive_without_feeding;

        feed_probs.indoor.successful_feed_ad       = probs()->indoor_successfulfeed_AD.GetValue(    m_SpeciesIndex, r_genome );
        feed_probs.indoor.die_before_feeding       = probs()->indoor_diebeforefeeding.GetValue(     m_SpeciesIndex, r_genome );
        feed_probs.indoor.not_available            = probs()->indoor_hostnotavailable.GetValue(     m_SpeciesIndex, r_genome );
        feed_probs.indoor.die_during_feeding       = probs()->indoor_dieduringfeeding.GetValue(     m_SpeciesIndex, r_genome ) * x_infectioushfmortmod;
        feed_probs.indoor.die_after_feeding        = probs()->indoor_diepostfeeding.GetValue( m_SpeciesIndex, r_genome ) * x_infectiouscorrection;
        feed_probs.indoor.successful_feed_human    = probs()->indoor_successfulfeed_human.GetValue( m_SpeciesIndex, r_genome ) * x_infectiouscorrection;

        feed_probs.outdoor.die_before_feeding      = probs()->outdoor_diebeforefeeding;
        feed_probs.outdoor.not_available           = probs()->outdoor_hostnotavailable.GetValue(     m_SpeciesIndex, r_genome );
        feed_probs.outdoor.die_during_feeding      = probs()->outdoor_dieduringfeeding.GetValue(     m_SpeciesIndex, r_genome ) * x_infectioushfmortmod;
        feed_probs.outdoor.die_after_feeding       = probs()->outdoor_diepostfeeding.GetValue( m_SpeciesIndex, r_genome ) * x_infectiouscorrection;
        feed_probs.outdoor.successful_feed_human   = probs()->outdoor_successfulfeed_human.GetValue( m_SpeciesIndex, r_genome ) * x_infectiouscorrection;

        UpdateSugarKilling( cohort, feed_probs );

        // -----------------------------------------------------------------------------------
        // --- We need to normalize the probabilities due to floating point rounding errors.
        // --- For example, 0.1 can turn into 0.0999999 but then we sum that up over the
        // --- number of people.  The error becomes slightly magnified and normalization
        // --- helps to remove it.  The code after this assumes the normalization.
        // -----------------------------------------------------------------------------------
        feed_probs.Normalize();

        AdjustForCumulativeProbability( feed_probs );

        // -------------------------------------------------------------------------------------
        // --- Normalization helps but does not resolve everyting.  Hence, we need to make sure
        // --- values that are supposed to be 1.0 are 1.0.
        // --- NOTE: survive_without_feeding can be > 0.0 when using spatial repellent.
        // ---       This means that successful_feed_attempt_outdoor will be < 1.0
        // -------------------------------------------------------------------------------------
        if( survive_without_feeding == 0.0 )
        {
            feed_probs.successful_feed_attempt_outdoor = 1.0;
            if( feed_attempt_outdoor == 0.0 )
            {
                feed_probs.successful_feed_attempt_indoor = 1.0;
                // We could go further but stopping since this should happen rarely
            }
        }

        // successful_feed_human can equal zero if all the other values are zero, but should be 1.0
        // if the others are not zero.  However, there can be floating point round off error,
        // so we need to force it to 1.
        if( feed_probs.indoor.successful_feed_human  != 0.0 ) feed_probs.indoor.successful_feed_human  = 1.0;
        if( feed_probs.outdoor.successful_feed_human != 0.0 ) feed_probs.outdoor.successful_feed_human = 1.0;

#if 0
        LOG_DEBUG_F( "die_local_mortality            =%14.11f\n", feed_probs.die_local_mortality );
        LOG_DEBUG_F( "die_without_attempting_to_feed =%14.11f\n", feed_probs.die_without_attempting_to_feed );
        LOG_DEBUG_F( "die_sugar_feeding              =%14.11f\n", feed_probs.die_sugar_feeding );
        LOG_DEBUG_F( "die_before_human_feeding       =%14.11f\n", feed_probs.die_before_human_feeding );
        LOG_DEBUG_F( "successful_feed_animal         =%14.11f\n", feed_probs.successful_feed_animal );
        LOG_DEBUG_F( "successful_feed_artifical_diet =%14.11f\n", feed_probs.successful_feed_artifical_diet );
        LOG_DEBUG_F( "successful_feed_attempt_indoor =%14.11f\n", feed_probs.successful_feed_attempt_indoor );
        LOG_DEBUG_F( "successful_feed_attempt_outdoor=%14.11f\n", feed_probs.successful_feed_attempt_outdoor );
        LOG_DEBUG_F( "survive_without_feeding        =%14.11f\n", feed_probs.survive_without_feeding );
        LOG_DEBUG_F( "indoor_successful_feed_ad      =%14.11f\n", feed_probs.indoor.successful_feed_ad );
        LOG_DEBUG_F( "indoor_die_before_feeding      =%14.11f\n", feed_probs.indoor.die_before_feeding );
        LOG_DEBUG_F( "indoor_not_available           =%14.11f\n", feed_probs.indoor.not_available );
        LOG_DEBUG_F( "indoor_die_during_feeding      =%14.11f\n", feed_probs.indoor.die_during_feeding );
        LOG_DEBUG_F( "indoor_die_after_feeding       =%14.11f\n", feed_probs.indoor.die_after_feeding );
        LOG_DEBUG_F( "indoor_successful_feed_human   =%14.11f\n", feed_probs.indoor.successful_feed_human );
        LOG_DEBUG_F( "outdoor_die_before_feeding     =%14.11f\n", feed_probs.outdoor.die_before_feeding );
        LOG_DEBUG_F( "outdoor_not_available          =%14.11f\n", feed_probs.outdoor.not_available );
        LOG_DEBUG_F( "outdoor_die_during_feeding     =%14.11f\n", feed_probs.outdoor.die_during_feeding );
        LOG_DEBUG_F( "outdoor_die_after_feeding      =%14.11f\n", feed_probs.outdoor.die_after_feeding );
        LOG_DEBUG_F( "outdoor_successful_feed_human  =%14.11f\n", feed_probs.outdoor.successful_feed_human );
#endif

        return feed_probs;
    }

    uint32_t VectorPopulation::CalculatePortionInProbability( uint32_t& rRemainingPop, float prob )
    {
        if( (rRemainingPop <= 0) || (prob <= 0.0) )
        {
            return 0;
        }
        else if( prob >= 1.0 )
        {
            // avoid drawing random number
            uint32_t num_in_prob = rRemainingPop;
            rRemainingPop = 0;
            return num_in_prob;
        }
        else
        {
            uint32_t num_in_prob = uint32_t( m_context->GetRng()->binomial_approx( rRemainingPop, prob ) );
            rRemainingPop -= num_in_prob;

            return num_in_prob;
        }
    }

    // ---------------------------------------------------------------------------------------------
    // --- GH-4251 - Number of Infectious Bites in Vector Model and Individual Are Not Similar
    // --- Dividing the amount deposited by the population average of the the probablity of
    // --- the vector transmitting.  All the values deposited will be normalized by the total
    // --- total number of people in the population - StrainAwareTransmissionGroups::EndUpdate().
    // --- IndividualHumanVector::Expose() will take the total contagion from SATG and multiply
    // --- this by the individuals probability of the vector transmitting to them.  This resulting
    // --- value will be used in the Poisson draw to determine how many bites the person received.
    // ---------------------------------------------------------------------------------------------
    void VectorPopulation::VectorToHumanDeposit( const VectorGenome& rGenome,
                                                 const IStrainIdentity& rStrain,
                                                 uint32_t numInfectiousBites,
                                                 TransmissionRoute::Enum route )
    {
        release_assert( (route == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR) ||
                        (route == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR) );

        bool first_one = false;
        IStrainIdentity* p_si = rStrain.Clone();
        StrainGenomeId sgi( p_si, rGenome.GetBits() );

        if( route == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR )
        {
            if( m_ContagionToDepositIndoor.count( sgi ) == 0 )
            {
                first_one = true;
                m_ContagionToDepositIndoor[ sgi ] = 0.0f;
            }
            // See comment above GH-4251
            float prob_transmit = probs()->indoor_dieduringfeeding.GetValue(     m_SpeciesIndex, rGenome )
                                + probs()->indoor_diepostfeeding.GetValue(       m_SpeciesIndex, rGenome )
                                + probs()->indoor_successfulfeed_human.GetValue( m_SpeciesIndex, rGenome );
            release_assert( prob_transmit > 0.0 );

            m_ContagionToDepositIndoor[ sgi ] += (float(numInfectiousBites) / prob_transmit);
        }
        else
        {
            if( m_ContagionToDepositOutdoor.count( sgi ) == 0 )
            {
                first_one = true;
                m_ContagionToDepositOutdoor[ sgi ] = 0.0f;
            }
            // See comment above GH-4251
            float prob_transmit = probs()->outdoor_dieduringfeeding.GetValue(     m_SpeciesIndex, rGenome )
                                + probs()->outdoor_diepostfeeding.GetValue(       m_SpeciesIndex, rGenome )
                                + probs()->outdoor_successfulfeed_human.GetValue( m_SpeciesIndex, rGenome );
            release_assert( prob_transmit > 0.0 );

            m_ContagionToDepositOutdoor[ sgi ] += (float(numInfectiousBites) / prob_transmit);
        }
        if( !first_one )
        {
            // if this is not the first one in the map, then we need to delete the object
            delete p_si;
        }
    }

    void VectorPopulation::DepositContagionHelper( TransmissionRoute::Enum route,
                                                   std::map<VectorPopulation::StrainGenomeId,float>& rContagionToDeposit )
    {
        for( auto& r_entry : rContagionToDeposit )
        {
            GeneticProbability gp_amount( m_SpeciesIndex,
                                          VectorGenome( r_entry.first.vectorGenomeBits ),
                                          r_entry.second );

            m_pNodeVector->DepositFromIndividual( *(r_entry.first.pStrainId), gp_amount, route );

            delete r_entry.first.pStrainId;
        }
        rContagionToDeposit.clear();
    }

    void VectorPopulation::DepositContagion()
    {
        DepositContagionHelper( TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR,
                                m_ContagionToDepositIndoor );

        DepositContagionHelper( TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR,
                                m_ContagionToDepositOutdoor );
    }

    uint32_t VectorPopulation::VectorToHumanTransmission( IVectorCohort* cohort,
                                                          const IStrainIdentity* pStrain,
                                                          uint32_t attemptFeed,
                                                          TransmissionRoute::Enum route )
    {
        release_assert( (route == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR ) ||
                        (route == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR) );

        uint32_t infectious_bites = 0;
        if( (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) && (attemptFeed > 0) )
        {
            // deposit indoor and outdoor contagion into vector-to-human group
            float modifier = GetDiseaseTransmissionModifier( cohort );
            float prob_transmission = species()->transmissionmod * modifier;

            infectious_bites = uint32_t( m_context->GetRng()->binomial_approx( attemptFeed, prob_transmission ) );

            release_assert( pStrain != nullptr );
            VectorToHumanDeposit( cohort->GetGenome(), *pStrain, infectious_bites, route);
        }
        return infectious_bites;
    }

    uint32_t VectorPopulation::CalculateHumanToVectorInfection( TransmissionRoute::Enum route,
                                                                IVectorCohort* cohort,
                                                                float probSuccessfulFeed,
                                                                uint32_t numHumanFeed )
    {
        uint32_t num_infected = 0;

        if( (cohort->GetState() == VectorStateEnum::STATE_ADULT) && (numHumanFeed > 0) )
        {
            GeneticProbability host_infectivity_gp = m_pNodeVector->GetTotalContagionGP( route );
            float host_infectivity = host_infectivity_gp.GetValue( m_SpeciesIndex, cohort->GetGenome() );

            float modifier = GetDiseaseAcquisitionModifier( cohort );

            float prob_infected = host_infectivity * modifier / probSuccessfulFeed;

            num_infected = uint32_t( m_context->GetRng()->binomial_approx( numHumanFeed, prob_infected ) );
        }

        return num_infected;
    }

    float VectorPopulation::GetDiseaseAcquisitionModifier( IVectorCohort* cohort )
    {
        //Wolbachia related impacts on infection susceptibility
        float from_wolbachia = 1.0;
        if( cohort->HasWolbachia() )
        {
            from_wolbachia = params()->WolbachiaInfectionModification;
        }

        int strain_index = cohort->GetGenome().GetMicrosporidiaStrainIndex();
        float duration = cohort->GetDurationOfMicrosporidia();
        float from_microsporidia = m_species_params->microsporidia_strains[ strain_index ]->disease_acquisition_modifier.getValueLinearInterpolation( duration, 1.0 );

        float from_genetic_trait = this->m_species_params->trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN, cohort->GetGenome() );

        float total_modifier = species()->acquiremod * from_wolbachia * from_microsporidia * from_genetic_trait;

        return total_modifier;
    }

    float VectorPopulation::GetDiseaseTransmissionModifier( IVectorCohort* cohort )
    {
        int strain_index = cohort->GetGenome().GetMicrosporidiaStrainIndex();
        float duration = cohort->GetDurationOfMicrosporidia();
        float from_microsporidia = m_species_params->microsporidia_strains[ strain_index ]->disease_transmission_modifier.getValueLinearInterpolation( duration, 1.0 );

        float from_genetic_trait = m_species_params->trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, cohort->GetGenome() );

        float total_modifier = from_microsporidia * from_genetic_trait;

        return total_modifier;
    }

    float VectorPopulation::CalculateEggBatchSize( IVectorCohort* cohort )
    {
        float fecundity_modifier = m_species_params->trait_modifiers.GetModifier( VectorTrait::FECUNDITY, cohort->GetGenome() );

        // Oocysts, not sporozoites affect egg batch size:
        // Hogg, J. C. and H. Hurd (1997). 
        //    "The effects of natural Plasmodium falciparum infection on the fecundity
        //    and mortality of Anopheles gambiae s. l. in north east Tanzania."
        // Parasitology 114(04): 325-331.
        float x_infectedeggbatchmod = (cohort->GetState() == VectorStateEnum::STATE_INFECTED) ? float( species()->infectedeggbatchmod ) : 1.0f;

        float egg_batch_size = species()->eggbatchsize * x_infectedeggbatchmod * fecundity_modifier;

        return egg_batch_size;
    }

    void VectorPopulation::StartGestating( uint32_t numFed, IVectorCohort* cohort )
    {
        num_fed_counter += numFed;

        // -----------------------------------------------------------------------------------------------------
        // --- Since the duration is likely to be a non-integer, we want to have some portion of the
        // --- vectors finish gestating on day1 (the truncated day) and the rest on day 2 (truncated day plus 1).
        // --- This should simulate how the individual vectors will randomly make this same decision.
        // -----------------------------------------------------------------------------------------------------
        float feeding_duration = GetFeedingCycleDuration();

        uint32_t day1 = uint32_t( feeding_duration );
        uint32_t day2 = day1 + 1;

        float feeding_day2_percent = feeding_duration - uint32_t( feeding_duration );

        uint32_t num_fed_day2 = m_context->GetRng()->binomial_approx( numFed, feeding_day2_percent );
        release_assert( numFed >= num_fed_day2 );
        uint32_t num_fed_day1 = numFed - num_fed_day2;

        release_assert( numFed == (num_fed_day1 + num_fed_day2) );
        release_assert( day1 != day2 );
        cohort->AddNewGestating( day1, num_fed_day1 );
        cohort->AddNewGestating( day2, num_fed_day2 );

        UpdateGestatingCount( cohort );
    }

    void VectorPopulation::AddEggsToLayingQueue( IVectorCohort* cohort, uint32_t numDoneGestating )
    {
        if( cohort->HasMated() )
        {
            float egg_batch_size = CalculateEggBatchSize( cohort );
            uint32_t num_eggs = uint32_t( egg_batch_size * float( numDoneGestating ) );

            VectorMatingGenome vmg( std::make_pair( cohort->GetGenome().GetBits(), cohort->GetMateGenome().GetBits() ) );
            neweggs += num_eggs;
            gender_mating_eggs[ vmg ] += num_eggs;

            LOG_DEBUG_F( "adding %d eggs to vector genome bits %lld-%lld.  current total=%d\n",
                         num_eggs,
                         cohort->GetGenome().GetBits(),
                         cohort->GetMateGenome().GetBits(),
                         gender_mating_eggs[ vmg ] );
        }
    }

    VectorPopulation::IndoorOutdoorResults 
    VectorPopulation::ProcessIndoorOutdoorFeeding( const VectorPopulation::IndoorOutdoorProbabilities& rProbs,
                                                   float probHumanFeed,
                                                   uint32_t attemptToFeed,
                                                   IVectorCohort* cohort,
                                                   TransmissionRoute::Enum routeHumanToVector,
                                                   TransmissionRoute::Enum routeVectorToHuman )
    {
        IndoorOutdoorResults results;

        if( attemptToFeed == 0 ) return results;

        uint32_t orig_attemptToFeed = attemptToFeed;

        // --------------------------------
        // --- Feeding Branch Subset
        // --- These probabilities are impacted by the individual-level interventions.
        // --- The results outside this method are impacted by the node-level interventions.
        // --- successful_feed_human shoule be 1.0 at this point.  Hence, all of those attempting
        // --- to feed should feed.
        // --------------------------------
        uint32_t ad_feed          = CalculatePortionInProbability( attemptToFeed, rProbs.successful_feed_ad    );
        uint32_t died_before_feed = CalculatePortionInProbability( attemptToFeed, rProbs.die_before_feeding    );
        uint32_t not_feed         = CalculatePortionInProbability( attemptToFeed, rProbs.not_available         );
        uint32_t died_during_feed = CalculatePortionInProbability( attemptToFeed, rProbs.die_during_feeding    );
        uint32_t died_after_feed  = CalculatePortionInProbability( attemptToFeed, rProbs.die_after_feeding     );
        uint32_t human_feed       = CalculatePortionInProbability( attemptToFeed, rProbs.successful_feed_human );

        num_attempt_but_not_feed += not_feed;

        static StrainIdentity fake_strain;

        results.num_died             = (died_before_feed + died_during_feed + died_after_feed);
        results.num_fed_ad           = ad_feed;
        results.num_fed_human        = human_feed;
        results.num_bites_total      = (died_during_feed + died_after_feed + human_feed); // used for human biting rate
        results.num_bites_infectious = VectorToHumanTransmission( cohort,
                                                                  &fake_strain,
                                                                  results.num_bites_total,
                                                                  routeVectorToHuman );

        // some successful feeds result in infected adults
        // NOTE: we only care about the vectors that survive
        results.num_infected = CalculateHumanToVectorInfection( routeHumanToVector,
                                                                cohort,
                                                                probHumanFeed,
                                                                results.num_fed_human );

        release_assert( orig_attemptToFeed >= (results.num_died + results.num_fed_ad + results.num_fed_human + not_feed) );
        release_assert( orig_attemptToFeed >= results.num_bites_total );
        release_assert( results.num_bites_total >= results.num_bites_infectious );
        release_assert( results.num_fed_human >= results.num_infected );

        return results;
    }

    uint32_t VectorPopulation::ProcessFeedingCycle( float dt, IVectorCohort* cohort )
    {
        if( cohort->GetPopulation() <= 0 )
            return 0;
        uint32_t population_before_deaths = cohort->GetPopulation();

        FeedingProbabilities feed_probs = CalculateFeedingProbabilities( dt, cohort );

        // -----------------------------------------------------------
        // --- Head of both Feeding and Non-Feeding Branches
        // --- Some portion of the total population should die due to
        // --- life expectancy, dry heat mortality, etc
        // -----------------------------------------------------------
        uint32_t died_local_mortality = cohort->AdjustGestatingForDeath( m_context->GetRng(), feed_probs.die_local_mortality, false ); // i.e. life expectancy

        AddEggsToLayingQueue( cohort, cohort->RemoveNumDoneGestating() );

        // ------------------------------------------------------------------
        // --- Non-Feeding Branch Continued
        // --- Of those that are NOT looking to feed, some might die due to
        // --- interventions that target non-feeding vectors
        // ------------------------------------------------------------------
        uint32_t died_without_feeding = cohort->AdjustGestatingForDeath( m_context->GetRng(), feed_probs.die_without_attempting_to_feed, true ); //contains ever day sugar trap killing

        // --------------------------------------------------------
        // --- Feeding Branch Continued
        // --- Now, we are only considering those vectors that are
        // --- looking to feed (i.e. a blood meal)
        // --------------------------------------------------------
        uint32_t looking_to_feed = cohort->GetNumLookingToFeed();
        num_looking_to_feed += looking_to_feed;
        num_gestating_begin += cohort->GetNumGestating();

        uint32_t died_sugar_feeding     = CalculatePortionInProbability( looking_to_feed, feed_probs.die_sugar_feeding               ); // every day/feed
        uint32_t died_before_feeding    = CalculatePortionInProbability( looking_to_feed, feed_probs.die_before_human_feeding        );
        uint32_t successful_animal_feed = CalculatePortionInProbability( looking_to_feed, feed_probs.successful_feed_animal          );
        uint32_t successful_AD_feed     = CalculatePortionInProbability( looking_to_feed, feed_probs.successful_feed_artifical_diet  );
        uint32_t attempt_feed_indoor    = CalculatePortionInProbability( looking_to_feed, feed_probs.successful_feed_attempt_indoor  );
        uint32_t attempt_feed_outdoor   = CalculatePortionInProbability( looking_to_feed, feed_probs.successful_feed_attempt_outdoor );
        LOG_VALID_F("looking_to_feed=%u , died_before_feeding=%u, successful_animal_feed=%u , attempt_feed_indoor=%u , attempt_feed_outdoor=%u  \n", 
            cohort->GetNumLookingToFeed(), 
            died_before_feeding,
            successful_animal_feed,
            attempt_feed_indoor,
            attempt_feed_outdoor);

        // NOTE: looking_to_feed could be non-zero at this point because
        // VectorProbabilites::survivewithoutsuccessfulfeed is non-zero
        // due to something like SpatialRepellent
        num_attempt_but_not_feed += looking_to_feed;
        num_attempt_feed_indoor  += attempt_feed_indoor;
        num_attempt_feed_outdoor += attempt_feed_outdoor;

        float prob_human_feed_indoor  = probs()->indoor_successfulfeed_human.GetValue( m_SpeciesIndex, cohort->GetGenome() );
        float prob_human_feed_outdoor = probs()->outdoor_successfulfeed_human.GetValue( m_SpeciesIndex, cohort->GetGenome() );

        // indoor feeds
        IndoorOutdoorResults indoor_results = ProcessIndoorOutdoorFeeding( feed_probs.indoor,
                                                                           prob_human_feed_indoor,
                                                                           attempt_feed_indoor, 
                                                                           cohort, 
                                                                           TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR,
                                                                           TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR );

        successful_AD_feed    += indoor_results.num_fed_ad;
        // should mosquito weight be in here?
        indoorbites           += float( indoor_results.num_bites_total );
        indoorinfectiousbites += float( indoor_results.num_bites_infectious );

        IndoorOutdoorResults outdoor_results = ProcessIndoorOutdoorFeeding( feed_probs.outdoor,
                                                                            prob_human_feed_outdoor,
                                                                            attempt_feed_outdoor, 
                                                                            cohort, 
                                                                            TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR,
                                                                            TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR );

        // should mosquito weight be in here?
        outdoorbites           += float( outdoor_results.num_bites_total );
        outdoorinfectiousbites += float( outdoor_results.num_bites_infectious );

        // cast to int32_t because LOG_VALID_F (varargs) was doing something weird with the unsigned integers
        LOG_VALID_F("Update Female Deaths cohort_id=%d, population_before_deaths=%d,  age=%f , died_before_feeding=%d, died_feed_related=%d microsporidia=%d \n",
                     int32_t(cohort->GetID()),
                     int32_t(population_before_deaths),
                     cohort->GetAge(),
                     int32_t(died_local_mortality + died_without_feeding + died_sugar_feeding + died_before_feeding),
                     int32_t(indoor_results.num_died + outdoor_results.num_died),
                     cohort->HasMicrosporidia() );
        // -------------------------
        // --- Update death counters
        // -------------------------
        dead_mosquitoes_before  += died_local_mortality + died_without_feeding + died_sugar_feeding + died_before_feeding;
        dead_mosquitoes_indoor  += indoor_results.num_died;
        dead_mosquitoes_outdoor += outdoor_results.num_died;

        uint32_t num_total_died = died_local_mortality
                                + died_without_feeding
                                + died_sugar_feeding
                                + died_before_feeding
                                + indoor_results.num_died
                                + outdoor_results.num_died;
        UpdateAgeAtDeath( cohort, num_total_died );

        // ---------------------
        // --- Adjust Population
        // ---------------------
        // died_local_mortality & died_without_feeding have already been subtracted from the population
        if( m_VectorMortality )
        {
            uint32_t num_died = died_sugar_feeding + died_before_feeding + indoor_results.num_died + outdoor_results.num_died;
            cohort->SetPopulation( cohort->GetPopulation() - num_died );
        }
        else
        {
            // When not killing the vectors, we want to reset the population to what it was before
            cohort->SetPopulation( cohort->GetPopulation() + died_local_mortality + died_without_feeding );
        }

        // ---------------------------------------------------------------------------------
        // --- Remove those that are infected so that the num_infected becomes a new cohort
        // ---------------------------------------------------------------------------------
        uint32_t num_infected = indoor_results.num_infected + outdoor_results.num_infected;
        indoor_results.num_fed_human  -= indoor_results.num_infected;
        outdoor_results.num_fed_human -= outdoor_results.num_infected;

        cohort->SetPopulation( cohort->GetPopulation() - num_infected );

        // -----------------------------------------
        // --- Start those that fed to be gestating
        // -----------------------------------------
        uint32_t successful_human_feed = indoor_results.num_fed_human + outdoor_results.num_fed_human;
        uint32_t num_fed = successful_human_feed + successful_AD_feed + successful_animal_feed;
        StartGestating( num_fed, cohort );

        return num_infected;
    }

    float VectorPopulation::GetFeedingCycleDuration() const
    {
        float mean_cycle_duration = species()->daysbetweenfeeds;
        if( species()->temperature_dependent_feeding_cycle != TemperatureDependentFeedingCycle::NO_TEMPERATURE_DEPENDENCE )
        {
            // Temperature-dependent gonotrophic cycle duration:
            // (1) Hoshen MB and Morse AP.
            //     "A weather-driven model of malaria transmission".
            //     Malaria Journal 2004, 3:32.
            // (2) Rua GL, Quinones ML, Velez ID, Zuluaga JS, Rojas W, Poveda G, Ruiz D.
            //     "Laboratory estimation of the effects of increasing temperatures on the duration of gonotrophic cycle of Anopheles albimanus (Diptera: Culicidae)"
            //     Mem Inst Oswaldo Cruz 2005, 100(5):515-20.

            float airtemp = m_context->GetLocalWeather()->airtemperature();

            // For VectorSpeciesParameters::daysbetweenfeeds = 3 days:
            //   Three-day feeding cycle at normal tropical temperatures (30 C)
            //   Colder -> longer cycles  (4 days at 24 C)
            //   Hotter -> shorter cycles (increasing fraction of 2-day cycles from 30 C to 48 C)

            // Whole temperature-dependent distribution can be shifted by changing
            // "Days_Between_Feeds" parameter, which is always the value at 30 degrees C.
            if ( species()->temperature_dependent_feeding_cycle == TemperatureDependentFeedingCycle::BOUNDED_DEPENDENCE)
            {
                mean_cycle_duration = (airtemp > 15) ? 1.0f + 37.0f * ( (species()->daysbetweenfeeds - 1.0f) / 2.0f ) / ( airtemp - 11.5f ) : 10.0f;
            }
            else if ( species()->temperature_dependent_feeding_cycle == TemperatureDependentFeedingCycle::ARRHENIUS_DEPENDENCE)
            {
                mean_cycle_duration = 1/( species()->cyclearrhenius1 * exp(-species()->cyclearrhenius2 / (airtemp + CELSIUS_TO_KELVIN)) );// * dt;  ( 4.090579e+10 * exp(-7.740230e+03 / (airtemp + CELSIUS_TO_KELVIN)) );
            }
            else
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                    "Unknown Temperature_Dependent_Feeding_Cycle in GetFeedingCycleDuration()",
                    species()->temperature_dependent_feeding_cycle,
                    TemperatureDependentFeedingCycle::pairs::lookup_key( species()->temperature_dependent_feeding_cycle ) );
            }

            LOG_VALID_F("Mean gonotrophic cycle duration = %0.5f days at %0.2f degrees C.\n", mean_cycle_duration, airtemp);
        }
        return mean_cycle_duration;
    }

    void VectorPopulation::Update_Immature_Queue( float dt )
    {
        for( auto cohort : ImmatureQueues )
        {
            // introduce climate dependence here if we can figure it out
            cohort->Update( m_context->GetRng(), dt, species()->trait_modifiers, dt * species()->immaturerate, cohort->HasMicrosporidia() );

            if( m_VectorMortality )
            {
                // calculate local mortality, includes outdoor area killling
                float p_local_mortality = GetLocalMatureMortalityProbability( dt, cohort );
                float outdoor_killing = probs()->outdoorareakilling.GetValue( m_SpeciesIndex, cohort->GetGenome() );
                p_local_mortality = p_local_mortality + (1.0f - p_local_mortality) * outdoor_killing;

                uint32_t die = uint32_t( m_context->GetRng()->binomial_approx( cohort->GetPopulation(), p_local_mortality ) );
                cohort->SetPopulation( cohort->GetPopulation() - die );
            }
            LOG_VALID_F("Update_Immature_Queue cohort_id=%u progress=%f sex=%d population=%u. \n",
                         cohort->GetID(), cohort->GetProgress(), cohort->GetGenome().GetGender(), cohort->GetPopulation());
        }
        // ----------------------------------------------------------------------
        // --- We need to add the males separately from the females so that
        // --- the number of males is constant while we are figuring out which
        // --- portion of females are mating with which males.
        // --- We could probably add the females first but it makes more sense for
        // --- the females to mate with the males that will be current at the
        // --- end of the timestep.
        // ----------------------------------------------------------------------
        CheckProgressionToAdultForMales();
        CheckProgressionToAdultForFemales();
    }

    void VectorPopulation::CheckProgressionToAdultForMales()
    {
        // Use the verbose "for" construct here because we may be modifying the list
        for( auto it = ImmatureQueues.begin(); it != ImmatureQueues.end(); ++it )
        {
            IVectorCohort* cohort = *it;

            if( cohort->GetGenome().GetGender() != VectorGender::VECTOR_MALE )
            {
                continue;
            }

            if( (cohort->GetProgress() >= 1) && (cohort->GetPopulation() > 0) )
            {
                ImmatureSugarTrapKilling( cohort );
                // creating new VectorCohortMale cohort instead of using the same cohort as before
                VectorCohortMale* male_cohort = VectorCohortMale::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                                cohort->GetAge(),
                                                                                cohort->GetProgress(),
                                                                                cohort->GetDurationOfMicrosporidia(),
                                                                                cohort->GetPopulation(),
                                                                                cohort->GetGenome(),
                                                                                cohort->GetSpeciesIndex() );

                queueIncrementTotalPopulation(male_cohort);
                MaleQueues.add(male_cohort, 0.0, true ); //adds cohort
                ImmatureQueues.remove( it );
                delete cohort; // deleting the cohort in which our males matured
            }
            else if( cohort->GetPopulation() <= 0 )
            {
                ImmatureQueues.remove( it );
                delete cohort;
            }
            else
            {
                queueIncrementTotalPopulation( cohort );//update counter
            }
        }
        ImmatureQueues.compact();
    }

    void VectorPopulation::CheckProgressionToAdultForFemales()
    {
        BuildMaleMatingCDF(true); // resetting the entire male population to "unmated"

        VectorGenome unmated;

        // Use the verbose "for" construct here because we may be modifying the list
        for( auto it = ImmatureQueues.begin(); it != ImmatureQueues.end(); ++it )
        {
            IVectorCohort* cohort = *it;

            if( cohort->GetGenome().GetGender() != VectorGender::VECTOR_FEMALE )
            {
                continue;
            }

            if( cohort->GetProgress() < 1 )
            {
                //Update counter if still immature or no males to mate with
                queueIncrementTotalPopulation( cohort );
            }
            else if( (cohort->GetPopulation() > 0) && (cohort->GetProgress() >= 1) )
            {
                ImmatureSugarTrapKilling( cohort );

                // Move as many females to ADULT as there are males to mate with
                AddAdultsAndMate( cohort, *pAdultQueues, true );

                if( cohort->GetPopulation() > 0 )
                {
                    AddAdultCohort( cohort,
                                    unmated,
                                    cohort->GetPopulation(),
                                    *pAdultQueues,
                                    true );
                }
            }

            if( cohort->GetPopulation() <= 0 )
            {
                ImmatureQueues.remove( it );
                delete cohort;
            }
        }
        ImmatureQueues.compact();
    }

    bool VectorPopulation::IsSugarTrapKillingEnabled()
    {
        VectorSugarFeeding::Enum vsf = species()->vector_sugar_feeding;

        bool is_sugar_trap_killing_enabled = ( (vsf == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_ON_EMERGENCE_ONLY) ||
                                               (vsf == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED       ) ||
                                               (vsf == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY        ) )
                                            && probs()->is_using_sugar_trap;

        return is_sugar_trap_killing_enabled;
    }

    float VectorPopulation::GetSugarTrapKilling( IVectorCohort* pCohort )
    {
        float sugar_trap_killing = 0.0;
        if( IsSugarTrapKillingEnabled() )
        {
            sugar_trap_killing = probs()->sugarTrapKilling.GetValue( m_SpeciesIndex, pCohort->GetGenome() );

        }
        return sugar_trap_killing;
    }

    void VectorPopulation::ImmatureSugarTrapKilling( IVectorCohort* pCohort )
    {
        // -------------------------------------------------------------------------------------------
        // --- now if sugar feeding exists every day or after each feed, and mortality is associated,
        // --- then check for killing
        // -------------------------------------------------------------------------------------------
        float sugar_trap_killing = GetSugarTrapKilling( pCohort );
        if( sugar_trap_killing > 0.0 )
        {
            uint32_t die = uint32_t( m_context->GetRng()->binomial_approx( pCohort->GetPopulation(), sugar_trap_killing ) );
            pCohort->SetPopulation( pCohort->GetPopulation() - die );
            if( pCohort->GetGenome().GetGender() == VectorGender::VECTOR_FEMALE )
            {
                dead_mosquitoes_before += die;
            }
        }
    }

    void VectorPopulation::BuildMaleMatingCDF( bool reset_population_to_unmated )
    {
        // ------------------------------------------------------------------------------
        // --- Setup the m_MaleMatingCDF so that there is one entry for each male cohort
        // --- and unmated_count_cdf is the cumulative count
        // ------------------------------------------------------------------------------
        m_MaleMatingCDF.clear();
        m_UnmatedMaleTotal = 0;
        for( auto p_icohort : this->MaleQueues )
        {
            VectorCohortMale* p_male_cohort = static_cast<VectorCohortMale*>( static_cast<VectorCohortAbstract*>( p_icohort ) );
            if ( reset_population_to_unmated )
            {
                // resets entire population to be unmated
                p_male_cohort->SetUnmatedCount( p_icohort->GetPopulation() );
            }
            if ( p_male_cohort->GetUnmatedCount() > 0 )
            {
                // avoids adding cohorts that have 0 unmated vectors
                m_UnmatedMaleTotal += p_male_cohort->GetUnmatedCount();
                p_male_cohort->SetUnmatedCountCDF( m_UnmatedMaleTotal );
                m_MaleMatingCDF.push_back( p_male_cohort );
            }
        }
    }

    void VectorPopulation::UpdateMaleMatingCDF( std::vector<VectorCohortMale*>::iterator selected_it )
    {
        // -----------------------------------------------------------------------
        // --- Deduct from the unmated_count since this cohort was selected.
        // --- This means that this cohort and all of the other ones that follow
        // --- need to have cumulative count deducted was well.
        // -----------------------------------------------------------------------
        uint32_t unmated_count = ( *selected_it )->GetUnmatedCount();
        release_assert( unmated_count > 0 );
        ( *selected_it )->SetUnmatedCount( unmated_count - 1 );

        while( selected_it != m_MaleMatingCDF.end() )
        {
            // setting VectorCohortMale.unmated_count_cdf -= 1 for all the subsequent CDFs
            ( *selected_it )->SetUnmatedCountCDF ( ( *selected_it )->GetUnmatedCountCDF() - 1);
            ++selected_it;
        }
        m_UnmatedMaleTotal -= 1;
    }

    std::vector<VectorCohortMale*>::iterator VectorPopulation::SelectMaleMatingCohort()
    {
        // ---------------------------------------------------------------------------------------
        // --- This must be called after BuildMaleMatingCDF() and after UpdateMaleMatingCDF()
        // ---------------------------------------------------------------------------------------
        release_assert( m_UnmatedMaleTotal > 0 );

        // ---------------------------------------------------------------
        // --- Select a male cohort based on the cumulative unmated count.
        // ---------------------------------------------------------------
        uint32_t ran = uint32_t( round( m_context->GetRng()->e() * float(m_UnmatedMaleTotal) ) );
        auto it = std::lower_bound(m_MaleMatingCDF.begin(), m_MaleMatingCDF.end(), ran, CompareMaleGenomeDist );
        release_assert( it != m_MaleMatingCDF.end() );

        // ------------------------------------------------------------------------------
        // --- If lower_bound() selects a cohort where everyone has already mated, then
        // --- find the next/nearby cohort that still has unmated males.
        // ------------------------------------------------------------------------------
        bool go_forward = ( *it )->GetUnmatedCountCDF() < m_UnmatedMaleTotal;
        while( ( *it )->GetUnmatedCount() == 0)
        {
            if( go_forward )
                ++it;
            else
                --it;
        }
        release_assert( it != m_MaleMatingCDF.end() );
        release_assert((*it)->GetUnmatedCount() > 0 );

        return it;
    }


    bool VectorPopulation::CompareMaleGenomeDist(const VectorCohortMale* rLeft, uint32_t val)
    {
        // -------------------------------------------------------------------------------------------------------
        // --- NOTE: std::lowerbound() needs this to be strictly less than.  If the input to lowerbound() is equal
        // --- to the max value, '<' allows this element to be chosen.
        // -------------------------------------------------------------------------------------------------------
        return rLeft->GetUnmatedCountCDF() < val;
    }

    void VectorPopulation::AddAdultsAndMate( IVectorCohort* pFemaleCohort,
                                             VectorCohortCollectionAbstract& rQueue,
                                             bool isNewAdult )
    {
        std::map< uint32_t, std::pair< VectorCohortMale*, uint32_t > > microsporidia_infected_males[ MAX_MICROSPORIDIA_STRAINS ];

        // 0 index no microsporidia, > 0 index implies has microsporidia
        GenomeCountMap_t genome_to_count_map[ MAX_MICROSPORIDIA_STRAINS ]; 

        uint32_t pop = pFemaleCohort->GetPopulation();
        std::vector<uint32_t> num_females_newly_infected_with_microsporidia( MAX_MICROSPORIDIA_STRAINS, 0 );

        for( uint32_t i = 0 ; (i < pop) && (m_UnmatedMaleTotal > 0); ++i )
        {
            auto it = SelectMaleMatingCohort(); // returns VectorCohortMale iterator
            VectorCohortMale* p_male_cohort = *it;
            release_assert( p_male_cohort->GetUnmatedCount() > 0);

            VectorGameteBitPair_t male_genome_bits = p_male_cohort->GetGenome().GetBits();

            int female_ms_strain_index = pFemaleCohort->GetGenome().GetMicrosporidiaStrainIndex();
            int male_ms_strain_index   = p_male_cohort->GetGenome().GetMicrosporidiaStrainIndex();

            bool female_has_microsporidia = pFemaleCohort->HasMicrosporidia();
            bool male_has_microsporidia   = p_male_cohort->HasMicrosporidia();

            if( male_has_microsporidia && !female_has_microsporidia )
            {
                // Male infecting Female
                float prob_transmission = m_species_params->microsporidia_strains[ male_ms_strain_index ]->male_to_female_transmission_probability;
                if( m_context->GetRng()->SmartDraw( prob_transmission ) )
                {
                    female_has_microsporidia = true;
                    female_ms_strain_index = male_ms_strain_index;
                    ++num_females_newly_infected_with_microsporidia[ male_ms_strain_index ];
                }
            }
            else if( !male_has_microsporidia && female_has_microsporidia )
            {
                // Female infecting Male
                float prob_transmission = m_species_params->microsporidia_strains[ female_ms_strain_index ]->female_to_male_transmission_probability;
                if( m_context->GetRng()->SmartDraw( prob_transmission ) )
                {
                    if( microsporidia_infected_males[ female_ms_strain_index ].count( p_male_cohort->GetID() ) == 0 )
                    {
                        microsporidia_infected_males[ female_ms_strain_index ][ p_male_cohort->GetID() ] = std::make_pair( p_male_cohort, uint32_t( 1 ) );
                    }
                    else
                    {
                        microsporidia_infected_males[female_ms_strain_index][ p_male_cohort->GetID() ].second += 1;
                    }

                    // -----------------------------------------------------------------------------------
                    // --- I'm having the female carry that the male was infected because it shows that
                    // --- both parents were/became infected.  I also think that if this tends towards all
                    // --- vectors getting infected, then this should cause the creation of fewer cohorts.
                    // --- This also helps with male-to-egg transmission
                    // -----------------------------------------------------------------------------------
                    VectorGenome male_genome = p_male_cohort->GetGenome();
                    male_genome.SetMicrosporidiaStrain( female_ms_strain_index );
                    male_genome_bits = male_genome.GetBits();
                }
            }

            // ---------------------------------------------------------------------------------------
            // --- Keep a count of which females mated with which males and were infected with which
            // --- strain of microsporidia.  We create cohorts later when we know how many are in each
            // --- cohort.  Should reduce the number of cohorts created.
            // ---------------------------------------------------------------------------------------
            if( genome_to_count_map[ female_ms_strain_index ].count(male_genome_bits) == 0 )
            {
                genome_to_count_map[ female_ms_strain_index ][ male_genome_bits ] = 0;
            }
            genome_to_count_map[ female_ms_strain_index ][ male_genome_bits ] += 1;

            UpdateMaleMatingCDF( it ); // keep CDF updated as we mate
        }

        // ---------------------------------------------------------------------
        // --- If some of the females got infected by the males, then split them
        // --- off into a new cohort.  If the input female cohort was infected
        // --- with microsporidia, then set the pointer to that cohort.
        // ---------------------------------------------------------------------
        std::vector<IVectorCohort*> female_cohort_list( MAX_MICROSPORIDIA_STRAINS, nullptr );
        for( int strain_index = 0; strain_index < MAX_MICROSPORIDIA_STRAINS; ++strain_index )
        {
            if( num_females_newly_infected_with_microsporidia[ strain_index ] == 0 )
            {
                female_cohort_list[ strain_index ] = pFemaleCohort;
            }
            else
            {
                female_cohort_list[ strain_index ] = pFemaleCohort->SplitNumber( m_context->GetRng(),
                                                                                 m_pNodeVector->GetNextVectorSuid().data,
                                                                                 num_females_newly_infected_with_microsporidia[ strain_index ] );
                release_assert( female_cohort_list[ strain_index ] != nullptr );
        
                female_cohort_list[ strain_index ]->InfectWithMicrosporidia( strain_index );
            }
        }
        
        // -----------------------------------------------------------------------------------------
        // --- Create new female, mated cohorts paring the female cohort with the right
        // --- collection of entries in genome_to_count_map. 0=no microsporidia, >0 = has microsporidia
        // --- If pFemaleCohort was infected with microsporidia, the genome_to_count_map[ 0 ] should
        // --- end up being empty. This seems like less code.
        // -----------------------------------------------------------------------------------------
        for( int strain_index = 0; strain_index < MAX_MICROSPORIDIA_STRAINS; ++strain_index )
        {
            IVectorCohort* p_cohort = female_cohort_list[ strain_index ];
            for( auto entry : genome_to_count_map[ strain_index ] )
            {
                AddAdultCohort( p_cohort,
                                VectorGenome(entry.first), //male genome
                                entry.second,
                                rQueue,
                                isNewAdult );
                if( p_cohort->GetPopulation() == 0 )
                {
                    break;
                }
            }
        }

        // ----------------------------------------------------------------------------
        // --- Delete the cohort that was created for the females that got infected
        // --- with microsporidia.  All of the vectors in the cohort should have mated
        // --- and should exist in a new cohort.
        // ----------------------------------------------------------------------------
        for( auto p_cohort : female_cohort_list )
        {
            if( (p_cohort != nullptr) && (p_cohort != pFemaleCohort) )
            {
                release_assert( p_cohort->GetPopulation() == 0 );
                delete p_cohort;
            }
        }
        female_cohort_list.clear();

        // -----------------------------------------------------------
        // --- Split off new Male cohorts infected with microsporidia
        // -----------------------------------------------------------
        for( int female_strain_index = 0; female_strain_index < MAX_MICROSPORIDIA_STRAINS; ++female_strain_index )
        {
            for( auto entry : microsporidia_infected_males[ female_strain_index ] )
            {
                release_assert( entry.first == entry.second.first->GetID() ); // making sure cohort is correctly under its ID
                VectorCohortMale* p_orig_cohort = entry.second.first; // vector cohort
                uint32_t           orig_unmated = p_orig_cohort->GetUnmatedCount();
                VectorCohortMale*  p_new_cohort = p_orig_cohort->SplitNumber( m_context->GetRng(),
                                                                              m_pNodeVector->GetNextVectorSuid().data,
                                                                              entry.second.second ); // num infected
                release_assert( p_new_cohort != nullptr );
                p_new_cohort->InfectWithMicrosporidia( female_strain_index );

                // ---------------------------------------------------------------
                // --- massaging unmated numbers in the cohorts:
                // --- the ones infected with microsporidia definitely all have mated 
                // --- if there were any unmated, all stay with original cohort
                // --- the unmated_count and unmated_count_cdf in the original cohort is up to date (see mating loop)
                // ---------------------------------------------------------------
                p_new_cohort->SetUnmatedCount( 0 ); // 
                p_orig_cohort->SetUnmatedCount( orig_unmated ); 
                release_assert( p_orig_cohort->GetPopulation() >= p_orig_cohort->GetUnmatedCount() );

                MaleQueues.add( p_new_cohort, 0.0, true );
            }
        }

        // -------------------------------------------------------------------------------------
        // --- I don't think we need to udpate the m_MaleMatingCDF because during this timestep
        // --- the newly infected male has just mated so can't mate again.
        // -------------------------------------------------------------------------------------
    }

    void VectorPopulation::AddAdultCohort( IVectorCohort* pFemaleCohort,
                                           const VectorGenome& rMaleGenome,
                                           uint32_t pop,
                                           VectorCohortCollectionAbstract& rQueue,
                                           bool isNewAdult )
    {
        IVectorCohort* temp_cohort = nullptr;
        if( isNewAdult )
        {
            temp_cohort = VectorCohort::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                      VectorStateEnum::STATE_ADULT,
                                                      0.0,
                                                      0.0,
                                                      pFemaleCohort->GetDurationOfMicrosporidia(),
                                                      pop,
                                                      pFemaleCohort->GetGenome(),
                                                      m_SpeciesIndex );
            release_assert( pFemaleCohort->GetPopulation() >= pop );
            pFemaleCohort->SetPopulation( pFemaleCohort->GetPopulation() - pop );
        }
        else
        {
            temp_cohort = pFemaleCohort->SplitNumber( m_context->GetRng(), m_pNodeVector->GetNextVectorSuid().data, pop );
            release_assert( temp_cohort != nullptr );
        }
        temp_cohort->SetMateGenome( rMaleGenome );
        if( isNewAdult )
        {
            queueIncrementTotalPopulation( temp_cohort );
            new_adults += temp_cohort->GetPopulation();
        }
        rQueue.add( temp_cohort, 0.0, true );
    }

    void VectorPopulation::AddReleasedCohort( VectorStateEnum::Enum state,
                                              const VectorGenome& rFemaleGenome,
                                              const VectorGenome& rMaleGenome,
                                              uint32_t pop )
    {
        if( pop == 0 ) return;

        float microsporidia_duration = 0.0;
        if( rFemaleGenome.HasMicrosporidia() )
        {
            // Set value so that microsporidia is fully mature
            microsporidia_duration = 1000.0;
        }

        VectorCohort* temp_cohort = VectorCohort::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                state,
                                                                0.0,
                                                                0.0,
                                                                microsporidia_duration,
                                                                pop,
                                                                rFemaleGenome,
                                                                m_SpeciesIndex );
        // if this is true, our vectors-to-be-released are mated and we're going to make them gestated
        if (VectorGender::VECTOR_MALE == rMaleGenome.GetGender())
        {
            temp_cohort->SetMateGenome(rMaleGenome);
            temp_cohort->AddNewGestating(1, pop); // setting them gestated and ready to lay
            if (params()->vector_aging)
            {
                //setting age to days-between-feeds because gestated (see AddInitialCohort)
                temp_cohort->SetAge(m_context->GetRng()->randomRound(GetFeedingCycleDuration())); 
            }
        }
        switch( state )
        {
            case VectorStateEnum::STATE_INFECTIOUS:
                m_ReleasedInfectious.push_back( temp_cohort );
                break;
            case VectorStateEnum::STATE_ADULT:
                m_ReleasedAdults.push_back( temp_cohort );
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "VectorStateEnum", state, EventType::pairs::lookup_key( state ) );
        }
    }

    void VectorPopulation::AddReleasedVectorsToQueues()
    {
        for( auto p_cohort : m_ReleasedInfectious )
        {
            new_adults += p_cohort->GetPopulation();

            InfectiousQueues.add( p_cohort, 0.0, true );
        }
        m_ReleasedInfectious.clear();

        for( auto p_cohort : m_ReleasedAdults )
        {
            new_adults += p_cohort->GetPopulation();

            pAdultQueues->add( p_cohort, 0.0, true );
        }
        m_ReleasedAdults.clear();

        for( auto p_cohort : m_ReleasedMales )
        {
            m_NeedToRefreshTheMatingCDF = true; // update m_MaleMatingCDF so newly-released males will mate same timestep as females
            MaleQueues.add( p_cohort, 0.0, true );
        }
        m_ReleasedMales.clear();
    }

    void VectorPopulation::Update_Larval_Queue( float dt )
    {
        // Use the verbose "for" construct here because we may be modifying the list
        for( auto it = LarvaQueues.begin(); it != LarvaQueues.end(); ++it )
        {
            IVectorCohort* cohort = *it;

            // Apply temperature and over-crowding dependent larval development
            cohort->Update( m_context->GetRng(), dt, species()->trait_modifiers, GetLarvalDevelopmentProgress(dt, cohort), cohort->HasMicrosporidia() );

            // Apply larval mortality, the probability of which may depend on over-crowding and Notre Dame instar-specific dynamics
            float p_larval_mortality = GetLarvalMortalityProbability(dt, cohort).GetValue( m_SpeciesIndex, cohort->GetGenome() );
            uint32_t nowPop = cohort->GetPopulation();
            uint32_t newPop = nowPop - uint32_t( m_context->GetRng()->binomial_approx( nowPop, p_larval_mortality ) );
            LOG_VALID_F( "Adjusting larval population from %d to %d based on overcrowding considerations, id = %d.\n", nowPop, newPop, cohort->GetID() );
            cohort->SetPopulation( newPop );

            queueIncrementTotalPopulation( cohort );

            if( (cohort->GetState() == VectorStateEnum::STATE_IMMATURE) || (cohort->GetPopulation() <= 0) )
            {
                LarvaQueues.remove( it );
                if ( cohort->GetPopulation() > 0)
                {
                    m_ProgressedLarvaeToImmatureCount  += cohort->GetPopulation();
                    m_ProgressedLarvaeToImmatureSumDur += cohort->GetAge()*cohort->GetPopulation();

                    // same issue as below with updating the larva in the habitat
                    cohort->ClearProgress();
                    cohort->SetAge( 0.0 );

                    ImmatureQueues.add( cohort, 0.0, false );
                    LOG_DEBUG_F("Immature adults emerging from larva queue: population = %d, genome bits = %lld, id = %d, age = %f , has microsporidia %d\n",
                                cohort->GetPopulation(), 
                                cohort->GetGenome().GetBits(), 
                                cohort->GetID(), cohort->GetAge(),
                                cohort->HasMicrosporidia());
                }
                else
                {
                    delete cohort;
                }
            }
            else
            {
                // ---------------------------------------------------------------------
                // --- I want to move ths into VectorCohort::Update(), but it depends
                // --- on both population AND progress.  The population also depends on
                // --- progress so unless you have the population calculation dependent
                // --- on the progress from last update or you move the population
                // --- calculation into VectorCohort, we are kind of stuck.
                // ---------------------------------------------------------------------
                // Pass back larva in this cohort to total count in habitat
                cohort->GetHabitat()->AddLarva( cohort->GetPopulation(), cohort->GetProgress() );
            }
        }
        LarvaQueues.compact();
    }

    float VectorPopulation::GetLarvalDevelopmentProgress(float dt, IVectorCohort* larva) const
    {
        // Get local larval growth modifier, which depends on larval crowding
        float local_larval_growth_mod = 1.0; 
        
        // if density dependent delay, slow growth
        if(!(params()->larval_density_dependence == LarvalDensityDependence::NO_DENSITY_DEPENDENCE ||
             params()->larval_density_dependence == LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY))
        {
            local_larval_growth_mod = larva->GetHabitat()->GetLocalLarvalGrowthModifier();
        }

        int strain_index = larva->GetGenome().GetMicrosporidiaStrainIndex();
        float microsporidia_modifier = m_species_params->microsporidia_strains[ strain_index ]->larval_growth_modifier;

        // Larval development is temperature dependent
        float temperature = m_context->GetLocalWeather()->airtemperature();

        // Craig, M. H., R. W. Snow, et al. (1999).
        // "A climate-based distribution model of malaria transmission in sub-Saharan Africa."
        // Parasitol Today 15(3): 105-111.
        float progress = ( species()->aquaticarrhenius1 * exp(-species()->aquaticarrhenius2 / (temperature + CELSIUS_TO_KELVIN)) ) * dt;
        progress *= local_larval_growth_mod;
        progress *= microsporidia_modifier;
        LOG_VALID_F( "%s returning %f based on temperature %f, and growth modifier %f, has microsporidia %d \n", __FUNCTION__, progress, temperature, local_larval_growth_mod, larva->HasMicrosporidia());

        return progress;
    }

    GeneticProbability VectorPopulation::GetLarvalMortalityProbability(float dt, IVectorCohort* larva) const
    {
        IVectorHabitat* habitat = larva->GetHabitat();

        // (1) Local larval mortality from larval competition in habitat
        // float larval_survival_weight = GetRelativeSurvivalWeight(habitat);
        float locallarvalmortality = habitat->GetLocalLarvalMortality(species()->aquaticmortalityrate, larva->GetProgress());

        // (2) Rainfall mortality
        float rainfallmortality = habitat->GetRainfallMortality();

        // (3) Artificial mortality (e.g. larvicides)
        // N.B. Larvicide mortality is modeled as instant and complete in treated habitat.
        //      May later add in survival of pupal stages, which are resistant to some larvicides.
        GeneticProbability artificialmortality = habitat->GetArtificialLarvalMortality();

        // Calculate total mortality probability from mortality rates by cause
        float larval_mortality = float(EXPCDF(-dt * (locallarvalmortality + rainfallmortality)));
        GeneticProbability p_larval_mortality = artificialmortality * (1.0f - larval_mortality) + larval_mortality;

        LOG_VALID_F( "%s returning %f based on local larval mortality (%f), rainfall mortality (%f), and artifical mortality (%f).\n",
                     __FUNCTION__, p_larval_mortality.GetDefaultValue(), locallarvalmortality, rainfallmortality, artificialmortality.GetDefaultValue() );

        return p_larval_mortality;
    }

    void VectorPopulation::Update_Egg_Laying( float dt )
    {
        // Get total larval capacity across habitats for calculation of fractional allocation of eggs
        float total_capacity = 0;
        for (auto habitat : (*m_larval_habitats))
        {
            total_capacity += habitat->GetCurrentLarvalCapacity();
        }

        // WARNING: How do we want to deal with the allocation of integer eggs to various cohorts (by genetic ID) and habitats
        //          when the numbers get small?  Rounding to the nearest integer or just rounding down are simple, but have the
        //          potential to add up to numbers less than the total number of new eggs, if fragmented into enough cohorts.
        //          Schemes that keep count of remaining eggs to give out, and adjust allocation accordingly, will always
        //          distribute the right number of eggs, but will have systematic biases in which genetic indices get the few eggs.
        //          At present, we will just round and live with the potential bias away from completely full larval habitat.

        // We are adding both genders of larva to the habitat so we need to add both genders of eggs to the habitat.
        uint32_t neweggs_both_genders = 2 * neweggs;

        // Loop over larval habitats
        for (auto habitat : (*m_larval_habitats))
        {
            float fractional_allocation = 0.0f;
            if( total_capacity != 0.0f )
            {
                fractional_allocation = habitat->GetCurrentLarvalCapacity() / total_capacity;
            }
            habitat->AddEggs( neweggs_both_genders * fractional_allocation );

            // Now lay each type of egg, laying male and female eggs, except for sterile ones
            for (auto& entry : gender_mating_eggs)
            {
                // Use the index of the egg map to construct the vector-genetics structure
                VectorMatingGenome parents_genome = entry.first;
                VectorGenome female_genome( parents_genome.first );
                VectorGenome male_genome( parents_genome.second );

                // If one of the parents was sterile, then the eggs are not viable
                if( (m_species_params->trait_modifiers.GetModifier( VectorTrait::STERILITY, female_genome ) == 0.0) ||
                    (m_species_params->trait_modifiers.GetModifier( VectorTrait::STERILITY, male_genome   ) == 0.0) )
                {
                    continue;
                }

                if( !AreWolbachiaCompatible( female_genome, male_genome ) )
                   continue;

                // Eggs laid in this habitat are scaled by fraction of available habitat here
                // TODO: revisit rounding issue described in warning above.
                uint32_t eggs_to_lay = uint32_t( float(entry.second) * fractional_allocation );
                if( eggs_to_lay == 0 )
                {
                    continue;
                }

                // -------------------------------------------------------------------------
                // --- The egg batch detailes to this point have all been for female eggs.
                // --- The fertilizer is generating both male and female genomes so we need
                // --- to double the number of eggs to include the male eggs.
                // -------------------------------------------------------------------------
                eggs_to_lay *= 2;

                GenomeCountPairVector_t fertilized_egg_list = m_Fertilizer.DetermineFertilizedEggs( m_context->GetRng(),
                                                                                                    female_genome,
                                                                                                    male_genome,
                                                                                                    eggs_to_lay );

                TransmitMicrosporidiaToEggs( female_genome, male_genome, fertilized_egg_list );

                for( auto fertilized_eggs : fertilized_egg_list )
                {
                    if( fertilized_eggs.count > 0 )
                    {
                        CreateEggCohortOfType( habitat, fertilized_eggs.genome, fertilized_eggs.count );
                    }
                }
            }
        }
    }

    void VectorPopulation::CreateEggCohortOfType(IVectorHabitat* habitat, const VectorGenome& rGenome, uint32_t eggs_to_lay )
    {
        IVectorCohort* cohort = VectorCohort::CreateCohort( habitat,
                                                            m_pNodeVector->GetNextVectorSuid().data,
                                                            VectorStateEnum::STATE_EGG,
                                                            0.0,
                                                            0.0,
                                                            eggs_to_lay,
                                                            rGenome,
                                                            m_SpeciesIndex );
        queueIncrementTotalPopulation( cohort );

        VectorGameteBitPair_t bits = cohort->GetGenome().GetBits();
        VectorHabitatType::Enum vht = habitat->GetVectorHabitatType();

        IVectorCohort* p_existing = m_EggCohortVectorOfMaps[ vht ][ bits ];
        if( p_existing != nullptr )
        {
            LOG_VALID_F( "Laying %d eggs into existing egg queue (bits=%s, habitat=%s).\n",
                         cohort->GetPopulation(),
                         species()->genes.GetGenomeName( rGenome ).c_str(),
                         VectorHabitatType::pairs::lookup_key( habitat->GetVectorHabitatType() ) );

            p_existing->SetPopulation( p_existing->GetPopulation() + cohort->GetPopulation() );
            delete cohort;
            cohort = nullptr;
        }
        else
        {
            LOG_VALID_F( "Laying %d eggs and pushing into new egg queue (bits=%s, habitat=%s).\n",
                         cohort->GetPopulation(),
                         species()->genes.GetGenomeName( rGenome ).c_str(),
                         VectorHabitatType::pairs::lookup_key( habitat->GetVectorHabitatType() ) );

            m_EggCohortVectorOfMaps[ vht ][ bits ] = cohort;
        }
    }

    void VectorPopulation::TransmitMicrosporidiaToEggs( const VectorGenome& rFemaleGenome,
                                                        const VectorGenome& rMaleGenome,
                                                        GenomeCountPairVector_t& rFertilizedEggList )
    {
        int female_strain_index = rFemaleGenome.GetMicrosporidiaStrainIndex();
        float female_to_egg_tran = m_species_params->microsporidia_strains[ female_strain_index ]->female_to_egg_transmission_probability;

        int male_strain_index = rMaleGenome.GetMicrosporidiaStrainIndex();
        float male_to_egg_tran = m_species_params->microsporidia_strains[ male_strain_index ]->male_to_egg_transmission_probability;

        float prob_transmission = 1.0 - (1.0 - female_to_egg_tran) * (1.0 - male_to_egg_tran);
        float fraction_female_strain = 1.0;
        if( (female_to_egg_tran + male_to_egg_tran) > 0.0 )
        {
            fraction_female_strain = female_to_egg_tran / (female_to_egg_tran + male_to_egg_tran);
        }

        if( prob_transmission > 0.0 )
        {
            int orig_num = rFertilizedEggList.size();
            for( int i = 0; i < orig_num; ++i )
            {

                uint32_t orig_count = rFertilizedEggList[ i ].count;
                uint32_t num_microporidia  = uint32_t( m_context->GetRng()->binomial_approx( orig_count, prob_transmission ) );
                if( num_microporidia > 0 )
                {
                    uint32_t num_female_strain = num_microporidia; // if fraction_female_strain == 1.0
                    if( fraction_female_strain == 0.0 )
                    {
                        num_female_strain = 0;
                    }
                    else if( fraction_female_strain < 1.0 )
                    {
                        num_female_strain = uint32_t( m_context->GetRng()->binomial_approx( num_microporidia, fraction_female_strain ) );
                        release_assert(  num_microporidia >= num_female_strain );
                    }
                    uint32_t num_male_strain = num_microporidia - num_female_strain;

                    if( num_female_strain > 0 )
                    {
                        release_assert( rFertilizedEggList[ i ].count >= num_female_strain );
                        rFertilizedEggList[ i ].count -= num_female_strain;

                        GenomeCountPair microsporidia_gcp( rFertilizedEggList[ i ].genome, num_female_strain );
                        microsporidia_gcp.genome.SetMicrosporidiaStrain( female_strain_index );
                        rFertilizedEggList.push_back( microsporidia_gcp );
                    }

                    if( num_male_strain > 0 )
                    {
                        release_assert( rFertilizedEggList[ i ].count >= num_male_strain );
                        rFertilizedEggList[ i ].count -= num_male_strain;

                        GenomeCountPair microsporidia_gcp( rFertilizedEggList[ i ].genome, num_male_strain );
                        microsporidia_gcp.genome.SetMicrosporidiaStrain( male_strain_index );
                        rFertilizedEggList.push_back( microsporidia_gcp );
                    }
                }
            }
        }
    }

    bool VectorPopulation::AreWolbachiaCompatible( const VectorGenome& rFemale, const VectorGenome& rMale) const
    {
        bool is_compatible = true;
        switch( rFemale.GetWolbachia() )
        {
            case VectorWolbachia::VECTOR_WOLBACHIA_FREE:
                if( rMale.GetWolbachia() != VectorWolbachia::VECTOR_WOLBACHIA_FREE )
                {
                    // Wolbachia-free female is cytoplasmically incompatible with any Wolbachia-infected male
                    is_compatible = false;
                }
                break;

            case VectorWolbachia::VECTOR_WOLBACHIA_A:
                if( (rMale.GetWolbachia() == VectorWolbachia::VECTOR_WOLBACHIA_B ) ||
                    (rMale.GetWolbachia() == VectorWolbachia::VECTOR_WOLBACHIA_AB) )
                {
                    // Female with Wolbachia A is incompatible to male with either B or AB
                    is_compatible = false;
                }
                break;

            case VectorWolbachia::VECTOR_WOLBACHIA_B:
                if( (rMale.GetWolbachia() == VectorWolbachia::VECTOR_WOLBACHIA_A ) ||
                    (rMale.GetWolbachia() == VectorWolbachia::VECTOR_WOLBACHIA_AB) )
                {
                    // Female with Wolbachia B is incompatible to male with either A or AB
                    is_compatible = false;
                }
                break;

            case VectorWolbachia::VECTOR_WOLBACHIA_AB:
                // Female with Wolbachia AB is compatible with any male
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "femaleWolbachia", rFemale.GetWolbachia(), VectorWolbachia::pairs::lookup_key( rFemale.GetWolbachia() ) );
        }

        // Otherwise, the mating is compatible
        return is_compatible;
    }

    void VectorPopulation::Update_Egg_Hatching( float dt )
    {
        // Calculate egg-hatch delay factor
        float eggHatchDelayFactor = dt;
        float localdensdephatchmod = 1.0;
        float temperature = m_context->GetLocalWeather()->airtemperature();

        // These options are mutually exclusive
        if(params()->temperature_dependent_egg_hatching)
        {
            float tempdephatch = ( params()->eggarrhenius1 * exp(-params()->eggarrhenius2 / (temperature + CELSIUS_TO_KELVIN)) );
            eggHatchDelayFactor *= tempdephatch;
        }
        else if( params()->egg_hatch_delay_dist != EggHatchDelayDist::NO_DELAY && params()->meanEggHatchDelay > 0 )
        {
            float egg_hatch_rate = 1.0f / params()->meanEggHatchDelay;
            eggHatchDelayFactor = EXPCDF(-egg_hatch_rate * dt);
        }
        // Extreme temperatures can cause this value to jump outside 0-1
        if( eggHatchDelayFactor < 0.0 ) eggHatchDelayFactor = 0.0;
        if( eggHatchDelayFactor > 1.0 ) eggHatchDelayFactor = 1.0;


        // Now process remaining eggs
        for( auto& r_egg_map : m_EggCohortVectorOfMaps )
        {
            for( auto& r_cohort_entry : r_egg_map )
            {
                if( r_cohort_entry.second == nullptr ) continue;

                IVectorCohort* cohort = r_cohort_entry.second;

                IVectorHabitat* habitat = cohort->GetHabitat();

                // Potential inter-species competitive weighting
                // float egg_survival_weight = GetRelativeSurvivalWeight(habitat);

                // Calculate egg-crowding correction for these eggs based on habitat and decrease population
                if (params()->delayed_hatching_when_habitat_dries_up)  // if delayed hatching is given, we need them to survive upon drought, thus only adjust population when there is water
                {
                    if( habitat->GetCurrentLarvalCapacity() >= 1 ) // no drought
                    {
                        NonNegativeFloat egg_crowding_correction = habitat->GetEggCrowdingCorrection();
                        uint32_t nowPop = cohort->GetPopulation();
                        uint32_t newPop = m_context->GetRng()->binomial_approx( nowPop, egg_crowding_correction );
                        LOG_VALID_F( "Updating egg population from %d to %d with egg_crowding_correction of %f\n",
                             nowPop, newPop, float( egg_crowding_correction ) );
                        cohort->SetPopulation( newPop );
                    }
                    // else do nothing (in case of drought, with the dhwhdr param set, don't reduce eggs with egg_crowding_correction)

                }
                else   // otherwise, use original 'anopheles implementation'
                {
                    NonNegativeFloat egg_crowding_correction = habitat->GetEggCrowdingCorrection();
                    uint32_t nowPop = cohort->GetPopulation();
                    uint32_t newPop = m_context->GetRng()->binomial_approx( nowPop, egg_crowding_correction );
                    LOG_VALID_F( "Updating egg population from %d to %d with egg_crowding_correction of %f\n",
                            nowPop, newPop, float( egg_crowding_correction ) );
                    cohort->SetPopulation( newPop );
                }

                // Include a daily egg mortality to prevent perfect hybernation
                if (params()->egg_mortality)
                {
                    uint32_t currPop = cohort->GetPopulation();
                    uint32_t newerPop = m_context->GetRng()->binomial_approx( currPop, species()->eggsurvivalrate );
                    cohort->SetPopulation( newerPop ); // (default 0.99 is based on Focks 1993)
                    LOG_VALID_F( "Updating egg population due to egg mortality: old_pop = %d, new_pop = %d.\n", currPop, newerPop );
                }

                float tmp_eggHatchDelayFactor = eggHatchDelayFactor;
                if( params()->delayed_hatching_when_habitat_dries_up &&
                    (habitat->GetCurrentLarvalCapacity() < 1) )
                {
                    LOG_VALID_F( "Multiplying eggHatchDelayFactor %f by drought delay factor from config %f\n", float( tmp_eggHatchDelayFactor ), params()->droughtEggHatchDelay );
                    tmp_eggHatchDelayFactor *= params()->droughtEggHatchDelay; //  (should be , default 1/3 is based on Focks 1993)
                }
                else if( (params()->egg_hatch_density_dependence == EggHatchDensityDependence::DENSITY_DEPENDENCE) &&
                         (habitat->GetTotalLarvaCount(PREVIOUS_TIME_STEP) > habitat->GetCurrentLarvalCapacity()) )
                {
                    // Get local density dependence hatching modifier, which depends on larval crowding
                    // if density dependent delay, slow growth
                    localdensdephatchmod = habitat->GetLocalLarvalGrowthModifier();
                    if (params()->delayed_hatching_when_habitat_dries_up)
                    {
                        localdensdephatchmod = max(params()->droughtEggHatchDelay,float(localdensdephatchmod));
                    }
                    LOG_VALID_F( "localdensdephatchmod set to %f due to egg_hatch_density_dependence = %s, larval CC = %f, larval growth mod = %f, and configurable dought egg hatch delay factor = %f.\n",
                                 float( localdensdephatchmod ),
                                 EggHatchDensityDependence::pairs::lookup_key( params()->egg_hatch_density_dependence ),
                                 float( habitat->GetCurrentLarvalCapacity() ),
                                 float( habitat->GetLocalLarvalGrowthModifier() ),
                                 params()->droughtEggHatchDelay
                               );
                }

                //}  NOTE: For now without if statement. Need to include a seperate parameter to address this.  
                float percent_hatched = float( tmp_eggHatchDelayFactor ) * float( localdensdephatchmod );
                uint32_t hatched = m_context->GetRng()->binomial_approx( cohort->GetPopulation(), percent_hatched );
                LOG_VALID_F( "temperature = %f, local density dependence modifier is %f, egg hatch delay factor is %f, current population is %d, hatched is %d, id = %d.\n",
                             float(temperature),
                             float(localdensdephatchmod),
                             float( tmp_eggHatchDelayFactor ),
                             cohort->GetPopulation(),
                             hatched,
                             cohort->GetID()
                           );

                if( hatched > 0 )
                {
                    IVectorCohort* temp_cohort = VectorCohort::CreateCohort( habitat,
                                                                             m_pNodeVector->GetNextVectorSuid().data,
                                                                             VectorStateEnum::STATE_LARVA,
                                                                             0.0,
                                                                             cohort->GetDurationOfMicrosporidia(),
                                                                             hatched,
                                                                             cohort->GetGenome(),
                                                                             m_SpeciesIndex );

                    LarvaQueues.add( temp_cohort, 0.0, false );
                    queueIncrementTotalPopulation( temp_cohort );
                    LOG_DEBUG_F("Hatching %d female eggs and pushing into larval queues (bits=%lld), id = %d , has microsporidia %d\n", hatched, cohort->GetGenome().GetBits(), temp_cohort->GetID(), cohort->HasMicrosporidia()  );

                    // Pass back larva in this cohort to total count in habitat
                    habitat->AddLarva( temp_cohort->GetPopulation(), temp_cohort->GetProgress() );
                }

                auto nowPop = cohort->GetPopulation();
                auto newPop = nowPop - hatched;
                LOG_VALID_F( "Updating egg population from %d to %d based on hatching of %d.\n", nowPop, newPop, hatched);
                cohort->SetPopulation( newPop );

                if(cohort->GetPopulation() <= 0)
                {
                    delete r_cohort_entry.second;
                    r_cohort_entry.second = nullptr;
                }
                else
                {
                    cohort->Update( m_context->GetRng(), dt, species()->trait_modifiers, 0.0, cohort->HasMicrosporidia() );
                }
            }
        }
    }

    void VectorPopulation::Update_Male_Queue( float dt )
    {
        MaleQueues.updateAge( dt );

        // Use the verbose "foreach" construct here because empty male cohorts (e.g. old vectors) will be removed
        for( auto it = MaleQueues.begin(); it != MaleQueues.end(); ++it )
        {
            IVectorCohort* cohort = *it;

            float p_local_male_mortality = GetLocalMatureMortalityProbability( dt, cohort ); // can be age dependent
            float p_outdoor_killing = probs()->outdoorareakilling.GetValue( m_SpeciesIndex, cohort->GetGenome() ); // SpaceSpraying
            float p_sugar_trap_killing = GetSugarTrapKilling( cohort );
            float p_outdoor_rest_killing = probs()->outdoorRestKilling.GetValue( m_SpeciesIndex, cohort->GetGenome() ); // OutdoorRestKill
            float p_node_emanator_killing = probs()->node_emanator_killing.GetValue( m_SpeciesIndex, cohort->GetGenome() ); // OutdoorNodeEmanator


            float p_male_mortality = p_local_male_mortality;
            p_male_mortality += (1.0f - p_male_mortality) * p_outdoor_killing;
            p_male_mortality += (1.0f - p_male_mortality) * p_sugar_trap_killing;
            p_male_mortality += (1.0f - p_male_mortality) * p_outdoor_rest_killing;
            p_male_mortality += (1.0f - p_male_mortality) * p_node_emanator_killing;

            // adults die
            uint32_t new_pop = cohort->GetPopulation();
            if( (new_pop > 0) && m_VectorMortality )
            {
                uint32_t dead_mosquitos = uint32_t( m_context->GetRng()->binomial_approx( new_pop, p_male_mortality ) );
                // cast to int32_t because LOG_VALID_F (varargs) was doing something weird with the unsigned integers
                LOG_VALID_F("Update Male Deaths cohort_id=%d , age=%f , population_before_deaths=%d , died=%d microsporidia=%d \n",
                             int32_t(cohort->GetID()),
                             cohort->GetAge(),
                             int32_t(new_pop),
                             int32_t(dead_mosquitos),
                             cohort->HasMicrosporidia());
                new_pop -= dead_mosquitos;
                UpdateAgeAtDeath( cohort, dead_mosquitos );
                cohort->SetPopulation(new_pop);
            }

            if( cohort->GetPopulation() <= 0 )
            {
                MaleQueues.remove( it );
                cohort->Release();
            }
            else
            {
                queueIncrementTotalPopulation( cohort );
            }
        }
        MaleQueues.compact();
    }

    void VectorPopulation::queueIncrementTotalPopulation( IVectorCohort* cohort )
    {
        if( cohort->GetPopulation() > 0 )
        {
            VectorStateEnum::Enum vector_state = cohort->GetState();
            VectorGameteBitPair_t bits = cohort->GetGenome().GetBits();

            state_counts[ vector_state ] += cohort->GetPopulation();
            genome_counts[ vector_state ][ bits ] += cohort->GetPopulation();

            int strain_index = cohort->GetGenome().GetMicrosporidiaStrainIndex();
            microsporidia_counts[ strain_index ][ vector_state ] += cohort->GetPopulation();
            release_assert( state_counts[ vector_state ] >= microsporidia_counts[ strain_index ][ vector_state ] );

            if( (vector_state == VectorStateEnum::STATE_ADULT     ) ||
                (vector_state == VectorStateEnum::STATE_INFECTED  ) ||
                (vector_state == VectorStateEnum::STATE_INFECTIOUS) )
            {
                VectorWolbachia::Enum wol = cohort->GetWolbachia();
                wolbachia_counts[ int(wol) ] += cohort->GetPopulation();
                if( !cohort->HasMated() )
                {
                    unmated_adults += cohort->GetPopulation();
                }
            }

            queueIncrementNumInfs( cohort );
        }
    }

    void VectorPopulation::queueIncrementNumInfs( IVectorCohort* cohort )
    {
        if( (cohort->GetState() == VectorStateEnum::STATE_INFECTED  ) ||
            (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) )
        {
            num_infs_per_state_counts[ cohort->GetState() ] += cohort->GetPopulation();
        }
    }


    void VectorPopulation::AddVectors( const VectorGenome& rGenome,
                                       const VectorGenome& rMateGenome,
                                       bool isFraction,
                                       uint32_t releasedNumber,
                                       float releasedFraction,
                                       float releasedInfectious )
    {
        uint32_t num_to_release = releasedNumber;

        // Insert into correct Male or Female list
        if( rGenome.GetGender() == VectorGender::VECTOR_FEMALE )
        {
            if( isFraction )
            {
                uint32_t pop = getCount( VectorStateEnum::STATE_ADULT )
                             + getCount( VectorStateEnum::STATE_INFECTED )
                             + getCount( VectorStateEnum::STATE_INFECTIOUS );
                num_to_release = uint32_t( float( pop ) * releasedFraction );
            }
            uint32_t num_infectious = uint32_t( float( num_to_release ) * releasedInfectious );
            num_to_release = (num_infectious >= num_to_release) ? 0 : (num_to_release - num_infectious);

            AddReleasedCohort( VectorStateEnum::STATE_ADULT,      rGenome, rMateGenome, num_to_release );
            AddReleasedCohort( VectorStateEnum::STATE_INFECTIOUS, rGenome, rMateGenome, num_infectious );
        }
        else
        {
            if( isFraction )
            {
                uint32_t pop = getCount( VectorStateEnum::STATE_MALE );
                num_to_release = uint32_t( float( pop ) * releasedFraction );
            }

            // progress is 1 since males should be at 1 from progressing from Immature to Male
            VectorCohortMale* tempentry = VectorCohortMale::CreateCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                          0.0,
                                                                          1.0,
                                                                          0.0,
                                                                          num_to_release,
                                                                          rGenome,
                                                                          m_SpeciesIndex );
            m_ReleasedMales.push_back( tempentry );
        }

        LOG_INFO_F( "We added %lu '%s' mosquitoes of type: %s\n",
                    num_to_release,
                    m_species_params->name.c_str(), 
                    VectorGender::pairs::lookup_key(rGenome.GetGender()) );
    }

    // Return a vector that contains a randomly order set of indexes that are from 0 to (N-1)
    std::vector<uint32_t> VectorPopulation::GetRandomIndexes( RANDOMBASE* pRNG, uint32_t N )
    {
        // Fill vector with indexes
        std::vector<uint32_t> selected_indexes;
        for( uint32_t index = 0 ; index < N ; ++index )
        {
            selected_indexes.push_back( index );
        }

        // Randomly shuffle the entries of the vector
        auto myran = [ pRNG ] ( int i ) { return pRNG->uniformZeroToN32( i ); };
        std::random_shuffle( selected_indexes.begin(), selected_indexes.end(), myran );

        return selected_indexes;
    }



    void VectorPopulation::SetupMigration( const std::string& idreference, 
                                           const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        m_pMigrationInfoVector = m_species_params->p_migration_factory->CreateMigrationInfoVector( idreference, 
                                                                                                   m_context, 
                                                                                                   rNodeIdSuidMap, 
                                                                                                   m_SpeciesIndex,
                                                                                                   &(m_species_params->genes) );
    }


    void VectorPopulation::Vector_Migration( float dt, VectorCohortVector_t* pMigratingQueue, bool migrate_males_only )
    {
        release_assert( m_pMigrationInfoVector );
        release_assert( pMigratingQueue );

        // setting up for male migration
        m_NeedToRefreshTheMatingCDF = false; // reset the bool, we will check if we need to refresh m_MaleMatingCDF at beginning of next timestep
        std::vector<suids::suid> reachable_nodes; // could be different per gender
        if( m_pMigrationInfoVector->MightTravel( VectorGender::VECTOR_MALE ) ) // checking if any of the queue might travel at all
        {
            reachable_nodes = m_pMigrationInfoVector->GetReachableNodes( Gender::MALE );
            auto non_male_migrating = pMigratingQueue->size();
            // Migrating males
            Vector_Migration_Queue( reachable_nodes, pMigratingQueue, MaleQueues );
            // if males are emmigrating, we'll need to refresh m_MaleMatingCDF next timestep
            if( pMigratingQueue->size() - non_male_migrating > 0 )
            {
                m_NeedToRefreshTheMatingCDF = true;
            }
        }
        
        // Migrating females
        // checking if any of the vectors might travel at all if non-genetics migration
        // we can do this before UpdateRates because if m_TotalRateFemale == 0 it will stay 0
        // UpdateRates does not change total migration rate, but, rather re-distributes where the vectors go
        //updating rates for females with the modifiers
        if (!migrate_males_only && m_pMigrationInfoVector->MightTravel( VectorGender::VECTOR_FEMALE ))
        {

            IVectorSimulationContext* p_vsc = nullptr;
            if( s_OK != m_context->GetParent()->QueryInterface( GET_IID( IVectorSimulationContext ), (void**)&p_vsc ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "m_context", "IVectorSimulationContext", "ISimulationContext" );
            }
            m_pMigrationInfoVector->UpdateRates( m_context->GetSuid(), get_SpeciesID(), p_vsc );
            reachable_nodes = m_pMigrationInfoVector->GetReachableNodes( Gender::FEMALE );
            Vector_Migration_Queue( reachable_nodes, pMigratingQueue, *pAdultQueues );
            Vector_Migration_Queue( reachable_nodes, pMigratingQueue, InfectedQueues );
            Vector_Migration_Queue( reachable_nodes, pMigratingQueue, InfectiousQueues );
        }
    }


    void VectorPopulation::Vector_Migration_Queue( const std::vector<suids::suid>& rReachableNodes,
                                                   VectorCohortVector_t* pMigratingQueue,
                                                   VectorCohortCollectionAbstract& rQueue )
    {

        for( auto it = rQueue.begin(); it != rQueue.end(); ++it )
        {
            const std::vector<float>* fraction_traveling = m_pMigrationInfoVector->GetFractionTraveling( *it );
            if( fraction_traveling == nullptr ) // returns nullptr when total rate of travel = 0
            {
                continue;
            }

            release_assert( fraction_traveling->size()  == rReachableNodes.size());
            std::vector<uint32_t> random_indexes = GetRandomIndexes( m_context->GetRng(), rReachableNodes.size() );

            for( uint32_t index : random_indexes )
            {
                suids::suid to_node  = rReachableNodes[ index ];
                if( to_node == m_context->GetSuid() )
                {
                    continue; // don't travel to the node you're already in
                }
                float percent_traveling = fraction_traveling->at( index );
                if( percent_traveling > 0.0f )
                {
                    IVectorCohort* p_traveling_vc = ( *it )->SplitPercent( m_context->GetRng(),
                                                                           m_pNodeVector->GetNextVectorSuid().data,
                                                                           percent_traveling );
                    if( p_traveling_vc == nullptr )
                    {
                        continue;
                    }
                    else if( p_traveling_vc->GetPopulation() == 0 )
                    {
                        delete p_traveling_vc;
                    }
                    else
                    {
                        IMigrate* emigre = p_traveling_vc->GetIMigrate();
                        MigrationType::Enum mig_type = MigrationType::LOCAL_MIGRATION; // always for vectors
                        emigre->SetMigrating( to_node, mig_type, 0.0, 0.0, false );
                        pMigratingQueue->push_back( p_traveling_vc );
                    }
                }
            }
        }

        for( auto it = rQueue.begin(); it != rQueue.end(); ++it )
        {
            IVectorCohort* p_vc = *it;
            if( p_vc->GetPopulation() <= 0 )
            {
                rQueue.remove( it );
                delete p_vc;
            }
        }
    }

    void VectorPopulation::AddImmigratingVector( IVectorCohort* pvc )
    {
        switch( pvc->GetState() )
        {
            case VectorStateEnum::STATE_ADULT:
                if( m_IsSortingVectors )
                    m_ImmigratingAdult.push_back( pvc );
                else
                    pAdultQueues->add( pvc, 0.0, true );
                break;

            case VectorStateEnum::STATE_INFECTED:
                if( m_IsSortingVectors )
                    m_ImmigratingInfected.push_back( pvc );
                else
                    InfectedQueues.add( pvc, GetInfectedProgress( pvc ), true );
                break;

            case VectorStateEnum::STATE_INFECTIOUS:
                if( m_IsSortingVectors )
                    m_ImmigratingInfectious.push_back( pvc );
                else
                    InfectiousQueues.add( pvc, 0.0, true );
                break;

            case VectorStateEnum::STATE_MALE:
                // males are immigrating, we'll need to refresh m_MaleMatingCDF next timestep
                m_NeedToRefreshTheMatingCDF = true;
                if (m_IsSortingVectors)
                    m_ImmigratingMale.push_back(pvc);
                else
                    MaleQueues.add(pvc, 0.0, true);

                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "IVectorCohort::GetState()", pvc->GetState(), VectorStateEnum::pairs::lookup_key( pvc->GetState() ) );
        }
    }

    void VectorPopulation::SetSortingVectors()
    {
        m_IsSortingVectors = true;
    }

    bool IdOrder( IVectorCohort* pLeft, IVectorCohort* pRight )
    {
        return pLeft->GetID() < pRight->GetID();
    }

    void VectorPopulation::SortImmigratingVectors()
    {
        release_assert( m_IsSortingVectors );

        std::sort( m_ImmigratingAdult.begin(),      m_ImmigratingAdult.end(),      IdOrder );
        std::sort( m_ImmigratingInfected.begin(),   m_ImmigratingInfected.end(),   IdOrder );
        std::sort( m_ImmigratingInfectious.begin(), m_ImmigratingInfectious.end(), IdOrder );
        std::sort( m_ImmigratingMale.begin(),       m_ImmigratingMale.end(),       IdOrder);

        for( auto pvc : m_ImmigratingAdult )
        {
            pAdultQueues->add( pvc, 0.0, true );
        }
        for( auto pvc : m_ImmigratingInfected )
        {
            InfectedQueues.add( pvc, GetInfectedProgress( pvc ), true );
        }
        for( auto pvc : m_ImmigratingInfectious )
        {
            InfectiousQueues.add( pvc, 0.0, true );
        }
        for (auto pvc : m_ImmigratingMale)
        {
            MaleQueues.add(pvc, 0.0, true);
        }
        m_ImmigratingAdult.clear();
        m_ImmigratingInfected.clear();
        m_ImmigratingInfectious.clear();
        m_ImmigratingMale.clear();
    }

    void VectorPopulation::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    {
        /* Strain-tracking exposure pattern only used in VectorPopulationIndividual at the moment */
    }

    float VectorPopulation::GetEIRByPool(VectorPoolIdEnum::Enum pool_id) const
    {
        switch(pool_id)
        {
        case VectorPoolIdEnum::INDOOR_VECTOR_POOL:
            return m_EIR_by_pool.first;

        case VectorPoolIdEnum::OUTDOOR_VECTOR_POOL:
            return m_EIR_by_pool.second;

        case VectorPoolIdEnum::BOTH_VECTOR_POOLS:
            return m_EIR_by_pool.first + m_EIR_by_pool.second;

        default:
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "VectorPopulation::GetEIRByPool is only valid for indoor/outdoor/combined biting." );
        }
    }

    float VectorPopulation::GetHBRByPool(VectorPoolIdEnum::Enum pool_id) const
    {
        switch(pool_id)
        {
        case VectorPoolIdEnum::INDOOR_VECTOR_POOL:
            return m_HBR_by_pool.first;

        case VectorPoolIdEnum::OUTDOOR_VECTOR_POOL:
            return m_HBR_by_pool.second;

        case VectorPoolIdEnum::BOTH_VECTOR_POOLS:
            return m_HBR_by_pool.first + m_HBR_by_pool.second;

        default:
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "VectorPopulation::GetHBRByPool is only valid for indoor/outdoor/combined biting." );
        }
    }

    float VectorPopulation::GetRelativeSurvivalWeight(VectorHabitat* habitat) const
    {
        float survival_weight = 1.0f;

#if 0
        // Modified for inter-species competitive advantage for egg-crowding and larval mortality in case of shared habitat
        if( params()->enable_vector_species_habitat_competition )
        {
            float max_capacity = habitat->GetMaximumLarvalCapacity();
            survival_weight = (max_capacity > 0) ? ( m_larval_capacities.at(habitat->GetVectorHabitatType()) / max_capacity ) : 0;
        }
#endif

        return survival_weight;
    }

    uint32_t VectorPopulation::getInfectedCount( IStrainIdentity* pStrain ) const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__,
                                          "Specific strains only used in individual vector model.\n" );
    }

    uint32_t VectorPopulation::getInfectiousCount( IStrainIdentity* pStrain ) const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__,
                                          "Specific strains only used in individual vector model.\n" );
    }

    uint32_t VectorPopulation::getCount( VectorStateEnum::Enum state ) const
    {
        return state_counts[ state ];
    }

    uint32_t VectorPopulation::getNumInfsCount( VectorStateEnum::Enum state ) const
    {
        return num_infs_per_state_counts[ state ];
    }

    uint32_t VectorPopulation::getGenomeCount( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const
    {
        VectorGameteBitPair_t bits = rGenome.GetBits();
        uint32_t count = 0;
        if( genome_counts[ state ].count( bits ) > 0 )
        {
            count = genome_counts[ state ].at( bits );
        }
        return count;
    }

    const std::vector<uint32_t>& VectorPopulation::getGestatingQueue() const { return num_gestating_queue; }
    uint32_t VectorPopulation::getIndoorBites()                 const { return indoorbites;             }
    uint32_t VectorPopulation::getInfectiousIndoorBites()       const { return indoorinfectiousbites;   }
    uint32_t VectorPopulation::getOutdoorBites()                const { return outdoorbites;            }
    uint32_t VectorPopulation::getInfectiousOutdoorBites()      const { return outdoorinfectiousbites;  }
    uint32_t VectorPopulation::getNumGestatingBegin()           const { return num_gestating_begin;     }
    uint32_t VectorPopulation::getNumGestatingEnd()             const { return num_gestating_end;       }
    uint32_t VectorPopulation::getNewEggsCount()                const { return neweggs;                 }
    uint32_t VectorPopulation::getNumLookingToFeed()            const { return num_looking_to_feed;     }
    uint32_t VectorPopulation::getNumFed()                      const { return num_fed_counter;         }
    uint32_t VectorPopulation::getNumAttemptFeedIndoor()        const { return num_attempt_feed_indoor; }
    uint32_t VectorPopulation::getNumAttemptFeedOutdoor()       const { return num_attempt_feed_outdoor;}
    uint32_t VectorPopulation::getNumAttemptButNotFeed()        const { return num_attempt_but_not_feed;}
    uint32_t VectorPopulation::getNewAdults()                   const { return new_adults;              }
    uint32_t VectorPopulation::getUnmatedAdults()               const { return unmated_adults;          }
    uint32_t VectorPopulation::getNumDiedBeforeFeeding()        const { return dead_mosquitoes_before;  }
    uint32_t VectorPopulation::getNumDiedDuringFeedingIndoor()  const { return dead_mosquitoes_indoor;  }
    uint32_t VectorPopulation::getNumDiedDuringFeedingOutdoor() const { return dead_mosquitoes_outdoor; }

    uint32_t VectorPopulation::getWolbachiaCount( VectorWolbachia::Enum wol ) const
    {
        return wolbachia_counts[ int(wol) ];
    }

    uint32_t VectorPopulation::getMicrosporidiaCount( int msStrainIndex, VectorStateEnum::Enum state ) const
    {
        return microsporidia_counts[ msStrainIndex ][ state ];
    }

    const std::string& VectorPopulation::get_SpeciesID() const { return m_species_params->name; }

    const VectorHabitatList_t& VectorPopulation::GetHabitats() const  { return (*m_larval_habitats); }

    GenomeNamePairVector_t VectorPopulation::GetPossibleGenomes() const
    {
        return m_Fertilizer.CreateAllPossibleGenomesWithNames();
    }

    const std::set<std::string>& VectorPopulation::GetPossibleAlleleNames() const
    {
        return m_species_params->genes.GetDefinedAlleleNames();
    }

    std::vector<PossibleGenome> VectorPopulation::GetAllPossibleGenomes( VectorGender::Enum gender ) const
    {
        GenomeNamePairVector_t all_gnpv = GetPossibleGenomes();

        // ----------------------------------------------------------------------
        // --- Pick out the genomes of the gender that we are collecting data on
        // ----------------------------------------------------------------------
        std::vector<PossibleGenome> all_possible_genomes;
        for( auto& gnp : all_gnpv )
        {
            if( (gender == VectorGender::VECTOR_BOTH_GENDERS) ||
                (gender == gnp.genome.GetGender()) )
            {
                all_possible_genomes.push_back( PossibleGenome( gnp ) );
            }
        }
        return all_possible_genomes;
    }

    bool PossibleGenomeCompare( const PossibleGenome& left, const PossibleGenome& right )
    {
        return left.genome < right.genome;
    }

    // Genomes are considered similar if for each locus (ignoring gender), the set of allele are
    // the same for each genome.  For example:
    //    1-1 & 1-1 = similar     - both genomes have 1
    //    1-0 & 0-1 = similar     - both genomes have 0 and 1
    //    2-0 & 0-1 = NOT similar - the right genome does not have 2 and the left does not have 1
    //    1-1 & 0-1 = NOT similar - the left genome does not have 0
    bool VectorPopulation::SimilarGenomes( uint8_t numGenes, const VectorGenome& rLeft, const VectorGenome& rRight ) const
    {
        for( uint8_t i = 0; i < numGenes; ++i )
        {
            std::pair<uint8_t, uint8_t> left_allele  = rLeft.GetLocus( i );
            std::pair<uint8_t, uint8_t> right_allele = rRight.GetLocus( i );

            // the allele in the left genome need to exist in the right genome to be similar
            if( (left_allele.first  != right_allele.first) && (left_allele.first  != right_allele.second) ) return false;
            if( (left_allele.second != right_allele.first) && (left_allele.second != right_allele.second) ) return false;

            // AND the allele in the right genome need to exist in the left genome to be similar
            if( (right_allele.first  != left_allele.first) && (right_allele.first  != left_allele.second) ) return false;
            if( (right_allele.second != left_allele.first) && (right_allele.second != left_allele.second) ) return false;
        }
        return true;
    }

    std::vector<PossibleGenome> VectorPopulation::CombinePossibleGenomes(
        bool combineSimilarGenomes,
        std::vector<PossibleGenome>& rPossibleGenomes ) const
    {
        if( !combineSimilarGenomes )
        {
            return rPossibleGenomes;
        }
        else
        {
            // sort the list because it helps to ensure the combining is
            // consistent during test
            std::sort( rPossibleGenomes.begin(), rPossibleGenomes.end(), PossibleGenomeCompare );

            int num_genes = GetNumGenes();
            std::vector<PossibleGenome> net_possible_genomes;

            for( auto& pg : rPossibleGenomes )
            {
                bool found = false;
                for( int i = 0; !found && (i < net_possible_genomes.size()); ++i )
                {
                    PossibleGenome& r_exist_gnp = net_possible_genomes[ i ];
                    if( SimilarGenomes( num_genes, pg.genome, r_exist_gnp.genome ) )
                    {
                        found = true;
                        r_exist_gnp.similar_genomes.push_back( pg.genome );
                    }
                }
                if( !found )
                {
                    net_possible_genomes.push_back( pg );
                }
            }
            return net_possible_genomes;
        }
    }

    std::vector<PossibleGenome> VectorPopulation::FindPossibleGenomes( VectorGender::Enum gender,
                                                                       bool combineSimilarGenomes ) const
    {
        std::vector<PossibleGenome> all_possible_genomes = GetAllPossibleGenomes( gender );
        std::vector<PossibleGenome> net_possible_genomes = CombinePossibleGenomes( combineSimilarGenomes,
                                                                                   all_possible_genomes );
        return net_possible_genomes;
    }

    uint8_t VectorPopulation::GetNumGenes() const
    {
        return m_species_params->genes.Size();
    }

    uint8_t VectorPopulation::GetLocusIndex( const std::string& rAlleleName ) const
    {
        return m_species_params->genes.GetLocusIndex( rAlleleName );
    }

    void VectorPopulation::ConvertAlleleCombinationsStrings( const std::string& rParameterName,
                                                             const std::vector<std::vector<std::string>>& rComboStrings,
                                                             GenomeNamePairVector_t* pPossibleGenomes ) const
    {
        VectorGameteBitPair_t bit_mask = 0;
        std::vector<VectorGameteBitPair_t> vgbp_genomes;

        m_species_params->genes.ConvertAlleleCombinationsStrings( rParameterName,
                                                                  rComboStrings,
                                                                  &bit_mask,
                                                                  &vgbp_genomes );

        for( auto vgbp : vgbp_genomes )
        {
            VectorGenome genome( vgbp );
            std::string name = m_species_params->genes.GetGenomeName( genome );
            pPossibleGenomes->push_back( GenomeNamePair( genome, name ) );
        }
    }

    uint32_t VectorPopulation::getDeathCount( VectorStateEnum::Enum state ) const
    {
        return m_AgeAtDeathByState[ state ].count;
    }

    uint32_t VectorPopulation::getDeathCount( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const
    {
        VectorGameteBitPair_t bits = rGenome.GetBits();
        uint32_t count = 0;
        if( m_AgeAtDeathByGenomeByState[ state ].count( bits ) > 0 )
        {
            count = m_AgeAtDeathByGenomeByState[ state ].at( bits ).count;
        }
        return count;
    }

    float VectorPopulation::getSumAgeAtDeath( VectorStateEnum::Enum state ) const
    {
        return m_AgeAtDeathByState[ state ].sum_age_days;
    }

    float VectorPopulation::getSumAgeAtDeath( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const
    {
        VectorGameteBitPair_t bits = rGenome.GetBits();
        float sum_age = 0.0;
        if( m_AgeAtDeathByGenomeByState[ state ].count( bits ) > 0 )
        {
            sum_age = m_AgeAtDeathByGenomeByState[ state ].at( bits ).sum_age_days;
        }
        return sum_age;
    }

    std::vector<std::string> VectorPopulation::GetMicrosporidiaStrainNames() const
    {
        std::vector<std::string> strain_name_list;
        for( auto& strain_name : species()->microsporidia_strains.GetStrainNames() )
        {
            strain_name_list.push_back( strain_name );
        }
        return strain_name_list;
    }

    uint32_t VectorPopulation::getProgressFromLarvaeToImmatureNum() const
    {
        return m_ProgressedLarvaeToImmatureCount;
    }

    float    VectorPopulation::getProgressFromLarvaeToImmatureSumDuration() const
    {
        return m_ProgressedLarvaeToImmatureSumDur;
    }

    void VectorPopulation::SetContextTo(INodeContext *context)
    {
        m_context = context;
        
        release_assert( m_SpeciesIndex >= 0 );

        m_vector_params = GET_CONFIGURABLE( SimulationConfig )->vector_params;
        if( m_vector_params->vector_species.Size() == 0 )
        {
            std::ostringstream msg;
            msg << "There is a vector population in the simulation but no vector species parameters could be loaded from the configuration.\n";
            msg << "Please use parameter 'Vector_Species_Params' in the configuration to define the vector species parameters.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( m_SpeciesIndex >= m_vector_params->vector_species.Size() )
        {
            std::ostringstream msg;
            msg << "There are more vector populations in the simulation than vector species parameters defined in the configuration.\n";
            msg << "Please use parameter 'Vector_Species_Params' in the configuration to define more vector species.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        m_species_params = m_vector_params->vector_species[ m_SpeciesIndex ];

        for( int strain_index = 0; strain_index < m_species_params->microsporidia_strains.Size(); ++strain_index )
        {
            std::vector<uint32_t> count_by_state( VectorStateEnum::pairs::count() , 0 );
            microsporidia_counts.push_back( count_by_state );
        }

        // Query for vector node context
        IVectorNodeContext* ivnc = nullptr;
        if( s_OK != context->QueryInterface( GET_IID( IVectorNodeContext ), (void**)&ivnc ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorNodeContext", "INodeContext" );
        }

        // Create larval habitat objects for this species 
        // and keep a list of pointers so the node can update it with new rainfall, etc.
        // Larval habitats can exist already when a serialized population was loaded.
        m_larval_habitats = ivnc->GetVectorHabitatsBySpecies( m_species_params->name );
        if( m_larval_habitats->size() == 0 )
        {
            SetupLarvalHabitat( m_context );
        }

        if( s_OK != m_context->QueryInterface( GET_IID( INodeVector ), (void**)&m_pNodeVector ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "m_context", "INodeVector", "INodeContext" );
        }

        InfectiousQueues.initialize( m_pNodeVector, true ); // need INodeVector for migration
        InfectedQueues.initialize(   m_pNodeVector, true ); // need INodeVector for migration
        pAdultQueues->initialize(    m_pNodeVector, true ); // need INodeVector for migration
        MaleQueues.initialize(       m_pNodeVector, true ); // need INodeVector for migration
        ImmatureQueues.initialize(   nullptr,       params()->vector_aging );
        LarvaQueues.initialize(      nullptr,       params()->vector_aging );
        //EggQueues - not a VectorCohortCollection

        m_Fertilizer.Initialize( &(m_species_params->genes), &(m_species_params->trait_modifiers), &(m_species_params->gene_drivers), &( m_species_params->maternal_deposition ) );

        // Set pointer to shared vector lifecycle probabilities container.
        // The species-independent probabilities, e.g. dependent on individual-human vector-control interventions, are updated once by the node
        // The species-specific probabilities are overwritten when FinalizeTransitionProbabilites is called in Update_Lifecycle_Probabilities
        m_probabilities  = ivnc->GetVectorLifecycleProbabilities();

        // -------------------------------------------------------------------------------------
        // --- If EggQueues or LarvaQueues are not empty, then we get here due to serialization
        // --- (VectorPopulation's don't migrate).  Hence, we need to replace the VectorHabitat
        // --- objects with the ones owned by NodeVector.  SetHabitat() will delete the copies
        // --- owned by VectorCohortWtihHabitat which were created for each cohort during
        // --- the deserialization process.
        // -------------------------------------------------------------------------------------

        // Rewire the habitat objects.  EggQueues is a temporary variable used during serialization.
        // Put objects in EggQueues into map
        for( auto cohort : EggQueues )
        {
            if( cohort != nullptr ) // older code serialized null cohorts
            {
                cohort->SetHabitat( ivnc->GetVectorHabitatBySpeciesAndType( m_species_params->name, cohort->GetHabitatType(), nullptr ) );

                VectorGameteBitPair_t bits = cohort->GetGenome().GetBits();
                VectorHabitatType::Enum vht = cohort->GetHabitat()->GetVectorHabitatType();
                m_EggCohortVectorOfMaps[ vht ][ bits ] = cohort;
            }
        }
        EggQueues.clear();

        // For each cohort in LarvaQueues, look at it's habitat type and (re)wire it to the correct habitat instance.
        for ( auto cohort : LarvaQueues )
        {
            cohort->SetHabitat( ivnc->GetVectorHabitatBySpeciesAndType( m_species_params->name, cohort->GetHabitatType(), nullptr ) );
        }

        if( m_pMigrationInfoVector != nullptr )
        {
            m_pMigrationInfoVector->SetContextTo( context );
        }
    }

    std::vector<uint32_t> VectorPopulation::GetNewlyInfectedVectorIds() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Unique mosquito IDs only used in individual vector model.\n" );
    }

    std::vector<uint32_t> VectorPopulation::GetInfectiousVectorIds() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Unique mosquito IDs only used in individual vector model.\n" );
    }

    std::map<uint32_t, uint32_t> VectorPopulation::getNumInfectiousByCohort() const
    {
        std::map<uint32_t, uint32_t> infectious_counts;
        for (auto cohort : InfectiousQueues)
        {
            uint32_t index = cohort->GetID();
            uint32_t count = cohort->GetPopulation();
            infectious_counts[index] = count;
        }
        return infectious_counts;
    }

    const infection_list_t& VectorPopulation::GetInfections() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only supported in individual vector model." );
    }

    float VectorPopulation::GetInterventionReducedAcquire() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only supported in individual vector model." );
    }

    void VectorPopulation::UpdateAgeAtDeath( IVectorCohort* pCohort, uint32_t numDied )
    {
        VectorStateEnum::Enum state = pCohort->GetState();
        float sum_age = float( numDied ) * pCohort->GetAge();
        VectorGameteBitPair_t bits = pCohort->GetGenome().GetBits();

        m_AgeAtDeathByState[ state ].count        += numDied;
        m_AgeAtDeathByState[ state ].sum_age_days += sum_age;

        m_AgeAtDeathByGenomeByState[ state ][ bits ].count        += numDied;
        m_AgeAtDeathByGenomeByState[ state ][ bits ].sum_age_days += sum_age;
    }

    suids::suid VectorPopulation::GetSuid() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Not supported in VECTOR_SIM" );
    }

    REGISTER_SERIALIZABLE(VectorPopulation);

    void VectorPopulation::serialize(IArchive& ar, VectorPopulation* obj)
    {
        VectorPopulation& population = *obj;
//        ar.labelElement("m_larval_habitats") & population.m_larval_habitats;  // NodeVector owns this information.
        ar.labelElement("dryheatmortality") & population.dryheatmortality;
        ar.labelElement("infectiouscorrection") & population.infectiouscorrection;
        ar.labelElement("indoorinfectiousbites") & population.indoorinfectiousbites;
        ar.labelElement("outdoorinfectiousbites") & population.outdoorinfectiousbites;
        ar.labelElement("indoorbites") & population.indoorbites;
        ar.labelElement("outdoorbites") & population.outdoorbites;
        ar.labelElement("m_EIR_by_pool"); Kernel::serialize(ar, population.m_EIR_by_pool);
        ar.labelElement("m_HBR_by_pool"); Kernel::serialize(ar, population.m_HBR_by_pool);

        // These get cleared at the beginning of UpdateVectorPopulation() so they do not need to be saved
        //ar.labelElement("gender_mating_eggs"); Kernel::serialize(ar, population.gender_mating_eggs);
        //ar.labelElement("neweggs") & population.neweggs;
        //ar.labelElement("genome_counts") & population.genome_counts;
        //ar.labelElement("wolbachia_counts") & population.wolbachia_counts;
        //ar.labelElement("microsporidia_counts") & population.microsporidia_counts;
        //ar.labelElement( "m_AgeAtDeathByState" ) & population.m_AgeAtDeathByState;
        //ar.labelElement( "m_AgeAtDeathByGenomeByState" ) & population.m_AgeAtDeathByGenomeByState;
        //ar.labelElement( "m_ProgressedLarvaeToImmatureCount" ) & population.m_ProgressedLarvaeToImmatureCount;
        //ar.labelElement( "m_ProgressedLarvaeToImmatureSumDur" ) & population.m_ProgressedLarvaeToImmatureSumDur;

        // Adding state_counts so that the check in SimulationVector for
        // if the node is initialized with vectors works as desired.
        ar.labelElement("state_counts") & population.state_counts;

        if( ar.IsWriter() )
        {
            population.EggQueues.clear();
            for( auto& r_egg_map : population.m_EggCohortVectorOfMaps )
            {
                for( auto entry : r_egg_map )
                {
                    // ------------------------------------------------------------------------------
                    // --- The maps are allowed to have nullptr entries because we don't want to be
                    // --- constantly changing the size of the map.  However, we don't want to
                    // --- serialize them.
                    // ------------------------------------------------------------------------------
                    if( entry.second != nullptr )
                    {
                        population.EggQueues.push_back( entry.second );
                    }
                }
            }
        }
        // -----------------------------------------------------------------------------
        // --- else the reader will put the data into EggQueues and SetContextTo() will
        // --- put it into m_EggCohortVectorOfMaps after the habitats get re-wired
        // -----------------------------------------------------------------------------

        std::string species_name;
        if( ar.IsWriter() )
        {
            release_assert( population.m_species_params != nullptr );
            species_name = population.m_species_params->name;
        }
        ar.labelElement( "species_name" ) & species_name;
        ar.labelElement( "m_SpeciesIndex" ) & population.m_SpeciesIndex;

        ar.labelElement("EggQueues") & population.EggQueues;
        ar.labelElement("LarvaQueues") & population.LarvaQueues;
        ar.labelElement("ImmatureQueues") & population.ImmatureQueues;
        ar.labelElement("AdultQueues") & *(population.pAdultQueues);
        ar.labelElement("InfectedQueues") & population.InfectedQueues;
        ar.labelElement("InfectiousQueues") & population.InfectiousQueues;
        ar.labelElement("MaleQueues") & population.MaleQueues;
        ar.labelElement("m_VectorMortality") & population.m_VectorMortality;

        // --------------------------------------------------------------------------
        // --- If reading a serialized file, we will update/refresh the 
        // --- m_UnmatedMaleTotal and m_MaleMatingCDF() from serialized unmated counts 
        // --- from MaleQueues 
        // --------------------------------------------------------------------------
        if( ar.IsReader() )
        {
            population.m_NeedToRefreshTheMatingCDF = true;
        }

        // Probabilities will be set in SetContextTo()
        // ar.labelElement("m_probabilities"); VectorProbabilities::serialize(ar, population.m_probabilities);

        // do not need to serialize these since they are updated each time step before they are used
        //m_LocalMortalityProbabilityTable()
        //m_DefaultLocalMortalityProbability( 1.0 )
    }

    void serialize(IArchive& ar, std::pair<float, float>& pair)
    {
        ar.startObject();
            ar.labelElement("first") & pair.first;
            ar.labelElement("second") & pair.second;
        ar.endObject();
    }

    void serialize(IArchive& ar, std::map<uint32_t, int>& mapping)
    {
        size_t count = ar.IsWriter() ? mapping.size() : -1;
        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto entry : mapping)
            {
                ar.startObject();
                    ar.labelElement("key") & (uint32_t&)entry.first;
                    ar.labelElement("value") & entry.second;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                uint32_t key;
                int value;
                ar.startObject();
                    ar.labelElement("key") & key;
                    ar.labelElement("value") & value;
                ar.endObject();
                mapping[key] = value;
            }
        }
        ar.endArray();
    }
}
