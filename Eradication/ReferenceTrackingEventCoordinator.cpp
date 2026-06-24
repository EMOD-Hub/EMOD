
#include "stdafx.h"

#include "ReferenceTrackingEventCoordinator.h"
#include "SimulationConfig.h"
#include "Simulation.h"
#include "SimulationEventContext.h"
#include <algorithm> // for std:find

SETUP_LOGGING( "ReferenceTrackingEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(ReferenceTrackingEventCoordinator)
    BEGIN_QUERY_INTERFACE_BODY(ReferenceTrackingEventCoordinator)
    END_QUERY_INTERFACE_BODY(ReferenceTrackingEventCoordinator)

    ReferenceTrackingEventCoordinator::ReferenceTrackingEventCoordinator()
        : StandardInterventionDistributionEventCoordinator( false )//false=don't use standard demographic coverage
        , m_Year2ValueMap(MIN_YEAR,MAX_YEAR,0.0f,1.0f)
        , m_EndYear(0.0f)
        , m_NumQualifiedWithout(0.0f)
        , m_NumQualifiedNeeding(0.0f)
        , m_QualifiedPeopleWithoutMap()
    {
    }

    bool
    ReferenceTrackingEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM) &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ReferenceTrackingEventCoordinator can only be used in STI and HIV simulations." );
        }

        float update_period = DAYSPERYEAR;
        initConfigTypeMap("Time_Value_Map", &m_Year2ValueMap, RTEC_Time_Value_Map_DESC_TEXT );
        initConfigTypeMap("Update_Period",  &update_period,   RTEC_Update_Period_DESC_TEXT, 1.0,      10*DAYSPERYEAR, DAYSPERYEAR );
        initConfigTypeMap("End_Year",       &m_EndYear,       RTEC_End_Year_DESC_TEXT,      MIN_YEAR, MAX_YEAR,       MAX_YEAR );

        auto ret = StandardInterventionDistributionEventCoordinator::Configure( inputJson );
        num_repetitions = -1; // unlimited
        if( JsonConfigurable::_dryrun == false )
        {
            float dt = GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep;
            tsteps_between_reps = update_period/dt; // this won't be precise, depending on math.
            if( tsteps_between_reps <= 0.0 )
            {
                // don't let this be zero or it will only update one time
                tsteps_between_reps = 1;
            }
            if( m_pInterventionNode != nullptr )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, 
                                                 "ReferenceTrackingEventCoordinator can only distribute interventions for individuals." );
            }
        }
        LOG_DEBUG_F( "ReferenceTrackingEventCoordinator configured with update_period = %f, m_EndYear = %f, and tsteps_between_reps (derived) = %d.\n", update_period, m_EndYear, tsteps_between_reps );
        return ret;
    }

    void ReferenceTrackingEventCoordinator::CheckStartDay( float campaignStartDay ) const
    {
        float campaign_start_year = campaignStartDay / DAYSPERYEAR + Simulation::base_year;
        if( m_EndYear <= campaign_start_year )
        {
            LOG_WARN_F( "Campaign starts on year %f (day=%f). A ReferenceTrackingEventCoordinator ends on End_Year %f.  It will not distribute any interventions.\n",
                        campaign_start_year, campaignStartDay, m_EndYear );
        }

        if( campaign_start_year != m_Year2ValueMap.begin()->first )
        {
            LOG_WARN_F( "Campaign starts on year %f (day=%f). A ReferenceTrackingEventCoordinator has a Time_Value_Map that starts on year %f.\n",
                        campaign_start_year, campaignStartDay, m_Year2ValueMap.begin()->first );
        }
    }

    void ReferenceTrackingEventCoordinator::InitializeRepetitions( const Configuration* inputJson )
    {
        // don't include repetition parameters since they are managed internally by this class
    }

    // Obviously don't need this if it's not doing anything useful.
    void ReferenceTrackingEventCoordinator::Update( float dt )
    {
        // Check if it's time for another distribution
        if( parent->GetSimulationTime().Year() >= m_EndYear )
        {
            LOG_INFO_F( "ReferenceTrackingEventCoordinator expired.\n" );
            distribution_complete = true;
        }
        LOG_DEBUG_F( "Update...ing.\n" );
        return StandardInterventionDistributionEventCoordinator::Update( dt );
    }


    void ReferenceTrackingEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        StandardInterventionDistributionEventCoordinator::SetContextTo( isec );

        // Initialize the map so that we don't clear the map and the memory for our vectors of people
        for (auto event_context : cached_nodes)
        {
            m_QualifiedPeopleWithoutMap[ event_context ] = std::vector<IIndividualHumanEventContext*>();
        }
    }

    // The purpose of this function is to calculate the existing coverage of the intervention in question
    // and then to set the target coverage based on the error between measured and configured (for current time).
    void ReferenceTrackingEventCoordinator::preDistribute()
    {
        // Two variables that will be used by lambda function that's called for each individual;
        // these vars accumulate values across the population. 
        NonNegativeFloat totalWithIntervention = 0.0f;
        NonNegativeFloat totalQualifyingPop = 0.0f;

        for (auto event_context : cached_nodes)
        {
            if( !node_property_restrictions.Qualifies( event_context->GetNodeContext()->GetNodeProperties() ) )
            {
                continue;
            }
            auto& r_qualified_people_wo = m_QualifiedPeopleWithoutMap[ event_context ];
            r_qualified_people_wo.clear();

            int num_people = event_context->GetIndividualHumanCount();
            r_qualified_people_wo.reserve( num_people );

            // This is the function that will be called for each individual in this node (event_context)
            INodeEventContext::individual_visit_function_t fn = 
                [ this, &totalWithIntervention, &totalQualifyingPop, &r_qualified_people_wo ](IIndividualHumanEventContext *ihec)
            {
                if( qualifiesDemographically( ihec ) )
                {
                    auto mcw = ihec->GetMonteCarloWeight();
                    totalQualifyingPop += mcw;

                    // Check whether this individual has this intervention
                    auto better_ptr = ihec->GetInterventionsContext();
                    InterventionName intervention_name = m_pInterventionIndividual->GetName();
                    if( better_ptr->ContainsExistingByName( intervention_name ) )
                    {
                        totalWithIntervention += mcw;
                    }
                    else
                    {
                        r_qualified_people_wo.push_back( ihec );
                    }
                }
            };

            event_context->VisitIndividuals( fn ); // does not return value, updates total existing coverage by capture
        }

        float dc = 0.0f;
        if( totalQualifyingPop > 0 )
        {
            Fraction currentCoverageForIntervention = totalWithIntervention/totalQualifyingPop;
            m_NumQualifiedWithout = totalQualifyingPop - totalWithIntervention;
            float default_value = 0.0f;
            float year = parent->GetSimulationTime().Year();
            float target_coverage  = m_Year2ValueMap.getValueLinearInterpolation(year, default_value);

            m_NumQualifiedNeeding = ( target_coverage * totalQualifyingPop ) - totalWithIntervention;
            NO_LESS_THAN( m_NumQualifiedNeeding, 0 );

            if( m_NumQualifiedWithout > 0 )
            {
                dc = m_NumQualifiedNeeding / m_NumQualifiedWithout;
            }
            LOG_INFO_F( "Setting demographic_coverage to %f based on target_coverage = %f, currentCoverageForIntervention = %f, total without intervention  = %f, total with intervention = %f.\n",
                            dc,
                            float(target_coverage),
                            float(currentCoverageForIntervention),
                            float(m_NumQualifiedWithout),
                            float(totalWithIntervention)
                        );
        }
        else
        {
            LOG_INFO( "Setting demographic_coverage to 0 since 0 qualifying population.\n");
        }
        demographic_restrictions.SetDemographicCoverage( dc );
    }

    void ReferenceTrackingEventCoordinator::DistributeInterventionsToIndividuals( INodeEventContext* event_context )
    {
        ICampaignCostObserver * pICCO = nullptr;nullptr;
        if( event_context->QueryInterface( GET_IID( ICampaignCostObserver ), (void**)&pICCO ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                            "event_context",
                                            "ICampaignCostObserver",
                                            "INodeEventContext" );
        }

        auto& r_qualified_people_wo = m_QualifiedPeopleWithoutMap[ event_context ];

        float cost_out = 0.0;
        int total_given = 0;
        for( auto ihec : r_qualified_people_wo )
        {
            auto mcw = ihec->GetMonteCarloWeight();

            if( TargetedIndividualIsCovered( ihec ) )
            {
                if( DistributeInterventionsToIndividual( ihec, cost_out, pICCO ) )
                {
                    ++total_given;
                    m_NumQualifiedNeeding -= mcw;
                }
            }
            m_NumQualifiedWithout -= mcw;

            // ----------------------------------------------------------------------------------------------
            // --- Update the coverage due to our procesing of the list.  Some people get the intervention
            // --- and some do not.  This keeps adjusting the coverage so that we can more closely give
            // --- the intervention to the m_NumQualifiedNeeding people.
            // ----------------------------------------------------------------------------------------------
            if( (m_NumQualifiedWithout > 0.0f) && (m_NumQualifiedNeeding > 0.0f) )
            {
                float dc = m_NumQualifiedNeeding / m_NumQualifiedWithout;
                if( dc > 1.0 ) dc = 1.0;
                demographic_restrictions.SetDemographicCoverage( dc );
            }
            else
            {
                break;
            }
        }
        r_qualified_people_wo.clear();

        if( LOG_LEVEL( INFO ) )
        {
            // Create log message 
            std::stringstream ss;
            ss << "UpdateNodes() gave out " << total_given << " '" << log_intervention_name.c_str() << "' interventions ";
            std::string restriction_str = demographic_restrictions.GetPropertyRestrictionsAsString();
            if( !restriction_str.empty() )
            {
                ss << " with property restriction(s) " << restriction_str << " ";
            }
            if( event_context != nullptr )
            {
                ss << "at node " << event_context->GetExternalId();
            }
            ss << "\n";
            LOG_INFO( ss.str().c_str() );
        }
    }
}

