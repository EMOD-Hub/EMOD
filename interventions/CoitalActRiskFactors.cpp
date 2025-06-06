
#include "stdafx.h"
#include "CoitalActRiskFactors.h"

#include <typeinfo>

#include "NodeEventContext.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "ISTIInterventionsContainer.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "CoitalActRisk" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( CoitalActRiskFactors )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( CoitalActRiskFactors )

    IMPLEMENT_FACTORY_REGISTERED( CoitalActRiskFactors )

    CoitalActRiskFactors::CoitalActRiskFactors()
        : BaseIntervention()
        , m_AcquisitionMultiplier(1.0)
        , m_TransmissionMultiplier(1.0)
        , m_ExpirationTrigger()
        , m_pExpirationDuration(nullptr)
        , m_ExpirationTimer()
        , m_TimerHasExpired(false)
        , m_pRiskData(nullptr)
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );

        m_ExpirationTimer.handle = std::bind( &CoitalActRiskFactors::TimerCallback, this, std::placeholders::_1 );
    }

    CoitalActRiskFactors::CoitalActRiskFactors( const CoitalActRiskFactors& rMaster )
        : BaseIntervention( rMaster )
        , m_AcquisitionMultiplier( rMaster.m_AcquisitionMultiplier )
        , m_TransmissionMultiplier( rMaster.m_TransmissionMultiplier )
        , m_ExpirationTrigger( rMaster.m_ExpirationTrigger )
        , m_pExpirationDuration( nullptr )
        , m_ExpirationTimer()
        , m_TimerHasExpired( false )
        , m_pRiskData( nullptr )
    {
        m_ExpirationTimer.handle = std::bind( &CoitalActRiskFactors::TimerCallback, this, std::placeholders::_1 );

        m_pExpirationDuration = rMaster.m_pExpirationDuration->Clone();
    }

    CoitalActRiskFactors::~CoitalActRiskFactors()
    {
        delete m_pExpirationDuration;
    }

    bool CoitalActRiskFactors::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10.0 );

        initConfigTypeMap( "Acquisition_Multiplier",  &m_AcquisitionMultiplier,  CARF_Acquisition_Multiplier_DESC_TEXT,  0.0f, 100.0f, 1.0f );
        initConfigTypeMap( "Transmission_Multiplier", &m_TransmissionMultiplier, CARF_Transmission_Multiplier_DESC_TEXT, 0.0f, 100.0f, 1.0f );

        DistributionFunction::Enum expiration_type( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Expiration_Period_Distribution", expiration_type, inputJson, MetadataDescriptor::Enum( "Expiration_Period_Distribution", Expiration_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );

        m_pExpirationDuration = DistributionFactory::CreateDistribution( this, expiration_type, "Expiration_Period", inputJson );

        initConfigTypeMap("Expiration_Event_Trigger", &m_ExpirationTrigger, CARF_Expiration_Event_Trigger_DESC_TEXT );

        bool configured =  BaseIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
        }
        return configured;
    }

    bool CoitalActRiskFactors::Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO )
    {
        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            m_ExpirationTimer = m_pExpirationDuration->Calculate( context->GetParent()->GetRng() );

            if( s_OK != context->QueryInterface( GET_IID( ICoitalActRiskData ), (void**)&m_pRiskData ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "context", "ICoitalActRiskData", "IIndividualHumanInterventionsContext" );
            }
        }
        return distributed;
    }

    void CoitalActRiskFactors::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        m_pRiskData->UpdateCoitalActRiskFactors( m_AcquisitionMultiplier, m_TransmissionMultiplier );

        m_ExpirationTimer.Decrement( dt );

        if( m_TimerHasExpired )
        {
            expired = true;
            if( !m_ExpirationTrigger.IsUninitialized() )
            {
                IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
                broadcaster->TriggerObservers( parent->GetEventContext(), m_ExpirationTrigger );
            }
        }
    }

    void CoitalActRiskFactors::TimerCallback( float dt )
    {
        m_TimerHasExpired = true;
    }

    void CoitalActRiskFactors::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );

        if( s_OK != context->GetInterventionsContext()->QueryInterface( GET_IID( ICoitalActRiskData ), (void**)&m_pRiskData ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context->GetInterventionsContext()",
                                           "ICoitalActRiskData",
                                           "IIndividualHumanInterventionsContext" );
        }
    }

    REGISTER_SERIALIZABLE( CoitalActRiskFactors );

    void CoitalActRiskFactors::serialize( IArchive& ar, CoitalActRiskFactors* obj )
    {
        BaseIntervention::serialize( ar, obj );
        CoitalActRiskFactors& rf = *obj;
        ar.labelElement( "m_AcquisitionMultiplier"  ) & rf.m_AcquisitionMultiplier;
        ar.labelElement( "m_TransmissionMultiplier" ) & rf.m_TransmissionMultiplier;
        ar.labelElement( "m_ExpirationTrigger"      ) & rf.m_ExpirationTrigger;
        ar.labelElement( "m_ExpirationTimer"        ) & rf.m_ExpirationTimer;
        ar.labelElement( "m_TimerHasExpired"        ) & rf.m_TimerHasExpired;

        // m_pExpirationDuration does not need to be serialized since
        // it is not needed after the intervention has been distributed.

        // m_pRiskFactorUpdater is set in SetContextTo()
    }
}
