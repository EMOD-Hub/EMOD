
#include "stdafx.h"
#include "NLHTIVNode.h"

#include "Log.h"
#include "Debug.h"
#include "ISimulationContext.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
//#include "IIndividualHuman.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "NLHTIVNode" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(NLHTIVNode)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(INodeEventObserver)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(NLHTIVNode)

    IMPLEMENT_FACTORY_REGISTERED(NLHTIVNode)

    NLHTIVNode::NLHTIVNode()
    : BaseNodeIntervention()
    , m_trigger_conditions()
    , max_duration(0)
    , duration(0)
    , node_property_restrictions()
    , blackout_period(0.0)
    , blackout_time_remaining(0.0)
    , blackout_event_trigger()
    , blackout_on_first_occurrence(false)
    , notification_occurred(false)
    , event_occurred_list()
    , _ndi(nullptr)
    , m_ClassName()
    {
    }

    NLHTIVNode::NLHTIVNode( const NLHTIVNode& rMaster )
    : BaseNodeIntervention( rMaster )
    , m_trigger_conditions( rMaster.m_trigger_conditions )
    , max_duration( rMaster.max_duration )
    , duration( rMaster.duration )
    , node_property_restrictions( rMaster.node_property_restrictions )
    , blackout_period( rMaster.blackout_period)
    , blackout_time_remaining( rMaster.blackout_time_remaining )
    , blackout_event_trigger( rMaster.blackout_event_trigger )
    , blackout_on_first_occurrence( rMaster.blackout_on_first_occurrence )
    , notification_occurred( rMaster.notification_occurred )
    , event_occurred_list( rMaster.event_occurred_list )
    , _ndi(nullptr)
    , m_ClassName( rMaster.m_ClassName )
    {
        if( rMaster._ndi != nullptr )
        {
            _ndi = rMaster._ndi->Clone();
        }
    }

    NLHTIVNode::~NLHTIVNode()
    { 
        delete _ndi;
    }
    int NLHTIVNode::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }
    int NLHTIVNode::Release()
    {
        return BaseNodeIntervention::Release();
    }

    bool
    NLHTIVNode::Configure(
        const Configuration * inputJson
    )
    {
        NodeInterventionConfig actual_node_intervention_config;
        initConfigComplexType( "Actual_NodeIntervention_Config", &actual_node_intervention_config, Actual_NodeIntervention_Config_DESC_TEXT );

        initConfigTypeMap("Duration", &max_duration, BT_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f ); // -1 is a convention for indefinite duration

        initConfigTypeMap( "Blackout_Period", &blackout_period, Blackout_Period_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Blackout_Event_Trigger", &blackout_event_trigger, NLHTIVNode_Blackout_Event_Trigger_DESC_TEXT );
        initConfigTypeMap( "Blackout_On_First_Occurrence", &blackout_on_first_occurrence, Blackout_On_First_Occurrence_DESC_TEXT, false );

        initConfigComplexType( "Node_Property_Restrictions", &node_property_restrictions, NLHTIVNode_Node_Property_Restrictions_DESC_TEXT );

        // --------------------------------------------------------------------------------------------------------------------
        // --- Phase 0 - Pre V2.0 - Trigger_Condition was an enum and users had to have all indivual events in the enum list.
        // --- Phase 1 -     V2.0 - Trigger_Condition was an enum but TriggerString and TriggerList were added to allow the 
        // ---                      user to use new User Defined events (i.e. strings).
        // --- Phase 2 - 11/12/15 - Trigger_Condition is now a string and so users don't need to use Trigger_Condition_String
        // ---                      in order to use the User Defined events.  However, we are leaving support here for 
        // ---                      Trigger_Condition_String for backward compatibility (and I'm lazy).
        // --- Phase 3 - 11/9/16    Consolidate so that the user only defines Trigger_Condition_List
        // --------------------------------------------------------------------------------------------------------------------
        initConfigTypeMap( "Trigger_Condition_List", &m_trigger_conditions, NLHTINode_Trigger_Condition_List_DESC_TEXT );

        bool retValue = BaseNodeIntervention::Configure( inputJson );

        //this section copied from standardevent coordinator
        if( retValue && !JsonConfigurable::_dryrun )
        {
            _ndi = InterventionFactory::CreateNDIIntervention( actual_node_intervention_config._json, inputJson->GetDataLocation(), "Actual_NodeIntervention_Config", true );
            m_ClassName = std::string( json::QuickInterpreter( actual_node_intervention_config._json )[ "class" ].As<json::String>() );

            event_occurred_list.resize( EventTriggerNodeFactory::GetInstance()->GetNumEventTriggers() );

            bool blackout_configured = (inputJson->Exist("Blackout_Event_Trigger")) || (inputJson->Exist("Blackout_Period")) || (inputJson->Exist("Blackout_On_First_Occurrence"));
            bool blackout_all_configured = (inputJson->Exist("Blackout_Event_Trigger")) && (inputJson->Exist("Blackout_Period")) && (inputJson->Exist("Blackout_On_First_Occurrence"));
            if (blackout_configured && !blackout_all_configured)
            {
                std::vector<string> blackout_missing_str;
                if( !inputJson->Exist("Blackout_Event_Trigger") )       blackout_missing_str.push_back("Blackout_Event_Trigger");
                if( !inputJson->Exist("Blackout_Period") )              blackout_missing_str.push_back("Blackout_Period");
                if( !inputJson->Exist("Blackout_On_First_Occurrence") ) blackout_missing_str.push_back("Blackout_On_First_Occurrence");

                throw MissingParameterFromConfigurationException(__FILE__, __LINE__, __FUNCTION__, inputJson->GetDataLocation().c_str(), blackout_missing_str, " All three Blackout parameters must be configured.");
            } 
        }
        JsonConfigurable::_useDefaults = false;
        return retValue;    
    }

    bool
    NLHTIVNode::Distribute(
        INodeEventContext *pNodeEventContext,
        IEventCoordinator2 *pEC
    )
    {
        bool was_distributed = BaseNodeIntervention::Distribute(pNodeEventContext, pEC);
        if (was_distributed)
        {
            LOG_DEBUG_F("Distributed Nodelevel health-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

            // QI to register ourself as a NLHTIVNode observer
            INodeEventBroadcaster * broadcaster = pNodeEventContext->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster();
            release_assert( broadcaster );
            for (auto &trigger : m_trigger_conditions)
            {
                LOG_DEBUG_F( "Registering as observer of event %s.\n", trigger.c_str() );
                broadcaster->RegisterObserver((INodeEventObserver*)this, trigger);
            }
        }
        else
        {
            LOG_WARN_F( "Failed to distribute intervention to node.\n" );
        }
        return was_distributed;
    }

    //returns false if didn't get the intervention
    bool NLHTIVNode::notifyOnEvent(
        INodeEventContext *pNode,
        const EventTriggerNode& trigger
    )
    {
        // ----------------------------------------------------------------------
        // --- Ignore events for nodes that don't qualify due to their properties
        // ----------------------------------------------------------------------
        if( !node_property_restrictions.Qualifies( parent->GetNodeContext()->GetNodeProperties() ) )
        {
            return false;
        }

        bool missed_intervention = false ;
        // the trigger event
        LOG_DEBUG_F("Node %d experienced event %s, check to see if it passes the conditions before distributing actual_intervention \n",
                    pNode->GetNodeContext()->GetSuid().data,
                    trigger.c_str()
                   );

        assert( parent );
        //assert( parent->GetRng() );

        release_assert( _ndi );

        // Huge performance win by cloning instead of configuring.
        INodeDistributableIntervention *ndi = _ndi->Clone();
        release_assert( ndi );
        ndi->AddRef();
        bool distributed =  ndi->Distribute( parent, nullptr );

        if( distributed )
        {
            LOG_INFO_F("Distributed '%s' intervention to node %d\n", m_ClassName.c_str(), parent->GetExternalId() );
        }
        ndi->Release();


        if( distributed )
        {
            if( blackout_on_first_occurrence )
            {
                blackout_time_remaining = blackout_period ;
            }
            else
            {
                notification_occurred = true ;
            }
            event_occurred_list[ trigger.GetIndex() ].insert( pNode->GetNodeContext()->GetSuid().data ); 
        }

        return distributed;
    }


    void NLHTIVNode::Unregister()
    {
        // unregister ourself as a node level health triggered observer
        INodeEventBroadcaster * broadcaster = parent->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster();
        release_assert( broadcaster );
        for (auto &trigger : m_trigger_conditions)
        {
            LOG_DEBUG_F( "Unregistering as observer of event %s.\n", trigger.c_str() );
            broadcaster->UnregisterObserver( (INodeEventObserver*)this, trigger );
        }
        SetExpired( true );
    }

    void NLHTIVNode::Update(float dt)
    {
        LOG_DEBUG_F( "Update\n" );
        if (!BaseNodeIntervention::UpdateNodesInterventionStatus())
        {
            Unregister();
            return;
        }

        duration += dt;
        if( max_duration >= 0 && duration > max_duration )
        {
            LOG_DEBUG_F("Node-Level HTI reached max_duration. Time to unregister...\n");
            Unregister();
        }
        event_occurred_list.clear();
        event_occurred_list.resize( EventTriggerNodeFactory::GetInstance()->GetNumEventTriggers() );

        blackout_time_remaining -= dt ;
        if( notification_occurred )
        {
            notification_occurred = false ;
            blackout_time_remaining = blackout_period ;
            LOG_DEBUG_F( "start blackout period - nodeid=%d\n",parent->GetExternalId() );
        }
    }
}
