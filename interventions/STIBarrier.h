
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
#include "IRelationship.h"
#include "EventTrigger.h"
#include "Timers.h"

namespace Kernel
{
    struct ISTIBarrierConsumer; 
    struct IDistribution;

    class STIBarrier : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, STIBarrier, IDistributableIntervention)

    public:
        STIBarrier();
        STIBarrier( const STIBarrier& rMaster );
        virtual ~STIBarrier();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

    protected:

        void TimerCallback( float dt );

        float early;
        float late;
        float midyear;
        float rate;
        RelationshipType::Enum rel_type;

        IDistribution* m_pUsageDurationDistribution;
        CountdownTimer m_UsageTimer;
        EventTrigger   m_UsageExpirationEvent;

        ISTIBarrierConsumer *ibc;

        DECLARE_SERIALIZABLE(STIBarrier);
    };
}
