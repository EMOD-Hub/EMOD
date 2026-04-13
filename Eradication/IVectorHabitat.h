
#pragma once

#include <list>
#include <map>

#include "ISerializable.h"
#include "VectorEnums.h"
#include "Types.h"
#include "GeneticProbability.h"

class Configuration;

namespace Kernel
{
    // Current timestep and immediately previous for total larva counts
    enum TimeStepIndex
    {
        CURRENT_TIME_STEP           = 0,
        PREVIOUS_TIME_STEP          = 1,
    };

    struct INodeContext;

    struct IVectorHabitat : ISerializable
    {
        virtual bool Configure( const Configuration* inputJson ) = 0;
        virtual IVectorHabitat* Clone() = 0;

        virtual void Update( float dt, INodeContext* node, const std::string& species ) = 0;

        virtual VectorHabitatType::Enum  GetVectorHabitatType()                   const = 0;
        virtual NonNegativeFloat         GetMaximumLarvalCapacity()               const = 0;
        virtual NonNegativeFloat         GetCurrentLarvalCapacity()               const = 0;
        virtual int32_t                  GetTotalLarvaCount(TimeStepIndex index)  const = 0;

        virtual void                     AddLarva(int32_t larva, float progress) = 0;
        virtual void                     AddEggs(int32_t eggs) = 0;
        virtual void                     SetMaximumLarvalCapacity(float) = 0;

        virtual float                     GetOvipositionTrapKilling()     const = 0;
        virtual const GeneticProbability& GetArtificialLarvalMortality()  const = 0;
        virtual float                     GetLarvicideHabitatScaling()    const = 0;
        virtual float                     GetRainfallMortality()          const = 0;

        virtual float GetEggCrowdingCorrection( bool refresh = false ) = 0;

        virtual float GetLocalLarvalGrowthModifier() const = 0;
        virtual float GetLocalLarvalMortality(float species_aquatic_mortality, float progress) const = 0;
        virtual const std::vector<float>& GetLarvalMicrosporidiaInfections() const = 0;

        virtual ~IVectorHabitat() {}
    };

    typedef std::list<IVectorHabitat *> VectorHabitatList_t;
}
