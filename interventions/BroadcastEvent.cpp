
#include "stdafx.h"
#include "BroadcastEvent.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "BroadcastEvent" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(BroadcastEvent)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(BroadcastEvent)

    IMPLEMENT_FACTORY_REGISTERED(BroadcastEvent)

    bool BroadcastEvent::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Broadcast_Event", &broadcast_event, Broadcast_Event_DESC_TEXT );

        bool ret = BaseIntervention::Configure( inputJson );

        if( !JsonConfigurable::_dryrun && broadcast_event.IsUninitialized() )
        {
            std::stringstream ss;
            ss << "Intervention 'BroadcastEvent' was configured with empty (or uninitialized) parameter 'Broadcast_Event' in <" << inputJson->GetDataLocation() << ">.\n";
            throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Broadcast_Event", *inputJson, ss.str().c_str() );
        }
        return ret;
    }

    BroadcastEvent::BroadcastEvent()
    : BaseIntervention()
    , broadcast_event()
    {
    }

    BroadcastEvent::BroadcastEvent( const BroadcastEvent& master )
        : BaseIntervention( master )
    {
        broadcast_event = master.broadcast_event;
    }

    void BroadcastEvent::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        if( !broadcast_event.IsUninitialized() )
        {
            // broadcast the event
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), broadcast_event );
        }

        // expire the intervention
        expired = true;
    }

    REGISTER_SERIALIZABLE(BroadcastEvent);

    void BroadcastEvent::serialize(IArchive& ar, BroadcastEvent* obj)
    {
        BaseIntervention::serialize( ar, obj );
        BroadcastEvent& be = *obj;
        ar.labelElement("broadcast_event") & be.broadcast_event;
    }
}
