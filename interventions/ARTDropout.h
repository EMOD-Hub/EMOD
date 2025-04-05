
#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"    // macros that 'auto'-register classes

namespace Kernel
{
    struct IHIVDrugEffectsApply;

    class ARTDropout : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, ARTDropout, IDistributableIntervention);

    public:
        ARTDropout();
        virtual ~ARTDropout();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update( float dt ) override;

    protected:

        DECLARE_SERIALIZABLE(ARTDropout);
    };
}
