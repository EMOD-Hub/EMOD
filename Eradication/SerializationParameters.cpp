
#include "stdafx.h"
#include "SerializationParameters.h"
#include "Log.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
#include "FileSystem.h"

using namespace Kernel;

SETUP_LOGGING( "SerializationParameters" )

GET_SCHEMA_STATIC_WRAPPER_IMPL(SerializationParameters, SerializationParameters)
BEGIN_QUERY_INTERFACE_BODY(SerializationParameters)
END_QUERY_INTERFACE_BODY(SerializationParameters)

SerializationParameters* SerializationParameters::p_instance = nullptr;


SerializationParameters::SerializationParameters()
    : m_serializationReadMask( 0 )
    , m_serializationWriteMask( 0 )
    , m_create_rng_from_serialized_data( false )
    , m_serialized_population_path()
    , m_serialized_population_filenames()
    , m_serialization_write_type( SerializationTypeWrite::NONE )
    , m_serialization_read_type( SerializationTypeRead::NONE )
    , m_serialization_precision( SerializationPrecision::REDUCED )
    , m_serialization_time_steps()
    , m_serialization_times()
    , m_supportedFlags( SerializationBitMask_t{}.set( SerializationFlags::LarvalHabitats ) )
{ }


SerializationTypeRead::Enum SerializationParameters::GetSerializedPopulationReadingType() const
{
    return this->m_serialization_read_type;
}


SerializationBitMask_t SerializationParameters::GetSerializationReadMask() const
{
    return m_serializationReadMask;
}

SerializationBitMask_t SerializationParameters::GetSerializationWriteMask() const
{
    return m_serializationWriteMask;
}

bool SerializationParameters::GetCreateRngFromSerializedData() const
{
    return m_create_rng_from_serialized_data;
}

std::string SerializationParameters::GetSerializedPopulationPath() const
{
    return m_serialized_population_path;
}

std::string SerializationParameters::GetSerializedPopulationFilename() const
{
    release_assert( EnvPtr->MPI.Rank < m_serialized_population_filenames.size() );
    const std::string population_filename = FileSystem::Concat( m_serialized_population_path, m_serialized_population_filenames[EnvPtr->MPI.Rank] );
    return population_filename;
}

std::deque<int32_t> SerializationParameters::GetSerializedTimeSteps( int32_t steps ) const
{
    float start_time = GET_CONFIGURABLE( SimulationConfig )->starttime;
    float step_size = GET_CONFIGURABLE( SimulationConfig )->Sim_Tstep;

    std::vector<int32_t> timesteps;
    switch( m_serialization_write_type )
    {
    case SerializationTypeWrite::NONE:
        // Bail early if there's no serialization specified
        return std::deque<int32_t>();

    case SerializationTypeWrite::TIME:
        // If using times, transform times into timesteps
        for( auto time : m_serialization_times )
        {
            // Special case: don't convert -1: it will become the last timestep
            if( time == -1.0f )
            {
                timesteps.push_back( -1 );
            }
            // Special case: don't convert 0: it will become the first timestep
            else if( time == 0.0f )
            {
                timesteps.push_back( 0 );
            }
            else
            {
                int32_t transformed_time = (int32_t)std::ceil( ( time - start_time ) / step_size );
                timesteps.push_back( transformed_time );
            }
        }
        break;

    case SerializationTypeWrite::TIMESTEP:
        // If using timesteps, use them directly
        timesteps = m_serialization_time_steps;
        break;

    default:
        // If other enum value specified, throw exception
        throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "m_serialization_write_type", m_serialization_write_type, "SerializationTypeRead" );
    }

    return ProcessConfig( timesteps, start_time, step_size, steps );
}

std::deque<int32_t> SerializationParameters::ProcessConfig( std::vector<int32_t> &specified_timesteps, float start_time, float step_size, int32_t steps ) const
{
    // Replace special values in-place: -1 for last step
    std::replace( specified_timesteps.begin(), specified_timesteps.end(), -1, steps );

    // Sort
    std::sort( specified_timesteps.begin(), specified_timesteps.end() );

    // Warn and eliminate duplicates
    std::set<int32_t> times_set( specified_timesteps.begin(), specified_timesteps.end() );
    std::list<int32_t> duplicate_times;
    std::set_difference( specified_timesteps.begin(), specified_timesteps.end(),
        times_set.begin(), times_set.end(),
        back_inserter( duplicate_times ) );
    for ( int32_t dupe : duplicate_times )
    {
        LOG_WARN_F( "Duplicate serialization step specified (step=%d, time=%f), removed from list\n", dupe, dupe*step_size+start_time );
    }
    specified_timesteps = std::vector<int32_t>( times_set.begin(), times_set.end() );

    // Warn and eliminate any too early or too late times
    std::vector<int32_t> bad_times;
    std::copy_if( specified_timesteps.begin(), specified_timesteps.end(), back_inserter( bad_times ),
        [&steps](int32_t i) { return i < 0 || i > steps; } );
    for( int32_t bad : bad_times )
    {
        LOG_WARN_F( "Out of range serialization step specified (step=%d, time=%f), removed from list\n", bad, bad*step_size+start_time );
        specified_timesteps.erase( std::remove(specified_timesteps.begin(), specified_timesteps.end(), bad), specified_timesteps.end() );
    }
        
    // Return as a deque
    return std::deque<int32_t>( specified_timesteps.begin(), specified_timesteps.end() );
}

void SerializationParameters::CheckSupportedFlags( SerializationBitMask_t serialization_mask, std::string& parameter ) const
{
    const SerializationBitMask_t unsupported_flags = serialization_mask & ~m_supportedFlags;

    if( unsupported_flags != 0 )
    {
        std::stringstream ss;
        ss << "Parameter '" << parameter << "' with value '" << ( uint32_t )serialization_mask.to_ulong()<<"' is not supported. Currently only '"<< m_supportedFlags.to_ulong() << "' and '0' are supported.";
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }
}

SerializationPrecision::Enum SerializationParameters::GetPrecision() const
{
    return m_serialization_precision;
}

void SerializationParameters::CheckConfiguration() const
{
    if( m_serialization_write_type != SerializationTypeWrite::NONE )
    {
        switch( m_serialization_write_type )
        {
            case SerializationTypeWrite::TIME:
                if( m_serialization_times.empty() )
                {
                    std::stringstream ss;
                    ss << "No time in 'Serialization_Times'.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                break;

            case SerializationTypeWrite::TIMESTEP:
                if( m_serialization_time_steps.empty() )
                {
                    std::stringstream ss;
                    ss << "No timestep in 'Serialization_Time_Steps'.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "m_serialization_write_type", m_serialization_write_type, "SerializationTypeRead" );
        }

        std::string parameter( "Serialization_Mask_Node_Write" );
        CheckSupportedFlags( m_serializationWriteMask, parameter );
    }

    if( m_serialization_read_type != SerializationTypeRead::NONE )
    {
        if( m_serialized_population_filenames.size() != EnvPtr->MPI.NumTasks )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "MPI.NumTasks", float( EnvPtr->MPI.NumTasks ), "filenames.size()", float( m_serialized_population_filenames.size() ), "Number of files listed in 'Serialized_Population_Filenames' doesn't match number of MPI tasks." );
        }

        for( const string& file: m_serialized_population_filenames )
        {
            const std::string population_filename = FileSystem::Concat( m_serialized_population_path, file );
            if( !FileSystem::FileExists( population_filename ) )
            {
                std::stringstream ss;
                ss << "Serialized population file "<< file << " defined in 'Serialized_Population_Filenames' does not exist ";
                ss << "in 'Serialized_Population_Path' = '" << m_serialized_population_path << "'.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,  ss.str().c_str());
            }
        }

        std::string parameter( "Serialization_Mask_Node_Read" );
        CheckSupportedFlags( m_serializationReadMask, parameter );
    }
}

bool SerializationParameters::Configure( const Configuration * inputJson )
{
    // Write serialized population
    initConfig( "Serialized_Population_Writing_Type", m_serialization_write_type, inputJson, MetadataDescriptor::Enum("Serialized_Population_Writing_Type", Serialized_Population_Writing_Type_DESC_TEXT, MDD_ENUM_ARGS(SerializationTypeWrite)) );
    initConfigTypeMap( "Serialization_Time_Steps", &m_serialization_time_steps, Serialization_Time_Steps_DESC_TEXT, -1, INT_MAX, false, "Serialized_Population_Writing_Type", "TIMESTEP" );
    initConfigTypeMap( "Serialization_Times", &m_serialization_times, Serialization_Times_DESC_TEXT, -1, FLT_MAX, false, "Serialized_Population_Writing_Type", "TIME" );
    initConfig( "Serialization_Precision", m_serialization_precision, inputJson, MetadataDescriptor::Enum( "Serialization_Precision", Serialization_Precision_DESC_TEXT, MDD_ENUM_ARGS( SerializationPrecision ) ), "Serialized_Population_Writing_Type", "TIMESTEP,TIME" );
    initConfigTypeMap( "Serialization_Mask_Node_Write", ( uint32_t* )&m_serializationWriteMask, SerializationMask_Node_Write_DESC_TEXT, 0, UINT32_MAX, 0, "Serialized_Population_Writing_Type","TIME,TIMESTEP" );

    // Read serialized population
    initConfig( "Serialized_Population_Reading_Type", m_serialization_read_type, inputJson, MetadataDescriptor::Enum( "Serialized_Population_Reading_Type", Serialized_Population_Reading_Type_DESC_TEXT, MDD_ENUM_ARGS( SerializationTypeRead ) ) );
    initConfigTypeMap( "Serialization_Mask_Node_Read", ( uint32_t* )&m_serializationReadMask, SerializationMask_Node_Read_DESC_TEXT, 0, UINT32_MAX, 0, "Serialized_Population_Reading_Type", "READ" );
    initConfigTypeMap( "Serialized_Population_Path", &m_serialized_population_path, Serialized_Population_Path_DESC_TEXT, ".", "Serialized_Population_Reading_Type", "READ" );
    initConfigTypeMap( "Serialized_Population_Filenames", &m_serialized_population_filenames, Serialized_Population_Filenames_DESC_TEXT, nullptr, empty_set, "Serialized_Population_Reading_Type", "READ" );
    initConfigTypeMap( "Enable_Random_Generator_From_Serialized_Population", &m_create_rng_from_serialized_data, Enable_Random_Generator_From_Serialized_Population_DESC_TEXT, false, "Serialized_Population_Reading_Type", "READ" );
    
    bool retValue =  JsonConfigurable::Configure( inputJson );
    if( retValue && !JsonConfigurable::_dryrun )
    {
        CheckConfiguration();
    }
    return retValue;
}

SerializationParameters* SerializationParameters::GetInstance()
{
    if( p_instance == nullptr )
    {
        p_instance = new SerializationParameters();
    }
    return p_instance; 
}

void SerializationParameters::ResetInstance()
{
    delete p_instance;
    p_instance = nullptr;
}
