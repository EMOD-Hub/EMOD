
#include "stdafx.h"
#include "FemaleContraceptive.h"

#include "IIndividualHumanContext.h"
#include "InterventionsContainer.h"
#include "IDistribution.h"
#include "DistributionFactory.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "FemaleContraceptive" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(FemaleContraceptive)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(FemaleContraceptive)

    IMPLEMENT_FACTORY_REGISTERED(FemaleContraceptive)

    FemaleContraceptive::FemaleContraceptive() 
        : BaseIntervention()
        , m_pWaningEffect(nullptr)
        , m_pUsageDurationDistribution(nullptr)
        , m_UsageTimer(0.0)
        , m_UsageExpirationEvent()
    {
        m_UsageTimer.handle = std::bind( &FemaleContraceptive::Callback, this, std::placeholders::_1 );
    }

    FemaleContraceptive::FemaleContraceptive( const FemaleContraceptive& master )
        : BaseIntervention( master )
        , m_pWaningEffect(nullptr)
        , m_pUsageDurationDistribution(nullptr)
        , m_UsageTimer( master.m_UsageTimer )
        , m_UsageExpirationEvent( master.m_UsageExpirationEvent )
    {
        m_UsageTimer.handle = std::bind( &FemaleContraceptive::Callback, this, std::placeholders::_1 );

        if( master.m_pWaningEffect != nullptr )
        {
            m_pWaningEffect = master.m_pWaningEffect->Clone();
        }
        if( master.m_pUsageDurationDistribution != nullptr )
        {
            m_pUsageDurationDistribution = master.m_pUsageDurationDistribution->Clone();
        }
    }

    FemaleContraceptive::~FemaleContraceptive()
    {
        delete m_pWaningEffect;
        delete m_pUsageDurationDistribution;
        m_pWaningEffect = nullptr;
        m_pUsageDurationDistribution = nullptr;
    }

    bool FemaleContraceptive::Configure( const Configuration * inputJson )
    {
        WaningConfig waning_config;
        initConfigComplexType("Waning_Config",  &waning_config, FC_Waning_Config_DESC_TEXT );

        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 0.0);

        DistributionFunction::Enum usage_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Usage_Duration_Distribution",
                    usage_function,
                    inputJson,
                    MetadataDescriptor::Enum( "Usage_Duration_Distribution",
                                              FC_Usage_Duration_Distribution_DESC_TEXT,
                                              MDD_ENUM_ARGS( DistributionFunction ) ) );
        m_pUsageDurationDistribution = DistributionFactory::CreateDistribution( this, usage_function, "Usage_Duration", inputJson );

        initConfigTypeMap("Usage_Expiration_Event", &m_UsageExpirationEvent, FC_Usage_Expiration_Event_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            m_pWaningEffect = WaningEffectFactory::getInstance()->CreateInstance( waning_config._json,
                                                                                  inputJson->GetDataLocation(),
                                                                                  "Waning_Config" );
        }

        return configured;
    }

    bool FemaleContraceptive::Distribute( IIndividualHumanInterventionsContext *context,
                                    ICampaignCostObserver * pCCO )
    {
        if (s_OK != context->QueryInterface(GET_IID(IBirthRateModifier), (void**)&m_pIBRM) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "IBirthRateModifier", "IIndividualHumanInterventionsContext" );
        }

        bool distributed =  BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            m_UsageTimer = m_pUsageDurationDistribution->Calculate( context->GetParent()->GetRng() );
        }
        return distributed;
    }

    void FemaleContraceptive::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );
        if( m_pWaningEffect != nullptr )
        {
            m_pWaningEffect->SetContextTo( context );
        }

        if (s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IBirthRateModifier), (void**)&m_pIBRM) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context->GetInterventionsContext()",
                                           "IBirthRateModifier",
                                           "IIndividualHumanInterventionsContext" );
        }
    }


    void FemaleContraceptive::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        m_pWaningEffect->Update(dt);

        float mod = 1.0 - m_pWaningEffect->Current();
        m_pIBRM->UpdateBirthRateMod( mod );

        m_UsageTimer.Decrement( dt );

        // --------------------------------------------------------------------------
        // --- NOTE: Not considering the expiration of the WaningEffect because that
        // --- is the loss of effectiveness.  We want to expire because the person
        // --- stopped using it.  The person could keep using it even if it is no
        // --- longer providing any benefit.
        // --------------------------------------------------------------------------

        if( expired )
        {
            BroadcastEvent( m_UsageExpirationEvent );
        }
    }

    void FemaleContraceptive::Callback( float dt )
    {
        expired = true;
    }

    void FemaleContraceptive::BroadcastEvent( const EventTrigger& rTrigger )
    {
        IIndividualHumanEventContext* p_hec = parent->GetEventContext();
        IIndividualEventBroadcaster* broadcaster = p_hec->GetNodeEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( p_hec, rTrigger );
    }

    REGISTER_SERIALIZABLE(FemaleContraceptive);

    void FemaleContraceptive::serialize(IArchive& ar, FemaleContraceptive* obj)
    {
        BaseIntervention::serialize( ar, obj );
        FemaleContraceptive& con = *obj;
        ar.labelElement("m_pWaningEffect"              ) & con.m_pWaningEffect;
        ar.labelElement("m_pUsageDurationDistribution" ) & con.m_pUsageDurationDistribution;
        ar.labelElement("m_UsageTimer"                 ) & con.m_UsageTimer;
        ar.labelElement("m_UsageExpirationEvent"       ) & con.m_UsageExpirationEvent;
    }
}
