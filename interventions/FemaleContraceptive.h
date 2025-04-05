
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"
#include "Timers.h"
#include "EventTrigger.h"

namespace Kernel
{
    struct IDistribution;
    struct IBirthRateModifier;
    struct ICampaignCostObserver;

    class FemaleContraceptive : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, FemaleContraceptive, IDistributableIntervention)

    public:
        FemaleContraceptive();
        FemaleContraceptive( const FemaleContraceptive& );
        virtual ~FemaleContraceptive();

        virtual bool Configure( const Configuration* pConfig ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual int AddRef() override { return BaseIntervention::AddRef(); }
        virtual int Release() override { return BaseIntervention::Release(); }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        virtual void Callback( float dt );

    protected:
        virtual void BroadcastEvent( const EventTrigger& rTrigger );

        IWaningEffect* m_pWaningEffect;
        IDistribution* m_pUsageDurationDistribution;
        CountdownTimer m_UsageTimer;
        EventTrigger   m_UsageExpirationEvent;

        IBirthRateModifier * m_pIBRM;

        DECLARE_SERIALIZABLE(FemaleContraceptive);
    };
}
