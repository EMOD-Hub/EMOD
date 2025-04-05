
#pragma once

#include "AntiretroviralTherapy.h"
#include "IDistribution.h"
#include "EventTrigger.h"
#include "Timers.h"

namespace Kernel
{
    struct IIndividualHumanHIV;
    struct IHIVDrugEffectsApply;

    class AntiretroviralTherapyFull : public AntiretroviralTherapy
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, AntiretroviralTherapyFull, IDistributableIntervention);

    public:
        AntiretroviralTherapyFull();
        AntiretroviralTherapyFull( const AntiretroviralTherapyFull& rMaster );
        virtual ~AntiretroviralTherapyFull();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, 
                                 ICampaignCostObserver * const pCCO ) override;
        virtual void Update( float dt ) override;

    protected:
        virtual void CalculateDelay();
        virtual void DeterminePointers( IIndividualHumanInterventionsContext *context );
        virtual bool CanDistribute();
        virtual void Callback( float dt );

        IIndividualHumanHIV* m_pHumanHIV;
        IHIVDrugEffectsApply* m_pDrugEffects;
        IDistribution* m_pDistributionTimeOn;
        CountdownTimer m_RemainingDays;
        EventTrigger   m_StopArtEvent;

        DECLARE_SERIALIZABLE( AntiretroviralTherapyFull );
    };
}
