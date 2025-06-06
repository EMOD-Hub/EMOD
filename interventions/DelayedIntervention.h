
#pragma once

#include "MathFunctions.h"
#include "Interventions.h"
#include "InterventionFactory.h"
#include "IDistribution.h"
#include "Timers.h"

namespace Kernel
{
    class DelayedIntervention: public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, DelayedIntervention, IDistributableIntervention)

    public:
        DelayedIntervention();
        DelayedIntervention( const DelayedIntervention& );
        virtual ~DelayedIntervention();

        virtual bool Configure( const Configuration* config ) override;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

        virtual void Callback( float dt );

    protected:
        virtual void PreConfigure( const Configuration* config );
        virtual void DistributionConfigure( const Configuration* config );
        virtual void InterventionConfigure( const Configuration* config );
        virtual void InterventionValidate( const std::string& rDataLocation );
        virtual void DelayValidate();

        virtual void CalculateDelay();

        CountdownTimer remaining_delay_days;
        float coverage;

        IDistribution* delay_distribution;

        std::vector<IDistributableIntervention*> m_Interventions;
        IndividualInterventionConfig actual_intervention_config;

        DECLARE_SERIALIZABLE(DelayedIntervention);
    };
}
