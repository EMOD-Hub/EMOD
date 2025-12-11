
#include "stdafx.h"

#include "SusceptibilityMalaria.h"
#include "Exceptions.h"
#include "Debug.h"
#include "Environment.h"
#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h" // for Die() interface
#include "InterventionsContainer.h"
#include "MalariaAntibody.h" // for CreateAntibody etc.
#include "Sigmoid.h"   // for Sigmoid::basic_sigmoid
#include "NodeDemographics.h"
#include "MalariaContexts.h"
#include "NodeEventContext.h"
#include "Infection.h" // for InfectionConfig::vital_disease_mortality 
#include "RANDOM.h"
#include "INodeContext.h"
#include "DistributionFactory.h"
#include "IdmDateTime.h"
#include "Individual.h"
#include "JsonFullWriter.h"
#include "JsonFullReader.h"

#include "Common.h"
#include "Malaria.h"
#include "Log.h"

SETUP_LOGGING( "SusceptibilityMalaria" )

namespace Kernel
{
    bool   SusceptibilityMalariaConfig::enable_maternal_antibodies_transmission  = false;
    InnateImmuneVariationType::Enum SusceptibilityMalariaConfig::innate_immune_variation_type = InnateImmuneVariationType::NONE;
    float  SusceptibilityMalariaConfig::base_gametocyte_mosquito_survival = 1.0f;
    float  SusceptibilityMalariaConfig::cytokine_gametocyte_inactivation  = 1.0f;
    float  SusceptibilityMalariaConfig::anemiaSevereLevel                 = 0.0f;
    float  SusceptibilityMalariaConfig::parasiteSevereLevel               = 0.0f;
    float  SusceptibilityMalariaConfig::parasiteSevereInvWidth            = 0.0f;
    float  SusceptibilityMalariaConfig::anemiaSevereInvWidth              = 0.0f;
    float  SusceptibilityMalariaConfig::feverSevereLevel                  = 0.0f;
    float  SusceptibilityMalariaConfig::feverSevereInvWidth               = 0.0f;
    float  SusceptibilityMalariaConfig::anemiaMortalityInvWidth           = 0.0f;
    float  SusceptibilityMalariaConfig::parasiteMortalityInvWidth         = 0.0f;
    float  SusceptibilityMalariaConfig::anemiaMortalityLevel              = 0.0f;
    float  SusceptibilityMalariaConfig::parasiteMortalityLevel            = 0.0f;
    float  SusceptibilityMalariaConfig::feverMortalityInvWidth            = 0.0f;
    float  SusceptibilityMalariaConfig::feverMortalityLevel               = 0.0f;
    float  SusceptibilityMalariaConfig::clinicalFeverThreshold_high       = 0.0f;
    float  SusceptibilityMalariaConfig::minDaysBetweenClinicalIncidents   = 0.0f;
    float  SusceptibilityMalariaConfig::clinicalFeverThreshold_low        = 0.0f;
    float  SusceptibilityMalariaConfig::memory_level                      = 0.0f;
    float  SusceptibilityMalariaConfig::hyperimmune_decay_rate            = 0.0f;
    float  SusceptibilityMalariaConfig::MSP1_antibody_growthrate          = 0.0f;
    float  SusceptibilityMalariaConfig::antibody_stimulation_c50          = 0.0f;
    float  SusceptibilityMalariaConfig::antibody_capacity_growthrate      = 0.0f;
    float  SusceptibilityMalariaConfig::antibody_days_to_long_term_decay  = 0.0f;
    float  SusceptibilityMalariaConfig::antibody_long_term_decay_days     = 0.0f;
    float  SusceptibilityMalariaConfig::minimum_adapted_response          = 0.0f;
    float  SusceptibilityMalariaConfig::non_specific_growth               = 0.0f;
    float  SusceptibilityMalariaConfig::antibody_csp_decay_days           = 0.0f;
    MaternalAntibodiesType::Enum SusceptibilityMalariaConfig::maternal_antibodies_type = MaternalAntibodiesType::OFF;
    float  SusceptibilityMalariaConfig::maternal_antibody_protection      = 0.0f;
    float  SusceptibilityMalariaConfig::maternal_antibody_decay_rate      = 0.0f;
    float  SusceptibilityMalariaConfig::erythropoiesis_anemia_effect      = 0.0f;
    float  SusceptibilityMalariaConfig::pyrogenic_threshold               = 0.0f;
    float  SusceptibilityMalariaConfig::fever_IRBC_killrate               = 0.0f;
    float  SusceptibilityMalariaConfig::PfHRP2_boost_rate                 = 0.0f;
    float  SusceptibilityMalariaConfig::PfHRP2_decay_rate                 = 0.0f;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! These values are configured in SimulationMalaria so that they are configured
    // !!! before ParasiteGenetics.  They are initialized to -1 so that SimulationMalaria
    // !!! can tell if these parameters have been configured by serialization.
    // !!! If they have been, then we don't want to change them because serialized
    // !!! m_XXX_antibodies parameters are all expected to be dimensioned
    // !!! with these parameters.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    int    SusceptibilityMalariaConfig::falciparumMSPVars                 = -1;
    int    SusceptibilityMalariaConfig::falciparumNonSpecTypes            = -1;
    int    SusceptibilityMalariaConfig::falciparumPfEMP1Vars              = -1;


    GET_SCHEMA_STATIC_WRAPPER_IMPL(Malaria.Susceptibility,SusceptibilityMalariaConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityMalariaConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityMalariaConfig)

    bool
    SusceptibilityMalariaConfig::Configure(
        const Configuration* config
    )
    {
        SusceptibilityConfig::enable_initial_susceptibility_distribution = false;
        SusceptibilityConfig::enable_immune_decay = false;

        initConfigTypeMap( "Anemia_Mortality_Threshold",              &anemiaMortalityLevel,              Anemia_Mortality_Threshold_DESC_TEXT,              0.0, 100.0f,      DEFAULT_ANEMIA_MORTALITY_LEVEL );
        initConfigTypeMap( "Parasite_Mortality_Threshold",            &parasiteMortalityLevel,            Parasite_Mortality_Threshold_DESC_TEXT,            0.0, FLT_MAX,     DEFAULT_PARASITE_MORTALITY_LEVEL );
        initConfigTypeMap( "Fever_Mortality_Threshold",               &feverMortalityLevel,               Fever_Mortality_Threshold_DESC_TEXT,               0.0, 1000.0,      DEFAULT_FEVER_MORTALITY_LEVEL );
        initConfigTypeMap( "Base_Gametocyte_Mosquito_Survival_Rate",  &base_gametocyte_mosquito_survival, Base_Gametocyte_Mosquito_Survival_Rate_DESC_TEXT,  0.0f, 1.0f,       DEFAULT_BASE_GAMETOCYTE_MOSQUITO_SURVIVAL );
        initConfigTypeMap( "Cytokine_Gametocyte_Inactivation",        &cytokine_gametocyte_inactivation,  Cytokine_Gametocyte_Inactivation_DESC_TEXT,        0.0f, 1.0f,       DEFAULT_CYTOKINE_GAMETOCYTE_INACTIVATION );
        initConfigTypeMap( "Anemia_Mortality_Inverse_Width",          &anemiaMortalityInvWidth,           Anemia_Mortality_Inverse_Width_DESC_TEXT,          0.1f, 1000000.0f, DEFAULT_ANEMIA_MORTALITY_INV_WIDTH );
        initConfigTypeMap( "Parasite_Mortality_Inverse_Width",        &parasiteMortalityInvWidth,         Parasite_Mortality_Inverse_Width_DESC_TEXT,        0.1f, 1000000.0f, DEFAULT_PARASITE_MORTALITY_INV_WIDTH );
        initConfigTypeMap( "Fever_Mortality_Inverse_Width",           &feverMortalityInvWidth,            Fever_Mortality_Inverse_Width_DESC_TEXT,           0.1f, 1000000.0f, DEFAULT_FEVER_MORTALITY_INV_WIDTH );
        initConfigTypeMap( "Anemia_Severe_Threshold",                 &anemiaSevereLevel,                 Anemia_Severe_Threshold_DESC_TEXT,                 0.0f, 100.0f,     DEFAULT_ANEMIA_SEVERE_LEVEL );
        initConfigTypeMap( "Parasite_Severe_Threshold",               &parasiteSevereLevel,               Parasite_Severe_Threshold_DESC_TEXT,               0.0f, FLT_MAX,    DEFAULT_PARASITE_SEVERE_LEVEL );
        initConfigTypeMap( "Fever_Severe_Threshold",                  &feverSevereLevel,                  Fever_Severe_Threshold_DESC_TEXT,                  0.0f, 10.0f,      DEFAULT_FEVER_SEVERE_LEVEL );
        initConfigTypeMap( "Anemia_Severe_Inverse_Width",             &anemiaSevereInvWidth,              Anemia_Severe_Inverse_Width_DESC_TEXT,             0.1f, 1000000.0f, DEFAULT_ANEMIA_SEVERE_INV_WIDTH );
        initConfigTypeMap( "Parasite_Severe_Inverse_Width",           &parasiteSevereInvWidth,            Parasite_Severe_Inverse_Width_DESC_TEXT,           0.1f, 1000000.0f, DEFAULT_PARASITE_SEVERE_INV_WIDTH );
        initConfigTypeMap( "Fever_Severe_Inverse_Width",              &feverSevereInvWidth,               Fever_Severe_Inverse_Width_DESC_TEXT,              0.1f, 1000000.0f, DEFAULT_FEVER_SEVERE_INV_WIDTH );
        initConfigTypeMap( "Clinical_Fever_Threshold_Low",            &clinicalFeverThreshold_low,        Clinical_Fever_Threshold_Low_DESC_TEXT,            0.0f, 5.0f,       DEFAULT_CLINICAL_FEVER_THRESHOLD_LOW );
        initConfigTypeMap( "Clinical_Fever_Threshold_High",           &clinicalFeverThreshold_high,       Clinical_Fever_Threshold_High_DESC_TEXT,           0.0f, 15.0f,      DEFAULT_CLINICAL_FEVER_THRESHOLD_HIGH );
        initConfigTypeMap( "Min_Days_Between_Clinical_Incidents",     &minDaysBetweenClinicalIncidents,   Min_Days_Between_Clinical_Incidents_DESC_TEXT,     0.0f, 1000000.0f, DEFAULT_MIN_DAYS_BETWEEN_INCIDENTS );
        initConfigTypeMap( "Antibody_Memory_Level",                   &memory_level,                      Antibody_Memory_Level_DESC_TEXT,                   0.0f, 0.35f,       0.2f );
        initConfigTypeMap( "Max_MSP1_Antibody_Growthrate",            &MSP1_antibody_growthrate,          Max_MSP1_Antibody_Growthrate_DESC_TEXT,            0.0f, 1.0f,       0.02f );
        initConfigTypeMap( "Antibody_Stimulation_C50",                &antibody_stimulation_c50,          Antibody_Stimulation_C50_DESC_TEXT,                0.1f, 10000.0f,   10.0f );
        initConfigTypeMap( "Antibody_Capacity_Growth_Rate",           &antibody_capacity_growthrate,      Antibody_Capacity_Growth_Rate_DESC_TEXT,           0.0f, 1.0f,       0.1f );
        initConfigTypeMap( "Min_Adapted_Response",                    &minimum_adapted_response,          Min_Adapted_Response_DESC_TEXT,                    0.0f, 1.0f,       0.02f );
        initConfigTypeMap( "Nonspecific_Antibody_Growth_Rate_Factor", &non_specific_growth,               Nonspecific_Antibody_Growth_Rate_Factor_DESC_TEXT, 0.0f, 1000.0f,    0.5f );
        initConfigTypeMap( "Antibody_CSP_Decay_Days",                 &antibody_csp_decay_days,           Antibody_CSP_Decay_Days_DESC_TEXT,                 1.0f, FLT_MAX,    DEFAULT_ANTIBODY_CSP_DECAY_DAYS );
        initConfigTypeMap( "Erythropoiesis_Anemia_Effect",            &erythropoiesis_anemia_effect,      Erythropoiesis_Anemia_Effect_DESC_TEXT,            0.0f, 1000.0f,    3.5f );
        initConfigTypeMap( "PfHRP2_Boost_Rate",                       &PfHRP2_boost_rate,                 PfHRP2_Boost_Rate_DESC_TEXT,                       0.0f, 1000.0f,    0.07f );
        initConfigTypeMap( "PfHRP2_Decay_Rate",                       &PfHRP2_decay_rate,                 PfHRP2_Decay_Rate_DESC_TEXT,                       0.0f, 1.0f,       0.172f );

        initConfigTypeMap( "Antibody_Days_To_Long_Term_Decay",        &antibody_days_to_long_term_decay,  Antibody_Days_To_Long_Term_Decay_DESC_TEXT,        1.0f, FLT_MAX,    FLT_MAX );
        initConfigTypeMap( "Antibody_Long_Term_Decay_Days",           &antibody_long_term_decay_days,     Antibody_Long_Term_Decay_Days_DESC_TEXT,           1.0f, FLT_MAX,    3650 );
        initConfigTypeMap("Enable_Maternal_Antibodies_Transmission",  &enable_maternal_antibodies_transmission, Enable_Maternal_Antibodies_Transmission_DESC_TEXT, false, "Simulation_Type","MALARIA_SIM");
        initConfig( "Maternal_Antibodies_Type", maternal_antibodies_type, config, MetadataDescriptor::Enum("maternal_antibodies_type", Maternal_Antibodies_Type_DESC_TEXT, MDD_ENUM_ARGS(MaternalAntibodiesType)), "Enable_Maternal_Antibodies_Transmission" );
        initConfigTypeMap( "Maternal_Antibody_Protection",        &maternal_antibody_protection,      Maternal_Antibody_Protection_DESC_TEXT,            0.0f, 1.0f,       0.1f, "Maternal_Antibodies_Type", "SIMPLE_WANING,CONSTANT_INITIAL_IMMUNITY" );
        initConfigTypeMap( "Maternal_Antibody_Decay_Rate",        &maternal_antibody_decay_rate,      Maternal_Antibody_Decay_Rate_DESC_TEXT,            0.0f, FLT_MAX,    0.01f, "Maternal_Antibodies_Type", "SIMPLE_WANING,CONSTANT_INITIAL_IMMUNITY" );

        initConfig( "Innate_Immune_Variation_Type", innate_immune_variation_type, config, MetadataDescriptor::Enum("innate_immune_variation_type", Innate_Immune_Variation_Type_DESC_TEXT, MDD_ENUM_ARGS(InnateImmuneVariationType)) );
        initConfigTypeMap( "Pyrogenic_Threshold", &pyrogenic_threshold, Pyrogenic_Threshold_DESC_TEXT, 0.1f, 20000.0f, 1000.0f );
        initConfigTypeMap( "Fever_IRBC_Kill_Rate",  &fever_IRBC_killrate, Fever_IRBC_Kill_Rate_DESC_TEXT, 0.0f, 1000.0f, DEFAULT_FEVER_IRBC_KILL_RATE );

        bool configured = JsonConfigurable::Configure( config );

        if( configured )
        {
            // JPS: Since the hyperimmunity decay time is set based on memory level,
            // memory level has a max of 0.35 to keep 0.4-memory_level from getting too close to zero
            // This sets the decay rate towards memory level so that the decay from antibody levels of 1 to levels of 0.4 is consistent
            hyperimmune_decay_rate = -log((0.4f - memory_level) / (1.0f - memory_level)) / 120.0f;
            // this is part of the new intrahost model explained in the Intrahost paper by Eckhoff
        }

        return configured;
    }

    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(SusceptibilityMalaria, SusceptibilityVector)
        HANDLE_INTERFACE(IMalariaSusceptibility)
    END_QUERY_INTERFACE_DERIVED(SusceptibilityMalaria, SusceptibilityVector)

    SusceptibilityMalaria::SusceptibilityMalaria()
        : SusceptibilityVector(),
        m_antigenic_flag(false),
        m_maternal_antibody_strength(0),
        m_CSP_antibody( MalariaAntibody::CreateAntibody( MalariaAntibodyType::CSP, 0 ) ),
        m_MSP_antibodies(),
        m_PfEMP1_minor_antibodies(),
        m_PfEMP1_major_antibodies(),
        m_active_MSP_antibodies(),
        m_active_PfEMP1_minor_antibodies(),
        m_active_PfEMP1_major_antibodies(),
        m_RBC(0),
        m_RBCcapacity(0),
        m_RBCproduction(0),
        m_inv_microliters_blood(0),
        m_TotalIRBC(0),
        m_TotalIRBCWithHRP(0),
        m_cytokines(0.0f),
        m_ind_pyrogenic_threshold(0.0f),
        m_ind_fever_kill_rate(0.0f),
        m_cytokine_stimulation(0.0f),
        m_parasite_density(0.0f),
        m_PfHRP2_pg( 0 ),
        m_antibodies_to_n_variations( MalariaAntibodyType::N_MALARIA_ANTIBODY_TYPES ),
        m_max_fever_in_tstep(0.0f),
        m_max_parasite_density_in_tstep(0.0f),
        severetype(SevereCaseTypesEnum::NONE),
        cumulative_days_of_clinical_incident(0.0f),
        cumulative_days_of_severe_incident(0.0f),
        cumulative_days_of_severe_anemia_incident(0.0f),
        days_between_incidents(0.0f)
    {
        m_MSP_antibodies.reserve(          SusceptibilityMalariaConfig::falciparumMSPVars );
        m_PfEMP1_minor_antibodies.reserve( SusceptibilityMalariaConfig::falciparumNonSpecTypes * MINOR_EPITOPE_VARS_PER_SET );
        m_PfEMP1_major_antibodies.reserve( SusceptibilityMalariaConfig::falciparumPfEMP1Vars );
    }

    SusceptibilityMalaria::SusceptibilityMalaria(IIndividualHumanContext *context)
        : SusceptibilityVector(context),
        m_antigenic_flag(false),
        m_maternal_antibody_strength(0),
        m_CSP_antibody( MalariaAntibody::CreateAntibody( MalariaAntibodyType::CSP, 0 ) ),
        m_MSP_antibodies(),
        m_PfEMP1_minor_antibodies(),
        m_PfEMP1_major_antibodies(),
        m_active_MSP_antibodies(),
        m_active_PfEMP1_minor_antibodies(),
        m_active_PfEMP1_major_antibodies(),
        m_RBC(0),
        m_RBCcapacity(0),
        m_RBCproduction(0),
        m_inv_microliters_blood(0),
        m_TotalIRBC(0),
        m_TotalIRBCWithHRP(0),
        m_cytokines(0.0f),
        m_ind_pyrogenic_threshold(0.0f),
        m_ind_fever_kill_rate(0.0f),
        m_cytokine_stimulation(0.0f),
        m_parasite_density(0.0f),
        m_PfHRP2_pg( 0 ),
        m_antibodies_to_n_variations(MalariaAntibodyType::N_MALARIA_ANTIBODY_TYPES),
        m_max_fever_in_tstep(0.0f),
        m_max_parasite_density_in_tstep(0.0f),
        severetype(SevereCaseTypesEnum::NONE),
        cumulative_days_of_clinical_incident(0.0f),
        cumulative_days_of_severe_incident(0.0f),
        cumulative_days_of_severe_anemia_incident(0.0f),
        days_between_incidents(0.0f)
    {
        m_MSP_antibodies.reserve(          SusceptibilityMalariaConfig::falciparumMSPVars );
        m_PfEMP1_minor_antibodies.reserve( SusceptibilityMalariaConfig::falciparumNonSpecTypes * MINOR_EPITOPE_VARS_PER_SET );
        m_PfEMP1_major_antibodies.reserve( SusceptibilityMalariaConfig::falciparumPfEMP1Vars );
    }

    void SusceptibilityMalaria::Initialize(float _age, float immmod, float riskmod)
    {
        SusceptibilityVector::Initialize(_age, immmod, riskmod);

        // how many RBC's a person should have determined by age
        if (_age > (20 * DAYSPERYEAR))
        {
            // This initializes the daily production of red blood cells for adults to maintain standard equilibrium RBC concentrations given RBC lifetime
            // N.B. Heavily caveated, because not all adults are same size.  However, this keeps consistent equilibrium RBC densities, allowing study of anemia, etc...
            m_RBCproduction = ADULT_RBC_PRODUCTION;// 2.0*10^11 (RBCs/day)*(120 days) = 2.4x10^13 RBCs ~= 5 liters * 5x10^6 RBCs/microliter

            // This equation tracks the blood volume separately from the equilibrium number of RBCs, allowing for non-constant RBC concentration.
            // m_inv_microliters_blood is 1/(microliters of blood in body)--this allows easy calculation of pathogen densities from pathogen numbers
            m_inv_microliters_blood = float(1 / ( (0.225 * (7300/DAYSPERYEAR) + 0.5) * 1e6 )); // 5 liters
        }
        else
        {
            // Initializes daily production of red blood cells for children to grow linearly from INFANT_RBC_PRODUCTION to ADULT_RBC_PRODUCTION by age 20
            // Only approximate due to linear increase in blood volume from 0.5 to 5 liters from birth to age 20 years.  Non-linear growth model might be better...
            m_RBCproduction = int64_t(INFANT_RBC_PRODUCTION + (_age * 0.000137) * (ADULT_RBC_PRODUCTION - INFANT_RBC_PRODUCTION)); //*.000137==/7300 (linear growth to age 20)
            m_inv_microliters_blood = float(1 / ( (0.225 * (_age/DAYSPERYEAR) + 0.5 ) * 1e6)); // linear growth from 0.5 liters at birth to 5 liters at age 20
        }

        m_RBCcapacity = m_RBCproduction * AVERAGE_RBC_LIFESPAN;  // Health equilibrium of RBC is production*lifetime.  This is the total number of RBC per human
        m_RBC         = m_RBCcapacity;

        // Set up variable pyrogenic thresholds + fever killing rates
        m_ind_pyrogenic_threshold = SusceptibilityMalariaConfig::pyrogenic_threshold;
        m_ind_fever_kill_rate = SusceptibilityMalariaConfig::fever_IRBC_killrate;

        float variation_modifier = 1.0;
        if( (SusceptibilityMalariaConfig::innate_immune_variation_type == InnateImmuneVariationType::CYTOKINE_KILLING) ||
            (SusceptibilityMalariaConfig::innate_immune_variation_type == InnateImmuneVariationType::PYROGENIC_THRESHOLD) )
        {
            // ------------------------------------------------------------------------------------
            // --- NOTE:  This is not very performant, but it is nice that all of the code for this
            // --- feature is located in spot.
            // ------------------------------------------------------------------------------------
            const NodeDemographics& r_demographics = parent->GetEventContext()->GetNodeEventContext()->GetDemographics();

            DistributionFunction::Enum innate_immune_dist_type = DistributionFunction::Enum( r_demographics[ "IndividualAttributes" ][ "InnateImmuneDistributionFlag" ].AsInt() );
            float innate_immune_dist1 = float( r_demographics[ "IndividualAttributes" ][ "InnateImmuneDistribution1" ].AsDouble() );
            float innate_immune_dist2 = float( r_demographics[ "IndividualAttributes" ][ "InnateImmuneDistribution2" ].AsDouble() );

            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( innate_immune_dist_type ) );
            distribution->SetParameters( innate_immune_dist1, innate_immune_dist2, 0.0 );

            variation_modifier = distribution->Calculate( parent->GetRng() );
        }

        switch( SusceptibilityMalariaConfig::innate_immune_variation_type )
        {
            case InnateImmuneVariationType::NONE:
                // no additional variation
                break;

            case InnateImmuneVariationType::PYROGENIC_THRESHOLD:
                m_ind_pyrogenic_threshold *= variation_modifier;
                break;

            case InnateImmuneVariationType::CYTOKINE_KILLING:
                m_ind_fever_kill_rate *= variation_modifier;
                break;

            case InnateImmuneVariationType::PYROGENIC_THRESHOLD_VS_AGE:
                // Roucher et al., Changing Malaria Epidemiology and Diagnostic Criteria for Plasmodium falciparum Clinical Malaria, PLoS One, 2012; 7(9): e46188
                // KM:10/15/2013 - TODO: Promote these function parameters to config variables: 
                //                 age of max threshold, max threshold, min threshold, threshold at 0, exponential decay rate
                if (_age < (2 * DAYSPERYEAR))
                {
                    m_ind_pyrogenic_threshold = 15000 + 500*_age/DAYSPERYEAR;
                }
                else
                {
                    m_ind_pyrogenic_threshold = 14500 * exp(-.09*(_age/DAYSPERYEAR - 2)) + 1500;
                }
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "innate_immune_variation_type", SusceptibilityMalariaConfig::innate_immune_variation_type,
                                                         InnateImmuneVariationType::pairs::lookup_key( SusceptibilityMalariaConfig::innate_immune_variation_type ) );
        }

        LOG_DEBUG_F("Individual pyrogenic threshold = %0.2f\n", m_ind_pyrogenic_threshold);
        LOG_DEBUG_F("Individual maximum fever killing rate = %0.2f\n", m_ind_fever_kill_rate);

        m_CSP_antibody = MalariaAntibody::CreateAntibody( MalariaAntibodyType::CSP, 0 );
    }

    SusceptibilityMalaria *SusceptibilityMalaria::CreateSusceptibility(IIndividualHumanContext *context, float _age, float immmod, float riskmod)
    {
        SusceptibilityMalaria *newsusceptibility = _new_ SusceptibilityMalaria(context);
        release_assert(newsusceptibility);
        newsusceptibility->Initialize(_age, immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilityMalaria::~SusceptibilityMalaria()
    {
    }

    void SusceptibilityMalaria::Update(float dt)
    {
        release_assert( params() );

        age += dt; // age in days
        m_age_dependent_biting_risk = BitingRiskAgeFactor(age);
        recalculateBloodCapacity(age);

        // Red blood cell dynamics
        if (SusceptibilityMalariaConfig::erythropoiesis_anemia_effect > 0)
        {
            // This is the amount of "erythropoietin", assume absolute amounts of erythropoietin correlate linearly with absolute increases in hemoglobin
            float anemia_erythropoiesis_multiplier = exp( SusceptibilityMalariaConfig::erythropoiesis_anemia_effect * (1 - get_RBC_availability()) );
            LOG_VALID_F( "Anemia erythropoiesis multiplier = %f at RBC availability of %f.\n", anemia_erythropoiesis_multiplier, get_RBC_availability() );
            m_RBC = int64_t(m_RBC - (m_RBC * .00833 - m_RBCproduction * anemia_erythropoiesis_multiplier) * dt); // *.00833 ==/120 (AVERAGE_RBC_LIFESPAN)
        }
        else
        {
            m_RBC = int64_t(m_RBC - (m_RBC * .00833 - m_RBCproduction) * dt); // *.00833 ==/120 (AVERAGE_RBC_LIFESPAN)
        }
        LOG_VALID_F( "I have %lld red blood cells\n", m_RBC );

        // Cytokines decay with time constant of 12 hours
        m_cytokines -= (m_cytokines * 2 * dt);
        if (m_cytokines < 0) { m_cytokines = 0; }

        LOG_VALID_F( "fever = %0.9f  cytokines = %0.9f\n", get_fever(), m_cytokines );

        // Reset parasite density
        m_parasite_density = 0; // this is accumulated in updateImmunityPfEMP1Minor

        // decay maternal antibodies
        m_maternal_antibody_strength -= dt * m_maternal_antibody_strength * SusceptibilityMalariaConfig::maternal_antibody_decay_rate;
        if ( m_maternal_antibody_strength < 0 ) { m_maternal_antibody_strength = 0; }

        // antibody capacities increase and antibodies released if antigen present, only process if antigens are present at all
        // concept of antibody stimulation threshold seen in other models--(Molineaux, Diebner et al. 2001; Paget-McNicol, Gatton et al. 2002; Dietz, Raddatz et al. 2006)
        // first CSP, then rest (but only process rest if there is an active infection), have to process CSP every time step
        updateImmunityCSP(dt);

        // number of antibody variants the immune system can produce
        countAntibodyVariations();

        // now all other antigens
        if ( m_antigenic_flag )
        {
            // Update antigen-antibody reactions for MSP and PfEMP1 minor/major epitopes, including cytokine stimulation
            float temp_cytokine_stimulation = 0; // used to track total stimulation of cytokines due to rupturing schizonts
            LOG_VALID("update MSP\n");
            updateImmunityMSP(dt, temp_cytokine_stimulation);
            LOG_VALID("update PfEMP1 minor\n");
            updateImmunityPfEMP1Minor(dt);
            LOG_VALID("update PfEMP1 major\n");
            updateImmunityPfEMP1Major(dt);

            LOG_VALID_F( "cytokine stimulation: %0.9f (MSP)   %0.9f (PfEMP)\n", temp_cytokine_stimulation, m_cytokine_stimulation );

            // inflammatory immune response--Stevenson, M. M. and E. M. Riley (2004). "Innate immunity to malaria." Nat Rev Immunol 4(3): 169-180.
            // now let cytokine be increased in response to IRBCs and ruptured schizonts, if any
            // pyrogenic threshold similar to previous models--(Molineaux, Diebner et al. 2001; Paget-McNicol, Gatton et al. 2002; Maire, Smith et al. 2006)
            m_cytokines = float(m_cytokines + CYTOKINE_STIMULATION_SCALE * Sigmoid::basic_sigmoid(m_ind_pyrogenic_threshold, m_cytokine_stimulation) * dt * 2);//12-hour time constant
            m_cytokines = float(m_cytokines + CYTOKINE_STIMULATION_SCALE * Sigmoid::basic_sigmoid(m_ind_pyrogenic_threshold, temp_cytokine_stimulation));//one time spike for rupturing schizonts
            m_cytokine_stimulation = 0; // and reset for next time step

            LOG_VALID_F( "m_cytokines: %0.9f\n", m_cytokines );

            // reset antigenic presence and IRBC counters
            m_antigenic_flag = false;
            for( auto p_antibody : m_active_MSP_antibodies )
            {
                p_antibody->ResetCounters();
            }

            for( auto p_antibody : m_active_PfEMP1_minor_antibodies )
            {
                p_antibody->ResetCounters();
            }

            for( auto p_antibody : m_active_PfEMP1_major_antibodies )
            {
                p_antibody->ResetCounters();
            }
        }

        UpdateDiagnosticVariables( dt, m_TotalIRBC, m_TotalIRBCWithHRP );

        // Determine the disease state (symptomatic, severe, or fatal) of current clinical incident
        updateClinicalStates(dt);

        m_TotalIRBC = 0; // clear so can be updated when the infections are
        m_TotalIRBCWithHRP = 0;

        m_active_MSP_antibodies.clear();
        m_active_PfEMP1_major_antibodies.clear();
        m_active_PfEMP1_minor_antibodies.clear();

        LOG_VALID( "END Update\n" );
    }

    void SusceptibilityMalaria::UpdateDiagnosticVariables( float dt, int64_t totalIRBC, int64_t totalIRBCWithHRP )
    {
        // --------------------------------------------------------------------------------------
        // --- "Modelling the dynamics of Plasmodium falciparum histidine-rich protein 2
        // --- in human malaria to better understand malaria rapid diagnostic test performance"
        // --- by Louise Marquart, Alice Butterworth, James S McCarthy and Michelle L Gatton
        // --- https://malariajournal.biomedcentral.com/articles/10.1186/1475-2875-11-74
        // ---
        // --- NOTE: We are approximating the decay as linear.
        // --------------------------------------------------------------------------------------
        m_PfHRP2_pg -= m_PfHRP2_pg*SusceptibilityMalariaConfig::PfHRP2_decay_rate*dt;
        m_PfHRP2_pg += SusceptibilityMalariaConfig::PfHRP2_boost_rate * float( totalIRBCWithHRP ) * dt;

        // Calculcate parasite density
        m_parasite_density = float( totalIRBC ) * m_inv_microliters_blood;
    }

    void SusceptibilityMalaria::UpdateIRBC( int64_t irbcFromInfection, int64_t irbcFromInfectionWithHRP )
    {
        m_TotalIRBC += irbcFromInfection;
        m_TotalIRBCWithHRP += irbcFromInfectionWithHRP;
    }

    float SusceptibilityMalaria::GetPfHRP2() const
    {
        return m_PfHRP2_pg * m_inv_microliters_blood;
    }

    void SusceptibilityMalaria::UpdateInfectionCleared()
    {
        // nothing special to do in malaria immune model when infections are cleared
    }

    void SusceptibilityMalaria::updateImmunityCSP( float dt )
    {
        if( m_CSP_antibody.GetAntigenicPresence() )
        {
	        // Hyper-immune response (could potentially keep this as part of the update in ExposeToInfectivity)
	        if (m_CSP_antibody.GetAntibodyCapacity() > 0.4)
	        { 
	            m_CSP_antibody.UpdateAntibodyCapacityByRate( dt, 0.33f );
	        }
	
	        m_CSP_antibody.UpdateAntibodyConcentration( dt );
	    }
    }

    void SusceptibilityMalaria::updateImmunityMSP( float dt, float& temp_cytokine_stimulation )
    {
        // Merozoite-specific immunity
        // Blackman, M. J., H. G. Heidrich, et al. (1990).
        // "A single fragment of a malaria merozoite surface protein remains on the parasite during
        //  red cell invasion and is the target of invasion-inhibiting antibodies." 
        // J Exp Med 172(1): 379-382.

        for (auto p_antibody : m_active_MSP_antibodies)
        {
            LOG_VALID_F( "\tMSP (before): capacity = %0.9f, Ab = %0.9f,  antigen = %lld\n", p_antibody->GetAntibodyCapacity(), p_antibody->GetAntibodyConcentration(), p_antibody->GetAntigenCount() );

            // Temporary cytokines stimulated by spikes in MSP antigenic presence after schizont bursts
            temp_cytokine_stimulation += p_antibody->StimulateCytokines( dt, m_inv_microliters_blood );

            p_antibody->UpdateAntibodyCapacity( dt, m_inv_microliters_blood );
            p_antibody->UpdateAntibodyConcentration( dt );

            LOG_VALID_F( "\t    (after): capacity = %0.9f, Ab = %0.9f,  antigen = %lld\n", p_antibody->GetAntibodyCapacity(), p_antibody->GetAntibodyConcentration(), p_antibody->GetAntigenCount() );
        }
    }

    void SusceptibilityMalaria::updateImmunityPfEMP1Minor( float dt )
    {
        // Minor epitope IRBC antigens
        // Recker, M., S. Nee, et al. (2004). 
        // "Transient cross-reactive immune responses can orchestrate antigenic variation in malaria." 
        // Nature 429(6991): 555-558.

        for (auto p_antibody : m_active_PfEMP1_minor_antibodies)
        {
            LOG_VALID_F( "\tPfEMP1 minor (before): capacity = %0.9f, Ab = %0.9f,  antigen = %lld\n", p_antibody->GetAntibodyCapacity(), p_antibody->GetAntibodyConcentration(), p_antibody->GetAntigenCount() );

            p_antibody->UpdateAntibodyCapacity( dt, m_inv_microliters_blood );
            p_antibody->UpdateAntibodyConcentration( dt );

            LOG_VALID_F( "\t    (after): capacity = %0.9f, Ab = %0.9f,  antigen = %lld\n", p_antibody->GetAntibodyCapacity(), p_antibody->GetAntibodyConcentration(), p_antibody->GetAntigenCount() );
        }
    }

    void SusceptibilityMalaria::updateImmunityPfEMP1Major( float dt )
    {
        for (auto p_antibody : m_active_PfEMP1_major_antibodies)
        {
            LOG_VALID_F( "\tPfEMP1 major (before): capacity = %0.9f, Ab = %0.9f,  antigen = %lld\n", p_antibody->GetAntibodyCapacity(), p_antibody->GetAntibodyConcentration(), p_antibody->GetAntigenCount() );

            // Cytokines released at low antibody concentration (if capacity hasn't switched into high proliferation rate yet)
            if( p_antibody->GetAntibodyCapacity() <= 0.4 )
            {
                m_cytokine_stimulation += p_antibody->StimulateCytokines( dt, m_inv_microliters_blood );
            }

            p_antibody->UpdateAntibodyCapacity( dt, m_inv_microliters_blood );
            p_antibody->UpdateAntibodyConcentration( dt );
            LOG_VALID_F( "\t    (after): capacity = %0.9f, Ab = %0.9f,  antigen = %lld\n", p_antibody->GetAntibodyCapacity(), p_antibody->GetAntibodyConcentration(), p_antibody->GetAntigenCount() );
        }
    }

    void AddAntibody( std::vector<MalariaAntibody*>& antibody_collection, MalariaAntibody* pAntibody )
    {
        if( pAntibody->GetActiveIndex() < 0 )
        {
            antibody_collection.push_back( pAntibody );
            pAntibody->SetActiveIndex( antibody_collection.size() - 1 );
        }
    }

    void SusceptibilityMalaria::SetActiveAntibody( MalariaAntibody* pAntibody )
    {
        switch( pAntibody->GetAntibodyType() )
        {
            case MalariaAntibodyType::MSP1:
                AddAntibody( m_active_MSP_antibodies, pAntibody );
                break;

            case MalariaAntibodyType::PfEMP1_major:
                AddAntibody( m_active_PfEMP1_major_antibodies, pAntibody );
                break;

            case MalariaAntibodyType::PfEMP1_minor:
                AddAntibody( m_active_PfEMP1_minor_antibodies, pAntibody );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "pAntibody->GetAntibodyType()",
                                                         int(pAntibody->GetAntibodyType()),
                                                         MalariaAntibodyType::pairs::lookup_key(pAntibody->GetAntibodyType()) );
        }
        m_antigenic_flag = true;
    }

    void SusceptibilityMalaria::recalculateBloodCapacity( float _age )
    {
        // How many RBCs a person should have determined by age.
        // This sets the daily production of red blood cells for adults to maintain 
        // standard equilibrium RBC concentrations given RBC lifetime
        if ( _age > (20 * DAYSPERYEAR) )
        {
            // Update adults every year.  TODO: this presumes DAYSPERYEAR is a multiple of dt
            if ( int(_age) % DAYSPERYEAR == 0 )
            {
                // 2.0*10^11 (RBCs/day)*(120 days)=2.4x10^13 RBCs ~= 5 liters * 5x10^6 RBCs/microliter
                m_RBCproduction         = ADULT_RBC_PRODUCTION;
                m_inv_microliters_blood = float(1 / ( (0.225 * (7300/DAYSPERYEAR) + 0.5) * 1e6 )); 
                m_RBCcapacity           = m_RBCproduction * AVERAGE_RBC_LIFESPAN; // Health equilibrium of RBC is production*lifetime
            }
        }
        else
        {
            // Update children every day.
            // Sets daily production of red blood cells for children to set their equilibrium RBC concentrations and blood volume given an RBC lifetime
            // Only approximate due to linear increase in blood volume from 0.5 to 5 liters with age, a better growth model would be nonlinear
            m_RBCproduction         = int64_t(INFANT_RBC_PRODUCTION + (_age * .000137) * (ADULT_RBC_PRODUCTION - INFANT_RBC_PRODUCTION)); //*.000137==/(20*DAYSPERYEAR)
            m_inv_microliters_blood = float(1 / ( (0.225 * (_age/DAYSPERYEAR) + 0.5 ) * 1e6 )); 
            m_RBCcapacity           = m_RBCproduction * AVERAGE_RBC_LIFESPAN; // Health equilibrium of RBC is production*lifetime
        }
    }

    void SusceptibilityMalaria::countAntibodyVariations()
    {
        m_antibodies_to_n_variations[MalariaAntibodyType::CSP]          = ( m_CSP_antibody.GetAntibodyCapacity() > 0 ) ? 1 : 0;
        m_antibodies_to_n_variations[MalariaAntibodyType::MSP1]         = m_MSP_antibodies.size();
        m_antibodies_to_n_variations[MalariaAntibodyType::PfEMP1_minor] = m_PfEMP1_minor_antibodies.size();
        m_antibodies_to_n_variations[MalariaAntibodyType::PfEMP1_major] = m_PfEMP1_major_antibodies.size();
    }

    void SusceptibilityMalaria::updateClinicalStates( float dt )
    {
        // State counters are reset every sub-timestep in IndividualHuman::Update (see infection_updates_per_tstep).
        // So, instead of querying these states directly from the individuals in NodeMalaria::updatePopulationStatistics, 
        // this function will tell the IndividualHuman that there has been a state change (clinical, severe).
        // New and non-fatal clinical states are accumulated in NodeMalaria::updateNodeStateCounters from IndividualHumanMalaria::m_clinical_symptoms
        // This function sets the parent individual's HumanStateChange to KilledByInfection directly using the Die() interface

        UpdateMaximumSymptoms(); // max fever + parasitemia in this time step

        // partial probabilities in case we want to assign severe and fatal cases to a particular cause
        float anemiaFatalFraction = 1.0f, parasiteFatalFraction = 1.0f, feverFatalFraction = 1.0f;
        float anemiaSevereFraction = 1.0f, parasiteSevereFraction = 1.0f, feverSevereFraction = 1.0f;

        // get fever, severe probability, and fatal probability
        float current_fever = get_fever();
        float prob_severe   = get_severe_disease_probability(dt, anemiaSevereFraction, parasiteSevereFraction, feverSevereFraction);
        float prob_fatal    = get_fatal_disease_probability(dt, anemiaFatalFraction, parasiteFatalFraction, feverFatalFraction);

        // sanity check on inconsistently specified sigmoids
        if( InfectionConfig::enable_disease_mortality && (prob_fatal > prob_severe) )
        {
            LOG_WARN_F( "Probability of mortality (%f) exceeds probability of severe disease (%f)!\n", prob_fatal, prob_severe);
        }

        if ( current_fever > SusceptibilityMalariaConfig::clinicalFeverThreshold_low ) 
        {
            // reset unique-incident counter
            days_between_incidents = float(SusceptibilityMalariaConfig::minDaysBetweenClinicalIncidents);

            bool is_new_clinical =  (current_fever > SusceptibilityMalariaConfig::clinicalFeverThreshold_high)
                                 && (cumulative_days_of_clinical_incident == 0);
            bool continuing_symptoms_clinical = (cumulative_days_of_clinical_incident > 0);
            if( is_new_clinical || continuing_symptoms_clinical )
            {
                ReportClinicalCase( ClinicalSymptomsEnum::CLINICAL_DISEASE, is_new_clinical );
            }

            // check for new symptomatic case and accumulate days with fever
            if ( current_fever > SusceptibilityMalariaConfig::clinicalFeverThreshold_high )
            {
                cumulative_days_of_clinical_incident += dt;
            }
            else if ( prob_fatal < 0.01 * dt ) // is it possible to have severe disease or die without fever?  should this be enforced explicitly?
            {
                return; // require a fever (or non-trivial fatal probability) to check for severe disease and mortality
            }

            // In order to avoid the potential event of a fatal (but not severe) disease case,
            // use a single random number draw for how unlucky this individual is this time step.
            float rand = parent->GetRng()->e();

            bool is_new_severe =  (rand < prob_severe)
                               && (cumulative_days_of_severe_incident == 0);
            bool continuing_symptoms_severe = (cumulative_days_of_severe_incident > 0);
            if( is_new_severe || continuing_symptoms_severe )
            {
                ReportClinicalCase( ClinicalSymptomsEnum::SEVERE_DISEASE, is_new_severe );
            }

            // check for new severe case and accumulate days in this state
            if ( rand < prob_severe )
            {
                if ( cumulative_days_of_severe_incident == 0 ) 
                { 
                    // Here is where we can use the partial fractions to do some logging of severe cases by cause:
                    if      ( rand < ( prob_severe * anemiaSevereFraction ) ) { severetype = SevereCaseTypesEnum::ANEMIA; }
                    else if ( rand < ( prob_severe * ( anemiaSevereFraction + parasiteSevereFraction ) ) ) { severetype = SevereCaseTypesEnum::PARASITES; }
                    else if ( rand < ( prob_severe * ( anemiaSevereFraction + parasiteSevereFraction + feverSevereFraction ) ) ) { severetype = SevereCaseTypesEnum::FEVER; }
                    else { LOG_WARN("This new severe case cannot be attributed to a cause (e.g. fever) because the sum of the partial fractions by cause exceed unity!\n"); }

                    LOG_DEBUG_F("New SEVERE case in %0.1f-year old: type = %s (Hg=%0.1f, parasites=%3.2e, fever=%0.1f C)\n", age/DAYSPERYEAR, SevereCaseTypesEnum::pairs::lookup_key(severetype), GetHemoglobin(), m_parasite_density, 37+current_fever);
                }
                cumulative_days_of_severe_incident += dt;
            }

            bool is_new_anemia =  (GetHemoglobin() < 5)
                               && (cumulative_days_of_severe_anemia_incident == 0);
            bool continuing_symptoms_anemia = (cumulative_days_of_severe_anemia_incident > 0);
            if( is_new_anemia || continuing_symptoms_anemia )
            {
                ReportClinicalCase( ClinicalSymptomsEnum::SEVERE_ANEMIA, is_new_anemia );
            }

            // check for severe anemia case
            if ( GetHemoglobin() < 5 ) 
            {
                cumulative_days_of_severe_anemia_incident += dt;
            }

            if( InfectionConfig::enable_disease_mortality )
            {
                // To query for mortality-reducing effects of drugs or vaccines
                IDrugVaccineInterventionEffects* idvie = nullptr;
                if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                                   "parent->GetInterventionsContext()",
                                                   "IDrugVaccineInterventionEffects",
                                                   "IIndividualHumanInterventionsContext" );
                }
                if ( rand < prob_fatal * idvie->GetInterventionReducedMortality() )
                {
                    parent->GetEventContext()->Die( HumanStateChange::KilledByInfection ); // set the individual's HumanStateChange to KilledByInfection

                    // Here is where we can use the partial fractions to do some logging of deaths by cause, for example:
                    // if ( rand < ( prob_fatal * anemiaFatalFraction ) ) { /* death by anemia */ }
                    // else if ( rand < ( prob_fatal * ( anemiaFatalFraction + parasiteFatalFraction ) ) ) { /* death by parasites */ }
                    // else if ( rand < ( prob_fatal * ( anemiaFatalFraction + parasiteFatalFraction + feverFatalFraction ) ) ) { /* death by fever */ }
                    // else { /* ERROR: this person is supposed to have died */ }
                }
            }
        }
        else
        {
            if ( days_between_incidents <= dt ) 
            {
                // clinical symptoms have been gone for long enough to start a new clinical case next time symptoms appear.
                cumulative_days_of_clinical_incident        = 0;
                cumulative_days_of_severe_incident          = 0;
                cumulative_days_of_severe_anemia_incident   = 0;
                days_between_incidents                      = 0;
            }
            else
            {
                // clinical symptoms have recently subsided.  continue decrementing the days between incidents unless a new incident starts before it reaches zero
                days_between_incidents -= dt;
            }
        }
    }

    void  SusceptibilityMalaria::ReportClinicalCase( ClinicalSymptomsEnum::Enum symptom, bool isNew )
    {
        // for reporting symptoms (clinical, severe, anemia, etc.)
        IMalariaHumanContext *imhc = nullptr;
        if ( s_OK != parent->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&imhc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IMalariaHumanContext", "IIndividualHumanContext" );
        }
        imhc->AddClinicalSymptom( symptom, isNew );
    }

    float SusceptibilityMalaria::get_severe_disease_probability( float dt, float& anemiaSevereFraction, float& parasiteSevereFraction, float& feverSevereFraction )
    {
        // The severe sigmoids should not be narrower than the fatal sigmoids 
        // or else the severe probability may be lower than fatal in the tail of the distribution!
        float total_severe_prob = get_combined_probability(
            dt, SusceptibilityMalariaConfig::anemiaSevereLevel, SusceptibilityMalariaConfig::anemiaSevereInvWidth, 
            SusceptibilityMalariaConfig::parasiteSevereLevel, SusceptibilityMalariaConfig::parasiteSevereInvWidth, 
            SusceptibilityMalariaConfig::feverSevereLevel, SusceptibilityMalariaConfig::feverSevereInvWidth,
            anemiaSevereFraction, parasiteSevereFraction, feverSevereFraction);

        return total_severe_prob;
    }

    float SusceptibilityMalaria::get_fatal_disease_probability( float dt, float& anemiaFatalFraction, float& parasiteFatalFraction, float& feverFatalFraction )
    {
        float total_fatal_prob = get_combined_probability(
            dt, float(SusceptibilityMalariaConfig::anemiaMortalityLevel), SusceptibilityMalariaConfig::anemiaMortalityInvWidth, 
            float(SusceptibilityMalariaConfig::parasiteMortalityLevel), SusceptibilityMalariaConfig::parasiteMortalityInvWidth, 
            float(SusceptibilityMalariaConfig::feverMortalityLevel), SusceptibilityMalariaConfig::feverMortalityInvWidth,
            anemiaFatalFraction, parasiteFatalFraction, feverFatalFraction);

        return total_fatal_prob;
    }

    void SusceptibilityMalaria::BoostAntibody( MalariaAntibodyType::Enum type, int variant, float boosted_antibody_concentration )
    {
        switch( type )
        {
        case MalariaAntibodyType::CSP:
            m_CSP_antibody.SetAntibodyConcentration( boosted_antibody_concentration );
            m_CSP_antibody.SetAntibodyCapacity(1);
            break;
            
        default:
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only implemented antibody boosting for CSP so far." );
        }
    }

    void SusceptibilityMalaria::GetAntibodiesForReporting( std::vector<MalariaAntibody>& r_antibodies, float currentTime, float dt, MalariaAntibodyType::Enum antibody_type)
    {
        if(antibody_type == MalariaAntibodyType::MSP1)
        {
            r_antibodies = m_MSP_antibodies;
        }
        else if(antibody_type == MalariaAntibodyType::PfEMP1_major)
        {
            r_antibodies = m_PfEMP1_major_antibodies;
        }
        else
        {
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only implemented antibody reporting for MSP1 and PfEMP1 major so far." );
        }

        for(auto& antibody : r_antibodies)
        {
            antibody.IncreaseAntigenCount( 0, currentTime, dt );
        }

        // sort by variant for reporting
        std::sort(
            r_antibodies.begin(),
            r_antibodies.end(),
            []( const MalariaAntibody& a, const MalariaAntibody& b )
            {
                return a.GetAntibodyVariant() < b.GetAntibodyVariant();
            }
        );
    }

    MalariaAntibody* SusceptibilityMalaria::RegisterAntibody(MalariaAntibodyType::Enum type, int variant, float capacity)
    {
        std::vector<MalariaAntibody> *variant_vector;

        switch( type )
        {
        case MalariaAntibodyType::CSP:
            return &m_CSP_antibody; // only one CSP variant, so ignore second argument for now 

        case MalariaAntibodyType::MSP1:
            variant_vector = &m_MSP_antibodies;
            break;

        case MalariaAntibodyType::PfEMP1_minor:
            variant_vector = &m_PfEMP1_minor_antibodies;
            break;

        case MalariaAntibodyType::PfEMP1_major:
            variant_vector = &m_PfEMP1_major_antibodies;
            break;

        default:
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "type", int(type), MalariaAntibodyType::pairs::lookup_key(type) );
        }
        
        MalariaAntibody* antibody = nullptr;
        for( int i = 0; i < variant_vector->size(); ++i )
        {
            if ( (*variant_vector)[i].GetAntibodyVariant() == variant )
            {
                antibody = &(*variant_vector)[i];
                break;
            }
        }

        if (antibody == nullptr) // make a new antibody if it hasn't been created yet
        {
            MalariaAntibody tmp_antibody = MalariaAntibody::CreateAntibody( type, variant, capacity );

            variant_vector->push_back(tmp_antibody);
            antibody = &((*variant_vector)[ variant_vector->size() - 1 ]);
        }

        return antibody;
    }

    void SusceptibilityMalaria::UpdateActiveAntibody( pfemp1_antibody_t &pfemp1_variant, int minor_variant, int major_variant )
    {
        if(pfemp1_variant.minor == nullptr)
        {
            pfemp1_variant.minor = RegisterAntibody(MalariaAntibodyType::PfEMP1_minor, minor_variant);
        }

        if(pfemp1_variant.major == nullptr)
        {
            pfemp1_variant.major = RegisterAntibody(MalariaAntibodyType::PfEMP1_major, major_variant);
        }
    }

    float SusceptibilityMalaria::get_fraction_of_variants_with_antibodies(MalariaAntibodyType::Enum type) const
    {
        float fraction = 0.0f;
        switch( int(type) )
        {
        case MalariaAntibodyType::CSP:
            fraction = float(m_antibodies_to_n_variations[MalariaAntibodyType::CSP]);
            break;
            
        case MalariaAntibodyType::MSP1:
            fraction = float(m_antibodies_to_n_variations[MalariaAntibodyType::MSP1]) / SusceptibilityMalariaConfig::falciparumMSPVars;
            break;

        case MalariaAntibodyType::PfEMP1_minor:
            fraction = float(m_antibodies_to_n_variations[MalariaAntibodyType::PfEMP1_minor]) / (MINOR_EPITOPE_VARS_PER_SET*SusceptibilityMalariaConfig::falciparumNonSpecTypes); // five minor epitopes for each non-spec type
            break;

        case MalariaAntibodyType::PfEMP1_major:
            fraction = float(m_antibodies_to_n_variations[MalariaAntibodyType::PfEMP1_major]) / SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
            break;

        default:
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "type", type, MalariaAntibodyType::pairs::lookup_key( type ) );
        }

        return fraction;
    }

    float SusceptibilityMalaria::get_combined_probability (
            float dt, 
            float anemiaThreshold, float anemiaInvWidth,
            float parasiteThreshold, float parasiteInvWidth,
            float feverThreshold, float feverInvWidth,
            float& anemiaPartialProb, float& parasitePartialProb, float& feverPartialProb)
    {
        // get partial probabilities (anemia is flipped because *low* RBC availability is bad)
        float prob_anemia   = dt * ( 1.0f - Sigmoid::variableWidthSigmoid( GetHemoglobin(),     anemiaThreshold,    anemiaInvWidth ) );
        float prob_parasite = dt *          Sigmoid::variableWidthSigmoid( m_parasite_density,  parasiteThreshold,  parasiteInvWidth );
        float prob_fever    = dt *          Sigmoid::variableWidthSigmoid( get_fever(),         feverThreshold,     feverInvWidth );

        // total probability of something happening is one minus the product of each event not occurring
        float total_prob = 1.0f - ( (1.0f - prob_anemia) * (1.0f - prob_parasite) * (1.0f - prob_fever) );

        // adjust partial probabilities passed in by reference
        float sum_prob = prob_anemia + prob_parasite + prob_fever; // N.B. not the same as total_prob, since sum_prob can exceed unity
        if ( sum_prob > 0)
        {
            anemiaPartialProb = prob_anemia / sum_prob;
            parasitePartialProb = prob_parasite / sum_prob;
            feverPartialProb = prob_fever / sum_prob;
        }
        else
        {
            anemiaPartialProb = 0;
            parasitePartialProb = 0;
            feverPartialProb = 0;
        }

        // return total probability
        return total_prob;
    }


    // Fever tracks the level of cytokines
    // This changes a limited cytokine range to more closely match the range of fevers experienced by patients
    // This simple multiplicative scaling factor, allows cytokine dynamics to be adjust for temporal patterns while having an extra parameter to fit fever values
    // This factor is not a necessary free parameter in the dynamics, but is just added to keep things simple
    float SusceptibilityMalaria::get_fever()                 const { return FEVER_DEGREES_CELSIUS_PER_UNIT_CYTOKINES * m_cytokines; }
    float SusceptibilityMalaria::get_fever_celsius()         const { return 37.0f + get_fever(); }
    float SusceptibilityMalaria::get_pyrogenic_threshold()   const { return m_ind_pyrogenic_threshold; }
    float SusceptibilityMalaria::get_cytokines()             const { return m_cytokines; }
    float SusceptibilityMalaria::get_parasite_density()      const { return m_parasite_density; }
    float SusceptibilityMalaria::get_fever_killing_rate()    const { return m_ind_fever_kill_rate; }

    float SusceptibilityMalaria::get_maternal_antibodies()     const
    {
        //if (m_maternal_antibody_strength > 0 ) LOG_DEBUG_F("Individual with age = %f days has m_maternal_antibody_strength = %f\n", age, m_maternal_antibody_strength);
        return m_maternal_antibody_strength;
    }

    void SusceptibilityMalaria::init_maternal_antibodies(float mother_factor)
    {
        if (SusceptibilityMalariaConfig::enable_maternal_antibodies_transmission)
        {
            switch (SusceptibilityMalariaConfig::maternal_antibodies_type)
            {
            case MaternalAntibodiesType::OFF:
                m_maternal_antibody_strength = 0.0f;
                break;

            case MaternalAntibodiesType::CONSTANT_INITIAL_IMMUNITY:
                m_maternal_antibody_strength = SusceptibilityMalariaConfig::maternal_antibody_protection;
                break;

            case MaternalAntibodiesType::SIMPLE_WANING:
                // Initialize newborn antibody protection proportional to 
                // possible mother's antibody history and configurable protection factor
                m_maternal_antibody_strength = SusceptibilityMalariaConfig::maternal_antibody_protection * mother_factor;
                break;

            default:
                throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "maternal_antibodies_type", SusceptibilityMalariaConfig::maternal_antibodies_type, MaternalAntibodiesType::pairs::lookup_key(SusceptibilityMalariaConfig::maternal_antibodies_type));
            }
        }
        else
        {
            m_maternal_antibody_strength = 0.0f;
        }
        LOG_DEBUG_F("Initialized individual with m_maternal_antibody_strength = %f based on mother factor of %f\n", m_maternal_antibody_strength, mother_factor);
    }

    void  SusceptibilityMalaria::ResetMaximumSymptoms()
    { 
        m_max_fever_in_tstep = 0;
        m_max_parasite_density_in_tstep = 0;
        severetype = SevereCaseTypesEnum::NONE;
    }

    void  SusceptibilityMalaria::UpdateMaximumSymptoms()
    {
        // update maximum fever (is zero if undetectable through whole time step)
        float fever_ = get_fever();
        if( fever_ > m_max_fever_in_tstep )
        {
            m_max_fever_in_tstep = fever_;
        }

        // update maximum parasite density (true value not smeared by measurement)
        if ( m_parasite_density > m_max_parasite_density_in_tstep )
        {
            m_max_parasite_density_in_tstep = m_parasite_density;
        }
    }

    float SusceptibilityMalaria::GetMaxFever()               const { return m_max_fever_in_tstep; }
    float SusceptibilityMalaria::GetMaxParasiteDensity()     const { return m_max_parasite_density_in_tstep; }

    float SusceptibilityMalaria::GetHemoglobin()             const
    {
        return get_RBC_count() * GRAMS_OF_HEMOGLOBIN_PER_RBC * get_inv_microliters_blood() * MICROLITERS_PER_DECILITER; // 30 picograms of Hg per RBC, 10^5 microliters per deciliter.
    }

    SevereCaseTypesEnum::Enum SusceptibilityMalaria::CheckSevereCaseType() const
    {
        return severetype;
    }

    int64_t SusceptibilityMalaria::get_RBC_count() const          { return m_RBC; }

    // TODO: use this to calculate parasite density?  trigger end of asexual cycle update timing?
    void SusceptibilityMalaria::remove_RBCs(int64_t infectedAsexual, int64_t infectedGametocytes, double RBC_destruction_multiplier)
    { 
        m_RBC -= ( int64_t(infectedAsexual*RBC_destruction_multiplier) + infectedGametocytes );
    } 

    double SusceptibilityMalaria::get_RBC_availability() const      { return (m_RBCcapacity > 0) ? (double(m_RBC) / m_RBCcapacity) : 0.0; }
    float SusceptibilityMalaria::get_inv_microliters_blood() const  { return m_inv_microliters_blood; }

    REGISTER_SERIALIZABLE(SusceptibilityMalaria);

    void SusceptibilityMalaria::serialize(IArchive& ar, SusceptibilityMalaria* obj)
    {
        if( ar.IsWriter() && (obj->GetParent() != nullptr) )
        {
            // Checking the type of writer so we only decay when serializing
            // to file versus for another core
            JsonFullWriter* p_jfw = dynamic_cast<JsonFullWriter*>(&ar);
            if( p_jfw != nullptr )
            {
                // ----------------------------------------------------------------------------
                // --- Ensure that all inactive antibodies have decayed up to the current time.
                // --- IncreaseAntigenCount() does the decay if needed.
                // ----------------------------------------------------------------------------
                float current_time = obj->GetParent()->GetEventContext()->GetNodeEventContext()->GetTime().time;
                float dt = IndividualHumanConfig::infection_timestep;

                // CPS works differently than the other antibodies so don't decay it
                //obj->m_CSP_antibody.IncreaseAntigenCount( 0, current_time, dt );

                for( int i = 0; i < obj->m_MSP_antibodies.size(); ++i )
                {
                    obj->m_MSP_antibodies[ i ].PrepareForSerialization( current_time, dt );
                }
                for( int i = 0; i < obj->m_PfEMP1_major_antibodies.size(); ++i )
                {
                    obj->m_PfEMP1_major_antibodies[ i ].PrepareForSerialization( current_time, dt );
                }
                for( int i = 0; i < obj->m_PfEMP1_minor_antibodies.size(); ++i )
                {
                    obj->m_PfEMP1_minor_antibodies[ i ].PrepareForSerialization( current_time, dt );
                }
            }
        }

        SusceptibilityVector::serialize(ar, obj);
        SusceptibilityMalaria& susceptibility = *obj;

// Boost serialiation didn't include this member.            ar.labelElement("m_antigenic_flag") & susceptibility.m_antigenic_flag;
        ar.labelElement("m_maternal_antibody_strength") & susceptibility.m_maternal_antibody_strength;
        ar.labelElement("m_CSP_antibody") & susceptibility.m_CSP_antibody;
        ar.labelElement("m_MSP_antibodies") & susceptibility.m_MSP_antibodies;
        ar.labelElement("m_PfEMP1_minor_antibodies") & susceptibility.m_PfEMP1_minor_antibodies;
        ar.labelElement("m_PfEMP1_major_antibodies") & susceptibility.m_PfEMP1_major_antibodies;

        // These are cleared after each Update() so they do not need to be serialized.
        //m_TotalIRBC
        //m_TotalIRBCWithHRP
        //m_active_MSP_antibodies.clear();
        //m_active_PfEMP1_major_antibodies.clear();
        //m_active_PfEMP1_minor_antibodies.clear();

        ar.labelElement("m_RBC") & susceptibility.m_RBC;
        ar.labelElement("m_RBCcapacity") & susceptibility.m_RBCcapacity;
        ar.labelElement("m_RBCproduction") & susceptibility.m_RBCproduction;
        ar.labelElement("m_inv_microliters_blood") & susceptibility.m_inv_microliters_blood;
        ar.labelElement("m_cytokines") & susceptibility.m_cytokines;
        ar.labelElement("m_ind_pyrogenic_threshold") & susceptibility.m_ind_pyrogenic_threshold;
        ar.labelElement("m_ind_fever_kill_rate") & susceptibility.m_ind_fever_kill_rate;
        ar.labelElement("m_cytokine_stimulation") & susceptibility.m_cytokine_stimulation;
        ar.labelElement("m_parasite_density") & susceptibility.m_parasite_density;
        ar.labelElement("m_PfHRP2_pg") & susceptibility.m_PfHRP2_pg;
        ar.labelElement("m_antibodies_to_n_variations") & susceptibility.m_antibodies_to_n_variations;
        ar.labelElement("m_max_fever_in_tstep") & susceptibility.m_max_fever_in_tstep;
        ar.labelElement("m_max_parasite_density_in_tstep") & susceptibility.m_max_parasite_density_in_tstep;
        ar.labelElement("severetype") & (uint32_t&)susceptibility.severetype;
        ar.labelElement("cumulative_days_of_clinical_incident") & susceptibility.cumulative_days_of_clinical_incident;
        ar.labelElement("cumulative_days_of_severe_incident") & susceptibility.cumulative_days_of_severe_incident;
        ar.labelElement("cumulative_days_of_severe_anemia_incident") & susceptibility.cumulative_days_of_severe_anemia_incident;
        ar.labelElement("days_between_incidents") & susceptibility.days_between_incidents;

        if( ar.IsReader() )
        {
            // Checking the type of reader so we only reset the time last actived when
            // deserializing from file versus for another core
            JsonFullReader* p_jfr = dynamic_cast<JsonFullReader*>(&ar);
            if( p_jfr != nullptr )
            {
                // ---------------------------------------------------------------------------
                // --- Assume that the antibodies have all decayed appropriately to the time
                // --- that they were serialized.  Since they have been decayed, we can call
                // --- that the time the antibody was last active. We need to set it here
                // --- because the day of the serialized file might be different than the
                // --- start time of the simulation that is reading the file.
                // ---------------------------------------------------------------------------
                release_assert( EnvPtr->Config != nullptr );
                if( !EnvPtr->Config->Exist( "Start_Time" ) )
                {
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "'Start_Time' must be defined in the config file." );
                }
                float start_time = (*EnvPtr->Config)["Start_Time"].As<Number>();

                // CPS is handled differently so we don't set the time
                //obj->m_CSP_antibody.SetTimeLastActive( start_time );

                for( int i = 0; i < obj->m_MSP_antibodies.size(); ++i )
                {
                    obj->m_MSP_antibodies[ i ].SetTimeLastActive( start_time );
                }
                for( int i = 0; i < obj->m_PfEMP1_major_antibodies.size(); ++i )
                {
                    obj->m_PfEMP1_major_antibodies[ i ].SetTimeLastActive( start_time );
                }
                for( int i = 0; i < obj->m_PfEMP1_minor_antibodies.size(); ++i )
                {
                    obj->m_PfEMP1_minor_antibodies[ i ].SetTimeLastActive( start_time );
                }
            }
        }
    }
}
