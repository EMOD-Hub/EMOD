
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "EventCoordinator.h"
#include "Configure.h"

namespace Kernel
{
    ENUM_DEFINE(MalariaChallengeType,
        ENUM_VALUE_SPEC(InfectiousBites         , 1)
        ENUM_VALUE_SPEC(Sporozoites             , 2))

    class MalariaChallenge : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(Outbreak)
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, MalariaChallenge, INodeDistributableIntervention)

    public:
        MalariaChallenge();
        MalariaChallenge( const MalariaChallenge& master );
        virtual ~MalariaChallenge() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC);
        virtual void SetContextTo(INodeEventContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

    protected:
        MalariaChallengeType::Enum challenge_type;
        int infectious_bite_count;
        int sporozoite_count;
        float coverage;
    };
}
