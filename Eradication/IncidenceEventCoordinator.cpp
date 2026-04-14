
#include "stdafx.h"

#include "IncidenceEventCoordinator.h"
#include "InterventionFactory.h"
#include "Environment.h"
#include "RANDOM.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "INodeContext.h"
#include "ISimulationContext.h"
#include "SimulationEventContext.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "IIndividualHuman.h"
#include "BaseEventTrigger.h"
#include "EventTriggerNode.h"
#include "EventTriggerCoordinator.h"

SETUP_LOGGING( "IncidenceEventCoordinator" )


namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- Action
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( Action )
    END_QUERY_INTERFACE_BODY( Action )

    Action::Action()
        : JsonConfigurable()
        , m_Threshold( 0 )
        , m_EventToBroadcast()
        , m_EventType(EventType::INDIVIDUAL)
        , m_TriggerCoordinator()
        , m_TriggerNode()
        , m_TriggerIndividual()
    {
    }

    Action::~Action()
    {
    }

    bool Action::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Threshold", &m_Threshold, ICE_Action_Threshold_DESC_TEXT, 0.0, FLT_MAX, 0.0 );
        initConfig("Event_Type", m_EventType, inputJson, MetadataDescriptor::Enum("Event_Type", ICE_Action_Event_Type_DESC_TEXT, MDD_ENUM_ARGS( EventType )));
        initConfigTypeMap( "Event_To_Broadcast", &m_EventToBroadcast, ICE_Action_Event_To_Broadcast_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckConfigurationTriggers();
        }
        return ret;
    }

    void Action::CheckConfigurationTriggers()
    {
        if( m_EventToBroadcast.empty() || (m_EventToBroadcast == JsonConfigurable::default_string) )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must define Event_To_Broadcast" );
        } 

        switch( m_EventType )
        {
            case EventType::INDIVIDUAL:
            {
                m_TriggerIndividual = EventTriggerFactory::GetInstance()->CreateTrigger( "Event_To_Broadcast", m_EventToBroadcast );
                break;
            }
            case EventType::NODE:
            {
                m_TriggerNode = EventTriggerNodeFactory::GetInstance()->CreateTrigger( "Event_To_Broadcast", m_EventToBroadcast );
                break;
            }
            case EventType::COORDINATOR:
            {
                m_TriggerCoordinator = EventTriggerCoordinatorFactory::GetInstance()->CreateTrigger( "Event_To_Broadcast", m_EventToBroadcast );
                break;
            }
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "Event_Type", m_EventType, EventType::pairs::lookup_key( m_EventType ) );
        }
    }


    float Action::GetThreshold() const
    {
        return m_Threshold;
    }

    const std::string& Action::GetEventToBroadcast() const
    {
        return m_EventToBroadcast;
    }

    // ------------------------------------------------------------------------
    // --- ActionList
    // ------------------------------------------------------------------------

    ActionList::ActionList()
        : JsonConfigurableCollection("ActionList")
    {
    }

    ActionList::~ActionList()
    {
    }

    bool GreaterThan( Action* pLeft, Action* pRight )
    {
        return (pLeft->GetThreshold() > pRight->GetThreshold());
    }

    void ActionList::CheckConfiguration()
    {
        if( Size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "At least one action must be defined for the IncidenceEventCoordinator.");
        }

        // sort into decending order
        std::sort( m_Collection.begin(), m_Collection.end(), GreaterThan );

        // Check if any of the thresholds equal another
        for( int i = 1; i < m_Collection.size(); ++i )
        {
            if( m_Collection[ i - 1 ]->GetThreshold() == m_Collection[ i ]->GetThreshold() )
            {
                std::stringstream ss;
                ss << "More than one action has a Threshold equal to " << m_Collection[ i ]->GetThreshold() << ".  Threshold values must be unique.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    Action* ActionList::CreateObject()
    {
        Action* p_act = new Action();
        return p_act;
    }

    // ------------------------------------------------------------------------
    // --- Responder
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( Responder )
    END_QUERY_INTERFACE_BODY( Responder )

    Responder::Responder()
        : JsonConfigurable()
        , m_ThresholdType(ThresholdType::COUNT)
        , m_ActionList()
        , m_pCurrentAction( nullptr )
        , m_sim(nullptr)
        , m_Parent(nullptr)
    {
    }

    Responder::~Responder()
    {
    }

    bool Responder::Configure( const Configuration * inputJson )
    {
        initConfig( "Threshold_Type", m_ThresholdType, inputJson, MetadataDescriptor::Enum( "ThresholdType", ICE_Threshold_Type_DESC_TEXT, MDD_ENUM_ARGS( ThresholdType ) ) );

        initConfigComplexCollectionType( "Action_List", &m_ActionList, ICE_Action_List_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckConfiguration( inputJson );
            m_ActionList.CheckConfiguration();
        }
        return ret;
    }

    void Responder::CheckConfiguration( const Configuration * inputJson )
    {
        if( m_ThresholdType == ThresholdType::PERCENTAGE_EVENTS )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "IncidenceEventCoordinator does not support 'Threshold_Type' == 'PERCENTAGE_EVENTS'" );
        }
    }

    bool Responder::visitIndividualCallback( IIndividualHumanEventContext *ihec, float & incrementalCostOut, ICampaignCostObserver * pICCO )
    {
        release_assert( m_pCurrentAction != nullptr );

        IIndividualEventBroadcaster* broadcaster = ihec->GetNodeEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( ihec, m_pCurrentAction->GetEventToBroadcastIndividual() );

        return true;
    }

    bool Responder::Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation )
    {
        float incidence_value = CalculateIncidence( numIncidences, qualifyingPopulation );

        m_pCurrentAction = GetAction( incidence_value );

        // If p_action = nullptr, then a threshold was not achieved and nothing will be broadcasted.
        if( m_pCurrentAction != nullptr )
        {
            int num_distributed = 0;
            std::stringstream ss;

            switch(m_pCurrentAction->GetEventType())
            {
            case EventType::INDIVIDUAL:
                for (INodeEventContext* p_node : nodes)
                {
                    num_distributed += p_node->VisitIndividuals( this );
                    ss << "Distribute() broadcasted '" << m_pCurrentAction->GetEventToBroadcast() << "' to " << num_distributed << " individuals\n";
                }
                break;
            case EventType::NODE:
                for (INodeEventContext* p_node : nodes)
                {
                    auto TriggerNode = m_pCurrentAction->GetEventToBroadcastNode();
                    p_node->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster()->TriggerObservers(p_node, TriggerNode);
                    ss << "Distribute() broadcasted Node Event: '" << TriggerNode.ToString() << "'\n";
                }
                break;
            case EventType::COORDINATOR:
                auto TriggerCoordinator = m_pCurrentAction->GetEventToBroadcastCooridnator();
                m_sim->GetCoordinatorEventBroadcaster()->TriggerObservers(m_Parent, TriggerCoordinator);
                ss << "Distribute() broadcasted Coordinator Event: '" << TriggerCoordinator.ToString() << "'\n";
                break;
            }
            LOG_INFO( ss.str().c_str() );
        }
        return (m_pCurrentAction != nullptr);
    }

    void Responder::SetContextTo(ISimulationEventContext* sim, IEventCoordinatorEventContext *isec)
    {
        m_sim = sim;
        m_Parent = isec;
    }

    float Responder::CalculateIncidence( uint32_t numIncidences, uint32_t qualifyingPopulation ) const
    {
        float value = float( numIncidences );
        if( (m_ThresholdType == ThresholdType::PERCENTAGE       ) ||
            (m_ThresholdType == ThresholdType::PERCENTAGE_EVENTS) )
        {
            if( qualifyingPopulation == 0 )
            {
                value = 0.0;
            }
            else
            {
                value = value / float( qualifyingPopulation );
            }
        }
        return value;
    }

    Action* Responder::GetAction( float value )
    {
        Action* p_action = nullptr;
        for( int i = 0; i < m_ActionList.Size(); ++i )
        {
            if( value >= m_ActionList[ i ]->GetThreshold() )
            {
                p_action = m_ActionList[ i ];
                break;
            }
        }
        return p_action;
    }

    const Action* Responder::GetCurrentAction() const
    {
        return m_pCurrentAction;
    }

    // ------------------------------------------------------------------------
    // --- IncidenceCounter
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY( IncidenceCounter )
    END_QUERY_INTERFACE_BODY( IncidenceCounter )

    IncidenceCounter::IncidenceCounter()
        : JsonConfigurable()
        , m_Count(0)
        , m_NodePropertyRestrictions()
        , m_DemographicRestrictions(ICE_Demographic_Coverage_DESC_TEXT)
        , m_TriggerConditionListIndividual()
        , m_CountEventsForNumTimeSteps(1)
        , m_NumTimeStepsCounted(-1)
        , m_IsDoneCounting(false)  
    {
        m_NumTimeStepsCounted = -1;
    }

    IncidenceCounter::~IncidenceCounter()
    {
    }

    bool IncidenceCounter::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Count_Events_For_Num_Timesteps", &m_CountEventsForNumTimeSteps, ICE_Count_Events_For_Num_Timesteps_DESC_TEXT, 1, INT_MAX, 1 );
        initConfigComplexType( "Node_Property_Restrictions", &m_NodePropertyRestrictions, Node_Property_Restrictions_DESC_TEXT );
        
        m_DemographicRestrictions.ConfigureRestrictions( this, inputJson );
        ConfigureTriggers( inputJson );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_DemographicRestrictions.CheckConfiguration();
            CheckConfigurationTriggers();
        }
        return ret;
    }

    void IncidenceCounter::ConfigureTriggers( const Configuration * inputJson )
    {
        initConfigTypeMap( "Trigger_Condition_List", &m_TriggerConditionListIndividual, ICE_Trigger_Condition_List_DESC_TEXT );        
    }

    void IncidenceCounter::CheckConfigurationTriggers()
    {
        //overwritten by child classes
    }

    bool IncidenceCounter::notifyOnEvent( IIndividualHumanEventContext *context,
                                          const EventTrigger& trigger )
    {
        if( m_NodePropertyRestrictions.Qualifies( context->GetNodeEventContext()->GetNodeContext()->GetNodeProperties() ) &&
            m_DemographicRestrictions.IsQualified( context ) &&
            !IsDoneCounting() )
        {
            float demographic_coverage = m_DemographicRestrictions.GetDemographicCoverage();
            if( (demographic_coverage > 0.0) )
            {
                bool count_event = true;
                // don't draw random number if coverage equals 1.
                if( demographic_coverage < 1.0 )
                {
                    RANDOMBASE* p_rng = context->GetInterventionsContext()->GetParent()->GetRng();
                    count_event = p_rng->SmartDraw( demographic_coverage );
                }
                if( count_event )
                {
                    ++m_Count;
                }
            }
        }
        return true;
    }

    uint32_t IncidenceCounter::GetCount() const
    {
        return m_Count;
    }

    // I can't seem to make this const due to the use of individual_visit_function
    uint32_t IncidenceCounter::GetCountOfQualifyingPopulation( const std::vector<INodeEventContext*>& rNodes )
    {
        uint32_t pop = 0;

        INodeEventContext::individual_visit_function_t fn =
            [ this, &pop ]( IIndividualHumanEventContext *phec )
        {
            if( m_NodePropertyRestrictions.Qualifies( phec->GetNodeEventContext()->GetNodeContext()->GetNodeProperties() ) &&
                m_DemographicRestrictions.IsQualified( phec ) )
            {
                ++pop;
            }
        };

        for( INodeEventContext* p_nec : rNodes )
        {
            p_nec->VisitIndividuals( fn );
        }

        return pop;
    }

    void IncidenceCounter::StartCounting()
    {
        m_NumTimeStepsCounted = 0;
        m_IsDoneCounting = false;
        m_Count = 0;
    }

    void IncidenceCounter::Update( float dt )
    {
        ++m_NumTimeStepsCounted;
        if( m_NumTimeStepsCounted >= m_CountEventsForNumTimeSteps )
        {
            m_IsDoneCounting = true;
        }
    }

    bool IncidenceCounter::IsDoneCounting() const
    {
        return m_IsDoneCounting;
    }

    void IncidenceCounter::RegisterForEvents( ISimulationEventContext * pSEC )
    {
        //to be overridden
    }

    void IncidenceCounter::UnregisterForEvents( ISimulationEventContext * pSEC )
    {
        //to be overridden
    }

    void IncidenceCounter::RegisterForEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto& trigger : m_TriggerConditionListIndividual )
        {
            broadcaster->RegisterObserver( this, trigger );
        }
    }

    void IncidenceCounter::UnregisterForEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto& trigger : m_TriggerConditionListIndividual )
        {
            broadcaster->UnregisterObserver( this, trigger );
        }
    }

    int32_t IncidenceCounter::GetCountEventsForNumTimeSteps() const
    {
        return m_CountEventsForNumTimeSteps;
    }

    bool IncidenceCounter::IsNodeQualified( INodeEventContext* pNEC )
    {
        return m_NodePropertyRestrictions.Qualifies( pNEC->GetNodeContext()->GetNodeProperties() );
    }

    individual_qualified_function_t IncidenceCounter::GetIndividualQualifiedFunction()
    {
        individual_qualified_function_t qual_func =
            [ this ]( IIndividualHumanEventContext* pIHEC )
        {
            return m_DemographicRestrictions.IsQualified( pIHEC );
        };
        return qual_func;
    }

    // ------------------------------------------------------------------------
    // --- IncidenceEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED( IncidenceEventCoordinator )

    BEGIN_QUERY_INTERFACE_BODY( IncidenceEventCoordinator )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IEventCoordinator )
        HANDLE_ISUPPORTS_VIA( IEventCoordinator )
    END_QUERY_INTERFACE_BODY( IncidenceEventCoordinator )

    IncidenceEventCoordinator::IncidenceEventCoordinator()
        : JsonConfigurable()
        , m_Parent(nullptr)
        , m_CachedNodes()
        , m_IsExpired(false)
        , m_HasBeenDistributed(false)
        , m_NumReps(-1)
        , m_NumTimestepsBetweenReps(1)
        , m_NumTimestepsInRep(0)
        , m_pIncidenceCounter(new IncidenceCounter())
        , m_pResponder(new Responder)
    {
        // Start at zero in constructor so it gets set to 1 on first call to Update()
        m_NumTimestepsInRep = 0;
    }

    IncidenceEventCoordinator::IncidenceEventCoordinator( IncidenceCounter* ic, Responder* pResponder )
        : JsonConfigurable()
        , m_CoordinatorName("IncidenceEventCoordinator")
        , m_Parent( nullptr )
        , m_CachedNodes()
        , m_IsExpired( false )
        , m_HasBeenDistributed( false )
        , m_NumReps( -1 )
        , m_NumTimestepsBetweenReps(1)
        , m_NumTimestepsInRep(0)
        , m_pIncidenceCounter(ic)
        , m_pResponder(pResponder)
    {
        // Start at zero in constructor so it gets set to 1 on first call to Update()
        m_NumTimestepsInRep = 0;
    }

    IncidenceEventCoordinator::~IncidenceEventCoordinator()
    {
        delete m_pIncidenceCounter;
        delete m_pResponder;
    }


    QuickBuilder IncidenceEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    bool IncidenceEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Coordinator_Name", &m_CoordinatorName, Coordinator_Name_DESC_TEXT, m_CoordinatorName.c_str() );
        initConfigTypeMap( "Incidence_Counter", m_pIncidenceCounter, ICE_Incidence_Counter_DESC_TEXT );
        initConfigTypeMap( "Responder", m_pResponder, ICE_Responder_DESC_TEXT );

        ConfigureRepetitions( inputJson );

        bool retValue = JsonConfigurable::Configure( inputJson );
        if( retValue && !JsonConfigurable::_dryrun )
        {
            CheckConfigureRepetitions();
        }
        return retValue;
    }

    void IncidenceEventCoordinator::ConfigureRepetitions( const Configuration * inputJson )
    {
        initConfigTypeMap( "Number_Repetitions", &m_NumReps, Number_Repetitions_DESC_TEXT, -1, 10000, 1 );
        initConfigTypeMap( "Timesteps_Between_Repetitions", &m_NumTimestepsBetweenReps, Timesteps_Between_Repetitions_DESC_TEXT, -1, 10000, -1 );
    }

    void IncidenceEventCoordinator::CheckConfigureRepetitions()
    {
        if( m_NumTimestepsBetweenReps < m_pIncidenceCounter->GetCountEventsForNumTimeSteps() )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                "Timesteps_Between_Repetitions", m_NumTimestepsBetweenReps,
                "Count_Events_For_Num_Timesteps", m_pIncidenceCounter->GetCountEventsForNumTimeSteps(),
                "'Timesteps_Between_Repetitions' must be >= 'Count_Events_For_Num_Timesteps'" );
        }
    }

    void IncidenceEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        m_Parent = isec;
        m_pIncidenceCounter->RegisterForEvents( isec );
        m_pResponder->SetContextTo( isec, GetEventContext() );
    }

    void IncidenceEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        m_CachedNodes.push_back( pNEC );

        m_pIncidenceCounter->RegisterForEvents( pNEC );
    }

    // Update() is called for all the coordinators before calling UpdateNodes()
    void IncidenceEventCoordinator::Update( float dt )
    {
        // -----------------------------------------------------------------------
        // --- Update the counter of time steps in this rep.
        // --- UpdateNodes() will be called before IsFinished() is checked and the
        // --- coordinator deleted.
        // -----------------------------------------------------------------------
        ++m_NumTimestepsInRep;
    }

    void IncidenceEventCoordinator::ConsiderResponding()
    {
        if( m_pIncidenceCounter->IsDoneCounting() && !m_HasBeenDistributed )
        {
            uint32_t num_incidences = m_pIncidenceCounter->GetCount();
            uint32_t num_qualifying_pop = m_pIncidenceCounter->GetCountOfQualifyingPopulation( m_CachedNodes );
            m_pResponder->Distribute( m_CachedNodes, num_incidences, num_qualifying_pop );
            m_HasBeenDistributed = true;
        }

        if( m_NumTimestepsInRep > m_NumTimestepsBetweenReps )
        {
            // Start at 1 because we are setting this after Update()
            m_NumTimestepsInRep = 1;

            m_pIncidenceCounter->StartCounting();
            m_HasBeenDistributed = false;

            if( m_NumReps > -1 )
            {
                --m_NumReps;
                m_IsExpired = (m_NumReps <= 0);
            }
        }
    }

    void IncidenceEventCoordinator::UpdateNodes( float dt )
    {
        m_pIncidenceCounter->Update( dt );
        ConsiderResponding();
    }

    bool IncidenceEventCoordinator::IsFinished()
    {
        if( m_IsExpired )
        {
            for( auto pNEC : m_CachedNodes )
            {                
                m_pIncidenceCounter->UnregisterForEvents( pNEC );    //unregister from node events 
            }

            m_pIncidenceCounter->UnregisterForEvents( m_Parent );    //unregister from coordinator event
        }
        return m_IsExpired;
    }

    IEventCoordinatorEventContext* IncidenceEventCoordinator::GetEventContext()
    {
        return this;
    }

    const std::string& IncidenceEventCoordinator::GetName() const
    {
        return m_CoordinatorName;
    }

    const IdmDateTime& IncidenceEventCoordinator::GetTime() const
    {
        release_assert( m_Parent );
        return m_Parent->GetSimulationTime();
    }

    IEventCoordinator* IncidenceEventCoordinator::GetEventCoordinator()
    {
        return this;
    }
}

