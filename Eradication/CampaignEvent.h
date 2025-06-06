
#pragma once

#include <string>
#include <list>
#include <vector>

#include "ISupports.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "NodeSet.h"
#include "ObjectFactory.h"

namespace Kernel
{
    struct INodeEventContext;
    struct ISimulationEventContext;
    class CampaignEvent;

    class CampaignEventFactory : public ObjectFactory<CampaignEvent,CampaignEventFactory>
    {
    public:
        virtual CampaignEvent* CreateInstance( const json::Element& rJsonElement,
                                               const std::string& rDataLocation,
                                               const char* parameterName ) override;

    protected:
        template<class IObject, class Factory> friend class Kernel::ObjectFactory;

        CampaignEventFactory();
    };


    struct IEventCoordinator;

    class CampaignEvent : public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(CampaignEventFactory, CampaignEvent, IConfigurable)

    public:
        friend class CampaignEventFactory;
        DECLARE_CONFIGURED(CampaignEvent)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        CampaignEvent();
        virtual ~CampaignEvent();
        float GetStartDay() const;
        int   GetEventIndex() const;
        void  SetEventIndex(int index);
        void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographic_node_ids);
        float GetDistributionDuration() const;

        void Dispatch(ISimulationEventContext *isec); // called when the event is to happen

    protected:
        float start_day;
        int event_index; // to avoid the priority queue from arbitrarily swapping the order of events on the same start day
        INodeSet *nodeset;
        IEventCoordinator *event_coordinator; // TODO: eventually try to instantiate and then serialize just this, once a solution for global type registration is worked out
        NodeSetConfig nodeset_config;
        EventConfig event_coordinator_config;
    };
}
