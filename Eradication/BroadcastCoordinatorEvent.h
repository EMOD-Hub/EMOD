
#pragma once

#include "EventCoordinator.h"
#include "Configure.h"
#include "EventTriggerCoordinator.h"

namespace Kernel
{
    class BroadcastCoordinatorEvent : public JsonConfigurable, public IEventCoordinator, public IEventCoordinatorEventContext
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( EventCoordinatorFactory, BroadcastCoordinatorEvent, IEventCoordinator )
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        BroadcastCoordinatorEvent();
        virtual ~BroadcastCoordinatorEvent();

        virtual bool Configure( const Configuration * inputJson ) override;
        virtual QuickBuilder GetSchema() override;

        virtual IConfigurable*  GetConfigurable()  override { return JsonConfigurable::GetConfigurable(); }

        // IEventCoordinator methods
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void CheckStartDay( float campaignStartDay ) const override { };
        virtual void AddNode( const suids::suid& suid ) override;
        virtual void Update( float dt ) override;
        virtual void UpdateNodes( float dt ) override;
        virtual bool IsFinished() override;
        virtual IEventCoordinatorEventContext* GetEventContext() override { return this; }

        // IEventCoordinatorEventContext
        virtual const std::string& GetName() const override;
        virtual const IdmDateTime& GetTime() const override;
        virtual IEventCoordinator* GetEventCoordinator() override { return this; };

    protected:
        ISimulationEventContext* m_Parent;
        std::string              m_CoordinatorName;
        EventTriggerCoordinator  m_EventToBroadcast;
        float                    m_CampaignCost;
        INodeEventContext*       m_pNodeForCampaignCost;
    };
}

