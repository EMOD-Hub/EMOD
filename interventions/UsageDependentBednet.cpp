
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
    class WaningConfigList : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        WaningConfigList();

        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) { return e_NOINTERFACE; }
        json::QuickBuilder GetSchema() override;
        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
        virtual bool HasValidDefault() const override { return true; }

        const std::vector<WaningConfig*>& GetList() const
        {
            return m_ConfigList;
        }

    protected:
        std::vector<WaningConfig*> m_ConfigList;
    };

    WaningConfigList::WaningConfigList()
        : JsonConfigurable()
        , m_ConfigList()
    {
    }

    json::QuickBuilder WaningConfigList::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:WaningConfigList" );

        WaningConfig config;

        schema[ ts ] = json::Array();
        schema[ ts ][ 0 ] = config.GetSchema();

        return schema;
    }

    void WaningConfigList::ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key )
    {
        json::QuickInterpreter qi( (*inputJson)[ key ] );
        try
        {
            json::Array config_array_json( qi.As<json::Array>() );
            for( int i = 0; i < config_array_json.Size(); i++ )
            {
                if( (config_array_json[ i ].Type() == json::NULL_ELEMENT) || json_cast<const json::Object&>(config_array_json[ i ]).Empty() )
                {
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Usage_Config_List' element cannot be empty.");
                }

                json::QuickInterpreter config_qi( config_array_json[ i ] );
                WaningConfig* p_config = new WaningConfig( &config_qi );
                m_ConfigList.push_back( p_config );
            }
        }
        catch( const json::Exception & )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), qi, "Expected ARRAY" );
        }
    }


    // ------------------------------------------------------------------------
    // --- UsageDependentBednet
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( UsageDependentBednet, AbstractBednet )
    END_QUERY_INTERFACE_DERIVED( UsageDependentBednet, AbstractBednet )

    IMPLEMENT_FACTORY_REGISTERED( UsageDependentBednet )

    UsageDependentBednet::UsageDependentBednet()
    : AbstractBednet()
    , m_UsageEffectList()
    , m_TriggerReceived()
    , m_TriggerUsing()
    , m_TriggerDiscard()
    , m_ExpirationDuration( nullptr )
    , m_ExpirationTimer(0.0)
    , m_TimerHasExpired(false)
    {
        m_ExpirationTimer.handle = std::bind( &UsageDependentBednet::Callback, this, std::placeholders::_1 );
    }

    UsageDependentBednet::UsageDependentBednet( const UsageDependentBednet& master )
    : AbstractBednet( master )
    , m_UsageEffectList()
    , m_TriggerReceived( master.m_TriggerReceived )
    , m_TriggerUsing( master.m_TriggerUsing )
    , m_TriggerDiscard( master.m_TriggerDiscard )
    , m_ExpirationDuration( master.m_ExpirationDuration->Clone() )
    , m_ExpirationTimer(master.m_ExpirationTimer)
    , m_TimerHasExpired(master.m_TimerHasExpired)
    {
        for( auto p_effect : master.m_UsageEffectList )
        {
            m_UsageEffectList.push_back( p_effect->Clone() );
        }
        m_ExpirationTimer.handle = std::bind( &UsageDependentBednet::Callback, this, std::placeholders::_1 );
    }

    UsageDependentBednet::~UsageDependentBednet()
    {
        for( auto p_effect : m_UsageEffectList )
        {
            delete p_effect;
        }
        m_UsageEffectList.clear();
        delete m_ExpirationDuration;
    }

    bool UsageDependentBednet::ConfigureUsage( const Configuration * inputJson )
    {
        WaningConfigList usage_config_list;
        initConfigComplexType( "Usage_Config_List", &usage_config_list, UDBednet_Usage_Config_List_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()

        if( configured && !JsonConfigurable::_dryrun )
        {
            int i = 0;
            for( auto p_config : usage_config_list.GetList() )
            {
                std::stringstream param_name;
                param_name << "Usage_Config_List[" << i << "]";

                IWaningEffect* p_effect = WaningEffectFactory::getInstance()->CreateInstance( p_config->_json,
                                                                                              inputJson->GetDataLocation(),
                                                                                              param_name.str().c_str() );
                m_UsageEffectList.push_back( p_effect );
                ++i;
            }
        }

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
        for( auto p_effect : m_UsageEffectList )
        {
            p_effect->Update( dt );
        }
    }

    float UsageDependentBednet::GetEffectUsage() const
    {
        float usage_effect = 1.0;
        for( auto p_effect : m_UsageEffectList )
        {
            usage_effect *= p_effect->Current();
        }
        return usage_effect;
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
        for( auto p_effect : m_UsageEffectList )
        {
            p_effect->SetContextTo( context );
        }
    }


    REGISTER_SERIALIZABLE( UsageDependentBednet );

    void UsageDependentBednet::serialize( IArchive& ar, UsageDependentBednet* obj )
    {
        AbstractBednet::serialize( ar, obj );
        UsageDependentBednet& bednet = *obj;

        ar.labelElement( "m_UsageEffectList" ) & bednet.m_UsageEffectList;
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
