
#pragma once

#include "stdafx.h"
#include "NodeSTIEventContext.h"

#include "INodeContext.h"
#include "INodeSTI.h"
#include "ISociety.h"
#include "ISocietyOverrideIntervention.h"
#include "Log.h"

SETUP_LOGGING( "NodeSTIEventContext" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( NodeSTIEventContextHost, NodeEventContextHost )
        HANDLE_INTERFACE( INodeSTIInterventionEffectsApply )
    END_QUERY_INTERFACE_DERIVED( NodeSTIEventContextHost, NodeEventContextHost )

    NodeSTIEventContextHost::NodeSTIEventContextHost(Node* _node) 
        : NodeEventContextHost(_node)
        , m_OverrideRelationshipForamtionRate()
        , m_OverrideCoitalActRate()
        , m_OverrideCondomUsage()
        , m_OverrideDurationHeterogeniety()
        , m_OverrideDurationScale()
        , m_StagedNodeInterventionsToPurge()
        , m_StagedInterventionsToAdd()
        , m_DT(1.0)
    {
        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            m_OverrideRelationshipForamtionRate.push_back( -1.0 );
            m_OverrideCoitalActRate.push_back( -1.0 );
            m_OverrideCondomUsage.push_back( nullptr );
            m_OverrideDurationHeterogeniety.push_back( -1.0 );
            m_OverrideDurationScale.push_back( -1.0 );
        }
    }

    NodeSTIEventContextHost::~NodeSTIEventContextHost()
    {
    }

    void NodeSTIEventContextHost::UpdateInterventions(float dt)
    {
        // I hate doing this but there are cases where a staged society
        // intervention needs Update() called and I need the dt.
        m_DT = dt;

        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            m_OverrideRelationshipForamtionRate[ i ] = -1.0;
            m_OverrideCoitalActRate[ i ]             = -1.0;
            m_OverrideCondomUsage[ i ]               = nullptr;
            m_OverrideDurationHeterogeniety[ i ]     = -1.0;
            m_OverrideDurationScale[ i ]             = -1.0;
        }

        NodeEventContextHost::UpdateInterventions(dt);
    }
    
    bool NodeSTIEventContextHost::GiveIntervention( INodeDistributableIntervention* iv )
    {
        // -----------------------------------------------------------------
        // --- See notes in ApplyStagedInterventions() for more information
        // --- This allows us to delay the intervention being added, but it
        // --- does require the intervention to implement this interface.
        // -----------------------------------------------------------------
        bool success = false;
        ISocietyOverrideIntervention* p_override = nullptr;
        if( iv->QueryInterface( GET_IID( ISocietyOverrideIntervention ), (void**)&p_override ) == s_OK )
        {
            m_StagedInterventionsToAdd.push_back( iv );
            success = true;
        }
        else
        {
            success = NodeEventContextHost::GiveIntervention( iv );
        }
        return success;
    }

    void NodeSTIEventContextHost::StageExistingForPurgingIfQualifies( const std::string& type_name,
                                                                      node_intervention_qualify_function_t qual_func )
    {
        // -----------------------------------------------------------------
        // --- See notes in ApplyStagedInterventions() for more information
        // -----------------------------------------------------------------
        for (auto intervention : node_interventions)
        {
            std::string cur_iv_type_name = typeid( *intervention ).name();
            if( (cur_iv_type_name == type_name) && qual_func( intervention ) )
            {
                LOG_INFO_F("Found an existing intervention '%s' in node %d. Purging.\n", type_name.c_str(), GetId() );
                m_StagedNodeInterventionsToPurge.push_back( intervention );
                break;
            }
        }
    }

    void NodeSTIEventContextHost::SetOverrideRelationshipFormationRate( RelationshipType::Enum relType, float rate )
    {
        m_OverrideRelationshipForamtionRate[ relType ] = rate;
    }
    
    void NodeSTIEventContextHost::SetOverrideCoitalActRate( RelationshipType::Enum relType, float rate )
    {
        m_OverrideCoitalActRate[ relType ] = rate;
    }
    
    void NodeSTIEventContextHost::SetOverrideCondomUsageProbabiity( RelationshipType::Enum relType, const Sigmoid* pOverride )
    {
        m_OverrideCondomUsage[ relType ] = pOverride;
    }
    
    void NodeSTIEventContextHost::SetOverrideRelationshipDuration( RelationshipType::Enum relType,
                                                                   float heterogeniety,
                                                                   float scale )
    {
        m_OverrideDurationHeterogeniety[ relType ] = heterogeniety;
        m_OverrideDurationScale[ relType ]         = scale;
    }
    
    void NodeSTIEventContextHost::ApplyStagedInterventions()
    {
        // --------------------------------------------------------------------------------------
        // --- The node-level inteventions that are changing the parameters around relationships
        // --- need to be staged since where they get used is in different places within the
        // --- timestep.  For example, formation rate is something that is used before we update
        // --- the node (i.e. update interventions and people) while coital act rate is something
        // --- that can impact the relationships when people are updated.  By staging, they all
        // --- take affect the next time step.
        // --- One thing that tends to encourage this solution is that an event coordinator will
        // --- distribute the intervention before the node is updated (i.e. before new relationships
        // --- are considered) while something like NLHTIV distributes them while updating the
        // --- interversions or people (i.e. after new relationships are considered).  
        // --------------------------------------------------------------------------------------

        // --------------------------------------------------------------------------------------
        // --- We need to stage the purging of these interventions so that they get removed when
        // --- we can actually apply the new intervention.  If we don't do this, there will be a
        // --- a gap between when the intervention is removed and when the new one is applies.
        // --------------------------------------------------------------------------------------
        for( auto p_intervention : m_StagedNodeInterventionsToPurge )
        {
            node_interventions.remove( p_intervention );
            delete p_intervention;
        }
        m_StagedNodeInterventionsToPurge.clear();

        // ----------------------------------------------------------------------------------
        // --- We add the intervention and Update() it here so that it can take affect during
        // --- the next time step.  ApplyStagedInterventions() is assumed to be called at the
        // --- end of udpating the node - after the interventions, people and relationships
        // --- have been updated.
        // ----------------------------------------------------------------------------------
        for( auto p_intervention : m_StagedInterventionsToAdd )
        {
            NodeEventContextHost::GiveIntervention( p_intervention );
            p_intervention->Update( m_DT );
        }
        m_StagedInterventionsToAdd.clear();

        // -------------------------------------------------------------------------------
        // --- Now that the new interventions have updated any override parameters, we can
        // --- update the overrides within society so they can take affect during the
        // --- next time step.
        // -------------------------------------------------------------------------------
        INodeSTI* p_node_sti = nullptr;
        if( GetNodeContext()->QueryInterface(GET_IID(INodeSTI), (void**)&p_node_sti) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetNodeContext()", "INodeSTI", "INodeContext" );
        }
        ISociety* p_society = p_node_sti->GetSociety();

        for( int i = 0; i < RelationshipType::COUNT; i++ )
        {
            RelationshipType::Enum rel_type = RelationshipType::Enum(i);

            p_society->SetOverrideRelationshipFormationRate( rel_type, m_OverrideRelationshipForamtionRate[ i ] );
            p_society->SetOverrideCoitalActRate( rel_type, m_OverrideCoitalActRate[ i ] );
            p_society->SetOverrideCondomUsageProbability( rel_type, m_OverrideCondomUsage[ i ] );
            p_society->SetOverrideRelationshipDuration( rel_type,
                                                        m_OverrideDurationHeterogeniety[ i ],
                                                        m_OverrideDurationScale[ i ] );
        }
    }
}

