
#pragma once

#include "StandardEventCoordinator.h"

namespace Kernel
{
    class CalendarEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED(EventCoordinatorFactory, CalendarEventCoordinator, IEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        CalendarEventCoordinator();

        virtual bool Configure(const Configuration* config) override;
        virtual void UpdateNodes(float dt) override;
        virtual bool TargetedIndividualIsCovered( IIndividualHumanEventContext *ihec ) override;

    protected:
        void BuildDistributionCalendar( std::vector<int>, std::vector<float> );
        virtual void InitializeRepetitions( const Configuration* inputJson ) override;
        virtual bool IsTimeToUpdate( float dt ) override;
        virtual void UpdateRepetitions() override;

        std::map< NaturalNumber, Fraction > times_and_coverages;
    };
}
