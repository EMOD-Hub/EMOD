
#pragma once

#include <stdint.h>
#include <list>
#include <vector>

#include "IVectorHabitat.h"
#include "IArchive.h"
#include "Configure.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
    struct INodeContext;
    class  SimulationConfig;

    class VectorHabitat : public JsonConfigurable, public IVectorHabitat 
    {
    public:
        static IVectorHabitat* CreateHabitat( VectorHabitatType::Enum type, const Configuration* inputJson );
        virtual ~VectorHabitat();
        virtual void Update( float dt, INodeContext* node, const std::string& species ) override;

        virtual bool Configure( const Configuration* inputJson ) override;

        virtual VectorHabitatType::Enum  GetVectorHabitatType()                   const override;
        virtual NonNegativeFloat         GetMaximumLarvalCapacity()               const override;
        virtual NonNegativeFloat         GetCurrentLarvalCapacity()               const override;
        virtual int32_t                  GetTotalLarvaCount(TimeStepIndex index)  const override;

        virtual void                     AddLarva(int32_t larva, float progress) override;
        virtual void                     AddEggs(int32_t eggs) override;
        virtual void                     SetMaximumLarvalCapacity(float) override;

        virtual float                     GetOvipositionTrapKilling()     const override;
        virtual const GeneticProbability& GetArtificialLarvalMortality()  const override;
        virtual float                     GetLarvicideHabitatScaling()    const override;
        virtual float                     GetRainfallMortality()          const override;

        virtual float GetEggCrowdingCorrection( bool refresh = false ) override;
        virtual float GetLocalLarvalGrowthModifier() const override;
        virtual float GetLocalLarvalMortality(float species_aquatic_mortality, float progress) const override;
        virtual const std::vector<float>& GetLarvalMicrosporidiaInfections() const override;

    protected:
        VectorHabitat( VectorHabitatType::Enum type );
        VectorHabitat( const VectorHabitat& rMaster );

        virtual QueryResult QueryInterface( iid_t, void** ) override { return e_NOINTERFACE; }
        virtual int32_t AddRef() override { return 1; }
        virtual int32_t Release() override { return 0; }

        void CalculateEggCrowdingCorrection();
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node);
        void UpdateLarvalProbabilities( float dt, INodeContext* node, const std::string& species );
        void UpdateRainfallMortality(float dt, float rainfall);
        void AdvanceTotalLarvaCounts();

        const SimulationConfig* params() const;

        static void serialize(IArchive& ar, VectorHabitat* obj);

        VectorHabitatType::Enum  m_habitat_type;
        float                    m_max_larval_capacity;
        float                    m_current_larval_capacity;
        std::vector<int32_t>     m_total_larva_count;
        int32_t                  m_new_egg_count;

        float                    m_oviposition_trap_killing;
        GeneticProbability       m_artificial_larval_mortality;
        float                    m_larvicide_habitat_scaling;
        float                    m_rainfall_mortality;
        float                    m_egg_crowding_correction;
        // Not serialized: recomputed each timestep from node-level microsporidia interventions.
        std::vector<float> m_portion_larva_infected_microsporidia;
    };

    void serialize(IArchive&, map<VectorHabitatType::Enum, float>&);

    /* TODO: transition some configuration parameters into these objects, e.g. decay constants, rainfall killing */
    /* TODO: encapsulate allowed combinations of egg-crowding and density-dependent parameterization? */

    class ConstantHabitat : public VectorHabitat
    {
    public:
        ConstantHabitat();
        ConstantHabitat( const ConstantHabitat& rMaster );

        virtual IVectorHabitat* Clone() override;
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node) override;
    protected:
        DECLARE_SERIALIZABLE(ConstantHabitat);
    };

    class TemporaryRainfallHabitat : public VectorHabitat
    {
    public:
        TemporaryRainfallHabitat();
        TemporaryRainfallHabitat( const TemporaryRainfallHabitat& rMaster );

        virtual IVectorHabitat* Clone() override;
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node) override;

    protected:
        DECLARE_SERIALIZABLE(TemporaryRainfallHabitat);
    };

    class WaterVegetationHabitat : public VectorHabitat
    {
    public:
        WaterVegetationHabitat();
        WaterVegetationHabitat( const WaterVegetationHabitat& rMaster );

        virtual IVectorHabitat* Clone() override;
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node) override;
 
    protected:
        DECLARE_SERIALIZABLE(WaterVegetationHabitat);
    };

    class HumanPopulationHabitat : public VectorHabitat
    {
    public:
        HumanPopulationHabitat();
        HumanPopulationHabitat( const HumanPopulationHabitat& rMaster );

        virtual IVectorHabitat* Clone() override;
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node) override;

    protected:
        DECLARE_SERIALIZABLE(HumanPopulationHabitat);
    };

    class BrackishSwampHabitat : public VectorHabitat
    {
    public:
        BrackishSwampHabitat();
        BrackishSwampHabitat( const BrackishSwampHabitat& rMaster );

        virtual IVectorHabitat* Clone() override;
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node) override;

    protected:
        DECLARE_SERIALIZABLE(BrackishSwampHabitat);
    };

    class LinearSplineHabitat : public VectorHabitat
    {
    public:
        LinearSplineHabitat();
        LinearSplineHabitat( const LinearSplineHabitat& rMaster );

        virtual bool Configure( const Configuration* inputJson );

        virtual IVectorHabitat* Clone() override;
        virtual void UpdateCurrentLarvalCapacity(float dt, INodeContext* node) override;

    protected:
        float day_of_distribution;
        float duration_of_distribution; 
        InterpolatedValueMap capacity_distribution;

        DECLARE_SERIALIZABLE(LinearSplineHabitat);
    };
}
