
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "EventTrigger.h"
#include "Timers.h"

namespace Kernel
{
    struct IDistribution;
    struct ICoitalActRiskData;

    class CoitalActRiskFactors : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, CoitalActRiskFactors, IDistributableIntervention )

    public:
        CoitalActRiskFactors();
        CoitalActRiskFactors( const CoitalActRiskFactors& rMaster );
        virtual ~CoitalActRiskFactors();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual void Update( float dt ) override;

    protected:
        void TimerCallback( float dt );

        float m_AcquisitionMultiplier;
        float m_TransmissionMultiplier;
        EventTrigger m_ExpirationTrigger;
        IDistribution* m_pExpirationDuration;
        CountdownTimer m_ExpirationTimer;
        bool m_TimerHasExpired;
        ICoitalActRiskData* m_pRiskData;

        DECLARE_SERIALIZABLE( CoitalActRiskFactors );
    };
}
