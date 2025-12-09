
#pragma once

#include "VectorContexts.h"
#include "ISupports.h"
#include "MalariaEnums.h"     // for ClinicalSymptomsEnum, MalariaAntibodyType, MalariaDiagnosticType, enum

namespace Kernel
{
    class MalariaAntibody;
    struct pfemp1_antibody_t;
    struct NodeDemographicsDistribution;
    class SimulationConfig;
    class RANDOMBASE;

    ENUM_DEFINE( MalariaInfectionStage,
                 ENUM_VALUE_SPEC( NEW             , 0 )
                 ENUM_VALUE_SPEC( HEPATOCYTE      , 1 )
                 ENUM_VALUE_SPEC( ASEXUAL         , 2 )
                 ENUM_VALUE_SPEC( GAMETOCYTE_ONLY , 3 ) )


    struct IMalariaSimulationContext : public ISupports
    {
        virtual suids::suid GetNextParasiteSuid() = 0;
        virtual suids::suid GetNextBiteSuid() = 0;
    };

    class INodeMalaria : public ISupports
    {
    public:
        virtual float GetNewClinicalCases()         const = 0;
        virtual float GetNewSevereCases()           const = 0;
        virtual float GetMaternalAntibodyFraction() const = 0;
    };

    class IInfectionMalaria : public ISupports
    {
    public:
        virtual int64_t get_MaleGametocytes(int stage) const = 0;
        virtual int64_t get_FemaleGametocytes(int stage) const = 0;
        virtual void apply_MatureGametocyteKillProbability(float pkill) = 0;

        virtual float get_asexual_density() const = 0;
        virtual float get_mature_gametocyte_density() const = 0;
        virtual int64_t get_irbc() const = 0;
        virtual int32_t get_hepatocytes() const = 0;

        virtual MalariaInfectionStage::Enum get_InfectionStage() const = 0;
        virtual bool did_InfectionStageChangeToday() const = 0;
    };

    struct IMalariaSusceptibility : public ISupports
    {
        virtual float   get_fever()                 const = 0;
        virtual float   get_fever_celsius()         const = 0;
        virtual float   get_fever_killing_rate()    const = 0;
        virtual float   get_cytokines()             const = 0;
        virtual double  get_RBC_availability()      const = 0;
        virtual int64_t get_RBC_count()             const = 0;
        virtual float   get_parasite_density()      const = 0;
        virtual float   get_maternal_antibodies()   const = 0;
        virtual float   get_inv_microliters_blood() const = 0;
        virtual float   GetMaxFever()               const = 0;
        virtual float   GetMaxParasiteDensity()     const = 0;
        virtual float   GetHemoglobin()             const = 0;
        virtual float   GetPfHRP2()                 const = 0;

        virtual float  get_fraction_of_variants_with_antibodies(MalariaAntibodyType::Enum type) const = 0;

        virtual MalariaAntibody* RegisterAntibody( MalariaAntibodyType::Enum type, int variant, float capacity=0.0f ) = 0;
        virtual SevereCaseTypesEnum::Enum  CheckSevereCaseType() const = 0;

        virtual void BoostAntibody( MalariaAntibodyType::Enum type, int variant, float boosted_antibody_concentration ) = 0;
        virtual void UpdateActiveAntibody( pfemp1_antibody_t &pfemp1_variant, int minor_variant, int major_variant ) = 0;
        virtual void ResetMaximumSymptoms() = 0;
        virtual void SetActiveAntibody( MalariaAntibody* pAntibody ) = 0;
        virtual void init_maternal_antibodies(float mother_factor) = 0;
        virtual void remove_RBCs( int64_t infectedAsexual, int64_t infectedGametocytes, double RBC_destruction_multiplier ) = 0;
        virtual void UpdateIRBC( int64_t irbcFromInfection, int64_t irbcFromInfectionWithHRP ) = 0;
        virtual std::vector<MalariaAntibody>& GetAntibodiesForReporting( float currentTime, float dt, MalariaAntibodyType::Enum antibody_type ) = 0;
    };

    struct IMalariaHumanContext : public ISupports
    {
        virtual const SimulationConfig *params() const = 0;
        virtual float GetParasiteDensity() const = 0; // the "true" value in the model
        virtual float GetGametocyteDensity() const = 0; // the "true" value in the model
        virtual void  AddClinicalSymptom( ClinicalSymptomsEnum::Enum symptom, bool isNew ) = 0;
        virtual bool  HasClinicalSymptomNew( ClinicalSymptomsEnum::Enum symptom ) const = 0;
        virtual bool  HasClinicalSymptomContinuing( ClinicalSymptomsEnum::Enum symptom ) const = 0;
        virtual IMalariaSusceptibility* GetMalariaSusceptibilityContext() const = 0;
        virtual std::vector< std::pair<int,int> > GetInfectingStrainIds() const = 0; // potentially this would be worth putting down in IndividualHuman (generic)

        virtual float MakeDiagnosticMeasurement( MalariaDiagnosticType::Enum mdType,
                                                 float measurementSensitivity ) = 0;

        virtual float GetDiagnosticMeasurementForReports( MalariaDiagnosticType::Enum mdType ) const = 0;

        virtual bool HasMaxInfections() const = 0;
        virtual float GetMaxInfectionDuration() const = 0;
    };

    struct IMalariaHumanInfectable : public ISupports
    {
        virtual bool ChallengeWithBites( int n_infectious_bites ) = 0;
        virtual bool ChallengeWithSporozoites( int n_sporozoites ) = 0;
    };
}
