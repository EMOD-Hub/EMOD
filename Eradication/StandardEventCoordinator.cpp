
#include "stdafx.h"

#include <sstream>
#include "Debug.h"
#include "Sugar.h"
#include "Environment.h"
#include "StandardEventCoordinator.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "FactorySupport.h"
#include "InterventionFactory.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "INodeContext.h"
#include "SimulationEventContext.h"
#include "NodeEventContext.h"
#include "Log.h"
#include "IdmString.h"
#include "RANDOM.h"

SETUP_LOGGING( "StandardEventCoordinator" )


// Let's have some fun and customize this. Log out which nodes get the intervention, and which inviduals, track
// them in a map, make bednet's conditional on some node info we get, and some individual info (not just coverage/
// randomness, then lets' repeat in 30 days 1 time.
// Then test out taking bednets away from migrators (just as a test).
namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(StandardInterventionDistributionEventCoordinator)

    IMPL_QUERY_INTERFACE2(StandardInterventionDistributionEventCoordinator, IEventCoordinator, IConfigurable)

    // ctor
    StandardInterventionDistributionEventCoordinator::StandardInterventionDistributionEventCoordinator( bool useDemographicCoverage ) 
    : parent(nullptr)
    , distribution_complete(false)
    , num_repetitions(1)
    , tsteps_between_reps(-1)
    , tsteps_since_last(0)
    //, include_emigrants(false)
    //, include_immigrants(false)
    , intervention_activated(false)
    , cached_nodes()
    , node_suids()
    , demographic_restrictions( true, TargetDemographicType::Everyone, useDemographicCoverage )
    , demographic_coverage(1.0)
    , node_property_restrictions()
    , use_demographic_coverage( useDemographicCoverage )
    , individual_selection_type( IndividualSelectionType::DEMOGRAPHIC_COVERAGE )
    , target_num_individuals( 1 )
    , log_intervention_name()
    , m_pInterventionIndividual( nullptr )
    , m_pInterventionNode( nullptr )
    {
        LOG_DEBUG("StandardInterventionDistributionEventCoordinator ctor\n");
    }

    StandardInterventionDistributionEventCoordinator::~StandardInterventionDistributionEventCoordinator()
    {
        delete m_pInterventionIndividual;
        delete m_pInterventionNode;
    }

    bool
    StandardInterventionDistributionEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        InterventionConfig intervention_config;
        initConfigComplexType( "Intervention_Config", &intervention_config, Intervention_Config_DESC_TEXT );

        InitializeRepetitions( inputJson );
        if( use_demographic_coverage )
        {
            InitializeIndividualSelectionType( inputJson );
        }

        //initConfigTypeMap("Include_Departures", &include_emigrants, Include_Departures_DESC_TEXT, false );
        //initConfigTypeMap("Include_Arrivals", &include_immigrants, Include_Arrivals_DESC_TEXT, false );

        demographic_restrictions.ConfigureRestrictions( this, inputJson );

        initConfigComplexType( "Node_Property_Restrictions", &node_property_restrictions, Node_Property_Restrictions_DESC_TEXT );

        bool retValue = JsonConfigurable::Configure( inputJson );
        if( retValue && !JsonConfigurable::_dryrun)
        {
            demographic_restrictions.CheckConfiguration();

            CheckRepetitionConfiguration();

            m_pInterventionIndividual = InterventionFactory::CreateIntervention( intervention_config._json, inputJson->GetDataLocation(), "Intervention_Config", false ); // don't throw if null
            if( m_pInterventionIndividual == nullptr )
            {
                m_pInterventionNode = InterventionFactory::CreateNDIIntervention( intervention_config._json, inputJson->GetDataLocation(), "Intervention_Config", false ); // don't throw if null
            }
            if( (m_pInterventionIndividual == nullptr) && (m_pInterventionNode == nullptr) )
            {
                std::string class_name = std::string(json::QuickInterpreter( intervention_config._json )["class"].As<json::String>()) ;

                std::stringstream ss;
                ss << "Invalid Intervention Type in '" << GetTypeName() << "'.\n";
                ss << "'" << class_name << "' is not a known intervention.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            log_intervention_name = std::string( json::QuickInterpreter( intervention_config._json )[ "class" ].As<json::String>() );

            if( m_pInterventionNode != nullptr )
            {
                // ---------------------------------------------------------------------------
                // --- If the user is attempting to define demographic restrictions when they
                // --- are using a node level intervention, then we need to error because these
                // --- restrictions are not doing anything.
                // ---------------------------------------------------------------------------
                if( !demographic_restrictions.HasDefaultRestrictions() ||
                    (individual_selection_type != IndividualSelectionType::DEMOGRAPHIC_COVERAGE) )
                {
                    std::ostringstream msg ;
                    msg << "Cannot target individuals when distributing nodel-level intervention (";
                    msg << std::string( json::QuickInterpreter( intervention_config._json )[ "class" ].As<json::String>() );
                    msg << ").\n";
                    msg << "In StandardInterventionDistributionEventCoordinator, you cannot target individuals\n";
                    msg << "when distributing node-level interventions such as NodeLevelHealthTriggeredIV or SpaceSpraying.\n";
                    msg << "You can only use parameters that select nodes such as 'Node_Property_Restrictions'.\n";
                    msg << "The following parameters can only be used when distributing individual-interventions:\n";
                    msg << "- Target_Demographic\n";
                    msg << "- Target_Gender\n";
                    msg << "- Target_Age_Min\n";
                    msg << "- Target_Age_Max\n";
                    msg << "- Target_Residents_Only\n";
                    msg << "- Individual_Selection_Type\n";
                    msg << "- Demographic_Coverage\n";
                    msg << "- Target_Num_Individuals\n";
                    msg << "- Property_Restrictions\n";
                    msg << "- Property_Restrictions_Within_Node\n";
                    msg << "- Targeting_Config\n";
                    msg << "The node level intervention must handle the targeting of individuals.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }

            if( (individual_selection_type == IndividualSelectionType::TARGET_NUM_INDIVIDUALS) &&
                (target_num_individuals == 0) )
            {
                LOG_WARN( "'Target_Num_Individuals'=0.  ZERO interventions will be distributed.\n" );
            }
        }

        return retValue;
    }

    void StandardInterventionDistributionEventCoordinator::InitializeRepetitions( const Configuration* inputJson )
    {
        initConfigTypeMap( "Number_Repetitions", &num_repetitions, Number_Repetitions_DESC_TEXT, -1, 10000, 1 );
        //if( num_repetitions > 1 ) // -1 = repeat without end, 0 is meaningless. want to think this one through more
        {
            initConfigTypeMap( "Timesteps_Between_Repetitions", &tsteps_between_reps, Timesteps_Between_Repetitions_DESC_TEXT, -1, 10000 /*undefined*/, -1 /*off*/ ); // , "Number_Repetitions", "<>0" );
        }
    }

    void StandardInterventionDistributionEventCoordinator::InitializeIndividualSelectionType( const Configuration* inputJson )
    {
        initConfig( "Individual_Selection_Type",
                    individual_selection_type,
                    inputJson,
                    MetadataDescriptor::Enum( "Individual_Selection_Type",
                                              Individual_Selection_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS( IndividualSelectionType ) ) );
        initConfigTypeMap( "Target_Num_Individuals",
                           &target_num_individuals,
                           Target_Num_Individuals_DESC_TEXT,
                           0, INT32_MAX, 1,
                           "Individual_Selection_Type", "TARGET_NUM_INDIVIDUALS" );
    }

    void StandardInterventionDistributionEventCoordinator::CheckRepetitionConfiguration()
    {
        if( num_repetitions == 0 )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "'Number_Repetitions' cannot equal zero.");
        }
    }

    void
    StandardInterventionDistributionEventCoordinator::SetContextTo(
        ISimulationEventContext *isec
    )
    {
        parent = isec;
        regenerateCachedNodeContextPointers();
    }

    // AddNode
    // EventCoordinators track nodes. Nodes can be used to get individuals, who can be queried for an intervention
    void
    StandardInterventionDistributionEventCoordinator::AddNode(
        const suids::suid& node_suid
    )
    {
        if( !intervention_activated )
        {
            intervention_activated = true;
            tsteps_since_last = tsteps_between_reps -1; // -1 is hack because Update is called before UpdateNodes and we inc in Update and check in UpdateNodes
        }

        // Store uids and node (event context) pointers
        node_suids.push_back(node_suid);
        cached_nodes.push_back(parent->GetNodeEventContext(node_suid));

#if 0
        INodeEventContext * pNec = parent->GetNodeEventContext(node_suid);
        // Register unconditionally to be notified when individuals arrive at our node so we can zap them!
        // TODO: Make this param driven
        if( include_immigrants )
        {
            pNec->RegisterTravelDistributionSource( this, INodeEventContext::Arrival );
        }
        if( include_emigrants )
        {
            pNec->RegisterTravelDistributionSource( this, INodeEventContext::Departure );
        }
#endif
    }

    void StandardInterventionDistributionEventCoordinator::Update( float dt )
    {
        // Check if it's time for another distribution
        if( intervention_activated && num_repetitions)
        {
            tsteps_since_last++;
        }
    }

    void StandardInterventionDistributionEventCoordinator::preDistribute()
    {
        return;
    }

    bool StandardInterventionDistributionEventCoordinator::IsTimeToUpdate( float dt )
    {
        // Only call VisitNodes on first call and if countdown == 0
        bool is_time_to_update = !((tsteps_since_last != tsteps_between_reps) || distribution_complete );
        return is_time_to_update;
    }

    void StandardInterventionDistributionEventCoordinator::UpdateNodes( float dt )
    {
        if( !IsTimeToUpdate( dt ) )
        {
            return;
        }

        std::vector<IIndividualHumanEventContext*> qualified_individuals;

        LOG_DEBUG_F("[UpdateNodes] visiting %d nodes per NodeSet\n", cached_nodes.size());

        preDistribute();

        for (auto event_context : cached_nodes)
        {
            if( !node_property_restrictions.Qualifies( event_context->GetNodeContext()->GetNodeProperties() ) )
            {
                continue;
            }

            if( m_pInterventionNode != nullptr )
            {
                DistributeInterventionsToNodes( event_context );
            }
            else
            {
                if( individual_selection_type == IndividualSelectionType::TARGET_NUM_INDIVIDUALS )
                {
                    if( target_num_individuals > 0 )
                    {
                        FindQualifyingIndividuals( event_context, qualified_individuals );
                    }
                }
                else
                {
                    DistributeInterventionsToIndividuals( event_context );
                }
            }
        }

        if( (individual_selection_type == IndividualSelectionType::TARGET_NUM_INDIVIDUALS) &&
            (target_num_individuals > 0) )
        {
            int totalIndivGivenIntervention = 0;
            float cost_out = 0.0;
            std::vector<IIndividualHumanEventContext*> selected = SelectIndividuals( qualified_individuals );
            for( auto p_ind : selected )
            {
                ICampaignCostObserver* pICCO;
                if( p_ind->GetNodeEventContext()->QueryInterface( GET_IID( ICampaignCostObserver ), (void**)&pICCO ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                                   "p_ind->GetNodeEventContext()",
                                                   "ICampaignCostObserver",
                                                   "INodeEventContext" );
                }

                if( DistributeInterventionsToIndividual( p_ind, cost_out, pICCO ) )
                {
                    ++totalIndivGivenIntervention;
                }
            }
            LogNumInterventionsDistributed( totalIndivGivenIntervention, nullptr );
        }

        UpdateRepetitions();
    }

    void StandardInterventionDistributionEventCoordinator::FindQualifyingIndividuals( INodeEventContext* pNEC,
                                                                                      std::vector<IIndividualHumanEventContext*>& r_qualified_individuals )
    {
        INodeEventContext::individual_visit_function_t fn =
            [ this, &r_qualified_individuals ]( IIndividualHumanEventContext *ihec )
        {
            if( qualifiesDemographically( ihec ) )
            {
                r_qualified_individuals.push_back( ihec );
            }
        };

        pNEC->VisitIndividuals( fn );
    }

    std::vector<IIndividualHumanEventContext*>
    StandardInterventionDistributionEventCoordinator::SelectIndividuals( const std::vector<IIndividualHumanEventContext*>& r_qualified_individuals )
    {
        if( target_num_individuals >= r_qualified_individuals.size() )
        {
            return r_qualified_individuals;
        }

        // ----------------------------------------------------------------------------------
        // --- Robert Floyd's Algorithm for Sampling without Replacement
        // --- http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html
        // ----------------------------------------------------------------------------------
        uint32_t N = r_qualified_individuals.size();
        uint32_t M = target_num_individuals;

        release_assert( cached_nodes.size() > 0 );
        std::set<uint32_t> selected_indexes = cached_nodes[ 0 ]->GetRng()->chooseMofN( M, N );

        std::vector<IIndividualHumanEventContext*> selected_individuals;
        for( auto index : selected_indexes )
        {
            selected_individuals.push_back( r_qualified_individuals[ index ] );
        }

        return selected_individuals;
    }

    void StandardInterventionDistributionEventCoordinator::UpdateRepetitions()
    {
        tsteps_since_last = 0;
        num_repetitions--;
        if( num_repetitions == 0 )
        {
            distribution_complete = true; // we're done, signal disposal ok
        }

        // this signals each process individually that its ok to clean up, in general if the completion times might be different on different nodes 
        // we'd want to coordinate the cleanup signal in Update()
    }

    bool StandardInterventionDistributionEventCoordinator::TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec)
    {
        float demographic_coverage = demographic_restrictions.GetDemographicCoverage();
        return ihec->GetInterventionsContext()->GetParent()->GetRng()->SmartDraw( demographic_coverage );
    }

    bool
    StandardInterventionDistributionEventCoordinator::visitIndividualCallback( 
        IIndividualHumanEventContext *ihec,
        float & incrementalCostOut,
        ICampaignCostObserver * pICCO
    )
    {
        bool distributed = true;
        {
            // Add real checks on demographics based on intervention demographic targetting. 
            // Return immediately if we hit a non-distribute condition
            if( qualifiesDemographically( ihec ) == false )
            {
                LOG_DEBUG("Individual not given intervention because not in target demographic\n");
                return false;
            }
            LOG_DEBUG("Individual meets demographic targeting criteria\n"); 

            if (!TargetedIndividualIsCovered(ihec))
            {
                incrementalCostOut = 0;
                return false;
            }
            else
            {
                incrementalCostOut = 0;

                distributed = DistributeInterventionsToIndividual( ihec, incrementalCostOut, pICCO );
            }
        }
        return distributed;
    }

    void StandardInterventionDistributionEventCoordinator::regenerateCachedNodeContextPointers()
    {
        // regenerate the cached INodeEventContext* pointers fromthe cached node suids
        // the fact that this needs to happen is probably a good argument for the EC to own the NodeSet, since it needs to query the SEC for the node ids and context pointers anyway
        cached_nodes.clear();
        for (auto& node_id : node_suids)
        {
            cached_nodes.push_back(parent->GetNodeEventContext(node_id));
        }
    }

    bool 
    StandardInterventionDistributionEventCoordinator::IsFinished()
    {
        return distribution_complete;
    }

    // private/protected
    bool
    StandardInterventionDistributionEventCoordinator::qualifiesDemographically(
        const IIndividualHumanEventContext* pIndividual
    )
    {
        return demographic_restrictions.IsQualified( pIndividual );
    }

    float StandardInterventionDistributionEventCoordinator::GetDemographicCoverage() const
    {
        return demographic_restrictions.GetDemographicCoverage();
    }

    float
    StandardInterventionDistributionEventCoordinator::getDemographicCoverageForIndividual(
        const IIndividualHumanEventContext *pInd
    )
    const
    {
        return GetDemographicCoverage();
    }

    TargetDemographicType::Enum StandardInterventionDistributionEventCoordinator::GetTargetDemographic() const
    {
        return demographic_restrictions.GetTargetDemographic();
    }

    float StandardInterventionDistributionEventCoordinator::GetMinimumAge() const
    {
        return demographic_restrictions.GetMinimumAge();
    }

    float StandardInterventionDistributionEventCoordinator::GetMaximumAge() const
    {
        return demographic_restrictions.GetMaximumAge();
    }

    void StandardInterventionDistributionEventCoordinator::ProcessDeparting(
        IIndividualHumanEventContext *pInd
    )
    {
        LOG_INFO("Individual departing from node receiving intervention. TODO: enforce demographic and other qualifiers.\n");
        float incrementalCostOut = 0.0f;
        visitIndividualCallback( pInd, incrementalCostOut, nullptr /* campaign cost observer */ );
    } // these do nothing for now

    void
    StandardInterventionDistributionEventCoordinator::ProcessArriving(
        IIndividualHumanEventContext *pInd
    )
    {
        LOG_INFO("Individual arriving at node receiving intervention. TODO: enforce demographic and other qualifiers.\n");
        float incrementalCostOut = 0.0f; 
        visitIndividualCallback( pInd, incrementalCostOut, nullptr /* campaign cost observer */ );
    }

    void StandardInterventionDistributionEventCoordinator::DistributeInterventionsToNodes( INodeEventContext* event_context )
    {
        INodeDistributableIntervention *ndi = m_pInterventionNode->Clone();
        ndi->AddRef();
        if( ndi->Distribute( event_context, this ) )
        {
            LOG_INFO_F( "UpdateNodes() distributed '%s' intervention to node %d\n", log_intervention_name.c_str(), event_context->GetId().data );
        }
        ndi->Release();
    }

    void StandardInterventionDistributionEventCoordinator::DistributeInterventionsToIndividuals( INodeEventContext* event_context )
    {
        int totalIndivGivenIntervention = event_context->VisitIndividuals( this );

        LogNumInterventionsDistributed( totalIndivGivenIntervention, event_context );
    }

    void StandardInterventionDistributionEventCoordinator::LogNumInterventionsDistributed( int totalIndivGivenIntervention, INodeEventContext* event_context )
    {
        if( LOG_LEVEL( INFO ) )
        {
            // Create log message 
            std::stringstream ss;
            ss << "UpdateNodes() gave out " << totalIndivGivenIntervention << " '" << log_intervention_name.c_str() << "' interventions ";
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

    bool StandardInterventionDistributionEventCoordinator::DistributeInterventionsToIndividual( IIndividualHumanEventContext *ihec,
                                                                                                float & incrementalCostOut,
                                                                                                ICampaignCostObserver * pICCO )
    {
        // instantiate and distribute intervention
        LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", log_intervention_name.c_str());
        IDistributableIntervention *di = m_pInterventionIndividual->Clone();
        release_assert( di );

        di->AddRef();
        bool distributed = di->Distribute( ihec->GetInterventionsContext(), pICCO );
        di->Release(); // a bit wasteful for now, could cache it for the next fellow

        if( distributed )
        {
            LOG_DEBUG_F( "Distributed an intervention (%p) to individual %d at a cost of %f\n",
                    di->GetName().c_str(), ihec->GetSuid().data, incrementalCostOut );
        }
        return distributed;
    }
}

