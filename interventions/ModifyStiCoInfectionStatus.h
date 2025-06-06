
#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class ModifyStiCoInfectionStatus : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(ModifyStiCoInfectionStatus)
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, ModifyStiCoInfectionStatus, IDistributableIntervention)
        DECLARE_QUERY_INTERFACE()

    public:
        ModifyStiCoInfectionStatus();
        virtual ~ModifyStiCoInfectionStatus() { }

        // INodeDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update(float dt) override;

    protected:
        bool set_flag_to;

        DECLARE_SERIALIZABLE(ModifyStiCoInfectionStatus);
    };
}
