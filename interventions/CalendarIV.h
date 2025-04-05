
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "HealthSeekingBehavior.h"

namespace Kernel
{
    class IVCalendar : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, IVCalendar, IDistributableIntervention)

    public:
        // We inherit AddRef/Release abstractly through IHealthSeekBehavior,
        // even though BaseIntervention has a non-abstract version.
        virtual int32_t AddRef() override { return BaseIntervention::AddRef(); }
        virtual int32_t Release() override { return BaseIntervention::Release(); }

        IVCalendar();
        IVCalendar( const IVCalendar& rMaster );
        virtual ~IVCalendar();
        bool Configure( const Configuration* config ) override;

        // IDistributingDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

    protected:
        std::map< float, float > age2ProbabilityMap;
        std::vector<IDistributableIntervention*> m_Interventions;
        bool dropout;

        DECLARE_SERIALIZABLE(IVCalendar);

    private:
        std::list<float> scheduleAges;
        std::string dumpCalendar();
    };
}
