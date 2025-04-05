
#pragma once

#include "IDrug.h"
#include "InterventionEnums.h"      // return types
#include "InterventionFactory.h"    // macros that 'auto'-register classes
#include "Configure.h"              // base classes
#include "SimulationEnums.h"        // for PkPdModel
#include "DrugModel.h"

namespace Kernel
{
    class RANDOMBASE;
    struct ICampaignCostObserver;

    class GenericDrug : public IDrug, public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, GenericDrug, IDistributableIntervention)

    public:
        GenericDrug( const GenericDrug& rThat );
        virtual ~GenericDrug();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual int AddRef() override;
        virtual int Release() override;

        // IDistributableIntervention
        virtual void Update(float dt) override;
        virtual bool NeedsInfectiousLoopUpdate() const
        { 
            // Drugs typically change things that affect the infection so
            // they need to be in the infectious update loop.
            return true;
        }
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        // IDrug
        virtual const std::string& GetDrugName() const override;
        virtual float GetDrugCurrentEfficacy() const override;
        virtual float GetDrugCurrentConcentration() const override;
        virtual int   GetNumRemainingDoses() const override;

    protected:
        GenericDrug( const std::string& rDefaultName = JsonConfigurable::default_string,
                     DrugModel* pDrugModel = nullptr );

        virtual bool IsTakingDose( float dt ) { return true; }

        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc );
        virtual float GetDrugReducedAcquire()  const;
        virtual float GetDrugReducedTransmit() const;

        virtual void TakeDose( float dt, RANDOMBASE* pRNG, IIndividualHumanInterventionsContext * ivc );
        virtual void DecayAndUpdateEfficacy( float dt );
        virtual void ResetForNextDose(float dt);
        virtual void ApplyEffects(); // virtual, not part of interface
        virtual void Expire();

        float CalculateEfficacy( float c50, float startConcentration, float endConcentration );


        DrugModel* p_drug_model;
        float dosing_timer;        //time to next dose for ongoing treatment
        int   remaining_doses;     //number of doses left in treatment
        float time_between_doses;

        DECLARE_SERIALIZABLE(GenericDrug);
    };
}
