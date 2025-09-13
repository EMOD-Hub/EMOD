
#pragma once

#include <string>

#include "ISupports.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "VectorEnums.h"
#include "VectorContexts.h"
#include "Types.h"
#include "LarvalHabitatMultiplier.h"
#include "IMosquitoReleaseConsumer.h"
#include "GeneticProbability.h"

namespace Kernel
{
    class Simulation;

    class INodeVectorInterventionEffectsApply : public ISupports
    {
    public:
        virtual void UpdateLarvalKilling(VectorHabitatType::Enum habitat, const GeneticProbability& killing) = 0;
        virtual void UpdateLarvalHabitatReduction(VectorHabitatType::Enum habitat, float reduction) = 0;
        virtual void UpdateLarvalHabitatReduction(const LarvalHabitatMultiplier& lhm) = 0;
        virtual void UpdateOutdoorKilling(const GeneticProbability& killing) = 0;
        virtual void UpdateOviTrapKilling(VectorHabitatType::Enum habitat, float killing) = 0;
        virtual void UpdateOutdoorNodeEmanator(const float coverage,
                                               const GeneticProbability& repelling, 
                                               const GeneticProbability& killing) = 0;
        virtual void UpdateADIVAttraction(float) = 0;
        virtual void UpdateADOVAttraction(float) = 0;
        virtual void UpdateSugarFeedKilling(const GeneticProbability& killing) = 0;
        virtual void UpdateAnimalFeedKilling(const GeneticProbability& killing) = 0;
        virtual void UpdateOutdoorRestKilling(const GeneticProbability& killing) = 0;
        virtual void UpdateIndoorKilling(const GeneticProbability& killing) = 0;
    };

    class NodeVectorEventContextHost :
        public NodeEventContextHost,
        public INodeVectorInterventionEffects,
        public INodeVectorInterventionEffectsApply,
        public IMosquitoReleaseConsumer
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()

    public:
        NodeVectorEventContextHost(Node* _node);
        virtual ~NodeVectorEventContextHost();
  
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;
       
        virtual void UpdateInterventions(float dt);

        // INodeVectorInterventionEffectsApply
        virtual void UpdateLarvalKilling(VectorHabitatType::Enum habitat, const GeneticProbability& killing) override;
        virtual void UpdateLarvalHabitatReduction(VectorHabitatType::Enum habitat, float reduction) override;
        virtual void UpdateLarvalHabitatReduction(const LarvalHabitatMultiplier& lhm) override;
        virtual void UpdateOutdoorKilling(const GeneticProbability& killing) override;
        virtual void UpdateOutdoorNodeEmanator(const float coverage,
                                               const GeneticProbability& repelling,
                                               const GeneticProbability& killing) override;
        virtual void UpdateADIVAttraction(float reduction) override;
        virtual void UpdateADOVAttraction(float reduction) override;
        virtual void UpdateSugarFeedKilling(const GeneticProbability& killing) override;
        virtual void UpdateOviTrapKilling(VectorHabitatType::Enum  habitat, float killing) override;
        virtual void UpdateAnimalFeedKilling(const GeneticProbability& killing) override;
        virtual void UpdateOutdoorRestKilling(const GeneticProbability& killing) override;
        virtual void UpdateIndoorKilling(const GeneticProbability& killing) override;

        // INodeVectorInterventionEffects;
        virtual const GeneticProbability& GetLarvalKilling(VectorHabitatType::Enum) const override;
        virtual float GetLarvalHabitatReduction(VectorHabitatType::Enum, const std::string& species) override;
        virtual const GeneticProbability& GetVillageSpatialRepellent() override;
        virtual const GeneticProbability& GetVillageEmanatorNotRepelledOrKilledOrAffected() override;
        virtual float GetADIVAttraction() override;
        virtual float GetADOVAttraction() override;
        virtual const GeneticProbability& GetOutdoorKilling() override;
        virtual float GetOviTrapKilling(VectorHabitatType::Enum) override;
        virtual const GeneticProbability& GetAnimalFeedKilling() override;
        virtual const GeneticProbability& GetOutdoorRestKilling() override;
        virtual bool  IsUsingIndoorKilling() const override;
        virtual const GeneticProbability& GetIndoorKilling() override;
        virtual bool  IsUsingSugarTrap() const override;
        virtual const GeneticProbability& GetSugarFeedKilling() const override;

        VectorHabitatType::Enum larval_killing_target;
        VectorHabitatType::Enum larval_reduction_target;
        VectorHabitatType::Enum ovitrap_killing_target;

        LarvalHabitatMultiplier larval_reduction;

        // IMosquitoReleaseConsumer
        virtual void ReleaseMosquitoes( const std::string& releasedSpecies,
                                        const    VectorGenome& rGenome,
                                        const    VectorGenome& rMateGenome,
                                        bool     isRatio,
                                        uint32_t releasedNumber,
                                        float    releasedRatio,
                                        float    releasedInfectious ) override;

    protected: 
        GeneticProbability pLarvalKilling;
        float              pLarvalHabitatReduction;
        GeneticProbability pVillageSpatialRepellent;
        GeneticProbability pVillageNotRepelledOrKilledOrAffected;
        float              pADIVAttraction;
        float              pADOVAttraction;
        GeneticProbability pOutdoorKilling;
        float              pOviTrapKilling;
        GeneticProbability pAnimalFeedKilling;
        GeneticProbability pOutdoorRestKilling;
        bool               isUsingIndoorKilling;
        GeneticProbability pIndoorKilling;
        bool               isUsingSugarTrap;
        GeneticProbability pSugarFeedKilling;

    private:
        NodeVectorEventContextHost() : NodeEventContextHost(nullptr) { }
    };
}
