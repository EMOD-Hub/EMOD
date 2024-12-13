
#include "stdafx.h"
#include "Migration.h"
#include "FileSystem.h"
#include "JsonObjectDemog.h"
#include "NoCrtWarnings.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "IdmString.h"
#include "RANDOM.h"
#include "INodeContext.h"
#include "config_params.rc"

#define UNINITIALIZED_STRING "UNINITIALIZED STRING"

SETUP_LOGGING( "Migration" )

#define MAX_DESTINATIONS (100) // maximum number of destinations per node in migration file

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- MigrationRateData
    // ------------------------------------------------------------------------

    MigrationRateData::MigrationRateData()
        : m_ToNodeSuid()
        , m_MigType( MigrationType::LOCAL_MIGRATION )
        , m_InterpMap()
    {
    }

    MigrationRateData::MigrationRateData( suids::suid to_node_id, MigrationType::Enum migType, InterpolationType::Enum interpType )
        : m_ToNodeSuid( to_node_id )
        , m_MigType( migType )
        , m_InterpType( interpType )
        , m_InterpMap()
    {
    }

    const suids::suid MigrationRateData::GetToNodeSuid() const
    {
        return m_ToNodeSuid;
    }

    MigrationType::Enum MigrationRateData::GetMigrationType() const
    {
        return m_MigType;
    }

    InterpolationType::Enum MigrationRateData::GetInterpolationType() const
    {
        return m_InterpType;
    }

    int MigrationRateData::GetNumRates() const
    {
        return m_InterpMap.size();
    }

    void MigrationRateData::AddRate( float ageYears, float rate )
    {
        m_InterpMap.add( ageYears, rate );
    }

    float MigrationRateData::GetRate( float ageYears ) const
    {
        // m_InterpMap is a std::map that is sorted on the first key (i.e. ageYears)
        float first_age_years = m_InterpMap.begin()->first;
        float first_rate      = m_InterpMap.begin()->second;
        float last_age_years  = m_InterpMap.rbegin()->first;
        float last_rate       = m_InterpMap.rbegin()->second;

        // ------------------------------------------------------------------------------------
        // --- Ages that are outside the range given in the metadata file are capped
        // --- at either the first or last value regardless if it is interpolation or piecewise
        // ------------------------------------------------------------------------------------
        float rate = 0; 
        if( ageYears <= first_age_years )
        {
            rate = first_rate;
        }
        else if( ageYears >= last_age_years )
        {
            rate = last_rate;
        }
        else if( m_InterpType == InterpolationType::LINEAR_INTERPOLATION )
        {
            rate = m_InterpMap.getValueLinearInterpolation( ageYears, first_rate );
        }
        else
        {
            rate = m_InterpMap.getValuePiecewiseConstant( ageYears, first_rate );
        }
        return rate;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoNull
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY(MigrationInfoNull)
    END_QUERY_INTERFACE_BODY(MigrationInfoNull)

    MigrationInfoNull::MigrationInfoNull()
    : m_EmptyListCDF()
    , m_EmptyListNodes()
    , m_EmptyListTypes()
    {
    }

    MigrationInfoNull::~MigrationInfoNull()
    {
    }

    void MigrationInfoNull::PickMigrationStep( RANDOMBASE* pRNG, 
                                               IIndividualHumanEventContext * traveler, 
                                               float migration_rate_modifier, 
                                               suids::suid &destination, 
                                               MigrationType::Enum &migration_type,
                                               float &time,
                                               float dt )
    {
        destination = suids::nil_suid();
        migration_type = MigrationType::NO_MIGRATION;
        time = -1.0;
    }

    void MigrationInfoNull::SetContextTo(INodeContext* _parent)
    {
    }

    float MigrationInfoNull::GetTotalRate( Gender::Enum gender ) const
    {
        return 0.0;
    }

    const std::vector<float>& MigrationInfoNull::GetCumulativeDistributionFunction( Gender::Enum gender ) const
    {
        return m_EmptyListCDF;
    }

    const std::vector<suids::suid>& MigrationInfoNull::GetReachableNodes( Gender::Enum gender ) const
    {
        return m_EmptyListNodes;
    }

    const std::vector<MigrationType::Enum>& MigrationInfoNull::GetMigrationTypes(Gender::Enum gender) const
    {
        return m_EmptyListTypes;
    }

    bool MigrationInfoNull::IsHeterogeneityEnabled() const
    {
        return false;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFixedRate
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(MigrationInfoFixedRate)
    END_QUERY_INTERFACE_BODY(MigrationInfoFixedRate)

    MigrationInfoFixedRate::MigrationInfoFixedRate( INodeContext * _parent, 
                                                    bool isHeterogeneityEnabled ) 
        : m_Parent(_parent) 
        , m_IsHeterogeneityEnabled( isHeterogeneityEnabled )
        , m_ReachableNodes()
        , m_MigrationTypes()
        , m_RateCDF()
        , m_TotalRate(0.0)
    {
    }

    MigrationInfoFixedRate::~MigrationInfoFixedRate() 
    {
    }

    void MigrationInfoFixedRate::Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData )
    {
        m_RateCDF.clear();
        m_TotalRate = 0.0;
        if( rRateData.size() > 0 )
        {
            for( auto& mrd : rRateData[0] )
            {
                m_MigrationTypes.push_back( mrd.GetMigrationType() );
                m_ReachableNodes.push_back( mrd.GetToNodeSuid()    );

                release_assert( mrd.GetNumRates() == 1 );

                m_RateCDF.push_back( mrd.GetRate( 0.0 ) );
                m_TotalRate += mrd.GetRate( 0.0 );
            }
            NormalizeRates( m_RateCDF, m_TotalRate );
        }
    }

    void MigrationInfoFixedRate::SetContextTo(INodeContext* _parent)
    { 
        m_Parent = _parent; 
    }

    const std::vector<suids::suid>& MigrationInfoFixedRate::GetReachableNodes( Gender::Enum gender ) const
    {
        return m_ReachableNodes;
    }

    const std::vector<MigrationType::Enum>& MigrationInfoFixedRate::GetMigrationTypes( Gender::Enum gender ) const
    { 
        return m_MigrationTypes;
    }

    bool MigrationInfoFixedRate::IsHeterogeneityEnabled() const
    { 
        return m_IsHeterogeneityEnabled; 
    }

    void MigrationInfoFixedRate::PickMigrationStep( RANDOMBASE* pRNG,
                                                    IIndividualHumanEventContext *traveler, 
                                                    float migration_rate_modifier,
                                                    suids::suid &destination, 
                                                    MigrationType::Enum &migration_type, 
                                                    float &time, 
                                                    float dt )
    {
        destination    = suids::nil_suid();
        migration_type = MigrationType::NO_MIGRATION;
        time           = 0.0;

        float        age_years = 0.0;
        Gender::Enum gender    = Gender::MALE;
        if( traveler != nullptr )
        {
            age_years = traveler->GetAge() / DAYSPERYEAR;
            gender = Gender::Enum( traveler->GetGender() );
        }

        CalculateRates( gender, age_years );

        float                                   total_rate        = GetTotalRate( gender );
        const std::vector<float              >& r_cdf             = GetCumulativeDistributionFunction( gender );
        const std::vector<suids::suid        >& r_reachable_nodes = GetReachableNodes( gender );
        const std::vector<MigrationType::Enum>& r_migration_types = GetMigrationTypes( gender );

        if( (r_cdf.size() == 0) || (total_rate == 0.0) )
        {
            return;
        }

        time = static_cast<float>(pRNG->expdist( migration_rate_modifier * total_rate ));
        if( time > dt ) // dt for humans always FLT_MAX; this check is for vector migration 
        {
            return;
        }

        int index = 0;
        float desttemp = pRNG->e();
        while( (desttemp > r_cdf[ index ]) && ((index+1) < r_cdf.size()) )
        {
            index++;
        }

        destination    = r_reachable_nodes[index];
        migration_type = r_migration_types[index];
    }

    void MigrationInfoFixedRate::NormalizeRates( std::vector<float>& r_rate_cdf, float& r_total_rate )
    {
        if( (r_rate_cdf.size() > 0) && (r_total_rate > 0.0) )
        {
            //  Set probability of each location
            // Values in the migration_rate_cdf[] are compared against values generated by RANDOMBASE::e(), which 
            // is guaranteed to create values between 0.0f and 1.0f.  We need to explicitly set values that are 
            // floating point rounding error close to 1 to 1, otherwise floating-point rounding errors might result 
            // in running past the end of valid values in the array when picking a migration destination
            r_rate_cdf[0] /= r_total_rate;
            for(int i = 1; i < r_rate_cdf.size(); i++)
            {
                float tmp_rate = ( r_rate_cdf[i] / r_total_rate ) + r_rate_cdf[i - 1];
                if( 1 - tmp_rate < FLT_EPSILON ) // if close to 1, call it 1
                {
                    r_rate_cdf[i] = 1;
                }
                else
                {
                    r_rate_cdf[i] = tmp_rate;
                }
            }
        }
    }

    void MigrationInfoFixedRate::CalculateRates( Gender::Enum gender, float ageYears ) 
    {
        // rates are fixed so do nothing
    }

    const std::vector<float>& MigrationInfoFixedRate::GetCumulativeDistributionFunction(Gender::Enum gender) const
    {
        return m_RateCDF;
    }

    float MigrationInfoFixedRate::GetTotalRate( Gender::Enum gender ) const
    {
        return m_TotalRate;
    }


    // ------------------------------------------------------------------------
    // --- MigrationInfoAgeAndGender
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoAgeAndGender, MigrationInfoFixedRate)
    END_QUERY_INTERFACE_DERIVED(MigrationInfoAgeAndGender, MigrationInfoFixedRate)

    MigrationInfoAgeAndGender::MigrationInfoAgeAndGender( INodeContext * _parent, 
                                                          bool isHeterogeneityEnabled ) 
        : MigrationInfoFixedRate( _parent, isHeterogeneityEnabled ) 
        , m_RateData()
        , m_ReachableNodesFemale()
        , m_MigrationTypesFemale()
    {
    }

    MigrationInfoAgeAndGender::~MigrationInfoAgeAndGender() 
    {
    }

    void MigrationInfoAgeAndGender::Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData )
    {
        release_assert( rRateData.size() == 2 );

        for( auto& mrd : rRateData[0] )
        {
            m_MigrationTypes.push_back( mrd.GetMigrationType() );
            m_ReachableNodes.push_back( mrd.GetToNodeSuid()    );
        }

        for( auto& mrd : rRateData[1] )
        {
            m_MigrationTypesFemale.push_back( mrd.GetMigrationType() );
            m_ReachableNodesFemale.push_back( mrd.GetToNodeSuid()    );
        }

        m_RateData = rRateData;
    }

    void MigrationInfoAgeAndGender::CalculateRates( Gender::Enum gender, float ageYears ) 
    {
        m_RateCDF.clear();
        m_TotalRate = 0.0;

        int gender_index = int(gender);

        for( auto& mrd : m_RateData[ gender_index ] )
        {
            float rate = mrd.GetRate( ageYears );
            m_RateCDF.push_back( rate );
            m_TotalRate += rate;
        }

        SaveRawRates(m_RateCDF, gender);
        NormalizeRates( m_RateCDF, m_TotalRate );
    }

    const std::vector<suids::suid>& MigrationInfoAgeAndGender::GetReachableNodes( Gender::Enum gender ) const
    {
        if( gender == Gender::FEMALE )
        {
            return m_ReachableNodesFemale;
        }
        else
        {
            return m_ReachableNodes;
        }
    }

    const std::vector<MigrationType::Enum>& MigrationInfoAgeAndGender::GetMigrationTypes( Gender::Enum gender ) const
    {
        if( gender == Gender::FEMALE )
        {
            return m_MigrationTypesFemale;
        }
        else
        {
            return m_MigrationTypes;
        }
    }

    // ------------------------------------------------------------------------
    // --- MigrationMetadata
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY(MigrationMetadata)
    END_QUERY_INTERFACE_BODY(MigrationMetadata)

    MigrationMetadata::MigrationMetadata( MigrationType::Enum expectedMigType,
                                          int defaultDestinationsPerNode )
        : m_ExpectedIdReference()
        , m_ExpectedMigrationType( expectedMigType )
        , m_DestinationsPerNode( defaultDestinationsPerNode )
        , m_NumNodes( 0 )
        , m_GenderDataType( GenderDataType::SAME_FOR_BOTH_GENDERS )
        , m_InterpolationType( InterpolationType::LINEAR_INTERPOLATION )
        , m_AgesYears()
        , m_GenderDataSize( 0 )
        , m_AgeDataSize( 0 )
    {
    }

    MigrationMetadata::~MigrationMetadata()
    {
    }

    bool MigrationMetadata::Configure( const Configuration* config )
    {
        MigrationType::Enum file_migration_type = MigrationType::NO_MIGRATION; // default because needs to default to something coherent
        ConfigMigrationType( config, file_migration_type );
        ConfigInterpolationType( config );
        ConfigDatavalueCount( config );
        initConfig( "GenderDataType", m_GenderDataType, config, MetadataDescriptor::Enum( "GenderDataType", MigrationMetadata_GenderDataType_DESC_TEXT, MDD_ENUM_ARGS( GenderDataType ) ) );

        std::string file_id_reference;
        initConfigTypeMap( "IdReference", &file_id_reference, MigrationMetadata_IdReference_DESC_TEXT  );
        initConfigTypeMap( "NodeCount",   &m_NumNodes,        MigrationMetadata_NodeCount_DESC_TEXT, 1,   INT_MAX,       1 );
        initConfigTypeMap( "AgesYears",   &m_AgesYears,       MigrationMetadata_AgesYears_DESC_TEXT, 0.0, MAX_HUMAN_AGE, true );

        bool is_configured = JsonConfigurable::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            // -------------------------------------
            // --- Check idreference - Must be equal
            // -------------------------------------
            if( file_id_reference != m_ExpectedIdReference )
            {
                std::stringstream ss;
                ss << config->GetDataLocation() << "[Metadata][IdReference]";
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Expected IdReference", m_ExpectedIdReference.c_str(),
                                                        ss.str().c_str(), file_id_reference.c_str());
            }

            // ----------------------------------------------------------
            // --- MigrationType - Verify that it is what we expect
            // ----------------------------------------------------------
            CheckMigrationType( config, file_migration_type );

            // ----------------------------------------------
            // --- GenderDataType - must be human stuff here
            // ----------------------------------------------
            CheckGenderDataType( config );

            // -----------------------------------------------
            // --- AgeYears - Make sure has at least one value
            // -----------------------------------------------
            if( m_AgesYears.size() == 0 )
            {
                // Look up logic depends on the default value being the max age
                m_AgesYears.push_back( MAX_HUMAN_AGE );
            }

            // -----------------------------------------------------------------
            // --- NodeCount - Verify that it was defined and greater than zero
            // -----------------------------------------------------------------
            if( !config->Exist( "NodeCount" ) )
            {
                std::stringstream ss;
                ss << config->GetDataLocation() << "[Metadata][NodeCount] must be defined.\n";
                ss << "It must indicate the number of 'from' nodes in the data.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );

            }
            else if( m_NumNodes <= 0 )
            {
                std::stringstream ss;
                ss << config->GetDataLocation() << "[Metadata][NodeCount] cannot be zero.\n";
                ss << "It must indicate the number of 'from' nodes in the data.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return is_configured;
    }

    void MigrationMetadata::ConfigMigrationType( const Configuration* config, MigrationType::Enum & file_migration_type )
    {
       bool migration_type_found = initConfig( "MigrationType", file_migration_type, config, MetadataDescriptor::Enum( "MigrationType", MigrationMetadata_MigrationType_DESC_TEXT, MDD_ENUM_ARGS( MigrationType ) ) );
       if( !migration_type_found ) // if parameter not found, use the expected migration
       {
           file_migration_type = m_ExpectedMigrationType;
       }
    }

    void MigrationMetadata::ConfigInterpolationType( const Configuration* config )
    {
        initConfig( "InterpolationType", m_InterpolationType, config, MetadataDescriptor::Enum( "InterpolationType", MigrationMetadata_InterpolationType_DESC_TEXT, MDD_ENUM_ARGS( InterpolationType ) ) );
    }

    void MigrationMetadata::ConfigDatavalueCount( const Configuration* config )
    {
        initConfigTypeMap( "DatavalueCount", &m_DestinationsPerNode, MigrationMetadata_DatavalueCount_DESC_TEXT, 1, MAX_DESTINATIONS, 8 );
    }

    void MigrationMetadata::SetExpectedIdReference( const std::string& rExpectedIdReference )
    {
        m_ExpectedIdReference = rExpectedIdReference;
    }

    void MigrationMetadata::CheckGenderDataType( const Configuration* config )
    {
        if( m_GenderDataType == GenderDataType::VECTOR_MIGRATION_BY_GENETICS )
        {
            std::stringstream ss;
            ss << config->GetDataLocation() << "[Metadata][GenderDataType] cannot be 'VECTOR_MIGRATION_BY_GENETICS' for humans.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void MigrationMetadata::CheckMigrationType( const Configuration* config, const MigrationType::Enum file_migration_type )
    {
        if( file_migration_type != m_ExpectedMigrationType )
        {
            std::string exp_mig_type_str = MigrationType::pairs::lookup_key( m_ExpectedMigrationType );
            std::string file_mig_type_str = MigrationType::pairs::lookup_key( file_migration_type );
            std::stringstream ss;
            ss << config->GetDataLocation() << "[Metadata][MigrationType]";
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                "Expected MigrationType", exp_mig_type_str.c_str(),
                ss.str().c_str(), file_mig_type_str.c_str() );
        }
    }

    uint32_t MigrationMetadata::GetNumGenderDataChunks() const
    {
        uint32_t num_gender_data_chunks= 1;
        if( m_GenderDataType == GenderDataType::ONE_FOR_EACH_GENDER )
        {
            num_gender_data_chunks = 2; // the first chuck is for males, the second is for females
        }
        else
        {
            num_gender_data_chunks = 1; // genders use same data, including VECTOR_MIGRATION_BY_GENETICS
        }
        return num_gender_data_chunks;
    }

    uint32_t MigrationMetadata::CalculateExpectedBinaryFileSize()
    {
        uint32_t num_gender_data_chunks = GetNumGenderDataChunks();
        uint32_t num_age_data_chunks    = m_AgesYears.size();

        m_AgeDataSize    = m_NumNodes * m_DestinationsPerNode * (sizeof(uint32_t) + sizeof(double));
        m_GenderDataSize = num_age_data_chunks * m_AgeDataSize;

        uint32_t exp_binary_file_size = num_gender_data_chunks * m_GenderDataSize;

        return exp_binary_file_size;
    }

    bool MigrationMetadata::IsFixedRate() const
    {
        uint32_t num_gender_data_chunks = GetNumGenderDataChunks();

        // data is fixed if one gender and one age and age is max
        bool is_fixed_rate = (num_gender_data_chunks == 1) &&
                             (m_AgesYears.size()     == 1) &&
                             (m_AgesYears[ 0 ]       == MAX_HUMAN_AGE);
        return is_fixed_rate;
    }

    // ------------------------------------------------------------------------
    // --- MigrationOffsetsData
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY(MigrationOffsetsData)
    END_QUERY_INTERFACE_BODY(MigrationOffsetsData)

    MigrationOffsetsData::MigrationOffsetsData( MigrationType::Enum expectedMigType,
                                                int defaultDestinationsPerNode )
        : m_pMetadata( new MigrationMetadata( expectedMigType, defaultDestinationsPerNode ) )
        , m_Offsets()
    {
    }

    MigrationOffsetsData::MigrationOffsetsData( MigrationMetadata* pMetadata )
        : m_pMetadata( pMetadata )
        , m_Offsets()
    {
    }

    MigrationOffsetsData::~MigrationOffsetsData()
    {
        delete m_pMetadata;
    }

    bool MigrationOffsetsData::Configure( const Configuration* config )
    {
        std::string offsets_str;
        initConfigTypeMap( "NodeOffsets", &offsets_str, MigrationOffsetsData_NodeOffsets_DESC_TEXT );
        initConfigTypeMap( "Metadata", m_pMetadata, MigrationOffsetsData_Metadata_DESC_TEXT );

        bool is_configured = JsonConfigurable::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            // ------------------------------------------------------------------------
            // --- Verify NodeOffests is the right length given the number of nodes.
            // --- There should be 16 characters for each node.  The first 8 characters
            // --- are for the External Node ID (i.e the node id used in the demographics)
            // --- and the second 8 characters are the offset in the file.
            // --- The node ids in the offset_str are the "from" nodes - the nodes that
            // --- individuals are trying migrate from.
            // ------------------------------------------------------------------------
            if( offsets_str.length() / 16 != m_pMetadata->GetNumNodes() )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "offsets_str.length() / 16", int( offsets_str.length() / 16 ),
                                                        "num_nodes", m_pMetadata->GetNumNodes() );
            }

            // -------------------------------------------
            // --- Calculate the expected binary file size
            // -------------------------------------------
            uint32_t exp_binary_file_size = m_pMetadata->CalculateExpectedBinaryFileSize();

            // -----------------------------------------------
            // --- Extract data from string and place into map
            // -----------------------------------------------
            for( int n = 0; n < m_pMetadata->GetNumNodes(); n++ )
            {
                ExternalNodeId_t nodeid = 0;
                uint32_t offset = 0;

                sscanf_s( offsets_str.substr( (n * 16), 8 ).c_str(), "%x", &nodeid );
                sscanf_s( offsets_str.substr( (n * 16) + 8, 8 ).c_str(), "%x", &offset );

                if( offset >= exp_binary_file_size )
                {
                    char offset_buff[ 20 ];
                    sprintf_s( offset_buff, 19, "0x%x", offset );
                    char filesize_buff[ 20 ];
                    sprintf_s( filesize_buff, 19, "0x%x", exp_binary_file_size );
                    std::stringstream ss;
                    ss << std::endl;
                    ss << "Invalid 'NodeOffsets' in " << config->GetDataLocation() << "." << std::endl;
                    ss << "Node ID=" << nodeid << " has an offset of " << offset_buff;
                    ss << " but the '.bin' file size is expected to be " << exp_binary_file_size << "(" << filesize_buff << ")." << std::endl;
                    throw FileIOException( __FILE__, __LINE__, __FUNCTION__, config->GetDataLocation().c_str(), ss.str().c_str() );
                }

                m_Offsets[ nodeid ] = offset;
            }
        }
        return is_configured;
    }

    void MigrationOffsetsData::SetExpectedIdReference( const std::string& rExpectedIdReference )
    {
        m_pMetadata->SetExpectedIdReference( rExpectedIdReference );
    }

    const std::vector<float>& MigrationOffsetsData::GetAgesYears() const
    {
        return m_pMetadata->GetAgesYears();
    }

    int MigrationOffsetsData::GetDestinationsPerNode() const
    {
        return m_pMetadata->GetDestinationsPerNode();
    }

    MigrationType::Enum MigrationOffsetsData::GetMigrationType() const
    {
        return m_pMetadata->GetMigrationType();
    }

    InterpolationType::Enum MigrationOffsetsData::GetInterpolationType() const
    {
        return m_pMetadata->GetInterpolationType();
    }

    uint32_t MigrationOffsetsData::CalculateExpectedBinaryFileSize()
    {
        return m_pMetadata->CalculateExpectedBinaryFileSize();
    }

    bool MigrationOffsetsData::IsFixedRate() const
    {
        return m_pMetadata->IsFixedRate();
    }

    bool MigrationOffsetsData::HasDataFor( uint32_t nodeID ) const
    {
        return (m_Offsets.count( nodeID ) > 0);
    }

    std::streamoff MigrationOffsetsData::GetOffset( int indexGender, int indexAge, uint32_t nodeID ) const
    {
        if( (m_pMetadata->GetGenderDataType() != GenderDataType::ONE_FOR_EACH_GENDER) && (indexGender == 1) )
        {
            // -----------------------------------------------------
            // --- The data has one set of rates for both genders 
            // --- so read the same data into the female parameters.
            // -----------------------------------------------------
            indexGender = 0;
        }

        uint32_t gender_size = m_pMetadata->GetGenderDataSize();
        uint32_t age_size    = m_pMetadata->GetAgeDataSize();

        std::streamoff offset = (indexGender * gender_size) + (indexAge * age_size) + m_Offsets.at( nodeID );
        return offset;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFile
    // ------------------------------------------------------------------------

    MigrationInfoFile::MigrationInfoFile( MigrationMetadata* pMetaData,
                                          const std::string& rFilename,
                                          float xModifier )
        : m_Filename( rFilename )
        , m_IsEnabled( false )
        , m_xModifier( xModifier )
        , m_ParameterNameEnable( UNINITIALIZED_STRING )
        , m_ParameterNameFilename( UNINITIALIZED_STRING )
        , m_FileStream()
        , m_IsInitialized( false )
        , m_pOffsetsData( new MigrationOffsetsData( pMetaData ) )
    {
    }

    MigrationInfoFile::MigrationInfoFile( MigrationType::Enum expectedMigType, 
                                          int defaultDestinationsPerNode )
        : MigrationInfoFile( new MigrationMetadata( expectedMigType, defaultDestinationsPerNode ), "", 0.0 )
    {
    }

    MigrationInfoFile::~MigrationInfoFile()
    {
        if( m_FileStream.is_open() )
        {
            m_FileStream.close();
        }
    }

    void MigrationInfoFile::Initialize( const std::string& idreference )
    {
        if( m_IsEnabled )
        {
            if( m_Filename.empty() || (m_Filename == "UNINITIALIZED STRING") )
            {
                release_assert(m_ParameterNameEnable != "UNINITIALIZED STRING");
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                        m_ParameterNameEnable.c_str(), "1", 
                                                        m_ParameterNameFilename.c_str(), "<empty>" );
            }

            std::string filepath = Environment::FindFileOnPath( m_Filename );

            uint32_t expected_binary_file_size = ParseMetadataForFile( filepath, idreference );

            OpenMigrationFile( filepath, expected_binary_file_size );
        }
        m_IsInitialized = true;
    }

    bool MigrationInfoFile::IsInitialized() const
    {
        return m_IsInitialized;
    }

    void MigrationInfoFile::SetEnableParameterName( const std::string& rName ) 
    { 
        m_ParameterNameEnable = rName;
    }

    void MigrationInfoFile::SetFilenameParameterName( const std::string& rName ) 
    { 
        m_ParameterNameFilename = rName;
    }

    bool MigrationInfoFile::ReadData( ExternalNodeId_t fromNodeID, 
                                      const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeidSuidMap,
                                      std::vector<std::vector<MigrationRateData>>& rRateData )
    {
        bool is_fixed_rate = true;
        if( m_IsEnabled && m_pOffsetsData->HasDataFor( fromNodeID ) )
        {
            if( rRateData.size() != 2 )
            {
                release_assert( rRateData.size() == 0 );
                for( uint32_t ig = 0; ig < 2; ig++ )
                {
                    rRateData.push_back( std::vector<MigrationRateData>() );
                }
            }

            is_fixed_rate = m_pOffsetsData->IsFixedRate();

            int destinations_per_node           = m_pOffsetsData->GetDestinationsPerNode();
            MigrationType::Enum mig_type        = m_pOffsetsData->GetMigrationType();
            InterpolationType::Enum interp_type = m_pOffsetsData->GetInterpolationType();

            for( uint32_t ig = 0; ig < 2; ig++ )
            {
                int initial_node_data_size = rRateData[ ig ].size();

                const std::vector<float>& r_ages_years = m_pOffsetsData->GetAgesYears();
                for( uint32_t ja = 0; ja < r_ages_years.size(); ja++ )
                {
                    float age_years = r_ages_years[ja];

                    uint32_t array_id[MAX_DESTINATIONS] = {0};
                    double   array_rt[MAX_DESTINATIONS] = {0};

                    std::streamoff offset = m_pOffsetsData->GetOffset( ig, ja, fromNodeID );

                    m_FileStream.seekg( offset, std::ios::beg );

                    m_FileStream.read( reinterpret_cast<char*>(array_id), destinations_per_node * sizeof(uint32_t) );
                    m_FileStream.read( reinterpret_cast<char*>(array_rt), destinations_per_node * sizeof(double)   );

                    if( m_FileStream.fail() )
                    {
                        // This should really just be a safety net
                        std::stringstream ss;
                        ss << "Error reading migration data for node " << fromNodeID;
                        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, m_Filename.c_str(), ss.str().c_str() );
                    }

                    // ---------------------------------------------------------------------------------
                    // --- array_id and array_rt are contiguous.  That is, the values we want to
                    // --- extract may not start at the beginning (i.e. i=0) and may have gaps.  Hence,
                    // --- we use node_index to keep track of what node we are on.
                    // ---------------------------------------------------------------------------------
                    int node_index = initial_node_data_size;
                    for( int i = 0; i < destinations_per_node; i++ )
                    {
                        if( array_id[ i ] > 0 )
                        {
                            ExternalNodeId_t to_node_id_external = array_id[ i ];
                            if( rNodeidSuidMap.left.count( to_node_id_external ) == 0 )
                            {
                                // -----------------------------------------------------------------------------------
                                // --- Is this the right thing to do?  In the future, could there ever be extra links 
                                // --- in a migration file to nodes not in the suid-map for this simulation?
                                // -----------------------------------------------------------------------------------
                                std::stringstream ss;
                                ss << "NodeId, " << array_id[i] << ", found in " << m_Filename << ", is not a node in the simulation.";
                                throw GeneralConfigurationException(  __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                            }
                            suids::suid to_node_id_suid = rNodeidSuidMap.left.at( to_node_id_external );
                            float rate = array_rt[i] * m_xModifier; // migration tuning knob

                            if( rRateData[ ig ].size() <= node_index )
                            {
                                MigrationRateData mrd( to_node_id_suid, mig_type, interp_type );
                                rRateData[ ig ].push_back( mrd );
                            }
                            else if( (rRateData[ ig ][ node_index ].GetToNodeSuid()      != to_node_id_suid) ||
                                     (rRateData[ ig ][ node_index ].GetMigrationType()   != mig_type       ) )
                            {
                                std::stringstream ss;
                                ss << "In file '" << m_Filename << "', the 'To' Node IDs are not the same for the Age Data sections for fromNodeId = " << fromNodeID;
                                throw GeneralConfigurationException(  __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                            }
                            rRateData[ ig ][ node_index ].AddRate( age_years, rate );
                            node_index++;
                        }
                    }
                }
            }
        }
        return is_fixed_rate;
    }

    uint32_t MigrationInfoFile::ParseMetadataForFile( const std::string& data_filepath, const std::string& idreference )
    {
        string metadata_filepath = data_filepath + ".json";
        Configuration* p_config = Environment::LoadConfigurationFile( metadata_filepath );

        // we assume in the metadata that not all parameters are required
        bool prev_use_defalts = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = true;

        m_pOffsetsData->SetExpectedIdReference( idreference );
        m_pOffsetsData->Configure( p_config );

        JsonConfigurable::_useDefaults = prev_use_defalts;

        return m_pOffsetsData->CalculateExpectedBinaryFileSize();
    }

    void MigrationInfoFile::OpenMigrationFile( const std::string& filepath, uint32_t expected_binary_file_size )
    {
        FileSystem::OpenFileForReading( m_FileStream, filepath.c_str(), true );

        m_FileStream.seekg(0, ios::end);
        int filelen = m_FileStream.tellg();

        if( filelen != expected_binary_file_size )
        {
            std::stringstream ss;
            ss << "Detected wrong size for migration data file.  Expected " << expected_binary_file_size << " bytes, read " << filelen << " bytes";
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filepath.c_str(), ss.str().c_str() );
        }

        m_FileStream.seekg( 0, ios::beg );
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryFile
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Migration.File,MigrationInfoFactoryFile)
    BEGIN_QUERY_INTERFACE_BODY(MigrationInfoFactoryFile)
    END_QUERY_INTERFACE_BODY(MigrationInfoFactoryFile)

    MigrationInfoFactoryFile::MigrationInfoFactoryFile()
    : JsonConfigurable()
    , m_InfoFileList()
    {
    }

    MigrationInfoFactoryFile::~MigrationInfoFactoryFile()
    {
        for( auto mig_file : m_InfoFileList )
        {
            delete mig_file;
        }
        m_InfoFileList.clear();
    }

    void MigrationInfoFactoryFile::CreateInfoFileList()
    {
        m_InfoFileList.push_back( new MigrationInfoFile( MigrationType::LOCAL_MIGRATION,    MAX_LOCAL_MIGRATION_DESTINATIONS    ) );
        m_InfoFileList.push_back( new MigrationInfoFile( MigrationType::AIR_MIGRATION,      MAX_AIR_MIGRATION_DESTINATIONS      ) );
        m_InfoFileList.push_back( new MigrationInfoFile( MigrationType::REGIONAL_MIGRATION, MAX_REGIONAL_MIGRATION_DESTINATIONS ) );
        m_InfoFileList.push_back( new MigrationInfoFile( MigrationType::SEA_MIGRATION,      MAX_SEA_MIGRATION_DESTINATIONS      ) );
        m_InfoFileList.push_back( new MigrationInfoFile( MigrationType::FAMILY_MIGRATION,   MAX_SEA_MIGRATION_DESTINATIONS      ) );
    }

    void MigrationInfoFactoryFile::InitializeInfoFileList( const Configuration* config )
    {
        CreateInfoFileList();

        initConfigTypeMap( "Enable_Migration_Heterogeneity",  &m_IsHeterogeneityEnabled, Enable_Migration_Heterogeneity_DESC_TEXT, false, "Migration_Model", "FIXED_RATE_MIGRATION" );

        initConfigTypeMap( "Enable_Local_Migration",      &(m_InfoFileList[0]->m_IsEnabled), Enable_Local_Migration_DESC_TEXT,    false, "Migration_Model", "FIXED_RATE_MIGRATION" );
        initConfigTypeMap( "Enable_Air_Migration",        &(m_InfoFileList[1]->m_IsEnabled), Enable_Air_Migration_DESC_TEXT,      false, "Migration_Model", "FIXED_RATE_MIGRATION" );
        initConfigTypeMap( "Enable_Regional_Migration",   &(m_InfoFileList[2]->m_IsEnabled), Enable_Regional_Migration_DESC_TEXT, false, "Migration_Model", "FIXED_RATE_MIGRATION" );
        initConfigTypeMap( "Enable_Sea_Migration",        &(m_InfoFileList[3]->m_IsEnabled), Enable_Sea_Migration_DESC_TEXT,      false, "Migration_Model", "FIXED_RATE_MIGRATION" );
        initConfigTypeMap( "Enable_Family_Migration",     &(m_InfoFileList[4]->m_IsEnabled), Enable_Family_Migration_DESC_TEXT,   false, "Migration_Model", "FIXED_RATE_MIGRATION" );

        initConfigTypeMap( "Local_Migration_Filename",    &(m_InfoFileList[0]->m_Filename),  Local_Migration_Filename_DESC_TEXT,    "", "Enable_Local_Migration" );
        initConfigTypeMap( "Air_Migration_Filename",      &(m_InfoFileList[1]->m_Filename),  Air_Migration_Filename_DESC_TEXT,      "", "Enable_Air_Migration" );
        initConfigTypeMap( "Regional_Migration_Filename", &(m_InfoFileList[2]->m_Filename),  Regional_Migration_Filename_DESC_TEXT, "", "Enable_Regional_Migration" );
        initConfigTypeMap( "Sea_Migration_Filename",      &(m_InfoFileList[3]->m_Filename),  Sea_Migration_Filename_DESC_TEXT,      "", "Enable_Sea_Migration" );
        initConfigTypeMap( "Family_Migration_Filename",   &(m_InfoFileList[4]->m_Filename),  Family_Migration_Filename_DESC_TEXT,   "", "Enable_Family_Migration" );

        initConfigTypeMap( "x_Local_Migration",           &(m_InfoFileList[0]->m_xModifier), x_Local_Migration_DESC_TEXT,    0.0f, FLT_MAX, 1.0f, "Enable_Local_Migration" );
        initConfigTypeMap( "x_Air_Migration",             &(m_InfoFileList[1]->m_xModifier), x_Air_Migration_DESC_TEXT,      0.0f, FLT_MAX, 1.0f, "Enable_Air_Migration" );
        initConfigTypeMap( "x_Regional_Migration",        &(m_InfoFileList[2]->m_xModifier), x_Regional_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f, "Enable_Regional_Migration" );
        initConfigTypeMap( "x_Sea_Migration",             &(m_InfoFileList[3]->m_xModifier), x_Sea_Migration_DESC_TEXT,      0.0f, FLT_MAX, 1.0f, "Enable_Sea_Migration" );
        initConfigTypeMap( "x_Family_Migration",          &(m_InfoFileList[4]->m_xModifier), x_Family_Migration_DESC_TEXT,   0.0f, FLT_MAX, 1.0f, "Enable_Family_Migration" );
        

        m_InfoFileList[0]->SetEnableParameterName( "Enable_Local_Migration"    );
        m_InfoFileList[1]->SetEnableParameterName( "Enable_Air_Migration"      );
        m_InfoFileList[2]->SetEnableParameterName( "Enable_Regional_Migration" );
        m_InfoFileList[3]->SetEnableParameterName( "Enable_Sea_Migration"      );
        m_InfoFileList[4]->SetEnableParameterName( "Enable_Family_Migration"   );

        m_InfoFileList[0]->SetFilenameParameterName( "Local_Migration_Filename"    );
        m_InfoFileList[1]->SetFilenameParameterName( "Air_Migration_Filename"      );
        m_InfoFileList[2]->SetFilenameParameterName( "Regional_Migration_Filename" );
        m_InfoFileList[3]->SetFilenameParameterName( "Sea_Migration_Filename"      );
        m_InfoFileList[4]->SetFilenameParameterName( "Family_Migration_Filename"   );
    }

    bool MigrationInfoFactoryFile::Configure( const Configuration* config )
    {
        InitializeInfoFileList( config );

        bool ret = JsonConfigurable::Configure( config );

        return ret;
    }

    bool MigrationInfoFactoryFile::IsAtLeastOneTypeConfiguredForIndividuals() const
    {
        for( auto mif : m_InfoFileList )
        {
            if( mif->m_IsEnabled && !mif->m_Filename.empty() )
                return true;
        }
        return false;
    }

    bool MigrationInfoFactoryFile::IsEnabled( MigrationType::Enum mt ) const
    {
        for( auto mif : m_InfoFileList )
        {
            if( (mif->GetMigrationType() == mt) && mif->m_IsEnabled )
                return true;
        }
        return false;
    }


    void MigrationInfoFactoryFile::Initialize( const ::Configuration* config, const string& idreference )
    {
        Configure( config );

        for( int i = 0; i < m_InfoFileList.size(); i++ )
        {
            m_InfoFileList[i]->Initialize( idreference );
        }
    }

    IMigrationInfo* MigrationInfoFactoryFile::CreateMigrationInfo( INodeContext *pParentNode, 
                                                                   const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        bool is_fixed_rate = true ;
        std::vector<std::vector<MigrationRateData>> rate_data = GetRateData( pParentNode, rNodeIdSuidMap, m_InfoFileList, &is_fixed_rate );

        // -------------------------------------------------------------------------
        // --- it's possible that all 4 migration-types are empty for a given node,
        // --- i.e. this node is a "fortress/island" node; in that case, we return
        // --- null object so this node is not considered for migration
        // ---
        // --- NOTE: I didn't make Initialize() part of IMigrationInfo so that
        // --- MigrationRateData is known everywhere.
        // -------------------------------------------------------------------------
        IMigrationInfo* p_new_migration_info; // = nullptr;
        if( rate_data.size() > 0 )
        {
            if( is_fixed_rate )
            {
                MigrationInfoFixedRate* p_mifr = _new_ MigrationInfoFixedRate( pParentNode, m_IsHeterogeneityEnabled );
                p_mifr->Initialize( rate_data ); 
                p_new_migration_info = p_mifr;
            }
            else
            {
                MigrationInfoAgeAndGender* p_miag = _new_ MigrationInfoAgeAndGender( pParentNode, m_IsHeterogeneityEnabled );
                p_miag->Initialize( rate_data );
                p_new_migration_info = p_miag;
            }
        }
        else
        {
            p_new_migration_info = new MigrationInfoNull();
        }

        return p_new_migration_info;
    }

    std::vector<std::vector<MigrationRateData>> MigrationInfoFactoryFile::GetRateData( INodeContext *pParentNode, 
                                                                                       const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                                       std::vector<MigrationInfoFile*>& infoFileList,
                                                                                       bool* pIsFixedRate )
    {
        suids::suid from_node_suid = pParentNode->GetSuid();

        if( rNodeIdSuidMap.right.count( from_node_suid ) == 0)
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "rNodeIdSuidMap.right.count(node_suid)", 0, "node_suid", from_node_suid.data );
        }

        ExternalNodeId_t from_node_id = rNodeIdSuidMap.right.at( from_node_suid );

        std::vector<bool> demog_enabled = pParentNode->GetMigrationTypeEnabledFromDemographics();
        release_assert( demog_enabled.size() == infoFileList.size() );

        *pIsFixedRate = true;
        std::vector<std::vector<MigrationRateData>> rate_data;
        for( int i = 0; i < infoFileList.size(); i++ )
        {
            if( demog_enabled[i] && (infoFileList[i] != nullptr) )
            {
                MigrationInfoFile* p_mif = dynamic_cast<MigrationInfoFile*>(infoFileList[i]);
                release_assert( p_mif );

                *pIsFixedRate &= p_mif->ReadData( from_node_id, rNodeIdSuidMap, rate_data );
            }
        }

        return rate_data;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryDefault
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Migration.Default,MigrationInfoFactoryDefault)
    BEGIN_QUERY_INTERFACE_BODY(MigrationInfoFactoryDefault)
    END_QUERY_INTERFACE_BODY(MigrationInfoFactoryDefault)

    MigrationInfoFactoryDefault::MigrationInfoFactoryDefault( int torusSize )
    : m_IsHeterogeneityEnabled(false)
    , m_xLocalModifier(1.0)
    , m_TorusSize( torusSize )
    {
        InitializeParameters();
    }

    MigrationInfoFactoryDefault::MigrationInfoFactoryDefault()
    : m_IsHeterogeneityEnabled(false)
    , m_xLocalModifier(1.0)
    , m_TorusSize(0)
    {
        InitializeParameters(); 
    }

    MigrationInfoFactoryDefault::~MigrationInfoFactoryDefault()
    {
    }

    void MigrationInfoFactoryDefault::InitializeParameters()
    {
        initConfigTypeMap( "x_Local_Migration", &m_xLocalModifier, x_Local_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f, "Enable_Local_Migration" );
    }

    bool MigrationInfoFactoryDefault::Configure( const Configuration* config )
    {
        bool ret = JsonConfigurable::Configure( config );
        return ret;
    }

    void MigrationInfoFactoryDefault::Initialize( const ::Configuration* config, const string& idreference )
    {
        Configure( config );
    }

    bool MigrationInfoFactoryDefault::IsAtLeastOneTypeConfiguredForIndividuals() const
    {
        return false;
    }

    bool MigrationInfoFactoryDefault::IsEnabled( MigrationType::Enum mt ) const
    {
        return (mt == MigrationType::LOCAL_MIGRATION);
    }

    IMigrationInfo* MigrationInfoFactoryDefault::CreateMigrationInfo( INodeContext *pParentNode, 
                                                                      const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        std::vector<std::vector<MigrationRateData>> rate_data = GetRateData( pParentNode, rNodeIdSuidMap, m_TorusSize, m_xLocalModifier );

        MigrationInfoFixedRate* new_migration_info = _new_ MigrationInfoFixedRate( pParentNode, m_IsHeterogeneityEnabled );
        new_migration_info->Initialize( rate_data );

        return new_migration_info;
    }

    std::vector<std::vector<MigrationRateData>> MigrationInfoFactoryDefault::GetRateData( INodeContext *pParentNode, 
                                                                                          const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                                          int torus_size,
                                                                                          float modifier )
    {
        suids::suid from_node_suid = pParentNode->GetSuid();

        if(rNodeIdSuidMap.right.count( from_node_suid ) == 0)
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "rNodeIdSuidMap.right.count(from_node_suid)", 0, "from_node_suid", from_node_suid.data );
        }

        ExternalNodeId_t from_node_id = rNodeIdSuidMap.right.at( from_node_suid );


        int offsets[]    = {  -(torus_size+1), -torus_size, -(torus_size-1),
                                        -1,                           1,
                               (torus_size-1),  torus_size,  (torus_size+1)};

        double basicrate = 1.0f / MAX_LOCAL_MIGRATION_DESTINATIONS / 10; // on average, a person should go to one of the 8 surrounding nodes every 10 days, per Philip

        basicrate *= modifier;

        // correct offsets if on any of the edges (of numbering grid scheme, not the torus)

        if( from_node_id % torus_size == 1 ) // left edge
        {
            offsets[0] += torus_size;
            offsets[3] += torus_size;
            offsets[5] += torus_size;
        }
        else if( from_node_id % torus_size == 0 ) //right edge
        {
            offsets[2] -= torus_size;
            offsets[4] -= torus_size;
            offsets[7] -= torus_size;
        }

        if( from_node_id <= uint32_t(torus_size) ) // top edge
        {
            offsets[0] += torus_size * torus_size;
            offsets[1] += torus_size * torus_size;
            offsets[2] += torus_size * torus_size;
        }
        else if( from_node_id > uint32_t(torus_size * (torus_size - 1)) ) // bottom edge
        {
            offsets[5] -= torus_size * torus_size;
            offsets[6] -= torus_size * torus_size;
            offsets[7] -= torus_size * torus_size;
        }

        LOG_DEBUG_F( "MAX_LOCAL_MIGRATION_DESTINATIONS = %d\n", MAX_LOCAL_MIGRATION_DESTINATIONS );

        std::vector<std::vector<MigrationRateData>> rate_data;
        rate_data.push_back( std::vector<MigrationRateData>() );
        rate_data.push_back( std::vector<MigrationRateData>() ); // for females (needed for vector migration as we always have migration per sex)

        for (int i = 0; i < MAX_LOCAL_MIGRATION_DESTINATIONS; i++)
        {
            release_assert( from_node_id + offsets[i] >= 1 );
            release_assert( from_node_id + offsets[i] <= uint32_t(torus_size * torus_size) );

            ExternalNodeId_t to_node_id = from_node_id + offsets[i];
            suids::suid to_node_suid = rNodeIdSuidMap.left.at( to_node_id );

            MigrationRateData mrd( to_node_suid, MigrationType::LOCAL_MIGRATION, InterpolationType::LINEAR_INTERPOLATION );
            mrd.AddRate( MAX_HUMAN_AGE, basicrate );
            rate_data[0].push_back( mrd );
            rate_data[1].push_back( mrd ); // for females (needed for vector migration as we always have migration per sex)
        }

        return rate_data;
    }

}
