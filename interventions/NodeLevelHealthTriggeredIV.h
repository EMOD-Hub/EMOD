
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"
#include "Configure.h"
#include "DemographicRestrictions.h"
#include "EventTrigger.h"

namespace Kernel
{
    class NodeLevelHealthTriggeredIV : public IIndividualEventObserver, public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, NodeLevelHealthTriggeredIV, INodeDistributableIntervention)

    public:
        NodeLevelHealthTriggeredIV();
        NodeLevelHealthTriggeredIV( const NodeLevelHealthTriggeredIV& rMaster );
        virtual ~NodeLevelHealthTriggeredIV();
        virtual int AddRef() override;
        virtual int Release() override;
        virtual bool Configure( const Configuration* config ) override;

        // INodeDistributableIntervention
        virtual bool Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

        // IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

    protected:
        std::vector<EventTrigger>   m_trigger_conditions;
        float max_duration;
        float duration;
        PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer> node_property_restrictions;
        DemographicRestrictions demographic_restrictions;
        bool m_disqualified_by_coverage_only;
        float blackout_period;
        float blackout_time_remaining;
        EventTrigger blackout_event_trigger;
        bool blackout_on_first_occurrence;
        bool notification_occurred;
        bool distribute_on_return_home;
        std::vector<std::set<uint32_t>> event_occurred_list;
        std::map<suids::suid,bool> event_occurred_while_resident_away;
        IndividualInterventionConfig actual_individual_intervention_config;
        NodeInterventionConfig actual_node_intervention_config;
        IDistributableIntervention *m_di;
        INodeDistributableIntervention *m_ndi;

        virtual bool qualifiesToGetIntervention( const IIndividualHumanEventContext * pIndividual );
        virtual float getDemographicCoverage() const;
        virtual void onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv );
        std::string GetInterventionClassName() const;
        void Unregister();
    };
}
