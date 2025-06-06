
#pragma once

#include "Diagnostics.h"


namespace Kernel
{
    class IDMAPI HIVSimpleDiagnostic : public SimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVSimpleDiagnostic, IDistributableIntervention)

    public:
        HIVSimpleDiagnostic();
        HIVSimpleDiagnostic( const HIVSimpleDiagnostic& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        // SimpleDiagnostic
        virtual void onNegativeTestResult() override;
        virtual bool positiveTestResult() override;

    protected:
        virtual void ConfigureOther( const Configuration* inputJson ) override;
        virtual void ConfigurePositiveConfig( const Configuration * inputJson ) override;
        virtual void ConfigureEventsConfigs( const Configuration * inputJson ) override;
        virtual void CheckEventsConfigs( const Configuration * inputJson ) override;
        virtual EventOrConfig::Enum getEventOrConfig( const Configuration* ) override;

        virtual void Callback( float dt );
        virtual bool UpdateIndividualsInterventionStatus(bool checkDisqualifyingProperties=true) override;
        virtual void ActOnResultsIfTime();

        bool firstUpdate;
        bool result_of_positive_test;
        float original_days_to_diagnosis;
        float absoluteDuration;
        EventTrigger negative_diagnosis_event;

        DECLARE_SERIALIZABLE(HIVSimpleDiagnostic);
    };
}
