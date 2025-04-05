
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"

namespace Kernel
{
    class StrainIdentity;

    struct IOutbreak : public ISupports
    {
        virtual int GetAntigen() const = 0;
        virtual int GetGenome() const = 0;
        virtual float GetImportAge() const = 0;
        virtual ~IOutbreak() { }; // needed for cleanup via interface pointer
    };

    class Outbreak : public IOutbreak, public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(Outbreak)
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, Outbreak, INodeDistributableIntervention)

    public:
        Outbreak();
        virtual ~Outbreak() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC);
        virtual void SetContextTo(INodeEventContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

        // IOutbreak
        virtual int GetAntigen() const  { return antigen; }
        virtual int GetGenome() const  { return genome; }
        virtual float GetImportAge() const { return import_age; }

    protected:
        virtual void ExtraConfiguration();

        int antigen;
        int genome;
        float import_age;
        int num_cases_per_node;
        ProbabilityNumber prob_infection;

        IStrainIdentity* GetNewStrainIdentity( INodeEventContext *context );
    };
}
