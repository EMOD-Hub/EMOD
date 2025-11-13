
#include "stdafx.h"
#include "NodeSet.h"

SETUP_LOGGING( "NodeSetAll" )

namespace Kernel
{
    // NodeSetAll 
    IMPLEMENT_FACTORY_REGISTERED(NodeSetAll)
    IMPL_QUERY_INTERFACE2(NodeSetAll, INodeSet, IConfigurable)

    bool NodeSetAll::Configure(const Configuration * pInputJson)
    {
        return true; // nothing to configure
    }

    bool NodeSetAll::Contains(INodeEventContext *ndc)
    {
        return true;
    }

    std::vector<ExternalNodeId_t> NodeSetAll::IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        std::vector<ExternalNodeId_t> dummy;
        return dummy;
    }

    REGISTER_SERIALIZABLE(NodeSetAll);

    void NodeSetAll::serialize(IArchive& ar, NodeSetAll* obj)
    {
        // Nothing to do here. NodeSetAll does its work in Contains()
    }
}
