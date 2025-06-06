
#include "stdafx.h"
#include "StandardDiagnostic.h"

#include "InterventionEnums.h"

SETUP_LOGGING( "StandardDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(StandardDiagnostic,SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(StandardDiagnostic,SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(StandardDiagnostic)

    StandardDiagnostic::StandardDiagnostic()
        : SimpleDiagnostic()
        , is_negative_response_required( false )
        , negative_diagnosis_event()
        , negative_diagnosis_config()
        , negative_diagnosis_intervention( nullptr )
    {
    }

    StandardDiagnostic::StandardDiagnostic( bool requireNegativeResponse )
        : SimpleDiagnostic()
        , is_negative_response_required( requireNegativeResponse )
        , negative_diagnosis_event()
        , negative_diagnosis_config()
        , negative_diagnosis_intervention( nullptr )
    {
    }

    StandardDiagnostic::StandardDiagnostic( const StandardDiagnostic& master )
        : SimpleDiagnostic( master )
        , is_negative_response_required( master.is_negative_response_required )
        , negative_diagnosis_event( master.negative_diagnosis_event )
        , negative_diagnosis_config( master.negative_diagnosis_config )
        , negative_diagnosis_intervention( nullptr )
    {
        if( master.negative_diagnosis_intervention != nullptr )
        {
            negative_diagnosis_intervention = master.negative_diagnosis_intervention->Clone();
        }
    }

    StandardDiagnostic::~StandardDiagnostic()
    {
        delete negative_diagnosis_intervention;
    }

    const char* StandardDiagnostic::GetDaysToDiagnosisDescription() const
    {
        return StdDiag_Days_To_Diagnosis_DESC_TEXT;
    }

    void StandardDiagnostic::ConfigureEventsConfigs( const Configuration * inputJson )
    {
        SimpleDiagnostic::ConfigureEventsConfigs( inputJson );
        ConfigureNegativeEvent( inputJson );
        ConfigureNegativeConfig( inputJson );
    }

    void StandardDiagnostic::CheckEventsConfigs( const Configuration * inputJson )
    {
        SimpleDiagnostic::CheckEventsConfigs( inputJson );
        negative_diagnosis_intervention = CheckEventConfig( inputJson, is_negative_response_required,
                                                            "Negative_Diagnosis_Event", negative_diagnosis_event,
                                                            "Negative_Diagnosis_Config", negative_diagnosis_config );
    }

    void StandardDiagnostic::ConfigureNegativeEvent( const Configuration * inputJson )
    {
        if( JsonConfigurable::_dryrun ||
            ((use_event_or_config == EventOrConfig::Event) && inputJson->Exist( "Negative_Diagnosis_Event" )) )
        {
            initConfigTypeMap( "Negative_Diagnosis_Event", &negative_diagnosis_event, StdDiag_Negative_Diagnosis_Event_DESC_TEXT );
        }
    }
    
    void StandardDiagnostic::ConfigureNegativeConfig( const Configuration * inputJson )
    {
        if( JsonConfigurable::_dryrun ||
            ((use_event_or_config == EventOrConfig::Config) && inputJson->Exist( "Negative_Diagnosis_Config" )) )
        {
            initConfigComplexType( "Negative_Diagnosis_Config", &negative_diagnosis_config, StdDiag_Negative_Diagnosis_Config_DESC_TEXT );
        }
    }

    void StandardDiagnostic::onNegativeTestResult()
    {
        negativeTestDistribute();
        SimpleDiagnostic::onNegativeTestResult();
    }

    void StandardDiagnostic::negativeTestDistribute()
    {
        DistributeResult( "negative", negative_diagnosis_event, negative_diagnosis_intervention );
    }

    REGISTER_SERIALIZABLE(StandardDiagnostic);

    void StandardDiagnostic::serialize(IArchive& ar, StandardDiagnostic* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        StandardDiagnostic& diagnostic = *obj;
        ar.labelElement("negative_diagnosis_event"       ) & diagnostic.negative_diagnosis_event;
        ar.labelElement("negative_diagnosis_config"      ) & diagnostic.negative_diagnosis_config;
        ar.labelElement("negative_diagnosis_intervention") & diagnostic.negative_diagnosis_intervention;
    }
}
