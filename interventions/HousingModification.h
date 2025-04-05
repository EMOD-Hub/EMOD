
#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"
#include "Insecticides.h"
#include "InsecticideWaningEffect.h"

namespace Kernel
{
    struct IHousingModificationConsumer;

    class SimpleHousingModification : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleHousingModification, IDistributableIntervention)

    public:
        SimpleHousingModification();
        SimpleHousingModification( const SimpleHousingModification& );
        virtual ~SimpleHousingModification();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void initConfigInsecticideName( InsecticideName* pName );
        virtual void initConfigRepelling( WaningConfig* pRepellingConfig );
        virtual void initConfigKilling( WaningConfig* pKillingConfig );
        virtual void SetInsecticideName( InsecticideName& rName );
        virtual void ApplyEffectsRepelling( float dt );
        virtual void ApplyEffectsKilling( float dt );

        IInsecticideWaningEffect* m_pInsecticideWaningEffect;
        IHousingModificationConsumer *m_pIHMC; // aka individual or individual vector interventions container

        DECLARE_SERIALIZABLE(SimpleHousingModification);
    };

    class IRSHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, IRSHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(IRSHousingModification);
    };

    class MultiInsecticideIRSHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory,
                                    MultiInsecticideIRSHousingModification,
                                    IDistributableIntervention )

        DECLARE_SERIALIZABLE(MultiInsecticideIRSHousingModification);

    public:
        virtual bool Configure( const Configuration * config ) override;

    protected:
    };

    class ScreeningHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, ScreeningHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(ScreeningHousingModification);
    };

    class SpatialRepellentHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SpatialRepellentHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(SpatialRepellentHousingModification);

    protected:
        virtual void initConfigKilling( WaningConfig* pKillingConfig );
        virtual void ApplyEffectsKilling( float dt ) override;
    };
}
