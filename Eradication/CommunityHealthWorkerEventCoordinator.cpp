
#include "stdafx.h"

#include "CommunityHealthWorkerEventCoordinator.h"
#include "InterventionFactory.h"
#include "Environment.h"
#include "RANDOM.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "INodeContext.h"
#include "SimulationEventContext.h"
#include "IndividualEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "IDistribution.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "CommunityHealthWorkerEventCoordinator" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- CommunityHealthWorkerEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(CommunityHealthWorkerEventCoordinator)
    IMPL_QUERY_INTERFACE2(CommunityHealthWorkerEventCoordinator, IEventCoordinator, IConfigurable)

    CommunityHealthWorkerEventCoordinator::CommunityHealthWorkerEventCoordinator()
    : m_Parent( nullptr )
    , m_CachedNodes()
    , m_InterventionName()
    , m_pInterventionIndividual( nullptr )
    , m_pInterventionNode( nullptr )
    , m_CoordinatorDaysRemaining(FLT_MAX)
    , m_DemographicRestrictions()
    , m_TriggerConditionList()
    , m_NodePropertyRestrictions()
    , m_MaxDistributedPerDay(UINT_MAX)
    , m_QueueWaitingPeriodDays(FLT_MAX)
    , m_QueueNode()
    , m_QueueIndividual()
    , m_RemoveIndividualEventList()
    , m_MapTime(-1.0)
    , m_InQueueMap()
    , m_pInitialAmount(nullptr)
    , m_CurrentStock(INT_MAX)
    , m_MaxStock(INT_MAX)
    , m_DaysBetweenShipments(FLT_MAX)
    , m_AmountInShipment(INT_MAX)
    , m_DaysToNextShipment(FLT_MAX)
    {
        m_RemoveIndividualEventList.push_back( EventTrigger::DiseaseDeaths    );
        m_RemoveIndividualEventList.push_back( EventTrigger::NonDiseaseDeaths );
        m_RemoveIndividualEventList.push_back( EventTrigger::Emigrating       );
    }

    CommunityHealthWorkerEventCoordinator::~CommunityHealthWorkerEventCoordinator()
    {
        delete m_pInitialAmount;
    }


    QuickBuilder CommunityHealthWorkerEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    bool CommunityHealthWorkerEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Duration",                &m_CoordinatorDaysRemaining, CHW_Duration_DESC_TEXT,                0.0f,     FLT_MAX, FLT_MAX );
        initConfigTypeMap("Max_Distributed_Per_Day", &m_MaxDistributedPerDay,     CHW_Max_Distributed_Per_Day_DESC_TEXT, 1,        INT_MAX, INT_MAX );
        initConfigTypeMap("Waiting_Period",          &m_QueueWaitingPeriodDays,   CHW_Waiting_Period_DESC_TEXT,          0.0f,     FLT_MAX, FLT_MAX );
        initConfigTypeMap("Days_Between_Shipments",  &m_DaysBetweenShipments,     CHW_Days_Between_Shipments_DESC_TEXT,  1.0f,     FLT_MAX, FLT_MAX );
        initConfigTypeMap("Amount_In_Shipment",      &m_AmountInShipment,         CHW_Amount_In_Shipment_DESC_TEXT,      0,        INT_MAX, INT_MAX );
        initConfigTypeMap("Max_Stock",               &m_MaxStock,                 CHW_Max_Stock_DESC_TEXT,               0,        INT_MAX, INT_MAX );

        DistributionFunction::Enum initial_amount_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Initial_Amount_Distribution", initial_amount_function, inputJson, MetadataDescriptor::Enum("Initial_Amount_Distribution_Type", CHW_Initial_Amount_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)) );

        m_pInitialAmount = DistributionFactory::CreateDistribution( this, initial_amount_function, "Initial_Amount", inputJson );

        m_DemographicRestrictions.ConfigureRestrictions( this, inputJson );

        initConfigTypeMap( "Trigger_Condition_List", &m_TriggerConditionList, CHW_Trigger_Condition_List_DESC_TEXT );

        initConfigComplexType( "Node_Property_Restrictions", &m_NodePropertyRestrictions, Node_Property_Restrictions_DESC_TEXT );

        InterventionConfig intervention_config;
        initConfigComplexType( "Intervention_Config", &intervention_config, Intervention_Config_DESC_TEXT );

        bool retValue = JsonConfigurable::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            // ------------------------------------------------------
            // --- Check that the intervention exists and initialize
            // ------------------------------------------------------
            m_pInterventionIndividual = InterventionFactory::CreateIntervention( intervention_config._json, inputJson->GetDataLocation(), "Intervention_Config", false ); // don't throw if null
            if( m_pInterventionIndividual == nullptr )
            {
                m_pInterventionNode = InterventionFactory::CreateNDIIntervention( intervention_config._json, inputJson->GetDataLocation(), "Intervention_Config", false ); // don't throw if null
            }
            if( (m_pInterventionIndividual == nullptr) && (m_pInterventionNode == nullptr) )
            {
                std::string class_name = std::string(json::QuickInterpreter( intervention_config._json )["class"].As<json::String>()) ;

                std::stringstream ss;
                ss << "Invalid Intervention Type in 'CommunityHealthWorkerEventCoordinator'.\n";
                ss << "'" << class_name << "' is not a known intervention.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            m_DemographicRestrictions.CheckConfiguration();

            m_InterventionName = std::string( json::QuickInterpreter(intervention_config._json)["class"].As<json::String>() );

            // ---------------------------------------------------------------------------
            // --- If the user is attempting to define demographic restrictions when they
            // --- are using a node level intervention, then we need to error because these
            // --- restrictions are not doing anything.
            // ---------------------------------------------------------------------------
            if( (m_pInterventionNode != nullptr) && !m_DemographicRestrictions.HasDefaultRestrictions() )
            {
                std::ostringstream msg ;
                msg << "In CommunityHealthWorkerEventCoordinator, demographic restrictions such as 'Demographic_Coverage'\n";
                msg << "and 'Target_Gender' do not apply when distributing nodel level interventions such as ";
                msg << m_InterventionName;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            for( auto& event_name : m_RemoveIndividualEventList )
            {
                m_TriggerConditionList.push_back( event_name );
            }
        }

        return retValue;
    }

    void CommunityHealthWorkerEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        m_Parent = isec;
    }

    void CommunityHealthWorkerEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        m_CachedNodes.push_back( pNEC );

        RegisterForEvents( pNEC );

        // --------------------
        // --- Initialize Stock
        // --------------------
        if( m_CurrentStock == INT_MAX )
        {
            m_CurrentStock = int( m_pInitialAmount->Calculate( pNEC->GetRng() ) + 0.5 ); // round up
            if( m_CurrentStock > m_MaxStock )
            {
                m_CurrentStock = m_MaxStock;
            }

            m_DaysToNextShipment = int( float( m_CurrentStock ) / float( m_MaxDistributedPerDay ) );
            if( m_DaysToNextShipment > m_DaysBetweenShipments )
            {
                m_DaysToNextShipment = m_DaysBetweenShipments;
            }
        }
    }

    template<typename T>
    void RemovePastWaitingPeriod( float currentTime, 
                                  float waitingPeriodDays, 
                                  std::list<QueueEntry<T>>& rQueue )
    {
        bool done = (rQueue.size() == 0);
        while( !done )
        {
            QueueEntry<T> entry = rQueue.front();

            if( (currentTime - entry.time_of_queue_entry_days) > waitingPeriodDays )
            {
                rQueue.pop_front();
            }
            else
            {
                done = true;
            }

            done = done || (rQueue.size() == 0);
        }
    }

    template<typename T>
    int ProcessQueue( CommunityHealthWorkerEventCoordinator* pThis, 
                      float currentTime, 
                      int numToDistribute, 
                      std::list<QueueEntry<T>>& rQueue )
    {
        int num_distributed = 0;

        bool done = (rQueue.size() == 0) || (numToDistribute == 0);
        while( !done )
        {
            QueueEntry<T> entry = rQueue.front();

            if( entry.time_of_queue_entry_days == currentTime )
            {
                // ----------------------------------------------------------------------------------
                // --- Entities can enter the queue before and after Update() & UpdateNodes()
                // --- are called for this coordinator.  Hence, we want to make the processing
                // --- consistent and don't want to process entities that entered the queue during
                // --- this time step.  If an entity enters on day T and the waiting period is 5 days,
                // --- then the entity will time out on the day > T+5.  For example, if an entity
                // --- enters on day 2 and the waiting period is 5 days, they could get the
                // --- intervention as late as day 7 (i.e. 5 days of opportunity).  On day 8, they
                // --- should be removed from the queue.
                // ----------------------------------------------------------------------------------
                done = true;
            }
            else if( numToDistribute > 0 )
            {
                if( pThis->Qualifies( entry.p_entity ) )
                {
                    bool distributed = pThis->Distribute( entry.p_entity );
                    if( distributed )
                    {
                        --numToDistribute;
                        num_distributed++;
                    }
                }
                rQueue.pop_front();
            }
            else
            {
                done = true;
            }

            done = done || (rQueue.size() == 0);
        }

        return num_distributed;
    }

    // Update() is called for all the coordinators before calling UpdateNodes()
    void CommunityHealthWorkerEventCoordinator::Update( float dt )
    {
        // ----------------
        // --- Update Stock
        // ----------------
        if( m_DaysToNextShipment <= 0.0 )
        {
            m_DaysToNextShipment = m_DaysBetweenShipments;
            m_CurrentStock += m_AmountInShipment;
            if( m_CurrentStock > m_MaxStock )
            {
                m_CurrentStock = m_MaxStock;
            }
        }
        else
        {
            m_DaysToNextShipment -= dt;
        }

        // -----------------------------------------------------------------------
        // --- Update how long this coordinator has to run before it is finished.
        // --- UpdateNodes() will be called before IsFinished() checked and the
        // --- coordinator deleted.
        // -----------------------------------------------------------------------
        m_CoordinatorDaysRemaining -= dt;

        // ----------------------------------------------------------------------------------
        // --- Remove entities from the front of the queue that are past the waiting period.
        // ----------------------------------------------------------------------------------
        float current_time = m_Parent->GetSimulationTime().time;

        // +dt because you can't be distributed to on the time step you enter the queue
        float wait = m_QueueWaitingPeriodDays + dt;

        if( m_pInterventionNode != nullptr )
        {
            RemovePastWaitingPeriod( current_time, wait, m_QueueNode );
        }
        else
        {
            RemovePastWaitingPeriod( current_time, wait, m_QueueIndividual );
        }
    }

    // ------------------------------------------------------------------------------------------------------
    // --- WARNING: This does not use the IVisitIndividual interface method of distributing the intervention
    // --- like StandardInterventionDistributionEventCoordinator.
    // ------------------------------------------------------------------------------------------------------
    void CommunityHealthWorkerEventCoordinator::UpdateNodes( float dt )
    {
        int num_to_distribute = int( float(m_MaxDistributedPerDay) * dt );
        if( num_to_distribute > m_CurrentStock )
        {
            num_to_distribute = m_CurrentStock;
        }

        float current_time = m_Parent->GetSimulationTime().time;
        int num_distributed = 0;
        if( m_pInterventionNode != nullptr )
        {
            num_distributed = ProcessQueue( this, current_time, num_to_distribute, m_QueueNode );
        }
        else
        {
            num_distributed = ProcessQueue( this, current_time, num_to_distribute, m_QueueIndividual );
        }
        m_CurrentStock -= num_distributed;

        std::stringstream ss;
        ss << "UpdateNodes() gave out " << num_distributed << " '" << m_InterventionName.c_str() << "' interventions; " << m_CurrentStock << " remaining in stock\n";
        LOG_INFO( ss.str().c_str() );
    }

    bool CommunityHealthWorkerEventCoordinator::Qualifies( INodeEventContext* pNEC )
    {
        bool qualifies = m_NodePropertyRestrictions.Qualifies( pNEC->GetNodeContext()->GetNodeProperties() );
        return qualifies;
    }

    bool CommunityHealthWorkerEventCoordinator::Qualifies( IIndividualHumanEventContext* pIHEC )
    {
        bool qualifies = m_NodePropertyRestrictions.Qualifies( pIHEC->GetNodeEventContext()->GetNodeContext()->GetNodeProperties() );
        qualifies = qualifies && m_DemographicRestrictions.IsQualified( pIHEC );
        return qualifies;
    }

    bool CommunityHealthWorkerEventCoordinator::Distribute( INodeEventContext* pNEC )
    {
        release_assert( m_pInterventionNode );

        // Huge performance win by cloning instead of configuring.
        INodeDistributableIntervention *ndi = m_pInterventionNode->Clone();
        release_assert( ndi );

        ndi->AddRef();

        bool distributed =  ndi->Distribute( pNEC, nullptr );

        if( distributed )
        {
            LOG_DEBUG_F("Distributed '%s' intervention to node %d\n", m_InterventionName.c_str(), pNEC->GetExternalId() );
        }

        ndi->Release();

        return distributed;
    }

    bool CommunityHealthWorkerEventCoordinator::Distribute( IIndividualHumanEventContext* pIHEC )
    {
        release_assert( m_pInterventionIndividual );

        IDistributableIntervention *di = m_pInterventionIndividual->Clone();
        release_assert(di);

        ICampaignCostObserver* p_icco = nullptr;
        if (s_OK != pIHEC->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&p_icco))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIHEC->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext" );
        }

        di->AddRef();

        bool distributed = di->Distribute( pIHEC->GetInterventionsContext(), p_icco );

        if( distributed )
        {
            LOG_DEBUG_F("Distributed '%s' intervention to individual %d\n", m_InterventionName.c_str(), pIHEC->GetSuid().data );
        }

        di->Release();

        return distributed;
    }

    bool CommunityHealthWorkerEventCoordinator::IsFinished()
    {
        bool is_finished = (m_CoordinatorDaysRemaining <= 0.0f);
        if( is_finished )
        {
            for( auto pNEC : m_CachedNodes )
            {
                UnregisterForEvents( pNEC );
            }
        }
        return is_finished;
    }


    bool CommunityHealthWorkerEventCoordinator::IsRemoveIndividualEvent( const EventTrigger& rTrigger ) const
    {
        bool found = (std::find( m_RemoveIndividualEventList.begin(),
                                 m_RemoveIndividualEventList.end(),
                                 rTrigger ) != m_RemoveIndividualEventList.end());

        return found;
    }

    void CommunityHealthWorkerEventCoordinator::RemoveEntity( IIndividualHumanEventContext *context )
    {
        // --------------------------------------------------------------------------------------
        // --- An entity could be in the queue multiple times so we need to remove all instances.
        // --------------------------------------------------------------------------------------
        release_assert( context != nullptr );
        std::list<QueueEntry<IIndividualHumanEventContext>>::iterator it = m_QueueIndividual.begin();
        bool done = (it == m_QueueIndividual.end());
        while( !done )
        {
            release_assert( it->p_entity != nullptr );
            if( it->p_entity->GetSuid() == context->GetSuid() )
            {
                it = m_QueueIndividual.erase( it );
            }
            else
            {
                ++it;
            }
            done = (it == m_QueueIndividual.end());
        }
    }

    bool CommunityHealthWorkerEventCoordinator::notifyOnEvent( IIndividualHumanEventContext *context, 
                                                              const EventTrigger& trigger)
    {
        release_assert( context != nullptr );
        if( IsRemoveIndividualEvent( trigger ) )
        {
            RemoveEntity( context );
        }
        else
        {
            float current_time = m_Parent->GetSimulationTime().time;

            if( m_pInterventionNode != nullptr )
            {
                INodeEventContext* p_nec = context->GetNodeEventContext();
                AddEntity( current_time, p_nec->GetId().data, p_nec, m_QueueNode );
            }
            else
            {
                AddEntity( current_time, context->GetSuid().data, context, m_QueueIndividual );
            }
        }
        return true;
    }

    bool CommunityHealthWorkerEventCoordinator::AlreadyInQueue( float currentTime, uint32_t id )
    {
        if( currentTime != m_MapTime )
        {
            m_InQueueMap.clear();
            return false;
        }
        else
        {
            return (m_InQueueMap.count( id ) > 0);
        }
    }

    void CommunityHealthWorkerEventCoordinator::RegisterForEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto& trigger : m_TriggerConditionList )
        {
            broadcaster->RegisterObserver( this, trigger );
        }
    }

    void CommunityHealthWorkerEventCoordinator::UnregisterForEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto& trigger : m_TriggerConditionList )
        {
            broadcaster->UnregisterObserver( this, trigger );
        }
    }

    int CommunityHealthWorkerEventCoordinator::GetCurrentStock() const
    { 
        return m_CurrentStock;
    }

    float CommunityHealthWorkerEventCoordinator::GetDaysToNextShipment() const
    { 
        return m_DaysToNextShipment;
    }

    float CommunityHealthWorkerEventCoordinator::GetDaysRemaining() const
    {
        return m_CoordinatorDaysRemaining;
    }

    int CommunityHealthWorkerEventCoordinator::GetNumEntitiesInQueue() const
    {
        if( m_pInterventionNode != nullptr )
        {
            return m_QueueNode.size();
        }
        else
        {
            return m_QueueIndividual.size();
        }
    }
}
