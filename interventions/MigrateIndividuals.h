
#pragma once

#include <string>

#include "Configuration.h"
#include "Configure.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "IDistribution.h"
#include "INodeContext.h"

namespace Kernel
{
    class IDMAPI MigrateIndividuals :  public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MigrateIndividuals, IDistributableIntervention)

    public: 
        MigrateIndividuals();
        MigrateIndividuals( const MigrateIndividuals& master );
        virtual ~MigrateIndividuals();
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

    protected:
        ExternalNodeId_t destination_external_node_id;
        IDistribution* duration_before_leaving;
        IDistribution* duration_at_node;
        bool is_moving;

        DECLARE_SERIALIZABLE(MigrateIndividuals);
    };
}
