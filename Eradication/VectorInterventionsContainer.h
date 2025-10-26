
#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionEnums.h"
#include "InterventionsContainer.h"
#include "VectorContexts.h"
#include "VectorInterventionsContainerContexts.h"

namespace Kernel
{
    // This container becomes a help implementation member of the relevant IndividualHuman class.
    // It needs to implement consumer interfaces for all the relevant intervention types.

    class VectorInterventionsContainer : public InterventionsContainer,
                                         public IVectorInterventionsEffects,
                                         public IVectorInterventionEffectsSetter,
                                         public IBednetConsumer,
                                         public IHousingModificationConsumer,
                                         public IIndividualRepellentConsumer,
                                         public IBitingRisk
    {
    public:
        VectorInterventionsContainer();
        virtual ~VectorInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;
        virtual int AddRef() override;
        virtual int Release() override;

        // IBednetConsumer
        virtual void UpdateProbabilityOfBlocking( const GeneticProbability& prob ) override;
        virtual void UpdateProbabilityOfKilling( const GeneticProbability& prob ) override;

        // IHousingModificationConsumer
        virtual void UpdateProbabilityOfHouseRepelling( const GeneticProbability& prob ) override;
        virtual void UpdateProbabilityOfHouseKilling( const GeneticProbability& prob ) override;
        virtual void UpdateProbabilityOfIndoorEmanatorKilling( const GeneticProbability& prob ) override;

        // IIndividualRepellentConsumer
        virtual void UpdateProbabilityOfIndRep( const GeneticProbability& prob ) override;

        // IVectorInterventionEffectsSetter
        virtual void UpdateArtificialDietAttractionRate( float rate ) override;
        virtual void UpdateArtificialDietKillingRate( float rate ) override;
        virtual void UpdateInsecticidalDrugKillingProbability( const GeneticProbability& prob ) override;

        virtual void InfectiousLoopUpdate( float dt ) override; 
        virtual void Update( float dt ) override; // update non-infectious loop update interventions once per time step

        // IVectorInterventionEffects
        virtual uint32_t GetHumanID() const override;
        virtual const GeneticProbability& GetDieBeforeFeeding() override;
        virtual const GeneticProbability& GetHostNotAvailable() override;
        virtual const GeneticProbability& GetDieDuringFeeding() override;
        virtual const GeneticProbability& GetDiePostFeeding() override;
        virtual const GeneticProbability& GetSuccessfulFeedHuman() override;
        virtual const GeneticProbability& GetSuccessfulFeedAD() override;
        virtual float                     GetOutdoorDieBeforeFeeding() override;
        virtual const GeneticProbability& GetOutdoorHostNotAvailable() override;
        virtual const GeneticProbability& GetOutdoorDieDuringFeeding() override;
        virtual const GeneticProbability& GetOutdoorDiePostFeeding() override;
        virtual const GeneticProbability& GetOutdoorSuccessfulFeedHuman() override;
        virtual const GeneticProbability& GetblockIndoorVectorAcquire() override;
        virtual const GeneticProbability& GetblockIndoorVectorTransmit() override;
        virtual const GeneticProbability& GetblockOutdoorVectorAcquire() override;
        virtual const GeneticProbability& GetblockOutdoorVectorTransmit() override;

        // IBitingRisk
        virtual void UpdateRelativeBitingRate( float rate ) override;

    protected:
        // These are calculated from the values set by the interventions and returned to the model
        GeneticProbability p_block_net;
        GeneticProbability p_kill_ITN;
        GeneticProbability p_penetrate_housingmod;
        bool               is_using_housingmod;
        GeneticProbability p_survive_irshousingmod;
        GeneticProbability p_survive_emanator;
        GeneticProbability p_indrep;
        float              p_attraction_ADIH;
        float              p_kill_ADIH;
        GeneticProbability p_survive_insecticidal_drug;

        // These are set by interventions
        GeneticProbability pDieBeforeFeeding;
        GeneticProbability pHostNotAvailable;
        GeneticProbability pDieDuringFeeding;
        GeneticProbability pDiePostFeeding;
        GeneticProbability pSuccessfulFeedHuman;
        GeneticProbability pSuccessfulFeedAD;
        float              pOutdoorDieBeforeFeeding;
        GeneticProbability pOutdoorHostNotAvailable;
        GeneticProbability pOutdoorDieDuringFeeding;
        GeneticProbability pOutdoorDiePostFeeding;
        GeneticProbability pOutdoorSuccessfulFeedHuman;
        GeneticProbability blockIndoorVectorAcquire;
        GeneticProbability blockIndoorVectorTransmit;
        GeneticProbability blockOutdoorVectorAcquire;
        GeneticProbability blockOutdoorVectorTransmit;

        DECLARE_SERIALIZABLE(VectorInterventionsContainer);
    };
}
