
#include "stdafx.h"
#include "RandomNumberGeneratorFactory.h"
#include "RANDOM.h"
#include "IArchive.h"
#include "Configuration.h"

#include "Log.h"

SETUP_LOGGING( "RandomNumberGeneratorFactory" )

namespace Kernel
{
    RANDOMBASE* RandomNumberGeneratorFactory::CreateRandomNumberGeneratorForReport()
    {
        uint16_t run_number = GET_CONFIG_INTEGER( EnvPtr->Config, "Run_Number" );
        uint16_t randomseed[2];
        randomseed[0] = (uint16_t) run_number;
        randomseed[1] = (uint16_t) EnvPtr->MPI.Rank;
        RANDOMBASE* p_rng = new PSEUDO_DES( *((uint32_t*)randomseed) );
        return p_rng;
    }

    GET_SCHEMA_STATIC_WRAPPER_IMPL( RandomNumberGeneratorFactory, RandomNumberGeneratorFactory )
    BEGIN_QUERY_INTERFACE_BODY( RandomNumberGeneratorFactory )
    END_QUERY_INTERFACE_BODY( RandomNumberGeneratorFactory )

    RandomNumberGeneratorFactory::RandomNumberGeneratorFactory()
        : JsonConfigurable()
        , m_RngType( RandomNumberGeneratorType::USE_PSEUDO_DES )
        , m_Policy( RandomNumberGeneratorPolicy::ONE_PER_CORE )
        , m_CreateFromSerializedData( false )
        , m_RunNumber( 0 )
        , m_NodeIds()
        , m_NodeIdsIndex(0)
        , m_pSeedRng( nullptr )
    {
    }

    RandomNumberGeneratorFactory::~RandomNumberGeneratorFactory()
    {
    }

    bool RandomNumberGeneratorFactory::Configure( const Configuration* inputJson )
    {
        int32_t run_number = 0;
        if( JsonConfigurable::_dryrun || inputJson->Exist( "Random_Number_Generator_Type" ) )
        {
            initConfig( "Random_Number_Generator_Type",
                        m_RngType, inputJson,
                        MetadataDescriptor::Enum( "Random_Number_Generator_Type",
                                                  Random_Number_Generator_Type_DESC_TEXT,
                                                  MDD_ENUM_ARGS( RandomNumberGeneratorType ) ) );
        }
        if( JsonConfigurable::_dryrun || inputJson->Exist( "Random_Number_Generator_Policy" ) )
        {
            initConfig( "Random_Number_Generator_Policy",
                        m_Policy, inputJson,
                        MetadataDescriptor::Enum( "Random_Number_Generator_Policy",
                                                  Random_Number_Generator_Policy_DESC_TEXT,
                                                  MDD_ENUM_ARGS( RandomNumberGeneratorPolicy ) ) );
        }

        // restrict this value to only have 16-bits of values
        initConfigTypeMap( "Run_Number", &run_number, Run_Number_DESC_TEXT, 0, USHRT_MAX, 1 );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_RunNumber = uint16_t( run_number );

            if( (m_Policy != RandomNumberGeneratorPolicy::ONE_PER_CORE) &&
                (m_RngType == RandomNumberGeneratorType::USE_LINEAR_CONGRUENTIAL) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Random_Number_Generator_Type", RandomNumberGeneratorType::pairs::lookup_key( m_RngType ),
                                                        "Random_Number_Generator_Policy", RandomNumberGeneratorPolicy::pairs::lookup_key( m_Policy ),
                                                        "\n'Random_Number_Generator_Policy' must be 'ONE_PER_CORE' when using 'Random_Number_Generator_Type' = 'USE_LINEAR_CONGRUENTIAL'." );
            }

            if( (m_Policy != RandomNumberGeneratorPolicy::ONE_PER_CORE) &&
                inputJson->Exist( "Allow_NodeID_Zero" ) &&
                ((*inputJson)[ "Allow_NodeID_Zero" ].As<json::Number>() == 1) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Allow_NodeID_Zero", 1,
                                                        "Random_Number_Generator_Policy", RandomNumberGeneratorPolicy::pairs::lookup_key( m_Policy ),
                                                        "\n'Random_Number_Generator_Policy' must be 'ONE_PER_CORE' when using 'Allow_NodeID_Zero' = 1." );
            }

            if( !m_CreateFromSerializedData && (m_Policy == RandomNumberGeneratorPolicy::ONE_PER_NODE) )
            {
                m_pSeedRng = CreateRng( m_RunNumber, 256 );
            }
        }
        return ret;
    }

    RandomNumberGeneratorPolicy::Enum RandomNumberGeneratorFactory::GetPolicy() const
    {
        return m_Policy;
    }

    void RandomNumberGeneratorFactory::SetNodeIds( const std::vector<ExternalNodeId_t>& rNodeIds )
    {
        if( !m_CreateFromSerializedData && (m_Policy == RandomNumberGeneratorPolicy::ONE_PER_NODE) )
        {
            m_NodeIds = rNodeIds;
            m_NodeIdsIndex = 0;
        }
    }

    RANDOMBASE* RandomNumberGeneratorFactory::CreateRng( ExternalNodeId_t externalNodeId )
    {
        RANDOMBASE* p_rng = nullptr;
        if( !m_CreateFromSerializedData && 
            ( ((m_Policy == RandomNumberGeneratorPolicy::ONE_PER_CORE) && (externalNodeId == UINT_MAX)                         ) ||
              ((m_Policy == RandomNumberGeneratorPolicy::ONE_PER_NODE) && (externalNodeId != UINT_MAX) && (externalNodeId >  0)) ) )
        {
            uint32_t seed = CreateSeed( externalNodeId );
            uint32_t cached_count = GetCachedCount();
            p_rng = CreateRng( seed, cached_count );
        }
        return p_rng;
    }

    RANDOMBASE* RandomNumberGeneratorFactory::CreateRng( uint32_t seed, uint32_t cachedCount )
    {
        RANDOMBASE* p_rng = nullptr;
        switch( m_RngType )
        {
            case RandomNumberGeneratorType::USE_LINEAR_CONGRUENTIAL:
                p_rng = _new_ LINEAR_CONGRUENTIAL( seed, cachedCount );
                break;

            case RandomNumberGeneratorType::USE_PSEUDO_DES:
                p_rng = _new_ PSEUDO_DES( uint64_t( seed ), cachedCount );
                break;

            case RandomNumberGeneratorType::USE_AES_COUNTER:
                p_rng = _new_ AES_COUNTER( uint64_t( seed ), cachedCount );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "m_RngType", m_RngType, "RandomNumberGeneratorType" );
        }
        LOG_DEBUG_F( "Using %s random number generator.\n", RandomNumberGeneratorType::pairs::lookup_key( m_RngType ) );
        return p_rng;
    }

    uint32_t RandomNumberGeneratorFactory::CreateSeed( ExternalNodeId_t externalNodeId )
    {
        uint32_t seed = 0;

        if( m_Policy == RandomNumberGeneratorPolicy::ONE_PER_CORE )
        {
            uint16_t randomseed[ 2 ];
            randomseed[ 0 ] = m_RunNumber;
            randomseed[ 1 ] = uint16_t( EnvPtr->MPI.Rank );

            seed = *reinterpret_cast<uint32_t*>(randomseed);
        }
        else
        {
            release_assert( m_pSeedRng != nullptr );
            release_assert( externalNodeId > 0 );

            // ------------------------------------------------------------------------
            // --- Here we are using a RNG that was seeded with just the Run_Number
            // --- to generate seeds for the RNGs for each node.  In order to get the
            // --- the same seed for each node regardless of core, we assume that each
            // --- node is generated in the same order on each core.  This method is
            // --- only being called for the nodes that are being created on this core.
            // --- This means we need to skip the id's and that node's seed in order
            // --- to get the correct seed for this node.
            // ------------------------------------------------------------------------
            do
            {
                release_assert( m_NodeIdsIndex < m_NodeIds.size() );
                seed = m_pSeedRng->ul();
            } while( externalNodeId != m_NodeIds[ m_NodeIdsIndex++ ] );
        }
        return seed;
    }

    uint32_t RandomNumberGeneratorFactory::GetCachedCount()
    {
        uint32_t cached_count = 1 << 20;
        if( m_Policy == RandomNumberGeneratorPolicy::ONE_PER_NODE )
        {
            cached_count = 1 << 18;
        }
        return cached_count;
    }

    void RandomNumberGeneratorFactory::CreateFromSerializeData( bool fromSerializedData )
    {
        m_CreateFromSerializedData = fromSerializedData;
    }

    REGISTER_SERIALIZABLE( RandomNumberGeneratorFactory );

    void RandomNumberGeneratorFactory::serialize( IArchive& ar, RandomNumberGeneratorFactory* pRngF )
    {
        RandomNumberGeneratorFactory& rRngF = *pRngF;

        uint32_t temp_run_number = rRngF.m_RunNumber;
        ar.labelElement( "m_RngType"   ) & (uint32_t&)rRngF.m_RngType;
        ar.labelElement( "m_Policy"    ) & (uint32_t&)rRngF.m_Policy;
        ar.labelElement( "m_RunNumber" ) & temp_run_number;

        rRngF.m_RunNumber = temp_run_number;

        // This should not be serialized since it should be set by Simulation.
        //ar.labelElement( "m_CreateFromSerializedData" ) & rRngF.m_CreateFromSerializedData;

        // These variables should only be needed at initialization
        //std::vector<ExternalNodeId_t> m_NodeIds;
        //uint32_t m_NodeIdsIndex;

        ar.labelElement( "m_pSeedRng" ) & rRngF.m_pSeedRng;
    }
}
