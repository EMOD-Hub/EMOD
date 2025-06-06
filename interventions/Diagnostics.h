
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Configuration.h"
#include "Configure.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "Timers.h"
#include "Types.h"
#include "EventTrigger.h"

namespace Kernel
{
    class SimpleDiagnostic :  public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleDiagnostic, IDistributableIntervention)

    public:
        SimpleDiagnostic();
        SimpleDiagnostic( const SimpleDiagnostic& master );
        virtual ~SimpleDiagnostic();

        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;
        virtual void Update(float dt) override;

        virtual bool positiveTestResult();
        virtual void onNegativeTestResult();
        virtual void onPatientDefault();
        virtual void positiveTestDistribute();
        virtual bool applySensitivityAndSpecificity( bool infected ) const;
        virtual void Callback( float dt );

    protected:

        void broadcastEvent( const EventTrigger& event );
        virtual void ConfigureSensitivitySpecificity( const Configuration* inputJson );
        virtual void ConfigureEventsConfigs( const Configuration * inputJson );
        virtual void ConfigureOther( const Configuration* inputJson );
        virtual void CheckEventsConfigs( const Configuration * inputJson );

        virtual EventOrConfig::Enum getEventOrConfig( const Configuration* );
        virtual void ConfigurePositiveEvent( const Configuration * inputJson );
        virtual void ConfigurePositiveConfig( const Configuration * inputJson );
        virtual IDistributableIntervention* CheckEventConfig( const Configuration * inputJson,
                                                              bool isRequired,
                                                              const char* eventParameterName,
                                                              const EventTrigger& event,
                                                              const char* configParameterName,
                                                              const IndividualInterventionConfig& config );
        virtual void DistributeResult( const char* resultTypeMsg,
                                       const EventTrigger& event,
                                       IDistributableIntervention* pIntervention );
        virtual const char* GetDaysToDiagnosisDescription() const;

        ProbabilityNumber base_specificity;
        ProbabilityNumber base_sensitivity;
        ProbabilityNumber treatment_fraction;
        CountdownTimer days_to_diagnosis; // can go negative if dt is > 1
        bool enable_isSymptomatic;

        EventOrConfig::Enum use_event_or_config;
        EventTrigger positive_diagnosis_event;
        IndividualInterventionConfig positive_diagnosis_config;
        IDistributableIntervention* positive_diagnosis_intervention;

        DECLARE_SERIALIZABLE(SimpleDiagnostic);
    };
}
