
#include "stdafx.h"
#include "HIVSimpleDiagnostic.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IIndividualHumanHIV.h"  // for IndividualHIV access
#include "IIndividualHumanContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "HIVSimpleDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVSimpleDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVSimpleDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVSimpleDiagnostic)

    HIVSimpleDiagnostic::HIVSimpleDiagnostic()
    : SimpleDiagnostic() // true implies only support events in SimpleDiagnostic
    , firstUpdate(true)
    , result_of_positive_test(false)
    , original_days_to_diagnosis(0.0)
    , negative_diagnosis_event()
    {
        initSimTypes(1, "HIV_SIM");

        days_to_diagnosis.handle = std::bind( &HIVSimpleDiagnostic::Callback, this, std::placeholders::_1 );
    }

    HIVSimpleDiagnostic::HIVSimpleDiagnostic( const HIVSimpleDiagnostic& master )
    : SimpleDiagnostic( master )
    {
        firstUpdate = master.firstUpdate;
        result_of_positive_test = master.result_of_positive_test;
        original_days_to_diagnosis = master.original_days_to_diagnosis;
        negative_diagnosis_event = master.negative_diagnosis_event;

        days_to_diagnosis.handle = std::bind( &HIVSimpleDiagnostic::Callback, this, std::placeholders::_1 );
    }

    EventOrConfig::Enum
    HIVSimpleDiagnostic::getEventOrConfig(
        const Configuration * inputJson
    )
    {
        // For those premature optimizers out there, this function is expected to get more interesting in the future.
        return EventOrConfig::Event;
    }

    void HIVSimpleDiagnostic::ConfigureOther( const Configuration * inputJson )
    {
        // don't keep Treatment_Fraction
        initConfigTypeMap( "Days_To_Diagnosis", &days_to_diagnosis, SD_Days_To_Diagnosis_DESC_TEXT, FLT_MAX, 0 );
    }

    void HIVSimpleDiagnostic::ConfigurePositiveConfig( const Configuration * inputJson )
    {
        // do nothing because we don't want the config part
    }

    void HIVSimpleDiagnostic::ConfigureEventsConfigs( const Configuration * inputJson )
    {
        SimpleDiagnostic::ConfigureEventsConfigs( inputJson );

        if( JsonConfigurable::_dryrun ||
            ((use_event_or_config == EventOrConfig::Event) && inputJson->Exist( "Negative_Diagnosis_Event" )) )
        {
            initConfigTypeMap( "Negative_Diagnosis_Event", &negative_diagnosis_event, HIV_SD_Negative_Diagnosis_Event_DESC_TEXT );
        }
    }

    void HIVSimpleDiagnostic::CheckEventsConfigs( const Configuration * inputJson )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We need to fix this so that we can ensure that Postive_Diagnosis_Event is defined.
        // !!! I'm stopping here so I reduce the number of changes in this phase.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //SimpleDiagnostic::CheckEventsConfigs( inputJson );
    }

    bool HIVSimpleDiagnostic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting tested.\n", parent->GetSuid().data );
        return BaseIntervention::Distribute( context, pICCO );
    }

    void HIVSimpleDiagnostic::Callback( float dt )
    {
        ActOnResultsIfTime();
    }

    void HIVSimpleDiagnostic::ActOnResultsIfTime()
    {
        // This can happen immediately if days_to_diagnosis is initialized to zero.
        if( result_of_positive_test )
        {
            LOG_DEBUG_F( "Individual %d tested positive.\n", parent->GetSuid().data );
            if( parent->GetRng()->SmartDraw( treatment_fraction ) )
            {
                positiveTestDistribute();
            }
            else
            {
                // this person doesn't get the positive test result
                // because they defaulted / don't want treatment
                onPatientDefault();
                expired = true;
            }
        }
        else
        {
            LOG_DEBUG_F( "Individual %d tested negative.\n", parent->GetSuid().data );
            onNegativeTestResult();
        }
    }

    void
    HIVSimpleDiagnostic::onNegativeTestResult()
    {
        auto iid = parent->GetSuid().data;
        LOG_DEBUG_F( "Individual %d tested 'negative' in HIVSimpleDiagnostic, receiving actual intervention.\n", iid );

        if( !negative_diagnosis_event.IsUninitialized() )
        {
            LOG_DEBUG_F( "Brodcasting event %s as negative diagnosis event for individual %d.", negative_diagnosis_event.c_str(), iid );
            broadcastEvent( negative_diagnosis_event );
        }
        else
        {
            LOG_DEBUG_F( "Negative diagnosis event is empty for individual %d.\n", iid );
        }
        expired = true;
    }


    // todo: lift to HIVIntervention or helper function (repeated in HIVDelayedIntervention)
    bool HIVSimpleDiagnostic::UpdateIndividualsInterventionStatus( bool checkDisqualifyingProperties )
    {
        if( firstUpdate )
        {
            original_days_to_diagnosis = days_to_diagnosis;
        }
        return SimpleDiagnostic::UpdateIndividualsInterventionStatus( checkDisqualifyingProperties );
    }

    void HIVSimpleDiagnostic::Update( float dt )
    {
        if( !UpdateIndividualsInterventionStatus() ) return;

        // Why is this different from base class behaviour? In which Distribute can call postiiveTestResult. 
        if( firstUpdate )
        {
            result_of_positive_test = positiveTestResult() ;
        }

        // ------------------------------------------------------------------------------
        // --- Count down the time until a positive test result comes back
        // ---    Update() is called the same day as Distribute() so we don't want
        // ---    to decrement the counter until the next day. 
        // Update: NOTE TRUE ANYMORE. CountdownTimer doesn't call callback same day as going <= 0
        // ------------------------------------------------------------------------------ 
        days_to_diagnosis.Decrement( dt );

        firstUpdate = false;
    }

    bool HIVSimpleDiagnostic::positiveTestResult()
    {
        IIndividualHumanHIV* HIVpersonptr = nullptr;

        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&HIVpersonptr) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext");
        }

        // Apply diagnostic test with given specificity/sensitivity
        bool  infected = HIVpersonptr->HasHIV();

        // True positive (sensitivity), or False positive (1-specificity)

        return applySensitivityAndSpecificity(infected);
    }

    REGISTER_SERIALIZABLE(HIVSimpleDiagnostic);

    void HIVSimpleDiagnostic::serialize(IArchive& ar, HIVSimpleDiagnostic* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        HIVSimpleDiagnostic& hsd = *obj;
        ar.labelElement("firstUpdate"               ) & hsd.firstUpdate;
        ar.labelElement("result_of_positive_test"   ) & hsd.result_of_positive_test;
        ar.labelElement("original_days_to_diagnosis") & hsd.original_days_to_diagnosis;
        ar.labelElement("absoluteDuration"          ) & hsd.absoluteDuration;
        ar.labelElement("negative_diagnosis_event"  ) & hsd.negative_diagnosis_event;
    }
}
