
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    struct IVectorInterventionEffectsSetter;

    class HumanHostSeekingTrap : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HumanHostSeekingTrap, IDistributableIntervention)

    public:
        HumanHostSeekingTrap();
        HumanHostSeekingTrap( const HumanHostSeekingTrap& );
        virtual ~HumanHostSeekingTrap();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        IWaningEffect* killing_effect;
        IWaningEffect* attract_effect;
        IVectorInterventionEffectsSetter *ivies;

        DECLARE_SERIALIZABLE(HumanHostSeekingTrap);
    };
}
