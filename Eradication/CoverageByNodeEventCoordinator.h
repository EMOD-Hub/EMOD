
#pragma once

#include "StandardEventCoordinator.h"

namespace Kernel
{
    class CoverageByNodeEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED(EventCoordinatorFactory, CoverageByNodeEventCoordinator, IEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        CoverageByNodeEventCoordinator();
        virtual bool Configure( const Configuration * inputJson ) override;

    protected:
        virtual bool TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec);

        std::map<uint32_t, float> node_coverage_map;
    };
}
