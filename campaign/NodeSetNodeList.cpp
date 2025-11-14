
#include "stdafx.h"
#include "NodeSet.h"
#include "NodeEventContext.h"
#include "Exceptions.h"

SETUP_LOGGING( "NodeSetNodeList" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(NodeSetNodeList)
    IMPL_QUERY_INTERFACE2(NodeSetNodeList, INodeSet, IConfigurable)

    bool NodeSetNodeList::Configure(const Configuration* inputJson)
    {
        initConfigTypeMap("Node_List", &nodelist, Node_List_DESC_TEXT);
        bool configured = JsonConfigurable::Configure(inputJson);

        if(configured && !JsonConfigurable::_dryrun)
        {
            std::vector<ExternalNodeId_t> nodes(nodelist.begin(), nodelist.end());
            std::set<ExternalNodeId_t> duplicates;

            for( int i = 0 ; i < nodes.size(); ++i )
            {
                for( int j = i + 1; j < nodes.size(); ++j )
                {
                    if( nodes[i] == nodes[j] )
                    {
                        duplicates.insert( nodes[i] );
                    }
                }
            }
            if( duplicates.size() > 0 )
            {
                std::stringstream ss;
                ss << "The NodeSetNodeList has the duplicate entries for the following nodes:\n";
                for( auto dup : duplicates )
                {
                    ss << dup << ", ";
                }
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return configured;
    }

    bool NodeSetNodeList::Contains(INodeEventContext *nec)
    {
        LOG_DEBUG_F("node id = %d\n", nec->GetId().data);

        // We have external ids, but Node never exposes this id because it will get abused, so go through encapsulating method.
        return nec->IsInExternalIdSet(nodelist);
    }

    std::vector<ExternalNodeId_t> NodeSetNodeList::IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        //sort both vectors, then determine difference
        std::vector<ExternalNodeId_t> nodeIDs_sorted(demographic_node_ids), difference;
        std::sort(nodeIDs_sorted.begin(), nodeIDs_sorted.end());

        std::vector<ExternalNodeId_t> nodes_campaign_sorted(nodelist.begin(), nodelist.end());
        std::sort(nodes_campaign_sorted.begin(), nodes_campaign_sorted.end());

        std::set_difference(nodes_campaign_sorted.begin(), nodes_campaign_sorted.end(), nodeIDs_sorted.begin(), nodeIDs_sorted.end(), std::inserter(difference, difference.begin()));

        return difference;
    }
}
