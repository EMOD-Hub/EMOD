
#pragma once

#include <map>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "Configure.h"
#include "InsecticideWaningEffect.h"

namespace Kernel
{
    struct IVectorInterventionEffectsSetter;

    class Ivermectin : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, Ivermectin, IDistributableIntervention)

    public:
        Ivermectin();
        Ivermectin( const Ivermectin& );
        virtual ~Ivermectin();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        IInsecticideWaningEffect* m_pInsecticideWaningEffect;
        IVectorInterventionEffectsSetter* m_pIVIES; // aka individual vector interventions container

        DECLARE_SERIALIZABLE(Ivermectin);
    };
}
