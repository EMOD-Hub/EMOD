
#include "stdafx.h"
#include "HealthSeekingBehavior.h"

#include "InterventionEnums.h"
#include "HealthSeekingBehavior.h"
#include "NodeEventContext.h"    // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "SimpleHealthSeekingBehavior" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleHealthSeekingBehavior)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleHealthSeekingBehavior)

    IMPLEMENT_FACTORY_REGISTERED(SimpleHealthSeekingBehavior)

    bool
    SimpleHealthSeekingBehavior::Configure(
        const Configuration * inputJson
    )
    {
        IndividualInterventionConfig actual_intervention_config;

        initConfig( "Event_Or_Config", use_event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        if( use_event_or_config == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Actual_IndividualIntervention_Event", &actual_intervention_event, HSB_Actual_IndividualIntervention_Event_DESC_TEXT );
        }

        if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Actual_IndividualIntervention_Config", &actual_intervention_config, HSB_Actual_IndividualIntervention_Config_DESC_TEXT );
        }

        initConfigTypeMap( "Tendency", &probability_of_seeking, HSB_Tendency_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Single_Use", &single_use, HSB_Single_Use_DESC_TEXT, true ); //true means it will expire after a single use

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( actual_intervention_event.IsUninitialized() &&
                (actual_intervention_config._json.Type() == ElementType::NULL_ELEMENT) )
            {
                const char* msg = "You must define either Actual_IndividualIntervention_Event or Actual_IndividualIntervention_Config";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg );
            }

            if( use_event_or_config == EventOrConfig::Config )
            {
                m_di = InterventionFactory::CreateIntervention( actual_intervention_config._json, inputJson->GetDataLocation(), "Actual_IndividualIntervention_Config" ); 
            }
        }
        return ret ;
    }

    SimpleHealthSeekingBehavior::SimpleHealthSeekingBehavior()
    : BaseIntervention()
    , probability_of_seeking(0)
    , use_event_or_config( EventOrConfig::Event )
    , actual_intervention_event()
    , m_di( nullptr )
    , single_use(true)
    {
    }

    SimpleHealthSeekingBehavior::SimpleHealthSeekingBehavior( const SimpleHealthSeekingBehavior& rMaster )
    : BaseIntervention( rMaster )
    , probability_of_seeking( rMaster.probability_of_seeking )
    , use_event_or_config( rMaster.use_event_or_config )
    , actual_intervention_event( rMaster.actual_intervention_event )
    , m_di( nullptr )
    , single_use( rMaster.single_use )
    {
        if( rMaster.m_di != nullptr )
        {
            m_di = rMaster.m_di->Clone();
        }
    }

    SimpleHealthSeekingBehavior::~SimpleHealthSeekingBehavior()
    {
        delete m_di;
    }

    // Each time this is called, the HSB intervention is going to decide for itself if
    // health should be sought. For start, just do it based on roll of the dice. If yes,
    // an intervention needs to be created (from 'actual' config) and distributed to the
    // individual who owns us.
    void SimpleHealthSeekingBehavior::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        if(expired) return; // don't give expired intervention.  should be cleaned up elsewhere anyways, though.

        LOG_DEBUG_F("Individual %d is seeking with tendency %f \n", parent->GetSuid().data, probability_of_seeking*dt);

        if( parent->GetRng()->SmartDraw( 1- pow( 1-probability_of_seeking, dt ) ) )
        //if( parent->GetRng()->e() < EXPCDF( probability_of_seeking * dt ) )
        {
            LOG_DEBUG_F("SimpleHealthSeekingBehavior is going to give the actual intervention to individual %d\n", parent->GetSuid().data );

            if( use_event_or_config == EventOrConfig::Event )
            {
                IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is broadcasting the actual intervention event to individual %d.\n", parent->GetSuid().data );
                broadcaster->TriggerObservers( parent->GetEventContext(), actual_intervention_event );
            }
            else 
            {
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is distributing actual intervention to individual %d.\n", parent->GetSuid().data );

                // Now make sure cost gets reported back to node
                ICampaignCostObserver* pICCO;
                if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                                   "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
                }
                IDistributableIntervention* di = m_di->Clone();
                di->AddRef();
                di->Distribute( parent->GetInterventionsContext(), pICCO );
                di->Release();

                //Expire();

            }
            if( single_use )
            {
                LOG_DEBUG_F( "HSB for individual %d is expiring after single use.\n", parent->GetSuid().data );
                expired = true;
            }
            else
            {
                LOG_DEBUG_F( "HSB for individual %d will not expire after use.\n", parent->GetSuid().data );
            }
        }
        else
        {
            LOG_DEBUG("SimpleHealthSeekingBehavior did not distribute\n");
        }
    }

    REGISTER_SERIALIZABLE(SimpleHealthSeekingBehavior);

    void SimpleHealthSeekingBehavior::serialize(IArchive& ar, SimpleHealthSeekingBehavior* obj)
    {
        BaseIntervention::serialize(ar, obj);
        SimpleHealthSeekingBehavior& intervention = *obj;
        ar.labelElement("probability_of_seeking"   ) & intervention.probability_of_seeking;
        ar.labelElement("use_event_or_config"      ) & (uint32_t&)intervention.use_event_or_config;
        ar.labelElement("m_di"                     ) & intervention.m_di;
        ar.labelElement("actual_intervention_event") & intervention.actual_intervention_event;
        ar.labelElement("single_use"               ) & intervention.single_use;
    }
}
