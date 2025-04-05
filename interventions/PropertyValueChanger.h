
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"

namespace Kernel
{
    class PropertyValueChanger : public BaseIntervention
    {
    public:
        virtual bool Configure( const Configuration * config ) override;

        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, PropertyValueChanger, IDistributableIntervention)

    public:
        PropertyValueChanger();
        PropertyValueChanger( const PropertyValueChanger& rThat );

        // factory method
        virtual ~PropertyValueChanger();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        virtual int AddRef() override;
        virtual int Release() override;

    protected:
        jsonConfigurable::ConstrainedString target_property_key;
        jsonConfigurable::ConstrainedString target_property_value;
        float probability;
        float revert;
        float max_duration;
        float action_timer;
        float reversion_timer;

        DECLARE_SERIALIZABLE(PropertyValueChanger);
    };
}
