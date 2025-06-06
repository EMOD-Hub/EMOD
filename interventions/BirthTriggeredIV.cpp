
#include "stdafx.h"
#include "BirthTriggeredIV.h"

#include "Log.h"
#include "Debug.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "EventTrigger.h"
#include "RANDOM.h"
#include "ISimulationContext.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "BirthTriggeredIV" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(BirthTriggeredIV)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IIndividualEventObserver)
        //HANDLE_INTERFACE(INodeDistributableInterventionParameterSetterInterface)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(BirthTriggeredIV)

    IMPLEMENT_FACTORY_REGISTERED(BirthTriggeredIV)

    BirthTriggeredIV::BirthTriggeredIV()
        : BaseNodeIntervention()
        , duration(0)
        , max_duration(0)
        , demographic_restrictions(false,TargetDemographicType::Everyone)
        , m_di( nullptr )
    {
    }

    BirthTriggeredIV::BirthTriggeredIV( const BirthTriggeredIV& rMaster )
        : BaseNodeIntervention( rMaster )
        , duration( rMaster.duration )
        , max_duration( rMaster.max_duration )
        , demographic_restrictions( rMaster.demographic_restrictions )
        , m_di( nullptr )
    {
        if( rMaster.m_di != nullptr )
        {
            m_di = rMaster.m_di->Clone();
        }
    }

    BirthTriggeredIV::~BirthTriggeredIV()
    {
        delete m_di;
    }

    int BirthTriggeredIV::AddRef() { return BaseNodeIntervention::AddRef(); }
    int BirthTriggeredIV::Release() { return BaseNodeIntervention::Release(); }

    bool
    BirthTriggeredIV::Configure(
        const Configuration * inputJson
    )
    {
        IndividualInterventionConfig actual_intervention_config;
        initConfigComplexType("Actual_IndividualIntervention_Config", &actual_intervention_config, Actual_IndividualIntervention_Config_DESC_TEXT);
        initConfigTypeMap("Duration", &max_duration, BT_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f ); // -1 is a convention for indefinite duration

        demographic_restrictions.ConfigureRestrictions( this, inputJson );

        bool ret = BaseNodeIntervention::Configure( inputJson );
        if( ret & !JsonConfigurable::_dryrun )
        {
            demographic_restrictions.CheckConfiguration();
            m_di = InterventionFactory::CreateIntervention( actual_intervention_config._json, inputJson->GetDataLocation(), "Actual_IndividualIntervention_Config", true ); 
        }
        return ret ;
    }

    bool
    BirthTriggeredIV::Distribute(
        INodeEventContext *pNodeEventContext,
        IEventCoordinator2 *pEC
    )
    {
        bool was_distributed = BaseNodeIntervention::Distribute(pNodeEventContext, pEC);
        if (was_distributed)
        {
            LOG_DEBUG_F("Distributed birth-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

            // QI to register ourself as a birth observer
            IIndividualEventBroadcaster * broadcaster = pNodeEventContext->GetIndividualEventBroadcaster();
            release_assert( broadcaster );
            broadcaster->RegisterObserver(this, EventTrigger::Births);
        }
        return was_distributed;
    }

    bool BirthTriggeredIV::notifyOnEvent(
        IIndividualHumanEventContext *pIndiv,
        const EventTrigger& trigger
    )
    {
        LOG_DEBUG("A baby was born, distribute actual_intervention (conditionally)\n");

        assert( parent );

        if( !demographic_restrictions.IsQualified( pIndiv ) )
        {
            return false;
        }

        // want some way to demonstrate selective distribution of calender; no rng available to us, individual property value???
        float demographic_coverage = demographic_restrictions.GetDemographicCoverage();
        LOG_DEBUG_F("demographic_coverage = %f\n", demographic_coverage);
        if( !pIndiv->GetInterventionsContext()->GetParent()->GetRng()->SmartDraw( demographic_coverage ) )
        {
            LOG_DEBUG("Demographic coverage ruled this out\n");
            return false;
        }

        // Query for campaign cost observer interface from INodeEventContext *parent
        ICampaignCostObserver *iCCO;
        if (s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }

        IDistributableIntervention *di = m_di->Clone();
        di->AddRef();
        di->Distribute( pIndiv->GetInterventionsContext(), iCCO );
        di->Release();

        LOG_DEBUG("A birth-triggered intervention was successfully distributed\n");
        return true;
    }

    void BirthTriggeredIV::Unregister()
    {
        // unregister ourself as a birth observer
        IIndividualEventBroadcaster * broadcaster = parent->GetIndividualEventBroadcaster();
        release_assert( broadcaster );
        broadcaster->UnregisterObserver(this, EventTrigger::Births);
        SetExpired( true );
    }

    void BirthTriggeredIV::Update( float dt )
    {
        if (!BaseNodeIntervention::UpdateNodesInterventionStatus())
        {
            Unregister();
            return;
        }

        duration += dt;
        LOG_DEBUG_F("[BirthTriggeredIV], Updating: duration = %f, max_duration = %f.\n", duration, max_duration);
        if( max_duration >= 0 && duration > max_duration )
        {
            Unregister();
        }
    }
}
