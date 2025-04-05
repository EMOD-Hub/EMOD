
#pragma once

#include "Vaccine.h"

namespace Kernel
{

     class SimpleBoosterVaccine : public SimpleVaccine
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleBoosterVaccine, IDistributableIntervention)

    public:
        SimpleBoosterVaccine();
        SimpleBoosterVaccine( const SimpleBoosterVaccine& );
        virtual ~SimpleBoosterVaccine();
 
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;

    protected:
        virtual void ApplyPrimingAndBoostingEffects(IIndividualHumanInterventionsContext *context);

        float prime_effect;
        float boost_effect;
        float boost_threshold;

        DECLARE_SERIALIZABLE(SimpleBoosterVaccine);
    };
}