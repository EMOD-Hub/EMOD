
#pragma once

#include "Configuration.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "EventTrigger.h"
#include "INodeInfo.h"
#include "EnumSupport.h"

namespace Kernel
{
    ENUM_DEFINE(NodeSelectionType,                   // Select the nodes to send the event to based on:
        ENUM_VALUE_SPEC(DISTANCE_ONLY, 0)            // - the distance between the nodes - uses latitude and longitude of the nodes
        ENUM_VALUE_SPEC(MIGRATION_NODES_ONLY, 1)     // - whether the node of interest is reachable via LOCAL or REGIONAL human migration (i.e. in the files)
        ENUM_VALUE_SPEC(DISTANCE_AND_MIGRATION, 2))  // - the node must be reachable and within the given distance

    // This intervention is used to broadcast an event to all of the individuals in other nodes.
    // For example, if an individual in the current node is detected to have malaria, one could use
    // this intervention to send the event to the surrounding nodes so that the people in those
    // nodes could be given anti-malaria drugs.
    class IDMAPI BroadcastEventToOtherNodes :  public BaseIntervention, public INodeQualifier
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, BroadcastEventToOtherNodes, IDistributableIntervention)

    public: 
        BroadcastEventToOtherNodes();
        BroadcastEventToOtherNodes( const BroadcastEventToOtherNodes& master );
        virtual ~BroadcastEventToOtherNodes() {  }

        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        // INodeQualifier
        virtual bool Qualifies( const INodeInfo& rni ) const  override;

    protected:

        INodeContext* p_node_context ;
        EventTrigger event_trigger;
        bool include_my_node ;
        NodeSelectionType::Enum node_selection_type ;
        float max_distance_km ;

        DECLARE_SERIALIZABLE(BroadcastEventToOtherNodes);
    };
}
