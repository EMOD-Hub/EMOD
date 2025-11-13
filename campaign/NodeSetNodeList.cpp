
#include "stdafx.h"
#include "NodeSet.h"
#include "NodeEventContext.h"
#include "Exceptions.h"

SETUP_LOGGING( "NodeSetNodeList" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(NodeSetNodeList)
    IMPL_QUERY_INTERFACE2(NodeSetNodeList, INodeSet, IConfigurable)

    void NodeListConfig::ConfigureFromJsonAndKey(const Configuration* inputJson, const std::string& key)
    {
        // vertices should be valid json array
        if( nodelist.size() == 0 )
        {
            // haven't parsed raw list yet.
            // Go through list and parse out.
            json::QuickInterpreter nodelist_qi( (*inputJson)[key] );
            try {
                json::QuickInterpreter nodeListJson( nodelist_qi.As<json::Array>() );

                for( int idx=0; idx<nodelist_qi.As<json::Array>().Size(); idx++ )
                {
                    try {
                        auto nodeId = ExternalNodeId_t(nodeListJson[idx].As<json::Number>());
                        nodelist.push_back( nodeId );
                    }
                    catch( json::Exception)
                    {
                        throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, std::to_string( idx ).c_str(), nodeListJson, "Expected NUMBER" );
                    }
                }
            }
            catch( json::Exception )
            {
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), nodelist_qi, "Expected ARRAY" );
            }
        }
    }

    json::QuickBuilder NodeListConfig::GetSchema()
    {
        json::QuickBuilder schema = json::QuickBuilder( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:NodeListConfig" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["type"] = json::String( "integer" );
        schema[ ts ][0]["min"] = json::Number( 0 );
        schema[ ts ][0]["description"] = json::String( "Id of Node" );
        return schema;
    }

    bool NodeSetNodeList::Configure(const Configuration * inputJson)
    {
        initConfigComplexType( "Node_List", &nodelist_config, Node_List_DESC_TEXT );
        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            std::vector<ExternalNodeId_t> nodes( nodelist_config.nodelist.begin(), nodelist_config.nodelist.end() );

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

    //
    // NodeSetNodeList class methods
    //
    bool NodeSetNodeList::Contains(INodeEventContext *nec)
    {
        LOG_DEBUG_F("node id = %d\n", nec->GetId().data);

        // We have external ids, but Node never exposes this id because it will get abused, so go through encapsulating method.
        return nec->IsInExternalIdSet( nodelist_config.nodelist );
    }

    //
    // Return a list of node ids which are present in nodelist_config but not in nodeIds.
    // 
    std::vector<ExternalNodeId_t> NodeSetNodeList::IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        //sort both vectors, then determine difference
        std::vector<ExternalNodeId_t> nodeIDs_sorted(demographic_node_ids), difference;
        std::sort(nodeIDs_sorted.begin(), nodeIDs_sorted.end());

        std::vector<ExternalNodeId_t> nodes_campaign_sorted(nodelist_config.nodelist.begin(), nodelist_config.nodelist.end());
        std::sort(nodes_campaign_sorted.begin(), nodes_campaign_sorted.end());

        std::set_difference(nodes_campaign_sorted.begin(), nodes_campaign_sorted.end(), nodeIDs_sorted.begin(), nodeIDs_sorted.end(), std::inserter(difference, difference.begin()));

        return difference;
    }
}
