
#pragma once

#include "Common.h"
#include "MalariaAntibody.h"
#include "MalariaEnums.h"
#include "MalariaContexts.h"
#include "SusceptibilityVector.h"

namespace Kernel
{
    class SusceptibilityMalariaConfig : public SusceptibilityVectorConfig
    {
        friend class IndividualHumanMalaria;

    public:
        friend class SusceptibilityMalaria;

        virtual bool Configure( const Configuration* config ) override;

        // These public configurable parameters are accessed by MalariaAntibody for Decay and Update functions
        static float memory_level;
        static float hyperimmune_decay_rate;
        static float MSP1_antibody_growthrate;
        static float antibody_stimulation_c50;
        static float antibody_capacity_growthrate;
        static float antibody_days_to_long_term_decay;
        static float antibody_long_term_decay_days;
        static float minimum_adapted_response;
        static float non_specific_growth;
        static float antibody_csp_decay_days;

        // antigen population
        static int falciparumMSPVars;
        static int falciparumNonSpecTypes;
        static int falciparumPfEMP1Vars;

    protected:
        static bool enable_maternal_antibodies_transmission;
        static MaternalAntibodiesType::Enum maternal_antibodies_type;
        static float maternal_antibody_protection;
        static float maternal_antibody_decay_rate;
        static InnateImmuneVariationType::Enum innate_immune_variation_type;
        static float base_gametocyte_mosquito_survival;
        static float cytokine_gametocyte_inactivation;
        static float anemiaMortalityLevel;
        static float parasiteMortalityLevel;
        static float feverMortalityLevel;
        static float anemiaMortalityInvWidth;
        static float parasiteMortalityInvWidth;
        static float feverMortalityInvWidth;
        static float anemiaSevereLevel;
        static float parasiteSevereLevel;
        static float feverSevereLevel;
        static float anemiaSevereInvWidth;
        static float parasiteSevereInvWidth;
        static float feverSevereInvWidth;
        static float erythropoiesis_anemia_effect;
        static float pyrogenic_threshold;
        static float fever_IRBC_killrate;
        static float PfHRP2_boost_rate; // Picograms/iRBC/day: 1.4e-13 grams per 48h / 2 (48h cycle to daily)
        static float PfHRP2_decay_rate; // Fraction Per Day - fraction removed after each day (based on 3.67 day half-life)

        // thresholds for defining a unique clinical incident (from spiking symptoms of concurrent infections)
        // EAW: are either of these redundant with IndividualHumanFlagsMalaria.feverDetectionThreshold?
        static float clinicalFeverThreshold_low;
        static float clinicalFeverThreshold_high;
        static float minDaysBetweenClinicalIncidents;

        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityMalariaConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    class SusceptibilityMalaria : public SusceptibilityVector, public IMalariaSusceptibility
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:

        static SusceptibilityMalaria *CreateSusceptibility(IIndividualHumanContext *context, float _age = 20 * DAYSPERYEAR, float immmod = 1.0f, float riskmod = 1.0f);
        virtual ~SusceptibilityMalaria();

        virtual void Update(float dt) override;
        virtual void UpdateInfectionCleared() override;

        // -------------------------------------
        // --- IMalariaSusceptibility interfaces
        // -------------------------------------
        virtual float   get_fever()                 const override;
        virtual float   get_fever_celsius()         const override;
        virtual float   get_fever_killing_rate()    const override;
        virtual float   get_cytokines()             const override;
        virtual double  get_RBC_availability()      const override;
        virtual int64_t get_RBC_count()             const override;
        virtual float   get_parasite_density()      const override;
        virtual float   get_maternal_antibodies()   const override;
        virtual float   get_inv_microliters_blood() const override;
        virtual float   GetMaxFever()               const override;
        virtual float   GetMaxParasiteDensity()     const override;
        virtual float   GetHemoglobin()             const override;
        virtual float   GetPfHRP2()                 const override;

        virtual float  get_fraction_of_variants_with_antibodies(MalariaAntibodyType::Enum type) const override;

        virtual MalariaAntibody* RegisterAntibody(MalariaAntibodyType::Enum type, int variant, float capacity=0.0f) override;
        virtual SevereCaseTypesEnum::Enum  CheckSevereCaseType() const override;

        // functions to mediate interaction with Infection_Malaria objects
        virtual void BoostAntibody( MalariaAntibodyType::Enum type, int variant, float boosted_antibody_concentration ) override;
        virtual void UpdateActiveAntibody( pfemp1_antibody_t &pfemp1_variant, int minor_variant, int major_variant ) override;

        virtual void ResetMaximumSymptoms() override;
        virtual void SetActiveAntibody( MalariaAntibody* pAntibody ) override;
        virtual void init_maternal_antibodies(float mother_factor) override;

        // functions to mediate interactions with red blood cell count
        virtual void remove_RBCs(int64_t infectedAsexual, int64_t infectedGametocytes, double RBC_destruction_multiplier) override;

        virtual void UpdateIRBC( int64_t irbcFromInfection, int64_t irbcFromInfectionWithHRP ) override;

        virtual std::vector<MalariaAntibody>& GetAntibodiesForReporting( float currentTime, float dt, MalariaAntibodyType::Enum antibody_type ) override;

    protected:

        // Functions to enforce antigen-antibody reactions (e.g. stimulation, decay)
        void  updateImmunityCSP( float dt );
        void  updateImmunityMSP( float dt, float& temp_cytokine_stimulation );
        void  updateImmunityPfEMP1Minor( float dt );
        void  updateImmunityPfEMP1Major( float dt );
        void  recalculateBloodCapacity( float _age );
        void  countAntibodyVariations();

        // Clinical outcome calculations
        void  updateClinicalStates( float dt );
        void  ReportClinicalCase( ClinicalSymptomsEnum::Enum symptom, bool isNew );
        float get_severe_disease_probability( float dt, float& anemiaSevereFraction, float& parasiteSevereFraction, float& feverSevereFraction );
        float get_fatal_disease_probability(  float dt, float& anemiaFatalFraction,  float& parasiteFatalFraction,  float& feverFatalFraction );

        // Calculates total probability and adjusts fractional probabilities by cause
        float get_combined_probability ( 
            float dt, 
            float anemiaThreshold, float anemiaInvWidth,
            float parasiteThreshold, float parasiteInvWidth,
            float feverThreshold, float feverInvWidth,
            float& anemiaPartialProb, float& parasitePartialProb, float& feverPartialProb);

        void  UpdateMaximumSymptoms();
        void  UpdateDiagnosticVariables( float dt, int64_t totalIRBC, int64_t totalIRBCWithHRP );

        bool m_antigenic_flag;

        // containers for antibody objects
        float m_maternal_antibody_strength;
        MalariaAntibody m_CSP_antibody;
        std::vector<MalariaAntibody> m_MSP_antibodies;
        std::vector<MalariaAntibody> m_PfEMP1_minor_antibodies;
        std::vector<MalariaAntibody> m_PfEMP1_major_antibodies;

        std::vector<MalariaAntibody*> m_active_MSP_antibodies;
        std::vector<MalariaAntibody*> m_active_PfEMP1_minor_antibodies;
        std::vector<MalariaAntibody*> m_active_PfEMP1_major_antibodies;

        // RBC information
        int64_t m_RBC;
        int64_t m_RBCcapacity;
        int64_t m_RBCproduction;   // how many RBC's a person should have /120 (AVERAGE_RBC_LIFESPAN)
        float   m_inv_microliters_blood; // ==/(age dependent estimate of blood volume)
        int64_t m_TotalIRBC;
        int64_t m_TotalIRBCWithHRP;

        // symptomatic variables
        float m_cytokines;
        float m_ind_pyrogenic_threshold;
        float m_ind_fever_kill_rate;
        float m_cytokine_stimulation;
        float m_parasite_density;
        float m_PfHRP2_pg; //picograms
        std::vector<int> m_antibodies_to_n_variations; // how many variations of each antigen type does an individual have antibodies to

        // maxima over sub-time-step updates.  reset every time step.
        float m_max_fever_in_tstep; 
        float m_max_parasite_density_in_tstep;
        SevereCaseTypesEnum::Enum severetype;

        // state variables for whether the individual is in clinical disease, severe disease, or dead
        float  cumulative_days_of_clinical_incident;
        float  cumulative_days_of_severe_incident;
        float  cumulative_days_of_severe_anemia_incident;
        float  days_between_incidents;

        DECLARE_SERIALIZABLE(SusceptibilityMalaria);

    private:

        SusceptibilityMalaria();
        SusceptibilityMalaria(IIndividualHumanContext *context);
        virtual void Initialize(float _age, float immmod, float riskmod) override;
    };
}
