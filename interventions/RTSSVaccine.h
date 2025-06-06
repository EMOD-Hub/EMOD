
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"

#include "MalariaContexts.h"  // for MalariaAntibodyType enum

namespace Kernel
{
    class RTSSVaccine : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, RTSSVaccine, IDistributableIntervention)

    public:
        RTSSVaccine();
        virtual ~RTSSVaccine() { }

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

    protected:
        MalariaAntibodyType::Enum antibody_type;
        int antibody_variant;
        float boosted_antibody_concentration;

        DECLARE_SERIALIZABLE(RTSSVaccine);
    };
}
