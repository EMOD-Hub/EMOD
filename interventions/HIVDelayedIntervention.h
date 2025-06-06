
#pragma once

#include "SimulationEnums.h"
#include "DelayedIntervention.h"
#include "Configure.h"
#include "InterpolatedValueMap.h"
#include "EventTrigger.h"

namespace Kernel
{
    class HIVDelayedIntervention: public DelayedIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVDelayedIntervention, IDistributableIntervention)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        HIVDelayedIntervention();
        HIVDelayedIntervention( const HIVDelayedIntervention& );

        virtual bool Configure( const Configuration* config ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        virtual void Update(float dt) override;

        virtual void Callback( float dt );

    protected:
        virtual void CalculateDelay();

        float days_remaining;

        EventTrigger broadcast_event;
        EventTrigger broadcast_on_expiration_event;

        DECLARE_SERIALIZABLE(HIVDelayedIntervention);
    };
}
