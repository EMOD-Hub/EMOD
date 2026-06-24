
#pragma once

#include "EventCoordinator.h"
#include "Configure.h"
#include "DemographicRestrictions.h"
#include "Interventions.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "JsonConfigurableCollection.h"
#include "EventTriggerNode.h"
#include "EventTriggerCoordinator.h"
#include "ReportStatsByIP.h"


namespace Kernel
{
    struct INodeEventContext;
    struct IIndividualHumanEventContext;

    ENUM_DEFINE( ThresholdType,
        ENUM_VALUE_SPEC( COUNT,             1 )
        ENUM_VALUE_SPEC( PERCENTAGE,        2 )
        ENUM_VALUE_SPEC( PERCENTAGE_EVENTS, 3 ) )

    // ------------------------------------------------------------------------
    // --- Action
    // ------------------------------------------------------------------------

    class Action : public JsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        Action();
        virtual ~Action();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration * inputJson ) override;
        void CheckConfigurationTriggers();
       

        // Other methods
        float GetThreshold() const;
        const std::string& GetEventToBroadcast() const;
        
        EventType::Enum GetEventType() { return m_EventType; };
        EventTriggerCoordinator& GetEventToBroadcastCooridnator() { return m_TriggerCoordinator; };
        EventTriggerNode& GetEventToBroadcastNode() { return m_TriggerNode; };
        EventTrigger& GetEventToBroadcastIndividual() { return m_TriggerIndividual; };


    private:
        float        m_Threshold;
        std::string  m_EventToBroadcast;
        EventType::Enum  m_EventType;
        EventTriggerCoordinator m_TriggerCoordinator;
        EventTriggerNode m_TriggerNode;
        EventTrigger m_TriggerIndividual;
    };

    // ------------------------------------------------------------------------
    // --- ActionList
    // ------------------------------------------------------------------------

    class ActionList : public JsonConfigurableCollection<Action>
    {
    public:
        ActionList();
        virtual ~ActionList();

        virtual void CheckConfiguration() override;

    protected:
        virtual Action* CreateObject() override;
    };

    // ------------------------------------------------------------------------
    // --- Responder
    // ------------------------------------------------------------------------

    class Responder : public JsonConfigurable, public IVisitIndividual
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        Responder();
        virtual ~Responder();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void CheckConfiguration( const Configuration * inputJson );

        // IVisitIndividual methods
        virtual bool visitIndividualCallback( IIndividualHumanEventContext *ihec, float & incrementalCostOut, ICampaignCostObserver * pICCO );

        // Other methods
        virtual bool Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation );
        void SetContextTo(ISimulationEventContext* sim, IEventCoordinatorEventContext *isec);

        const Action* GetCurrentAction() const;

    protected:
        float CalculateIncidence( uint32_t numIncidences, uint32_t qualifyingPopulation ) const;
        Action* GetAction( float value );

        ThresholdType::Enum m_ThresholdType;
        ActionList m_ActionList;
        Action* m_pCurrentAction;
        ISimulationEventContext* m_sim;
        IEventCoordinatorEventContext* m_Parent;
    };

    // ------------------------------------------------------------------------
    // --- IncidenceCounter
    // ------------------------------------------------------------------------

    class IncidenceCounter : public JsonConfigurable, public IIndividualEventObserver
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        IncidenceCounter();
        virtual ~IncidenceCounter();

        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void ConfigureTriggers( const Configuration * inputJson );
        virtual void CheckConfigurationTriggers();

        uint32_t GetCount() const;
        virtual uint32_t GetCountOfQualifyingPopulation( const std::vector<INodeEventContext*>& rNodes );
        virtual void StartCounting();
        virtual void Update( float dt );
        virtual bool IsDoneCounting() const;
        virtual void RegisterForEvents( ISimulationEventContext* pSEC );
        virtual void UnregisterForEvents( ISimulationEventContext* pSEC );
        virtual void RegisterForEvents( INodeEventContext* pNEC );
        virtual void UnregisterForEvents( INodeEventContext* pNEC );
        int32_t GetCountEventsForNumTimeSteps() const;

        // for ReportStatsByIP
        bool IsNodeQualified( INodeEventContext* pNEC );
        individual_qualified_function_t GetIndividualQualifiedFunction();

        // IIndividualEventObserver methods
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context,
            const EventTrigger& StateChange ) override;

    protected:
        uint32_t m_Count;
        PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer> m_NodePropertyRestrictions;
        DemographicRestrictions   m_DemographicRestrictions;
        std::vector<EventTrigger> m_TriggerConditionListIndividual;

    private:
        int32_t m_CountEventsForNumTimeSteps;
        int32_t m_NumTimeStepsCounted;
        bool m_IsDoneCounting;
    };

    // ------------------------------------------------------------------------
    // --- IncidenceEventCoordinator
    // ------------------------------------------------------------------------

    class IncidenceEventCoordinator : public JsonConfigurable
                                    , public IEventCoordinator
                                    , public IEventCoordinatorEventContext
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( EventCoordinatorFactory, IncidenceEventCoordinator, IEventCoordinator )
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        IncidenceEventCoordinator();
        virtual ~IncidenceEventCoordinator();

        virtual bool Configure( const Configuration * inputJson ) override;
        virtual QuickBuilder GetSchema() override;

        virtual IConfigurable* GetConfigurable() override { return JsonConfigurable::GetConfigurable(); }

        // IEventCoordinator methods
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void CheckStartDay( float campaignStartDay ) const override { };
        virtual void AddNode( const suids::suid& suid ) override;
        virtual void Update( float dt ) override;
        virtual void ConsiderResponding();
        virtual void UpdateNodes( float dt ) override;
        virtual bool IsFinished() override;
        virtual IEventCoordinatorEventContext* GetEventContext() override;

        // IEventCoordinatorEventContext methods
        virtual const std::string& GetName() const override;
        virtual const IdmDateTime& GetTime() const override;
        virtual IEventCoordinator* GetEventCoordinator() override;

        // Other methods - for testing
        int GetNumTimeStepsInRep() const { return m_NumTimestepsInRep; }
        int GetNumReps() const { return m_NumReps; }
        int GetEventCount() const { return m_pIncidenceCounter->GetCount(); }
        uint32_t GetQualifyingCount() { return m_pIncidenceCounter->GetCountOfQualifyingPopulation( m_CachedNodes ); }

    protected:
        virtual void ConfigureRepetitions( const Configuration * inputJson );
        virtual void CheckConfigureRepetitions();

        IncidenceEventCoordinator( IncidenceCounter* ic, Responder* pResponder );

        std::string                     m_CoordinatorName;
        ISimulationEventContext*        m_Parent;
        std::vector<INodeEventContext*> m_CachedNodes;
        bool                            m_IsExpired;
        bool                            m_HasBeenDistributed;
        int32_t                         m_NumReps;
        int32_t                         m_NumTimestepsBetweenReps;
        int32_t                         m_NumTimestepsInRep;
        IncidenceCounter*               m_pIncidenceCounter;
        Responder*                      m_pResponder;
    };
}

