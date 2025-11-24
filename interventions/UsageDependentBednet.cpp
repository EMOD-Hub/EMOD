
#include "stdafx.h"
#include "UsageDependentBednet.h"
#include "Log.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "RANDOM.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "UsageDependentBednet" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- UsageDependentBednet
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( UsageDependentBednet, AbstractBednet )
    END_QUERY_INTERFACE_DERIVED( UsageDependentBednet, AbstractBednet )

    IMPLEMENT_FACTORY_REGISTERED( UsageDependentBednet )

    UsageDependentBednet::UsageDependentBednet()
        : AbstractBednet()
        , m_EffectCollection()
        , m_TriggerReceived()
        , m_TriggerUsing()
        , m_TriggerDiscard()
        , m_ExpirationDuration(nullptr)
        , m_ExpirationTimer(0.0)
        , m_TimerHasExpired(false)
    {
        m_ExpirationTimer.handle = std::bind( &UsageDependentBednet::Callback, this, std::placeholders::_1 );
    }

    UsageDependentBednet::UsageDependentBednet( const UsageDependentBednet& rOrig )
        : AbstractBednet( rOrig )
        , m_EffectCollection()
        , m_TriggerReceived( rOrig.m_TriggerReceived )
        , m_TriggerUsing( rOrig.m_TriggerUsing )
        , m_TriggerDiscard( rOrig.m_TriggerDiscard )
        , m_ExpirationDuration( rOrig.m_ExpirationDuration->Clone() )
        , m_ExpirationTimer(rOrig.m_ExpirationTimer)
        , m_TimerHasExpired(rOrig.m_TimerHasExpired)
    {
        for( int i = 0 ; i < rOrig.m_EffectCollection.Size() ; ++i )
        {
            m_EffectCollection.Add( rOrig.m_EffectCollection[ i ]->Clone() );
        }
        m_ExpirationTimer.handle = std::bind( &UsageDependentBednet::Callback, this, std::placeholders::_1 );
    }

    UsageDependentBednet::~UsageDependentBednet()
    {
        delete m_ExpirationDuration;
    }

    bool UsageDependentBednet::ConfigureUsage( const Configuration * inputJson )
    {
        initConfigComplexType( "Usage_Config_List", &m_EffectCollection, UDBednet_Usage_Config_List_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()

        return configured;
    }

    bool UsageDependentBednet::ConfigureEvents( const Configuration * inputJson )
    {
        DistributionFunction::Enum expiration_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig("Expiration_Period_Distribution", expiration_function, inputJson, MetadataDescriptor::Enum("Expiration_Period_Distribution", Expiration_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)));                 
        m_ExpirationDuration = DistributionFactory::CreateDistribution( this, expiration_function, "Expiration_Period", inputJson );

        initConfigTypeMap( "Received_Event", &m_TriggerReceived, UDBednet_Received_Event_DESC_TEXT );
        initConfigTypeMap( "Using_Event",    &m_TriggerUsing,    UDBednet_Using_Event_DESC_TEXT );
        initConfigTypeMap( "Discard_Event",  &m_TriggerDiscard,  UDBednet_Discard_Event_DESC_TEXT );

        return JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()
    }

    bool UsageDependentBednet::Distribute( IIndividualHumanInterventionsContext *context,
                                           ICampaignCostObserver * const pCCO )
    {
        bool distributed = AbstractBednet::Distribute( context, pCCO );
        if( distributed )
        {
            m_ExpirationTimer = m_ExpirationDuration->Calculate( context->GetParent()->GetRng() );
            BroadcastEvent( m_TriggerReceived );

            // ----------------------------------------------------------------------------
            // --- Assuming dt=1.0 and decrementing timer so that a timer of zero expires
            // --- when it is distributed but is not used.  A timer of one should be used
            // --- the day it is distributed but expire:
            // ---    distributed->used->expired on all same day
            // ----------------------------------------------------------------------------
            m_ExpirationTimer.Decrement( 1.0 );
        }
        return distributed;
    }

    bool UsageDependentBednet::IsUsingBednet() const
    {
        float usage_effect = GetEffectUsage();

        // Check expiratin in case it expired when it was distributed
        bool is_using = !m_TimerHasExpired && parent->GetRng()->SmartDraw( usage_effect );

        if( is_using )
        {
            BroadcastEvent( m_TriggerUsing );
        }

        return is_using;
    }

    void UsageDependentBednet::UpdateUsage( float dt )
    {
        for(int i=0; i < m_EffectCollection.Size(); ++i)
        {
            m_EffectCollection[i]->Update(dt);
        }
    }

    float UsageDependentBednet::GetEffectUsage() const
    {
        float current = 1.0f;

        for(int i=0; i < m_EffectCollection.Size(); ++i)
        {
            current *= m_EffectCollection[i]->Current();
        }

        return current;
    }

    bool UsageDependentBednet::CheckExpiration( float dt )
    {
        m_ExpirationTimer.Decrement( dt );

        return m_TimerHasExpired;
    }

    void UsageDependentBednet::Callback( float dt )
    {
        m_TimerHasExpired = true;
    }

    void UsageDependentBednet::SetExpired( bool isExpired )
    {
        AbstractBednet::SetExpired( isExpired );
        if( Expired() )
        {
            BroadcastEvent( m_TriggerDiscard );
        }
    }

    void UsageDependentBednet::SetContextTo( IIndividualHumanContext *context )
    {
        AbstractBednet::SetContextTo( context );
        for(int i=0; i < m_EffectCollection.Size(); ++i)
        {
            m_EffectCollection[i]->SetContextTo(context);
        }
    }

    REGISTER_SERIALIZABLE( UsageDependentBednet );

    void UsageDependentBednet::serialize( IArchive& ar, UsageDependentBednet* obj )
    {
        AbstractBednet::serialize( ar, obj );
        UsageDependentBednet& bednet = *obj;

        ar.labelElement( "m_EffectCollection") & bednet.m_EffectCollection;
        ar.labelElement( "m_TriggerReceived" ) & bednet.m_TriggerReceived;
        ar.labelElement( "m_TriggerUsing"    ) & bednet.m_TriggerUsing;
        ar.labelElement( "m_TriggerDiscard"  ) & bednet.m_TriggerDiscard;
        ar.labelElement( "m_ExpirationTimer" ) & bednet.m_ExpirationTimer;
        ar.labelElement( "m_TimerHasExpired" ) & bednet.m_TimerHasExpired;
        // this needs to be serialized incase it was serialized before it was distributed
        ar.labelElement( "m_ExpirationDuration" ) & bednet.m_ExpirationDuration;
    }

    // ------------------------------------------------------------------------
    // --- UsageDependentBednet
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( MultiInsecticideUsageDependentBednet, UsageDependentBednet )
    END_QUERY_INTERFACE_DERIVED( MultiInsecticideUsageDependentBednet, UsageDependentBednet )

    IMPLEMENT_FACTORY_REGISTERED( MultiInsecticideUsageDependentBednet )

    MultiInsecticideUsageDependentBednet::MultiInsecticideUsageDependentBednet()
        : UsageDependentBednet()
    {
    }

    MultiInsecticideUsageDependentBednet::MultiInsecticideUsageDependentBednet( const MultiInsecticideUsageDependentBednet& master )
        : UsageDependentBednet( master )
    {
    }

    MultiInsecticideUsageDependentBednet::~MultiInsecticideUsageDependentBednet()
    {
    }

    bool MultiInsecticideUsageDependentBednet::ConfigureBlockingAndKilling( const Configuration * inputJson )
    {
        InsecticideWaningEffectCollection* p_iwec = new InsecticideWaningEffectCollection(false,true,true,true);

        initConfigComplexCollectionType( "Insecticides", p_iwec, MI_UDBednet_Insecticides_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()
        if( !JsonConfigurable::_dryrun && configured )
        {
            p_iwec->CheckConfiguration();
            m_pInsecticideWaningEffect = p_iwec;
        }
        return configured;
    }

    REGISTER_SERIALIZABLE( MultiInsecticideUsageDependentBednet );

    void MultiInsecticideUsageDependentBednet::serialize( IArchive& ar, MultiInsecticideUsageDependentBednet* obj )
    {
        UsageDependentBednet::serialize( ar, obj );
        MultiInsecticideUsageDependentBednet& bednet = *obj;
    }
}
