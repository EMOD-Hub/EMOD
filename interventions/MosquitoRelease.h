
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
#include "VectorEnums.h"
#include "VectorGenome.h"

namespace Kernel
{
    ENUM_DEFINE( MosquitoReleaseType, 
        ENUM_VALUE_SPEC( FIXED_NUMBER , 0 )
        ENUM_VALUE_SPEC( RATIO        , 1 ))

    class MosquitoRelease : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(MosquitoRelease)
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, MosquitoRelease, INodeDistributableIntervention)

    public:
        MosquitoRelease();
        MosquitoRelease( const MosquitoRelease& master );
        virtual ~MosquitoRelease() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC);
        virtual void SetContextTo(INodeEventContext *context) { } // not needed for this intervention
        virtual void Update(float dt);

    protected:
        jsonConfigurable::ConstrainedString m_ReleasedSpecies;
        uint32_t     m_TotalToRelease;
        VectorGenome m_Genome;
        VectorGenome m_MateGenome;
        bool         m_IsRatio;
        float        m_RatioToRelease;
        float        m_FractionToInfect;
    };
}
