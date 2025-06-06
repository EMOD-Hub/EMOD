
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    ENUM_DEFINE(SimpleVaccineType,
        ENUM_VALUE_SPEC(Generic              , 1)
        ENUM_VALUE_SPEC(TransmissionBlocking , 2)
        ENUM_VALUE_SPEC(AcquisitionBlocking  , 3)
        ENUM_VALUE_SPEC(MortalityBlocking    , 4))

    struct IVaccineConsumer;
    struct ICampaignCostObserver;

    struct IVaccine : public ISupports
    {
        virtual bool ApplyVaccineTake( IIndividualHumanContext* pihc ) = 0;
        virtual ~IVaccine() { } // needed for cleanup via interface pointer
    };

    class SimpleVaccine : public BaseIntervention, public IVaccine
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleVaccine, IDistributableIntervention)

    public:
        SimpleVaccine();
        SimpleVaccine( const SimpleVaccine& );
        virtual ~SimpleVaccine();
        virtual int AddRef() override { return BaseIntervention::AddRef(); }
        virtual int Release() override { return BaseIntervention::Release(); }
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;
        virtual bool NeedsInfectiousLoopUpdate() const;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IVaccine
        virtual bool  ApplyVaccineTake( IIndividualHumanContext* pihc ); 

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:

        int   vaccine_type;
        float vaccine_take;
        bool  vaccine_took;
        bool efficacy_is_multiplicative;
        IWaningEffect* waning_effect;
        IVaccineConsumer * ivc; // interventions container

        DECLARE_SERIALIZABLE(SimpleVaccine);
    };
}
