
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "InsecticideWaningEffect.h"

namespace Kernel
{
    struct IIndividualRepellentConsumer;

    class SimpleIndividualRepellent : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleIndividualRepellent, IDistributableIntervention)

    public:
        virtual bool Configure( const Configuration * config ) override;

        SimpleIndividualRepellent();
        SimpleIndividualRepellent( const SimpleIndividualRepellent& );
        virtual ~SimpleIndividualRepellent();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt);

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        IInsecticideWaningEffect* m_pInsecticideWaningEffect;
        IIndividualRepellentConsumer *m_pIRC; // aka individual or individual vector interventions container

        DECLARE_SERIALIZABLE(SimpleIndividualRepellent);
    };
}
