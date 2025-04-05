
#pragma once

#include <string>

#include "Configuration.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "EventTriggerCoordinator.h"
#include "EventCoordinator.h"

namespace Kernel
{
    class EventCoordinatorEventContextAdapter : public IEventCoordinatorEventContext
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        EventCoordinatorEventContextAdapter( BaseNodeIntervention* pNodeIntervention,
                                             INodeEventContext* pNEC );
        virtual ~EventCoordinatorEventContextAdapter();

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IEventCoordinatorEventContext
        virtual const std::string& GetName() const override;
        virtual const IdmDateTime& GetTime() const override;
        virtual IEventCoordinator* GetEventCoordinator() override;

    private:
        BaseNodeIntervention* m_pIntervention;
        INodeEventContext*    m_pNEC;
        std::string           m_Name;
    };


    class IDMAPI BroadcastCoordinatorEventFromNode : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, BroadcastCoordinatorEventFromNode, INodeDistributableIntervention)

    public:
        BroadcastCoordinatorEventFromNode();
        BroadcastCoordinatorEventFromNode( const BroadcastCoordinatorEventFromNode& master );
        virtual ~BroadcastCoordinatorEventFromNode();

        bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

    protected:
        EventTriggerCoordinator m_EventToBroadcast;
    };
}
