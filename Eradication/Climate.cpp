
#include "stdafx.h"
#include <iomanip>
#include <stdio.h>
#include "Climate.h"
#include "ClimateKoppen.h"
#include "ClimateByData.h"
#include "ClimateConstant.h"
#include "Common.h"
#include "Debug.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "Log.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "INodeContext.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "Climate" )

// Used with ParseMetadataForFile
#define METADATA            "Metadata"
#define ID_REFERENCE        "IdReference"
#define UPDATE_RESOLUTION   "UpdateResolution"
#define DATAVALUE_COUNT     "DatavalueCount"
#define SCHEMA_VERSION      "WeatherSchemaVersion"
#define CELL_COUNT          "WeatherCellCount"
#define NODE_COUNT          "NodeCount"
#define DTK_NODE_COUNT      "NumberDTKNodes"
#define NODE_OFFSETS        "NodeOffsets"

namespace Kernel {

    const float Climate::min_airtemp  = -55;      // Celsius
    const float Climate::max_airtemp  =  45;      // Celsius
    const float Climate::min_landtemp = -55;      // Celsius
    const float Climate::max_landtemp =  60;      // Celsius
    const float Climate::max_rainfall =  0.150F;  // meters/day

    ClimateStructure::Enum ClimateFactory::climate_structure = ClimateStructure::CLIMATE_OFF;

    bool
    Climate::Configure(
        const Configuration* config
    )
    {
        LOG_DEBUG( "Configure\n" );

        enable_climate_stochasticity = true;

        initConfigTypeMap( "Enable_Climate_Stochasticity", &enable_climate_stochasticity, Enable_Climate_Stochasticity_DESC_TEXT, false, "Climate_Model", "CLIMATE_CONSTANT,CLIMATE_BY_DATA" );
        initConfigTypeMap( "Air_Temperature_Variance", &airtemperature_variance, Air_Temperature_Variance_DESC_TEXT, 0.0f, 5.0f, 2.0f, "Enable_Climate_Stochasticity" );
        initConfigTypeMap( "Land_Temperature_Variance", &landtemperature_variance, Land_Temperature_Variance_DESC_TEXT, 0.0f, 7.0f, 2.0f, "Enable_Climate_Stochasticity" );
        initConfigTypeMap( "Enable_Rainfall_Stochasticity", &rainfall_variance_enabled, Enable_Rainfall_Stochasticity_DESC_TEXT, true, "Enable_Climate_Stochasticity" );
        initConfigTypeMap( "Relative_Humidity_Variance", &humidity_variance, Relative_Humidity_Variance_DESC_TEXT, 0.0f, 0.12f, 0.05f, "Enable_Climate_Stochasticity" );

        bool bRet = JsonConfigurable::Configure( config );
        base_rainfall /= MILLIMETERS_PER_METER;
        return bRet;
    }

    BEGIN_QUERY_INTERFACE_BODY(Climate)
    END_QUERY_INTERFACE_BODY(Climate)
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Climate.General,ClimateFactory)
    BEGIN_QUERY_INTERFACE_BODY(ClimateFactory)
    END_QUERY_INTERFACE_BODY(ClimateFactory)

    Climate::Climate(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent)
        : climate_update_resolution(update_resolution)
        , base_airtemperature(-FLT_MAX)
        , base_landtemperature(-FLT_MAX)
        , base_rainfall(-FLT_MAX)
        , base_humidity(-FLT_MAX)
        , airtemperature_offset(-FLT_MAX)
        , landtemperature_offset(-FLT_MAX)
        , rainfall_scale_factor(FLT_MIN)
        , humidity_scale_factor(FLT_MIN)
        , enable_climate_stochasticity(false)
        , airtemperature_variance(2.0f)      //see default in initConfigTypeMap
        , landtemperature_variance(2.0f)     //see default in initConfigTypeMap
        , rainfall_variance_enabled(false)
        , humidity_variance(FLT_MAX)
        , m_airtemperature(-FLT_MAX)
        , m_landtemperature(-FLT_MAX)
        , m_accumulated_rainfall(FLT_MAX)
        , m_humidity(-FLT_MAX)
        , parent(_parent)
    {
        switch(update_resolution)
        {
            case ClimateUpdateResolution::CLIMATE_UPDATE_YEAR:       resolution_correction = 1.0f / DAYSPERYEAR; break;
            case ClimateUpdateResolution::CLIMATE_UPDATE_MONTH:      resolution_correction = 1.0f / IDEALDAYSPERMONTH; break;
            case ClimateUpdateResolution::CLIMATE_UPDATE_WEEK:       resolution_correction = 1.0f / DAYSPERWEEK; break;
            case ClimateUpdateResolution::CLIMATE_UPDATE_DAY:        resolution_correction = 1.0f; break;
            case ClimateUpdateResolution::CLIMATE_UPDATE_HOUR:       resolution_correction = HOURSPERDAY; break;
            default:                                                 resolution_correction = 0.0f; break;
        }
    }

    void Climate::UpdateWeather( float time, float dt, RANDOMBASE* pRNG, bool initialization )
    {
        // -----------------------------------------------------------------------------------
        // --- We don't want to add stochasticity during initialization because it can change
        // --- the random number stream compared when running from a serialized file comapred
        // --- to running the full sim
        // -----------------------------------------------------------------------------------
        if( enable_climate_stochasticity && !initialization )
            AddStochasticity( pRNG, airtemperature_variance, landtemperature_variance, rainfall_variance_enabled, humidity_variance );

        // cap values to within physically-possible bounds
        if(m_humidity > 1)
            m_humidity = 1;
        else if(m_humidity < 0)
            m_humidity = 0;

        if(m_accumulated_rainfall < 0)
            m_accumulated_rainfall = 0;
    }

    void Climate::AddStochasticity( RANDOMBASE* pRNG, float airtemp_variance, float landtemp_variance, bool rainfall_variance_enabled, float humidity_variance )
    {
        // air-temp
        if(airtemp_variance != 0.0)
            m_airtemperature += float( pRNG->eGauss() * airtemp_variance ); // varies as a Gaussian with stdev as specified in degree C

        // land-temp
        if(landtemp_variance != 0.0)
            m_landtemperature += float( pRNG->eGauss() * landtemp_variance ); // varies as a Gaussian with stdev as specified in degree C

        //rainfall
        if(rainfall_variance_enabled)
            if(m_accumulated_rainfall > 0.0)
                m_accumulated_rainfall = float( pRNG->expdist(1.0 / m_accumulated_rainfall) ); // varies over exponential distribution with mean of calculated rainfall value

        // humidity
        if(humidity_variance != 0.0)
            m_humidity += float( pRNG->eGauss() * humidity_variance ); // varies as a Gaussian with stdev as specified in %
    }

    ClimateFactory *
    ClimateFactory::CreateClimateFactory(boost::bimap<ExternalNodeId_t, suids::suid> * nodeid_suid_map, const ::Configuration *config, const string idreference)
    {
        ClimateFactory* factory = _new_ ClimateFactory(nodeid_suid_map);
        if(!factory->Initialize(config, idreference))
        {
            delete factory;
            factory = nullptr;
        }

        return factory;
    }

    ClimateFactory::ClimateFactory( boost::bimap<ExternalNodeId_t, suids::suid> * nodeid_suid_map )
        : climate_airtemperature_filename( "" )
        , climate_landtemperature_filename( "" )
        , climate_rainfall_filename( "" )
        , climate_relativehumidity_filename( "" )
        , climate_update_resolution(ClimateUpdateResolution::CLIMATE_UPDATE_DAY)
        , num_datavalues(0)
        , num_nodes(0)
        , num_badnodes(-1)
        , start_time(-1.0f)
    {
        num_badnodes = 0;
        this->nodeid_suid_map = nodeid_suid_map;
    }


    bool
    ClimateFactory::Configure(
        const Configuration* config
    )
    {
        LOG_DEBUG( "Configure\n" );

        initConfig( "Climate_Model", climate_structure, config, MetadataDescriptor::Enum("climate_structure", Climate_Model_DESC_TEXT, MDD_ENUM_ARGS(ClimateStructure)), "Simulation_Type", "VECTOR_SIM, MALARIA_SIM" );

        initConfig( "Climate_Update_Resolution", climate_update_resolution, config, MetadataDescriptor::Enum("climate_update_resolution", Climate_Update_Resolution_DESC_TEXT, MDD_ENUM_ARGS(ClimateUpdateResolution)), "Climate_Model", "CLIMATE_CONSTANT,CLIMATE_BY_DATA,CLIMATE_KOPPEN" );

        initConfigTypeMap( "Air_Temperature_Filename",   &climate_airtemperature_filename,   Air_Temperature_Filename_DESC_TEXT,   "", "Climate_Model", "CLIMATE_BY_DATA" );
        initConfigTypeMap( "Land_Temperature_Filename",  &climate_landtemperature_filename,  Land_Temperature_Filename_DESC_TEXT,  "", "Climate_Model", "CLIMATE_BY_DATA" );
        initConfigTypeMap( "Rainfall_Filename",          &climate_rainfall_filename,         Rainfall_Filename_DESC_TEXT,          "", "Climate_Model", "CLIMATE_BY_DATA" );
        initConfigTypeMap( "Relative_Humidity_Filename", &climate_relativehumidity_filename, Relative_Humidity_Filename_DESC_TEXT, "", "Climate_Model", "CLIMATE_BY_DATA" );

        initConfigTypeMap( "Koppen_Filename", &climate_koppen_filename, Koppen_Filename_DESC_TEXT, "", "Climate_Model", "CLIMATE_KOPPEN" );

        return JsonConfigurable::Configure( config );
    }

    bool ClimateFactory::Initialize(const ::Configuration* config, const string idreference)
    {
        LOG_DEBUG( "Initialize\n" );
        Configure( EnvPtr->Config );

        try
        {
            if(climate_structure == ClimateStructure::CLIMATE_OFF)
                return true;

            if((!GET_CONFIGURABLE(SimulationConfig)->demographics_initial) && climate_structure != ClimateStructure::CLIMATE_CONSTANT)
            {
                // ERROR: ("CLIMATE_STRUCTURE must be set to CLIMATE_CONSTANT (and associated constant climate parameters set) if running with DEMOGRAPHICS_INITIAL disabled\n");                
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Demographics_Builtin", !(GET_CONFIGURABLE(SimulationConfig)->demographics_initial), "Climate_Model", ClimateStructure::pairs::lookup_key(climate_structure));
            }

            // store start_time so we can initialize Climates when they're created
            start_time = GET_CONFIGURABLE(SimulationConfig)->starttime;

            // prepare any input files, etc

            switch( climate_structure )
            {
                case ClimateStructure::CLIMATE_CONSTANT:
                // nothing to do here...
                break;

                case ClimateStructure::CLIMATE_KOPPEN:
                {
                num_nodes = -1;

                if( climate_koppen_filename == "" )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_KOPPEN:", "climate_koppen_filename", "<empty>" );
                }
                std::string koppen_filepath = Environment::FindFileOnPath( climate_koppen_filename  );
                ParseMetadataForFile(koppen_filepath, idreference, nullptr, nullptr, &num_nodes, koppentype_offsets);

                if(!OpenClimateFile(koppen_filepath, num_nodes * sizeof(int), climate_koppentype_file))
                    return false;

                num_datavalues = 1;
                }
                break;

                case ClimateStructure::CLIMATE_BY_DATA:
                {
                // Parse metadata for all input files

                num_datavalues = -1;

                // num_nodes = -1;
                // We no longer require climate files to have identical structure nor are they
                // required to have unique entries for each node (i.e. multiple simulation nodes
                // may utilize the same data if, e.g., the simulation node size is smaller than the
                // resolution of the climate cell(s).
                int32_t num_airtemp_entries = -1;
                int32_t num_landtemp_entries = -1;
                int32_t num_rainfall_entries = -1;
                int32_t num_humidity_entries = -1;

                if( climate_airtemperature_filename == "" )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_BY_DATA:", "climate_airtemperature_filename", "<empty>" );
                }
                std::string airtemp_filepath = Environment::FindFileOnPath( climate_airtemperature_filename );
                ParseMetadataForFile(airtemp_filepath, idreference, &climate_update_resolution, &num_datavalues, &num_airtemp_entries, airtemperature_offsets);

                if( climate_landtemperature_filename == "" )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_BY_DATA:", "climate_landtemperature_filename", "<empty>" );
                }
                std::string landtemp_filepath = Environment::FindFileOnPath( climate_landtemperature_filename );
                ParseMetadataForFile(landtemp_filepath, idreference, &climate_update_resolution, &num_datavalues, &num_landtemp_entries, landtemperature_offsets);

                if( climate_rainfall_filename == "" )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_BY_DATA:", "climate_rainfall_filename", "<empty>" );
                }
                std::string rainfall_filepath = Environment::FindFileOnPath( climate_rainfall_filename );
                ParseMetadataForFile(rainfall_filepath, idreference, &climate_update_resolution, &num_datavalues, &num_rainfall_entries, rainfall_offsets);

                if( climate_relativehumidity_filename == "" )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_BY_DATA:", "climate_relativehumidity_filename", "<empty>" );
                }
                std::string humidity_filepath = Environment::FindFileOnPath( climate_relativehumidity_filename );
                ParseMetadataForFile(humidity_filepath, idreference, &climate_update_resolution, &num_datavalues, &num_humidity_entries, humidity_offsets);

                // open all input files

                if(!OpenClimateFile(airtemp_filepath, num_datavalues * num_airtemp_entries * sizeof(float), climate_airtemperature_file))
                    return false;
                if(!OpenClimateFile(landtemp_filepath, num_datavalues * num_landtemp_entries * sizeof(float), climate_landtemperature_file))
                    return false;
                if(!OpenClimateFile(rainfall_filepath, num_datavalues * num_rainfall_entries * sizeof(float), climate_rainfall_file))
                    return false;
                if(!OpenClimateFile(humidity_filepath, num_datavalues * num_humidity_entries * sizeof(float), climate_humidity_file))
                    return false;
                }
                break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", climate_structure, ClimateStructure::pairs::lookup_key( climate_structure ) );
            }
        }
        catch (Exception &e)
        {
            // ERROR: "Exception during climate initialization:\n");
            // ERROR: ("%s\n", e.what());
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
        }

        return true;
    }

    int ReadIntegerFromConfig( const json::QuickInterpreter& json, const char* key, const string& filename )
    {
        int value = 0;

        if ( json.Exist(key) )
        {
            try
            {
                value = int( json[key].As<json::Number>() );
            }
            catch (json::Exception&)
            {
                std::ostringstream msg;
                msg << "Value for key '" << key << "' in file '" << filename.c_str() << "' should be numeric.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
        else
        {
            std::ostringstream msg;
            msg << "Key \"" << key << "\" not found in file '" << filename.c_str() << "'" << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return value;
    }

    string ReadStringFromConfig( const json::QuickInterpreter& json, const char* key, const string& filename )
    {
        string value;

        if ( json.Exist(key) )
        {
            try
            {
                value = json[key].As<json::String>();
            }
            catch (json::Exception&)
            {
                std::ostringstream msg;
                msg << "Value for key '" << key << "' in file '" << filename.c_str() << "' should be a string.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
        else
        {
            std::ostringstream msg;
            msg << "Key \"" << key << "\" not found in file '" << filename.c_str() << "'" << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return value;
    }

    bool ClimateFactory::ParseMetadataForFile(
        string data_filepath,
        string idreference,
        ClimateUpdateResolution::Enum * const update_resolution,
        int * const pNumDatavalues,
        int * const pNumEntries,
        std::unordered_map<uint32_t, uint32_t> &node_offsets
    )
    {
        LOG_DEBUG_F( "%s: %s\n", __FUNCTION__, data_filepath.c_str() );
        release_assert(pNumEntries);

        string metadata_filepath = data_filepath + ".json";

        Configuration* config = Configuration::Load(metadata_filepath);

        if (config == nullptr)
        {
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, metadata_filepath.c_str() );
        }

        auto metadata = (*config)[METADATA];

        string json_id_reference( ReadStringFromConfig( metadata, ID_REFERENCE, metadata_filepath ) );
        string idreference_lower(idreference);  // Make a copy to transform so we do not modify the original.
        std::transform(idreference_lower.begin(), idreference_lower.end(), idreference_lower.begin(), ::tolower);
        std::transform(json_id_reference.begin(), json_id_reference.end(), json_id_reference.begin(), ::tolower);
        if (json_id_reference != idreference_lower)
        {
            std::ostringstream msg;
            msg << "IdReference used to generate climate file " << data_filepath << " doesn't match the IdReference used for the demographics" << std::endl;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if(update_resolution != nullptr)
        {
            string str_clim_res( ReadStringFromConfig( metadata, UPDATE_RESOLUTION, metadata_filepath ));
            int md_updateres = ClimateUpdateResolution::pairs::lookup_value(str_clim_res.c_str());

            if(md_updateres == -1 || (*update_resolution != ClimateUpdateResolution::Enum(md_updateres)))
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Update_Resolution", ClimateUpdateResolution::pairs::lookup_key(*update_resolution), (std::string("metadata from ") + metadata_filepath).c_str(), str_clim_res.c_str() );
            }
        }

        if(pNumDatavalues != nullptr)
        {
            int md_datavalues = ReadIntegerFromConfig( metadata, DATAVALUE_COUNT, metadata_filepath );

            if(*pNumDatavalues == -1)
                *pNumDatavalues = md_datavalues;
            else if(*pNumDatavalues != md_datavalues)
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "num_datavalues", *pNumDatavalues, "md_datavalues", md_datavalues );
            }
        }

        // Look for "Metadata" : { "WeatherSchemaVersion" } key. If present, use "WeatherCellCount" for md_num_entries
        // and "NumberDTKNodes" for md_num_offsets.

        int md_num_entries = -1;
        int md_num_offsets = -1;

        if ( (*config)[METADATA].Exist(SCHEMA_VERSION) )
        {
            std::string schema_version( ReadStringFromConfig( metadata, SCHEMA_VERSION, metadata_filepath ) );
            if ( schema_version == "2.0" )
            {
                LOG_DEBUG( "Found 'WeatherSchemaVersion' \"2.0\" in climate file metadata. Using 'WeatherCellCount' and 'NumberDTKNodes'\n" );
                md_num_entries = ReadIntegerFromConfig( metadata, CELL_COUNT, metadata_filepath );
                md_num_offsets = ReadIntegerFromConfig( metadata, DTK_NODE_COUNT, metadata_filepath );
            }
            else
            {
                // Could use GeneralConfigurationException here, perhaps.
                std::ostringstream msg;
                msg << "Unsupported 'WeatherSchemaVersion': " << schema_version;
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
        else // Fallback to "original" behavior.
        {
            LOG_DEBUG( "No 'WeatherSchemaVersion' found in climate file metadata. Falling back to 'NodeCount' and 'NumberDTKNodes'\n" );
            md_num_entries = ReadIntegerFromConfig( metadata, NODE_COUNT, metadata_filepath );
            md_num_offsets = md_num_entries;

            // "Slim" climate files map multiple DTK nodes to a single climate cell (node).
            // "NodeCount" gives the number of climate cells (nodes).
            // NumberDTKNodes (if present) gives the number of entries in the NodeOffsets string.
            // Could use try/catch here, but lacking the NumberDTKNodes (DTK_NODE_COUNT) key is allowed (i.e. not an exception).
            if ((*config)[METADATA].Exist(DTK_NODE_COUNT)) {
                md_num_offsets = ReadIntegerFromConfig( metadata, DTK_NODE_COUNT, metadata_filepath );
            }
        }

        if (*pNumEntries == -1)
        {
            *pNumEntries = md_num_entries;
        }

        string offsets_str = ReadStringFromConfig( *config, NODE_OFFSETS, metadata_filepath );
        if ( offsets_str.length() / 16 < md_num_offsets)
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "offsets_str.length() / 16", int( offsets_str.length() / 16 ), "*md_num_offsets", md_num_offsets);
        }

        uint32_t nodeid = 0, offset = 0;

        for(int n = 0; n < md_num_offsets; n++)
        {
#ifdef _MSC_VER
            sscanf_s(offsets_str.substr(n * 16, 8).c_str(), "%x", &nodeid);
            sscanf_s(offsets_str.substr((n * 16) + 8, 8).c_str(), "%x", &offset);
#else
            sscanf(offsets_str.substr(n * 16, 8).c_str(), "%x", &nodeid);
            sscanf(offsets_str.substr((n * 16) + 8, 8).c_str(), "%x", &offset);
#endif
            node_offsets[nodeid] = offset;
        }

        delete config;
        return true;
    }

    bool ClimateFactory::OpenClimateFile(string filepath, uint32_t expected_size, std::ifstream &file)
    {
        FileSystem::OpenFileForReading( file, filepath.c_str(), true );

        // "Slim" climate files point several nodes to the same data, thus the size may be less than
        // expected_size (generally num_datavalues * num_nodes * sizeof(float)).
        file.seekg(0, ios::end);
        int filelen = (int)file.tellg();

        if(filelen != expected_size)
        {
            ostringstream msg;
            msg << "Expected climate file '"
                << filepath.c_str()
                << "' to be "
                << expected_size << " bytes but is actually "
                << filelen << " bytes long.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        file.seekg(0, ios::beg);

        return true;
    }

    Climate* ClimateFactory::CreateClimate( INodeContext *parent_node, float altitude, float latitude, RANDOMBASE* pRNG )
    {
        LOG_DEBUG( "CreateClimate\n" );
        Climate* new_climate = nullptr;

        release_assert(parent_node);
        suids::suid node_suid = parent_node->GetSuid();

        release_assert(nodeid_suid_map);
        if(nodeid_suid_map->right.count(node_suid) == 0)
        {
            // ERROR: "Error: Couldn't find matching NodeID for suid " << node_suid.data << endl;
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "(nodeid_suid_map->right.count(node_suid)", 0, "node_suid", node_suid.data );
        }

        uint32_t nodeid = nodeid_suid_map->right.at(node_suid);
        LOG_DEBUG_F( "Processing nodeid %d\n", nodeid );

        switch( climate_structure )
        {
            case ClimateStructure::CLIMATE_CONSTANT:
                new_climate = ClimateConstant::CreateClimate( ClimateUpdateResolution::CLIMATE_UPDATE_DAY, parent_node, start_time, pRNG );
                break;

            case ClimateStructure::CLIMATE_KOPPEN:
            {
                if(koppentype_offsets.count(nodeid) == 0)
                {
                    //std::cerr << "Error: Couldn't find offset for NodeID " << nodeid << " in ClimateKoppen file" << endl;
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "(koppentype_offsets.count(node_suid)", 0, "node_suid", node_suid.data );
                }

                // seek to where we expect the data for that node
                climate_koppentype_file.seekg(koppentype_offsets[nodeid], std::ios::beg);

                // now read in koppen-type of climate
                int koppen_type;
                climate_koppentype_file.read((char *)(&koppen_type), sizeof(koppen_type));

                new_climate = ClimateKoppen::CreateClimate( ClimateUpdateResolution::CLIMATE_UPDATE_MONTH,
                                                            parent_node,
                                                            koppen_type,
                                                            altitude,
                                                            latitude,
                                                            start_time,
                                                            pRNG );
            }
            break;

            case ClimateStructure::CLIMATE_BY_DATA:
            {
                if(landtemperature_offsets.count(nodeid) == 0 ||
                    airtemperature_offsets.count(nodeid) == 0 ||
                    rainfall_offsets.count(nodeid) == 0 ||
                    humidity_offsets.count(nodeid) == 0)
                {
                    //std::cerr << "Error: Couldn't find offset for NodeID " << nodeid << " in ClimateByData files" << endl;
                    LOG_INFO_F( "landtemperature_offsets.count(nodeid) = %d, airtemperature_offsets.count(nodeid) = %d, rainfall_offsets.count(nodeid) = %d, humidity_offsets.count(nodeid) = %d\n",
                                landtemperature_offsets.count(nodeid), airtemperature_offsets.count(nodeid), rainfall_offsets.count(nodeid), humidity_offsets.count(nodeid)
                              );
                    ostringstream msg;
                    msg << "Didn't find data for demographics node "
                        << nodeid
                        << " ("
                        << std::hex << std::uppercase << std::setfill('0') << setw(8) << nodeid << std::dec
                        << ") in the following file(s):"
                        << endl;
                    if (airtemperature_offsets.count(nodeid) == 0)  { msg << "\tair temperature" << endl; }
                    if (landtemperature_offsets.count(nodeid) == 0) { msg << "\tland temperature" << endl; }
                    if (rainfall_offsets.count(nodeid) == 0)        { msg << "\trainfall" << endl; }
                    if (humidity_offsets.count(nodeid) == 0)        { msg << "\trelative humidity" << endl; }
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }

                climate_airtemperature_file.seekg(airtemperature_offsets[nodeid], std::ios::beg);
                climate_landtemperature_file.seekg(landtemperature_offsets[nodeid], std::ios::beg);
                climate_rainfall_file.seekg(rainfall_offsets[nodeid], std::ios::beg);
                climate_humidity_file.seekg(humidity_offsets[nodeid], std::ios::beg);

                new_climate = ClimateByData::CreateClimate( climate_update_resolution,
                                                            parent_node,
                                                            num_datavalues,
                                                            climate_airtemperature_file,
                                                            climate_landtemperature_file,
                                                            climate_rainfall_file,
                                                            climate_humidity_file,
                                                            start_time,
                                                            pRNG );
            }
            break;

            default:
            {
                // climate_structure not one of the recognized (non-off) types... why is this being called??
                //std::cerr << "Error: CreateClimate() was called for an invalid climate-structure: " << climate_structure << std::endl;
                //throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", climate_structure, ClimateStructure::pairs::lookup_key(climate_structure) );
            }
        }

        if(new_climate != nullptr && !new_climate->IsPlausible())
            num_badnodes++;

        return new_climate;
    }

    ClimateFactory::~ClimateFactory()
    {
        if(num_badnodes > 0)
            LOG_WARN_F("WARNING: Detected %d nodes with suspicious climate values\n", num_badnodes); 

        if (climate_airtemperature_file.is_open())
            climate_airtemperature_file.close();
        if (climate_landtemperature_file.is_open())
            climate_landtemperature_file.close();
        if (climate_rainfall_file.is_open())
            climate_rainfall_file.close();
        if (climate_humidity_file.is_open())
            climate_humidity_file.close();
        if (climate_koppentype_file.is_open())
            climate_koppentype_file.close();
    }


    Climate::~Climate() { }

    void
    Climate::SetContextTo(INodeContext* _parent) { parent = _parent; }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, Climate& climate, const unsigned int file_version)
    {
        ar & climate.m_airtemperature;
        ar & climate.m_landtemperature;
        ar & climate.m_accumulated_rainfall;
        ar & climate.m_humidity; 
        ar & climate.resolution_correction;
    }
}
#endif
