
#pragma once

#include "AntiMalarialDrug.h"

namespace Kernel
{
    class DrugModelAntiMalarial;

    class MultiPackComboDrug : public AntimalarialDrug
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MultiPackComboDrug, IDistributableIntervention)

    public:
        MultiPackComboDrug();
        MultiPackComboDrug( const MultiPackComboDrug& rThat );
        virtual ~MultiPackComboDrug();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual int32_t AddRef() override { return AntimalarialDrug::AddRef(); }
        virtual int32_t Release() override { return AntimalarialDrug::Release(); }

        // IDrug
        virtual const std::string& GetDrugName() const override;
        virtual float GetDrugCurrentEfficacy() const override;
        virtual float GetDrugCurrentConcentration() const override;

        // IMalariaDrugEffects
        float get_drug_IRBC_killrate( const IStrainIdentity& rStrain ) override;
        float get_drug_hepatocyte(    const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyte02(  const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyte34(  const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyteM(   const IStrainIdentity& rStrain ) override;

    protected:
        virtual int GetNumDoses() const override;
        virtual float GetDoseInterval() const override;
        virtual void ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc ) override;
        virtual void TakeDose( float dt, RANDOMBASE* pRNG, IIndividualHumanInterventionsContext * ivc ) override;
        virtual void DecayAndUpdateEfficacy( float dt ) override;
        virtual void ResetForNextDose( float dt ) override;
        void UpdateDrugName();

        virtual void ConfigureDrugType( const Configuration *intputJson ) override;
        virtual void CheckConfigureDrugType( const Configuration *intputJson ) override;

        std::vector<std::vector<std::string>> m_DosesNames;
        float m_DoseInterval;
        int m_CurrentDoseIndex;
        std::string m_NamesOfDrugsTaken; // needed so that GetDrugName() can return a reference to it.
        std::map<std::string, DrugModelAntiMalarial*> m_DrugsTaken;

        DECLARE_SERIALIZABLE( MultiPackComboDrug );
    };
}
