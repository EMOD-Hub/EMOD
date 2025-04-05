
#pragma once

#include <map>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "Configure.h"
#include "IDistribution.h"

namespace Kernel
{
    struct IBitingRisk;

    class BitingRisk : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, BitingRisk, IDistributableIntervention )

    public:
        BitingRisk();
        BitingRisk( const BitingRisk& );
        virtual ~BitingRisk();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual void Update( float dt ) override;

    protected:
        IBitingRisk* m_IBitingRisk; // aka individual vector interventions container
        IDistribution* m_Distribution;

        DECLARE_SERIALIZABLE( BitingRisk );
    };
}
