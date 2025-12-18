
#pragma once

#include <map>
#include <stdint.h>

#include "EnumSupport.h"
#include "Malaria.h"

namespace Kernel
{
    // ENUM defs for MALARIA_MODEL
    ENUM_DEFINE(MalariaModel, 
        ENUM_VALUE_SPEC( MALARIA_MECHANISTIC_MODEL                          , 0 )
        ENUM_VALUE_SPEC( MALARIA_MECHANISTIC_MODEL_WITH_CO_TRANSMISSION     , 1 )
        ENUM_VALUE_SPEC( MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS   , 2 ))

    // ENUM defs for MALARIA_STRAINS
    ENUM_DEFINE(MalariaStrains, 
        ENUM_VALUE_SPEC(FALCIPARUM_NONRANDOM_STRAIN                         , 11)
        ENUM_VALUE_SPEC(FALCIPARUM_RANDOM50_STRAIN                          , 12)
        ENUM_VALUE_SPEC(FALCIPARUM_RANDOM_STRAIN                            , 13)
        ENUM_VALUE_SPEC(FALCIPARUM_FIXED_STRAIN                             , 14)
        ENUM_VALUE_SPEC(FALCIPARUM_STRAIN_GENERATOR                         , 20))

    // ENUM defs for PARASITE_SWITCH_TYPE
    ENUM_DEFINE(ParasiteSwitchType, 
        ENUM_VALUE_SPEC(CONSTANT_SWITCH_RATE_2VARS                          , 0)
        ENUM_VALUE_SPEC(RATE_PER_PARASITE_7VARS                             , 1)
        ENUM_VALUE_SPEC(RATE_PER_PARASITE_5VARS_DECAYING                    , 2))

    ENUM_DEFINE(ClinicalSymptomsEnum,
        ENUM_VALUE_SPEC(CLINICAL_DISEASE        , 0)
        ENUM_VALUE_SPEC(SEVERE_DISEASE          , 1)
        ENUM_VALUE_SPEC(SEVERE_ANEMIA           , 2)
        ENUM_VALUE_SPEC(CLINICAL_SYMPTOMS_COUNT , 3))

    ENUM_DEFINE(SevereCaseTypesEnum,
        ENUM_VALUE_SPEC(NONE,           0)
        ENUM_VALUE_SPEC(ANEMIA,         1)
        ENUM_VALUE_SPEC(PARASITES,      2)
        ENUM_VALUE_SPEC(FEVER,          3))

    // ENUM defs for how maternal antibodies are modeled
    // SIMPLE_WANING draws a PfEMP1 antibody strength is initialized at a fraction of the mother's level and wanes exponentially
    // CONSTANT_INITIAL_IMMUNITY ignores the mother's level (or mean adult population level) of immunity and gives some constant initial immunity level to all children, which wanes exponentially.
    // TODO: EXPLICIT_ANTIBODIES for immune-initialization-style draw at capacity=0 and concentration=mother's level (?)
    ENUM_DEFINE(MaternalAntibodiesType,
        ENUM_VALUE_SPEC(OFF,                       0)
        ENUM_VALUE_SPEC(SIMPLE_WANING,             1)
        ENUM_VALUE_SPEC(CONSTANT_INITIAL_IMMUNITY, 2))

    ENUM_DEFINE(InnateImmuneVariationType,
        ENUM_VALUE_SPEC(NONE,                                                               0)
        ENUM_VALUE_SPEC(PYROGENIC_THRESHOLD,                                                1)
        ENUM_VALUE_SPEC(CYTOKINE_KILLING,                                                   2)
        ENUM_VALUE_SPEC(PYROGENIC_THRESHOLD_VS_AGE_CONCAVE,                                 3)
        ENUM_VALUE_SPEC(PYROGENIC_THRESHOLD_VS_AGE_INCREASING_AND_CYTOKINE_KILLING_INVERSE, 4))

    // CSP:    Circumsporozoite protein
    // MSP1:   Merozoite surface protein
    // PfEMP1: Plasmodium falciparum erythrocyte membrane protein (minor non-specific epitopes)
    //                                                            (major epitopes)

    ENUM_DEFINE(MalariaAntibodyType,
        ENUM_VALUE_SPEC(CSP,                      0)
        ENUM_VALUE_SPEC(MSP1,                     1)
        ENUM_VALUE_SPEC(PfEMP1_minor,             2)
        ENUM_VALUE_SPEC(PfEMP1_major,             3)
        ENUM_VALUE_SPEC(N_MALARIA_ANTIBODY_TYPES, 4))

    ENUM_DEFINE(AsexualCycleStatus,
        ENUM_VALUE_SPEC(NoAsexualCycle,     0)
        ENUM_VALUE_SPEC(AsexualCycle,       1)
        ENUM_VALUE_SPEC(HepatocyteRelease,  2))

    // 5 stages of development and mature gametocytes
    ENUM_DEFINE(GametocyteStages,
        ENUM_VALUE_SPEC(Stage0,     0)
        ENUM_VALUE_SPEC(Stage1,     1)
        ENUM_VALUE_SPEC(Stage2,     2)
        ENUM_VALUE_SPEC(Stage3,     3)
        ENUM_VALUE_SPEC(Stage4,     4)
        ENUM_VALUE_SPEC(Mature,     5)
        ENUM_VALUE_SPEC(Count,      6))

    ENUM_DEFINE( MalariaDiagnosticType,
        ENUM_VALUE_SPEC( BLOOD_SMEAR_PARASITES,   0 )
        ENUM_VALUE_SPEC( BLOOD_SMEAR_GAMETOCYTES, 1 )
        ENUM_VALUE_SPEC( PCR_PARASITES,           2 )
        ENUM_VALUE_SPEC( PCR_GAMETOCYTES,         3 )
        ENUM_VALUE_SPEC( PF_HRP2,                 4 )
        ENUM_VALUE_SPEC( TRUE_PARASITE_DENSITY,   5 )
        ENUM_VALUE_SPEC( FEVER,                   6 ))
}