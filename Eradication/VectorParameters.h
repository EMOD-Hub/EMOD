
#pragma once
#include <map>

#include "Vector.h"
#include "VectorEnums.h"   // remove dependency when egg, larva, and vector enums are configured outside this class
#include "Configure.h"
#include "VectorSpeciesParameters.h"
#include "Insecticides.h"

namespace Kernel
{
    class VectorSpeciesParameters;


    struct VectorParameters
    {
        EggHatchDelayDist::Enum                 egg_hatch_delay_dist;                // eggHatchDelayDist
        EggSaturation::Enum                     egg_saturation;                      // eggSaturation
        LarvalDensityDependence::Enum           larval_density_dependence;           // larvalDensityDependence
        VectorSamplingType::Enum                vector_sampling_type;                // Vector_Sampling_Type
        VectorRainfallMortality::Enum           vector_larval_rainfall_mortality;    // vectorLarvalRainfallMortality
        EggHatchDensityDependence::Enum         egg_hatch_density_dependence;        // Egg_Hatch_Density_Dependence 

        bool  enable_vector_species_report;
        bool  vector_aging;
        bool  temperature_dependent_egg_hatching;
        bool  egg_mortality; 
        bool  delayed_hatching_when_habitat_dries_up;
        float meanEggHatchDelay;
        float droughtEggHatchDelay;
        float WolbachiaMortalityModification;
        float WolbachiaInfectionModification;
        float HEGhomingRate;
        float HEGfecundityLimiting;
        float human_feeding_mortality;
        float eggarrhenius1;
        float eggarrhenius2;
        bool  temperature_dependent_microsporidia_infectivity;
        float microsporidia_arrhenius1;
        float microsporidia_arrhenius2;

        // allows configuration of larval habitat decay time constants
        float tempHabitatDecayScalar;
        float semipermanentHabitatDecayRate;
        float mmRainfallToFillSwamp;
        float larval_rainfall_mortality_threshold;

        // allows configuration of larval density dependence
        float larvalDensityMortalityScalar;
        float larvalDensityMortalityOffset;

        float x_tempLarvalHabitat;

        VectorSpeciesCollection vector_species;
        InsecticideCollection insecticides;
        GeneticProbability blood_meal_mortality;

        VectorParameters()
        : egg_hatch_delay_dist(EggHatchDelayDist::NO_DELAY)
        , egg_saturation(EggSaturation::NO_SATURATION)
        , larval_density_dependence(LarvalDensityDependence::NO_DENSITY_DEPENDENCE)
        , vector_sampling_type(VectorSamplingType::TRACK_ALL_VECTORS)
        , vector_larval_rainfall_mortality(VectorRainfallMortality::NONE)
        , egg_hatch_density_dependence( EggHatchDensityDependence::NO_DENSITY_DEPENDENCE )
        , enable_vector_species_report(false)
        , vector_aging(false)
        , temperature_dependent_egg_hatching(false)
        , egg_mortality(false)
        , delayed_hatching_when_habitat_dries_up(false)
        , meanEggHatchDelay(0.0f)
        , droughtEggHatchDelay(0.0f)
        , WolbachiaMortalityModification(0.0f)
        , WolbachiaInfectionModification(0.0f)
        , HEGhomingRate(0.0f)
        , HEGfecundityLimiting(0.0f)
        , human_feeding_mortality(DEFAULT_HUMAN_FEEDING_MORTALITY)
        , eggarrhenius1( 0.0f)
        , eggarrhenius2( 0.0f)
        , temperature_dependent_microsporidia_infectivity(false)
        , microsporidia_arrhenius1(0.0f)
        , microsporidia_arrhenius2(0.0f) 
        , tempHabitatDecayScalar(0.0f)
        , semipermanentHabitatDecayRate(0.0f)
        , mmRainfallToFillSwamp(1.0f)
        , larval_rainfall_mortality_threshold(1.0f)
        , larvalDensityMortalityScalar(10.0f)
        , larvalDensityMortalityOffset(0.1f)
        , x_tempLarvalHabitat(1.0f)
        , vector_species()
        , insecticides(&vector_species)
        , blood_meal_mortality(0.0)
        {
        }
    };
}
