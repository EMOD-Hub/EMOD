
#pragma once

#include "NodeVectorEventContext.h"
#include "VectorDefs.h"

namespace Kernel
{
    class ISporozoiteChallengeConsumer : public ISupports
    {
        public:
            virtual void ChallengeWithSporozoites(int n_sporozoites, float coverage=1.0f) = 0;
            virtual void ChallengeWithInfectiousBites(int n_bites, float coverage=1.0f) = 0;
    };

    class NodeMalariaEventContextHost : public NodeVectorEventContextHost,
                                        public ISporozoiteChallengeConsumer
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        NodeMalariaEventContextHost(Node* _node);
        virtual ~NodeMalariaEventContextHost();

        // ISporozoiteChallengeConsumer
        virtual void ChallengeWithSporozoites(int n_sporozoites, float coverage);
        virtual void ChallengeWithInfectiousBites(int n_bites, float coverage);
    };
}