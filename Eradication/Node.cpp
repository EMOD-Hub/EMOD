
#include "stdafx.h"
#include "NodeDemographics.h"
#include "Node.h"
#include "ISimulationContext.h"
#include "ISimulation.h"

#include <iostream>
#include <algorithm>
#include <numeric>

#include "Debug.h" // for release_assert
#include "MathFunctions.h" // for fromDistribution
#include "Common.h"
#include "IIndividualHumanContext.h"
#include "Exceptions.h"
#include "InterventionEnums.h"
#include "Types.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "TransmissionGroupMembership.h"
#include "TransmissionGroupsFactory.h"
#include "Report.h" // before we were including Simulation.h to get IReport! Very bad.
#include "SimulationConfig.h"
#include "StatusReporter.h" // for initialization progress
#include "StrainIdentity.h"
#include "IInfectable.h"
#include "FileSystem.h"
#include "IMigrationInfo.h"
#include "Individual.h"
#include "IntranodeTransmissionTypes.h"
#include "SerializationParameters.h"
#include "IdmMpi.h"
#include "EventTrigger.h"
#include "IdmDateTime.h"
#include "RANDOM.h"
#include "Susceptibility.h" // for susceptibility_initialization_distribution_type
#include "DistributionFactory.h"
#include "Infection.h"

SETUP_LOGGING( "Node" )


#include "Properties.h"

namespace Kernel
{
    // QI stoff in case we want to use it more extensively
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Node,Node)

    //------------------------------------------------------------------
    //   Initialization methods
    //------------------------------------------------------------------
    SerializationBitMask_t Node::serializationFlagsDefault = SerializationBitMask_t{}.set( SerializationFlags::Population )
                                                           | SerializationBitMask_t{}.set( SerializationFlags::Parameters );

    // <ERAD-291>
    // TODO: Make simulation object initialization more consistent.  Either all pass contexts to constructors or just have empty constructors
    Node::Node(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid _suid)
        : serializationFlags( 0 )     // "Initialize" in ::serialize
        , SusceptibilityDistribution( nullptr )
        , FertilityDistribution( nullptr )
        , MortalityDistribution( nullptr )
        , MortalityDistributionMale( nullptr )
        , MortalityDistributionFemale( nullptr )
        , AgeDistribution( nullptr )
        , _latitude(FLT_MAX)
        , _longitude(FLT_MAX)
        , ind_sampling_type( IndSamplingType::TRACK_ALL )
        , population_density_infectivity_correction( PopulationDensityInfectivityCorrection::CONSTANT_INFECTIVITY )
        , age_initialization_distribution_type(DistributionType::DISTRIBUTION_OFF)
        , population_scaling(PopulationScaling::USE_INPUT_FILE)
        , susceptibility_scaling_type( SusceptibilityScalingType::CONSTANT_SUSCEPTIBILITY )
        , susceptibility_scaling( false )
        , susceptibility_scaling_rate( 1.0f )
        , susceptibility_dynamic_scaling( 1.0f )
        , suid(_suid)
        , birthrate(DEFAULT_BIRTHRATE)
        , individualHumans()
        , home_individual_ids()
        , family_waiting_to_migrate(false)
        , family_migration_destination(suids::nil_suid())
        , family_migration_type(MigrationType::NO_MIGRATION)
        , family_time_until_trip(0.0f)
        , family_time_at_destination(0.0f)
        , family_is_destination_new_home(false)
        , transmissionGroups( nullptr )
        , localWeather(nullptr)
        , migration_info(nullptr)
        , demographics()
        , externalId(externalNodeId)
        , node_properties()
        , event_context_host(nullptr)
        , events_from_other_nodes()
        , statPop(0)
        , Infected(0)
        , Births(0.0f)
        , Disease_Deaths(0.0f)
        , new_infections(0.0f)
        , new_reportedinfections(0.0f)
        , Cumulative_Infections(0.0f)
        , Cumulative_Reported_Infections(0.0f)
        , Campaign_Cost(0.0f)
        , Possible_Mothers(0)
        , mean_age_infection(0.0f)
        , newInfectedPeopleAgeProduct(0.0f)           // counters starting with this one were only used for old spatial reporting and can probably now be removed
        , infected_people_prior()
        , infected_age_people_prior()
        , mInfectivity(0.0f)
        , parent( nullptr )
        , parent_sim( nullptr )
        , enable_demographics_risk(false)
        , base_sample_rate(1.0f)
        , max_sampling_cell_pop(0.0f)
        , sample_rate_birth(0.0f)
        , sample_rate_0_18mo(0.0f)
        , sample_rate_18mo_4yr(0.0f)
        , sample_rate_5_9(0.0f)
        , sample_rate_10_14(0.0f)
        , sample_rate_15_19(0.0f)
        , sample_rate_20_plus(0.0f)
        , rel_sample_rate_immune(0.0f)
        , immune_threshold_for_downsampling(0.0f)
        , prob_maternal_infection_transmission(0.0f)
        , population_density_c50(0.0f)
        , population_scaling_factor(1.0f)
        , vital_dynamics(false)
        , enable_natural_mortality(false)
        , enable_maternal_infection_transmission(false)
        , enable_initial_prevalence(false)
        , enable_infectivity_reservoir(false)
        , vital_birth(false)
        , vital_birth_dependence(VitalBirthDependence::FIXED_BIRTH_RATE)
        , vital_birth_time_dependence(VitalBirthTimeDependence::NONE)
        , x_birth(1.0f)
        , x_othermortality(1.0f)
        , susceptibility_dist_type(DistributionFunction::CONSTANT_DISTRIBUTION)
        , susceptibility_dist1(0.0f)
        , susceptibility_dist2(0.0f)
        , risk_dist_type(DistributionFunction::CONSTANT_DISTRIBUTION)
        , risk_dist1(0.0f)
        , risk_dist2(0.0f)
        , migration_dist_type(DistributionFunction::CONSTANT_DISTRIBUTION)
        , migration_dist1(0.0f)
        , migration_dist2(0.0f)
        , infectivity_scaling(InfectivityScaling::CONSTANT_INFECTIVITY)
        , routes()
        , infectivity_sinusoidal_forcing_amplitude( 1.0f )
        , infectivity_sinusoidal_forcing_phase( 0.0f )
        , infectivity_boxcar_forcing_amplitude( 1.0f )
        , infectivity_boxcar_start_time( 0.0 )
        , infectivity_boxcar_end_time( 0.0 )
        , infectivity_exponential_baseline( 1.0f )
        , infectivity_exponential_rate( 1.0f )
        , infectivity_exponential_delay (0.0f )
        , infectivity_reservoir_end_time( 0.0f )
        , infectivity_reservoir_size( 0.0f )
        , infectivity_reservoir_start_time( 0.0f )
        , birth_rate_sinusoidal_forcing_amplitude( 1.0f )
        , birth_rate_sinusoidal_forcing_phase( 0.0f )
        , birth_rate_boxcar_forcing_amplitude( 1.0f )
        , birth_rate_boxcar_start_time( 0.0 )
        , birth_rate_boxcar_end_time( 0.0 )
        , bSkipping( false )
        , maxInfectionProb()
        , gap(0)
        , vital_death_dependence(VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER)
        , m_pRng( nullptr )
        , m_IndividualHumanSuidGenerator(0,0)
        , symptomatic( 0.0f )
        , newly_symptomatic( 0.0f )
    {
        SetContextTo(_parent_sim);  // TODO - this should be a virtual function call, but it isn't because the constructor isn't finished running yet.
    }

    Node::Node()
        : serializationFlags( 0 )     // "Initialize" in ::serialize
        , SusceptibilityDistribution( nullptr )
        , FertilityDistribution( nullptr )
        , MortalityDistribution( nullptr )
        , MortalityDistributionMale( nullptr )
        , MortalityDistributionFemale( nullptr )
        , AgeDistribution( nullptr )
        , _latitude(FLT_MAX)
        , _longitude(FLT_MAX)
        , ind_sampling_type( IndSamplingType::TRACK_ALL )
        , population_density_infectivity_correction( PopulationDensityInfectivityCorrection::CONSTANT_INFECTIVITY )
        , age_initialization_distribution_type(DistributionType::DISTRIBUTION_OFF)
        , population_scaling(PopulationScaling::USE_INPUT_FILE)
        , susceptibility_scaling_type( SusceptibilityScalingType::CONSTANT_SUSCEPTIBILITY )
        , susceptibility_scaling( false )
        , susceptibility_scaling_rate( 1.0f )
        , susceptibility_dynamic_scaling( 1.0f )
        , suid(suids::nil_suid())
        , birthrate(DEFAULT_BIRTHRATE)
        , individualHumans()
        , home_individual_ids()
        , family_waiting_to_migrate(false)
        , family_migration_destination(suids::nil_suid())
        , family_migration_type(MigrationType::NO_MIGRATION)
        , family_time_until_trip(0.0f)
        , family_time_at_destination(0.0f)
        , family_is_destination_new_home(false)
        , transmissionGroups( nullptr )
        , localWeather(nullptr)
        , migration_info(nullptr)
        , demographics()
        , externalId(0)
        , node_properties()
        , event_context_host(nullptr)
        , events_from_other_nodes()
        , statPop(0)
        , Infected(0)
        , Births(0.0f)
        , Disease_Deaths(0.0f)
        , new_infections(0.0f)
        , new_reportedinfections(0.0f)
        , Cumulative_Infections(0.0f)
        , Cumulative_Reported_Infections(0.0f)
        , Campaign_Cost(0.0f)
        , Possible_Mothers(0)
        , mean_age_infection(0.0f)
        , newInfectedPeopleAgeProduct(0.0f)           // counters starting with this one were only used for old spatial reporting and can probably now be removed
        , infected_people_prior()
        , infected_age_people_prior()
        , mInfectivity(0.0f)
        , parent(nullptr)
        , parent_sim( nullptr )
        , enable_demographics_risk(false)
        , base_sample_rate(1.0f)
        , max_sampling_cell_pop(0.0f)
        , sample_rate_birth(0.0f)
        , sample_rate_0_18mo(0.0f)
        , sample_rate_18mo_4yr(0.0f)
        , sample_rate_5_9(0.0f)
        , sample_rate_10_14(0.0f)
        , sample_rate_15_19(0.0f)
        , sample_rate_20_plus(0.0f)
        , rel_sample_rate_immune(0.0f)
        , immune_threshold_for_downsampling(0.0f)
        , prob_maternal_infection_transmission(0.0f)
        , population_density_c50(0.0f)
        , population_scaling_factor(1.0f)
        , vital_dynamics(false)
        , enable_natural_mortality(false)
        , enable_maternal_infection_transmission(false)
        , enable_initial_prevalence(false)
        , enable_infectivity_reservoir(false)
        , vital_birth(false)
        , vital_birth_dependence(VitalBirthDependence::FIXED_BIRTH_RATE)
        , vital_birth_time_dependence(VitalBirthTimeDependence::NONE)
        , x_birth(1.0f)
        , x_othermortality(1.0f)
        , susceptibility_dist_type(DistributionFunction::CONSTANT_DISTRIBUTION)
        , susceptibility_dist1(0.0f)
        , susceptibility_dist2(0.0f)
        , risk_dist_type(DistributionFunction::CONSTANT_DISTRIBUTION)
        , risk_dist1(0.0f)
        , risk_dist2(0.0f)
        , migration_dist_type(DistributionFunction::CONSTANT_DISTRIBUTION)
        , migration_dist1(0.0f)
        , migration_dist2(0.0f)
        , infectivity_scaling(InfectivityScaling::CONSTANT_INFECTIVITY)
        , routes()
        , infectivity_sinusoidal_forcing_amplitude( 1.0f )
        , infectivity_sinusoidal_forcing_phase( 0.0f )
        , infectivity_boxcar_forcing_amplitude( 1.0f )
        , infectivity_boxcar_start_time( 0.0 )
        , infectivity_boxcar_end_time( 0.0 )
        , infectivity_exponential_baseline( 1.0f )
        , infectivity_exponential_rate( 1.0f )
        , infectivity_exponential_delay (0.0f )
        , infectivity_reservoir_end_time( 0.0f )
        , infectivity_reservoir_size( 0.0f )
        , infectivity_reservoir_start_time( 0.0f )
        , birth_rate_sinusoidal_forcing_amplitude( 1.0f )
        , birth_rate_sinusoidal_forcing_phase( 0.0f )
        , birth_rate_boxcar_forcing_amplitude( 1.0f )
        , birth_rate_boxcar_start_time( 0.0 )
        , birth_rate_boxcar_end_time( 0.0 )
        , bSkipping( false )
        , maxInfectionProb()
        , gap(0)
        , vital_death_dependence(VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER)
        , m_pRng( nullptr )
        , m_IndividualHumanSuidGenerator(0,0)
        , symptomatic( 0.0f )
        ,newly_symptomatic( 0.0f )
    {
    }

    Node::~Node()
    {
        if (suid.data % 10 == 0) LOG_INFO_F("Freeing Node %d \n", suid.data);

        /* Let all of this dangle, we're about to exit the process...
        for (auto individual : individualHumans)
        {
            delete individual;
        }

        individualHumans.clear();
        home_individual_ids.clear();

        if (transmissionGroups) delete transmissionGroups;
        if (migration_info)     delete migration_info;

        delete event_context_host;

        delete SusceptibilityDistribution;
        delete FertilityDistribution;
        delete MortalityDistribution;
        delete MortalityDistributionMale;
        delete MortalityDistributionFemale;
        delete AgeDistribution;
        */
    }

    float Node::GetLatitudeDegrees()
    {
        if( _latitude == FLT_MAX )
        {
            _latitude  = float(demographics["NodeAttributes"]["Latitude"].AsDouble());
        }
        return _latitude ;
    }

    float Node::GetLongitudeDegrees()
    {
        if( _longitude == FLT_MAX )
        {
            _longitude = float(demographics["NodeAttributes"]["Longitude"].AsDouble());
        }
        return _longitude ;
    }
        
    QueryResult Node::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        ISupports* foundInterface;
        if ( iid == GET_IID(INodeContext)) 
            foundInterface = static_cast<INodeContext*>(this);
        else if ( iid == GET_IID(ISupports) )
            foundInterface = static_cast<ISupports*>(static_cast<INodeContext*>(this));
        else
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    Node *Node::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        Node *newnode = _new_ Node(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();

        return newnode;
    }

    bool Node::Configure( const Configuration* config )
    {
        initConfigTypeMap( "Enable_Vital_Dynamics", &vital_dynamics, Enable_Vital_Dynamics_DESC_TEXT, true );
        initConfigTypeMap( "Enable_Infectivity_Reservoir", &enable_infectivity_reservoir, Enable_Infectivity_Reservoir_DESC_TEXT, false );
        initConfigTypeMap( "Enable_Natural_Mortality", &enable_natural_mortality, Enable_Natural_Mortality_DESC_TEXT, false, "Enable_Vital_Dynamics" );
        initConfig( "Death_Rate_Dependence", vital_death_dependence, config, MetadataDescriptor::Enum(Death_Rate_Dependence_DESC_TEXT, Death_Rate_Dependence_DESC_TEXT, MDD_ENUM_ARGS(VitalDeathDependence)), "Enable_Natural_Mortality" ); // node only (move) 
        LOG_DEBUG_F( "Death_Rate_Dependence configured as %s\n", VitalDeathDependence::pairs::lookup_key( vital_death_dependence ) );

        // Susceptibility scaling options
        initConfigTypeMap("Enable_Susceptibility_Scaling", &susceptibility_scaling, Enable_Susceptibility_Scaling_DESC_TEXT, false);
        initConfig("Susceptibility_Scaling_Type", susceptibility_scaling_type, config, MetadataDescriptor::Enum("Susceptibility_Scaling_Type", Susceptibility_Scaling_Type_DESC_TEXT, MDD_ENUM_ARGS(SusceptibilityScalingType)),"Enable_Susceptibility_Scaling");
        initConfigTypeMap("Susceptibility_Scaling_Rate", &susceptibility_scaling_rate, Susceptibility_Scaling_Rate_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Susceptibility_Scaling_Type", "LOG_LINEAR_FUNCTION_OF_TIME");

        initConfig( "Age_Initialization_Distribution_Type", age_initialization_distribution_type, config, MetadataDescriptor::Enum(Age_Initialization_Distribution_Type_DESC_TEXT, Age_Initialization_Distribution_Type_DESC_TEXT, MDD_ENUM_ARGS(DistributionType)) );

        initConfig( "Infectivity_Scale_Type", infectivity_scaling, config, MetadataDescriptor::Enum("infectivity_scaling", Infectivity_Scale_Type_DESC_TEXT, MDD_ENUM_ARGS(InfectivityScaling)) );
        initConfigTypeMap( "Infectivity_Sinusoidal_Forcing_Amplitude", &infectivity_sinusoidal_forcing_amplitude, Infectivity_Sinusoidal_Forcing_Amplitude_DESC_TEXT, 0.0f, 1.0f,    0.0f, "Infectivity_Scale_Type", "SINUSOIDAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Infectivity_Sinusoidal_Forcing_Phase",     &infectivity_sinusoidal_forcing_phase,     Infectivity_Sinusoidal_Forcing_Phase_DESC_TEXT,     0.0f, 365.0f,  0.0f, "Infectivity_Scale_Type", "SINUSOIDAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Infectivity_Exponential_Baseline",         &infectivity_exponential_baseline,         Infectivity_Exponential_Baseline_DESC_TEXT,         0.0f, 1.0f,    0.0f, "Infectivity_Scale_Type", "EXPONENTIAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Infectivity_Exponential_Delay",            &infectivity_exponential_delay,            Infectivity_Exponential_Delay_DESC_TEXT,            0.0f, FLT_MAX, 0.0f, "Infectivity_Scale_Type", "EXPONENTIAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Infectivity_Exponential_Rate",             &infectivity_exponential_rate,             Infectivity_Exponential_Rate_DESC_TEXT,             0.0f, FLT_MAX, 0.0f, "Infectivity_Scale_Type", "EXPONENTIAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Infectivity_Boxcar_Forcing_Amplitude",     &infectivity_boxcar_forcing_amplitude,     Infectivity_Boxcar_Forcing_Amplitude_DESC_TEXT,     0.0f, FLT_MAX, 0.0f, "Infectivity_Scale_Type", "ANNUAL_BOXCAR_FUNCTION" );
        initConfigTypeMap( "Infectivity_Boxcar_Forcing_Start_Time",    &infectivity_boxcar_start_time,            Infectivity_Boxcar_Forcing_Start_Time_DESC_TEXT,    0.0f, 365.0f,  0.0f, "Infectivity_Scale_Type", "ANNUAL_BOXCAR_FUNCTION" );
        initConfigTypeMap( "Infectivity_Boxcar_Forcing_End_Time",      &infectivity_boxcar_end_time,              Infectivity_Boxcar_Forcing_End_Time_DESC_TEXT,      0.0f, 365.0f,  0.0f, "Infectivity_Scale_Type", "ANNUAL_BOXCAR_FUNCTION" );

        // TODO: there is conditionality in which of the following configurable parameters need to be read in based on the values of certain enums/booleans
        initConfigTypeMap( "x_Other_Mortality",               &x_othermortality,        x_Other_Mortality_DESC_TEXT,    0.0f, FLT_MAX, 1.0f, "Enable_Natural_Mortality" );
        
        initConfigTypeMap( "Enable_Birth",                    &vital_birth,             Enable_Birth_DESC_TEXT,         true,                "Enable_Vital_Dynamics" );

        initConfigTypeMap( "Enable_Demographics_Risk",        &enable_demographics_risk,       Enable_Demographics_Risk_DESC_TEXT,       false, "Simulation_Type", "VECTOR_SIM,MALARIA_SIM");  // DJK*: Should be "Enable_Risk_Factor_Heterogeneity_At_Birth"

        initConfigTypeMap( "Enable_Maternal_Infection_Transmission",    &enable_maternal_infection_transmission,   Enable_Maternal_Infection_Transmission_DESC_TEXT,   false, "Enable_Birth" );
        initConfigTypeMap( "Maternal_Infection_Transmission_Probability", &prob_maternal_infection_transmission, Maternal_Infection_Transmission_Probability_DESC_TEXT, 0.0f, 1.0f,    0.0f, "Enable_Maternal_Infection_Transmission" );

        initConfig( "Birth_Rate_Dependence", vital_birth_dependence, config, MetadataDescriptor::Enum(Birth_Rate_Dependence_DESC_TEXT, Birth_Rate_Dependence_DESC_TEXT, MDD_ENUM_ARGS(VitalBirthDependence)), "Enable_Birth" );

        initConfig( "Individual_Sampling_Type", ind_sampling_type, config, MetadataDescriptor::Enum("ind_sampling_type", Individual_Sampling_Type_DESC_TEXT, MDD_ENUM_ARGS(IndSamplingType)) );

        initConfigTypeMap( "Base_Individual_Sample_Rate",       &base_sample_rate,                  Base_Individual_Sample_Rate_DESC_TEXT,       0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "FIXED_SAMPLING,ADAPTED_SAMPLING_BY_IMMUNE_STATE");
        initConfigTypeMap( "Relative_Sample_Rate_Immune",       &rel_sample_rate_immune,            Relative_Sample_Rate_Immune_DESC_TEXT,       0.001f,  1.0f,  0.1f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_IMMUNE_STATE" );
        initConfigTypeMap( "Immune_Threshold_For_Downsampling", &immune_threshold_for_downsampling, Immune_Threshold_For_Downsampling_DESC_TEXT, 0.0f,    1.0f,  0.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_IMMUNE_STATE" );
        initConfigTypeMap( "Sample_Rate_Birth",                 &sample_rate_birth,                 Sample_Rate_Birth_DESC_TEXT,                 0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Sample_Rate_0_18mo",                &sample_rate_0_18mo,                Sample_Rate_0_18mo_DESC_TEXT,                0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Sample_Rate_18mo_4yr",              &sample_rate_18mo_4yr,              Sample_Rate_18mo_4yr_DESC_TEXT,              0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Sample_Rate_5_9",                   &sample_rate_5_9,                   Sample_Rate_5_9_DESC_TEXT,                   0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Sample_Rate_10_14",                 &sample_rate_10_14,                 Sample_Rate_10_14_DESC_TEXT,                 0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Sample_Rate_15_19",                 &sample_rate_15_19,                 Sample_Rate_15_19_DESC_TEXT,                 0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Sample_Rate_20_Plus",               &sample_rate_20_plus,               Sample_Rate_20_plus_DESC_TEXT,               0.001f,  1.0f,  1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        initConfigTypeMap( "Max_Node_Population_Samples",       &max_sampling_cell_pop,             Max_Node_Population_Samples_DESC_TEXT,       1.0f, FLT_MAX, 30.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_POPULATION_SIZE,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );

        initConfig( "Population_Scale_Type", population_scaling,  config, MetadataDescriptor::Enum(Population_Scale_Type_DESC_TEXT, Population_Scale_Type_DESC_TEXT, MDD_ENUM_ARGS(PopulationScaling)) );
        initConfigTypeMap( "x_Base_Population", &population_scaling_factor,  x_Base_Population_DESC_TEXT,      0.0f, FLT_MAX, 1.0f, "Population_Scale_Type", "FIXED_SCALING" );

        initConfig( "Population_Density_Infectivity_Correction", population_density_infectivity_correction, config, MetadataDescriptor::Enum("population_density_infectivity_correction", Population_Density_Infectivity_Correction_DESC_TEXT, MDD_ENUM_ARGS(PopulationDensityInfectivityCorrection)) ); // node only (move)
        initConfigTypeMap( "Population_Density_C50", &population_density_c50, Population_Density_C50_DESC_TEXT, 0.0f, FLT_MAX, 10.0f, "Population_Density_Infectivity_Correction", PopulationDensityInfectivityCorrection::pairs::lookup_key( PopulationDensityInfectivityCorrection::SATURATING_FUNCTION_OF_DENSITY ) );

        initConfigTypeMap( "x_Birth",                           &x_birth,                    x_Birth_DESC_TEXT,                           0.0f, FLT_MAX, 1.0f, "Enable_Birth" );


        initConfig( "Birth_Rate_Time_Dependence", vital_birth_time_dependence, config, MetadataDescriptor::Enum("vital_birth_time_dependence", Birth_Rate_Time_Dependence_DESC_TEXT, MDD_ENUM_ARGS(VitalBirthTimeDependence)), "Enable_Birth" ); // node only (move)
        initConfigTypeMap( "Birth_Rate_Sinusoidal_Forcing_Amplitude", &birth_rate_sinusoidal_forcing_amplitude, Birth_Rate_Sinusoidal_Forcing_Amplitude_DESC_TEXT, 0.0f,    1.0f, 0.0f, "Birth_Rate_Time_Dependence", "SINUSOIDAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Birth_Rate_Sinusoidal_Forcing_Phase",     &birth_rate_sinusoidal_forcing_phase,     Birth_Rate_Sinusoidal_Forcing_Phase_DESC_TEXT,     0.0f,  365.0f, 0.0f, "Birth_Rate_Time_Dependence", "SINUSOIDAL_FUNCTION_OF_TIME" );
        initConfigTypeMap( "Birth_Rate_Boxcar_Forcing_Amplitude",     &birth_rate_boxcar_forcing_amplitude,     Birth_Rate_Boxcar_Forcing_Amplitude_DESC_TEXT,     0.0f, FLT_MAX, 0.0f, "Birth_Rate_Time_Dependence", "ANNUAL_BOXCAR_FUNCTION" );
        initConfigTypeMap( "Birth_Rate_Boxcar_Forcing_Start_Time",    &birth_rate_boxcar_start_time,            Birth_Rate_Boxcar_Forcing_Start_Time_DESC_TEXT,    0.0f,  365.0f, 0.0f, "Birth_Rate_Time_Dependence", "ANNUAL_BOXCAR_FUNCTION" );
        initConfigTypeMap( "Birth_Rate_Boxcar_Forcing_End_Time",      &birth_rate_boxcar_end_time,              Birth_Rate_Boxcar_Forcing_End_Time_DESC_TEXT,      0.0f,  365.0f, 0.0f, "Birth_Rate_Time_Dependence", "ANNUAL_BOXCAR_FUNCTION" );

        initConfigTypeMap( "Enable_Initial_Prevalence", &enable_initial_prevalence, Enable_Initial_Prevalence_DESC_TEXT, false, "Simulation_Type", "GENERIC_SIM, VECTOR_SIM, STI_SIM, MALARIA_SIM" );

        bool ret = JsonConfigurable::Configure( config );

        return ret;
    }

    void Node::Initialize()
    {
        // -----------------------------------------------------
        // --- Call this here for normal setup, but it is 
        // --- called in Simulation::populateFromDemographics()
        // --- when reading from a serialized population file
        // -----------------------------------------------------
        SetupEventContextHost();

        Configure( EnvPtr->Config );

        if(susceptibility_scaling_type == SusceptibilityScalingType::LOG_LINEAR_FUNCTION_OF_TIME)
        {
            susceptibility_dynamic_scaling = 0.0f; // set susceptibility to zero so it may ramp up over time according to the scaling function
        }
    }

    void Node::SetupEventContextHost()
    {
        event_context_host = _new_ NodeEventContextHost(this);
    }

    void Node::SetupMigration( IMigrationInfoFactory * migration_factory, 
                               const std::string& idreference,
                               MigrationStructure::Enum ms,
                               const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        migration_dist_type = DistributionFunction::CONSTANT_DISTRIBUTION ;
        migration_dist1 = 0.0 ;
        migration_dist2 = 0.0 ;

        if( ms != MigrationStructure::NO_MIGRATION )
        {
            migration_info = migration_factory->CreateMigrationInfo( this, rNodeIdSuidMap );
            release_assert( migration_info != nullptr );

            if( migration_info->IsHeterogeneityEnabled() )
            {
                LOG_DEBUG( "Parsing MigrationHeterogeneityDistribution\n" );
                migration_dist_type = (DistributionFunction::Enum)(demographics["IndividualAttributes"]["MigrationHeterogeneityDistributionFlag"].AsInt());
                migration_dist1     = (float)                     (demographics["IndividualAttributes"]["MigrationHeterogeneityDistribution1"   ].AsDouble());
                migration_dist2     = (float)                     (demographics["IndividualAttributes"]["MigrationHeterogeneityDistribution2"   ].AsDouble());

                std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( migration_dist_type ) );
                distribution->SetParameters( migration_dist1, migration_dist2, 0.0 );
                
                for( auto ind : this->individualHumans )
                {
                    // this is only done during initialization.  During the sim, configureAndAddNewIndividual() will set this
                    float temp_migration = distribution->Calculate( GetRng() );
                    ind->SetMigrationModifier( temp_migration );
                }
            }                          
        }
    }

    void Node::SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory )
    {
        // Parameters set from an input filestream
        // TODO: Jeff, this is a bit hack-y that I had to do this. is there a better way?
        NodeDemographics *demographics_temp = demographics_factory->CreateNodeDemographics(this);
        release_assert( demographics_temp );
        demographics = *(demographics_temp); // use copy constructor
        delete demographics_temp;
        uint32_t temp_externalId = demographics["NodeID"].AsUint();
        release_assert( this->externalId == temp_externalId );

        //////////////////////////////////////////////////////////////////////////////////////
        // Hack: commenting out for pymod work. Need real solution once I understand all this.
        if( NPFactory::GetInstance() )
        {
            node_properties = NPFactory::GetInstance()->GetInitialValues( GetRng(), demographics.GetJsonObject() );
        }

        LOG_DEBUG( "Looking for Individual_Properties in demographics.json file(s)\n" );
        if( IPFactory::GetInstance() )
        {
            IPFactory::GetInstance()->Initialize( GetExternalID(), demographics.GetJsonObject() );
        }
        //////////////////////////////////////////////////////////////////////////////////////

        birthrate = float(demographics["NodeAttributes"]["BirthRate"].AsDouble());
        
        release_assert(params());

        if (enable_natural_mortality) // this break pymod
        {
            if( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER )
            {
                LOG_DEBUG( "Parsing IndividualAttributes->MortalityDistribution tag in node demographics file.\n" );
                MortalityDistribution = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistribution"], "gender", "age");
            }
            else if( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER )
            {
                if (vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER)
                {
                    LOG_DEBUG("Parsing IndividualAttributes->MortalityDistribution tag in node demographics file.\n");
                    MortalityDistribution = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistribution"], "gender", "age");
                }
                else if (vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER)
                {
                    LOG_DEBUG("Parsing IndividualAttributes->MortalityDistributionMale and IndividualAttributes->MortalityDistributionFemale tags in node demographics file.\n");
                    MortalityDistributionMale = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistributionMale"], "age", "year");
                    MortalityDistributionFemale = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistributionFemale"], "age", "year");
                }
            }
        }

        LoadOtherDiseaseSpecificDistributions();

        if (vital_birth_dependence != VitalBirthDependence::FIXED_BIRTH_RATE)// births per cell per day is population dependent
        {
            // If individual pregnancies will begin based on age-dependent fertility rates, create the relevant distribution here:
            if (vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR)
            {
                LOG_DEBUG( "Parsing IndividualAttributes->FertilityDistribution tag in node demographics file.\n" );
                FertilityDistribution = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["FertilityDistribution"], "age", "year");
            }

            if (birthrate > BIRTHRATE_SANITY_VALUE)
            {
                // report error message to error file and error iostream
                // ERROR: ("Check birthrate/vital_birth_dependence mismatch in Node::SetParameters()\n");
                //throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__ );

                // Error handling in execution without instant termination
                // still allow simulation to run, but set birthrate depending on value of vital_birth_dependence
                if (vital_birth_dependence == VitalBirthDependence::POPULATION_DEP_RATE)
                {
                    birthrate = float(TWO_PERCENT_PER_YEAR); // TBD: literal should be defined as float
                }
                else if ( vital_birth_dependence == VitalBirthDependence::DEMOGRAPHIC_DEP_RATE   ||
                    vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES ||
                    vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR )
                {
                    birthrate = float(INDIVIDUAL_BIRTHRATE); // TBD: literal should be defined as float // DJK: why is this even needed for age-specific fertility?
                }
                else
                {
                    birthrate = float(FALLBACK_BIRTHRATE);
                }
            }
        }
        
        ExtractDataFromDemographics();

#ifndef DISABLE_CLIMATE
        if (ClimateFactory::climate_structure != ClimateStructure::CLIMATE_OFF)
        {
            LOG_DEBUG( "Parsing NodeAttributes->Altitude tag in node demographics file.\n" );
            float altitude = float(demographics["NodeAttributes"]["Altitude"].AsDouble());
            localWeather = climate_factory->CreateClimate( this, altitude, GetLatitudeDegrees(), GetRng() );
        }
#endif

        SetupIntranodeTransmission();
    }

    void Node::LoadImmunityDemographicsDistribution()
    {
        // If not overridden, "SusceptibilityDistribution" provides age-specific probabilities of being susceptible (1.0 = not immune; 0.0 = immune)
        LOG_DEBUG( "Parsing IndividualAttributes->SusceptibilityDistribution tag in node demographics file.\n" );
        SusceptibilityDistribution = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["SusceptibilityDistribution"]);
    }

    ITransmissionGroups* Node::CreateTransmissionGroups()
    {
        return TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::StrainAwareGroups, GetRng() );
    }

    void Node::AddDefaultRoute( void )
    {
        AddRoute( "contact" );
    }

    void Node::AddRoute( const std::string& rRouteName )
    { 
        LOG_DEBUG_F("Adding route %s.\n", rRouteName.c_str());
        routes.push_back( rRouteName );
    }

    void Node::BuildTransmissionRoutes( float contagionDecayRate )
    {
        NaturalNumber antigenCount = InfectionConfig::number_basestrains;
        NaturalNumber substrainCount = InfectionConfig::number_substrains;
        transmissionGroups->Build( contagionDecayRate, antigenCount, substrainCount );
    }

    void Node::SetupIntranodeTransmission()
    {
        transmissionGroups = CreateTransmissionGroups();

        if( IPFactory::GetInstance() && IPFactory::GetInstance()->HasIPs() && params()->heterogeneous_intranode_transmission_enabled ) 
        {
            ValidateIntranodeTransmissionConfiguration();

            for( auto p_ip : IPFactory::GetInstance()->GetIPList() )
            {
                if( p_ip->GetIntraNodeTransmission( GetExternalID() ).HasMatrix() )
                {
                    std::string routeName = p_ip->GetIntraNodeTransmission( GetExternalID() ).GetRouteName();

                    AddRoute( routeName );

                    transmissionGroups->AddProperty( p_ip->GetKeyAsString(),
                                                     p_ip->GetValues<IPKeyValueContainer>().GetValuesToList(),
                                                     p_ip->GetIntraNodeTransmission( GetExternalID() ).GetMatrix() );
                                                     
                }
                else //HINT is enabled, but no transmission matrix is detected
                {
                    AddDefaultRoute();
                }
            }
        }
        else //HINT is not enabled
        {
            AddDefaultRoute();
        }

        BuildTransmissionRoutes( 1.0f );
    }

    void Node::GetGroupMembershipForIndividual(const RouteList_t& route, const IPKeyValueContainer& properties, TransmissionGroupMembership_t& transmissionGroupMembership)
    {
        LOG_DEBUG_F( "Calling GetGroupMembershipForProperties\n" );
        transmissionGroups->GetGroupMembershipForProperties(properties, transmissionGroupMembership );
    }

    std::map< std::string, float >
    Node::GetContagionByRoute()
    const
    {
        // Honestly not sure how to implement this in the general case yet.
        std::map<std::string, float> contagionByRoute;
        release_assert( GetTransmissionRoutes().size() > 0 );
        for( auto & route: GetTransmissionRoutes() )
        {
            // how do we get membership? That's from an individual, but we are at node level here?????
            // Need to get proper mapping for route name, route idx, and group id. Just hacking it here.
            // This shouldn't be so "exposed". The tx group class should worry about indices.
            // I don't want to know/worry about route indices here but don't want to rearch tx groups API either.  
            auto contagion = transmissionGroups->GetTotalContagion();
            contagionByRoute.insert( std::make_pair( route, contagion ) );
        }
        return contagionByRoute;
    }

    float Node::GetTotalContagion( void )
    {
        return transmissionGroups->GetTotalContagion();
    }

    const RouteList_t& Node::GetTransmissionRoutes() const
    {
        return routes;
    }

    float Node::GetContagionByRouteAndProperty( const std::string& route, const IPKeyValue& property_value )
    {
        return transmissionGroups->GetContagionByProperty( property_value );
    }

    void Node::UpdateTransmissionGroupPopulation(const IPKeyValueContainer& properties, float size_changes, float mc_weight)
    {
        TransmissionGroupMembership_t membership;
        transmissionGroups->GetGroupMembershipForProperties( properties, membership ); 
        transmissionGroups->UpdatePopulationSize(membership, size_changes, mc_weight);
    }

    void Node::ExposeIndividual(IInfectable* candidate, TransmissionGroupMembership_t individual, float dt)
    {
        if( bSkipping )
        {
            return;
        }
        transmissionGroups->ExposeToContagion(candidate, individual, dt, TransmissionRoute::TRANSMISSIONROUTE_CONTACT);
    }

    void Node::DepositFromIndividual( const IStrainIdentity& strain_IDs, float contagion_quantity, TransmissionGroupMembership_t individual, TransmissionRoute::Enum route )
    {
        LOG_DEBUG_F("deposit from individual: antigen index =%d, substain index = %d, quantity = %f\n", strain_IDs.GetAntigenID(), strain_IDs.GetGeneticID(), contagion_quantity);
        transmissionGroups->DepositContagion( strain_IDs, contagion_quantity, individual );
    }
    
    //------------------------------------------------------------------
    //   Every timestep Update() methods
    //------------------------------------------------------------------

    void Node::SetWaitingForFamilyTrip( suids::suid migrationDestination, 
                                        MigrationType::Enum migrationType, 
                                        float timeUntilTrip, 
                                        float timeAtDestination,
                                        bool isDestinationNewHome )
    {
        family_waiting_to_migrate      = true;
        family_migration_destination   = migrationDestination;
        family_migration_type          = migrationType;
        family_time_until_trip         = timeUntilTrip;
        family_time_at_destination     = timeAtDestination;
        family_is_destination_new_home = isDestinationNewHome;
    }

    void Node::ManageFamilyTrip( float currentTime, float dt )
    {
        if( family_waiting_to_migrate )
        {
            bool leave_on_trip = IsEveryoneHome() ;
            for (auto individual : individualHumans)
            {
                if( home_individual_ids.count( individual->GetSuid().data ) > 0 )
                {
                    if( leave_on_trip )
                    {
                        individual->SetGoingOnFamilyTrip( family_migration_destination, 
                                                          family_migration_type, 
                                                          family_time_until_trip, 
                                                          family_time_at_destination,
                                                          family_is_destination_new_home );
                    }
                    else
                    {
                        individual->SetWaitingToGoOnFamilyTrip();
                    }
                }
            }
            if( leave_on_trip )
            {
                family_waiting_to_migrate      = false ;
                family_migration_destination   = suids::nil_suid();
                family_migration_type          = MigrationType::NO_MIGRATION;
                family_time_until_trip         = 0.0f;
                family_time_at_destination     = 0.0f ;
                family_is_destination_new_home = false;
            }
            else
            {
                family_time_until_trip -= dt ;
            }
        }
    }

    void Node::PreUpdate()
    {
        if( IndividualHumanConfig::enable_skipping )
        {
            if( gap == 1 )
            {
                bSkipping = false;
                //ProbabilityNumber max_prob = std::max( maxInfectionProb[ TransmissionRoute::TRANSMISSIONROUTE_CONTACT ], maxInfectionProb[ TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL ] );
                gap = calcGap();
                LOG_DEBUG_F( "The (next) gap to skip for this node is calculated as: %d.\n", gap );
            }
            else
            {
                bSkipping=true;
                gap--;
            }
        }
    }

    void Node::PostUpdate()
    {
        // Do nothing for generic
    }

    void Node::Update(float dt)
    {
#ifndef DISABLE_CLIMATE
        // Update weather
        if(localWeather)
        {
            localWeather->UpdateWeather( GetTime().time, dt, GetRng() );
        }
#endif
        // Update node-level interventions
        if (params()->interventions) 
        {
            release_assert(event_context_host);
            event_context_host->UpdateInterventions(dt); // update refactored node-owned node-targeted interventions

            // -------------------------------------------------------------------------
            // --- I'm putting this after updating the interventions because if one was
            // --- supposed to expire this timestep, then this event should not fire it.
            // -------------------------------------------------------------------------
            for( auto event_trigger : events_from_other_nodes )
            {
                for (auto individual : individualHumans)
                {
                    event_context_host->TriggerObservers( individual->GetEventContext(), event_trigger );
                }
            }
        }

        ManageFamilyTrip( GetTime().time, dt );

        //-------- Accumulate infectivity and reporting counters ---------

        resetNodeStateCounters();

        // Update the likelihood of an individual becoming infected.
        // This is based on the current infectiousness at the start of the timestep of all individuals present at the start of the timestep
        updateInfectivity(dt);

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! GH-798 - I had to change this for loop to use an index so that a user could have an Outbreak intervention within
        // !!! a NLHTIV. Outbreak will import/add people to the scenario.  This gets around the issue of the iterator being violated.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        for( int i = 0 ; i < individualHumans.size() ; ++i )
        {
            IIndividualHuman* individual = individualHumans[i];

            // TBD: Move this to Typhoid layer without copy-pasting the entire function
            // Each route has separate max prob and so separate gap. Has to be handled
            PreUpdate();

            individual->Update(GetTime().time, dt);
        }

        PostUpdate();

        // -------------------------------------------------------------------------------------------------
        // --- Break out the updating of the reports to be after all of the individuals have been updated.
        // --- This is particularly important to diseases based on relationships.  When we collect data on
        // --- a relationships (i.e. discordant vs concordant), we want both individuals to have been updated.
        // -------------------------------------------------------------------------------------------------
        for( int i = 0 ; i < individualHumans.size() ; ++i )
        {
            IIndividualHuman* individual = individualHumans[i];

            // JPS: Should we do this later, after updateVitalDynamics() instead?  
            //      That way we could track births in the report class instead of having to do it in Node...
            for (auto report : parent->GetReportsNeedingIndividualData())
            {
                report->LogIndividualData(individual);
            }

            updateNodeStateCounters(individual);
        }

        finalizeNodeStateCounters();

        //----------------------------------------------------------------

        // Vital dynamics for this time step at community level (handles mainly births)

        if (vital_dynamics)
        {
            updateVitalDynamics(dt);
        }

        if (ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_IMMUNE_STATE)
        {
            LOG_DEBUG_F( "Check whether any individuals need to be down-sampled based on immunity.\n" );
            for (auto individual : individualHumans) 
            {
                float current_mc_weight = float(individual->GetMonteCarloWeight());
                if (current_mc_weight == 1.0f/base_sample_rate)  //KM: In immune state downsampling, there is only the regular sampling and downsampled; don't need to go through the logic if we've already been downsampled.
                {
                   float desired_mc_weight = 1.0f/float(adjustSamplingRateByImmuneState(base_sample_rate, ( individual->GetAcquisitionImmunity() < immune_threshold_for_downsampling ) && !individual->IsInfected() ));
                   if (desired_mc_weight > current_mc_weight)
                   {
                       LOG_DEBUG_F("MC Sampling: acq. immunity = %f, is infected = %i, current MC weight = %f, desired MC weight = %f\n", individual->GetAcquisitionImmunity(), individual->IsInfected(), current_mc_weight,desired_mc_weight);
                       individual->UpdateMCSamplingRate(desired_mc_weight);
                       LOG_DEBUG_F("MC Weight now %f, state change = %i\n", float(individual->GetMonteCarloWeight()), (int)individual->GetStateChange());
                   }
                   /*else if (desired_mc_weight < current_mc_weight)
                   {
                       //KM: Placeholder to allow a "clone individual and increase sampling rate" functionality if desired
                   }*/
                }
            }
        }

        // If individual has migrated or died -- HAPPENS AT THE END OF TIME STEP -- he/she still contributes to the infectivity
        for( int iHuman = 0 ; iHuman < individualHumans.size() ; /* control in loop */ )
        {
            IIndividualHuman* individual = individualHumans[iHuman];
            release_assert( individual );

            // clorton auto state_change = individual->GetStateChange();
            if( individual->IsDead() )
            {
                if (individual->GetStateChange() == HumanStateChange::KilledByInfection)
                    Disease_Deaths += float(individual->GetMonteCarloWeight());

                RemoveHuman( iHuman );

                // ---------------------------------------
                // --- We want individuals to die at home
                // ---------------------------------------
                if( individual->AtHome() )
                {
                    home_individual_ids.erase( individual->GetSuid().data ); // if this person doesn't call this home, then nothing happens

                    delete individual;
                    individual = nullptr;
                }
                else
                {
                    // individual must go home to officially die
                    individual->GoHome();
                    processEmigratingIndividual(individual);
                }
            }
            else if (individual->IsMigrating())
            {
                // don't remove from home_individual_ids because they are just migrating

                RemoveHuman( iHuman );

                // subtract individual from group population(s)
                processEmigratingIndividual(individual);
            }
            else
            {
                ++iHuman;
            }
        }

        if(susceptibility_scaling_type == SusceptibilityScalingType::LOG_LINEAR_FUNCTION_OF_TIME)
        {
            susceptibility_dynamic_scaling += dt*susceptibility_scaling_rate;

            if(susceptibility_dynamic_scaling > 1.0f)
            {
                susceptibility_dynamic_scaling = 1.0f;
            }
        }

        // Increment simulation time counter
    }

    void Node::clearTransmissionGroups()
    {
        // -------------------------------------------------------------------------
        // --- Clearing the population size assumes that we are updating it in
        // --- accumulateIndividualPopulationStatistics().  This way it is update
        // --- with the most recent status before we call EndUpdate()
        // -------------------------------------------------------------------------
        this->transmissionGroups->ClearPopulationSize();
    }

    void Node::updateInfectivity(float dt)
    {
        clearTransmissionGroups();

        // Process population to update who is infectious, etc...
        updatePopulationStatistics(dt);
        LOG_DEBUG_F("Statistical population of %d at Node ID = %d.\n", GetStatPop(), GetSuid().data);

        if ( statPop <=0 )
        {
            LOG_WARN_F("No individuals at Node ID = %d.  infectionrate = 0\n", GetSuid().data);
            return;
        }

        // Single route implementation
        LOG_DEBUG_F("[updateInfectivity] starting infectionrate = %f\n", mInfectivity/statPop);

        float infectivity_multiplier = 1.0;

        // Add-in population density correction
        if (population_density_infectivity_correction == PopulationDensityInfectivityCorrection::SATURATING_FUNCTION_OF_DENSITY)
        {
            infectivity_multiplier *= getDensityContactScaling();
        }

        // Add in seasonality
        if (infectivity_scaling == InfectivityScaling::FUNCTION_OF_TIME_AND_LATITUDE)
        {
            infectivity_multiplier *= getSeasonalInfectivityCorrection();
        }
        else if (infectivity_scaling == InfectivityScaling::FUNCTION_OF_CLIMATE)
        {
            if( localWeather == nullptr )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Simulation_Type", SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type ), "Infectivity_Scaling", "FUNCTION_OF_CLIMATE" );
            }
            infectivity_multiplier *= getClimateInfectivityCorrection();
        }
        else if(infectivity_scaling == InfectivityScaling::SINUSOIDAL_FUNCTION_OF_TIME)
        {
            infectivity_multiplier *= getSinusoidalCorrection(infectivity_sinusoidal_forcing_amplitude, 
                                                              infectivity_sinusoidal_forcing_phase);
        }
        else if(infectivity_scaling == InfectivityScaling::ANNUAL_BOXCAR_FUNCTION)
        {
            infectivity_multiplier *= getBoxcarCorrection(infectivity_boxcar_forcing_amplitude,
                                                          infectivity_boxcar_start_time,
                                                          infectivity_boxcar_end_time);
        }
        else if(infectivity_scaling == InfectivityScaling::EXPONENTIAL_FUNCTION_OF_TIME)
        {
            infectivity_multiplier *= getExponentialCorrection(infectivity_exponential_baseline,
                                                               infectivity_exponential_rate, infectivity_exponential_delay);    
        }

        // Incorporate additive infectivity
        float infectivity_addition = 0.0f;

        if( enable_infectivity_reservoir )
        {
            if(GetTime().time >= infectivity_reservoir_start_time &&
               GetTime().time <  infectivity_reservoir_end_time     )
            {
                infectivity_addition += infectivity_reservoir_size * dt;
            }
        }

        mInfectivity  += infectivity_addition;

        transmissionGroups->EndUpdate(infectivity_multiplier, infectivity_addition);
        LOG_DEBUG_F("[updateInfectivity] final infectionrate = %f\n", mInfectivity/statPop);

        if( IndividualHumanConfig::enable_skipping )
        {
            computeMaxInfectionProb( dt );
            gap = calcGap();
            LOG_INFO_F( "The (initial) gap to skip for this node is calculated as: %d.\n", int(gap) );
        }
    }

    void Node::accumulateIndividualPopStatsByValue(
        float mcw, float infectiousness, bool poss_mom, bool is_infected, bool is_symptomatic, bool is_newly_symptomatic
    )
    {
        // These are zeroed out in ResetNodeStateCounters before being accumulated each time step
        statPop          += mcw;
        Possible_Mothers += long( poss_mom ? mcw : 0);
        mInfectivity     += infectiousness;
        Infected         += is_infected ? mcw : 0;
        symptomatic      += is_symptomatic ? mcw : 0;
        newly_symptomatic+= is_newly_symptomatic ? mcw : 0;
    }

    void Node::accumulateIndividualPopulationStatistics(float dt, IIndividualHuman* individual)
    {
        individual->UpdateInfectiousness(dt);

        float mcw = float(individual->GetMonteCarloWeight());
        float infectiousness = mcw * individual->GetInfectiousness();
        if( infectiousness > 0 )
        {
            LOG_DEBUG_F( "infectiousness = %f\n", infectiousness );
        }
        accumulateIndividualPopStatsByValue( mcw, infectiousness, individual->IsPossibleMother(), individual->IsInfected(), individual->IsSymptomatic(), individual->IsNewlySymptomatic() );

        individual->UpdateGroupPopulation( 1.0 );
    }

    void Node::updatePopulationStatistics(float dt)
    {
        for (auto individual : individualHumans)
        {
            // This function is modified in derived classes to accumulate
            // disease-specific individual properties
            accumulateIndividualPopulationStatistics(dt, individual);
        }
    }

    float Node::getDensityContactScaling()
    {
        float localdensity      = 0;
        float densitycorrection = 1.00;

        if (params()->lloffset > 0) // check to make sure area will be nonzero
        {
            // calculate area of cell in km^2 from the lloffset, which is half the cell width in degrees
            // under the sphere coordinate, dA = R^2*sin(theta) d theta d phi
            // where theta is 90 degrees-latitude, phi is latitude, both in radians
            // note: under spherical coordinate, theta is 90 degree - latitude
            // therefore, integrate between theta1, theta2 and phi1, phi2, A = R^2 * (phi2-phi1) * cos(theta1)-cos(theta2)

            // in degrees,
            // phi2-phi1 = 2 *lloffset
            // theta1 = 90 - latitude - lloffset
            // theta2 = 90 - latitude + lloffset

            // Pi/180 will convert degree to radian
            float lat_deg = GetLatitudeDegrees() ;
            float lat_rad1 = float(( 90.0 - lat_deg - params()->lloffset ) * PI / 180.0);
            float lat_rad2 = float(( 90.0 - lat_deg + params()->lloffset ) * PI / 180.0);

            float cellarea = float(EARTH_RADIUS_KM * EARTH_RADIUS_KM * (cos(lat_rad1) - cos(lat_rad2)) * params()->lloffset * 2.0 * (PI / 180.0));

            localdensity = statPop / cellarea;
            LOG_DEBUG_F("[getDensityContactScaling] cellarea = %f, localdensity = %f\n", cellarea, localdensity);
        }

        if (population_density_c50 > 0)
        {
            densitycorrection = EXPCDF(-localdensity / population_density_c50);
        }
        else
        {
            densitycorrection = 1.0;
        }

        LOG_DEBUG_F("[getDensityContactScaling] densitycorrection = %f\n", densitycorrection);
        return densitycorrection;
    }

    float Node::getSeasonalInfectivityCorrection()
    {
        // This FUNCTION_OF_TIME_AND_LATITUDE correction varies infectivity from its full value to half its value as a function of latitude and time of year with a one year period
        // to approximate seasonal forcing for different diseases which have a peak season from fall to spring
        // When infectivity is at its full value in the northern hemisphere, it is at its lowest value in the southern hemisphere
        // The phase is chosen so that in spring and fall, the infectivities in northern and southern latitude are approximately equal.
        // the latitude offset moves the center of the peak infectivity cosine from 23.5 N latitude to 23.5 S latitude

        float lat_deg = GetLatitudeDegrees();
        float correction = 1.0 / 4 * (3.0 - cos(2 * 2 * PI / 360 * (lat_deg - 23.5 * sin((GetTime().time - 100) / DAYSPERYEAR * 2.0 * PI))));
        LOG_DEBUG_F( "Infectivity scale factor = %f at latitude = %f and simulation time = %f.\n", correction, lat_deg, (float) GetTime().time );

        return correction;
    }

    float Node::getClimateInfectivityCorrection() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, 
            "The use of \"Infectivity_Scale_Type\" : \"FUNCTION_OF_CLIMATE\" is not supported in \"GENERIC_SIM\"." );
    }

    float Node::getExponentialCorrection(float baseline, float rate, float delay)
    {
        float correction = baseline;
        if (GetTime().time >= delay) {
            correction = 1 - (1-baseline)*exp(-1*(GetTime().time - delay)/rate);
        }
        return correction;
    }

    // This allows subclass to override this function and replace it, specifically PyMod in which node has no individuals and
    // individual parameters are passed
    void Node::considerPregnancyForIndividual(
        bool bPossibleMother,
        bool bIsPregnant,
        float age,
        int individual_id,
        float dt,
        IIndividualHuman* pIndividual
    )
    {
        if( vital_birth_dependence == VitalBirthDependence::FIXED_BIRTH_RATE ||
            vital_birth_dependence == VitalBirthDependence::POPULATION_DEP_RATE ||
            vital_birth_dependence == VitalBirthDependence::DEMOGRAPHIC_DEP_RATE
          )
        {
            return;
        }

        if( bIsPregnant )
        {
            if( pIndividual != nullptr )
            {
                if( pIndividual->UpdatePregnancy( dt ) )
                {
                    populateNewIndividualFromMotherPointer( pIndividual );
                }
            }
            else
            {
                // UPDATE PREGNANCY BY INDIVIDUAL (MOTHER) ID
                if( updatePregnancyForIndividual( individual_id, dt ) )
                {
                    populateNewIndividualFromMotherId( individual_id );
                }
            }
        }
        else if( bPossibleMother )
        {
            ProbabilityNumber step_birthrate = birthrate * dt * x_birth;

            // If we are using an age-dependent fertility rate, then this needs to be accessed/interpolated based on the current possible-mother's age.
            if( vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR  ) 
            {
                // "FertilityDistribution" is added to map in Node::SetParameters if 'vital_birth_dependence' flag is set to INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR 
                float temp_birthrate = FertilityDistribution->DrawResultValue(age, float(GetTime().Year()));
                LOG_DEBUG_F("%d-year-old possible mother has annual fertility rate = %f\n", int(age/DAYSPERYEAR), temp_birthrate * DAYSPERYEAR);

                // In the limit of low birth rate, the probability of becoming pregnant is equivalent to the birth rate.
                // However, at higher birth rates, some fraction of possible mothers will already be pregnant.  
                // Roughly speaking, if we want women to give birth every other year, and they gestate for one year,
                // then the expected time between pregnancy has to be one year, not two.
                // Hence, the maximum possible birth rate is 1 child per woman per gestation period.
                if ( temp_birthrate * DAYSPERWEEK * WEEKS_FOR_GESTATION >= 1.0 )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Check birthrate/vital_birth_dependence mismatch in Node::updateVitalDynamics()" );
                }
                else
                {
                    temp_birthrate /= (1.0F - temp_birthrate * DAYSPERWEEK * WEEKS_FOR_GESTATION);
                }

                step_birthrate = temp_birthrate * dt * x_birth;
            }

            if( pIndividual != nullptr )
            {
                // Modify the birth rate - i.e. like from a contraceptive
                step_birthrate *= pIndividual->GetBirthRateMod();
            }

            if( (GetRng() != nullptr) && GetRng()->SmartDraw( step_birthrate ) )
            {
                LOG_DEBUG_F("New pregnancy for %d-year-old\n", int(age/DAYSPERYEAR));
                float duration = (DAYSPERWEEK * WEEKS_FOR_GESTATION) - (GetRng()->e() *dt);
                if( pIndividual != nullptr )
                {
                    pIndividual->InitiatePregnancy( duration );
                }
                else
                {
                    initiatePregnancyForIndividual( individual_id, duration );
                }
            }
        }
    }

    // These functions are for implementation in derived classes.
    float Node::initiatePregnancyForIndividual( int individual_id, float duration )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Should be implemented by derived class" );
    }

    bool Node::updatePregnancyForIndividual( int individual_id, float dt )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Should be implemented by derived class" );
    }

    void Node::updateVitalDynamics(float dt)
    {
        long int newborns    = 0;
        float step_birthrate = birthrate * dt * x_birth;

        if (!vital_birth) //  Births
        {
            return;
        }

        switch(vital_birth_time_dependence)
        { 
            case VitalBirthTimeDependence::NONE:
            break; 

            case VitalBirthTimeDependence::SINUSOIDAL_FUNCTION_OF_TIME:
            step_birthrate *= getSinusoidalCorrection(birth_rate_sinusoidal_forcing_amplitude, 
                                                      birth_rate_sinusoidal_forcing_phase);
            break; 

            case VitalBirthTimeDependence::ANNUAL_BOXCAR_FUNCTION:
            step_birthrate *= getBoxcarCorrection(birth_rate_boxcar_forcing_amplitude,    
                                                  birth_rate_boxcar_start_time,
                                                  birth_rate_boxcar_end_time);
            break;
        }

        switch (vital_birth_dependence)
        {
            case VitalBirthDependence::FIXED_BIRTH_RATE:
            //  Calculate births for this time step from constant rate and add them
            newborns = long( GetRng()->Poisson(step_birthrate));
            populateNewIndividualsByBirth(newborns);
            break;

            case VitalBirthDependence::POPULATION_DEP_RATE:
            //  Birthrate dependent on current population, determined by census
            if( ind_sampling_type )
            {
                newborns = long( GetRng()->Poisson(step_birthrate * statPop) );
            }
            else
            { 
                newborns = long( GetRng()->Poisson(step_birthrate * individualHumans.size()) );
            }
            LOG_DEBUG_F( "Poisson draw based on birthrate of %f and POPULATION_DEP_RATE mode, with pop = %f says we need %d new babies.\n",
                         step_birthrate, statPop, newborns
                       );
            populateNewIndividualsByBirth(newborns);
            break;

            case VitalBirthDependence::DEMOGRAPHIC_DEP_RATE:
            // Birthrate dependent on current census females in correct age range
            if (ind_sampling_type)
            {
                newborns = long( GetRng()->Poisson(step_birthrate * Possible_Mothers) );
            }       // KTO !TRACK_ALL?  is this right?
            else
            {
                newborns = long( GetRng()->Poisson(step_birthrate * Possible_Mothers) );
            }
            LOG_DEBUG_F( "Poisson draw based on birthrate of %f and DEMOGRPHIC_DEP_RATE mode, with possible moms = %d says we need %d new babies.\n",
                         step_birthrate, Possible_Mothers, newborns
                       );
            populateNewIndividualsByBirth(newborns);
            break;

            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES:
            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR:
            // Birthrate dependent on current census females in correct age range, who have carried a nine-month pregnancy
            // need to process list of people, count down pregnancy counters, if a birth occurs, call Populate(pointer to mother), then come up with new pregnancies
            for( int iHuman = 0 ; iHuman < individualHumans.size() ; iHuman++ )
            {
                auto individual = individualHumans.at( iHuman );
                considerPregnancyForIndividual( individual->IsPossibleMother(), individual->IsPregnant(), individual->GetAge(), individual->GetSuid().data, dt, individual );
            }
            break;
        }
    }

    float Node::GetNonDiseaseMortalityRateByAgeAndSex( float age, Gender::Enum sex ) const
    {
        if( enable_natural_mortality == false )
        {
            return 0.0f;
        }

        float rate = 0;
        if ( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER ||
             vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER )
        {
            // DJK TODO: Compute natural death at initiation and use timer <ERAD-1857>
            // for performance, cache and recalculate mortality rate only every month
            {
                if( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER )
                {
                    // "MortalityDistribution" is added to map in Node::SetParameters if Death_Rate_Dependence is NONDISEASE_MORTALITY_BY_AGE_AND_GENDER
                    rate = MortalityDistribution->DrawResultValue( sex == Gender::FEMALE, age);
                }
                else
                {
                    float year = GetTime().Year(); 
                    release_assert( MortalityDistributionMale );
                    if( sex == Gender::MALE )
                    {
                        rate = MortalityDistributionMale->DrawResultValue( age, year);
                    }
                    else
                    {
                        rate = MortalityDistributionFemale->DrawResultValue( age, year);
                    }
                }
            }
            rate *= x_othermortality;
        }
        return rate;
    }

    //------------------------------------------------------------------
    //   Population initialization methods
    //------------------------------------------------------------------

    // This function allows one to scale the initial population by a factor without modifying an input demographics file.
    void Node::PopulateFromDemographics( NodeDemographicsFactory *demographics_factory )
    {
        m_IndividualHumanSuidGenerator = suids::distributed_generator( GetSuid().data, demographics_factory->GetNodeIDs().size() );

        uint32_t InitPop = uint32_t(demographics["NodeAttributes"]["InitialPopulation"].AsUint64());

        // correct initial population if necessary (historical simulation for instance
        if ( population_scaling )
        {
            InitPop = uint32_t(InitPop * population_scaling_factor);
        }

        populateNewIndividualsFromDemographics(InitPop);
    }

    // This function adds the initial population to the node according to behavior determined by the settings of various flags:
    // (1) ind_sampling_type: TRACK_ALL, FIXED_SAMPLING, ADAPTED_SAMPLING_BY_POPULATION_SIZE, ADAPTED_SAMPLING_BY_AGE_GROUP, ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE
    // (2) demographics_initial
    // (3) enable_age_initialization, age_initialization_distribution_type
    // (4) vital_birth_dependence: INDIVIDUAL_PREGNANCIES must have initial pregnancies initialized
    void Node::populateNewIndividualsFromDemographics(int count_new_individuals)
    {
        if ((ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_IMMUNE_STATE) && (count_new_individuals*rel_sample_rate_immune*base_sample_rate < 1.0)) // At least one individual must be sampled
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Sample Rate Immune", rel_sample_rate_immune*base_sample_rate, "Number of Individuals", float(count_new_individuals), "and Individual_Sampling_Type: ADAPTED_SAMPLING_BY_IMMUNE_STATE");
        }

        int32_t num_adults = 0 ;
        int32_t num_children = 0 ;

        // TODO: throw exception on disallowed combinations of parameters (i.e. adaptive sampling without initial demographics)?

        // Set default values for configureAndAddIndividual arguments, sampling rate, etc.
        double temp_age           = 0;
        double initial_prevalence = 0;
        double female_ratio       = 0.5;
        const double default_age  = 20 * DAYSPERYEAR; // age to use by default if age_initialization_distribution_type config parameter is off.
        float temp_sampling_rate  = 1.0f;             // default sampling rate

        // Base sampling rate is only modified for FIXED_SAMPLING or ADAPTED_SAMPLING_BY_IMMUNE_STATE
        if(ind_sampling_type == IndSamplingType::FIXED_SAMPLING ||
           ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_IMMUNE_STATE)
        {
            temp_sampling_rate = base_sample_rate;
        }

        // Cache pointers to the initial age distribution with the node, so it doesn't have to be created for each individual.
        // After the demographic initialization is complete, it can be removed from the map and deleted
        if( age_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX )
        {
            if( !demographics.Contains( "IndividualAttributes" ) || !demographics["IndividualAttributes"].Contains( "AgeDistribution" ) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Age_Initialization_Distribution_Type", "DISTRIBUTION_COMPLEX", "['IndividualAttributes']['AgeDistribution']", "<not found>" );
            }
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistribution tag in node demographics file.\n" );
            AgeDistribution = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["AgeDistribution"]);
        }

        //set initial prevalence
        if (enable_initial_prevalence)
        {
            LOG_DEBUG("Parsing IndividualAttributes->PrevalenceDistributionFlag tag in node demographics file.\n");
            auto prevalence_distribution_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["PrevalenceDistributionFlag"].AsInt());
            LOG_DEBUG("Parsing IndividualAttributes->PrevalenceDistribution1 tag in node demographics file.\n");
            float prevdist1 = float(demographics["IndividualAttributes"]["PrevalenceDistribution1"].AsDouble());
            LOG_DEBUG("Parsing IndividualAttributes->PrevalenceDistribution2 tag in node demographics file.\n");
            float prevdist2 = float(demographics["IndividualAttributes"]["PrevalenceDistribution2"].AsDouble());

            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( prevalence_distribution_type ) );
            distribution->SetParameters( prevdist1, prevdist2, 0.0 );
            initial_prevalence = distribution->Calculate( GetRng() );
        }
        // Modify sampling rate in case of adapted sampling by population size
        if ( ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_POPULATION_SIZE ||
             ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE )
        {
            if (count_new_individuals > max_sampling_cell_pop)
            {
                temp_sampling_rate *= max_sampling_cell_pop / count_new_individuals;
            }
        }

        // Keep track of the adapted sampling rate in case it will be further modified on an age-dependent basis for *each* individual in the loop below
        float temp_node_sampling_rate = temp_sampling_rate;

        // Loop over 'count_new_individuals' initial statistical population
        for (int i = 1; i <= count_new_individuals; ++i)
        {
            // For age-dependent adaptive sampling, we need to draw an individual age before adjusting the sampling rate
            if ( ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP || ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE )
            {
                temp_age = calculateInitialAge(default_age);
                temp_sampling_rate = adjustSamplingRateByAge(temp_node_sampling_rate, temp_age);
            }

            // Condition for rejecting potential individuals based on sampling rate in case we're using sampling
            if ( ind_sampling_type != IndSamplingType::TRACK_ALL && GetRng()->e() > temp_sampling_rate )
            {
                LOG_VALID( "Not creating individual\n" );
                continue;
            }

            // Draw individual's age if we haven't already done it to determine adaptive sampling rate
            if ( (ind_sampling_type != IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP             ) &&
                 (ind_sampling_type != IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE) )
            {
                temp_age = calculateInitialAge(default_age);
                if( demographics["IndividualAttributes"].Contains( "PercentageChildren" ) )
                {
                    float required_percentage_of_children = demographics["IndividualAttributes"]["PercentageChildren"].AsDouble();

                    if( (required_percentage_of_children > 0.0f)  && IndividualHumanConfig::IsAdultAge( 0.0 ) )
                    {
                        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                            "Minimum_Adult_Age_Years", 0.0f,
                            "<demographics>::Defaults/Node.IndividualAttributes.PercentageChildren", required_percentage_of_children,
                            "You can't have any children if everyone is an adult (Minimum_Adult_Age_Years = 0 implies every one is an adult)." );
                    }

                    float required_percentage_of_adults = 1.0  -required_percentage_of_children ;
                    float percent_children = float(num_children) / float(count_new_individuals) ;
                    float percent_adults   = float(num_adults)   / float(count_new_individuals) ;
                    float age_years = temp_age / DAYSPERYEAR ;

                    // if a child and already have enough children, recalculate age until we get an adult
                    while( !IndividualHumanConfig::IsAdultAge( age_years ) && (percent_children >= required_percentage_of_children) )
                    {
                        temp_age = calculateInitialAge(default_age);
                        age_years = temp_age / DAYSPERYEAR ;
                    }

                    // if an adult and already have enough adults, recalculate age until we get a child
                    while( IndividualHumanConfig::IsAdultAge( age_years ) && (percent_adults >= required_percentage_of_adults) )
                    {
                        temp_age = calculateInitialAge(default_age);
                        age_years = temp_age / DAYSPERYEAR ;
                    }

                    if( IndividualHumanConfig::IsAdultAge( age_years ) )
                    {
                        num_adults++ ;
                    }
                    else
                    {
                        num_children++ ;
                    }
                }
                parent->CheckMemoryFailure( true );
            }

            IIndividualHuman* tempind = configureAndAddNewIndividual(1.0F / temp_sampling_rate, float(temp_age), float(initial_prevalence), float(female_ratio));
            
            if(tempind && tempind->GetAge() == 0)
            {
                tempind->setupMaternalAntibodies(nullptr, this);
            }

            // For now, do it unconditionally and see if it can catch all the memory failures cases with minimum cost
            parent->CheckMemoryFailure( false );

            // Every 1000 individuals, do a StatusReport...
            if( individualHumans.size() % 1000 == 0 && EnvPtr && EnvPtr->getStatusReporter() )
            {
                EnvPtr->getStatusReporter()->ReportInitializationProgress( individualHumans.size(), count_new_individuals );
            }
        }

        // Infection rate needs to be updated for the first timestep
        updateInfectivity(0.0f); 

        // Don't need this distribution after demographic initialization is completed
        // (If we ever want to use it in the future, e.g. in relation to Outbreak ImportCases, we can remove the following.  Clean-up would then be done only in the destructor.)
        if( age_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX )
        {
            delete AgeDistribution;
            AgeDistribution = nullptr;
        }

        if( SusceptibilityConfig::enable_initial_susceptibility_distribution )
        {
            if( SusceptibilityConfig::susceptibility_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX )
            {
                delete SusceptibilityDistribution;
                SusceptibilityDistribution = nullptr;
            }
        }
    }

    void Node::InitializeTransmissionGroupPopulations()
    {
        for (auto individual : individualHumans)
        {
            individual->UpdateGroupMembership();
        }
    }

    void Node::ExtractDataFromDemographics()
    {
        if (SusceptibilityConfig::enable_initial_susceptibility_distribution)
        {
            LOG_DEBUG("Parsing SusceptibilityDistribution\n");

            if (SusceptibilityConfig::susceptibility_initialization_distribution_type == DistributionType::DISTRIBUTION_SIMPLE)
            {
                susceptibility_dist_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["SusceptibilityDistributionFlag"].AsInt());

                // Only allowing FIXED(0), UNIFORM(1), and BIMODAL(6)
                if(susceptibility_dist_type == 0)
                {
                    susceptibility_dist1 = float(demographics["IndividualAttributes"]["SusceptibilityDistribution1"].AsDouble());
                }
                else if(susceptibility_dist_type == 1)
                {
                    susceptibility_dist1 = float(demographics["IndividualAttributes"]["SusceptibilityDistribution1"].AsDouble());
                    susceptibility_dist2 = float(demographics["IndividualAttributes"]["SusceptibilityDistribution2"].AsDouble());
                }
                else if(susceptibility_dist_type == 6)
                {
                    susceptibility_dist1 = float(demographics["IndividualAttributes"]["SusceptibilityDistribution1"].AsDouble());
                    susceptibility_dist2 = float(demographics["IndividualAttributes"]["SusceptibilityDistribution2"].AsDouble());
                }
                else
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "SusceptibilityDistributionFlag must be set to 0, 1, or 6.");
                }
            }
            else if (SusceptibilityConfig::susceptibility_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX)
            {
                LoadImmunityDemographicsDistribution();
            }
        }

        if (enable_demographics_risk)
        {
            LOG_DEBUG( "Parsing RiskDistribution\n" );

            risk_dist_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["RiskDistributionFlag"].AsInt());
            risk_dist1     =                      float(demographics["IndividualAttributes"]["RiskDistribution1"   ].AsDouble());
            risk_dist2     =                      float(demographics["IndividualAttributes"]["RiskDistribution2"   ].AsDouble());
        }

        if(enable_infectivity_reservoir)
        {
            LOG_DEBUG( "Parsing InfectivityReservoirSize, InfectivityReservoirStartTime, and InfectivityReservoirEndTime\n" );

            infectivity_reservoir_size       = static_cast<float>(demographics["NodeAttributes"]["InfectivityReservoirSize"].AsDouble());
            infectivity_reservoir_start_time = 0.0f;
            infectivity_reservoir_end_time   = FLT_MAX;

            if(infectivity_reservoir_size < 0.0f)
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "InfectivityReservoirSize", infectivity_reservoir_size, 0.0f);
            }
            if(demographics["NodeAttributes"].Contains("InfectivityReservoirStartTime"))
            {
                infectivity_reservoir_start_time = static_cast<float>(demographics["NodeAttributes"]["InfectivityReservoirStartTime"].AsDouble());
                if(infectivity_reservoir_start_time < 0.0f)
                {
                    throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "InfectivityReservoirStartTime", infectivity_reservoir_start_time, 0.0f);
                }
            }
            if(demographics["NodeAttributes"].Contains("InfectivityReservoirEndTime" ))
            {
                infectivity_reservoir_end_time = static_cast<float>(demographics["NodeAttributes"]["InfectivityReservoirEndTime"].AsDouble());
                if(infectivity_reservoir_end_time < infectivity_reservoir_start_time)
                {
                    throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "InfectivityReservoirEndTime", infectivity_reservoir_end_time, infectivity_reservoir_start_time);
                }
            }
        }
    }

    // This function adds newborns to the node according to behavior determined by the settings of various flags:
    // (1) ind_sampling_type: TRACK_ALL, FIXED_SAMPLING, ADAPTED_SAMPLING_BY_POPULATION_SIZE, ADAPTED_SAMPLING_BY_AGE_GROUP, ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE
    // (3) enable_maternal_infection_transmission
    // (4) vital_birth_dependence: FIXED_BIRTH_RATE, POPULATION_DEP_RATE, DEMOGRAPHIC_DEP_RATE. (INDIVIDUAL_PREGNANCIES handled in PopulateNewIndividualFromPregnancy)
    void Node::populateNewIndividualsByBirth(int count_new_individuals)
    {
        // Set default values for configureAndAddIndividual arguments, sampling rate, etc.
        double temp_prevalence    = 0;
        double female_ratio       = 0.5;   // TODO: it would be useful to add the possibility to read this from demographics (e.g. in India where there is a significant gender imbalance at birth)
        float  temp_sampling_rate = 1.0f;  // default sampling rate

        // Base sampling rate is only modified for FIXED_SAMPLING or ADAPTED_SAMPLING_BY_IMMUNE_STATE
        if(ind_sampling_type == IndSamplingType::FIXED_SAMPLING ||
           ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_IMMUNE_STATE)
        {
            temp_sampling_rate = base_sample_rate;
        }

        // Determine the prevalence from which maternal transmission events will be calculated, depending on birth model
        if (enable_maternal_infection_transmission) 
        {
            switch (vital_birth_dependence) 
            {
            case VitalBirthDependence::FIXED_BIRTH_RATE:
            case VitalBirthDependence::POPULATION_DEP_RATE:
                temp_prevalence = (statPop > 0) ? float(Infected) / statPop : 0; break;
            case VitalBirthDependence::DEMOGRAPHIC_DEP_RATE:
                temp_prevalence = getPrevalenceInPossibleMothers(); break;

            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES: break;
            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR: break;
            default: break;
            }
        }

        // For births, the adapted sampling by age uses the 'sample_rate_birth' parameter
        if (ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP || 
            ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE)
        {
            temp_sampling_rate *= sample_rate_birth;
        }

        // Modify sampling rate according to population size if so specified
        if (ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_POPULATION_SIZE || 
            ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE)
        {
            if (statPop > max_sampling_cell_pop)
            {
                temp_sampling_rate *= max_sampling_cell_pop / statPop;
            }
        }

        // Now correct sampling rate, in case it is over 100 percent
        if (temp_sampling_rate > 1.0f) 
        { 
            LOG_WARN("Total sampling rate > 1.0; value reset to 1.0\n");
            temp_sampling_rate = 1.0f; 
        }

        // Loop through potential new births 
        for (int i = 1; i <= count_new_individuals; i++)
        {
            // Condition for rejecting potential individuals based on sampling rate in case we're using sampling
            if ( ind_sampling_type != IndSamplingType::TRACK_ALL && GetRng()->e() >= temp_sampling_rate )
            {
                LOG_VALID( "Not creating individual\n" );
                continue;
            }

            IIndividualHuman* child = nullptr;
            child = configureAndAddNewIndividual( 1.0F / temp_sampling_rate, 0.0F, (float(temp_prevalence) * prob_maternal_infection_transmission), float( female_ratio ) ); // N.B. temp_prevalence=0 without enable_maternal_infection_transmission flag

            if( child != nullptr ) // valid in pymod
            {
                child->setupMaternalAntibodies(nullptr, this);
            }
            Births += 1.0f / temp_sampling_rate;
        }
    }

    // Populate with an Individual as an argument
    // This individual is the mother, and will give birth to a child under vital_birth_dependence==INDIVIDUAL_PREGNANCIES
    // This function should only get called frmy PyMod
    void Node::populateNewIndividualFromMotherId( unsigned int mother_id )
    {
        IIndividualHuman* mother = nullptr;
        for( auto pIndividual : individualHumans )
        {
            if( pIndividual->GetSuid().data == mother_id )
            {
                mother = pIndividual;
                break;
            }
        }

        populateNewIndividualFromMotherPointer(  mother );
    }

    void Node::populateNewIndividualFromMotherPointer( IIndividualHuman* mother )
    {
        float mcw      = mother->GetMonteCarloWeight(); // same sampling weight as mother
        int child_infections = 0;
        if ( enable_maternal_infection_transmission )
        {
            if ( mother->IsInfected() )
            {
                auto actual_prob_maternal_transmission = mother->getProbMaternalTransmission();
                if( GetRng()->SmartDraw( actual_prob_maternal_transmission ) )
                {
                    LOG_DEBUG_F( "Mother transmitting infection to newborn.\n" );
                    child_infections = 1;
                }
            }
        }

        // This ridiculous seeming roundabout is for pymod. Need to probably avoid doing this in regular case.
        auto child_id = populateNewIndividualFromMotherParams( mcw, child_infections );
        IIndividualHuman* child = nullptr;
        // This is overhead that looks nasty to accomodate pymod refactor. This 'loop' should hit its 
        // target immediately by doing reverse loop.
        for( auto r_it = individualHumans.rbegin(); r_it != individualHumans.rend(); ++r_it )
        {
            if( (*r_it)->GetSuid().data == child_id )
            {
                child = (*r_it);
                break;
            }
        }

        auto context = dynamic_cast<IIndividualHumanContext*>(mother);
        child->setupMaternalAntibodies(context, this);
    }

    unsigned int Node::populateNewIndividualFromMotherParams( float mcw, unsigned int child_infections )
    {
        //  holds parameters for new birth
        float child_age      = 0;
        int child_gender     = Gender::MALE;
        float female_ratio   = 0.5; 

        if ( GetRng()->e() < female_ratio ) { child_gender = Gender::FEMALE; }

        IIndividualHuman *child = addNewIndividual(mcw, child_age, child_gender, child_infections, 1.0, 1.0, 1.0);
        Births += mcw;//  Born with age=0 and no infections and added to sim with same sampling weight as mother
        unsigned int new_child_suid = 0;
        if( child != nullptr )
        {
            new_child_suid = child->GetSuid().data;
        }
        return new_child_suid;
    }

    ProbabilityNumber Node::GetProbMaternalTransmission() const
    {
        return prob_maternal_infection_transmission;
    }

    float Node::getPrevalenceInPossibleMothers()
    {
        float prevalence = 0;
        if (Possible_Mothers == 0) return prevalence; // prevent divide by zero up here

        for (auto individual : individualHumans)
        {
            if (individual->IsPossibleMother() && individual->IsInfected())
            {
                prevalence += float(individual->GetMonteCarloWeight());
            }
        }

        // Normalize to get prevalence in possible mothers
        prevalence /= Possible_Mothers;

        return prevalence;
    }

    void Node::conditionallyInitializePregnancy(IIndividualHuman* individual)
    {
        if( individual == nullptr )
        {
            // yes this is actually valid under certain scenarios.
            return;
        }

        float duration;
        float temp_birthrate = birthrate;

        if (individual->IsPossibleMother()) // woman of child-bearing age?
        {
            if(vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR) 
            { 
                // "FertilityDistribution" is added to map in Node::SetParameters if 'vital_birth_dependence' flag is set to INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR

                temp_birthrate = FertilityDistribution->DrawResultValue(individual->GetAge(), float(GetTime().Year()));
                LOG_DEBUG_F("%d-year-old possible mother has annual fertility rate = %f\n", int(individual->GetAge()/DAYSPERYEAR), temp_birthrate * DAYSPERYEAR);
            }

            float prob = x_birth * temp_birthrate * (DAYSPERWEEK * WEEKS_FOR_GESTATION);
            if( GetRng()->SmartDraw( prob ) ) // is the woman within any of the 40 weeks of pregnancy?
            {
                duration = float( GetRng()->e() ) * (DAYSPERWEEK * WEEKS_FOR_GESTATION); // uniform distribution over 40 weeks
                LOG_DEBUG_F("Initial pregnancy of %f remaining days for %d-year-old\n", duration, (int)(individual->GetAge()/DAYSPERYEAR));
                individual->InitiatePregnancy(duration);// initialize the pregnancy
            }
        }
    }

    IIndividualHuman* Node::createHuman(suids::suid id, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHuman::CreateHuman(this, id, monte_carlo_weight, initial_age, gender);
    }

    float Node::drawInitialSusceptibility(float ind_init_age)
    {
        float temp_susceptibility = 1.0;

        switch( SusceptibilityConfig::susceptibility_initialization_distribution_type )
        {
        case DistributionType::DISTRIBUTION_COMPLEX:
        {
            // Previous implementation was a linear interpolation in age providing fractional susceptibility to all agents.
            // Current implementation is an age-dependent probability of being totally susceptible, no agents start with fractional values
            // Revision suggested as appropriate for GENERIC sims by Philip on 28Jan2018.
            float randdraw = GetRng()->e();
            temp_susceptibility = (randdraw > SusceptibilityDistribution->DrawFromDistribution(ind_init_age)) ? 0.0f : 1.0;
            LOG_VALID_F( "creating individual with age = %f and susceptibility = %f, with randdraw = %f\n",  ind_init_age, temp_susceptibility, randdraw);
            break;
        }
        case DistributionType::DISTRIBUTION_SIMPLE:
        {
            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( susceptibility_dist_type ) );
            distribution->SetParameters( susceptibility_dist1, susceptibility_dist2, 0.0 );
            temp_susceptibility = distribution->Calculate( GetRng() );
            LOG_VALID_F( "creating individual with age = %f and susceptibility = %f\n", ind_init_age, temp_susceptibility );
            break;
        }
        case DistributionType::DISTRIBUTION_OFF:
            temp_susceptibility = 1.0;
            LOG_VALID_F( "creating individual with age = %f and susceptibility = %f\n",  ind_init_age, temp_susceptibility);
            break;

        default:
            if( !JsonConfigurable::_dryrun )
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "susceptibility_initialization_distribution_type", SusceptibilityConfig::susceptibility_initialization_distribution_type, DistributionType::pairs::lookup_key( SusceptibilityConfig::susceptibility_initialization_distribution_type ) );
            }
        }

        return temp_susceptibility;
    }

    IIndividualHuman* Node::configureAndAddNewIndividual(float ind_MCweight, float ind_init_age, float comm_init_prev, float comm_female_ratio)
    {
        //  Holds draws from demographic distribution
        int   temp_infs      = 0;
        int   temp_gender    = Gender::MALE;
        float temp_susceptibility  = 1.0;
        float temp_risk      = 1.0;
        float temp_migration = 1.0;

        // gender distribution exists regardless of gender demographics, but is ignored unless vital_birth_dependence is 2 or 3
        if( GetRng()->SmartDraw( comm_female_ratio ) )
        { 
            temp_gender = Gender::FEMALE;
        }

        // if individual *could* be infected, do a random draw to determine his/her initial infected state
        if(GetRng()->SmartDraw( comm_init_prev ) )
        { 
            temp_infs = 1;
        }

        if (SusceptibilityConfig::enable_initial_susceptibility_distribution)
        {
            // set initial immunity (or heterogeneous innate immunity in derived malaria code)
            temp_susceptibility = drawInitialSusceptibility(ind_init_age);

            // Range checking here because this function doesn't get overridden in 
            // disease-specific builds (unlike drawInitialSusceptibility)
            if(temp_susceptibility > 1.0)
            {
                LOG_WARN_F("Initial susceptibility to infection of %5.3f > 1.0; reset to 1.0\n", temp_susceptibility);
                temp_susceptibility = 1.0;
            }
            else if (temp_susceptibility < 0.0)
            {
                LOG_WARN_F("Initial susceptibility to infection of %5.3f < 0.0; reset to 0.0\n", temp_susceptibility);
                temp_susceptibility = 0.0;
            }
        }

        if (enable_demographics_risk)
        {
            // set heterogeneous risk
            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( risk_dist_type ) );
            distribution->SetParameters( risk_dist1, risk_dist2, 0.0 );
            temp_risk = distribution->Calculate( GetRng() );
        }

        if( (migration_info != nullptr) && migration_info->IsHeterogeneityEnabled() )
        {
            // This is not done during initialization but other times when the individual is created.           
            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( migration_dist_type ) );
            distribution->SetParameters( migration_dist1, migration_dist2, 0.0 );
            temp_migration = distribution->Calculate( GetRng() );
        }


        IIndividualHuman* tempind = addNewIndividual(ind_MCweight, ind_init_age, temp_gender, temp_infs, temp_susceptibility, temp_risk, temp_migration);

        // Now if tracking individual pregnancies, need to see if this new Individual is pregnant to begin the simulation
        if ( vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES ||
             vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR )
        { 
            conditionallyInitializePregnancy(tempind);
        }

        return tempind;
    }

    IIndividualHuman* Node::addNewIndividual(float mc_weight, float initial_age, 
        int gender, int initial_infections, float susceptibility_parameter, float risk_parameter, 
        float migration_heterogeneity)
    {
        // new creation process
        IIndividualHuman* new_individual = createHuman( m_IndividualHumanSuidGenerator(), mc_weight, initial_age, gender ); // initial_infections isn't needed here if SetInitialInfections function is used

        // EAW: is there a reason that the contents of the two functions below aren't absorbed into CreateHuman?  this whole process seems very convoluted.
        new_individual->SetParameters( this, 1.0, susceptibility_parameter, risk_parameter, migration_heterogeneity);// default values being used except for total number of communities
        new_individual->SetInitialInfections(initial_infections);
        new_individual->UpdateGroupMembership();

        individualHumans.push_back(new_individual);
        home_individual_ids.insert( std::make_pair( new_individual->GetSuid().data, new_individual->GetSuid() ) );
        new_individual->SetHome( this->GetSuid() );

        event_context_host->TriggerObservers( new_individual->GetEventContext(), EventTrigger::Births ); // EAW: this is not just births!!  this will also trigger on e.g. AddImportCases

        if( new_individual->GetParent() == nullptr )
        {
            LOG_INFO( "In addNewIndividual, indiv had no context, setting (migration hack path)\n" );
            new_individual->SetContextTo( this );
        }
        LOG_DEBUG_F("Added individual %d to node %d.\n", new_individual->GetSuid().data, GetSuid().data );

        new_individual->InitializeHuman();

        return new_individual;
    }

    IIndividualHuman* Node::addNewIndividualFromSerialization()
    {
        LOG_DEBUG_F( "1. %s\n", __FUNCTION__ );
        IIndividualHuman* new_individual = createHuman( suids::nil_suid(), 0, 0, 0);
        new_individual->SetParameters( this, 1.0, 0, 0, 0);// default values being used except for total number of communities

#if 0
        new_individual->SetInitialInfections(0);

        // Set up transmission groups
        if (params()->heterogeneous_intranode_transmission_enabled) 
        {
            new_individual->UpdateGroupMembership();
        }
#endif
        //individualHumans.push_back(new_individual);
#if 0
        event_context_host->TriggerObservers( new_individual->GetEventContext(), EventTrigger::Births ); // EAW: this is not just births!!  this will also trigger on e.g. AddImportCases

        if( new_individual->GetParent() == nullptr )
        {
            LOG_INFO( "In addNewIndividual, indiv had no context, setting (migration hack path)\n" );
            new_individual->SetContextTo( this );
        }
#endif
        //processImmigratingIndividual( new_individual );
        LOG_DEBUG_F( "addNewIndividualFromSerialization,individualHumans size: %d, ih context=%p\n",individualHumans.size(), new_individual->GetParent() );

        return new_individual;
    }


    double Node::calculateInitialAge(double age)
    {
        // Set age from distribution, or if no proper distribution set, make all initial individuals 20 years old (7300 days)

        if(age_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX)
        {
            // "AgeDistribution" is added to map in Node::SetParameters if 'enable_age_initialization_distribution' flag is set
            age = AgeDistribution->DrawFromDistribution( GetRng()->e() );
        }
        else if (age_initialization_distribution_type == DistributionType::DISTRIBUTION_SIMPLE)
        {
            if( !demographics.Contains( "IndividualAttributes" ) ||
                !demographics["IndividualAttributes"].Contains( "AgeDistributionFlag" ) ||
                !demographics["IndividualAttributes"].Contains( "AgeDistribution1" ) ||
                !demographics["IndividualAttributes"].Contains( "AgeDistribution2" ) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Age_Initialization_Distribution_Type", "DISTRIBUTION_SIMPLE", "['IndividualAttributes']['AgeDistributionFlag' or 'AgeDistribution1' or 'AgeDistribution2']", "<not found>" );
            }
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistributionFlag tag in node demographics file.\n" );
            auto age_distribution_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["AgeDistributionFlag"].AsInt());
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistribution1 tag in node demographics file.\n" );
            float agedist1 = float(demographics["IndividualAttributes"]["AgeDistribution1"].AsDouble()); 
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistribution2 tag in node demographics file.\n" );
            float agedist2 = float(demographics["IndividualAttributes"]["AgeDistribution2"].AsDouble());

            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( age_distribution_type ) );
            distribution->SetParameters( agedist1, agedist2, 0.0 );
            age = distribution->Calculate( GetRng() );
        }

        return age;
    }

    Fraction Node::adjustSamplingRateByAge(Fraction sampling_rate, double age) const
    {
        Fraction tmp = sampling_rate;
        if (age < (18 * IDEALDAYSPERMONTH)) { sampling_rate *= sample_rate_0_18mo; }
        else if (age <  (5 * DAYSPERYEAR))  { sampling_rate *= sample_rate_18mo_4yr; }
        else if (age < (10 * DAYSPERYEAR))  { sampling_rate *= sample_rate_5_9; }
        else if (age < (15 * DAYSPERYEAR))  { sampling_rate *= sample_rate_10_14; }
        else if (age < (20 * DAYSPERYEAR))  { sampling_rate *= sample_rate_15_19; }
        else { sampling_rate *= sample_rate_20_plus; }

        // Now correct sampling rate, in case it is over 100 percent
        if (sampling_rate > 1.0f) 
        { 
            LOG_WARN("Total sampling rate > 1.0; value reset to 1.0\n");
            sampling_rate = 1.0f; 
        }

        LOG_DEBUG_F( "%s: sampling_rate in = %f, age_in_years = %d, sampling_rate out = %f\n", __FUNCTION__, (float) tmp, (int) age/DAYSPERYEAR, (float) sampling_rate );
        return sampling_rate;
    }

    Fraction Node::adjustSamplingRateByImmuneState(Fraction sampling_rate, bool is_immune) const
    {
        Fraction tmp = sampling_rate;
        if (is_immune)
        {
            sampling_rate *= rel_sample_rate_immune;
        }

        // Now correct sampling rate, in case it is over 100 percent
        if (sampling_rate > 1.0f) 
        { 
            LOG_WARN("Total sampling rate > 1.0; value reset to 1.0\n");
            sampling_rate = 1.0f; 
        }

        LOG_DEBUG_F( "%s: sampling_rate in = %f, is_immune = %d, sampling_rate out = %f\n", __FUNCTION__, (float) tmp, (int) is_immune, (float) sampling_rate );
        return sampling_rate;
    }

    //------------------------------------------------------------------
    //   Individual migration methods
    //------------------------------------------------------------------

    void Node::processEmigratingIndividual(IIndividualHuman* individual)
    {
        release_assert( individual );

        // hack for now: legacy components of departure code
        resolveEmigration(individual);

        individual->SetContextTo(nullptr);
        postIndividualMigration(individual);
    }

    void Node::postIndividualMigration(IIndividualHuman* individual)
    {
        parent->PostMigratingIndividualHuman(individual);
    }

    void Node::resolveEmigration(IIndividualHuman* individual)
    {
        LOG_DEBUG_F( "individual %lu is leaving node %lu and going to %lu\n", individual->GetSuid().data, GetSuid().data,
            individual->GetMigrationDestination().data );

        event_context_host->TriggerObservers( individual->GetEventContext(), EventTrigger::Emigrating );

        // Do emigration logic here
        // Handle departure-linked interventions for individual
        if (params()->interventions ) // && departure_linked_dist)
        {
            event_context_host->ProcessDepartingIndividual(individual);
        }
    }

    IIndividualHuman* Node::processImmigratingIndividual(IIndividualHuman* movedind)
    {
        if( movedind->IsDead() )
        {
            // -------------------------------------------------------------
            // --- We want individuals to officially die in their home node
            // -------------------------------------------------------------
            movedind->SetContextTo(getContextPointer());
            release_assert( movedind->AtHome() );

            home_individual_ids.erase( movedind->GetSuid().data );
        }
        else
        {
            individualHumans.push_back(movedind);
            movedind->SetContextTo(getContextPointer());

            // check for arrival-linked interventions BEFORE!!!! setting the next migration
            if (params()->interventions )
            {
                event_context_host->ProcessArrivingIndividual(movedind);
            }
            event_context_host->TriggerObservers( movedind->GetEventContext(), EventTrigger::Immigrating );

            movedind->UpdateGroupMembership();
        }
        return movedind;
    }

    bool IdOrder( IIndividualHuman* pLeft, IIndividualHuman* pRight )
    {
        return pLeft->GetSuid().data < pRight->GetSuid().data;
    }

    void Node::SortHumans()
    {
        std::sort( individualHumans.begin(), individualHumans.end(), IdOrder );
        for( int i = 1; i < individualHumans.size(); ++i )
        {
            release_assert( individualHumans[i-1]->GetSuid().data < individualHumans[ i ]->GetSuid().data );
        }
    }


    bool Node::IsEveryoneHome() const
    {
        if( individualHumans.size() < home_individual_ids.size() )
        {
            // someone is missing
            return false ;
        }
        // there could be more people in the node than call it home

        int num_people_found = 0 ;
        for( auto individual : individualHumans )
        {
            if( home_individual_ids.count( individual->GetSuid().data ) > 0 )
            {
                num_people_found++ ;
                if( num_people_found == home_individual_ids.size() )
                {
                    return true ;
                }
            }
        }
        return false ;
    }

    //------------------------------------------------------------------
    //   Reporting methods
    //------------------------------------------------------------------

    void Node::resetNodeStateCounters()
    {
        Possible_Mothers       = 0;
        statPop                = 0;  // Population for statistical purposes
        Infected               = 0;
        mInfectivity           = 0;
        new_infections         = 0;
        new_reportedinfections = 0;
        newInfectedPeopleAgeProduct = 0;
        symptomatic            = 0;
        newly_symptomatic      = 0;
    }

    void Node::updateNodeStateCounters(IIndividualHuman* ih)
    {
        switch(ih->GetNewInfectionState()) 
        { 
        case NewInfectionState::NewlyDetected: 
            reportDetectedInfection(ih);
            break; 

        case NewInfectionState::NewAndDetected: 
            reportDetectedInfection(ih);
            reportNewInfection(ih);
            break; 

        case NewInfectionState::NewInfection: 
            reportNewInfection(ih);
            break;

        case NewInfectionState::Invalid: break;
        default: break;
        } 

        ih->ClearNewInfectionState();
    }

    void Node::reportNewInfection(IIndividualHuman *ih)
    {
        float monte_carlo_weight = float(ih->GetMonteCarloWeight());

        new_infections += monte_carlo_weight; 
        Cumulative_Infections += monte_carlo_weight; 

        newInfectedPeopleAgeProduct += monte_carlo_weight * float(ih->GetAge());
    }

    void Node::reportDetectedInfection(IIndividualHuman *ih)
    {
        float monte_carlo_weight = float(ih->GetMonteCarloWeight());

        new_reportedinfections += monte_carlo_weight; 
        Cumulative_Reported_Infections += monte_carlo_weight; 
    }

    void Node::finalizeNodeStateCounters(void)
    {
        infected_people_prior.push_back( float(new_infections) );
        if( infected_people_prior.size() > infection_averaging_window )
        {
            infected_people_prior.pop_front();
        }
        if( newInfectedPeopleAgeProduct < 0 )
        {
            throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "newInfectedPeopleAgeProduct", newInfectedPeopleAgeProduct, 0 );
        }

        infected_age_people_prior.push_back( float(newInfectedPeopleAgeProduct) );
        if( infected_age_people_prior.size() > infection_averaging_window )
        {
            infected_age_people_prior.pop_front();
        }

        double numerator = std::accumulate(infected_age_people_prior.begin(), infected_age_people_prior.end(), 0.0);
        if( numerator < 0.0 )
        {
            throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "numerator", numerator, 0 );
        }

        float denominator = std::accumulate( infected_people_prior.begin(), infected_people_prior.end(), 0 );
        if( denominator && numerator )
        {
            mean_age_infection = numerator/( denominator * DAYSPERYEAR);
            LOG_DEBUG_F( "mean_age_infection = %f/%f*365.\n", numerator, denominator );
        }
        else
        {
            mean_age_infection = 0; // necessary? KM comment - yes, if numerator = 0 then normal calc is OK; if denom is 0 (say, in an eradication context), then above calc will throw Inf/NaN/exception, depending on divide-by-zero handling.
        }
    }

    //------------------------------------------------------------------
    //   Campaign event related
    //------------------------------------------------------------------

    void Node::AddEventsFromOtherNodes( const std::vector<EventTrigger>& rTriggerList )
    {
        events_from_other_nodes.clear();
        for( auto trigger : rTriggerList )
        {
            events_from_other_nodes.push_back( trigger );
        }
    }

    // Determines if Node is in a defined lat-long polygon
    // checks for line crossings when extending a ray from the Node's location to increasing longitude
    // odd crosses included, even crossed excluded

    bool Node::IsInPolygon(float* vertex_coords, int numcoords)
    {
        bool inside = false;

        if (numcoords < 6) { return 0; }//no inclusion in a line or point
        if (numcoords % 2 != 0)
        {
            std::stringstream s ;
            s << "Error parsing polygon inclusion: numcords(=" << numcoords << ") is not even." << std::endl ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }

        float lat = GetLatitudeDegrees() ;
        float lon = GetLongitudeDegrees();
        for (int i = 0; i < numcoords / 2 - 1; i++) // increase to the second to last coordinate pair
        {
            // first check if latitude is between the vertex latitude coordinates
            if ((lat < vertex_coords[2 * i + 1]) != (lat < vertex_coords[2 * i + 3])) // also prevents divide by zero
            {
                if (lon < (vertex_coords[2 * i + 2] - vertex_coords[2 * i]) * (lat - vertex_coords[2 * i + 1]) / (vertex_coords[2 * i + 3] - vertex_coords[2 * i + 1]) + vertex_coords[2 * i])
                {
                    inside = !inside;   // crossing!
                }
            }
        }

        return inside;
    }

    // TODO: Cache results so we don't recalculate all the time. (that's why it's non-const)
    bool Node::IsInPolygon( const json::Array &poly )
    {
        bool inside = false;

        // Assume polygon was validated prior to us by NodeSet class for having at least 3 vertices.
        Array::const_iterator itPoly( poly.Begin()), itPolyEnd( poly.End() );
        QuickInterpreter poly_interp(poly);
        int i = 0;
        float lat = GetLatitudeDegrees() ;
        float lon = GetLongitudeDegrees();
        for (; itPoly != itPolyEnd; ++itPoly) // increase to the second to last coordinate pair
        {
            // first check if latitude is between the vertex latitude coordinates
            //if ((ndc->latitude < vertex_coords[2 * i + 1]) != (ndc->latitude < vertex_coords[2 * i + 3])) // also prevents divide by zero
            if (lat < float(poly_interp[i][1].As<Number>()) != (lat < float(poly_interp[i+1][1].As<Number>()))) // also prevents divide by zero
            {
                //if (ndc->longitude < (vertex_coords[2 * i + 2] - vertex_coords[2 * i]) * (ndc->latitude - vertex_coords[2 * i + 1]) / (vertex_coords[2 * i + 3] - vertex_coords[2 * i + 1]) + vertex_coords[2 * i])
                // for clarity
                float curLong  = float(poly_interp[i  ][0].As<Number>());
                float curLat   = float(poly_interp[i  ][1].As<Number>());
                float nextLong = float(poly_interp[i+1][0].As<Number>());
                float nextLat  = float(poly_interp[i+1][1].As<Number>());
                if (lon < (nextLong - curLong) * (lat - curLat) / (nextLat - curLat) + curLong)
                {
                    inside = !inside;   // crossing!
                }
            }
            i++;
        }
        return inside;
    }

    bool Node::IsInExternalIdSet( const std::vector<ExternalNodeId_t> &nodelist )
    {
        if( std::find( nodelist.begin(), nodelist.end(), externalId ) == nodelist.end() )
        {
            return false;
        }

        return true;
    }

    void Node::RemoveHuman( int index )
    {
        LOG_DEBUG_F( "Purging individual %d from node.\n", individualHumans[ index ]->GetSuid().data );
        individualHumans[ index ] = individualHumans.back();
        individualHumans.pop_back();
    }

    //------------------------------------------------------------------
    //   Assorted getters and setters
    //------------------------------------------------------------------

    std::vector<bool> Node::GetMigrationTypeEnabledFromDemographics() const
    {
        std::vector<bool> demog_enabled ;

        // Booleans from demographics; should be removed
        demog_enabled.push_back( true );
        demog_enabled.push_back( true );
        demog_enabled.push_back( true );
        demog_enabled.push_back( true );
        demog_enabled.push_back( true );

        return demog_enabled ;
    }


    RANDOMBASE* Node::GetRng()
    {
        // ------------------------------------------------------------------------------------
        // --- Use parent_sim here so that access to RNG is harder to get from the simulation.
        // --- Ideally, we don't want objects using the RNG from the simulation directly.
        // --- It is better that they either use it from the node or individual.
        // --- If it is a decision for a node, use the node GetRng().
        // --- If it is a decision for an individual, use the individual->GetRng()
        // ------------------------------------------------------------------------------------
        if( m_pRng == nullptr )
        {
            release_assert( parent_sim );
            if( parent_sim == nullptr )
            {
                throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "parent_sim", "ISimulation*" );
            }
            return parent_sim->GetRng();
        }
        else
        {
            return m_pRng;
        }
    }

    void Node::SetRng( RANDOMBASE* prng )
    {
        // don't change pointer in case created from serialization
        if( prng != nullptr )
        {
            m_pRng = prng;
        }
    }

    void Node::propagateContextToDependents()
    {
        INodeContext *context = getContextPointer();

        for (auto individual : individualHumans)
        {
            individual->SetContextTo(context);
        }
#ifndef DISABLE_CLIMATE
        if (localWeather)
        {
            localWeather->SetContextTo(context);
        }
#endif
        if (migration_info)
        {
            migration_info->SetContextTo(context);
        }

        if (event_context_host)
        {
            event_context_host->SetContextTo(context); 
        }
    }

    void Node::SetContextTo(ISimulationContext* context)
    {
        parent = context;
        propagateContextToDependents();
        demographics.SetContext( parent->GetDemographicsContext(), (INodeContext*)(this) );

        // needed to get access to RNG - see GetRng() for more info
        if( parent->QueryInterface( GET_IID( ISimulation ), (void**)&parent_sim ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ISimulation", "ISimulationContext" );
        }
        release_assert( parent_sim );
    }

    // INodeContext methods
    ISimulationContext* Node::GetParent() { return parent; }
    suids::suid Node::GetSuid() const { return suid; }
    suids::suid Node::GetNextInfectionSuid() { return parent->GetNextInfectionSuid(); }

    IMigrationInfo* Node::GetMigrationInfo() { return migration_info; }
    const NodeDemographics* Node::GetDemographics() const { return &demographics; }
    NPKeyValueContainer& Node::GetNodeProperties() { return node_properties; }

    // Methods for implementing time dependence in infectivity, birth rate, migration, etc.
    float Node::getSinusoidalCorrection(float sinusoidal_amplitude, float sinusoidal_phase) const
    {
        float correction = 1 + sinusoidal_amplitude * sin(2.0 * PI * (GetTime().time - sinusoidal_phase) / DAYSPERYEAR );
        LOG_DEBUG_F( "%s returning %f\n", __FUNCTION__, correction );
        return correction;
    }

    float Node::getBoxcarCorrection(float boxcar_amplitude, float boxcar_start_time, float boxcar_end_time) const
    {
        float correction = 1;
        //Handle cases in which start_time is earlier than end_time, and when end_time is earlier than start_time (i.e., when the high season extends over adjacent years)
        float modTime = fmod(GetTime().time, DAYSPERYEAR);
        
        if( (boxcar_start_time < boxcar_end_time) && (modTime > boxcar_start_time) && (modTime < boxcar_end_time) )
        {
            correction = 1 + boxcar_amplitude;
        }
        if( (boxcar_end_time < boxcar_start_time) &&  ( (modTime < boxcar_end_time) || (modTime > boxcar_start_time) ) )
        {
            correction = 1 + boxcar_amplitude;
        }
        return correction;
    }

    // Reporting methods
    const IdmDateTime&
    Node::GetTime()          const { return parent->GetSimulationTime(); }

    float
    Node::GetInfected()      const { return Infected; }

    float
    Node::GetSymptomatic() const { return symptomatic; }

    float
    Node::GetNewlySymptomatic() const { return newly_symptomatic; }

    float
    Node::GetStatPop()       const { return statPop; }

    float
    Node::GetBirths()        const { return Births; }

    float
    Node::GetCampaignCost()  const { return Campaign_Cost; }

    float
    Node::GetInfectivity()   const { return mInfectivity; }

    float
    Node::GetSusceptDynamicScaling() const { return susceptibility_dynamic_scaling; }

    ExternalNodeId_t
    Node::GetExternalID()    const { return externalId; }

    const Climate*
    Node::GetLocalWeather()    const { return localWeather; }

    long int
    Node::GetPossibleMothers() const { return Possible_Mothers ; }

    float
    Node::GetMeanAgeInfection()      const { return mean_age_infection; }

    INodeEventContext* Node::GetEventContext() { return (INodeEventContext*)event_context_host; }

    INodeContext *Node::getContextPointer()    { return this; }

    float Node::GetBasePopulationScaleFactor() const
    {
        return population_scaling_factor;
    }

    const SimulationConfig* Node::params() const
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    bool Node::IsValidTransmissionRoute( string& transmissionRoute )
    {
        std::transform(transmissionRoute.begin(), transmissionRoute.end(), transmissionRoute.begin(), ::tolower);
        bool isValid = ((transmissionRoute == "contact") || (transmissionRoute == "environmental"));
        return isValid;
    }

    int Node::calcGap()
    {
        int gap = 1;
        if( IndividualHumanConfig::enable_skipping )
        {
            release_assert( maxInfectionProb.size()>0 );
            float maxProb = GetMaxInfectionProb( TransmissionRoute::TRANSMISSIONROUTE_CONTACT );
            if (maxProb>=1.0)
            {
                gap=1;
            }
            else if (maxProb>0.0)
            {
                gap=int(ceil(log(1.0- GetRng()->e())/log(1.0-maxProb))); //geometric based on fMaxInfectionProb
            }
        }
        return gap;
    }

    void Node::computeMaxInfectionProb( float dt )
    {
        float maxProbRet = 0.0f;
        auto contagionByRouteMap = GetContagionByRoute();
        release_assert( contagionByRouteMap.size() > 0 );
        for( auto map_iter: contagionByRouteMap  )
        {
            auto route = map_iter.first;
            auto contagion = map_iter.second;
            auto contact_contagion = contagion;
            ProbabilityNumber prob = EXPCDF(-contact_contagion * dt);
            if( prob > maxProbRet )
            {
                maxProbRet = prob;
            }
            maxInfectionProb[ TransmissionRoute::TRANSMISSIONROUTE_CONTACT ] = maxProbRet;
            LOG_INFO_F( "The max probability of infection over the contact route (w/o immunity or interventions) for this node is: %f.\n", maxProbRet );
        } 
        gap = 1;
        bSkipping = false; // changes from individual to individual. Initialize to false
    }

    void Node::ValidateIntranodeTransmissionConfiguration()
    {
        bool oneOrMoreMatrices = false;

        for( auto p_ip : IPFactory::GetInstance()->GetIPList() )
        {
            if( p_ip->GetIntraNodeTransmission( GetExternalID() ).HasMatrix() )
            {
                oneOrMoreMatrices = true;

                string route_name = p_ip->GetIntraNodeTransmission( GetExternalID() ).GetRouteName();
                if( !IsValidTransmissionRoute( route_name ) )
                {
                    ostringstream message;
                    message << "HINT Configuration: Unsupported route '" << route_name << "'." << endl;
                    message << "For generic sims, only \"contact\" route (contagion is reset at each timestep) is supported." << endl;
                    message << "For malaria sims, transmissionMatrix is not supported (yet)." << endl;
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, message.str().c_str());
                }

                oneOrMoreMatrices = true;
            }
        }

        if (!oneOrMoreMatrices) 
        {
            LOG_WARN("HINT Configuration: heterogeneous intranode transmission is enabled, but no transmission matrices were found in the demographics file(s).\n");
            // TODO throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HINT Configuration: No transmission matrices were found in the demographics file(s).");
        }
    }

    REGISTER_SERIALIZABLE(Node);

    void Node::serialize(IArchive& ar, Node* obj)
    {
        Node& node = *obj;

        if ( ar.IsWriter() )    // If we are writing out data, use the specified mask to skip writing certain data.
        {
            node.serializationFlags = node.serializationFlagsDefault & ~SerializationParameters::GetInstance()->GetSerializationWriteMask();
        }

        ar.labelElement("serializationFlags") & (uint32_t&)node.serializationFlags;

        if ( ar.IsReader() )    // If we are reading in data, use the specified mask to skip reading certain data.
        {
            node.serializationFlags = node.serializationFlags & ~SerializationParameters::GetInstance()->GetSerializationReadMask();
        }

        ar.labelElement("suid")       & node.suid;
        ar.labelElement("externalId") & node.externalId;
            ar.labelElement( "m_pRng" ) & node.m_pRng;
            ar.labelElement( "m_IndividualHumanSuidGenerator" ) & node.m_IndividualHumanSuidGenerator;
            
        if ( node.serializationFlags.test( SerializationFlags::Population ) )
        {
            ar.labelElement("individualHumans"   ) & node.individualHumans;
            ar.labelElement("home_individual_ids") & node.home_individual_ids;
        }

        if ( node.serializationFlags.test( SerializationFlags::Parameters ) )
        {
            ar.labelElement("ind_sampling_type")                            & (uint32_t&)node.ind_sampling_type;
            ar.labelElement("population_density_infectivity_correction")    & (uint32_t&)node.population_density_infectivity_correction;
            ar.labelElement("age_initialization_distribution_type")         & (uint32_t&)node.age_initialization_distribution_type;

            ar.labelElement("susceptibility_scaling_type")  & (uint32_t&)node.susceptibility_scaling_type;
            ar.labelElement("susceptibility_scaling")       & node.susceptibility_scaling;
            ar.labelElement("susceptibility_scaling_rate")  & node.susceptibility_scaling_rate;

            ar.labelElement("base_sample_rate") & node.base_sample_rate;

            ar.labelElement("enable_demographics_risk")             & node.enable_demographics_risk;
            ar.labelElement("max_sampling_cell_pop")                & node.max_sampling_cell_pop;
            ar.labelElement("sample_rate_birth")                    & node.sample_rate_birth;
            ar.labelElement("sample_rate_0_18mo")                   & node.sample_rate_0_18mo;
            ar.labelElement("sample_rate_18mo_4yr")                 & node.sample_rate_18mo_4yr;
            ar.labelElement("sample_rate_5_9")                      & node.sample_rate_5_9;
            ar.labelElement("sample_rate_10_14")                    & node.sample_rate_10_14;
            ar.labelElement("sample_rate_15_19")                    & node.sample_rate_15_19;
            ar.labelElement("sample_rate_20_plus")                  & node.sample_rate_20_plus;
            ar.labelElement("rel_sample_rate_immune")               & node.rel_sample_rate_immune;
            ar.labelElement("immune_threshold_for_downsampling")    & node.immune_threshold_for_downsampling;

            ar.labelElement("population_density_c50")                   & node.population_density_c50;
            ar.labelElement("population_scaling")                       & (uint32_t&)node.population_scaling;
            ar.labelElement("population_scaling_factor")                & node.population_scaling_factor;
            ar.labelElement("enable_maternal_infection_transmission")   & node.enable_maternal_infection_transmission;
            ar.labelElement("prob_maternal_infection_transmission")     & node.prob_maternal_infection_transmission;

            ar.labelElement("vital_dynamics")               & node.vital_dynamics;
            ar.labelElement("enable_initial_prevalence")    & node.enable_initial_prevalence;
            ar.labelElement("enable_infectivity_reservoir") & node.enable_infectivity_reservoir;
            ar.labelElement("vital_birth")                  & node.vital_birth;
            ar.labelElement("vital_birth_dependence")       & (uint32_t&)node.vital_birth_dependence;
            ar.labelElement("vital_birth_time_dependence")  & (uint32_t&)node.vital_birth_time_dependence;
            ar.labelElement("x_birth")                      & node.x_birth;
            ar.labelElement("enable_natural_mortality")     & node.enable_natural_mortality;
            ar.labelElement("x_othermortality")             & node.x_othermortality;
            ar.labelElement("vital_death_dependence")       & (uint32_t&)node.vital_death_dependence;

            ar.labelElement("infectivity_scaling")      & (uint32_t&)node.infectivity_scaling;

            ar.labelElement("infectivity_sinusoidal_forcing_amplitude") & node.infectivity_sinusoidal_forcing_amplitude;
            ar.labelElement("infectivity_sinusoidal_forcing_phase")     & node.infectivity_sinusoidal_forcing_phase;
            ar.labelElement("infectivity_boxcar_forcing_amplitude")     & node.infectivity_boxcar_forcing_amplitude;

            ar.labelElement("infectivity_boxcar_start_time")            & node.infectivity_boxcar_start_time;

            ar.labelElement("infectivity_boxcar_end_time")              & node.infectivity_boxcar_end_time;
            ar.labelElement("infectivity_exponential_baseline")         & node.infectivity_exponential_baseline;
            ar.labelElement("infectivity_exponential_rate")             & node.infectivity_exponential_rate;
            ar.labelElement("infectivity_exponential_delay")            & node.infectivity_exponential_delay;
            ar.labelElement("infectivity_reservoir_end_time")           & node.infectivity_reservoir_end_time;
            ar.labelElement("infectivity_reservoir_size")               & node.infectivity_reservoir_size;
            ar.labelElement("infectivity_reservoir_start_time")         & node.infectivity_reservoir_start_time;
            ar.labelElement("birth_rate_sinusoidal_forcing_amplitude")  & node.birth_rate_sinusoidal_forcing_amplitude;
            ar.labelElement("birth_rate_sinusoidal_forcing_phase")      & node.birth_rate_sinusoidal_forcing_phase;
            ar.labelElement("birth_rate_boxcar_forcing_amplitude")      & node.birth_rate_boxcar_forcing_amplitude;
            ar.labelElement("birth_rate_boxcar_start_time")             & node.birth_rate_boxcar_start_time;
            ar.labelElement("birth_rate_boxcar_end_time")               & node.birth_rate_boxcar_end_time;
        }

        if ( node.serializationFlags.test( SerializationFlags::Properties ) ) 
        {
            ar.labelElement("_latitude") & node._latitude;
            ar.labelElement("_longitude") & node._longitude;
            ar.labelElement("birthrate") & node.birthrate;
            ar.labelElement("family_waiting_to_migrate") & node.family_waiting_to_migrate;
            ar.labelElement("family_migration_destination") & node.family_migration_destination;
            ar.labelElement("family_migration_type") & (uint32_t&)node.family_migration_type;
            ar.labelElement("family_time_until_trip") & node.family_time_until_trip;
            ar.labelElement("family_time_at_destination") & node.family_time_at_destination;
            ar.labelElement("family_is_destination_new_home") & node.family_is_destination_new_home;
            ar.labelElement("base_sample_rate") & node.base_sample_rate;
            ar.labelElement("susceptibility_dynamic_scaling") & node.susceptibility_dynamic_scaling;
            ar.labelElement("node_properties") & node.node_properties;
            ar.labelElement("statPop") & node.statPop;
            ar.labelElement("Infected") & node.Infected;
            ar.labelElement("Births") & node.Births;
            ar.labelElement("Disease_Deaths") & node.Disease_Deaths;
            ar.labelElement("new_infections") & node.new_infections;
            ar.labelElement("new_reportedinfections") & node.new_reportedinfections;
            ar.labelElement("Cumulative_Infections") & node.Cumulative_Infections;
            ar.labelElement("Cumulative_Reported_Infections") & node.Cumulative_Reported_Infections;
            ar.labelElement("Campaign_Cost") & node.Campaign_Cost;
            ar.labelElement("mean_age_infection") & node.mean_age_infection;
            ar.labelElement("newInfectedPeopleAgeProduct") & node.newInfectedPeopleAgeProduct;
            ar.labelElement("mInfectivity") & node.mInfectivity;
            ar.labelElement("prob_maternal_transmission") & node.prob_maternal_infection_transmission;
            ar.labelElement("susceptibility_dist_type") & (uint32_t&)node.susceptibility_dist_type;
            ar.labelElement("susceptibility_dist1") & node.susceptibility_dist1;
            ar.labelElement("susceptibility_dist2") & node.susceptibility_dist2;
            ar.labelElement("risk_dist_type") & (uint32_t&)node.risk_dist_type;
            ar.labelElement("risk_dist1") & node.risk_dist1;
            ar.labelElement("risk_dist2") & node.risk_dist2;
            ar.labelElement("migration_dist_type") & (uint32_t&)node.migration_dist_type;
            ar.labelElement("migration_dist1") & node.migration_dist1;
            ar.labelElement("migration_dist2") & node.migration_dist2;
            ar.labelElement("routes") & node.routes;
            ar.labelElement("bSkipping") & node.bSkipping;
        }
    }
}
