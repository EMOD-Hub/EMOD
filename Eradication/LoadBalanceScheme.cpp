
#include "stdafx.h"

#include <fstream>

#include "LoadBalanceScheme.h"
#include "Log.h"
#include "Debug.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "Configure.h"
#include "Configuration.h"

SETUP_LOGGING( "LoadBalanceScheme" )

namespace Kernel 
{
    // ---------------------------------------
    // --- JsonInitialLoadBalanceScheme
    // ---------------------------------------
    class JsonInitialLoadBalanceScheme : public JsonConfigurable,
                                         public IInitialLoadBalanceScheme
    {
    public:
        JsonInitialLoadBalanceScheme()
            : JsonConfigurable()
            , IInitialLoadBalanceScheme()
            , m_InitialNodeRankMapping()
        {
        }

        virtual ~JsonInitialLoadBalanceScheme()
        {
        }

        virtual void Initialize( const std::string& rFilename, uint32_t expectedNumNodes, uint32_t numTasks ) override
        {
            release_assert( expectedNumNodes > 0 );
            release_assert( numTasks > 0 );

            std::vector< std::vector<int> > matrix ;
            initConfigTypeMap( "Load_Balance_Scheme_Nodes_On_Core_Matrix", &matrix, "N/A", 0, INT_MAX, 0 );

            Configuration* p_config = Configuration::Load( rFilename );
            release_assert( p_config );
            Configure( p_config );
            delete p_config ;

            if( matrix.size() != numTasks )
            {
                std::ostringstream msg ;
                msg << "The Load Balance Scheme file, '" << rFilename << "', ";
                msg << "assumes that it is used with " << matrix.size() << " cores, but only " << numTasks << " have been allocated." ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            for( int rank = 0 ; rank < matrix.size() ; rank++ )
            {
                for( auto node_id : matrix[ rank ] )
                {
                    m_InitialNodeRankMapping[ node_id ] = rank ;
                }
            }
        }

        virtual int GetInitialRankFromNodeId(uint32_t node_id) override
        {
            if( m_InitialNodeRankMapping.count( node_id ) > 0 )
            {
                return m_InitialNodeRankMapping.at( node_id ); 
            }
            else
            {
                return 0 ;
            }
        }

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject)  override { return Kernel::e_NOINTERFACE; }
        virtual int32_t AddRef()  override { return -1 ; }
        virtual int32_t Release() override { return -1 ; }
    private:
        std::map<uint32_t, int> m_InitialNodeRankMapping;
    };

    // ---------------------------------------
    // --- LegacyFileInitialLoadBalanceScheme
    // ---------------------------------------
    class LegacyFileInitialLoadBalanceScheme : public IInitialLoadBalanceScheme
    {
    public:
        LegacyFileInitialLoadBalanceScheme()
            : IInitialLoadBalanceScheme()
            , m_InitialNodeRankMapping()
        {
        }

        virtual ~LegacyFileInitialLoadBalanceScheme()
        {
        }

        virtual void Initialize( const std::string& rFilename, uint32_t expectedNumNodes, uint32_t numTasks ) override
        {
            release_assert( expectedNumNodes > 0 );
            release_assert( numTasks > 0 );

            std::ifstream loadbalancefile;
            FileSystem::OpenFileForReading( loadbalancefile, rFilename.c_str(), true );

            loadbalancefile.seekg(0, std::ios::end);
            int filelen = (int)loadbalancefile.tellg();
            int expected_size = sizeof(uint32_t) + (expectedNumNodes * (sizeof(uint32_t) + sizeof(float)));

            if(filelen != expected_size)
            {
                std::ostringstream msg ;
                msg << "Problem with load-balancing file: " << rFilename <<".  " ;
                msg << "File-size ("<< filelen <<") doesn't match expected size (" << expected_size << ") ";
                msg << "given the number of nodes (" << expectedNumNodes << ") in the demographics file";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            uint32_t num_nodes;
            loadbalancefile.seekg(0, std::ios::beg);
            loadbalancefile.read((char*)&num_nodes, 1*sizeof(num_nodes));

            if(num_nodes != expectedNumNodes)
            {
                std::ostringstream msg ;
                msg << "Malformed load-balancing file: " << rFilename <<".  " ;
                msg << "The node-count specified in file (" << num_nodes << ") doesn't match the expected number of nodes (" << expectedNumNodes << ")";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            LOG_INFO_F("Opened %s, reading balancing data for %d nodes...\n", rFilename.c_str(), num_nodes);

            std::vector<uint32_t> nodeids(num_nodes);
            std::vector<float> balance_scale(num_nodes);

            loadbalancefile.read((char*)&nodeids[0], (num_nodes)*sizeof(uint32_t));
            loadbalancefile.read((char*)&balance_scale[0], (num_nodes)*sizeof(float));

            for(uint32_t i = 0; i < num_nodes; i++)
            {
                m_InitialNodeRankMapping[nodeids[i]] = (int)(numTasks*balance_scale[i]);
            }
            LOG_INFO("Static initial load balancing scheme initialized.\n");

            loadbalancefile.close();
        }

        virtual int GetInitialRankFromNodeId(uint32_t node_id) override
        {
            return m_InitialNodeRankMapping[ node_id ]; 
        }

    private:
        std::map<uint32_t, int> m_InitialNodeRankMapping;
    };


    // ----------------------------------------
    // --- CheckerboardInitialLoadBalanceScheme
    // ----------------------------------------
    class CheckerboardInitialLoadBalanceScheme : public IInitialLoadBalanceScheme
    {
    public:
        CheckerboardInitialLoadBalanceScheme()
            : IInitialLoadBalanceScheme()
            , m_NumRanked(0)
            , m_NumTasks(0)
        {
        }

        virtual ~CheckerboardInitialLoadBalanceScheme()
        {
        }

        virtual void Initialize( const std::string& rFilename, uint32_t expectedNumNodes, uint32_t numTasks ) override
        {
            release_assert( expectedNumNodes > 0 );
            release_assert( numTasks > 0 );

            m_NumTasks = numTasks ;
        }

        virtual int GetInitialRankFromNodeId(uint32_t node_id) override
        {
            return (m_NumRanked++) % m_NumTasks; 
        }

    private:
        uint32_t m_NumRanked;
        uint32_t m_NumTasks;
    };

    // ------------------------------------
    // --- StripedInitialLoadBalanceScheme
    // ------------------------------------
    class StripedInitialLoadBalanceScheme : public IInitialLoadBalanceScheme
    {
    public:
        StripedInitialLoadBalanceScheme()
            : IInitialLoadBalanceScheme()
            , m_NumRanked(0)
            , m_NumTasks(0)
            , m_NumNodes(0)
        {
        }

        virtual ~StripedInitialLoadBalanceScheme()
        {
        }

        virtual void Initialize( const std::string& rFilename, uint32_t expectedNumNodes, uint32_t numTasks ) override
        {
            release_assert( expectedNumNodes > 0 );
            release_assert( numTasks > 0 );
            m_NumNodes = expectedNumNodes ;
            m_NumTasks = numTasks ;
        }

        virtual int GetInitialRankFromNodeId(uint32_t node_id) override
        {
            return (int)(((float)(m_NumRanked++) / m_NumNodes) * m_NumTasks); 
        }

    private:
        uint32_t m_NumRanked;
        uint32_t m_NumTasks;
        uint32_t m_NumNodes;
    };

    // -----------------------------
    // --- LoadBalanceSchemeFactory
    // -----------------------------

    IInitialLoadBalanceScheme* LoadBalanceSchemeFactory::Create( const std::string& rFilename, 
                                                                 uint32_t expectedNumNodes, 
                                                                 uint32_t numTasks )
    {
        std::string extension = "" ;
        std::size_t pos = rFilename.find_last_of(".");
        if( (pos != std::string::npos) && ((pos+1) < rFilename.length()) )
        {
            extension = rFilename.substr( pos+1 );
        }

        IInitialLoadBalanceScheme* p_lbs = nullptr ;
        if( extension == "json" )
        {
            LOG_INFO_F("Using Json Load Balance Scheme from file: %s.\n", rFilename);
            p_lbs = new JsonInitialLoadBalanceScheme();
        }
        else if( !rFilename.empty() && FileSystem::FileExists( rFilename ) )
        {
            LOG_INFO_F("Using Legacy Load Balance Scheme from file: %s.\n", rFilename);
            p_lbs = new LegacyFileInitialLoadBalanceScheme();
        }
        else
        {
            LOG_INFO("Using the default Checkerboard Load Balance Scheme.\n");
            p_lbs = new CheckerboardInitialLoadBalanceScheme();
        }
        p_lbs->Initialize( rFilename, expectedNumNodes, numTasks );

        return p_lbs ;
    }
}
