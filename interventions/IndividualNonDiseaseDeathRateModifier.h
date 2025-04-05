
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "Timers.h"
#include "EventTrigger.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
    struct IDistribution;
    struct INonDiseaseDeathRateModifier;
    struct ICampaignCostObserver;

    class IndividualNonDiseaseDeathRateModifier : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, IndividualNonDiseaseDeathRateModifier, IDistributableIntervention)

    public:
        IndividualNonDiseaseDeathRateModifier();
        IndividualNonDiseaseDeathRateModifier( const IndividualNonDiseaseDeathRateModifier& );
        virtual ~IndividualNonDiseaseDeathRateModifier();

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

        InterpolatedValueMap m_DurationToModifier;
        float                m_DurationDays;
        IDistribution*       m_pExpirationDurationDistribution;
        CountdownTimer       m_ExpirationTimer;
        EventTrigger         m_ExpirationEvent;

        INonDiseaseDeathRateModifier * m_pINDDRM;

        DECLARE_SERIALIZABLE(IndividualNonDiseaseDeathRateModifier);
    };
}
