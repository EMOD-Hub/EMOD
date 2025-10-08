
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
        virtual void UpdateVillageSpatialRepellent(const GeneticProbability& repelling) = 0;
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
        virtual void UpdateVillageSpatialRepellent(const GeneticProbability& repelling) override;
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
        virtual const GeneticProbability&  GetVillageSpatialRepellent() const override;
        virtual float GetADIVAttraction() const override;
        virtual float GetADOVAttraction() const override;
        virtual const GeneticProbability& GetOutdoorKilling() const override;
        virtual float GetOviTrapKilling(VectorHabitatType::Enum) const override;
        virtual const GeneticProbability& GetAnimalFeedKilling() const override;
        virtual const GeneticProbability& GetOutdoorRestKilling() const override;
        virtual bool  IsUsingIndoorKilling() const override;
        virtual const GeneticProbability& GetIndoorKilling() const override;
        virtual bool  IsUsingSugarTrap() const override;
        virtual const GeneticProbability& GetSugarFeedKilling() const override;

        VectorHabitatType::Enum larval_reduction_target;
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
        std::vector<GeneticProbability> larval_killing_list;
        std::vector<float> oviposition_killing_list;
        float              pLarvalHabitatReduction;
        GeneticProbability pVillageSpatialRepellent;
        float              pADIVAttraction;
        float              pADOVAttraction;
        GeneticProbability pOutdoorKilling;
        GeneticProbability pAnimalFeedKilling;
        GeneticProbability pOutdoorRestKilling;
        bool               isUsingIndoorKilling;
        GeneticProbability pIndoorKilling;
        bool               isUsingSugarTrap;
        GeneticProbability pSugarFeedKilling;

    private:
        float CombineProbabilities( float prob1, float prob2 );
        NodeVectorEventContextHost() : NodeEventContextHost(nullptr) { }
    };
}
