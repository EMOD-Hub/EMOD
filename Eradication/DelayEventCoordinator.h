
#pragma once
#include "TriggeredEventCoordinator.h"
#include "IDistribution.h"
#include "Timers.h"

namespace Kernel
{
    class DelayEventCoordinator : public TriggeredEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED( EventCoordinatorFactory, DelayEventCoordinator, IEventCoordinator )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        DelayEventCoordinator();
        ~DelayEventCoordinator();

        virtual bool Configure(const Configuration* config) override;
        void Callback( float dt );
        void UpdateNodes( float dt ) override;
        virtual void Update( float dt ) override;
        virtual bool notifyOnEvent( IEventCoordinatorEventContext * pEntity, const EventTriggerCoordinator & trigger ) override;

    private:
        CountdownTimer remaining_delay_days;
        IDistribution* delay_distribution;
    };
}
