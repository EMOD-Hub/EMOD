
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
#include "EventTrigger.h"

namespace Kernel
{
    struct ISTICircumcisionConsumer;

    struct ICircumcision : ISupports
    {
        virtual bool ApplyIfHigherReducedAcquire() const = 0;
        virtual float GetReducedAcquire() const = 0;
    };

    class MaleCircumcision : public BaseIntervention, public ICircumcision
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MaleCircumcision, IDistributableIntervention)

    public:
        MaleCircumcision();
        virtual ~MaleCircumcision() { }

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        // ICircumcision
        virtual bool ApplyIfHigherReducedAcquire() const override;
        virtual float GetReducedAcquire() const override;

    protected:
        float m_ReducedAcquire;
        bool m_ApplyIfHigherReducedAcquire;
        EventTrigger m_DistrbutedEventTrigger;
        ISTICircumcisionConsumer* m_pCircumcisionConsumer;
        bool has_been_applied;

        DECLARE_SERIALIZABLE(MaleCircumcision);
    };
}
