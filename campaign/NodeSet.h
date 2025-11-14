
#pragma once

#include <string>

#include "IdmApi.h"
#include "Configuration.h"
#include "ExternalNodeId.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "ISerializable.h"
#include "ObjectFactory.h"

namespace Kernel
{
    struct INodeEventContext;

    struct IDMAPI INodeSet : public ISerializable
    {
        virtual bool Contains(INodeEventContext *ndc) = 0; // must provide access to demographics id, lat, long, etc
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids) = 0;
    };

    class NodeSetFactory : public ObjectFactory<INodeSet,NodeSetFactory>
    {
    };

    // class defines a simple set of nodes...either by id, 
    class IDMAPI NodeSetAll : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(NodeSetFactory, NodeSetAll, INodeSet)

    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual bool Configure(const Configuration* config);

        virtual bool Contains(INodeEventContext *ndc);
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids);

    protected:
        DECLARE_SERIALIZABLE(NodeSetAll);
    };

    class IDMAPI NodeSetNodeList : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(NodeSetFactory, NodeSetNodeList, INodeSet)

    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual bool Configure(const Configuration* config);

        virtual bool Contains(INodeEventContext *ndc);
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids);

    protected:
        std::vector<ExternalNodeId_t> nodelist;
    };
};
