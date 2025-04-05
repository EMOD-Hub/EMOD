
#include "stdafx.h"
#include "STIBarrier.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "STIInterventionsContainer.h"  // for ISTIBarrierConsumer methods
#include "IRelationship.h"
#include "Sigmoid.h"
#include "Common.h"
#include "SimulationEnums.h"
#include "IndividualEventContext.h"
#include "DistributionFactory.h"
#include "NodeEventContext.h"


SETUP_LOGGING( "STIBarrier" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(STIBarrier)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(STIBarrier)

    IMPLEMENT_FACTORY_REGISTERED(STIBarrier)
    
    STIBarrier::STIBarrier()
        : BaseIntervention()
        , early(1.0)
        , late(1.0)
        , midyear(2000)
        , rate(1.0)
        , rel_type( RelationshipType::TRANSITORY )
        , m_pUsageDurationDistribution( nullptr )
        , m_UsageTimer(0.0)
        , m_UsageExpirationEvent()
        , ibc( nullptr )
    {
        m_UsageTimer.handle = std::bind( &STIBarrier::TimerCallback, this, std::placeholders::_1 );

        LOG_DEBUG_F( "STIBarrier\n" );
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    STIBarrier::STIBarrier( const STIBarrier& rMaster )
        : BaseIntervention( rMaster )
        , early( rMaster.early )
        , late( rMaster.late )
        , midyear( rMaster.midyear )
        , rate( rMaster.rate )
        , rel_type( rMaster.rel_type )
        , m_pUsageDurationDistribution( nullptr )
        , m_UsageTimer( rMaster.m_UsageTimer )
        , m_UsageExpirationEvent( rMaster.m_UsageExpirationEvent )
        , ibc( nullptr )
    {
        m_UsageTimer.handle = std::bind( &STIBarrier::TimerCallback, this, std::placeholders::_1 );

        if( rMaster.m_pUsageDurationDistribution != nullptr )
        {
            m_pUsageDurationDistribution = rMaster.m_pUsageDurationDistribution->Clone();
        }
    }

    STIBarrier::~STIBarrier()
    {
        delete m_pUsageDurationDistribution;
    }

    bool
    STIBarrier::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, STI_Barrier_Cost_To_Consumer_DESC_TEXT, 0.0,   999999,   10.0 );
        initConfigTypeMap( "Early",            &early,         STI_Barrier_Early_DESC_TEXT,            0.0,      1.0,    1.0 );
        initConfigTypeMap( "Late",             &late,          STI_Barrier_Late_DESC_TEXT,             0.0,      1.0,    1.0 );
        initConfigTypeMap( "MidYear",          &midyear,       STI_Barrier_MidYear_DESC_TEXT,     MIN_YEAR, MAX_YEAR, 2000.0 );
        initConfigTypeMap( "Rate",             &rate,          STI_Barrier_Rate_DESC_TEXT,                    -100.0,    100.0,    1.0 );

        initConfig( "Relationship_Type",
                    rel_type,
                    inputJson,
                    MetadataDescriptor::Enum( "Relationship_Type",
                                              STI_Barrier_Relationship_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(RelationshipType) ) );

        DistributionFunction::Enum usage_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Usage_Duration_Distribution",
                    usage_function,
                    inputJson,
                    MetadataDescriptor::Enum( "Usage_Duration_Distribution",
                                              STI_Barrier_Usage_Duration_Distribution_DESC_TEXT,
                                              MDD_ENUM_ARGS( DistributionFunction ) ) );

        m_pUsageDurationDistribution = DistributionFactory::CreateDistribution( this, usage_function, "Usage_Duration", inputJson );

        initConfigTypeMap("Usage_Expiration_Event", &m_UsageExpirationEvent, STI_Barrier_Usage_Expiration_Event_DESC_TEXT );

        bool is_configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && is_configured )
        {
        }
        return is_configured;
    }

    bool
    STIBarrier::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if( context->GetParent()->GetEventContext()->GetGender() == Gender::FEMALE )
        {
            LOG_WARN("Trying to give STIBarrier (condom usage) to females.\n");
            return false;
        }

        bool distributed =  BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            if (s_OK != context->QueryInterface(GET_IID(ISTIBarrierConsumer), (void**)&ibc) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTIBarrierConsumer", "IIndividualHumanInterventionsContext" );
            }

            m_UsageTimer = m_pUsageDurationDistribution->Calculate( context->GetParent()->GetRng() );
        }
        return distributed;
    }

    void STIBarrier::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        Sigmoid probs( early, late, midyear, rate );
        ibc->UpdateSTIBarrierProbabilitiesByType( rel_type, probs );

        m_UsageTimer.Decrement( dt );

        if( expired )
        {
            IIndividualHumanEventContext* p_hec = parent->GetEventContext();
            IIndividualEventBroadcaster* broadcaster = p_hec->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( p_hec, m_UsageExpirationEvent );
        }
    }

    void STIBarrier::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );

        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(ISTIBarrierConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTIBarrierConsumer", "IIndividualHumanContext" );
        }
    }

    void STIBarrier::TimerCallback( float dt )
    {
        expired = true;
    }

    REGISTER_SERIALIZABLE(STIBarrier);

    void STIBarrier::serialize(IArchive& ar, STIBarrier* obj)
    {
        BaseIntervention::serialize( ar, obj );
        STIBarrier& barrier = *obj;
        ar.labelElement("early"   ) & barrier.early;
        ar.labelElement("late"    ) & barrier.late;
        ar.labelElement("midyear" ) & barrier.midyear;
        ar.labelElement("rate"    ) & barrier.rate;
        ar.labelElement("rel_type") & (uint32_t&)barrier.rel_type;

        ar.labelElement("m_pUsageDurationDistribution") & barrier.m_pUsageDurationDistribution;
        ar.labelElement("m_UsageTimer"                ) & barrier.m_UsageTimer;
        ar.labelElement("m_UsageExpirationEvent"      ) & barrier.m_UsageExpirationEvent;

        // ibc is set in SetContextTo()
    }
}
