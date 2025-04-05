
#include "stdafx.h"
#include "Diagnostics.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "SimpleDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleDiagnostic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(SimpleDiagnostic)

    EventOrConfig::Enum
    SimpleDiagnostic::getEventOrConfig(
        const Configuration * inputJson
    )
    {
        EventOrConfig::Enum event_or_config = EventOrConfig::Event;
        initConfig( "Event_Or_Config", event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        return event_or_config;
    }

    // This is deliberately broken out into separate function so that derived classes can invoke
    // without calling SD::Configure -- if they want Positive_Dx_Config/Event but not Treatment_Fraction.
    void SimpleDiagnostic::ConfigurePositiveEvent( const Configuration * inputJson )
    {
        if( use_event_or_config == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Positive_Diagnosis_Event", &positive_diagnosis_event, Positive_Diagnosis_Event_DESC_TEXT );
        }
    }
    
    void SimpleDiagnostic::ConfigurePositiveConfig( const Configuration * inputJson )
    {
        if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Positive_Diagnosis_Config", &positive_diagnosis_config, SD_Positive_Diagnosis_Config_DESC_TEXT );
        }
    }

    void SimpleDiagnostic::ConfigureEventsConfigs( const Configuration * inputJson )
    {
        // Some subclasses override this so that Event_Or_Config is not defined but always returns Event.
        use_event_or_config = getEventOrConfig( inputJson );

        ConfigurePositiveEvent( inputJson );
        ConfigurePositiveConfig( inputJson );
    }

    void SimpleDiagnostic::CheckEventsConfigs( const Configuration* inputJson )
    {
        positive_diagnosis_intervention = CheckEventConfig( inputJson, true,
                                                            "Positive_Diagnosis_Event", positive_diagnosis_event,
                                                            "Positive_Diagnosis_Config", positive_diagnosis_config );
    }

    void SimpleDiagnostic::ConfigureSensitivitySpecificity( const Configuration* inputJson )
    {
        initConfigTypeMap( "Base_Specificity", &base_specificity, SD_Base_Specificity_DESC_TEXT, 1.0f );
        initConfigTypeMap( "Base_Sensitivity", &base_sensitivity, SD_Base_Sensitivity_DESC_TEXT, 1.0f );
        initConfigTypeMap( "Enable_Is_Symptomatic", &enable_isSymptomatic, SD_Enable_Is_Symptomatic_DESC_TEXT, 0 );
    }

    void SimpleDiagnostic::ConfigureOther( const Configuration* inputJson )
    {
        initConfigTypeMap( "Treatment_Fraction", &treatment_fraction, SD_Treatment_Fraction_DESC_TEXT, 1.0f );
        initConfigTypeMap( "Days_To_Diagnosis", &days_to_diagnosis, GetDaysToDiagnosisDescription(), FLT_MAX, 0 );
    }

    bool SimpleDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        ConfigureSensitivitySpecificity( inputJson );
        ConfigureOther( inputJson );

        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0 );

        ConfigureEventsConfigs( inputJson );

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret )
        {
            CheckEventsConfigs( inputJson );
        }
        return ret;
    }

    const char* SimpleDiagnostic::GetDaysToDiagnosisDescription() const
    {
        return SD_Days_To_Diagnosis_DESC_TEXT;
    }

    IDistributableIntervention* SimpleDiagnostic::CheckEventConfig( const Configuration * inputJson,
                                                                    bool isRequired,
                                                                    const char* eventParameterName,
                                                                    const EventTrigger& event,
                                                                    const char* configParameterName,
                                                                    const IndividualInterventionConfig& config )
    {
        IDistributableIntervention* p_intervention = nullptr;
        if( !JsonConfigurable::_dryrun )
        {
            if( use_event_or_config == EventOrConfig::Config )
            {
                if( isRequired && (config._json.Type() == ElementType::NULL_ELEMENT) )
                {
                    std::stringstream ss;
                    ss << "'Event_Or_Config' is set to 'Config' and '" << configParameterName << "' is not defined.\n";
                    ss << "You must define it.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,ss.str().c_str() );
                }
                if( config._json.Type() != ElementType::NULL_ELEMENT )
                {
                    p_intervention = InterventionFactory::CreateIntervention( config._json,
                                                                              inputJson->GetDataLocation(),
                                                                              configParameterName,
                                                                              isRequired ); // if required throw
                }
            }

            if( isRequired && (use_event_or_config == EventOrConfig::Event) && event.IsUninitialized() )
            {
                std::stringstream ss;
                ss << "'Event_Or_Config' is set to 'Event' and '" << eventParameterName << "' is not defined.\n";
                ss << "You must define it.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return p_intervention;
    }

    SimpleDiagnostic::SimpleDiagnostic()
    : BaseIntervention()
    , base_specificity(0)
    , base_sensitivity(0)
    , treatment_fraction(1.0f)
    , days_to_diagnosis(0)
    , enable_isSymptomatic(false)
    , use_event_or_config( EventOrConfig::Event )
    , positive_diagnosis_event()
    , positive_diagnosis_config()
    , positive_diagnosis_intervention( nullptr )
    {
        days_to_diagnosis.handle = std::bind( &SimpleDiagnostic::Callback, this, std::placeholders::_1 );
    }

    SimpleDiagnostic::SimpleDiagnostic( const SimpleDiagnostic& master )
        : BaseIntervention( master )
        , base_specificity(master.base_specificity)
        , base_sensitivity(master.base_sensitivity)
        , treatment_fraction(master.treatment_fraction)
        , days_to_diagnosis(master.days_to_diagnosis)
        , enable_isSymptomatic(master.enable_isSymptomatic )
        , use_event_or_config(master.use_event_or_config)
        , positive_diagnosis_event(master.positive_diagnosis_event)
        , positive_diagnosis_config(master.positive_diagnosis_config)
        , positive_diagnosis_intervention( nullptr )
    {
        days_to_diagnosis.handle = std::bind( &SimpleDiagnostic::Callback, this, std::placeholders::_1 );
        if( master.positive_diagnosis_intervention != nullptr )
        {
            positive_diagnosis_intervention = master.positive_diagnosis_intervention->Clone();
        }
    }

    SimpleDiagnostic::~SimpleDiagnostic()
    {
        delete positive_diagnosis_intervention;
    }

    bool SimpleDiagnostic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        // ----------------------------------------------------------------------------------
        // --- Putting this here because we don't want anything to happen if we are aborting
        // ----------------------------------------------------------------------------------
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting tested and positive_diagnosis_event = %s.\n", parent->GetSuid().data, positive_diagnosis_event.c_str() );

        // Positive test result and distribute immediately if days_to_diagnosis = 0
        if ( positiveTestResult() )
        {
            LOG_DEBUG_F( "Individual %d tested positive: treatment fraction = %f.\n", parent->GetSuid().data, (float) treatment_fraction );

            if( parent->GetRng()->SmartDraw(treatment_fraction) )
            {
                // distribute immediately if days_to_diagnosis = 0
                if ( days_to_diagnosis == 0 )
                {
                    LOG_DEBUG_F( "Individual %d getting a diagnostic right now during Distribute, instead of waiting till Update.\n", parent->GetSuid().data );
                    Callback( 0 ); // positiveTestDistribute(); - since there is no waiting time, distribute intervention right now
                }
            }
            else
            {
                onPatientDefault();  // this person doesn't get the intervention despite the positive test
            }
            //else (regular case) we have to wait for days_to_diagnosis to count down, person does get intervention/event
        }
        else // Negative test result
        {
            LOG_DEBUG_F( "Individual %d tested negative.\n", parent->GetSuid().data );
            onNegativeTestResult();
        }

        return BaseIntervention::Distribute( context, pICCO );
    }

    void SimpleDiagnostic::Update( float dt )
    {
        if ( expired )
        {
            return; // don't give expired intervention.  should be cleaned up elsewhere anyways, though.
        }

        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        // Count down the time until a positive test result comes back
        days_to_diagnosis.Decrement( dt );
    }

    bool SimpleDiagnostic::applySensitivityAndSpecificity(bool infected) const
    {
        // True positive (sensitivity), or False positive (1-specificity)
        bool positiveTestReported = ( ( infected  && parent->GetRng()->SmartDraw( base_sensitivity   ) ) ||
                                      ( !infected && parent->GetRng()->SmartDraw( 1-base_specificity ) )
                                    ) ;
        LOG_DEBUG_F( "%s is returning %d\n", __FUNCTION__, positiveTestReported );
        return positiveTestReported;
    }

    bool SimpleDiagnostic::positiveTestResult()
    {
        // Apply diagnostic test with given specificity/sensitivity
        bool test_result;
        if( enable_isSymptomatic )
        {
            test_result = parent->GetEventContext()->IsInfected() && parent->GetEventContext()->IsSymptomatic();
        }
        else
        {
            test_result = parent->GetEventContext()->IsInfected();
        }
        return applySensitivityAndSpecificity( test_result );
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! NOTE: The naming of onPatientDefault() and onNegativeTestResult() implies that
    // !!! "on" the <event> occurring this action will occur.  "on" the event of patient 
    // !!! default the onPatientDefault() action is performed.  If things were consistent,
    // !!! positiveTestDistribute() would be called onPositveTestResult().
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void
    SimpleDiagnostic::onPatientDefault()
    {
        expired = true;
    }

    void
    SimpleDiagnostic::onNegativeTestResult()
    {
        expired = true;
    }

    void
    SimpleDiagnostic::broadcastEvent( const EventTrigger& event )
    {
        if( !event.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            LOG_DEBUG_F( "SimpleDiagnostic broadcasting event = %s.\n", event.c_str() );
            broadcaster->TriggerObservers( parent->GetEventContext(), event );
        }
    }

    void SimpleDiagnostic::Callback( float dt )
    {
        positiveTestDistribute();
    }

    void SimpleDiagnostic::positiveTestDistribute()
    {
        DistributeResult( "positive", positive_diagnosis_event, positive_diagnosis_intervention );
        expired = true;
    }

    void SimpleDiagnostic::DistributeResult( const char* resultTypeMsg,
                                             const EventTrigger& event,
                                             IDistributableIntervention* pIntervention )
    {
        release_assert( parent );
        LOG_DEBUG_F( "Individual %d tested '%s' in %^s\n", parent->GetSuid().data, resultTypeMsg, GetName().c_str() );

        if( use_event_or_config == EventOrConfig::Event )
        {
            broadcastEvent( event );
        }
        else if( pIntervention != nullptr )
        {
            ICampaignCostObserver* pICCO;
            // Now make sure cost of the intervention is reported back to node
            if( parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext" );
            }

            // Distribute the test-positive intervention
            IDistributableIntervention *di = pIntervention->Clone();
            di->AddRef();
            di->Distribute( parent->GetInterventionsContext(), pICCO );
            pICCO->notifyCampaignEventOccurred( (IBaseIntervention*)di, (IBaseIntervention*)this, parent );
            di->Release();
        }
    }

    REGISTER_SERIALIZABLE(SimpleDiagnostic);

    void SimpleDiagnostic::serialize(IArchive& ar, SimpleDiagnostic* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleDiagnostic& diagnostic = *obj;
        ar.labelElement("base_specificity"); diagnostic.base_specificity.serialize(ar);
        ar.labelElement("base_sensitivity"); diagnostic.base_sensitivity.serialize(ar);
        ar.labelElement("treatment_fraction"); diagnostic.treatment_fraction.serialize(ar);
        ar.labelElement("days_to_diagnosis") & diagnostic.days_to_diagnosis;
        ar.labelElement("enable_isSymptomatic") & diagnostic.enable_isSymptomatic;
        ar.labelElement("use_event_or_config") & (uint32_t&)diagnostic.use_event_or_config;
        ar.labelElement("positive_diagnosis_event") & diagnostic.positive_diagnosis_event;
        ar.labelElement("positive_diagnosis_config") & diagnostic.positive_diagnosis_config;
        ar.labelElement("positive_diagnosis_intervention") & diagnostic.positive_diagnosis_intervention;
    }
}
