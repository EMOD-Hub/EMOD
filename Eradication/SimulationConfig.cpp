
#include "stdafx.h"
#ifdef WIN32
#include "windows.h"
#endif

#include "SimulationConfig.h"

// These headers are included here mostly for 
// many constants defines which eventually in SS4 will be
// moved back to where they belong so that we don't have to 
// include all these header files
#include "Common.h"
#include "Debug.h"
#include "Configure.h"
#include "InterventionEnums.h"
#include "Log.h"
#include "Sugar.h"
#include "NoCrtWarnings.h"

#ifndef DISABLE_VECTOR
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#endif

SETUP_LOGGING( "SimulationConfig" )

namespace Kernel
{

ISimulationConfigFactory * SimulationConfigFactory::_instance = nullptr;
static const char* malaria_drug_placeholder_string = "<malaria_drug_name_goes_here>";

ISimulationConfigFactory * SimulationConfigFactory::getInstance()
{
    if( _instance == nullptr )
    {
        _instance = new SimulationConfigFactory();
    }
    return _instance;
}

SimulationConfig* SimulationConfigFactory::CreateInstance(Configuration * config)
{
    SimulationConfig *SimConfig =  _new_ SimulationConfig();
    release_assert(SimConfig);
    if (SimConfig)
    {           
        SimConfig->SetFixedParameters(config);
        if (SimConfig->Configure(config))
        {
            SimConfig->AddRef();
        }
        else
        {
            SimConfig->Release();
            SimConfig = nullptr;
        }
    }
    return SimConfig;
}

void SimulationConfigFactory::Register(string classname, instantiator_function_t _if)
{  
    getRegisteredClasses()[classname] = _if;
}

support_spec_map_t& SimulationConfigFactory::getRegisteredClasses()
{ 
    static support_spec_map_t registered_classes; 
    return registered_classes; 
}

IMPLEMENT_FACTORY_REGISTERED(SimulationConfig)

BEGIN_QUERY_INTERFACE_BODY(SimulationConfig)
     HANDLE_INTERFACE(IConfigurable)
END_QUERY_INTERFACE_BODY(SimulationConfig)

SimulationConfig::~SimulationConfig()
{
}

void SimulationConfig::SetFixedParameters(Configuration * inputJson)
{
    SimType::Enum sim_type;
    initConfig("Simulation_Type", sim_type, inputJson, MetadataDescriptor::Enum(Simulation_Type_DESC_TEXT, Simulation_Type_DESC_TEXT, MDD_ENUM_ARGS(SimType))); // simulation only (???move)

    switch (sim_type)
    {
    case SimType::MALARIA_SIM:
        inputJson->Add( "Enable_Initial_Susceptibility_Distribution", 0 ); //must exist because Enable_Immunity: 1
        inputJson->Add( "Enable_Susceptibility_Scaling", 0 ); // not used in Malaria
        inputJson->Add( "Enable_Immune_Decay", 0 ); //not used in Malaria
        break;
    case SimType::HIV_SIM:
        // inputJson->Add("Enable_Immunity", 1);    // Immunity is already enabled in the code. Setting it here adds parameter
                                                    // Enable_Immunity to the configuration what then requires all paramters that 
                                                    // depend on Enable_Immunity (also those not needed for HIV) to be specified. 
                                                    // For SimType HIV this means that all the Post_Infection_XXX parameters must be configured although they are not used.
        inputJson->Add("Enable_Immune_Decay", 0);   //must exist because Enable_Immunity: 1
        inputJson->Add("Enable_Initial_Susceptibility_Distribution", 0); //must exist because Enable_Immunity: 1
        inputJson->Add("Enable_Maternal_Infection_Transmission", 1);
        inputJson->Add("Enable_Vital_Dynamics", 1);
        break;
    }
}

bool SimulationConfig::Configure(const Configuration * inputJson)
{
    LOG_DEBUG( "Configure\n" );

    release_assert(inputJson);

    m_jsonConfig = inputJson;

    // --------------------------------------------------
    // --- Define the parameters to be read from the JSON
    // --------------------------------------------------

    // ------
    // Generic Parameters
    // ------
    initConfig( "Simulation_Type", sim_type,  inputJson, MetadataDescriptor::Enum(Simulation_Type_DESC_TEXT, Simulation_Type_DESC_TEXT, MDD_ENUM_ARGS(SimType)) ); // simulation only (???move)

    initConfigTypeMap( "Simulation_Duration", &Sim_Duration, Simulation_Duration_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // 'global'? (controller only)
    initConfigTypeMap( "Simulation_Timestep", &Sim_Tstep,    Simulation_Timestep_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // 'global'? (controller only)
    initConfigTypeMap( "Start_Time",          &starttime,    Start_Time_DESC_TEXT,          0.0f, 1000000.0f, 1.0f ); // simulation only (actually Climate also!)
    initConfigTypeMap( "Config_Name",         &ConfigName,   Config_Name_DESC_TEXT );

    bool demographics_builtin = false;
    initConfigTypeMap( "Enable_Demographics_Builtin",               &demographics_builtin,    Enable_Demographics_Builtin_DESC_TEXT, false ); // 'global' (3 files)
    initConfigTypeMap( "Default_Geography_Initial_Node_Population", &default_node_population, Default_Geography_Initial_Node_Population_DESC_TEXT, 0, 1e6, 1000, "Enable_Demographics_Builtin");
    initConfigTypeMap( "Default_Geography_Torus_Size",              &default_torus_size,      Default_Geography_Torus_Size_DESC_TEXT,              3, 100,   10, "Enable_Demographics_Builtin");
    initConfigTypeMap( "Node_Grid_Size", &node_grid_size, Node_Grid_Size_DESC_TEXT, 0.004167f, 90.0f, 0.004167f );

    initConfig( "Migration_Model",       migration_structure,    inputJson, MetadataDescriptor::Enum(Migration_Model_DESC_TEXT,       Migration_Model_DESC_TEXT,       MDD_ENUM_ARGS(MigrationStructure)) ); // 'global'

    initConfigTypeMap( "Enable_Interventions", &interventions, Enable_Interventions_DESC_TEXT, false );

    initConfigTypeMap( "Enable_Heterogeneous_Intranode_Transmission",
                       &heterogeneous_intranode_transmission_enabled,
                       Enable_Heterogeneous_Intranode_Transmission_DESC_TEXT,
                       false,
                       "Simulation_Type",
                       "GENERIC_SIM" );


    //vector enums
    if (sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM )
    {
        VectorInitConfig( inputJson );
    }

    // --------------------------------------
    // --- Read the parameters from the JSON
    // --------------------------------------
    LOG_DEBUG_F( "Calling main Configure..., use_defaults = %d\n", JsonConfigurable::_useDefaults );

    bool ret = JsonConfigurable::Configure( inputJson );
    demographics_initial = !demographics_builtin;

    // ---------------------------------------
    // --- Return if just generating schema
    // ---------------------------------------
    if( JsonConfigurable::_dryrun == true )
    {
        return true;
    }

    // ------------------------------------------------------------------
    // --- Update, check, and read other parameters given the input data
    // ------------------------------------------------------------------
    lloffset = 0.5f * node_grid_size;

    if (sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM )
    {
        VectorCheckConfig( inputJson );
    }

    return ret;
}

QuickBuilder SimulationConfig::GetSchema()
{
    LOG_DEBUG( "GetSchema\n" );
    json::Object * job = new json::Object( GetSchemaBase() );
    json::QuickBuilder retJson( *job );

    return retJson;
}

SimulationConfig::SimulationConfig()
    : migration_structure(MigrationStructure::NO_MIGRATION) 
    , sim_type(SimType::GENERIC_SIM) 
    , demographics_initial(false)
    , default_torus_size( 10 )
    , default_node_population( 1000 )
    , lloffset(0) 
    , interventions(false)
    , airtemperature(-42.0f)
    , landtemperature(-42.0f)
    , rainfall(-42.0f)
    , humidity(-42.0f)
    , airtemperature_offset(-42.0f)
    , landtemperature_offset(-42.0f)
    , rainfall_scale_factor(-42.0f)
    , humidity_scale_factor(-42.0f)
    , airtemperature_variance(-42.0f)
    , landtemperature_variance(-42.0f)
    , rainfall_variance_enabled(false)
    , humidity_variance(-42.0f)
    , heterogeneous_intranode_transmission_enabled(false)
    , Sim_Duration(-42.0f)
    , Sim_Tstep(-42.0f)
    , starttime(0.0f)
    , node_grid_size(0.0f)
    , Run_Number(-42)
    , ConfigName("UNSPECIFIED")
    , airmig_filename("UNSPECIFIED")
    , campaign_filename("UNSPECIFIED")
    , climate_airtemperature_filename("UNSPECIFIED")
    , climate_koppen_filename("UNSPECIFIED")
    , climate_landtemperature_filename("UNSPECIFIED")
    , climate_rainfall_filename("UNSPECIFIED")
    , climate_relativehumidity_filename("UNSPECIFIED")
    , loadbalance_filename("UNSPECIFIED")
    , localmig_filename("UNSPECIFIED")
    , regionmig_filename("UNSPECIFIED")
    , seamig_filename("UNSPECIFIED")
    , m_jsonConfig(nullptr)
    , vector_params(nullptr)
{
#ifndef DISABLE_VECTOR
    vector_params = new VectorParameters();
#endif
}

// ----------------------------------------------------------------------------
// --- VectorParameters
// ----------------------------------------------------------------------------

void SimulationConfig::VectorInitConfig( const Configuration* inputJson )
{
#ifndef DISABLE_VECTOR
    initConfig( "Vector_Sampling_Type",             vector_params->vector_sampling_type,             inputJson, MetadataDescriptor::Enum(Vector_Sampling_Type_DESC_TEXT,             Vector_Sampling_Type_DESC_TEXT,             MDD_ENUM_ARGS(VectorSamplingType))      ); // node (vector) only
    initConfig( "Egg_Hatch_Delay_Distribution",     vector_params->egg_hatch_delay_dist,             inputJson, MetadataDescriptor::Enum(Egg_Hatch_Delay_Distribution_DESC_TEXT,     Egg_Hatch_Delay_Distribution_DESC_TEXT,     MDD_ENUM_ARGS(EggHatchDelayDist))       ); // vector pop only
    initConfig( "Egg_Saturation_At_Oviposition",    vector_params->egg_saturation,                   inputJson, MetadataDescriptor::Enum(Egg_Saturation_At_Oviposition_DESC_TEXT,    Egg_Saturation_At_Oviposition_DESC_TEXT,    MDD_ENUM_ARGS(EggSaturation))           ); // vector pop only
    initConfig( "Larval_Density_Dependence",        vector_params->larval_density_dependence,        inputJson, MetadataDescriptor::Enum(Larval_Density_Dependence_DESC_TEXT,        Larval_Density_Dependence_DESC_TEXT,        MDD_ENUM_ARGS(LarvalDensityDependence)) ); // vector pop only
    initConfig( "Egg_Hatch_Density_Dependence",     vector_params->egg_hatch_density_dependence,     inputJson, MetadataDescriptor::Enum(Egg_Hatch_Density_Dependence_DESC_TEXT,     Egg_Hatch_Density_Dependence_DESC_TEXT,     MDD_ENUM_ARGS(EggHatchDensityDependence)) ); // vector pop only

    initConfig( "Vector_Larval_Rainfall_Mortality", vector_params->vector_larval_rainfall_mortality, inputJson, MetadataDescriptor::Enum(Vector_Larval_Rainfall_Mortality_DESC_TEXT, Vector_Larval_Rainfall_Mortality_DESC_TEXT, MDD_ENUM_ARGS(VectorRainfallMortality)) );
    // get the c50 value if there is rainfall mortality
    initConfigTypeMap( "Larval_Rainfall_Mortality_Threshold", &(vector_params->larval_rainfall_mortality_threshold), Larval_Rainfall_Mortality_Threshold_DESC_TEXT, 0.01f, 1000.0f, 100.0f, "Vector_Larval_Rainfall_Mortality", "SIGMOID,SIGMOID_HABITAT_SHIFTING" );

    //Q added stuff
    initConfigTypeMap( "Enable_Temperature_Dependent_Egg_Hatching", &(vector_params->temperature_dependent_egg_hatching), Enable_Temperature_Dependent_Egg_Hatching_DESC_TEXT, false );
    initConfigTypeMap( "Egg_Arrhenius1", &(vector_params->eggarrhenius1), Egg_Arrhenius1_DESC_TEXT, 0.0f, 10000000000.0f, 61599956.864f, "Enable_Temperature_Dependent_Egg_Hatching" );
    initConfigTypeMap( "Egg_Arrhenius2", &(vector_params->eggarrhenius2), Egg_Arrhenius2_DESC_TEXT, 0.0f, 10000000000.0f, 5754.033f, "Enable_Temperature_Dependent_Egg_Hatching" );
    initConfigTypeMap( "Enable_Egg_Mortality", &(vector_params->egg_mortality), Enable_Egg_Mortality_DESC_TEXT, false ); // not hooked up yet
    initConfigTypeMap( "Enable_Drought_Egg_Hatch_Delay", &(vector_params->delayed_hatching_when_habitat_dries_up), Enable_Drought_Egg_Hatch_Delay_DESC_TEXT, false );
    initConfigTypeMap( "Drought_Egg_Hatch_Delay", &(vector_params->droughtEggHatchDelay), Drought_Egg_Hatch_Delay_DESC_TEXT, 0.0f, 1.0f, 0.33f, "Enable_Drought_Egg_Hatch_Delay" );

    // get the larval density dependence parameters 
    initConfigTypeMap( "Larval_Density_Mortality_Scalar", &(vector_params->larvalDensityMortalityScalar), Larval_Density_Mortality_Scalar_DESC_TEXT,  0.01f, 1000.0f, 10.0f, "Larval_Density_Dependence", "GRADUAL_INSTAR_SPECIFIC,LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY" );
    initConfigTypeMap( "Larval_Density_Mortality_Offset", &(vector_params->larvalDensityMortalityOffset), Larval_Density_Mortality_Offset_DESC_TEXT, 0.0001f, 1000.0f, 0.1f, "Larval_Density_Dependence", "GRADUAL_INSTAR_SPECIFIC,LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY" );

    //initConfigTypeMap( "Enable_Vector_Species_Habitat_Competition", &enable_vector_species_habitat_competition, VECTOR_Enable_Vector_Species_Habitat_Competition, false );

    initConfigTypeMap( "Enable_Vector_Species_Report", &(vector_params->enable_vector_species_report), VECTOR_Enable_Vector_Species_Report_DESC_TEXT, false );
    initConfigTypeMap( "Enable_Vector_Aging",          &(vector_params->vector_aging),                 Enable_Vector_Aging_DESC_TEXT,                 false );

    initConfigTypeMap( "Mean_Egg_Hatch_Delay",                       &(vector_params->meanEggHatchDelay),                   Mean_Egg_Hatch_Delay_DESC_TEXT, 0, 120, 0 );
    initConfigTypeMap( "Wolbachia_Mortality_Modification",           &(vector_params->WolbachiaMortalityModification),      Wolbachia_Mortality_Modification_DESC_TEXT, 0, 100, 1 );
    initConfigTypeMap( "Wolbachia_Infection_Modification",           &(vector_params->WolbachiaInfectionModification),      Wolbachia_Infection_Modification_DESC_TEXT, 0, 100, 1 );

    initConfigTypeMap( "Human_Feeding_Mortality", &(vector_params->human_feeding_mortality), Human_Feeding_Mortality_DESC_TEXT, 0.0f, 1.0f, 0.1f );

    initConfigTypeMap( "Temporary_Habitat_Decay_Factor",   &(vector_params->tempHabitatDecayScalar),        Temporary_Habitat_Decay_Factor_DESC_TEXT,   0.001f,    100.0f, 0.05f );
    initConfigTypeMap( "Semipermanent_Habitat_Decay_Rate", &(vector_params->semipermanentHabitatDecayRate), Semipermanent_Habitat_Decay_Rate_DESC_TEXT, 0.0001f,   100.0f, 0.01f );
    initConfigTypeMap( "Rainfall_In_mm_To_Fill_Swamp",     &(vector_params->mmRainfallToFillSwamp),         Rainfall_In_mm_To_Fill_Swamp_DESC_TEXT,        1.0f, 10000.0f, 1000.0f );

    // This is a key parameter for the mosquito ecology and can vary quite a lot
    initConfigTypeMap( "x_Temporary_Larval_Habitat", &(vector_params->x_tempLarvalHabitat), x_Temporary_Larval_Habitat_DESC_TEXT, 0.0f, 10000.0f, 1.0f ); // should this be renamed vector_weight?

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! Moved the initialization of Vector_Species_Params to SimulationVector
    // !!! so that it can be initialized after ParasiteGenetics.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //initConfigComplexCollectionType( "Vector_Species_Params", &(vector_params->vector_species), Vector_Species_Params_DESC_TEXT );
#endif // DISABLE_VECTOR
}

void SimulationConfig::VectorCheckConfig( const Configuration* inputJson )
{
#ifndef DISABLE_VECTOR
    if( Sim_Tstep != 1.0f )
    {
        // There has not been sufficient testing for vector related simulations when dt != 1.
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Vector-based simulations assume that the Simulation_Timestep = 1" );
    }

    if( !vector_params->temperature_dependent_egg_hatching &&
        (vector_params->egg_hatch_delay_dist != EggHatchDelayDist::NO_DELAY) &&
        (vector_params->meanEggHatchDelay == 0.0f) )
    {
        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                "Egg_Hatch_Delay_Distribution", EggHatchDelayDist::pairs::lookup_key( vector_params->egg_hatch_delay_dist ),
                                                "Mean_Egg_Hatch_Delay", "0.0",
                                                "Mean_Egg_Hatch_Delay cannot be zero if Enable_Temperature_Dependent_Egg_Hatching=false and Egg_Hatch_Delay_Distribution != NO_DELAY" );
    }

#endif // DISABLE_VECTOR
}
}
