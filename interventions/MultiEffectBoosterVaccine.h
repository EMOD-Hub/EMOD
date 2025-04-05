
#pragma once

#include "Vaccine.h"
#include "MultiEffectVaccine.h"

namespace Kernel
{ 
    class MultiEffectBoosterVaccine : public MultiEffectVaccine
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MultiEffectBoosterVaccine, IDistributableIntervention)

    public:
        MultiEffectBoosterVaccine();
        MultiEffectBoosterVaccine( const MultiEffectBoosterVaccine& );
        virtual ~MultiEffectBoosterVaccine();

        virtual bool Configure( const Configuration* pConfig ) override;

        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;


    protected:
        virtual void ApplyPrimingAndBoostingEffects(IIndividualHumanInterventionsContext *context);

        float prime_acquire;
        float prime_transmit;
        float prime_mortality;
        float boost_acquire;
        float boost_transmit;
        float boost_mortality;
        float boost_threshold_acquire;
        float boost_threshold_transmit;
        float boost_threshold_mortality;

        DECLARE_SERIALIZABLE(MultiEffectBoosterVaccine);
    };
}
