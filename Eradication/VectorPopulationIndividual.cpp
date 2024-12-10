
#include "stdafx.h"
#include "SimulationConfig.h"
#include "VectorPopulationIndividual.h"
#include "VectorCohortIndividual.h"
#include "Vector.h"
#include "IContagionPopulation.h"
#include "StrainIdentity.h"
#include "INodeContext.h"
#include "Climate.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"
#include "IMigrationInfoVector.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "RANDOM.h"
#include "TransmissionGroupMembership.h"
#include "IContagionPopulationGP.h"
#include "ISimulationContext.h"
#include "VectorToHumanAdapter.h"
#include "NodeEventContext.h"
#include "BroadcasterObserver.h"

SETUP_LOGGING("VectorPopulationIndividual")

// VectorPopulationIndividual handles treatment of Vector population as an ensemble
// of individual vectors, although it is possible to run the code with each vector
// entry representing multiple real world vectors

// All vectors are in AdultQueues, the others are not used

namespace Kernel
{
    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorPopulationIndividual, VectorPopulation)
    END_QUERY_INTERFACE_DERIVED(VectorPopulationIndividual, VectorPopulation)

    VectorPopulationIndividual::VectorPopulationIndividual(uint32_t mosquito_weight)
        : VectorPopulation()
        , m_mosquito_weight(mosquito_weight)
        , m_average_oviposition_killing(0.0f)
        , current_vci(nullptr)
    {
        delete pAdultQueues;
        pAdultQueues = new VectorCohortCollectionStdVector();
    }

    void VectorPopulationIndividual::AddInitialFemaleCohort( const VectorGenome& rGenomeFemale,
                                                             const VectorGenome& rGenomeMate,
                                                             uint32_t num )
    {
        uint32_t adjusted_population = num / m_mosquito_weight;
        for (uint32_t i = 0; i < adjusted_population; i++)
        {
            VectorCohortIndividual* pvci = CreateAdultCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                              VectorStateEnum::STATE_ADULT,
                                                              0.0,
                                                              0.0,
                                                              0.0,
                                                              m_mosquito_weight,
                                                              rGenomeFemale,
                                                              m_SpeciesIndex );
            pvci->SetMateGenome(rGenomeMate);

            RandomlySetOvipositionTimer(pvci);

            pAdultQueues->add( pvci, 0.0, false );
            queueIncrementTotalPopulation( pvci );
        }
    }

    VectorPopulationIndividual *VectorPopulationIndividual::CreatePopulation( INodeContext *context,
                                                                              int speciesIndex,
                                                                              uint32_t adult,
                                                                              uint32_t mosquito_weight)
    {
        VectorPopulationIndividual *newpopulation = _new_ VectorPopulationIndividual(mosquito_weight);
        newpopulation->Initialize(context, speciesIndex, adult);

        return newpopulation;
    }

    VectorPopulationIndividual::~VectorPopulationIndividual()
    {
        VectorCohortIndividual::DeleteSupply();
    }

    VectorCohortIndividual *VectorPopulationIndividual::CreateAdultCohort( uint32_t vectorID,
                                                                           VectorStateEnum::Enum state,
                                                                           float age,
                                                                           float progress,
                                                                           float microsporidiaDuration,
                                                                           uint32_t initial_population,
                                                                           const VectorGenome& rGenome,
                                                                           int speciesIndex )
    {
        VectorCohortIndividual* pvci = VectorCohortIndividual::CreateCohort( vectorID,
                                                                             state,
                                                                             age,
                                                                             progress,
                                                                             microsporidiaDuration,
                                                                             initial_population,
                                                                             rGenome,
                                                                             speciesIndex );
        return pvci;
    }

    void VectorPopulationIndividual::Update_Infectious_Queue(float dt)
    {
        // no queue entries, since all entries are in the Adult_Queue
    }

    void VectorPopulationIndividual::Update_Infected_Queue(float dt)
    {
        // no queue entries, since all entries are in the Adult_Queue
    }

    void VectorPopulationIndividual::Update_Adult_Queue(float dt)
    {
        // Get habitat-weighted average oviposition-trap killing fraction to be used in ProcessFeedingCycle
        // TODO: it might be more consistent to put this in with the other lifecycle probabilities somehow
        m_average_oviposition_killing = 0;
        float total_larval_capacity = 0;
        for (auto habitat : (*m_larval_habitats))
        {
            float capacity = habitat->GetCurrentLarvalCapacity();
            m_average_oviposition_killing += capacity * habitat->GetOvipositionTrapKilling();
            total_larval_capacity += capacity;
        }
        
        if( total_larval_capacity != 0 )
        {
            m_average_oviposition_killing /= total_larval_capacity;
        }

        // Use the verbose "foreach" construct here because empty queues (i.e. dead individuals) will be removed
        for( auto it = pAdultQueues->begin(); it != pAdultQueues->end(); ++it )
        {
            IVectorCohort* cohort = *it;

            // static cast is _much_ faster than QI and we _must_ have an IVectorCohortIndividual here.
            IVectorCohortIndividual* tempentry2 = current_vci = static_cast<IVectorCohortIndividual*>(static_cast<VectorCohortIndividual*>(static_cast<VectorCohortAbstract*>(cohort)));

            // Increment age of individual mosquitoes
            UpdateAge(cohort, dt);

            bool prev_has_microsporidia = cohort->HasMicrosporidia();
            if( !cohort->HasMated() )
            {
                AddAdultsAndMate( cohort, *pAdultQueues, false );
            }

            if( cohort->GetPopulation() > 0 )
            {
                ProcessFeedingCycle(dt, cohort);
            }

            if( cohort->GetPopulation() > 0 )
            {
                float infected_progress = GetInfectedProgress( cohort );
                if( (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) ||
                    (cohort->GetState() == VectorStateEnum::STATE_INFECTED  ) )
                {
                    LOG_VALID_F("infected progress this timestep %f state %u id %u temperature %f  \n",
                                 infected_progress,
                                 cohort->GetState(),
                                 cohort->GetID(),
                                 m_context->GetLocalWeather()->airtemperature());
                }

                cohort->Update( m_context->GetRng(), dt, species()->trait_modifiers, infected_progress, prev_has_microsporidia );
            }

            // Remove empty cohorts (i.e. dead mosquitoes)
            if (cohort->GetPopulation() <= 0)
            {
                UpdateAgeAtDeath( cohort, 1 );

                pAdultQueues->remove( it );
                VectorCohortIndividual::reclaim(tempentry2);
            }
            else
            {
                // Increment counters
                queueIncrementTotalPopulation(cohort);
            }
        }

        // Acquire infections with strain tracking for exposed queues

        LOG_DEBUG("Exposure to contagion: human to vector.\n");
        m_pNodeVector->ExposeVector( (IInfectable*)this, dt );
        IndoorExposedQueues.clear();
        OutdoorExposedQueues.clear();
    }

    void VectorPopulationIndividual::AdjustForCumulativeProbability(VectorPopulation::FeedingProbabilities& rFeedProbs)
    {
        // ----------------------
        // --- Not Feeding Branch
        // ----------------------
        rFeedProbs.die_without_attempting_to_feed += rFeedProbs.die_local_mortality;

        // -------------------
        // --- Feeding Branch
        // -------------------
        rFeedProbs.die_sugar_feeding               += rFeedProbs.die_local_mortality;
        rFeedProbs.die_before_human_feeding        += rFeedProbs.die_sugar_feeding;
        rFeedProbs.successful_feed_animal          += rFeedProbs.die_before_human_feeding;
        rFeedProbs.successful_feed_artifical_diet  += rFeedProbs.successful_feed_animal;
        rFeedProbs.successful_feed_attempt_indoor  += rFeedProbs.successful_feed_artifical_diet;
        rFeedProbs.successful_feed_attempt_outdoor += rFeedProbs.successful_feed_attempt_indoor;
        rFeedProbs.survive_without_feeding         += rFeedProbs.successful_feed_attempt_outdoor;

        // -----------------------------------------------------------------
        // --- Attempt To Feed Indoor Branch - subset of the Feeding Branch
        // -----------------------------------------------------------------
        // start with successful_feed_ad
        rFeedProbs.indoor.die_before_feeding    += rFeedProbs.indoor.successful_feed_ad;
        rFeedProbs.indoor.not_available         += rFeedProbs.indoor.die_before_feeding;
        rFeedProbs.indoor.die_during_feeding    += rFeedProbs.indoor.not_available;
        rFeedProbs.indoor.die_after_feeding     += rFeedProbs.indoor.die_during_feeding;
        rFeedProbs.indoor.successful_feed_human += rFeedProbs.indoor.die_after_feeding;

        // -----------------------------------------------------------------
        // --- Attempt To Feed Outdoor Branch - subset of the Feeding Branch
        // -----------------------------------------------------------------
        // start with outdoor_die_before_feeding
        rFeedProbs.outdoor.not_available         += rFeedProbs.outdoor.die_before_feeding;
        rFeedProbs.outdoor.die_during_feeding    += rFeedProbs.outdoor.not_available;
        rFeedProbs.outdoor.die_after_feeding     += rFeedProbs.outdoor.die_during_feeding;
        rFeedProbs.outdoor.successful_feed_human += rFeedProbs.outdoor.die_after_feeding;
    }

    bool VectorPopulationIndividual::DidDie( IVectorCohort* cohort,
                                             float probDies,
                                             float outcome,
                                             bool enableMortality,
                                             uint32_t& rCounter,
                                             const char* msg,
                                             const char* name )
    {
        bool vector_dies = (outcome <= probDies);
        if (vector_dies)
        {
            if( msg != nullptr )
            {
                LOG_VALID_F("The mosquito died %s%s, id = %d.\n", msg, name, cohort->GetID());
            }
            if (enableMortality)
            {
                //mosquito dies
                cohort->SetPopulation(0);
            }
            else
            {
                cohort->AddNewGestating( 0, 0 ); // set as not gestating
            }
            ++rCounter;
        }
        return vector_dies;
    }

    bool VectorPopulationIndividual::DidFeed( IVectorCohort* cohort,
                                              float probFeed,
                                              float outcome,
                                              uint32_t& rCounter,
                                              const char* msg,
                                              const char* name )
    {
        bool fed = (outcome <= probFeed);
        if (fed)
        {
            rCounter += cohort->GetPopulation();
            if (msg != nullptr)
            {
                LOG_VALID_F("The mosquito successfully %s%s, id = %d, state = %s .\n",
                             msg,
                             name, 
                             cohort->GetID(),
                             VectorStateEnum::pairs::lookup_key(cohort->GetState()));
            }
        }
        return fed;
    }

    bool VectorPopulationIndividual::DiedLayingEggs( IVectorCohort* cohort, const FeedingProbabilities& rFeedProbs )
    {
        uint32_t numDoneGestating = cohort->RemoveNumDoneGestating();
        if ( numDoneGestating > 0)
        {
            if (m_average_oviposition_killing > 0) // avoid drawing random number if zero
            {
                if( DidDie( cohort, m_average_oviposition_killing, m_context->GetRng()->e(), m_VectorMortality, dead_mosquitoes_before, "laying eggs", "" ) ) return true;
            }

            AddEggsToLayingQueue( cohort, numDoneGestating );
            current_vci->IncrementParity(); // increment number of times vector has laid eggs
        }

        return false;
    }

    VectorPopulationIndividual::IndoorOutdoorResults
    VectorPopulationIndividual::ProcessIndoorOutdoorFeeding(
        const char* pIndoorOutdoorName,
        const VectorPopulationIndividual::IndoorOutdoorProbabilities& rProbs,
        float probHumanFeed,
        IVectorCohort* cohort,
        TransmissionRoute::Enum routeVectorToHuman,
        VectorCohortVector_t& rExposedQueues )
    {
        IndoorOutdoorResults results;

        // --------------------------------
        // --- Indoor/OutdoorFeeding Branch Subset
        // --------------------------------
        // Reset cumulative probability for another random draw
        // to assign indoor-feeding outcomes among the following:
        float outcome = m_context->GetRng()->e();

        bool died_or_did_not_feed = false;
        uint32_t num_died_before_feed = 0;
        uint32_t num_not_feed         = 0;
        uint32_t num_died_during_feed = 0;
        uint32_t num_died_after_feed  = 0;
        uint32_t num_ad_feed          = 0;
        uint32_t num_human_feed       = 0;

        bool did_feed = DidFeed( cohort, rProbs.successful_feed_ad, outcome, num_ad_feed, "fed on an artificial diet", pIndoorOutdoorName );

        if( !did_feed )
        {
            // The 'or' should stop calling DidDie() once died_or_not_feed=true
            died_or_did_not_feed =                         DidDie( cohort, rProbs.die_before_feeding, outcome, m_VectorMortality, num_died_before_feed, "attempting to feed", pIndoorOutdoorName );
            died_or_did_not_feed = died_or_did_not_feed || DidDie( cohort, rProbs.not_available,      outcome, false            , num_not_feed,         "attempting to feed", pIndoorOutdoorName );
            died_or_did_not_feed = died_or_did_not_feed || DidDie( cohort, rProbs.die_during_feeding, outcome, m_VectorMortality, num_died_during_feed, "attempting to feed", pIndoorOutdoorName );
            died_or_did_not_feed = died_or_did_not_feed || DidDie( cohort, rProbs.die_after_feeding,  outcome, m_VectorMortality, num_died_after_feed,  "attempting to feed", pIndoorOutdoorName );
        }

        if( !died_or_did_not_feed && !did_feed )
        {
            // successful_feed_human should equal 1.0 so fixing at 1.0 to avoid floating point rounding issues
            did_feed = DidFeed( cohort, /*rProbs.successful_feed_human*/1.0, outcome, num_human_feed, "fed on an human"   , pIndoorOutdoorName );
        }
        results.num_died             = (num_died_before_feed + num_died_during_feed + num_died_after_feed);
        results.num_fed_ad           = num_ad_feed;
        results.num_fed_human        = num_human_feed;
        results.num_bites_total      = (num_died_during_feed + num_died_after_feed + num_human_feed); // used for human biting rate
        results.num_bites_infectious = VectorToHumanTransmission( cohort,
                                                                  current_vci->GetStrainIdentity(),
                                                                  results.num_bites_total , 
                                                                  routeVectorToHuman );

        num_attempt_but_not_feed += num_not_feed;

        if( died_or_did_not_feed ) return results;

        release_assert( ((num_human_feed > 0) || (num_ad_feed > 0)) && did_feed );

        // (c) successful human feed?
        if( (num_human_feed > 0) && (cohort->GetState() == VectorStateEnum::STATE_ADULT) )
        {
            release_assert( probHumanFeed > 0 );

            // push back to exposed cohort (to be checked for new infection)
            rExposedQueues.push_back(cohort);
        }
        //else ++num_attempt_but_not_feed;

        return results;
    }

    uint32_t VectorPopulationIndividual::ProcessFeedingCycle(float dt, IVectorCohort* cohort)
    {
        if (cohort->GetPopulation() <= 0)
            return 0;

        IVectorCohortIndividual *tempentry2 = current_vci;

        FeedingProbabilities feed_probs = CalculateFeedingProbabilities(dt, cohort);

        // Random draw to be used in both Feeding and Non-Feeding Branches
        float outcome = m_context->GetRng()->e();

        // --------------------------------------------------
        // --- Head of both Feeding and Non-Feeding Branches
        // --- Check if the vector should have died due to 
        // --- life expectancy, dry heat mortality, etc.
        // --------------------------------------------------
        if( DidDie( cohort, feed_probs.die_local_mortality, outcome, m_VectorMortality, dead_mosquitoes_before, "local mortality", "" ) ) return 0;

        // Determine whether it will feed?
        tempentry2->SetOvipositionTimer(tempentry2->GetOvipositionTimer() - int(dt));
        if( tempentry2->GetOvipositionTimer() > 0 )
        {
            // -------------------------------------------------
            // --- Non-Feeding Branch Continued
            // --- not feeding so determine if some died to due
            // --- interventions targeted at non-feeding vectors
            // -------------------------------------------------
            if( DidDie( cohort, feed_probs.die_without_attempting_to_feed, outcome, m_VectorMortality, dead_mosquitoes_before, "without attempting to feed", "" ) ) return 0;

            num_gestating_begin += cohort->GetNumGestating();
            UpdateGestatingCount( cohort );
            return 0;
        }

        // Lays eggs and calculate oviposition killing before feeding
        if( DiedLayingEggs( cohort, feed_probs ) ) return 0;

        // ----------------------------
        // --- Feeding Branch Continued
        // ----------------------------
        release_assert( cohort->GetNumLookingToFeed() > 0 );
        num_looking_to_feed += cohort->GetNumLookingToFeed();

        // Use initial random draw to assign individual mosquito feeding outcomes among the following:
        //     (0) die_sugar_feeding
        //     (1) diebeforeattempttohumanfeed
        //     (2) successfulfeed_animal
        //     (3) successfulfeed_AD
        //     (4) indoorattempttohumanfeed
        //     (5) outdoorattempttohumanfeed

        // Counters for feeding-cycle results
        uint32_t successful_animal_feed = 0;
        uint32_t successful_AD_feed     = 0;
        uint32_t successful_human_feed  = 0;

        // (0) die_sugar_feeding
        if( DidDie( cohort, feed_probs.die_sugar_feeding, outcome, m_VectorMortality, dead_mosquitoes_before, "sugar trap", "") ) return 0;

        // (1) die before human feeding?
        if (DidDie(cohort, feed_probs.die_before_human_feeding, outcome, m_VectorMortality, dead_mosquitoes_before, "before human feeding", "")) return 0;

        do { // at least one feeding attempt

             // (2) feed on animal?
            if (DidFeed(cohort, feed_probs.successful_feed_animal, outcome, successful_animal_feed, "fed on an animal", "")) continue;

            // (3) feed on artificial diet?
            if (DidFeed(cohort, feed_probs.successful_feed_artifical_diet, outcome, successful_AD_feed, "fed on an artificial diet", "")) continue;

            // (4) attempt indoor feed?
            uint32_t attempt_indoor_feed = 0;
            if (DidFeed(cohort, feed_probs.successful_feed_attempt_indoor, outcome, attempt_indoor_feed, nullptr, nullptr ))
            {
                num_attempt_feed_indoor += attempt_indoor_feed;

                float prob_human_feed  = probs()->indoor_successfulfeed_human.GetValue( m_SpeciesIndex, cohort->GetGenome() );

                // --------------------------------
                // --- Inddoor Feeding Branch Subset
                // --------------------------------
                IndoorOutdoorResults results = ProcessIndoorOutdoorFeeding( "-indoor",
                                                                            feed_probs.indoor,
                                                                            prob_human_feed,
                                                                            cohort, 
                                                                            TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR,
                                                                            IndoorExposedQueues );

                dead_mosquitoes_indoor += results.num_died;
                successful_AD_feed     += results.num_fed_ad;
                successful_human_feed  += results.num_fed_human;
                indoorbites            += float( results.num_bites_total);  // update human biting rate as well
                indoorinfectiousbites  += float( results.num_bites_infectious );

                if( (results.num_fed_ad > 0) || (results.num_fed_human > 0) )
                {
                    continue;
                }
                else
                {
                    return 0;
                }
            }

            // Part of Feeding Branch
            // (5) attempt outdoor feed?
            uint32_t attempt_outdoor_feed = 0;
            if (DidFeed(cohort, feed_probs.successful_feed_attempt_outdoor, outcome, attempt_outdoor_feed, nullptr, nullptr ))
            {
                num_attempt_feed_outdoor += attempt_outdoor_feed;

                float prob_human_feed  = probs()->outdoor_successfulfeed_human.GetValue( m_SpeciesIndex, cohort->GetGenome() );

                // --------------------------------
                // --- Outdoor Feeding Branch Subset
                // --------------------------------
                IndoorOutdoorResults results = ProcessIndoorOutdoorFeeding( "-outdoor",
                                                                            feed_probs.outdoor,
                                                                            prob_human_feed,
                                                                            cohort, 
                                                                            TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR,
                                                                            OutdoorExposedQueues );

                dead_mosquitoes_outdoor += results.num_died;
                successful_human_feed   += results.num_fed_human;
                outdoorbites            += float( results.num_bites_total );  // update human biting rate as well
                outdoorinfectiousbites  += float( results.num_bites_infectious );

                if( (results.num_fed_ad > 0) || (results.num_fed_human > 0) )
                {
                    continue;
                }
                else
                {
                    return 0;
                }
            }
            // NOTE: We can get to this point because VectorProbabilites::survivewithoutsuccessfulfeed
            // is non-zero due to something like SpatialRepellent
            ++num_attempt_but_not_feed;
        }
        while (0); // just one feed for now, but here is where we will decide whether to continue feeding

        // Start those that fed to be gestating and reset oviposition timer for next cycle
        uint32_t num_fed = successful_human_feed + successful_AD_feed + successful_animal_feed;
        StartGestating( num_fed, cohort );

        return 0; // not used
    }

    void VectorPopulationIndividual::StartGestating( uint32_t numFed, IVectorCohort* cohort )
    {
        num_fed_counter += numFed;
        cohort->AddNewGestating( CalculateOvipositionTime(), numFed ); // also sets oviposition timer
        UpdateGestatingCount( cohort );
    }

    void VectorPopulationIndividual::UpdateGestatingCount( const IVectorCohort* cohort )
    {
        // Overrode this method for performance reasons
        if( cohort->GetPopulation() > 0 )
        {
            uint32_t num = cohort->GetNumGestating();
            num_gestating_end += num;

            const IVectorCohortIndividual* p_ivci = static_cast<const IVectorCohortIndividual*>(static_cast<const VectorCohortIndividual*>(static_cast<const VectorCohortAbstract*>(cohort)));
            int day = p_ivci->GetOvipositionTimer();
            if( (num <= 0) || (day < 1) ) day = 1;
            while( day > num_gestating_queue.size() )
            {
                num_gestating_queue.push_back( 0 );
            }
            num_gestating_queue[ day - 1 ] += num;
        }
    }

    uint32_t VectorPopulationIndividual::CalculateOvipositionTime()
    {
        // Get duration that can be a temperature-dependent gonotrophic cycle duration:
        float mean_cycle_duration = GetFeedingCycleDuration();

        // Allocate timers randomly to upper and lower bounds of fractional duration
        // If mean is 2.8 days: 80% will have 3-day cycles, and 20% will have 2-day cycles
        uint32_t timer = m_context->GetRng()->randomRound( mean_cycle_duration );
        LOG_VALID_F("Mean gonotrophic cycle duration = %0.5f days at %0.2f degrees C , timer set to %u .\n",
                     mean_cycle_duration, m_context->GetLocalWeather()->airtemperature(), timer);

        return timer;
    }

    void VectorPopulationIndividual::RandomlySetOvipositionTimer(VectorCohortIndividual* pvci)
    {
        uint32_t days_between_feeds = CalculateOvipositionTime();
        LOG_VALID_F("The feeding duration is %d days.\n", days_between_feeds);

        uint32_t timer = m_context->GetRng()->uniformZeroToN32( days_between_feeds );

        // --------------------------------------------------------------------------------------
        // --- We shift these values by one day so that 1/days_between_feeds will feed each day.
        // --- The group with timer=1, will feed the first day, timer=2 will feed the second day
        // --- and so on.  If we allow timer=0, then that group AND the timer=1 group will feed
        // --- on the first day.  Also, there will be NO one feeding on timer=days_between_feeds.
        // --------------------------------------------------------------------------------------
        timer += 1;

        if (params()->vector_aging)
        {
            float age = float((days_between_feeds + 1) - timer); // +1 to offset +1 to timer above
            pvci->SetAge(age);
        }
        pvci->AddNewGestating( timer, 1 ); // also sets timer
    }

    void VectorPopulationIndividual::AddAdultsAndMate( IVectorCohort* pFemaleCohort,
                                                       VectorCohortCollectionAbstract& rQueue,
                                                       bool isNewAdult )
    {
        if( isNewAdult )
        {
            pFemaleCohort->SetPopulation( pFemaleCohort->GetPopulation() / m_mosquito_weight );
            VectorPopulation::AddAdultsAndMate( pFemaleCohort, rQueue, isNewAdult );
            pFemaleCohort->SetPopulation( pFemaleCohort->GetPopulation() * m_mosquito_weight );
        }
        else if( m_UnmatedMaleTotal > 0 )
        {
            auto it = SelectMaleMatingCohort();
            VectorCohortMale* p_male_cohort = *it;
            release_assert(p_male_cohort->GetUnmatedCount() > 0);
            // updating this here so when it comes to the new microsporidia cohort, 
            // the numbers are all updated already (like in VectorPopulation)
            UpdateMaleMatingCDF(it); 

            VectorGameteBitPair_t male_genome_bits = p_male_cohort->GetGenome().GetBits();

            int female_ms_strain_index = pFemaleCohort->GetGenome().GetMicrosporidiaStrainIndex();
            int   male_ms_strain_index = p_male_cohort->GetGenome().GetMicrosporidiaStrainIndex();

            bool female_has_microsporidia = pFemaleCohort->HasMicrosporidia();
            bool   male_has_microsporidia = p_male_cohort->HasMicrosporidia();

            if( male_has_microsporidia && !female_has_microsporidia )
            {
                // Male infecting Female
                float prob_transmission = m_species_params->microsporidia_strains[ male_ms_strain_index ]->male_to_female_transmission_probability;
                if( m_context->GetRng()->SmartDraw( prob_transmission ) )
                {
                    pFemaleCohort->InfectWithMicrosporidia( male_ms_strain_index );
                }
            }
            else if( !male_has_microsporidia && female_has_microsporidia )
            {
                // Female infecting Male
                float prob_transmission = m_species_params->microsporidia_strains[ female_ms_strain_index ]->female_to_male_transmission_probability;
                if( m_context->GetRng()->SmartDraw( prob_transmission ) )
                {
                    uint32_t orig_unmated = p_male_cohort->GetUnmatedCount();
                    VectorCohortMale* p_new_male_cohort = p_male_cohort->SplitNumber( m_context->GetRng(), 
                                                                                      m_pNodeVector->GetNextVectorSuid().data, 
                                                                                      1 );
                    release_assert( p_new_male_cohort != nullptr );
                    p_new_male_cohort->InfectWithMicrosporidia(female_ms_strain_index);
                    // ---------------------------------------------------------------
                    // --- massaging unmated numbers in the cohorts:
                    // --- the ones infected with microsporidia definitely all have mated 
                    // --- if there were any unmated, all stay with original cohort
                    // --- the unmated_count and unmated_count_cdf in the original cohort is up to date (see mating loop)
                    // ---------------------------------------------------------------
                    p_new_male_cohort->SetUnmatedCount( 0 );
                    p_male_cohort->SetUnmatedCount( orig_unmated ); 
                    release_assert( p_male_cohort->GetPopulation() >= p_male_cohort->GetUnmatedCount() );
                    // -----------------------------------------------------------------------------------
                    // --- I'm having the female carry that the male was infected because it shows that
                    // --- both parents were/became infected.  I also think that if this tends towards all
                    // --- vectors getting infected, then this should cause the creation of fewer cohorts.
                    // -----------------------------------------------------------------------------------
                    male_genome_bits = p_new_male_cohort->GetGenome().GetBits();
                    m_NeedToRefreshTheMatingCDF = true; // need to update cdf in case new microsporidia cohorts were merged with present ones
                    MaleQueues.add( p_new_male_cohort, 0.0, true );

                }
            }
            pFemaleCohort->SetMateGenome( VectorGenome( male_genome_bits ) );
        }
    }

    void VectorPopulationIndividual::AddAdultCohort( IVectorCohort* pFemaleCohort,
                                                     const VectorGenome& rMaleGenome,
                                                     uint32_t pop,
                                                     VectorCohortCollectionAbstract& rQueue,
                                                     bool isNewAdult )
    {
        release_assert( isNewAdult ); // assume AddAdultsAndMate() only gives us the new adults

        for( uint32_t i = 0; i < pop; ++i )
        {
            VectorCohortIndividual* tempentrynew = CreateAdultCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                      VectorStateEnum::STATE_ADULT,
                                                                      0.0,
                                                                      0.0,
                                                                      pFemaleCohort->GetDurationOfMicrosporidia(),
                                                                      m_mosquito_weight,
                                                                      pFemaleCohort->GetGenome(),
                                                                      m_SpeciesIndex );
            tempentrynew->SetMateGenome( rMaleGenome );
            queueIncrementTotalPopulation( tempentrynew );//to keep accounting consistent with non-aging version
            new_adults += 1;
            rQueue.add( tempentrynew, 0.0, false );
        }
        pFemaleCohort->SetPopulation( pFemaleCohort->GetPopulation() - pop );
    }

    void VectorPopulationIndividual::AddReleasedCohort( VectorStateEnum::Enum state,
                                                        const VectorGenome& rFemaleGenome,
                                                        const VectorGenome& rMaleGenome,
                                                        uint32_t pop )
    {
        float microsporidia_duration = 0.0;
        if( rFemaleGenome.HasMicrosporidia() )
        {
            // Set value so that microsporidia is fully mature
            microsporidia_duration = 1000.0;
        }

        for( uint32_t i = 0; i < pop; ++i )
        {
            VectorCohortIndividual* tempentrynew = CreateAdultCohort( m_pNodeVector->GetNextVectorSuid().data,
                                                                      VectorStateEnum::STATE_ADULT,
                                                                      0.0,
                                                                      0.0,
                                                                      microsporidia_duration,
                                                                      m_mosquito_weight,
                                                                      rFemaleGenome,
                                                                      m_SpeciesIndex );
            // if this is true, our vectors-to-be-released are mated and we're going to make them gestated
            if (VectorGender::VECTOR_MALE == rMaleGenome.GetGender())
            {
                tempentrynew->SetMateGenome(rMaleGenome);
                tempentrynew->AddNewGestating(1, 1); //setting them gestated and ready to lay
                if (params()->vector_aging)
                {
                    //setting age to days-between-feeds because gestated (see RandomlySetOvipositionTimer)
                    tempentrynew->SetAge(CalculateOvipositionTime()); 
                }
            }
            if( (state == VectorStateEnum::STATE_INFECTED) || (state == VectorStateEnum::STATE_INFECTIOUS) )
            {
                // assume no ParasiteGenomes
                StrainIdentity tmp;
                tempentrynew->AcquireNewInfection( &tmp );
                tempentrynew->SetState( state );
            }
            m_ReleasedAdults.push_back( tempentrynew );
        }
    }

    void VectorPopulationIndividual::Expose( const IContagionPopulation* cp, 
                                             float dt,
                                             TransmissionRoute::Enum transmission_route )
    {
        IContagionPopulationGP* cp_gp = nullptr;
        if ( s_OK != (const_cast<IContagionPopulation*>(cp))->QueryInterface(GET_IID(IContagionPopulationGP), (void**)&cp_gp) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "cp", "IContagionPopulationGP", "IContagionPopulation" );
        }

        // Get the infectiouness from the contagion population
        GeneticProbability infection_prob = cp_gp->GetTotalContagionGP();

        // Nothing to do if there isn't any
        if( infection_prob.GetSum() == 0 ) return;

        // Determine whether we are acting on an indoor or outdoor exposed population
        if (transmission_route == TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR)
        {
            ExposeCohortList( cp, IndoorExposedQueues, probs()->indoor_successfulfeed_human, infection_prob, true );
        }
        else if (transmission_route == TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR)
        {
            ExposeCohortList( cp, OutdoorExposedQueues, probs()->outdoor_successfulfeed_human, infection_prob, false );
        }
        else
        {
            std::ostringstream oss;
            oss << "Error in " << __FUNCTION__ << " : don't know what to do with transmission_route = " << transmission_route << std::endl;
            throw IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, oss.str().c_str());
        }
    }

    void VectorPopulationIndividual::ExposeCohortList( const IContagionPopulation* cp,
                                                       VectorCohortVector_t& list,
                                                       const GeneticProbability& success_prob_gp,
                                                       const GeneticProbability& infection_prob_gp,
                                                       bool isIndoors )
    {
        for (auto exposed : list)
        {
            // we don't model a vector having multiple strains. It can have only one infection.
            if( exposed->GetState() != VectorStateEnum::STATE_ADULT ) continue;

            float modifier = GetDiseaseAcquisitionModifier( exposed );

            float infection_prob = infection_prob_gp.GetValue( m_SpeciesIndex, exposed->GetGenome() );
            float success_prob = success_prob_gp.GetValue( m_SpeciesIndex, exposed->GetGenome() );
            float prob = infection_prob * modifier / success_prob;

            // Determine if there is a new infection
            if( m_context->GetRng()->SmartDraw( prob ) )
            {
                LOG_DEBUG_F("(individual) VECTOR (%d) acquired infection from HUMAN.\n", exposed->GetPopulation());
                
                // Draw weighted random strain from ContagionPopulation
                StrainIdentity strainID;
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // !!! In the co-transmission model, this step selects the person !!!
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                if( cp->ResolveInfectingStrain( &strainID ) )
                {
                    current_vci = nullptr;
                    if ((exposed)->QueryInterface(GET_IID(IVectorCohortIndividual), (void**)&current_vci) != s_OK)
                    {
                        throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "exposed", "IVectorCohortIndividual", "VectorCohort");
                    }

                    // Set state to STATE_INFECTED and store strainID
                    AcquireNewInfection( exposed->GetID(), current_vci, strainID, isIndoors );

                    // Adjust population-level counters
                    state_counts[ VectorStateEnum::STATE_ADULT    ] -= exposed->GetPopulation();
                    state_counts[ VectorStateEnum::STATE_INFECTED ] += exposed->GetPopulation();

                    genome_counts[ VectorStateEnum::STATE_ADULT    ][ exposed->GetGenome().GetBits() ] -= exposed->GetPopulation();
                    genome_counts[ VectorStateEnum::STATE_INFECTED ][ exposed->GetGenome().GetBits() ] += exposed->GetPopulation();

                    int strain_index = exposed->GetGenome().GetMicrosporidiaStrainIndex();
                    microsporidia_counts[ strain_index ][ VectorStateEnum::STATE_ADULT    ] -= exposed->GetPopulation();
                    microsporidia_counts[ strain_index ][ VectorStateEnum::STATE_INFECTED ] += exposed->GetPopulation();

                    queueIncrementNumInfs( exposed );
                }
            }
        }
    }

    void VectorPopulationIndividual::AcquireNewInfection( uint32_t vectorID,
                                                          IVectorCohortIndividual* pVCI,
                                                          const StrainIdentity& rStrain,
                                                          bool isIndoors )
    {
        VectorToHumanAdapter adapter( m_context, vectorID );
        IIndividualEventBroadcaster* broadcaster = m_context->GetEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( &adapter, EventTrigger::HumanToVectorTransmission );

        pVCI->AcquireNewInfection( &rStrain );
    }

    void VectorPopulationIndividual::Vector_Migration( float dt, VectorCohortVector_t* pMigratingQueue, bool migrate_males_only )
    {
        release_assert(m_pMigrationInfoVector);
        release_assert(pMigratingQueue);
        
        // migrating males only 
        VectorPopulation::Vector_Migration(dt, pMigratingQueue, true);
        
        // checking if any of the vectors might travel at all if non-genetics migration
        // we can do this before UpdateRates because if m_TotalRateFemale == 0 it will stay 0
        // UpdateRates does not change total migration rate, but, rather re-distributes where the vectors go
        if( !m_pMigrationInfoVector->MightTravel( VectorGender::VECTOR_FEMALE ) ) 
        { 
            return;
        }

        // updating migration rates and migrating females
        IVectorSimulationContext* p_vsc = nullptr;
        if (s_OK != m_context->GetParent()->QueryInterface(GET_IID(IVectorSimulationContext), (void**)&p_vsc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "m_context->GetParent()", "IVectorSimulationContext", "ISimulationContext" );
        }
        suids::suid current_node = m_context->GetSuid();
        m_pMigrationInfoVector->UpdateRates( current_node, get_SpeciesID(), p_vsc );

        // initializing these outside the loop to be updated inside
        VectorToHumanAdapter adapter( m_context, 0 ); 

        suids::suid      destination = suids::nil_suid();
        MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
        float                   time = 0.0;

        // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
        for( auto it = pAdultQueues->begin(); it != pAdultQueues->end(); ++it )
        {
            IVectorCohort* this_vector = *it;
            adapter.SetVectorID( ( this_vector )->GetID() );
            // we don't need the actual fraction traveling (that's for cohorts), 
            // it updates the total migration rate and migration rcd that's used by PickMigrationStep
            // returns nullptr when total rate of travel = 0
            if( m_pMigrationInfoVector->GetFractionTraveling( this_vector ) == nullptr ) 
            {
                continue;
            }


            m_pMigrationInfoVector->PickMigrationStep( m_context->GetRng(), &adapter, 1.0, destination, mig_type, time, dt );

            // test if vector will migrate: no destination = no migration, also don't migrate to node you're already in
            if( !destination.is_nil() && ( destination != current_node ) ) 
            {
                pAdultQueues->remove( it );
                // Used to use dynamic_cast here which is _very_ slow.
                IMigrate* emigre = this_vector->GetIMigrate();
                emigre->SetMigrating( destination, MigrationType::LOCAL_MIGRATION, 0.0, 0.0, false );
                pMigratingQueue->push_back( this_vector );
            }
        }   
    }

    void VectorPopulationIndividual::AddImmigratingVector(IVectorCohort* pvc)
    {
        switch (pvc->GetState())
        {
            case VectorStateEnum::STATE_INFECTED:
            case VectorStateEnum::STATE_INFECTIOUS:
            case VectorStateEnum::STATE_ADULT:
                if (m_IsSortingVectors)
                    m_ImmigratingAdult.push_back(pvc);
                else
                    pAdultQueues->add(pvc, 0.0, true);
                break;

            case VectorStateEnum::STATE_MALE:
                m_NeedToRefreshTheMatingCDF = true;
                if (m_IsSortingVectors)
                    m_ImmigratingMale.push_back(pvc);
                else
                    MaleQueues.add(pvc, 0.0, true);
                break;

            default:
                throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "IVectorCohort::GetState()", pvc->GetState(), VectorStateEnum::pairs::lookup_key(pvc->GetState()));
        }
    }

    uint32_t VectorPopulationIndividual::CountStrains( VectorStateEnum::Enum state,
                                                       const VectorCohortCollectionAbstract& queue,
                                                       IStrainIdentity* pStrain ) const
    {
        uint32_t num_infected = state_counts[ state ];
        if( pStrain != nullptr )
        {
            num_infected = 0;
            for( auto p_cohort : queue )
            {
                IVectorCohortIndividual *p_vci = nullptr;
                if( p_cohort->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**)&p_vci ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                                   "p_cohort", "IVectorCohortIndividual", "IVectorCohort" );
                }

                if( (p_cohort->GetState() == state) && p_vci->HasStrain( *pStrain ) )
                {
                    num_infected += p_cohort->GetPopulation();
                }
            }
        }
        return num_infected;
    }

    uint32_t VectorPopulationIndividual::getInfectedCount(IStrainIdentity* pStrain) const
    {
        return CountStrains(VectorStateEnum::STATE_INFECTED, *pAdultQueues, pStrain);
    }

    uint32_t VectorPopulationIndividual::getInfectiousCount(IStrainIdentity* pStrain) const
    {
        return CountStrains(VectorStateEnum::STATE_INFECTIOUS, *pAdultQueues, pStrain);
    }

    std::vector<uint32_t> VectorPopulationIndividual::GetNewlyInfectedVectorIds() const
    {
        std::vector<uint32_t> suids;
        for (auto cohort : *pAdultQueues)
        {
            if ((cohort->GetState() == VectorStateEnum::STATE_INFECTED) && (cohort->GetProgress() == 0))
            {
                suids.push_back(cohort->GetID());
            }
        }

        return suids;
    }

    std::vector<uint32_t> VectorPopulationIndividual::GetInfectiousVectorIds() const
    {
        std::vector<uint32_t> suids;
        for (auto cohort : *pAdultQueues)
        {
            if (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS)
            {
                suids.push_back(cohort->GetID());
            }
        }

        return suids;
    }

    std::map<uint32_t, uint32_t> VectorPopulationIndividual::getNumInfectiousByCohort() const
    {
        throw NotYetImplementedException(__FILE__, __LINE__, __FUNCTION__, "Counts of infectious mosquitoes only supported in cohort model.\n");
    }

    REGISTER_SERIALIZABLE(VectorPopulationIndividual);

    void VectorPopulationIndividual::serialize(IArchive& ar, VectorPopulationIndividual* obj)
    {
        VectorPopulation::serialize(ar, obj);

        VectorPopulationIndividual& population = *obj;
        ar.labelElement("m_mosquito_weight") & population.m_mosquito_weight;

        // ------------------------------------------------------------------
        // --- The following are only use temporarily during an update cycle.
        // ---  They are cleared and populated each update.
        // ------------------------------------------------------------------
        //m_average_oviposition_killing
        //IndoorExposedQueues
        //OutdoorExposedQueues
        //current_vci
    }
}