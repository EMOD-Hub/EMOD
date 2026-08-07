
#pragma once

#include "TriggeredEventCoordinator.h"
#include "JsonConfigurableCollection.h"
#include "ExternalNodeId.h"
#include "EventTriggerCoordinator.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- NodeIdToEvent
    // ------------------------------------------------------------------------

    class NodeIdToEvent : public JsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        NodeIdToEvent();
        virtual ~NodeIdToEvent();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration * inputJson ) override;
       
        // Other methods
        ExternalNodeId_t GetExternalNodeId() const;
        const EventTriggerCoordinator& GetCoordinatorEvent() const;
        bool WasEventSent() const;
        void ClearEventSent();
        void EventWasSent();

    private:
        ExternalNodeId_t m_NodeId;
        EventTriggerCoordinator m_CoordinatorEvent;
        bool m_WasEventSent;
    };

    // ------------------------------------------------------------------------
    // --- NodeIdToEventList
    // ------------------------------------------------------------------------

    class NodeIdToEventList : public JsonConfigurableCollection<NodeIdToEvent>
    {
    public:
        NodeIdToEventList();
        virtual ~NodeIdToEventList();

        virtual void CheckConfiguration() override;

    protected:
        virtual NodeIdToEvent* CreateObject() override;
    };

    // ------------------------------------------------------------------------
    // --- FirstNodeWithNodePropertyEventCoordinator
    // ------------------------------------------------------------------------

    class FirstNodeWithNodePropertyEventCoordinator : public TriggeredEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED( EventCoordinatorFactory, FirstNodeWithNodePropertyEventCoordinator, IEventCoordinator )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        FirstNodeWithNodePropertyEventCoordinator();
        virtual ~FirstNodeWithNodePropertyEventCoordinator();

        virtual bool Configure(const Configuration* config) override;
        virtual void UpdateNodes( float dt ) override;
        virtual void Update( float dt ) override;
        virtual bool notifyOnEvent( IEventCoordinatorEventContext * pEntity, const EventTriggerCoordinator & trigger ) override;

    private:
        void SelectNodeAndBroadcastEvent();

        NPKeyValue              m_NodePropertyKeyValueToHave;
        NodeIdToEventList       m_NodeIdToEventList;
        EventTriggerCoordinator m_CoordinatorEventNotFound;
    };
}
