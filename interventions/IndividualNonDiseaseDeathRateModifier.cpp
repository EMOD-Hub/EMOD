
#include "stdafx.h"
#include "IndividualNonDiseaseDeathRateModifier.h"

#include "IIndividualHumanContext.h"
#include "InterventionsContainer.h"
#include "IDistribution.h"
#include "DistributionFactory.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "IndividualNonDiseaseDeathRateModifier" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(IndividualNonDiseaseDeathRateModifier)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(IndividualNonDiseaseDeathRateModifier)

    IMPLEMENT_FACTORY_REGISTERED(IndividualNonDiseaseDeathRateModifier)

    IndividualNonDiseaseDeathRateModifier::IndividualNonDiseaseDeathRateModifier() 
        : BaseIntervention()
        , m_DurationToModifier()
        , m_DurationDays(0.0)
        , m_pExpirationDurationDistribution(nullptr)
        , m_ExpirationTimer(0.0)
        , m_ExpirationEvent()
    {
        m_ExpirationTimer.handle = std::bind( &IndividualNonDiseaseDeathRateModifier::Callback, this, std::placeholders::_1 );
    }

    IndividualNonDiseaseDeathRateModifier::IndividualNonDiseaseDeathRateModifier( const IndividualNonDiseaseDeathRateModifier& master )
        : BaseIntervention( master )
        , m_DurationToModifier( master.m_DurationToModifier )
        , m_DurationDays( master.m_DurationDays )
        , m_pExpirationDurationDistribution(nullptr)
        , m_ExpirationTimer( master.m_ExpirationTimer )
        , m_ExpirationEvent( master.m_ExpirationEvent )
    {
        m_ExpirationTimer.handle = std::bind( &IndividualNonDiseaseDeathRateModifier::Callback, this, std::placeholders::_1 );

        if( master.m_pExpirationDurationDistribution != nullptr )
        {
            m_pExpirationDurationDistribution = master.m_pExpirationDurationDistribution->Clone();
        }
    }

    IndividualNonDiseaseDeathRateModifier::~IndividualNonDiseaseDeathRateModifier()
    {
        delete m_pExpirationDurationDistribution;
        m_pExpirationDurationDistribution = nullptr;
    }

    bool IndividualNonDiseaseDeathRateModifier::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 0.0);
        initConfigTypeMap("Duration_To_Modifier",  &m_DurationToModifier, INDDRM_Duration_To_Modifier_DESC_TEXT );

        DistributionFunction::Enum expiration_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Expiration_Duration_Distribution",
                    expiration_function,
                    inputJson,
                    MetadataDescriptor::Enum( "Expiration_Duration_Distribution",
                                              INDDRM_Expiration_Duration_Distribution_DESC_TEXT,
                                              MDD_ENUM_ARGS( DistributionFunction ) ) );
        m_pExpirationDurationDistribution = DistributionFactory::CreateDistribution( this, expiration_function, "Expiration_Duration", inputJson );

        initConfigTypeMap("Expiration_Event", &m_ExpirationEvent, INDDRM_Expiration_Event_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );
        //if( configured && !JsonConfigurable::_dryrun )
        //{
        //}

        return configured;
    }

    bool IndividualNonDiseaseDeathRateModifier::Distribute( IIndividualHumanInterventionsContext *context,
                                    ICampaignCostObserver * pCCO )
    {
        if (s_OK != context->QueryInterface(GET_IID(INonDiseaseDeathRateModifier), (void**)&m_pINDDRM) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "INonDiseaseDeathRateModifier", "IIndividualHumanInterventionsContext" );
        }

        bool distributed =  BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            m_ExpirationTimer = m_pExpirationDurationDistribution->Calculate( context->GetParent()->GetRng() );
        }
        return distributed;
    }

    void IndividualNonDiseaseDeathRateModifier::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );

        if (s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(INonDiseaseDeathRateModifier), (void**)&m_pINDDRM) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context->GetInterventionsContext()",
                                           "INonDiseaseDeathRateModifier",
                                           "IIndividualHumanInterventionsContext" );
        }
    }


    void IndividualNonDiseaseDeathRateModifier::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        float mod = m_DurationToModifier.getValueLinearInterpolation( m_DurationDays, 1.0 );
        m_DurationDays += dt;

        m_pINDDRM->UpdateNonDiseaseDeathRateMod( mod );

        m_ExpirationTimer.Decrement( dt );

        // --------------------------------------------------------------------------
        // --- NOTE: Not considering the expiration of the WaningEffect because that
        // --- is the loss of effectiveness.  We want to expire because the person
        // --- stopped using it.  The person could keep using it even if it is no
        // --- longer providing any benefit.
        // --------------------------------------------------------------------------

        if( expired )
        {
            BroadcastEvent( m_ExpirationEvent );
        }
    }

    void IndividualNonDiseaseDeathRateModifier::Callback( float dt )
    {
        expired = true;
    }

    void IndividualNonDiseaseDeathRateModifier::BroadcastEvent( const EventTrigger& rTrigger )
    {
        IIndividualHumanEventContext* p_hec = parent->GetEventContext();
        IIndividualEventBroadcaster* broadcaster = p_hec->GetNodeEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( p_hec, rTrigger );
    }

    REGISTER_SERIALIZABLE(IndividualNonDiseaseDeathRateModifier);

    void IndividualNonDiseaseDeathRateModifier::serialize(IArchive& ar, IndividualNonDiseaseDeathRateModifier* obj)
    {
        BaseIntervention::serialize( ar, obj );
        IndividualNonDiseaseDeathRateModifier& con = *obj;
        ar.labelElement("m_DurationToModifier"              ) & con.m_DurationToModifier;
        ar.labelElement("m_pExpirationDurationDistribution" ) & con.m_pExpirationDurationDistribution;
        ar.labelElement("m_ExpirationTimer"                 ) & con.m_ExpirationTimer;
        ar.labelElement("m_ExpirationEvent"                 ) & con.m_ExpirationEvent;
    }
}
