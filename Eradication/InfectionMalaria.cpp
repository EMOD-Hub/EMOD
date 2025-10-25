
#include "stdafx.h"
#include "InfectionMalaria.h"

#include <numeric>

#include "Common.h"
#include "Sugar.h"
#include "Environment.h"
#include "Exceptions.h"
#include "Log.h"
#include "MalariaEnums.h"

#include "Sigmoid.h"
#include "SusceptibilityMalaria.h"
#include "Interventions.h"
#include "SimulationConfig.h"
#include "IndividualEventContext.h" // for Die() interface
#include "IIndividualHumanContext.h"
#include "MalariaInterventionsContainerContexts.h"
#include "StrainIdentity.h"
#include "StrainIdentityMalariaVarGenes.h"
#include "RANDOM.h"
#include "NodeEventContext.h"
#include "EventTrigger.h"

SETUP_LOGGING( "InfectionMalaria" )

namespace Kernel
{
    ParasiteSwitchType::Enum InfectionMalariaConfig::parasite_switch_type = ParasiteSwitchType::RATE_PER_PARASITE_7VARS;
    MalariaStrains::Enum     InfectionMalariaConfig::malaria_strains      = MalariaStrains::FALCIPARUM_RANDOM_STRAIN;

    float InfectionMalariaConfig::antibody_IRBC_killrate = 0.0f;
    float InfectionMalariaConfig::MSP1_merozoite_kill = 0.0f;
    float InfectionMalariaConfig::gametocyte_stage_survival = 0.0f;
    float InfectionMalariaConfig::base_gametocyte_sexratio = 0.0f;
    float InfectionMalariaConfig::base_gametocyte_production = 0.0f;
    float InfectionMalariaConfig::antigen_switch_rate = 0.0f;
    float InfectionMalariaConfig::merozoites_per_hepatocyte = 0.0f;
    float InfectionMalariaConfig::merozoites_per_schizont = 0.0f;
    float InfectionMalariaConfig::non_specific_antigenicity = 0.0f;
    float InfectionMalariaConfig::RBC_destruction_multiplier = 0.0f;
    int   InfectionMalariaConfig::n_asexual_cycles_wo_gametocytes = 0;

    // QI stuff (none in InfectionVector or Infection for now)
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Malaria.Infection,InfectionMalariaConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionMalariaConfig)
    END_QUERY_INTERFACE_BODY(InfectionMalariaConfig)

    // 120 primes
    const std::vector<uint32_t> InfectionMalaria::STRIDE_LENGTHS = {73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809};

    BEGIN_QUERY_INTERFACE_BODY(InfectionMalaria)
        HANDLE_INTERFACE(IInfectionMalaria)
    END_QUERY_INTERFACE_BODY(InfectionMalaria)

    InfectionMalaria::InfectionMalaria() : Kernel::InfectionVector(),
        m_IRBCtimer(0.0),
        m_hepatocytes(0),
        m_asexual_phase(AsexualCycleStatus::NoAsexualCycle),
        m_asexual_cycle_count(0),
        m_MSPtype(0),
        m_nonspectype(0),
        m_MSP_antibody(nullptr),
        m_PfEMP1_antibodies(CLONAL_PfEMP1_VARIANTS),
        m_IRBC_count(CLONAL_PfEMP1_VARIANTS),
        m_gametorate(0.0),
        m_gametosexratio(0.0),
        m_measured_duration(0),
        m_start_measuring(false),
        m_temp_duration(0),
        m_max_parasites(0),
        m_inv_microliters_blood(INV_MICROLITERS_BLOOD_ADULT),
        m_pMDE( nullptr ),
        m_CurrentInfectionStage( MalariaInfectionStage::HEPATOCYTE ),
        m_TimeSinceInfectionStageChange( 0.0f )
    {
    }

    InfectionMalaria::InfectionMalaria(IIndividualHumanContext *context) : Kernel::InfectionVector(context),
        m_IRBCtimer(0.0),
        m_hepatocytes(0),
        m_asexual_phase(AsexualCycleStatus::NoAsexualCycle),
        m_asexual_cycle_count(0),
        m_MSPtype(0),
        m_nonspectype(0),
        m_MSP_antibody(nullptr),
        m_PfEMP1_antibodies(CLONAL_PfEMP1_VARIANTS),
        m_IRBC_count(CLONAL_PfEMP1_VARIANTS),
        m_gametorate(0.0),
        m_gametosexratio(0.0),
        m_measured_duration(0),
        m_start_measuring(false),
        m_temp_duration(0),
        m_max_parasites(0),
        m_inv_microliters_blood(INV_MICROLITERS_BLOOD_ADULT),
        m_pMDE( nullptr ),
        m_CurrentInfectionStage( MalariaInfectionStage::HEPATOCYTE ),
        m_TimeSinceInfectionStageChange( 0.0f )
    {
    }

    void InfectionMalariaConfig::ConfigureMalariaStrainModel( const Configuration* config )
    {
        initConfig( "Malaria_Strain_Model", malaria_strains, config, MetadataDescriptor::Enum("malaria_strains", Malaria_Strain_Model_DESC_TEXT, MDD_ENUM_ARGS(MalariaStrains)) ); // 'global'
    }

    bool
    InfectionMalariaConfig::Configure(
        const Configuration * config
    )
    {
        // number_substrains is not configured from the JSON like the other diseases
        // TBD: Would really like to not do it this way, setting node variable from here. TBD revisit.
        InfectionConfig::number_substrains = 1;

        ConfigureMalariaStrainModel( config );

        initConfig( "Parasite_Switch_Type", parasite_switch_type, config, MetadataDescriptor::Enum("parasite_switch_type", Parasite_Switch_Type_DESC_TEXT, MDD_ENUM_ARGS(ParasiteSwitchType)) ); // infection (malaria) only 

        initConfigTypeMap( "Antibody_IRBC_Kill_Rate", &antibody_IRBC_killrate, Antibody_IRBC_Kill_Rate_DESC_TEXT, 0.0f, 1000.0f, DEFAULT_ANTIBODY_IRBC_KILLRATE ); // malaria
        initConfigTypeMap( "Nonspecific_Antigenicity_Factor", &non_specific_antigenicity, Nonspecific_Antigenicity_Factor_DESC_TEXT, 0.0f, 1000.0f, float(DEFAULT_NON_SPECIFIC_ANTIGENICITY) ); // malaria
        initConfigTypeMap( "MSP1_Merozoite_Kill_Fraction", &MSP1_merozoite_kill, MSP1_Merozoite_Kill_Fraction_DESC_TEXT, 0.0f, 1.0f, DEFAULT_MSP1_MEROZOITE_KILL ); // malaria
        initConfigTypeMap( "Gametocyte_Stage_Survival_Rate", &gametocyte_stage_survival, Gametocyte_Stage_Survival_Rate_DESC_TEXT, 0.0f, 1.0f, DEFAULT_GAMETOCYTE_STAGE_SURVIVAL ); // malaria
        initConfigTypeMap( "Base_Gametocyte_Fraction_Male", &base_gametocyte_sexratio, Base_Gametocyte_Fraction_Male_DESC_TEXT, 0.0f, 1.0f, DEFAULT_BASE_GAMETOCYTE_SEX_RATIO ); // malaria
        initConfigTypeMap( "Base_Gametocyte_Production_Rate", &base_gametocyte_production, Base_Gametocyte_Production_Rate_DESC_TEXT, 0.0f, 1.0f, DEFAULT_BASE_GAMETOCYTE_PRODUCTION ); // malaria
        initConfigTypeMap( "Antigen_Switch_Rate", &antigen_switch_rate, Antigen_Switch_Rate_DESC_TEXT, 0.0f, 1.0f, 2.0e-9f ); // malaria
        initConfigTypeMap( "Merozoites_Per_Hepatocyte", &merozoites_per_hepatocyte, Merozoites_Per_Hepatocyte_DESC_TEXT, 0.0f, FLT_MAX, DEFAULT_MEROZOITES_PER_HEPATOCYTE ); // malaria
        initConfigTypeMap( "Merozoites_Per_Schizont", &merozoites_per_schizont, Merozoites_Per_Schizont_DESC_TEXT, 0.0f, 1000.0f, DEFAULT_MEROZOITES_PER_SCHIZONT ); // malaria
        initConfigTypeMap( "RBC_Destruction_Multiplier", &RBC_destruction_multiplier, RBC_Destruction_Multiplier_DESC_TEXT, 0.0f, 30.0f, DEFAULT_RBC_DESTRUCTION_MULTIPLIER ); // malaria GH
        initConfigTypeMap( "Number_Of_Asexual_Cycles_Without_Gametocytes", &n_asexual_cycles_wo_gametocytes, Number_Of_Asexual_Cycles_Without_Gametocytes_DESC_TEXT, 0, 500, DEFAULT_ASEXUAL_CYCLES_WITHOUT_GAMETOCYTES );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( base_gametocyte_production == 1.0 )
            {
                LOG_WARN( "Base_Gametocyte_Production_Rate = 1.0.  Vectors cannot become infected.  If you want vectors to become infected, select a value < 1.0.\n" );
            }
        }
        return ret;
    }

    void InfectionMalaria::Initialize(suids::suid suid, int initial_hepatocytes)
    {
        Kernel::Infection::Initialize(suid);

        duration       = 0;// total duration, including incubation
        infectiousness = 0;
        StateChange    = InfectionStateChange::None;
        
        m_hepatocytes = initial_hepatocytes;

        ZERO_ARRAY(m_malegametocytes);
        ZERO_ARRAY(m_femalegametocytes);

        ZERO_ARRAY(m_IRBCtype);
        ZERO_ARRAY(m_minor_epitope_type);

        incubation_timer = InfectionConfig::incubation_distribution->Calculate( parent->GetRng() );
    }

    InfectionMalaria *InfectionMalaria::CreateInfection(IIndividualHumanContext *context, suids::suid suid, int initial_hepatocytes)
    {
        InfectionMalaria *newinfection = _new_ InfectionMalaria(context);
        newinfection->Initialize(suid, initial_hepatocytes);

        return newinfection;
    }

    InfectionMalaria::~InfectionMalaria()
    {
    }

    void InfectionMalaria::SetParameters( const IStrainIdentity *_infstrain, int incubation_period_override )
    {
        // Set up infection strain
        CreateInfectionStrain(_infstrain);

        // Here we set the antigenic repertoire of the infection
        // Can be completely distinct strains, or partially overlapping repertoires of antigens
        // Bull, P. C., B. S. Lowe, et al. (1998). "Parasite antigens on the infected red cell surface are targets for naturally acquired immunity to malaria." Nat Med 4(3): 358-360.
        // Recker, M., S. Nee, et al. (2004). "Transient cross-reactive immune responses can orchestrate antigenic variation in malaria." Nature 429(6991): 555-558.
        // In our model, not all antigens are expressed at the same time, but switching occurs.  This just sets the total repertoire

        // Variables for strain generator
        // In the future replace this with the specific strain ID
        unsigned int tempstrainID = 0;
        unsigned int tempStridePosition = 0;
        unsigned int tempStrideLength = 0;

        switch (InfectionMalariaConfig::malaria_strains)
        {
            case MalariaStrains::FALCIPARUM_NONRANDOM_STRAIN:
                m_MSPtype = 0;
                m_nonspectype = 0;

                #pragma loop(hint_parallel(8))
                for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
                {
                    m_IRBCtype[i] = i;
                    m_minor_epitope_type[i] = parent->GetRng()->uniformZeroToN16(MINOR_EPITOPE_VARS_PER_SET) + MINOR_EPITOPE_VARS_PER_SET * m_nonspectype;
                }
                break;

            // limited to CLONAL_PfEMP1_VARIANTS variants and 5 nonspec minor epitopes
            case MalariaStrains::FALCIPARUM_RANDOM50_STRAIN:
                m_MSPtype = 0;
                m_nonspectype = 0;

                #pragma loop(hint_parallel(8))
                for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
                {
                    m_IRBCtype[i]           = parent->GetRng()->uniformZeroToN16(CLONAL_PfEMP1_VARIANTS);
                    m_minor_epitope_type[i] = parent->GetRng()->uniformZeroToN16(MINOR_EPITOPE_VARS_PER_SET) + MINOR_EPITOPE_VARS_PER_SET * m_nonspectype;
                }
                break;

            case MalariaStrains::FALCIPARUM_RANDOM_STRAIN:
                m_MSPtype     = parent->GetRng()->uniformZeroToN16( SusceptibilityMalariaConfig::falciparumMSPVars      );
                m_nonspectype = parent->GetRng()->uniformZeroToN16( SusceptibilityMalariaConfig::falciparumNonSpecTypes );

                #pragma loop(hint_parallel(8))
                for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
                {
                    m_IRBCtype[i] = parent->GetRng()->uniformZeroToN16( SusceptibilityMalariaConfig::falciparumPfEMP1Vars );
                    m_minor_epitope_type[i] = parent->GetRng()->uniformZeroToN16(MINOR_EPITOPE_VARS_PER_SET) + MINOR_EPITOPE_VARS_PER_SET * m_nonspectype;
                }
                break;

            case MalariaStrains::FALCIPARUM_STRAIN_GENERATOR:
                // in the future replace this with the specific strain ID
                tempstrainID = infection_strain->GetGeneticID();

                m_MSPtype          =  tempstrainID%SusceptibilityMalariaConfig::falciparumMSPVars;
                m_nonspectype      = (tempstrainID/SusceptibilityMalariaConfig::falciparumMSPVars)%SusceptibilityMalariaConfig::falciparumNonSpecTypes;
                tempStridePosition =  tempstrainID%SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
                tempStrideLength = InfectionMalaria::STRIDE_LENGTHS[(tempstrainID/SusceptibilityMalariaConfig::falciparumPfEMP1Vars)%InfectionMalaria::STRIDE_LENGTHS.size()];// in case it goes over limit of primes

                #pragma loop(hint_parallel(8))
                for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
                {
                    m_IRBCtype[i] = tempStridePosition; 
                    tempStridePosition = (tempStridePosition + tempStrideLength)% SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
                    m_minor_epitope_type[i] = parent->GetRng()->uniformZeroToN16(MINOR_EPITOPE_VARS_PER_SET) + MINOR_EPITOPE_VARS_PER_SET * m_nonspectype;
                }
                break;

            case MalariaStrains::FALCIPARUM_FIXED_STRAIN:
            {
                static std::map<int, StrainIdentityMalariaVarGenes> gen_id_to_sivg;

                StrainIdentityMalariaVarGenes* p_sivp = dynamic_cast<StrainIdentityMalariaVarGenes*>(infection_strain);

                if( (p_sivp != nullptr) && (gen_id_to_sivg.count( infection_strain->GetGeneticID() ) == 0) )
                {
                    gen_id_to_sivg[ p_sivp->GetGeneticID() ] = *p_sivp;
                }
                else if( p_sivp == nullptr )
                {
                    release_assert( gen_id_to_sivg.count( infection_strain->GetGeneticID() ) != 0 );

                    p_sivp = new StrainIdentityMalariaVarGenes( gen_id_to_sivg[ infection_strain->GetGeneticID() ] );
                    delete infection_strain;
                    infection_strain = p_sivp;
                }

                m_MSPtype     = p_sivp->GetMSPType();
                m_nonspectype = 0; // not needed since minor epitopes are provided

                release_assert( p_sivp->GetIRBCType().size()         == CLONAL_PfEMP1_VARIANTS );
                release_assert( p_sivp->GetMinorEpitopeType().size() == CLONAL_PfEMP1_VARIANTS );

                for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
                {
                    m_IRBCtype[ i ] = p_sivp->GetIRBCType()[ i ];
                    m_minor_epitope_type[ i ] = p_sivp->GetMinorEpitopeType()[ i ];
                }
                break;
            }

            // default is falciparum single strain, with only a single minor epitope for all PfEMP-1 variants
            default:
                m_MSPtype = 0;
                m_nonspectype = 0;

                #pragma loop(hint_parallel(8))
                for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
                {
                    m_IRBCtype[i] = i;
                    m_minor_epitope_type[i] = 0;
                }
                break;
        }

        #pragma loop(hint_parallel(8))
        for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
        {
            if ((m_IRBCtype[i] > SusceptibilityMalariaConfig::falciparumPfEMP1Vars) || (m_IRBCtype[i] < 0))
            {
                throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "m_IRBCtype[i]", m_IRBCtype[i], 0.0f );
            }
        }
    }

    void InfectionMalaria::InitInfectionImmunology(ISusceptibilityContext* _immunity)
    {
        if( _immunity == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "_immunity", "Susceptibility" );
        }

        //SusceptibilityMalaria* immunity = static_cast<SusceptibilityMalaria*>(_immunity);
        IMalariaSusceptibility* immunity = nullptr;
        if (s_OK != _immunity->QueryInterface(GET_IID(IMalariaSusceptibility), (void **)&immunity))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "_immunity", "IMalariaSusceptibility", "Susceptibility" );
        }

        m_MSP_antibody = immunity->RegisterAntibody(MalariaAntibodyType::MSP1, m_MSPtype);

        // Can we postpone the creation of MalariaAntibody pointers until an actual infection variant has non-zero IRBC?
        // TODO: there are never any IRBC at this point, so no need to have the RegisterAntibody() calls... also, the NULL initialization can go up somewhere else...
        for( int ivariant = 0; ivariant < m_PfEMP1_antibodies.size(); ivariant++ )
        {
            m_PfEMP1_antibodies[ivariant].major = nullptr;
            m_PfEMP1_antibodies[ivariant].minor = nullptr;

            if ( m_IRBC_count[ivariant] > 0 )
            {
                m_PfEMP1_antibodies[ivariant].minor  = immunity->RegisterAntibody(MalariaAntibodyType::PfEMP1_minor, m_minor_epitope_type[ivariant]);
                m_PfEMP1_antibodies[ivariant].major  = immunity->RegisterAntibody(MalariaAntibodyType::PfEMP1_major, m_IRBCtype[ivariant]);
            }
        }
    }


    void InfectionMalaria::Update( float currentTime, float dt, ISusceptibilityContext* _immunity )
    {
        LOG_VALID("\n--------------------------------------------------\n\n");

        // Needed for this timestep
        //SusceptibilityMalaria *immunity = static_cast<SusceptibilityMalaria *>(_immunity);
        // TBD: Should we cache the IMalariaSusceptibility? Should be constant for an infection. But Infection does not own it,
        // so need to find right point? Or access always through a wrapper?
        IMalariaSusceptibility* immunity = nullptr;
        if (s_OK != _immunity->QueryInterface(GET_IID(IMalariaSusceptibility), (void **)&immunity))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "Susceptibility", "IMalariaSusceptibility", "_immunity" );
        }

        m_inv_microliters_blood = immunity->get_inv_microliters_blood();

        StateChange = InfectionStateChange::None; // reset state change of previous time-step
        duration += dt;

        // process hepatocytes (if any)--can be killed by drugs
        if (m_hepatocytes > 0)
        {
            malariaProcessHepatocytes(dt, immunity);
        }

        // Asexual phase processed only if necessary
        if (m_asexual_phase > AsexualCycleStatus::NoAsexualCycle) // saves processing and memory during hepatocyte queue
        {
            // do not decrement timer if it was just set by the hepatocytes this time step (asexual_phase==2), or else the timer gets decreased one timestep too many
            if (m_asexual_phase == AsexualCycleStatus::HepatocyteRelease)
            {
                m_asexual_phase = AsexualCycleStatus::AsexualCycle;
            }
            else
            {
                m_IRBCtimer -= dt;
            }

            // process end of asexual cycle events if appropriate
            if (m_IRBCtimer <= 0)
            {
                processEndOfAsexualCycle( currentTime, dt, immunity );
            }

            // check for death due to death of all RBCs
            if (immunity->get_RBC_count() < 1)
            {
                LOG_WARN("Individual RBC Count = 0, calling Die() \n");
                parent->GetEventContext()->Die(HumanStateChange::KilledByInfection); //KM - used to be StateChange = InfectionStateChange::Fatal, but this requires vital dynamics;
            }

            // Immune Interaction
            // Infection Effect on Immune System--
            // in Susceptibility object update, antibody capacities increase and antibodies produced in response to antigenic-specific parasite load, tolerance--lack of inflamatory response-- develops
            malariaImmuneStimulation( currentTime, dt, immunity );

            // Immune and Drug Effects on Infection
            malariaImmunityIRBCKill(dt, immunity);

            // Immune and Drug Effects on Gametocytes
            malariaImmunityGametocyteKill(dt, immunity);

            //make sure MSP type generates antibodies during an ongoing infection, not just during the short time of IRBC rupturing, since the stimulation may persist
            // TODO: could just SetAntigenPresent instead?
            m_MSP_antibody->IncreaseAntigenCount( 1, currentTime, dt );

            immunity->SetActiveAntibody( m_MSP_antibody ); // NOTE: this has an interesting behavior in that it continues to update MSP capacity AFTER there are no IRBC (only gametocytes)
        }

        // check for death, clearance, and take care of end-of-timestep bookkeeping
        malariaCheckInfectionStatus(dt, immunity);

        // Tell immunity how many of infected red blood cells are due to this infection
        int64_t total_IRBC = CalculateTotalIRBC();
        int64_t total_IRBC_with_HRP = CalculateTotalIRBCWithHRP( total_IRBC );
        immunity->UpdateIRBC( total_IRBC, total_IRBC_with_HRP );
    }

    int64_t InfectionMalaria::CalculateTotalIRBCWithHRP( int64_t totalIRBC ) const
    {
        return totalIRBC;
    }

    void InfectionMalaria::processEndOfAsexualCycle( float currentTime, float dt, IMalariaSusceptibility* immunity )
    {
        // Merozoite-specific antibodies can limit merozoite success--Blackman, M. J., H. G. Heidrich, et al. (1990).
        // "A single fragment of a malaria merozoite surface protein remains on the parasite during red cell invasion
        // and is the target of invasion-inhibiting antibodies." J Exp Med 172(1): 379-382.
        double RBCavailability = immunity->get_RBC_availability();

        // Merozoite survival limited at very low density according to density-dependent probability-of-success formula
        double merozoitesurvival = max(0.0, (1.0 - InfectionMalariaConfig::MSP1_merozoite_kill * m_MSP_antibody->GetAntibodyConcentration() ) * EXPCDF(-RBCavailability / MEROZOITE_LIMITING_RBC_THRESHOLD));

        LOG_VALID("\n===================================================\n\n");
        LOG_VALID_F("RBCavailability = %0.9f  merozoitesurvival = %0.9f\n", RBCavailability, merozoitesurvival);

        // How many rupture for this infection handed to suscept object for total stimulation calculations
        int64_t totalIRBC = CalculateTotalIRBC();
        if( totalIRBC > 0 )
        {
            m_MSP_antibody->IncreaseAntigenCount( totalIRBC, currentTime, dt );
        }

        // Move immature gametocytes forward a stage and create initial stage gametocytes from previous merozoites
        // This is the last function to use m_IRBC_count from the previous cycle
        malariaCycleGametocytes(merozoitesurvival);

        // Calculate antigenic switching and create asexual IRBCss for next asexual cycle
        // After this function, m_IRBC_count will have been updated
        malariaIRBCAntigenSwitch(merozoitesurvival);

        totalIRBC = 0;
        #pragma loop(hint_parallel(8))
        for ( int j = 0; j < CLONAL_PfEMP1_VARIANTS; j++ )
        {
            if ( m_IRBC_count[j] > 0 )
            {
                totalIRBC += m_IRBC_count[j];
                immunity->UpdateActiveAntibody( m_PfEMP1_antibodies[j], m_minor_epitope_type[j], m_IRBCtype[j] ); // insert into set of antigens the immune system has ever "seen"
            }
        }

        // Uninfected RBC killing diminishing in proportion to RBC availability
        double destruction_factor_ = max(1.0, InfectionMalariaConfig::RBC_destruction_multiplier * EXPCDF(-RBCavailability / MEROZOITE_LIMITING_RBC_THRESHOLD) );
        immunity->remove_RBCs( totalIRBC, m_malegametocytes[0] + m_femalegametocytes[0], destruction_factor_ );

        LOG_VALID_F("Removed %lld RBC (asexual) plus %lld (gametocytes)\n", int64_t(totalIRBC * destruction_factor_), m_malegametocytes[0] + m_femalegametocytes[0]);
        LOG_VALID("\n===================================================\n\n");

        // reset timer for next asexual cycle
        m_IRBCtimer = 2.0;

        // increment counter of completed asexual cycles
        m_asexual_cycle_count++;
    }

    // Sums up the current parasite counts for the infection and determines if the infection is cleared or if death occurs this time step
    // All end of time step bookkeeping, basically
    void InfectionMalaria::malariaCheckInfectionStatus(float dt, IMalariaSusceptibility *immunity)
    {
        // check for valid inputs
        if (immunity)
        {
            int64_t totalgametocytes = 0;

            // total the gametocytes and IRBCs
            #pragma loop(hint_parallel(8))
            for (int i = 0; i < GametocyteStages::Mature; i++)
            {
                totalgametocytes += m_malegametocytes[i] + m_femalegametocytes[i];
            }

            int64_t totalIRBC = CalculateTotalIRBC();
            if (totalIRBC > m_max_parasites)
            {
                m_max_parasites = totalIRBC;
            }

            // calculation of infectiousness is done at individual level

            // track measured duration
            if (double(totalIRBC)*m_inv_microliters_blood > MINIMUM_IRBC_COUNT)
            {
                m_start_measuring = true;
            }

            if (m_start_measuring)
            {
                m_temp_duration += dt;

                if (double(totalIRBC * m_inv_microliters_blood) > MINIMUM_IRBC_COUNT)
                {
                    m_measured_duration += m_temp_duration;
                    m_temp_duration = 0;
                }
            }

            // Does individual die, or is infection cleared?
            if ((totalIRBC + m_hepatocytes + totalgametocytes) < 1)
            {
                LOG_VALID("CLEARED infection\n");
                StateChange = InfectionStateChange::Cleared;
                auto broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
                broadcaster->TriggerObservers( parent->GetEventContext(), EventTrigger::InfectionCleared );
            }

            MalariaInfectionStage::Enum prev_stage = m_CurrentInfectionStage;
            if( m_hepatocytes > 0 )
            {
                m_CurrentInfectionStage = MalariaInfectionStage::HEPATOCYTE;
            }
            else if( totalIRBC > 0 )
            {
                m_CurrentInfectionStage = MalariaInfectionStage::ASEXUAL;
            }
            else
            {
                m_CurrentInfectionStage = MalariaInfectionStage::GAMETOCYTE_ONLY;
            }
            if( prev_stage != m_CurrentInfectionStage )
            {
                m_TimeSinceInfectionStageChange = 0.0;
            }
            else
            {
                m_TimeSinceInfectionStageChange += dt;
            }
        }
    }

    // Calculates the IRBC killing from drugs and immune action
    void InfectionMalaria::malariaImmunityIRBCKill(float dt, IMalariaSusceptibility *immunity)
    {
        // check for valid inputs
        if (dt > 0 && immunity)
        {
            // inflammatory response--Stevenson, M. M. and E. M. Riley (2004).
            // "Innate immunity to malaria." Nat Rev Immunol 4(3): 169-180.

            // Offset basic sigmoid: effect rises as basic sigmoid beginning from a fever of MIN_FEVER_DEGREES_KILLING
            double fever_cytokine_killrate = (immunity->get_fever() > MIN_FEVER_DEGREES_KILLING) ? immunity->get_fever_killing_rate() * Sigmoid::basic_sigmoid(1.0, immunity->get_fever() - MIN_FEVER_DEGREES_KILLING) : 0.0;
            //LOG_VALID_F("fever = %0.9f (%0.2f C)  killrate = %0.9f\n", immunity->get_fever(), immunity->get_fever_celsius(), fever_cytokine_killrate);

            // ability to query for drug effects
            IIndividualHumanContext *patient = GetParent();
            IIndividualHumanInterventionsContext *context = patient->GetInterventionsContext();

            if( m_pMDE == nullptr )
            {
                context->QueryInterface( GET_IID( IMalariaDrugEffects ), (void **)&m_pMDE );
            }

            double drug_killrate = 0;
            if( m_pMDE != nullptr )
            {
                drug_killrate = m_pMDE->get_drug_IRBC_killrate( *infection_strain );
            }

            RANDOMBASE* p_rng = parent->GetRng();
            double fever_plus_drug = fever_cytokine_killrate + drug_killrate;

            float maternal_antibodies = immunity->get_maternal_antibodies();

            #pragma loop(hint_parallel(8))
            for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
            {
                if ( m_IRBC_count[i] == 0 ) continue; // don't need to estimate killing if there are no IRBC of this variant to kill!

                //LOG_VALID_F( "Ab concentration for variant %d: %0.9f (major) %0.9f (minor)\n", i, m_PfEMP1_antibodies[i].major->GetAntibodyConcentration(), m_PfEMP1_antibodies[i].minor->GetAntibodyConcentration() );
                // total = antibodies (major, minor, maternal) + fever + drug
                double pkill = EXPCDF( -dt * ( (   m_PfEMP1_antibodies[i].major->GetAntibodyConcentration()
                                                 + m_PfEMP1_antibodies[i].minor->GetAntibodyConcentration() * InfectionMalariaConfig::non_specific_antigenicity
                                                 + maternal_antibodies
                                               ) * InfectionMalariaConfig::antibody_IRBC_killrate
                                               + fever_plus_drug
                                             )
                                     );
                
                // Now here there is an interesting issue: to save massive amounts of computational time, can use a Gaussian approximation for the true binomial, but this returns a float
                // This is fine for large numbers of killed IRBC's, but an issue arises for small numbers
                // big question, is 1.5 killed IRBC's 1 or 2 killed?

                //LOG_VALID_F("pkill = %0.9f\n", pkill);

                double tempval1 = m_IRBC_count[i] * pkill;
                if ( tempval1 > 0 ) // don't need to smear the killing by a random number if it is going to be zero
                    tempval1 = p_rng->eGauss() * sqrt(tempval1 * (1.0 - pkill)) + tempval1;

                if (tempval1 < 0.5)
                    tempval1 = 0;


                // so add a continuity correction 0.5, and then convert to integer
                m_IRBC_count[i] -= int64_t(tempval1 + 0.5);

                if (m_IRBC_count[i] < 1)
                    m_IRBC_count[i] = 0;   // check for too large a time step

            }
        }
    }

    // Calculates stimulation of immune system by malaria infection
    void InfectionMalaria::malariaImmuneStimulation( float currentTime, float dt, IMalariaSusceptibility *immunity )
    {
        // check for valid inputs
        if ( dt <= 0 || immunity == nullptr )
        {
            LOG_WARN("Invalid input to malariaImmuneStimulation\n");
            return;
        }

        // antibody capacity for MSP 1 and MSP-2 are above
        // antibody capacity for the different RBC surface variants
        // transfer total IRBC to array owned by Susceptibility_Malaria, which then calculates total immune stimulation by all concurrent infections
        #pragma loop(hint_parallel(8))
        for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
        {
            // only update if there are actually IRBCs
            if (m_IRBC_count[i] > 0)
            {
                //LOG_VALID_F("Increasing IRBC count by %lld for variant %d:  ( %d, %d )\n", m_IRBC_count[i], i, m_PfEMP1_antibodies[i].minor->GetAntibodyVariant(), m_PfEMP1_antibodies[i].major->GetAntibodyVariant());

                // PfEMP-1 major epitopes
                m_PfEMP1_antibodies[i].major->IncreaseAntigenCount( m_IRBC_count[i], currentTime, dt );

                // PfEMP-1 minor epitopes
                m_PfEMP1_antibodies[i].minor->IncreaseAntigenCount( m_IRBC_count[i], currentTime, dt );

                // Tell susceptibility which antibodies are in use/active
                immunity->SetActiveAntibody( m_PfEMP1_antibodies[i].major );
                immunity->SetActiveAntibody( m_PfEMP1_antibodies[i].minor );
            }
            else if (m_IRBC_count[i] < 0)
            {
                m_IRBC_count[i] = 0;
                LOG_WARN_F( "malariaImmuneStimulation() IRBC count at index %d should not be negative\n", i );
            }
        }
    }

    // Calculates immature gametocyte killing from drugs and immune action
    void InfectionMalaria::malariaImmunityGametocyteKill(float dt, IMalariaSusceptibility *immunity)
    {
        // ability to query for drug effects
        IIndividualHumanContext *patient              = GetParent();
        IIndividualHumanInterventionsContext *context = patient->GetInterventionsContext();

        // check for valid inputs
        if (dt > 0 && immunity)
        {
            #pragma loop(hint_parallel(8))
            for (int i = 0; i < GametocyteStages::Mature; i++)
            {
                // Currently have fever and inflammatory cytokines limiting infectivity, 
                // rather than killing gametocytes.  See IndividualHumanMalaria::DepositInfectiousnessFromGametocytes()
                // We leave this variable here at zero incase we change DepositInfectiousnessFromGametocytes()
                double fever_cytokine_killrate = 0; // 0 = don't kill due to fever

                if( m_pMDE == nullptr )
                {
                    context->QueryInterface( GET_IID( IMalariaDrugEffects ), (void **)&m_pMDE );
                }

                // gametocyte killing drugs
                double drug_killrate = 0;
                if( m_pMDE != nullptr )
                {
                    if (i < GametocyteStages::Stage3)
                        drug_killrate = m_pMDE->get_drug_gametocyte02( *infection_strain );
                    else
                        drug_killrate = m_pMDE->get_drug_gametocyte34( *infection_strain );
                }

                // no randomness in gametocyte killing, but a continuity correction
                double gametocyte_kill_fraction = EXPCDF( -dt * (fever_cytokine_killrate + drug_killrate) );

                m_malegametocytes[i] -= int64_t( 0.5 + m_malegametocytes[i] * gametocyte_kill_fraction );
                if (m_malegametocytes[i] < 1)
                    m_malegametocytes[i] = 0;

                m_femalegametocytes[i] -= int64_t( 0.5 + m_femalegametocytes[i] * gametocyte_kill_fraction );
                if (m_femalegametocytes[i] < 1)
                    m_femalegametocytes[i] = 0;
            }

            // mature gametocytes die as part Individual_Malaria::getinfectiousness()
        }
    }

    // Calculates the antigenic switching when an asexual cycle completes and creates next generation of IRBC's
    void InfectionMalaria::malariaIRBCAntigenSwitch(double merozoitesurvival)
    {
        int64_t switchingIRBC[SWITCHING_IRBC_VARIANT_COUNT];
        std::vector<int64_t> tmpIRBCcount(CLONAL_PfEMP1_VARIANTS);

        // check for valid range of input, and only create next cycle if valid
        if (merozoitesurvival < 0)
        {
            // ERROR: ("malariaIRBCAntigenSwitch() merozoitesuvival argument (%f) should not be negative.\n", merozoitesurvival);
            throw CalculatedValueOutOfRangeException(  __FILE__, __LINE__, __FUNCTION__, "merozoitesurvival", merozoitesurvival, 0.0f );
        }

        // Several antigen switching mechanisms are supported
        #pragma loop(hint_parallel(8))
        for (int j = 0; j < CLONAL_PfEMP1_VARIANTS; j++)
        {
            // parasite switching studied in Paget-McNicol, S., M. Gatton, et al. (2002). "The Plasmodium falciparum var gene switching rate, switching mechanism and patterns of parasite recrudescence described by mathematical modelling." Parasitology 124(Pt 3): 225-235.
            // experimental studies in Horrocks, P., R. Pinches, et al. (2004). "Variable var transition rates underlie antigenic variation in malaria." Proceedings of the National Academy of Sciences of the United States of America 101(30): 11129-11134.
            // review in Horrocks, P., S. A. Kyes, et al. (2005). Molecular Aspects of Antigenic Variation in Plasmodium falciparum. Molecular Approaches to Malaria. I. W. Sherman. Washington DC, ASM Press: 399-415.

            if ( m_IRBC_count[j] <= 0 ) continue; // no IRBC means no contribution to next time step

            switch (InfectionMalariaConfig::parasite_switch_type)
            {
            case ParasiteSwitchType::RATE_PER_PARASITE_7VARS:
                {
                    int64_t temp_sum_IRBC = 0;
                    if (InfectionMalariaConfig::antigen_switch_rate > 0)
                    {
                        #pragma loop(hint_parallel(8))
                        for ( int iswitch = 0; iswitch < SWITCHING_IRBC_VARIANT_COUNT; iswitch++ )
                        {
                            switchingIRBC[iswitch] = (iswitch < 7) ? parent->GetRng()->Poisson(InfectionMalariaConfig::antigen_switch_rate * m_IRBC_count[j]) : 0;
                        }

                        // now test to see if these add up to more than 100 percent
                        temp_sum_IRBC = std::accumulate(switchingIRBC, switchingIRBC + SWITCHING_IRBC_VARIANT_COUNT, temp_sum_IRBC);

                        // if more than 100 percent minus those switching to gametocyte production, scale down in multiplicative way
                        if (temp_sum_IRBC > ((1.0 - m_gametorate)*m_IRBC_count[j]))
                        {
                            #pragma loop(hint_parallel(8))
                            for (int iswitch = 0; iswitch < SWITCHING_IRBC_VARIANT_COUNT; iswitch++)
                                switchingIRBC[iswitch] = int64_t(switchingIRBC[iswitch] * ((1.0f - m_gametorate) * m_IRBC_count[j] / temp_sum_IRBC));

                            temp_sum_IRBC = int64_t((1.0 - m_gametorate) * m_IRBC_count[j]);
                        }
                    }

                    // Now switch to next stages based on predetermined number of switching IRBC's
                    tmpIRBCcount[j] = int64_t(tmpIRBCcount[j] + ((1.0 - m_gametorate) * m_IRBC_count[j] - temp_sum_IRBC) * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival);
                    if (InfectionMalariaConfig::antigen_switch_rate > 0)
                    {
                        #pragma loop(hint_parallel(8))
                        for ( int iswitch = 0; iswitch < SWITCHING_IRBC_VARIANT_COUNT; iswitch++)
                        {
                            tmpIRBCcount[(j + iswitch + 1) % CLONAL_PfEMP1_VARIANTS]  = int64_t(tmpIRBCcount[(j + iswitch + 1) % CLONAL_PfEMP1_VARIANTS] + switchingIRBC[iswitch] * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival);
                        }
                    }

                }
                break;

            case ParasiteSwitchType::RATE_PER_PARASITE_5VARS_DECAYING:
                {
                    switchingIRBC[0] = parent->GetRng()->Poisson(1.000 * InfectionMalariaConfig::antigen_switch_rate * m_IRBC_count[j]);  // This switching model preferentially swtiches to the next one in the queue, with later variants being less probable
                    switchingIRBC[1] = parent->GetRng()->Poisson(0.200 * InfectionMalariaConfig::antigen_switch_rate * m_IRBC_count[j]);  // Switching to five variants is the current option, this can be varied by the programmer for testing
                    switchingIRBC[2] = parent->GetRng()->Poisson(0.040 * InfectionMalariaConfig::antigen_switch_rate * m_IRBC_count[j]);  // This switch rate model is NOT the canonical model of the Intrahost paper
                    switchingIRBC[3] = parent->GetRng()->Poisson(0.010 * InfectionMalariaConfig::antigen_switch_rate * m_IRBC_count[j]);  //
                    switchingIRBC[4] = parent->GetRng()->Poisson(0.002 * InfectionMalariaConfig::antigen_switch_rate * m_IRBC_count[j]);  //
                    int64_t temp_sum_IRBC = 0;
                    temp_sum_IRBC = std::accumulate(switchingIRBC, switchingIRBC + 5, temp_sum_IRBC);

                    tmpIRBCcount[j]                     = int64_t(tmpIRBCcount[j] + ( (1.0 - m_gametorate) * m_IRBC_count[j] - temp_sum_IRBC ) * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival);
                    tmpIRBCcount[(j + 1) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 1) % CLONAL_PfEMP1_VARIANTS] + switchingIRBC[0] * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival); // 
                    tmpIRBCcount[(j + 2) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 2) % CLONAL_PfEMP1_VARIANTS] + switchingIRBC[1] * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival); // 
                    tmpIRBCcount[(j + 3) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 3) % CLONAL_PfEMP1_VARIANTS] + switchingIRBC[2] * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival); // 
                    tmpIRBCcount[(j + 4) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 4) % CLONAL_PfEMP1_VARIANTS] + switchingIRBC[3] * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival); // 
                    tmpIRBCcount[(j + 5) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 5) % CLONAL_PfEMP1_VARIANTS] + switchingIRBC[4] * InfectionMalariaConfig::merozoites_per_schizont * merozoitesurvival); // 
                }
                break;

            default:
                {
                    // OLD SWITCHING brought down from processEndOfAsexualCycle body
                    // TODO: deprecate
                    int64_t antigenswitch = parent->GetRng()->SmartDraw( InfectionMalariaConfig::antigen_switch_rate ) ? 1 : 0;
                    double samerate = InfectionMalariaConfig::merozoites_per_schizont * ((1.0 - m_gametorate) - 0.02 * antigenswitch);
                    double nextrate = InfectionMalariaConfig::merozoites_per_schizont * (0.01 * antigenswitch);
                    tmpIRBCcount[j]                                = int64_t(tmpIRBCcount[j]                                + m_IRBC_count[j] * samerate * merozoitesurvival);
                    tmpIRBCcount[(j + 1) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 1) % CLONAL_PfEMP1_VARIANTS] + m_IRBC_count[j] * nextrate * merozoitesurvival); // mod(CLONAL_PfEMP1_VARIANTS) allows wrapping at end of queue
                    tmpIRBCcount[(j + 2) % CLONAL_PfEMP1_VARIANTS] = int64_t(tmpIRBCcount[(j + 2) % CLONAL_PfEMP1_VARIANTS] + m_IRBC_count[j] * nextrate * merozoitesurvival);
                }
                break;
            }
        }

        LOG_VALID_F("m_IRBC_count (previous) = %s\n", ValidateIRBCCounts().c_str());
        m_IRBC_count.swap(tmpIRBCcount); // swap temporarily accumulated vector of next time step into data member
        LOG_VALID_F("m_IRBC_count (updated) = %s\n", ValidateIRBCCounts().c_str());

    } // end malariaIRBCAntigenSwitch()

    // Moves all falciparum gametocytes forward a development stage when an asexual cycle completes, and creates the stage 0 immature gametocytes
    void InfectionMalaria::malariaCycleGametocytes(double merozoitesurvival)
    {
        // set gametocyte production rate for next cycle
        if ( m_asexual_cycle_count >= InfectionMalariaConfig::n_asexual_cycles_wo_gametocytes )
        {
            m_gametorate     = double(InfectionMalariaConfig::base_gametocyte_production); // gametocyte production used by all switching calculations, here is where factors modifying production would go
            m_gametosexratio = double(InfectionMalariaConfig::base_gametocyte_sexratio);
        }

        // check for valid range of input, and only create next cycle if valid
        if (merozoitesurvival >= 0)
        {
            LOG_VALID_F("m_female_gametocytes (previous) = %s\n", ValidateGametocyteCounts().c_str());
            #pragma loop(hint_parallel(8))
            //process gametocytes--5 stages--Sinden, R. E., G. A. Butcher, et al. (1996). "Regulation of Infectivity of Plasmodium to the Mosquito Vector." Advances in Parasitology 38: 53-117.
            for (int j = GametocyteStages::Mature; j > 0; j--) // move developing gametocytes forward a class, moving backwards through stages to not override next stage's values
            {
                m_malegametocytes[j] = int64_t(m_malegametocytes[j] + m_malegametocytes[j - 1] * InfectionMalariaConfig::gametocyte_stage_survival);
                m_malegametocytes[j - 1] = 0;

                if (m_malegametocytes[j] < 1)
                    m_malegametocytes[j] = 0;

                m_femalegametocytes[j] = int64_t(m_femalegametocytes[j] + (m_femalegametocytes[j - 1] * InfectionMalariaConfig::gametocyte_stage_survival));
                m_femalegametocytes[j - 1] = 0;

                if (m_femalegametocytes[j] < 1)
                     m_femalegametocytes[j] = 0;
            }

            LOG_VALID_F("m_female_gametocytes (cycled) = %s\n", ValidateGametocyteCounts().c_str());
            #pragma loop(hint_parallel(8))
            // Now create the new stage 1 gametocytes based on production ratios and the prevIRBC counts
            for (int j = 0; j < CLONAL_PfEMP1_VARIANTS; j++)
            {
                // review of production rates and sex ratios in Sinden, R. E., G. A. Butcher, et al. (1996). "Regulation of Infectivity of Plasmodium to the Mosquito Vector." Advances in Parasitology 38: 53-117.
                // each factor may be variable, but here we leave it constant at the moment, conservatively not including the possible senescence of transmission in late infection
                m_malegametocytes[GametocyteStages::Stage0]   = int64_t(m_malegametocytes[GametocyteStages::Stage0]   + m_IRBC_count[j] * m_gametorate * m_gametosexratio * merozoitesurvival * InfectionMalariaConfig::merozoites_per_schizont);
                m_femalegametocytes[GametocyteStages::Stage0] = int64_t(m_femalegametocytes[GametocyteStages::Stage0] + m_IRBC_count[j] * m_gametorate * (1.0 - m_gametosexratio) * merozoitesurvival * InfectionMalariaConfig::merozoites_per_schizont);
            }
            LOG_VALID_F("m_female_gametocytes (updated) = %s\n", ValidateGametocyteCounts().c_str());
        }
    }

    std::string InfectionMalaria::ValidateGametocyteCounts() const
    {
        std::ostringstream oss;
        oss << "[ ";
        for(int i=0; i < GametocyteStages::Count; i++) 
            oss << m_femalegametocytes[i] << " ";
        oss << "]";
        return oss.str();
    }

    std::string InfectionMalaria::ValidateIRBCCounts() const
    {
        std::ostringstream oss;
        oss << "[ ";
        for(int i=0; i<CLONAL_PfEMP1_VARIANTS; i++)
            oss << m_IRBC_count[i] << " ";
        oss << "]";
        return oss.str();
    }

    // Process all infected hepatocytes
    void InfectionMalaria::malariaProcessHepatocytes(float dt, IMalariaSusceptibility *immunity)
    {
        // check for valid inputs
        if (dt > 0 && immunity && m_hepatocytes > 0)
        {
            // ability to query for drug effects
            IIndividualHumanContext *patient = GetParent();
            IIndividualHumanInterventionsContext *context = patient->GetInterventionsContext();

            if( m_pMDE == nullptr )
            {
                context->QueryInterface( GET_IID( IMalariaDrugEffects ), (void **)&m_pMDE );
            }

            double drug_killrate = 0;
            if( m_pMDE != nullptr )
            {
                drug_killrate = m_pMDE->get_drug_hepatocyte( *infection_strain );
            }

            //binomial chance of survival
            if (drug_killrate > 0)
            {
                int tempval1 = 0;
                double pkill = EXPCDF(-dt * (drug_killrate));

                for (int i = 0; i < m_hepatocytes; i++)
                {
                    if( parent->GetRng()->SmartDraw( pkill ) )
                        tempval1++;
                }

                m_hepatocytes -= tempval1;
            }

            // ----------------------------------------------------------------------------------------------------------------------
            // --- latency in hepatocyte phase Collins, W. E. and G. M. Jeffery (1999). 
            // --- "A retrospective examination of sporozoite- and trophozoite-induced infections with Plasmodium falciparum:
            // --- development of parasitologic and clinical immunity during primary infection." Am J Trop Med Hyg 61(1 Suppl): 4-19.
            // --- process start of asexual phase if the incubation period is over and there are still hepatocytes
            // ----------------------------------------------------------------------------------------------------------------------
            if (m_asexual_phase == AsexualCycleStatus::NoAsexualCycle && duration >= incubation_timer )
            {
                m_IRBC_count.assign(CLONAL_PfEMP1_VARIANTS, 0);

                // testing starting with multiple antigens, which reduces the probability of a single first variant being cleared by a pre-existing antibody response
                // picked starting with 5 variants after exploring different options in work developing Intrahost model
                const int INITIAL_PFEMP1_VARIANTS = 5;
                #pragma loop(hint_parallel(8))
                for ( int i=0; i<INITIAL_PFEMP1_VARIANTS; i++ )
                {
                    m_IRBC_count[i] = int64_t(m_hepatocytes * InfectionMalariaConfig::merozoites_per_hepatocyte / INITIAL_PFEMP1_VARIANTS);
                    immunity->UpdateActiveAntibody( m_PfEMP1_antibodies[i], m_minor_epitope_type[i], m_IRBCtype[i] ); // insert into set of antigens the immune system has ever "seen"
                }

                // now back to normal
                m_hepatocytes   = 0;
                m_IRBCtimer     = 2.0;  // P. falciparum has a 2-day asexual cycle
                m_asexual_phase = AsexualCycleStatus::HepatocyteRelease;    // HepatocyteRelease means the asexual cycle is just beginning, so that the IRBCtimer is not decremented in the same time step
            }
        }
    }

    void InfectionMalaria::SetContextTo(IIndividualHumanContext *context)
    {
        Kernel::Infection::SetContextTo(context);

        // test more performant serialization
        IMalariaSusceptibility *immunity;
        if( s_OK != context->GetSusceptibilityContext()->QueryInterface(GET_IID(IMalariaSusceptibility), (void**)&immunity) )
        {
            LOG_WARN("Couldn't query for IMalariaSusceptibilityContext interface from ISusceptibilityContext\n");
            return;
        }

        m_MSP_antibody = immunity->RegisterAntibody(MalariaAntibodyType::MSP1, m_MSPtype);

        // Re-consider if only the non-zero IRBC antibodies need to be re-registered.
        for( int ivariant = 0; ivariant < m_PfEMP1_antibodies.size(); ivariant++ )
        {
            m_PfEMP1_antibodies[ivariant].minor  = immunity->RegisterAntibody(MalariaAntibodyType::PfEMP1_minor, m_minor_epitope_type[ivariant]);

            if ( m_IRBC_count[ivariant] > 0 )
                m_PfEMP1_antibodies[ivariant].major  = immunity->RegisterAntibody(MalariaAntibodyType::PfEMP1_major, m_IRBCtype[ivariant]);
        }

    }

    int64_t InfectionMalaria::get_MaleGametocytes(int stage) const
    { 
        return m_malegametocytes[stage]; 
    }

    int64_t ApplyKillProbability( float pkill, int64_t initial_gc, double eGauss )
    {
        double numkilled = (eGauss * sqrt( pkill * initial_gc * (1.0 - pkill) ) + pkill * initial_gc);
        numkilled = max( 0.0, numkilled ); //can't add by killing
        int64_t new_gc = int64_t( initial_gc - numkilled );
        return max( 0L, new_gc );
    }

    void InfectionMalaria::apply_MatureGametocyteKillProbability(float pkill)
    { 
        // Gaussian approximation of binomial errors for male and female mature gametocytes
        m_femalegametocytes[ GametocyteStages::Mature ] = ApplyKillProbability( pkill, m_femalegametocytes[ GametocyteStages::Mature ], parent->GetRng()->eGauss() );
        m_malegametocytes[ GametocyteStages::Mature ] = ApplyKillProbability(   pkill, m_malegametocytes[   GametocyteStages::Mature ], parent->GetRng()->eGauss() );
    }

    int64_t InfectionMalaria::get_FemaleGametocytes(int stage) const
    { 
        return m_femalegametocytes[stage]; 
    }

    float InfectionMalaria::get_asexual_density() const
    { 
        int64_t totalIRBC = CalculateTotalIRBC();
        return totalIRBC * m_inv_microliters_blood;
    }

    float InfectionMalaria::get_mature_gametocyte_density() const
    {
        int64_t mature_female_gametocytes = get_FemaleGametocytes(GametocyteStages::Mature);
        return mature_female_gametocytes * m_inv_microliters_blood;
    }

    int64_t InfectionMalaria::get_irbc() const
    {
        return CalculateTotalIRBC();
    }

    MalariaInfectionStage::Enum InfectionMalaria::get_InfectionStage() const
    {
        return m_CurrentInfectionStage;
    }

    bool InfectionMalaria::did_InfectionStageChangeToday() const
    {
        // assume that timestep is 1
        return (m_TimeSinceInfectionStageChange <= 1.0);
    }

    int64_t InfectionMalaria::CalculateTotalIRBC() const
    {
        return std::accumulate( m_IRBC_count.begin(), m_IRBC_count.end(), int64_t(0) );
    }

    REGISTER_SERIALIZABLE(InfectionMalaria);

    void InfectionMalaria::serialize(IArchive& ar, InfectionMalaria* obj)
    {
        InfectionVector::serialize(ar, obj);
        InfectionMalaria& infection = *obj;
        ar.labelElement("m_IRBCtimer") & infection.m_IRBCtimer;
        ar.labelElement("m_hepatocytes") & infection.m_hepatocytes;
        ar.labelElement("m_asexual_phase") & (uint32_t&)infection.m_asexual_phase;
        ar.labelElement("m_asexual_cycle_count") & infection.m_asexual_cycle_count;
        ar.labelElement("m_MSPtype") & infection.m_MSPtype;
        ar.labelElement("m_nonspectype") & infection.m_nonspectype;
        ar.labelElement("m_minor_epitope_type"); ar.serialize(infection.m_minor_epitope_type, CLONAL_PfEMP1_VARIANTS);
        ar.labelElement("m_IRBCtype"); ar.serialize(infection.m_IRBCtype, CLONAL_PfEMP1_VARIANTS);
// shared pointer        ar.labelElement("m_MSP_antibody"); Kernel::serialize<MalariaAntibody>(ar, infection.m_MSP_antibody);
// recreated in SetContextTo         ar.labelElement("m_PfEMP1_antibodies") & infection.m_PfEMP1_antibodies;
        ar.labelElement("m_IRBC_count") & infection.m_IRBC_count;
        ar.labelElement("m_malegametocytes"); ar.serialize(infection.m_malegametocytes, GametocyteStages::Count);
        ar.labelElement("m_femalegametocytes"); ar.serialize(infection.m_femalegametocytes, GametocyteStages::Count);
        ar.labelElement("m_gametorate") & infection.m_gametorate;
        ar.labelElement("m_gametosexratio") & infection.m_gametosexratio;
        ar.labelElement("m_measured_duration") & infection.m_measured_duration;
        ar.labelElement("m_start_measuring") & infection.m_start_measuring;
        ar.labelElement("m_temp_duration") & infection.m_temp_duration;
        ar.labelElement("m_max_parasites") & infection.m_max_parasites;
        ar.labelElement("m_inv_microliters_blood") & infection.m_inv_microliters_blood;
    }
}
