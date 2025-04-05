
#pragma once

#include "Diagnostics.h"

namespace Kernel
{
    class IDMAPI StandardDiagnostic :  public SimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, StandardDiagnostic, IDistributableIntervention )

    public:
        StandardDiagnostic();
        StandardDiagnostic( bool requireNegativeResponse );
        StandardDiagnostic( const StandardDiagnostic& master );
        virtual ~StandardDiagnostic();

        virtual void onNegativeTestResult() override;

    protected:
        virtual const char* GetDaysToDiagnosisDescription() const override;
        virtual void ConfigureEventsConfigs( const Configuration * inputJson ) override;
        virtual void CheckEventsConfigs( const Configuration * inputJson ) override;

        virtual void ConfigureNegativeEvent( const Configuration * inputJson );
        virtual void ConfigureNegativeConfig( const Configuration * inputJson );
        virtual void negativeTestDistribute();

        bool is_negative_response_required;
        EventTrigger negative_diagnosis_event;
        IndividualInterventionConfig negative_diagnosis_config;
        IDistributableIntervention* negative_diagnosis_intervention;

        DECLARE_SERIALIZABLE(StandardDiagnostic);
    };
}
