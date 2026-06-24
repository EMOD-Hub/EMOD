
#include "stdafx.h"
#include "MigrateFamily.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"
#include "INodeContext.h"
#include "ISimulationContext.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "MigrateFamily" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MigrateFamily)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MigrateFamily)

    IMPLEMENT_FACTORY_REGISTERED(MigrateFamily)

    bool MigrateFamily::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun && 
            //(EnvPtr->getSimulationConfig() != nullptr) &&
            (GET_CONFIGURABLE(SimulationConfig)->migration_structure == MigrationStructure::NO_MIGRATION) )
        {
            std::stringstream msg;
            msg << _module << " cannot be used when 'Migration_Model' = 'NO_MIGRATION'.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        initConfigTypeMap( "NodeID_To_Migrate_To", &destination_external_node_id,NodeID_To_Migrate_To_DESC_TEXT, 0, UINT_MAX, 0 );
        initConfigTypeMap( "Is_Moving", &is_moving, MF_Is_Moving_DESC_TEXT, false );

        DistributionFunction::Enum before_leaving_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Duration_Before_Leaving_Distribution", before_leaving_function, inputJson, MetadataDescriptor::Enum( "Duration_Before_Leaving_Distribution", MF_Duration_Before_Leaving_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );
        duration_before_leaving = DistributionFactory::CreateDistribution( this, before_leaving_function, "Duration_Before_Leaving", inputJson );

        DistributionFunction::Enum at_node_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Duration_At_Node_Distribution", at_node_function, inputJson, MetadataDescriptor::Enum( "Duration_At_Node_Distribution", Duration_At_Node_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );
        duration_at_node = DistributionFactory::CreateDistribution( this, at_node_function, "Duration_At_Node", inputJson );

        return BaseNodeIntervention::Configure( inputJson );
    }

    MigrateFamily::MigrateFamily()
        : BaseNodeIntervention()
        , destination_external_node_id( 0 )
        , duration_before_leaving()
        , duration_at_node()
        , is_moving( false )
    {
    }

    MigrateFamily::MigrateFamily( const MigrateFamily& master )
        : BaseNodeIntervention( master )
        , destination_external_node_id( master.destination_external_node_id )
        , duration_before_leaving( master.duration_before_leaving->Clone() )
        , duration_at_node( master.duration_at_node->Clone() )
        , is_moving( master.is_moving )
    {
    }

    MigrateFamily::~MigrateFamily()
    {
        delete duration_before_leaving;
        delete duration_at_node;
    }

    bool MigrateFamily::Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC )
    {
        ISimulationContext* p_sim = pNodeEventContext->GetNodeContext()->GetParent();
        if( !p_sim->CanSupportFamilyTrips() )
        {
            std::stringstream msg;
            msg << "Invalid Configuration for Family Trips.  MigrateFamily cannot be used." << std::endl;
            msg << "Migration_Pattern must be SINGLE_ROUND_TRIPS and the 'XXX_Migration_Roundtrip_Probability' must equal 1.0 if that Migration Type is enabled." << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        return BaseNodeIntervention::Distribute( pNodeEventContext, pEC );
    }

    void MigrateFamily::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        INodeContext* p_node_context = parent->GetNodeContext();

        if( !Expired() )
        {
            // expire the intervention
            SetExpired( true );

            // Don't have the family migrate to the node they are in.
            if( p_node_context->GetExternalID() != destination_external_node_id )
            {
                suids::suid destination_id = p_node_context->GetParent()->GetNodeSuid( destination_external_node_id );

                float duration_before = duration_before_leaving->Calculate( parent->GetRng() );
                float duration_at     = duration_at_node->Calculate( parent->GetRng() );

                p_node_context->SetWaitingForFamilyTrip( destination_id, MigrationType::INTERVENTION_MIGRATION, duration_before, duration_at, is_moving );
            }
        }
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! TODO - can't do until BaseNodeIntervention is serializable
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //REGISTER_SERIALIZABLE(MigrateFamily);

    //void MigrateFamily::serialize(IArchive& ar, MigrateFamily* obj)
    //{
    //    BaseNodeIntervention::serialize( ar, obj );
    //    MigrateFamily& mf = *obj;
    //    ar.labelElement("destination_external_node_id") & mf.destination_external_node_id;
    //    ar.labelElement("duration_before_leaving"     ) & mf.duration_before_leaving;
    //    ar.labelElement("duration_at_node"            ) & mf.duration_at_node;
    //    ar.labelElement("is_moving"                   ) & mf.is_moving;
    //}
}
