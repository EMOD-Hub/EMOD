
#include "stdafx.h"
#include "BroadcastCoordinatorEventFromNode.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanContext.h"
#include "INodeContext.h"
#include "ISimulationContext.h"
#include "SimulationEventContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "BroadcastCoordinatorEventFromNode" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(EventCoordinatorEventContextAdapter)
        HANDLE_INTERFACE(IEventCoordinatorEventContext)
        HANDLE_ISUPPORTS_VIA(IEventCoordinatorEventContext)
    END_QUERY_INTERFACE_BODY(EventCoordinatorEventContextAdapter)


    EventCoordinatorEventContextAdapter::EventCoordinatorEventContextAdapter( BaseNodeIntervention* pNodeIntervention,
                                                                              INodeEventContext* pNEC )
        : m_pIntervention( pNodeIntervention )
        , m_pNEC( pNEC )
        , m_Name()
    {
        release_assert( m_pIntervention != nullptr );
        m_Name = m_pIntervention->GetName().ToString();
    }

    EventCoordinatorEventContextAdapter::~EventCoordinatorEventContextAdapter()
    {
    }

    const std::string& EventCoordinatorEventContextAdapter::GetName() const
    {
        return m_Name;
    }

    const IdmDateTime& EventCoordinatorEventContextAdapter::GetTime() const
    {
        return m_pNEC->GetTime();
    }

    IEventCoordinator* EventCoordinatorEventContextAdapter::GetEventCoordinator()
    {
        release_assert( false );
        return nullptr;
    }


    BEGIN_QUERY_INTERFACE_BODY(BroadcastCoordinatorEventFromNode)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(BroadcastCoordinatorEventFromNode)

    IMPLEMENT_FACTORY_REGISTERED(BroadcastCoordinatorEventFromNode)

    BroadcastCoordinatorEventFromNode::BroadcastCoordinatorEventFromNode()
    : BaseNodeIntervention()
    , m_EventToBroadcast()
    {
    }

    BroadcastCoordinatorEventFromNode::BroadcastCoordinatorEventFromNode( const BroadcastCoordinatorEventFromNode& master )
        : BaseNodeIntervention( master )
        , m_EventToBroadcast( master.m_EventToBroadcast )
    {
    }

    BroadcastCoordinatorEventFromNode::~BroadcastCoordinatorEventFromNode()
    {
    }

    bool BroadcastCoordinatorEventFromNode::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Broadcast_Event", &m_EventToBroadcast, BCEFN_Broadcast_Event_DESC_TEXT );

        bool configured = BaseNodeIntervention::Configure( inputJson );

        if( configured && !JsonConfigurable::_dryrun && m_EventToBroadcast.IsUninitialized() )
        {
            std::stringstream ss;
            ss << "Intervention 'BroadcastCoordinatorEventFromNode' was configured with empty (or uninitialized) parameter 'Broadcast_Event' in <" << inputJson->GetDataLocation() << ">.\n";
            throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Broadcast_Event", *inputJson, ss.str().c_str() );
        }
        return configured;
    }

    void BroadcastCoordinatorEventFromNode::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        EventCoordinatorEventContextAdapter adapter( this, this->parent );
        parent->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetCoordinatorEventBroadcaster()->TriggerObservers( &adapter, m_EventToBroadcast );

        // expire the intervention
        expired = true;
    }
}
