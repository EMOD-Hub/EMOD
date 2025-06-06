
#pragma once

#include "Types.h"
#include "InterventionFactory.h"
#include "Interventions.h"

namespace Kernel
{
    class IHIVMTCTEffects;

    class IDMAPI PMTCT : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, PMTCT, IDistributableIntervention)
        DECLARE_QUERY_INTERFACE()

    public: 
        PMTCT();
        PMTCT( const PMTCT& );
        ~PMTCT();
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver* const pEC) override;

protected:
        IHIVMTCTEffects * ivc; // interventions container
        NonNegativeFloat timer;
        float efficacy;

        DECLARE_SERIALIZABLE(PMTCT);
    };
}
