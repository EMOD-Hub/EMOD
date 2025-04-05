
#include "stdafx.h"
#include "DelayedIntervention.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "NodeEventContext.h"
#include "RANDOM.h"
#include "DistributionFactory.h"


SETUP_LOGGING( "DelayedIntervention" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(DelayedIntervention)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(DelayedIntervention)

    IMPLEMENT_FACTORY_REGISTERED(DelayedIntervention)

    void DelayedIntervention::PreConfigure( const Configuration * inputJson )
    {
        initConfigTypeMap("Coverage", &coverage, DI_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f);
    }

    void DelayedIntervention::DistributionConfigure( const Configuration * inputJson )
    {
    }

    void DelayedIntervention::InterventionConfigure( const Configuration * inputJson )
    {
        initConfigComplexType("Actual_IndividualIntervention_Configs", &actual_intervention_config, DI_Actual_IndividualIntervention_Configs_DESC_TEXT);
    }

    void DelayedIntervention::InterventionValidate( const std::string& rDataLocation )
    {
        InterventionFactory::CreateInterventionList( actual_intervention_config._json, rDataLocation, "Actual_IndividualIntervention_Configs", m_Interventions );
    }

    void DelayedIntervention::DelayValidate()
    {
    }

    bool DelayedIntervention::Configure( const Configuration * inputJson )
    {
        PreConfigure(inputJson);
        DistributionConfigure(inputJson);
        InterventionConfigure(inputJson);

        DistributionFunction::Enum delay_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Delay_Period_Distribution", delay_function, inputJson, MetadataDescriptor::Enum( "Delay_Distribution", Delay_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );
        delay_distribution = DistributionFactory::CreateDistribution( this, delay_function, "Delay_Period", inputJson );

        bool retValue = BaseIntervention::Configure( inputJson );
        if( retValue && !JsonConfigurable::_dryrun )
        { 
            InterventionValidate( inputJson->GetDataLocation() );
            DelayValidate();
        }
        return retValue;
    }

    bool DelayedIntervention::Distribute(
        IIndividualHumanInterventionsContext *context, // interventions container usually
        ICampaignCostObserver * const pICCO
    )
    {
        // Distribute this intervention, which contains the callback to set the parent context
        if( !BaseIntervention::Distribute(context, pICCO) )
            return false;

        // If individual isn't covered, immediately set DelayedIntervention to expired without distributing "actual" IVs
        if( !parent->GetRng()->SmartDraw( coverage ) )
        {
            LOG_DEBUG_F("Random draw outside of %0.2f covered fraction in DelayedIntervention.\n", coverage);
            expired = true;
            return false;
        }

        CalculateDelay();

        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", float(remaining_delay_days), DistributionFunction::pairs::lookup_key(delay_distribution->GetType()));
        return true;
    }

    void
    DelayedIntervention::CalculateDelay()
    {
        remaining_delay_days = delay_distribution->Calculate( parent->GetRng() );
        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", float(remaining_delay_days), DistributionFunction::pairs::lookup_key(delay_distribution->GetType()));
    }

    DelayedIntervention::DelayedIntervention()
    : BaseIntervention()
    , remaining_delay_days(0.0)
    , coverage(1.0)
    , delay_distribution( nullptr )
    , actual_intervention_config()
    , m_Interventions()
    {
        remaining_delay_days.handle = std::bind( &DelayedIntervention::Callback, this, std::placeholders::_1 );
    }

    DelayedIntervention::DelayedIntervention( const DelayedIntervention& master )
        : BaseIntervention( master )
        , remaining_delay_days( master.remaining_delay_days )
        , coverage( master.coverage )
        , delay_distribution( master.delay_distribution->Clone() )
        //, actual_intervention_config( master.actual_intervention_config )
    { 
        actual_intervention_config = master.actual_intervention_config;
        remaining_delay_days.handle = std::bind( &DelayedIntervention::Callback, this, std::placeholders::_1 );

        for( auto p_intervention : master.m_Interventions )
        {
            m_Interventions.push_back( p_intervention->Clone() );
        }
    }

    void DelayedIntervention::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        remaining_delay_days.Decrement( dt );
    }

    void DelayedIntervention::Callback( float dt )
    {
        // don't give expired intervention.  should be cleaned up elsewhere anyways, though.
        if(expired)
            return;

        expired = true;

        // Now make sure cost gets reported back to node
        ICampaignCostObserver* pICCO;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), reinterpret_cast<void**>(&pICCO)) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
        }
        LOG_DEBUG_F("interventions array size = %d\n", m_Interventions.size());

        for( auto p_intervention: m_Interventions )
        {
            IDistributableIntervention *di = p_intervention->Clone();
            di->AddRef();
            di->Distribute( parent->GetInterventionsContext(), pICCO );
            di->Release();
        }
    }

    DelayedIntervention::~DelayedIntervention()
    { 
        LOG_DEBUG("Destructing DelayedIntervention\n");
        delete delay_distribution;
        for( auto p_intervention : m_Interventions )
        {
            delete p_intervention;
        }
    }

    REGISTER_SERIALIZABLE(DelayedIntervention);

    void DelayedIntervention::serialize(IArchive& ar, DelayedIntervention* obj)
    {
        BaseIntervention::serialize( ar, obj );

        DelayedIntervention& intervention = *obj;

        ar.labelElement("remaining_delay_days"      ) & intervention.remaining_delay_days;
        ar.labelElement("coverage"                  ) & intervention.coverage;
        ar.labelElement("delay_distribution"        ) & intervention.delay_distribution;
        ar.labelElement("actual_intervention_config") & intervention.actual_intervention_config;
        ar.labelElement("m_Interventions"           ) & intervention.m_Interventions;
    }
}
