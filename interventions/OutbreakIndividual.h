
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"

namespace Kernel
{
    struct IStrainIdentity;
    struct IIndividualHumanContext;

    struct IOutbreakIndividual : public ISupports
    {
        virtual int GetAntigen() const = 0;
        virtual int GetGenome() const = 0;
        virtual ~IOutbreakIndividual() { }; // needed for cleanup via interface pointer
    };

    class OutbreakIndividual : public IOutbreakIndividual, public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(OutbreakIndividual)
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, OutbreakIndividual, IDistributableIntervention)

    public:
        OutbreakIndividual();
        virtual ~OutbreakIndividual() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual void SetContextTo(IIndividualHumanContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

        // IOutbreakIndividual
        virtual int GetAntigen() const  { return antigen; }
        virtual int GetGenome() const  { return genome; }

        // other methods
        virtual void ConfigureAntigen( const Configuration * inputJson );
        virtual void ConfigureGenome( const Configuration * inputJson );

    protected:
        virtual IStrainIdentity* GetNewStrainIdentity( INodeEventContext *context, IIndividualHumanContext* pIndiv );

        int antigen;
        int genome;
        bool ignoreImmunity;
        int incubation_period_override;
    };
}
