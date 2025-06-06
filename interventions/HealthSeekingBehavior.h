
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "NodeEventContext.h"    // for INodeEventContext (ICampaignCostObserver)
#include "EventTrigger.h"

namespace Kernel
{

    class SimpleHealthSeekingBehavior : public BaseIntervention //, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleHealthSeekingBehavior, IDistributableIntervention)

    public:
        SimpleHealthSeekingBehavior();
        SimpleHealthSeekingBehavior( const SimpleHealthSeekingBehavior& rMaster );
        virtual ~SimpleHealthSeekingBehavior();

        // We inherit AddRef/Release abstractly through IHealthSeekBehavior,
        // even though BaseIntervention has a non-abstract version.
        virtual int32_t AddRef() override { return BaseIntervention::AddRef(); }
        virtual int32_t Release() override { return BaseIntervention::Release(); }

        virtual bool Configure( const Configuration* config ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

        //virtual void Expire();

    protected:

        float probability_of_seeking;
        EventOrConfig::Enum use_event_or_config;
        EventTrigger actual_intervention_event;
        IDistributableIntervention* m_di;
        bool single_use;

        DECLARE_SERIALIZABLE(SimpleHealthSeekingBehavior);
    };
}
