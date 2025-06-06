
#include "stdafx.h"
#include "Bednet.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "VectorInterventionsContainerContexts.h"  // for IBednetConsumer methods
#include "Log.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "Insecticides.h"

SETUP_LOGGING( "SimpleBednet" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- AbstractBednet
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( AbstractBednet )
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY( AbstractBednet )

    AbstractBednet::AbstractBednet()
        : BaseIntervention()
        , m_pInsecticideWaningEffect( nullptr )
        , m_pConsumer( nullptr )
    {
        initSimTypes( 2, "MALARIA_SIM", "VECTOR_SIM" );
    }

    AbstractBednet::AbstractBednet( const AbstractBednet& master )
        : BaseIntervention( master )
        , m_pInsecticideWaningEffect( nullptr )
        , m_pConsumer( nullptr )
    {
        if( master.m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect = master.m_pInsecticideWaningEffect->Clone();
        }
    }

    AbstractBednet::~AbstractBednet()
    {
        delete m_pInsecticideWaningEffect;
    }

    bool AbstractBednet::Configure( const Configuration * inputJson )
    {
        bool configured = true;

        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );

        if( configured || JsonConfigurable::_dryrun )
        {
            configured = ConfigureBlockingAndKilling( inputJson );
        }
        if( configured || JsonConfigurable::_dryrun )
        {
            configured = ConfigureUsage( inputJson );
        }
        if( configured || JsonConfigurable::_dryrun )
        {
            configured = ConfigureEvents( inputJson );
        }

        return configured;
    }

    bool AbstractBednet::ConfigureBlockingAndKilling( const Configuration * inputJson )
    {
        WaningConfig killing_config;
        WaningConfig blocking_config;
        WaningConfig repelling_config;
        InsecticideName name;

        initConfigComplexType( "Killing_Config", &killing_config, SB_Killing_Config_DESC_TEXT );
        initConfigComplexType( "Blocking_Config", &blocking_config, SB_Blocking_Config_DESC_TEXT );
        initConfigComplexType( "Repelling_Config", &repelling_config, Repelling_Config_DESC_TEXT );
        initConfigTypeMap( "Insecticide_Name", &name, INT_Insecticide_Name_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );

        if( configured && !JsonConfigurable::_dryrun )
        {
            if( (killing_config._json.Type() == json::NULL_ELEMENT) || json_cast<const json::Object&>(killing_config._json).Empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Killing_Config' must be defined.");
            }
            if( (blocking_config._json.Type() == json::NULL_ELEMENT) || json_cast<const json::Object&>(blocking_config._json).Empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Blocking_Config' must be defined.");
            }
            if( (repelling_config._json.Type() == json::NULL_ELEMENT) || json_cast<const json::Object&>(repelling_config._json).Empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Repelling_Config' must be defined.");
            }

            WaningConfig empty_config;
            m_pInsecticideWaningEffect = new InsecticideWaningEffect( empty_config, repelling_config, blocking_config, killing_config );

            name.CheckConfiguration( GetName().ToString(), "Insecticide_Name");
            m_pInsecticideWaningEffect->SetName( name );
        }

        return configured;
    }

    bool AbstractBednet::Distribute( IIndividualHumanInterventionsContext *context,
                                     ICampaignCostObserver * const pCCO )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        std::list<IDistributableIntervention*> net_list = context->GetInterventionsByName( GetName() );
        for( IDistributableIntervention* p_bednet : net_list )
        {
            p_bednet->SetExpired( true );
        }

        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            SetContextTo( context->GetParent() );
        }
        return distributed;
    }

    void AbstractBednet::Update( float dt )
    {
        if( Expired() ) return;

        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        UpdateUsage( dt );
        UpdateBlockingAndKilling( dt );

        if( IsUsingBednet() )
        {
            UseBednet();
        }

        if( CheckExpiration( dt ) )
        {
            SetExpired( true );
        }
    }

    ReportInterventionData AbstractBednet::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

        // call the method in AbstractBednet so that SimpleBednet does not include coverage
        data.efficacy_repelling = AbstractBednet::GetEffectRepelling().GetSum();
        data.efficacy_blocking  = AbstractBednet::GetEffectBlocking().GetSum();
        data.efficacy_killing   = AbstractBednet::GetEffectKilling().GetSum();
        data.efficacy_usage     = GetEffectUsage();

        return data;
    }

    GeneticProbability AbstractBednet::GetEffectKilling() const
    {
        return m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING );
    }

    GeneticProbability AbstractBednet::GetEffectBlocking() const
    {
        return m_pInsecticideWaningEffect->GetCurrent( ResistanceType::BLOCKING );
    }

    GeneticProbability AbstractBednet::GetEffectRepelling() const
    {
        return m_pInsecticideWaningEffect->GetCurrent( ResistanceType::REPELLING );
    }

    void AbstractBednet::UseBednet()
    {
        GeneticProbability current_killingrate  = GetEffectKilling();
        GeneticProbability current_blockingrate = GetEffectBlocking();
        GeneticProbability current_repellingrate = GetEffectRepelling();

        release_assert( m_pConsumer != nullptr );

        m_pConsumer->UpdateProbabilityOfKilling( current_killingrate );
        m_pConsumer->UpdateProbabilityOfBlocking( current_blockingrate  );
        m_pConsumer->UpdateProbabilityOfHouseRepelling( current_repellingrate );
    }

    void AbstractBednet::UpdateBlockingAndKilling( float dt )
    {
        m_pInsecticideWaningEffect->Update( dt );
    }

    void AbstractBednet::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );
        if( s_OK != context->GetInterventionsContext()->QueryInterface( GET_IID( IBednetConsumer ), (void**)&m_pConsumer ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBednetConsumer", "IIndividualHumanContext" );
        }
        m_pInsecticideWaningEffect->SetContextTo( context );
    }

    void AbstractBednet::BroadcastEvent( const EventTrigger& trigger ) const
    {
        if( !trigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), trigger );
        }
    }

    void AbstractBednet::serialize( IArchive& ar, AbstractBednet* obj )
    {
        BaseIntervention::serialize( ar, obj );
        AbstractBednet& bednet = *obj;
        ar.labelElement( "m_pInsecticideWaningEffect" ) & bednet.m_pInsecticideWaningEffect;
    }

    // ------------------------------------------------------------------------
    // --- SimpleBednet
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( SimpleBednet, AbstractBednet )
    END_QUERY_INTERFACE_DERIVED( SimpleBednet, AbstractBednet )

    IMPLEMENT_FACTORY_REGISTERED(SimpleBednet)
    
    SimpleBednet::SimpleBednet()
    : AbstractBednet()
    , m_pEffectUsage( nullptr )
    {
    }

    SimpleBednet::SimpleBednet( const SimpleBednet& master )
    : AbstractBednet( master )
    , m_pEffectUsage( nullptr )
    {
        if( master.m_pEffectUsage != nullptr )
        {
            m_pEffectUsage = master.m_pEffectUsage->Clone();
        }
    }

    SimpleBednet::~SimpleBednet()
    {
        delete m_pEffectUsage;
    }

    bool SimpleBednet::ConfigureUsage( const Configuration * inputJson )
    {
        WaningConfig usage_config;
        initConfigComplexType( "Usage_Config", &usage_config, SB_Usage_Config_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()

        if( configured && !JsonConfigurable::_dryrun )
        {
            m_pEffectUsage = WaningEffectFactory::getInstance()->CreateInstance( usage_config._json,
                                                                                 inputJson->GetDataLocation(),
                                                                                 "Usage_Config" );
        }

        return configured;
    }

    bool SimpleBednet::IsUsingBednet() const
    {
        // -----------------------------------------------------------------------------------
        // --- true because we use the usage effect to adjust the killing and blocking effects
        // -----------------------------------------------------------------------------------
        return true;
    }

    void SimpleBednet::UpdateUsage( float dt )
    {
        m_pEffectUsage->Update( dt );
    }

    GeneticProbability SimpleBednet::GetEffectKilling() const
    {
        return AbstractBednet::GetEffectKilling() * GetEffectUsage();
    }

    GeneticProbability SimpleBednet::GetEffectBlocking() const
    {
        return AbstractBednet::GetEffectBlocking() * GetEffectUsage();
    }

    GeneticProbability SimpleBednet::GetEffectRepelling() const
    {
        return AbstractBednet::GetEffectRepelling() * GetEffectUsage();
    }

    float SimpleBednet::GetEffectUsage() const
    {
        return m_pEffectUsage->Current();
    }

    bool SimpleBednet::CheckExpiration( float dt )
    {
        return m_pEffectUsage->Expired();
    }

    void SimpleBednet::SetContextTo( IIndividualHumanContext *context )
    {
        AbstractBednet::SetContextTo( context );
        m_pEffectUsage->SetContextTo( context );
    }


    REGISTER_SERIALIZABLE(SimpleBednet);

    void SimpleBednet::serialize(IArchive& ar, SimpleBednet* obj)
    {
        AbstractBednet::serialize( ar, obj );
        SimpleBednet& bednet = *obj;
        ar.labelElement("m_pEffectUsage") & bednet.m_pEffectUsage;
    }
}
