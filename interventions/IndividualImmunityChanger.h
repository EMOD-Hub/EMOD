
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Susceptibility.h"
#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"

namespace Kernel
{
    struct IIndividualImmunityChangerEffects;
    /* Keep around as an identity solution??? */
    struct IIndividualImmunityChanger : public ISupports
    {
    };

    class IndividualImmunityChanger : public BaseIntervention, public IIndividualImmunityChanger
    {
        public:
        bool Configure( const Configuration * config );

        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, IndividualImmunityChanger, IDistributableIntervention)

    public:
        IndividualImmunityChanger();

        // factory method
        virtual ~IndividualImmunityChanger();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

        virtual int AddRef();
        virtual int Release();

    protected:
        IIndividualHumanContext *parent;
        ISusceptibilityContext *isc;
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
        float update_acquire;
        float update_transmit;
        float update_mortality;

        DECLARE_SERIALIZABLE(IndividualImmunityChanger);
    };
}
