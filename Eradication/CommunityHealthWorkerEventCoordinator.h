
#pragma once

#include "EventCoordinator.h"
#include "Configure.h"
#include "DemographicRestrictions.h"
#include "Interventions.h"

namespace Kernel
{
    struct IDistribution;
    struct INodeEventContext;
    struct IIndividualHumanEventContext;

    template<typename T>
    struct QueueEntry
    {
        QueueEntry()
        : time_of_queue_entry_days(0.0)
        , p_entity(nullptr)
        {
        }

        QueueEntry( float currentTime, T* pEntity )
        : time_of_queue_entry_days(currentTime)
        , p_entity(pEntity)
        {
        }

        float time_of_queue_entry_days;
        T*    p_entity;
    } ;

    // ------------------------------------------------------------------------
    // --- CommunityHealthWorkerEventCoordinator
    // ------------------------------------------------------------------------

    class IDMAPI CommunityHealthWorkerEventCoordinator : public IEventCoordinator, public JsonConfigurable, public IIndividualEventObserver
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, CommunityHealthWorkerEventCoordinator, IEventCoordinator)    
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        CommunityHealthWorkerEventCoordinator();
        virtual ~CommunityHealthWorkerEventCoordinator();

        virtual bool Configure( const Configuration * inputJson ) override;
        virtual QuickBuilder GetSchema() override;

        virtual IConfigurable* GetConfigurable() override { return JsonConfigurable::GetConfigurable(); }

        // IEventCoordinator methods
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void CheckStartDay( float campaignStartDay ) const override {};
        virtual void AddNode( const suids::suid& suid ) override;
        virtual void Update( float dt ) override;
        virtual void UpdateNodes( float dt ) override;
        virtual bool IsFinished() override;
        virtual IEventCoordinatorEventContext* GetEventContext() override { return nullptr; }

        // IIndividualEventObserver methods
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& StateChange) override;

        // Other Methods
        bool Qualifies( INodeEventContext* pNEC );
        bool Qualifies( IIndividualHumanEventContext* pIHEC );

        bool Distribute( INodeEventContext* pNEC );
        bool Distribute( IIndividualHumanEventContext* pIHEC );

        int   GetCurrentStock()       const;
        float GetDaysToNextShipment() const;
        int   GetNumEntitiesInQueue() const;
        float GetDaysRemaining()      const;

    protected:

        bool AlreadyInQueue( float currentTime, uint32_t id );
        void RegisterForEvents( INodeEventContext* pNEC );
        void UnregisterForEvents( INodeEventContext* pNEC );

        bool IsRemoveIndividualEvent( const EventTrigger& rTrigger ) const;
        void RemoveEntity( IIndividualHumanEventContext *context );

        template<typename T>
        void AddEntity( float currentTime, uint32_t id, T* pEntity, std::list<QueueEntry<T>>& rQueue )
        {
            if( !AlreadyInQueue( currentTime, id ) )
            {
                rQueue.push_back( QueueEntry<T>( currentTime, pEntity ) );
                m_InQueueMap.insert( std::make_pair( id, currentTime ) );
                m_MapTime = currentTime;
            }
        }

        ISimulationEventContext*        m_Parent;
        std::vector<INodeEventContext*> m_CachedNodes;
        std::string                     m_InterventionName;
        IDistributableIntervention*     m_pInterventionIndividual;
        INodeDistributableIntervention* m_pInterventionNode;

        float                           m_CoordinatorDaysRemaining;
        DemographicRestrictions         m_DemographicRestrictions;
        std::vector<EventTrigger>       m_TriggerConditionList;
        PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer> m_NodePropertyRestrictions;

        int                                                 m_MaxDistributedPerDay; //interventions per day
        float                                               m_QueueWaitingPeriodDays;
        std::list<QueueEntry<INodeEventContext>>            m_QueueNode;
        std::list<QueueEntry<IIndividualHumanEventContext>> m_QueueIndividual;
        std::vector<EventTrigger>                           m_RemoveIndividualEventList;

        float                    m_MapTime;
        std::map<uint32_t,float> m_InQueueMap;

        IDistribution* m_pInitialAmount;
        int   m_CurrentStock;
        int   m_MaxStock;
        float m_DaysBetweenShipments;
        int   m_AmountInShipment;
        float m_DaysToNextShipment;
    };
}

