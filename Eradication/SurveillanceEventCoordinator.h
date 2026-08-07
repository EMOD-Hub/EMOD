

#pragma once

#include "IncidenceEventCoordinator.h"
#include "BroadcasterObserver.h"
#include "EventTriggerCoordinator.h"
#include "ISurveillanceReporting.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- ResponderSurveillance
    // ------------------------------------------------------------------------

    class ResponderSurveillance : public Responder
    {
    public:
        ResponderSurveillance();
        virtual ~ResponderSurveillance();

        // Responder methods
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void CheckConfiguration( const Configuration * inputJson ) override;
        virtual bool Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation );

        const std::vector<std::string>& GetPercentageEventsToCount() const { return m_PercentageEventsToCount; }

    private:
        EventTriggerCoordinator m_RespondedEvent;
        std::vector<std::string> m_PercentageEventsToCount;
    };

    // ------------------------------------------------------------------------
    // --- SurveillanceEventCoordinator
    // ------------------------------------------------------------------------

    class SurveillanceEventCoordinator : public IncidenceEventCoordinator
                                       , public ICoordinatorEventObserver
                                       , public ISurveillanceReporting
    {
        DECLARE_FACTORY_REGISTERED(EventCoordinatorFactory, SurveillanceEventCoordinator, IEventCoordinator)
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        SurveillanceEventCoordinator();
        virtual ~SurveillanceEventCoordinator();

        virtual bool Configure(const Configuration * inputJson) override;       
        virtual bool notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger ) override;
        void Register();
        void Unregister();
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void Update(float dt) override;

        virtual bool IsFinished() override;
        virtual void ConsiderResponding();
        virtual void AddNode( const suids::suid& suid ) override;
        virtual void UpdateNodes( float dt ) override;
        ISimulationEventContext* GetSimulationContext() { return m_Parent; };

        // ISurveillanceReporting
        virtual void CollectStats( ReportStatsByIP& rStats ) override;
        virtual uint32_t GetNumCounted() const override;
        virtual float GetCurrentActionThreshold() const override;

    protected:
        virtual void ConfigureRepetitions( const Configuration * inputJson ) override { };
        virtual void CheckConfigureRepetitions() override {}
        void CheckConfigurationTriggers();

        std::vector<EventTriggerCoordinator> m_StartTriggerConditionList;
        std::vector<EventTriggerCoordinator> m_StopTriggerConditionList;

    private:
        bool m_IsActive;
        bool m_IsStarting;
        bool m_IsStopping;
        float m_Duration;
        float m_CounterPeriod;
        bool m_DurationExpired;
        EventTriggerCoordinator m_RespondedEvent;
    };




}