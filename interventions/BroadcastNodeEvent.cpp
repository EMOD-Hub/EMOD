
#include "stdafx.h"
#include "BroadcastNodeEvent.h"
#include "NodeEventContext.h"
#include "SimulationEventContext.h"
#include "ISimulationContext.h"

SETUP_LOGGING( "BroadcastNodeEvent" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( BroadcastNodeEvent )
        HANDLE_INTERFACE( INodeDistributableIntervention )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_ISUPPORTS_VIA( INodeDistributableIntervention )
    END_QUERY_INTERFACE_BODY( BroadcastNodeEvent )

    IMPLEMENT_FACTORY_REGISTERED( BroadcastNodeEvent )

    BroadcastNodeEvent::BroadcastNodeEvent()
        : BaseNodeIntervention()
        , m_EventToBroadcast()
    {
    }

    BroadcastNodeEvent::BroadcastNodeEvent( const BroadcastNodeEvent& master )
        : BaseNodeIntervention( master )
        , m_EventToBroadcast( master.m_EventToBroadcast )
    {
    }

    BroadcastNodeEvent::~BroadcastNodeEvent()
    {
    }

    bool BroadcastNodeEvent::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Broadcast_Event", &m_EventToBroadcast, BNE_Broadcast_Event_DESC_TEXT );
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0.0f, 999999, 0.0f );

        bool retValue = BaseNodeIntervention::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            if( m_EventToBroadcast.IsUninitialized() )
            {
                std::stringstream ss;
                ss << "BroadcastNodeEvent was configured with empty (or uninitialized) Broadcast_Event.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }

        return retValue;
    }

    void BroadcastNodeEvent::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        INodeEventBroadcaster* broadcaster = parent->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster();
        broadcaster->TriggerObservers( parent, m_EventToBroadcast );

        expired = true;
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! TODO - can't do until BaseNodeIntervention is serializable
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //REGISTER_SERIALIZABLE(BroadcastNodeEvent);

    //void BroadcastNodeEvent::serialize(IArchive& ar, BroadcastNodeEvent* obj)
    //{
    //    BaseNodeIntervention::serialize( ar, obj );
    //    BroadcastNodeEvent& mf = *obj;
    //    ar.labelElement("m_EventToBroadcast") & mf.m_EventToBroadcast;
    //    ar.labelElement("cost_per_unit"     ) & mf.cost_per_unit;
    //}
}
