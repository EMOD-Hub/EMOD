
#pragma once
#include "Configure.h"
#include "EnumSupport.h"
#include <deque>
#include <vector>
#include <bitset>

using namespace Kernel;

ENUM_DEFINE( SerializationTypeRead,
    ENUM_VALUE_SPEC( NONE, 0 )
    ENUM_VALUE_SPEC( READ, 1 ) )

ENUM_DEFINE( SerializationTypeWrite,
    ENUM_VALUE_SPEC( NONE, 0 )
    ENUM_VALUE_SPEC( TIME, 1 )
    ENUM_VALUE_SPEC( TIMESTEP, 2 ) )

ENUM_DEFINE( SerializationPrecision,
    ENUM_VALUE_SPEC( REDUCED, 0 )
    ENUM_VALUE_SPEC( FULL, 1 ) )

typedef std::bitset<32> SerializationBitMask_t;


// Bit positions...
enum SerializationFlags : uint32_t
{
    Population = 0,
    Parameters = 1,
    Properties = 2,
    VectorPopulation = 3,
    LarvalHabitats = 4
};

class SerializationParameters : public JsonConfigurable
{
IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
DECLARE_QUERY_INTERFACE()
GET_SCHEMA_STATIC_WRAPPER( SerializationParameters )
public:
    static SerializationParameters* GetInstance();
    static void ResetInstance();
    std::deque<int32_t> GetSerializedTimeSteps( int32_t steps ) const;
    virtual bool Configure( const Configuration *config );    
    SerializationPrecision::Enum GetPrecision() const;
    void CheckConfiguration() const;

    SerializationTypeRead::Enum GetSerializedPopulationReadingType() const;
    SerializationBitMask_t GetSerializationReadMask() const;
    SerializationBitMask_t GetSerializationWriteMask() const;
    bool GetCreateRngFromSerializedData() const;
    std::string GetSerializedPopulationPath() const;
    std::string GetSerializedPopulationFilename() const;
    int32_t GetMaxHumansPerCollection() const;

private:
    SerializationParameters( const SerializationParameters& ) = delete;
    SerializationParameters();
    std::deque<int32_t> ProcessConfig( std::vector<int32_t> &specified_timesteps, float start_time, float step_size, int32_t steps ) const;
    void CheckSupportedFlags( SerializationBitMask_t  serialization_mask, std::string& parameter ) const;

    SerializationBitMask_t m_serializationReadMask;
    SerializationBitMask_t m_serializationWriteMask;
    bool m_create_rng_from_serialized_data;
    std::string m_serialized_population_path;
    vector<string> m_serialized_population_filenames;
    SerializationTypeWrite::Enum m_serialization_write_type;
    SerializationTypeRead::Enum m_serialization_read_type;
    SerializationPrecision::Enum m_serialization_precision;
    std::vector<int32_t> m_serialization_time_steps;
    std::vector<float> m_serialization_times;
    static SerializationParameters* p_instance;
    SerializationBitMask_t m_supportedFlags;
    int32_t m_MaxHumansPerCollection;
};
