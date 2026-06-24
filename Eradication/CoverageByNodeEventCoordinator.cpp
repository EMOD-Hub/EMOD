
#include "stdafx.h"
#include "CoverageByNodeEventCoordinator.h"

#include "NodeEventContext.h"
#include "IIndividualHumanContext.h"
#include "RANDOM.h"
#include "JsonConfigurableCollection.h"

SETUP_LOGGING( "CoverageByNodeEventCoordinator" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- NodeIdAndCoverage
    // ------------------------------------------------------------------------

    class NodeIdAndCoverage : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

    public:
        NodeIdAndCoverage()
            : JsonConfigurable()
            , m_NodeId( 0 )
            , m_Coverage( 0.0 )
        {
        }

        NodeIdAndCoverage( const NodeIdAndCoverage& rMaster )
            : JsonConfigurable( rMaster )
            , m_NodeId( rMaster.m_NodeId )
            , m_Coverage( rMaster.m_Coverage )
        {
        }

        virtual ~NodeIdAndCoverage()
        {
        }

        virtual bool Configure( const Configuration* inputJson ) override
        {
            initConfigTypeMap("Node_Id", &m_NodeId, CBNEC_NodeId_DESC_TEXT, 0, 999999, 0 );
            initConfigTypeMap("Coverage", &m_Coverage, CBNEC_Coverage_DESC_TEXT, 0.0, 1.0, 0.0 );

            bool configured = JsonConfigurable::Configure( inputJson );
            return configured;
        }

        uint32_t m_NodeId;
        float m_Coverage;
    };

    // ------------------------------------------------------------------------
    // --- NodeIdAndCoverageList
    // ------------------------------------------------------------------------

    class NodeIdAndCoverageList : public JsonConfigurableCollection<NodeIdAndCoverage>
    {
    public:
        NodeIdAndCoverageList()
            : JsonConfigurableCollection("NodeIdAndCoverageList")
        {
        }

        NodeIdAndCoverageList( const NodeIdAndCoverageList& rMaster )
            : JsonConfigurableCollection( rMaster )
        {
        }

        virtual ~NodeIdAndCoverageList()
        {
        }

    protected:
        virtual NodeIdAndCoverage* CreateObject() override
        {
            return new NodeIdAndCoverage();
        }
    };

    // ------------------------------------------------------------------------
    // --- CoverageByNodeEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(CoverageByNodeEventCoordinator)
    BEGIN_QUERY_INTERFACE_BODY(CoverageByNodeEventCoordinator)
    END_QUERY_INTERFACE_BODY(CoverageByNodeEventCoordinator)

    CoverageByNodeEventCoordinator::CoverageByNodeEventCoordinator()
        : StandardInterventionDistributionEventCoordinator( false )//false=don't use standard demographic coverage
    {
    }

    bool CoverageByNodeEventCoordinator::Configure( const Configuration * inputJson )
    {
        NodeIdAndCoverageList coverage_by_node;
        initConfigComplexCollectionType("Coverage_By_Node", &coverage_by_node, CBNEC_Coverage_By_Node_DESC_TEXT );

        bool configured = StandardInterventionDistributionEventCoordinator::Configure(inputJson);

        if( configured && !JsonConfigurable::_dryrun )
        {
            for( int i = 0; i < coverage_by_node.Size(); ++i )
            {
                uint32_t node_id = coverage_by_node[ i ]->m_NodeId;
                float coverage = coverage_by_node[ i ]->m_Coverage;
                auto ret = node_coverage_map.insert( std::make_pair(node_id,coverage) );
                if (ret.second == false)
                {
                    LOG_WARN_F("Duplicate coverage specified for node with ID=%d. Using first coverage value specified.\n", node_id);
                }
            }
        }
        return configured;
    }

    bool CoverageByNodeEventCoordinator::TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec)
    {
        bool covered=false;

        ExternalNodeId_t nodeid = ihec->GetNodeEventContext()->GetExternalId();

        auto coverage_it = node_coverage_map.find(nodeid);
        
        if (coverage_it != node_coverage_map.end())
        {
            float coverage = coverage_it->second;
            covered = ihec->GetInterventionsContext()->GetParent()->GetRng()->SmartDraw( coverage );
        }
        else
        {
            LOG_WARN_F("No coverage specified for Node with ID = %d.  Defaulting to zero coverage.\n", nodeid);
        }

        return covered;
    }
}
