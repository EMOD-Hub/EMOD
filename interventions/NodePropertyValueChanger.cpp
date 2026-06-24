
#include "stdafx.h"
#include "NodePropertyValueChanger.h"
#include "Debug.h" // for release_assert
#include "RANDOM.h"
#include "Common.h"             // for INFINITE_TIME
#include "DistributionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "INodeContext.h"
#include "EventTrigger.h"

SETUP_LOGGING( "NodePropertyValueChanger" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED( NodePropertyValueChanger )

    BEGIN_QUERY_INTERFACE_BODY( NodePropertyValueChanger )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA( INodeDistributableIntervention )
    END_QUERY_INTERFACE_BODY( NodePropertyValueChanger )

    NodePropertyValueChanger::NodePropertyValueChanger()
        : BaseNodeIntervention()
        , m_TargetKeyValue()
        , probability(1.0)
        , revert(0.0f)
        , max_duration(0.0f)
        , action_timer(0.0f)
        , reversion_timer(0.0f)
    {
    }

    NodePropertyValueChanger::NodePropertyValueChanger( const NodePropertyValueChanger& rThat )
        : BaseNodeIntervention( rThat )
        , m_TargetKeyValue( rThat.m_TargetKeyValue )
        , probability( rThat.probability )
        , revert( rThat.revert )
        , max_duration( rThat.max_duration )
        , action_timer( 0.0f )
        , reversion_timer( rThat.reversion_timer )
    {
    }

    NodePropertyValueChanger::~NodePropertyValueChanger()
    {
    }

    bool NodePropertyValueChanger::Configure( const Configuration * inputJson )
    {
        NPKeyValueParameter np_key_value;
        initConfigTypeMap("Target_NP_Key_Value", &np_key_value, NPC_Target_NP_Key_Value_DESC_TEXT );
        initConfigTypeMap("Daily_Probability",   &probability,  NPC_Daily_Probability_DESC_TEXT, 0.0f, 1.0f );
        initConfigTypeMap("Maximum_Duration",    &max_duration, NPC_Maximum_Duration_DESC_TEXT, -1.0f, FLT_MAX, FLT_MAX);
        initConfigTypeMap("Revert",              &revert,       NPC_Revert_DESC_TEXT,            0.0f, FLT_MAX, 0.0f );

        bool ret = BaseNodeIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_TargetKeyValue = np_key_value;
            if( !m_TargetKeyValue.IsValid() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must specify 'Target_NP_Key_Value'" );
            }
        }
        return ret;
    }

    bool NodePropertyValueChanger::Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC )
    {
        if( probability < 1.0 )
        {
            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( DistributionFunction::EXPONENTIAL_DISTRIBUTION ) );
            distribution->SetParameters( (double)probability, (double)0.0, (double)0.0 );
            action_timer = distribution->Calculate( pNodeEventContext->GetRng() );

            if( action_timer > max_duration )
            {
                action_timer = FLT_MAX;
            }
            LOG_DEBUG_F( "Time until property change occurs = %f\n", action_timer );
        }
        return BaseNodeIntervention::Distribute( pNodeEventContext, pEC );
    }

    void NodePropertyValueChanger::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        INodeContext* p_node_context = parent->GetNodeContext();

        NPKeyValue current_prop_value;
        if( reversion_timer > 0 )
        {
            reversion_timer -= dt;
            if( reversion_timer <= 0 )
            {
                probability = 1.0;
            }
        }

        if( probability == 1.0 || action_timer < 0 )
        {
            if( revert )
            {
                current_prop_value = p_node_context->GetNodeProperties().Get( m_TargetKeyValue.GetKey<NPKey>() );
            }
            p_node_context->GetNodeProperties().Set( m_TargetKeyValue );

            //broadcast that the individual changed properties
            IIndividualEventBroadcaster* broadcaster = parent->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( nullptr, EventTrigger::NodePropertyChange );

            if( revert )
            {
                m_TargetKeyValue = current_prop_value;
                probability = 0.0; // keep it simple for now, reversion is guaranteed
                reversion_timer = revert;
                action_timer= FLT_MAX;
                LOG_DEBUG_F( "Initializing reversion timer to %f\n", reversion_timer );
                revert = 0; // no more reversion from reversion
            }
            else
            {
                expired = true;
            }
        }
        action_timer -= dt;
    }

    int NodePropertyValueChanger::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }

    int NodePropertyValueChanger::Release()
    {
        return BaseNodeIntervention::Release();
    }
}
