
#include "stdafx.h"
#include "TriggeredEventCoordinator.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "TriggeredEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED( TriggeredEventCoordinator )
    BEGIN_QUERY_INTERFACE_BODY(TriggeredEventCoordinator)
    END_QUERY_INTERFACE_BODY(TriggeredEventCoordinator)

    TriggeredEventCoordinator::TriggeredEventCoordinator()
        : StandardInterventionDistributionEventCoordinator( true )
        , m_IsActive( false )
        , m_IsStarting( false )
        , m_IsStopping( false )
        , m_Duration( -1.0f )
        , m_DurationExpired( false )
        , m_CoordinatorName( "TriggeredEventCoordinator" )
        , m_CompletionEvent()
        , m_InputNumRepetitions(0)
    {
        initSimTypes( 1, "*" );
    }
    TriggeredEventCoordinator::~TriggeredEventCoordinator()
    {
        //not called
    }

    bool TriggeredEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Trigger_Condition_List", &m_StartTriggerConditionList, TEC_Start_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Stop_Trigger_Condition_List", &m_StopTriggerConditionList, TEC_Stop_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Completion_Event", &m_CompletionEvent, TEC_Completion_Event_DESC_TEXT );
        initConfigTypeMap( "Coordinator_Name", &m_CoordinatorName, Coordinator_Name_DESC_TEXT, "TriggeredEventCoordinator" );
        initConfigTypeMap( "Duration", &m_Duration, TEC_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f );

        bool retValue = StandardInterventionDistributionEventCoordinator::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            CheckConfigTriggers(inputJson);
            m_InputNumRepetitions = num_repetitions;
        }

        return retValue;
    }

    void TriggeredEventCoordinator::CheckConfigTriggers( const Configuration * inputJson )
    {
        if( m_CompletionEvent.IsUninitialized() )
        {
            LOG_WARN( "Completion_Event is not defined which is ok, but event will not be broadcasted.\n" );
        }
        if( m_StartTriggerConditionList.empty() )
        {
            std::stringstream ss;
            ss << "Start_Trigger_Condition_List of " << GetName() << " is empty, there is thus no way to start this TriggeredEventCoordinator.\n";
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void TriggeredEventCoordinator::Update( float dt )
    {        
        if( m_Duration != -1.0f )
        {
            if( m_Duration <= 0 )
            {
                m_DurationExpired = true;
            }
            else
            {
                m_Duration -= dt;
            }
        }
        if( m_IsStarting )
        {
            m_IsStarting = false;
            m_IsActive = true;
            num_repetitions = m_InputNumRepetitions;
            tsteps_since_last = tsteps_between_reps-1;
            distribution_complete = false;
        }
        if( m_IsStopping )
        {
            m_IsStopping = false;
            m_IsActive = false;
        }

        if ( !m_IsActive ) return;
        StandardInterventionDistributionEventCoordinator::Update( dt );
    }

    void TriggeredEventCoordinator::UpdateNodes( float dt )
    {       
        if( !m_IsActive ) return;

        //Finsish current repetition, then stop
        if( IsTimeToUpdate(dt) && m_IsStopping )
        {
            m_IsActive = false;
        }
        StandardInterventionDistributionEventCoordinator::UpdateNodes( dt );
    }

    void TriggeredEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        parent = isec;
        Register();
    }

    void TriggeredEventCoordinator::Register()
    {
        for ( EventTriggerCoordinator& ect : m_StartTriggerConditionList )
        {
            parent->GetCoordinatorEventBroadcaster()->RegisterObserver( this, ect );
            LOG_INFO_F( "%s: registered Start_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
        for ( EventTriggerCoordinator& ect : m_StopTriggerConditionList )
        {
            parent->GetCoordinatorEventBroadcaster()->RegisterObserver( this, ect );
            LOG_INFO_F( "%s: registered Stop_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
    }

    void TriggeredEventCoordinator::UpdateRepetitions()
    {
        StandardInterventionDistributionEventCoordinator::UpdateRepetitions();
        if ( num_repetitions == 0 )
        {
            BroadcastCompletionEvent();
        }        
    }

    bool TriggeredEventCoordinator::notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger )
    {
        LOG_DEBUG_F( "%s: notifyOnEvent received: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str());
        auto it_start = find( m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), trigger );
        if( it_start != m_StartTriggerConditionList.end() )
        {
            LOG_INFO_F( "%s: notifyOnEvent received start: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str() );
            m_IsStarting = true;
            m_IsStopping = false;
        }
        auto it_stop = find( m_StopTriggerConditionList.begin(), m_StopTriggerConditionList.end(), trigger );
        if( it_stop != m_StopTriggerConditionList.end() )
        {
            LOG_INFO_F( "%s: notifyOnEvent received stop: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str() );
            m_IsStarting = false;
            m_IsStopping = true;
        }
        return true;
    }

    void TriggeredEventCoordinator::Unregister()
    {
        for( EventTriggerCoordinator& ect : m_StartTriggerConditionList )
        {
            parent->GetCoordinatorEventBroadcaster()->UnregisterObserver( this, ect );
            LOG_INFO_F( "%s: Unregistered Start_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
        for( EventTriggerCoordinator& ect: m_StopTriggerConditionList )
        {
            parent->GetCoordinatorEventBroadcaster()->UnregisterObserver( this, ect );
            LOG_INFO_F( "%s: Unregistered Stop_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }       
    }
  
    void TriggeredEventCoordinator::BroadcastCompletionEvent()
    {
        parent->GetCoordinatorEventBroadcaster()->TriggerObservers( this, m_CompletionEvent );
    }

    const std::string& TriggeredEventCoordinator::GetName() const
    {
        return m_CoordinatorName;
    }

    const IdmDateTime& TriggeredEventCoordinator::GetTime() const
    {
        return parent->GetSimulationTime();
    }

    bool TriggeredEventCoordinator::IsFinished()
    {
        if( m_DurationExpired )
        {
            Unregister();
        }
        return m_DurationExpired;
    }
}
