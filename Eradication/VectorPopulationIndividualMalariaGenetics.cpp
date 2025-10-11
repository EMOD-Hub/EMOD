
#include "stdafx.h"

#include "VectorPopulationIndividualMalariaGenetics.h"
#include "MalariaGeneticsContexts.h"
#include "VectorCohortIndividualMalariaGenetics.h"
#include "INodeContext.h"
#include "VectorSpeciesParameters.h"
#include "VectorParameters.h"
#include "VectorToHumanAdapter.h"
#include "NodeEventContext.h"
#include "BroadcasterObserver.h"
#include "ParasiteGenetics.h"
#include "StrainIdentityMalariaGenetics.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "VectorPopulationIndividualMalariaGenetics" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( VectorPopulationIndividualMalariaGenetics, VectorPopulationIndividual )
        HANDLE_INTERFACE( IVectorPopulationReportingMalariaGenetics )
    END_QUERY_INTERFACE_DERIVED( VectorPopulationIndividualMalariaGenetics, VectorPopulationIndividual )

    VectorPopulationIndividualMalariaGenetics*
        VectorPopulationIndividualMalariaGenetics::CreatePopulation( INodeContext *context,
                                                                     int speciesIndex,
                                                                     uint32_t adult,
                                                                     uint32_t mosquito_weight )
    {
        VectorPopulationIndividualMalariaGenetics *newpopulation = _new_ VectorPopulationIndividualMalariaGenetics( mosquito_weight );
        newpopulation->Initialize( context, speciesIndex, adult );

        return newpopulation;
    }

    VectorPopulationIndividualMalariaGenetics::VectorPopulationIndividualMalariaGenetics( uint32_t mosquito_weight )
        : VectorPopulationIndividual()
        , m_pNodeGenetics( nullptr )
        , m_NumBitesAdult( 0 )
        , m_NumBitesInfected( 0 )
        , m_NumBitesInfectious( 0 )
        , m_VectorCounters()
    {
    }

    VectorPopulationIndividualMalariaGenetics::~VectorPopulationIndividualMalariaGenetics()
    {
    }

    VectorCohortIndividual *VectorPopulationIndividualMalariaGenetics::CreateAdultCohort( uint32_t vectorID,
                                                                                          VectorStateEnum::Enum state,
                                                                                          float age,
                                                                                          float progress,
                                                                                          float microsporidiaDuration,
                                                                                          uint32_t initial_population,
                                                                                          const VectorGenome& rGenome,
                                                                                          int speciesIndex )
    {
        VectorCohortIndividual* pvci = VectorCohortIndividualMalariaGenetics::CreateCohort( vectorID,
                                                                                            state,
                                                                                            age,
                                                                                            progress,
                                                                                            microsporidiaDuration,
                                                                                            initial_population,
                                                                                            rGenome,
                                                                                            speciesIndex );
        return pvci;

    }

    void VectorPopulationIndividualMalariaGenetics::SetContextTo( INodeContext* context )
    {
        VectorPopulationIndividual::SetContextTo( context );

        if( context->QueryInterface( GET_IID( INodeMalariaGenetics ), (void**)&m_pNodeGenetics ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "m_context", "INodeMalariaGenetics", "INodeContext" );
        }
    }

    void VectorPopulationIndividualMalariaGenetics::UpdateVectorPopulation( float dt )
    {
        m_NumBitesAdult = 0;
        m_NumBitesInfected = 0;
        m_NumBitesInfectious = 0;

        VectorPopulationIndividual::UpdateVectorPopulation( dt );

        if( EnvPtr->Log->CheckLogLevel( Logger::VALIDATION, _module ) )
        {
            for (auto it = pAdultQueues->begin(); it != pAdultQueues->end(); ++it)
            {
                VectorCohortIndividualMalariaGenetics* p_cohort = static_cast<VectorCohortIndividualMalariaGenetics*>(*it);
                std::string results = p_cohort->GetLogSporozoiteInfo( species()->genes );
                if (!results.empty())
                {
                    LOG_VALID(results.c_str());
                }
            }
        }
    }

    void VectorPopulationIndividualMalariaGenetics::Update_Adult_Queue( float dt )
    {
        for( auto it = pAdultQueues->begin(); it != pAdultQueues->end(); ++it )
        {
            IVectorCohort* cohort = *it;

            IVectorCohortIndividualMalariaGenetics* p_vcimg = static_cast<IVectorCohortIndividualMalariaGenetics*>(static_cast<VectorCohortIndividualMalariaGenetics*>(static_cast<VectorCohortAbstract*>(cohort)));
            p_vcimg->SetParasiteIdGenderator( m_pNodeGenetics->GetParasiteIdGenerator() );
        }
        VectorPopulationIndividual::Update_Adult_Queue( dt );
    }

    VectorPopulationIndividual::IndoorOutdoorResults
        VectorPopulationIndividualMalariaGenetics::ProcessIndoorOutdoorFeeding(
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
        uint32_t num_not_feed = 0;
        uint32_t num_died_during_feed = 0;
        uint32_t num_died_after_feed = 0;
        uint32_t num_ad_feed = 0;
        uint32_t num_human_feed = 0;
        IVectorInterventionsEffects* p_ivie = nullptr;

        IndoorOutdoorProbabilities probs = rProbs;

        bool did_feed = DidFeed( cohort, probs.successful_feed_ad, outcome, num_ad_feed, "fed on an artificial diet", pIndoorOutdoorName );

        if( !did_feed )
        {
            // The 'or' should stop calling DidDie() once died_or_did_not_feed=true
            died_or_did_not_feed =                         DidDie( cohort, probs.die_before_feeding, outcome, m_VectorMortality, num_died_before_feed, "attempting to feed", pIndoorOutdoorName );
            died_or_did_not_feed = died_or_did_not_feed || DidDie( cohort, probs.not_available,      outcome, false,             num_not_feed,         "attempting to feed", pIndoorOutdoorName );

            // We nest the checks below because GeneticProbability.GetValue() can take processing time.
            if( !died_or_did_not_feed )
            {
                p_ivie = m_pNodeGenetics->SelectPersonToAttemptToFeedOn( m_SpeciesIndex, cohort->GetGenome(), (routeVectorToHuman == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR) );

                probs = GetProbabilitiesForPerson( cohort, routeVectorToHuman, p_ivie, probs );

                died_or_did_not_feed = died_or_did_not_feed || DidDie( cohort, probs.die_during_feeding, outcome, m_VectorMortality, num_died_during_feed, "attempting to feed", pIndoorOutdoorName );
                died_or_did_not_feed = died_or_did_not_feed || DidDie( cohort, probs.die_after_feeding,  outcome, m_VectorMortality, num_died_after_feed,  "attempting to feed", pIndoorOutdoorName );

                if( !died_or_did_not_feed )
                {
                    // We assume that if the vector gets this far, she must have fed and got away successfully
                    did_feed = DidFeed( cohort, probs.successful_feed_human, outcome, num_human_feed, "fed on an human", pIndoorOutdoorName );
                }
            }
        }

        results.num_died = (num_died_before_feed + num_died_during_feed + num_died_after_feed);
        results.num_fed_ad = num_ad_feed;
        results.num_fed_human = num_human_feed;
        results.num_bites_total = (num_died_during_feed + num_died_after_feed + num_human_feed); // used for human biting rate

        if( results.num_bites_total > 0 )
        {
            switch( cohort->GetState() )
            {
                case VectorStateEnum::STATE_ADULT:
                    m_NumBitesAdult += results.num_bites_total;
                    break;
                case VectorStateEnum::STATE_INFECTED:
                    m_NumBitesInfected += results.num_bites_total;
                    break;
                case VectorStateEnum::STATE_INFECTIOUS:
                    m_NumBitesInfectious += results.num_bites_total;
                    break;
                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "GetState()", cohort->GetState(), "unknown VectorStateEnum" );
            }

            VectorCohortIndividualMalariaGenetics* p_vcimg = static_cast<VectorCohortIndividualMalariaGenetics*>(cohort);

            std::vector<IParasiteCohort*> sporozoites;
            if( cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
            {
                results.num_bites_infectious = results.num_bites_total;

                float modifier = m_species_params->trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, cohort->GetGenome() );
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // FPG-TODO - We need to either add support for transmission modifier or make sure people can't set it
                //modifier *= float( results.num_bites_infectious ) * species()->transmissionmod;
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                sporozoites = p_vcimg->GetSporozoitesForBite( m_context->GetRng(), m_pNodeGenetics->GetParasiteIdGenerator(), modifier );
            }
            uint32_t bite_id = m_pNodeGenetics->GetNextBiteId();
            const GametocytesInPerson& r_gametocytes_in_person = m_pNodeGenetics->VectorBitesPerson( p_ivie->GetHumanID(),
                                                                                                     cohort->GetID(),
                                                                                                     bite_id,
                                                                                                     sporozoites );

            bool is_allowed = true;
            if( ParasiteGenetics::GetInstance()->IsFPGSimulatingBaseModel() )
            {
                // If trying to simulate the base model then only non-infected vector can get infected.
                // Infected/infectious vectors can get new infections in the base model.
                is_allowed = (cohort->GetState() == VectorStateEnum::STATE_ADULT);
            }

            // Only need to extract gametocytes if the vector lived i.e. successfully fed
            if( (r_gametocytes_in_person.infectiousness > 0) && (num_human_feed > 0) && is_allowed )
            {
                float infectiousness = r_gametocytes_in_person.infectiousness;

                float modifier = GetDiseaseAcquisitionModifier( cohort );

                float prob = infectiousness * modifier;

                // Determine if there is a new infection
                if( m_context->GetRng()->SmartDraw( prob ) )
                {
                    bool infected = p_vcimg->ExtractGametocytes( m_context->GetRng(),
                                                                 m_context->GetTime().time,
                                                                 m_context->GetExternalID(),
                                                                 cohort->GetID(),
                                                                 bite_id,
                                                                 m_pNodeGenetics->GetParasiteIdGenerator(),
                                                                 r_gametocytes_in_person );
                    if( infected )
                    {
                        VectorToHumanAdapter adapter( m_context, cohort->GetID() );
                        IIndividualEventBroadcaster* broadcaster = m_context->GetEventContext()->GetIndividualEventBroadcaster();
                        broadcaster->TriggerObservers( &adapter, EventTrigger::HumanToVectorTransmission );
                    }
                }
            }
        }

        num_attempt_but_not_feed += num_not_feed;

        return results;
    }

    VectorPopulationIndividualMalariaGenetics::IndoorOutdoorProbabilities
        VectorPopulationIndividualMalariaGenetics::GetProbabilitiesForPerson( IVectorCohort* pCohort,
                                                                              TransmissionRoute::Enum routeVectorToHuman,
                                                                              IVectorInterventionsEffects* pIVIE,
                                                                              const VectorPopulationIndividual::IndoorOutdoorProbabilities& rIOProbs )
    {
        const VectorGenome& r_genome = pCohort->GetGenome();
        IndoorOutdoorProbabilities probs;

        if( routeVectorToHuman == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR )
        {
            probs.die_during_feeding    = pIVIE->GetDieDuringFeeding().GetValue(    m_SpeciesIndex, r_genome );
            probs.die_after_feeding     = pIVIE->GetDiePostFeeding().GetValue(      m_SpeciesIndex, r_genome );
            probs.successful_feed_human = pIVIE->GetSuccessfulFeedHuman().GetValue( m_SpeciesIndex, r_genome );
        }
        else if( routeVectorToHuman == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR )
        {
            probs.die_during_feeding    = pIVIE->GetOutdoorDieDuringFeeding().GetValue(    m_SpeciesIndex, r_genome );
            probs.die_after_feeding     = pIVIE->GetOutdoorDiePostFeeding().GetValue(      m_SpeciesIndex, r_genome );
            probs.successful_feed_human = pIVIE->GetOutdoorSuccessfulFeedHuman().GetValue( m_SpeciesIndex, r_genome );
        }
        else
        {
            release_assert( false ); // shouldn't get here
        }

        // Adjust if the vector has sporozoites
        if( pCohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
        {
            probs.die_during_feeding    *= species()->infectioushfmortmod;
            probs.die_after_feeding     *= infectiouscorrection;
            probs.successful_feed_human *= infectiouscorrection;
        }

        // -----------------------------------------------------------------------------
        // --- Adjust the remaining probabilities based on the person being bit so that
        // --- they are a CDF starting where the population level probability left off.
        // -----------------------------------------------------------------------------
        float sum = probs.die_during_feeding
                  + probs.die_after_feeding
                  + probs.successful_feed_human;
        float adj = (1.0 - rIOProbs.not_available) / sum;

        probs.not_available         = rIOProbs.not_available;
        probs.die_during_feeding    = probs.not_available      + probs.die_during_feeding    * adj;
        probs.die_after_feeding     = probs.die_during_feeding + probs.die_after_feeding     * adj;
        probs.successful_feed_human = probs.die_after_feeding  + probs.successful_feed_human * adj;

        release_assert( fabs( 1.0 - probs.successful_feed_human ) < 10 * FLT_EPSILON );
        probs.successful_feed_human = 1.0;

        return probs;
    }

    void VectorPopulationIndividualMalariaGenetics::queueIncrementNumInfs( IVectorCohort* cohort )
    {
        if( (cohort->GetState() == VectorStateEnum::STATE_INFECTED) ||
            (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) )
        {
            VectorCohortIndividualMalariaGenetics* p_vcimg = static_cast<VectorCohortIndividualMalariaGenetics*>(cohort);
            num_infs_per_state_counts[ cohort->GetState() ] += p_vcimg->GetNumParasiteCohortsOocysts() + p_vcimg->GetNumParasiteCohortsSporozoites();
        }

        for( IVectorCounter* p_counter : m_VectorCounters )
        {
            p_counter->CountVector( cohort );
        }
    }

    void VectorPopulationIndividualMalariaGenetics::RegisterCounter( IVectorCounter* pCounter )
    {
        m_VectorCounters.push_back( pCounter );
    }

    void VectorPopulationIndividualMalariaGenetics::ExtractOtherVectorStats( OtherVectorStats& rOVS ) const
    {
        rOVS.num_bites_adults     += m_NumBitesAdult;
        rOVS.num_bites_infected   += m_NumBitesInfected;
        rOVS.num_bites_infectious += m_NumBitesInfectious;
    }

    REGISTER_SERIALIZABLE( VectorPopulationIndividualMalariaGenetics );

    void VectorPopulationIndividualMalariaGenetics::serialize( IArchive& ar, VectorPopulationIndividualMalariaGenetics* obj )
    {
        VectorPopulationIndividual::serialize( ar, obj );

        // These get cleared at the beginning of each time step so we don't need to serialize them
        //m_pNodeGenetics;
        //m_NumBitesAdult;
        //m_NumBitesInfected;
        //m_NumBitesInfectious;
    }
}