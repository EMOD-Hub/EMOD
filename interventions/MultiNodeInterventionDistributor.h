
#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class MultiNodeInterventionDistributor : public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED( NodeIVFactory, MultiNodeInterventionDistributor, INodeDistributableIntervention )

    public:
        MultiNodeInterventionDistributor();
        MultiNodeInterventionDistributor( const MultiNodeInterventionDistributor& rMaster );
        virtual ~MultiNodeInterventionDistributor();

        virtual bool Configure( const Configuration* config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void Update( float dt ) override;
        virtual bool Distribute( INodeEventContext *context, IEventCoordinator2* pEC = nullptr ) override;

    protected:
        std::vector<INodeDistributableIntervention*> m_Interventions;
    };
}
