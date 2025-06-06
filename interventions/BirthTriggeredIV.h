
#pragma once

#include <string>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"
#include "Configure.h"
#include "DemographicRestrictions.h"

namespace Kernel
{
    class BirthTriggeredIV : public IIndividualEventObserver, public BaseNodeIntervention // , public INodeDistributableInterventionParameterSetterInterface
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, BirthTriggeredIV, INodeDistributableIntervention)
    
        friend class CalendarEventCoordinator;
    
    public:
        BirthTriggeredIV();
        BirthTriggeredIV( const BirthTriggeredIV& rMaster );
        virtual ~BirthTriggeredIV();
        virtual int AddRef() override;
        virtual int Release() override;
        virtual bool Configure( const Configuration* config ) override;

        // INodeDistributableIntervention
        virtual bool Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

        // IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

        virtual void SetDemographicCoverage(float new_coverage) { demographic_restrictions.SetDemographicCoverage( new_coverage );};
        virtual void SetMaxDuration(float new_duration) {max_duration = new_duration;};

    protected:
        void Unregister();

        float max_duration;
        float duration;
        DemographicRestrictions demographic_restrictions;
        IDistributableIntervention* m_di;
    };
}
