
#pragma once

#include "Bednet.h"
#include "Timers.h"
#include "IDistribution.h"
#include "WaningEffectCombo.h"

namespace Kernel
{
    struct IBednetConsumer;

    class UsageDependentBednet : public AbstractBednet
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, UsageDependentBednet, IDistributableIntervention)

    public:
        UsageDependentBednet();
        UsageDependentBednet( const UsageDependentBednet& );
        virtual ~UsageDependentBednet();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetExpired( bool isExpired ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;

    protected:
        virtual bool ConfigureUsage(  const Configuration* config ) override; // should call JsonConfigurable::Configure()
        virtual bool ConfigureEvents( const Configuration* config ) override; // should call JsonConfigurable::Configure()

        virtual void UpdateUsage( float dt ) override;
        virtual bool IsUsingBednet() const override;
        virtual bool CheckExpiration( float dt ) override;
        virtual float GetEffectUsage() const override;

        virtual void Callback( float dt );

        WaningEffectCollection m_EffectCollection;
        EventTrigger m_TriggerReceived;
        EventTrigger m_TriggerUsing;
        EventTrigger m_TriggerDiscard;
        IDistribution* m_ExpirationDuration;
        CountdownTimer m_ExpirationTimer;
        bool m_TimerHasExpired;

        DECLARE_SERIALIZABLE( UsageDependentBednet );
    };

    class MultiInsecticideUsageDependentBednet : public UsageDependentBednet
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MultiInsecticideUsageDependentBednet, IDistributableIntervention)
    public:
        MultiInsecticideUsageDependentBednet();
        MultiInsecticideUsageDependentBednet( const MultiInsecticideUsageDependentBednet& rMaster );
        virtual ~MultiInsecticideUsageDependentBednet();

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        virtual bool ConfigureBlockingAndKilling( const Configuration* config ) override;

        DECLARE_SERIALIZABLE( MultiInsecticideUsageDependentBednet );
    };
}
