
#include "stdafx.h"

#include "FirstNodeWithNodePropertyEventCoordinator.h"

SETUP_LOGGING( "FirstNodeWithNodePropertyEventCoordinator" )


namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- NodeIdToEvent
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( NodeIdToEvent )
    END_QUERY_INTERFACE_BODY( NodeIdToEvent )

    NodeIdToEvent::NodeIdToEvent()
        : JsonConfigurable()
        , m_NodeId( 0 )
        , m_CoordinatorEvent()
        , m_WasEventSent( false )
    {
    }

    NodeIdToEvent::~NodeIdToEvent()
    {
    }

    bool NodeIdToEvent::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Node_ID", &m_NodeId, FNWNPEC_Node_ID_DESC_TEXT, 0, UINT_MAX, 0 );
        initConfigTypeMap( "Coordinator_Event_To_Broadcast", &m_CoordinatorEvent, FNWNPEC_Coordinator_Event_To_Broadcast_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( m_CoordinatorEvent.IsUninitialized() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                 "'Coordinator_Event_To_Broadcast' must be defined in FirstNodeWithNodePropertyEventCoordinator." );
            }
        }
        return configured;
    }

    ExternalNodeId_t NodeIdToEvent::GetExternalNodeId() const
    {
        return m_NodeId;
    }

    const EventTriggerCoordinator& NodeIdToEvent::GetCoordinatorEvent() const
    {
        return m_CoordinatorEvent;
    }

    void NodeIdToEvent::EventWasSent()
    {
        m_WasEventSent = true;
    }

    bool NodeIdToEvent::WasEventSent() const
    {
        return m_WasEventSent;
    }

    void NodeIdToEvent::ClearEventSent()
    {
        m_WasEventSent = false;
    }


    // ------------------------------------------------------------------------
    // --- NodeIdToEventList
    // ------------------------------------------------------------------------

    NodeIdToEventList::NodeIdToEventList()
        : JsonConfigurableCollection("NodeIdToEventList")
    {
    }

    NodeIdToEventList::~NodeIdToEventList()
    {
    }

    bool LessThan( NodeIdToEvent* pLeft, NodeIdToEvent* pRight )
    {
        return (pLeft->GetExternalNodeId() < pRight->GetExternalNodeId());
    }

    void NodeIdToEventList::CheckConfiguration()
    {
        if( Size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                             "At least one Node_ID:Coordinator_Event_To_Broadcast pair must be defined for FirstNodeWithNodePropertyEventCoordinator.");
        }

        // sort into ascending order
        std::sort( m_Collection.begin(), m_Collection.end(), LessThan );

        // Check if any of the Node Ids equal another
        for( int i = 1; i < m_Collection.size(); ++i )
        {
            if( m_Collection[ i - 1 ]->GetExternalNodeId() == m_Collection[ i ]->GetExternalNodeId() )
            {
                std::stringstream ss;
                ss << "More than 'Node_ID' is equal to " << m_Collection[ i ]->GetExternalNodeId() << " in FirstNodeWithNodePropertyEventCoordinator.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    NodeIdToEvent* NodeIdToEventList::CreateObject()
    {
        NodeIdToEvent* p_obj = new NodeIdToEvent();
        return p_obj;
    }

    // ------------------------------------------------------------------------
    // --- FirstNodeWithNodePropertyEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED( FirstNodeWithNodePropertyEventCoordinator )
    BEGIN_QUERY_INTERFACE_BODY(FirstNodeWithNodePropertyEventCoordinator)
    END_QUERY_INTERFACE_BODY(FirstNodeWithNodePropertyEventCoordinator)

    FirstNodeWithNodePropertyEventCoordinator::FirstNodeWithNodePropertyEventCoordinator()
        : TriggeredEventCoordinator()
        , m_NodePropertyKeyValueToHave()
        , m_NodeIdToEventList()
        , m_CoordinatorEventNotFound()
    {
        initSimTypes( 1, "*" );
    }

    FirstNodeWithNodePropertyEventCoordinator::~FirstNodeWithNodePropertyEventCoordinator()
    {
    }

    bool FirstNodeWithNodePropertyEventCoordinator::Configure( const Configuration * inputJson )
    {
        NPKeyValueParameter np_parameter;

        initConfigTypeMap( "Coordinator_Name",                &m_CoordinatorName,           Coordinator_Name_DESC_TEXT, "FirstNodeWithNodePropertyEventCoordinator");
        initConfigTypeMap( "Start_Trigger_Condition_List",    &m_StartTriggerConditionList, FNWNPEC_Start_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Not_Found_Coordinator_Event",     &m_CoordinatorEventNotFound,  FNWNPEC_Not_Found_Coordinator_Event_DESC_TEXT );
        initConfigTypeMap( "Node_Property_Key_Value_To_Have", &np_parameter,                FNWNPEC_Node_Property_Key_Value_To_Have_DESC_TEXT );

        initConfigComplexCollectionType( "Node_ID_To_Coordinator_Event_List", &m_NodeIdToEventList, FNWNPEC_Node_ID_To_Coordinator_Event_List_DESC_TEXT );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Do NOT call TriggeredEventCoordinator::Configure() so we only get the parameters here
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            CheckConfigTriggers( inputJson );

            if( EnvPtr->MPI.NumTasks > 1 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "FirstNodeWithNodePropertyEventCoordinator is not supported on multi-core.");
            }
            m_NodePropertyKeyValueToHave = np_parameter;
        }
        return configured;
    }

    void FirstNodeWithNodePropertyEventCoordinator::Update( float dt )
    {
    }

    void FirstNodeWithNodePropertyEventCoordinator::UpdateNodes( float dt )
    {
        if( !m_IsActive ) return;

        // ------------------------------------------------------------
        // --- Clear the list so that in the upcoming time step a node
        // --- can be selected again.
        // ------------------------------------------------------------
        for( int i = 0; i < m_NodeIdToEventList.Size(); ++i )
        {
            m_NodeIdToEventList[ i ]->ClearEventSent();
        }
        m_IsActive = false;
    }

    bool FirstNodeWithNodePropertyEventCoordinator::notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger )
    {
        auto it_start = find( m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), trigger );
        if( it_start != m_StartTriggerConditionList.end() )
        {
            SelectNodeAndBroadcastEvent();
            m_IsActive = true;
        }
        else
        {
            m_IsActive = false;
        }
        return true;
    }

    void FirstNodeWithNodePropertyEventCoordinator::SelectNodeAndBroadcastEvent()
    {
        bool found = false;
        for( int i = 0; !found && (i < m_NodeIdToEventList.Size()); ++i )
        {
            // --------------------------------------------------------------------
            // --- ignore nodes that were sent this time step because the node 
            // --- won't have an opportunity to mark itself as having been selected
            // --------------------------------------------------------------------
            if( !m_NodeIdToEventList[ i ]->WasEventSent() )
            {
                ExternalNodeId_t node_id = m_NodeIdToEventList[ i ]->GetExternalNodeId();
                for( auto p_node : cached_nodes )
                {
                    if( (p_node->GetExternalId() == node_id) &&
                        p_node->GetNodeContext()->GetNodeProperties().Contains( m_NodePropertyKeyValueToHave ) )
                    {
                        const EventTriggerCoordinator& r_ce = m_NodeIdToEventList[ i ]->GetCoordinatorEvent();
                        parent->GetCoordinatorEventBroadcaster()->TriggerObservers( this, r_ce );

                        // tag the event has being sent this time step
                        m_NodeIdToEventList[ i ]->EventWasSent();

                        found = true;
                        break;
                    }
                }
            }
        }

        if( !found && !m_CoordinatorEventNotFound.IsUninitialized() )
        {
            parent->GetCoordinatorEventBroadcaster()->TriggerObservers( this, m_CoordinatorEventNotFound );
        }
    }
}