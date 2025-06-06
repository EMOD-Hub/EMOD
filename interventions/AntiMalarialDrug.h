
#pragma once

#include "Drugs.h"
#include "MalariaInterventionsContainerContexts.h"  // for IMalariaDrugEffects methods

namespace Kernel
{
    class MalariaDrugTypeParameters;

    class AntimalarialDrug : public GenericDrug, public IMalariaDrugEffects
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, AntimalarialDrug, IDistributableIntervention)

    public:
        AntimalarialDrug();
        AntimalarialDrug( const AntimalarialDrug& rThat );
        virtual ~AntimalarialDrug();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual int32_t AddRef() override { return GenericDrug::AddRef(); }
        virtual int32_t Release() override { return GenericDrug::Release(); }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;

        // IMalariaDrugEffects
        float get_drug_IRBC_killrate( const IStrainIdentity& rStrain ) override;
        float get_drug_hepatocyte(    const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyte02(  const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyte34(  const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyteM(   const IStrainIdentity& rStrain ) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc ) override;  // read in PkPd and drug-killing parameters from SusceptibilityFlagsMalaria
        virtual void ResetForNextDose(float dt) override;

        virtual void ApplyEffects() override;
        virtual void Expire() override;

        virtual int GetNumDoses() const;
        virtual float GetDoseInterval() const;

        virtual void ConfigureDrugType( const Configuration *intputJson );
        virtual void CheckConfigureDrugType( const Configuration *intputJson );

        jsonConfigurable::ConstrainedString tmp_drug_name; // only used for reading in the drug name between ConfigureDrugType() and CheckConfigureDrugType()

        IMalariaDrugEffectsApply * imda;

        DECLARE_SERIALIZABLE(AntimalarialDrug);
    };
}
