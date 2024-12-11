
#pragma once

#include "stdafx.h"
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>

#ifdef __GNUC__
namespace std
{
     using namespace __gnu_cxx;
}
#endif

#include "IMigrationInfo.h"
#include "InterpolatedValueMap.h"
#include "IndividualEventContext.h"

#define MAX_LOCAL_MIGRATION_DESTINATIONS    (8)
#define MAX_AIR_MIGRATION_DESTINATIONS      (60)
#define MAX_REGIONAL_MIGRATION_DESTINATIONS (30)
#define MAX_SEA_MIGRATION_DESTINATIONS      (5)



namespace Kernel
{
    ENUM_DEFINE(GenderDataType,
        ENUM_VALUE_SPEC( SAME_FOR_BOTH_GENDERS       , 0)   // The one set of the data is used for both genders
        ENUM_VALUE_SPEC( ONE_FOR_EACH_GENDER         , 1)   // There are two sets of data - one for each gender
        ENUM_VALUE_SPEC( VECTOR_MIGRATION_BY_GENETICS, 2 )) // Only for vector migration: we will be using 
                                                            // vector genetics (which accounts for gender) for migration

    ENUM_DEFINE(InterpolationType,
        ENUM_VALUE_SPEC(LINEAR_INTERPOLATION , 0)  // Interpolate between ages - no extrapolation
        ENUM_VALUE_SPEC(PIECEWISE_CONSTANT   , 1)) // Use the value if the age is greater than the current and less than the next

    // ---------------------------
    // --- MigrationRateData
    // ---------------------------

    // MigrationRateData contains data about migrating to a particular node.
    // The rates can be age dependent.
    class IDMAPI MigrationRateData
    {
    public:
        MigrationRateData();
        MigrationRateData( suids::suid to_node_suid, MigrationType::Enum migType, InterpolationType::Enum interpType );

        const suids::suid       GetToNodeSuid()           const;
        MigrationType::Enum     GetMigrationType()        const;
        InterpolationType::Enum GetInterpolationType()    const;
        int                     GetNumRates()             const;
        float                   GetRate( float ageYears ) const;

        void AddRate( float ageYears, float rate );
    private:
        suids::suid             m_ToNodeSuid ;
        MigrationType::Enum     m_MigType ;
        InterpolationType::Enum m_InterpType ;
        InterpolatedValueMap    m_InterpMap;
    };

    // ---------------------------
    // --- MigrationInfoNull
    // ---------------------------

    // MigrationInfoNull is the null object in the Null Object Pattern.
    // Essentially, this object implements the IMigrationInfo interface
    // but doesn't do anything.  This object is given to nodes when migration
    // is on but the node does not have any migration away from it.
    class IDMAPI MigrationInfoNull : virtual public IMigrationInfo
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoNull();

        // IMigrationInfo methods
        virtual void PickMigrationStep( RANDOMBASE* pRNG,
                                        IIndividualHumanEventContext * traveler, 
                                        float migration_rate_modifier, 
                                        suids::suid &destination, 
                                        MigrationType::Enum &migration_type,
                                        float &time, 
                                        float dt = FLT_MAX ) override; // FLT_MAX for humans, dt for vectors
        virtual void  SetContextTo( INodeContext* _parent ) override;
        virtual float GetTotalRate( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<float>&               GetCumulativeDistributionFunction( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<suids::suid>&         GetReachableNodes( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypes( Gender::Enum gender = Gender::MALE ) const override;

        virtual bool IsHeterogeneityEnabled() const override;

    protected:
        friend class MigrationInfoFactoryFile;

        MigrationInfoNull();

        // We need these member variables so that GetReachableNodes() and GetMigrationTypes()
        // can return references to objects that exist.
        std::vector<float>               m_EmptyListCDF;
        std::vector<suids::suid>         m_EmptyListNodes;
        std::vector<MigrationType::Enum> m_EmptyListTypes;
    };

    // ---------------------------
    // --- MigrationInfoFixedRate
    // ---------------------------

    // MigrationInfoFixedRate is an IMigrationInfo object that has fixed/constant rates.
    class IDMAPI MigrationInfoFixedRate : virtual public IMigrationInfo
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoFixedRate();

        // IMigrationInfo methods
        virtual void PickMigrationStep( RANDOMBASE* pRNG,
                                        IIndividualHumanEventContext * traveler, 
                                        float migration_rate_modifier, 
                                        suids::suid &destination, 
                                        MigrationType::Enum &migration_type,
                                        float &time, 
                                        float dt = FLT_MAX ) override;  // FLT_MAX for humans, dt for vectors
        virtual void  SetContextTo( INodeContext* _parent ) override;
        virtual float GetTotalRate( Gender::Enum gender = Gender::MALE ) const override;
        virtual bool  IsHeterogeneityEnabled() const override;
        virtual const std::vector<float>&               GetCumulativeDistributionFunction( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<suids::suid>&         GetReachableNodes( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypes( Gender::Enum gender = Gender::MALE ) const override;


    protected:
        friend class MigrationInfoFactoryFile;
        friend class MigrationInfoFactoryDefault;

        MigrationInfoFixedRate( INodeContext* _parent,
                                bool isHeterogeneityEnabled );

        virtual void Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData );
        virtual void CalculateRates( Gender::Enum gender, float ageYears );
        virtual void NormalizeRates( std::vector<float>& r_rate_cdf, float& r_total_rate );
        virtual void SaveRawRates( std::vector<float>& r_rate_cdf, Gender::Enum gender ) {}


        INodeContext *                   m_Parent;
        bool                             m_IsHeterogeneityEnabled;
        std::vector<suids::suid>         m_ReachableNodes;
        std::vector<MigrationType::Enum> m_MigrationTypes;
        std::vector<float>               m_RateCDF;
        float                            m_TotalRate;
    };

    // -----------------------------
    // --- MigrationInfoAgeAndGender
    // -----------------------------

    // MigrationInfoAgeAndGender is an IMigrationInfo object that is used when the rates
    // between nodes is dependent on gender and age.  The reachable/"to nodes" can be different
    // for each gender.
    class IDMAPI MigrationInfoAgeAndGender : public MigrationInfoFixedRate
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoAgeAndGender();

    protected:
        friend class MigrationInfoFactoryFile;

        MigrationInfoAgeAndGender( INodeContext* _parent,
                                   bool isHeterogeneityEnabled );

        virtual void Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData );
        virtual void CalculateRates( Gender::Enum gender, float ageYears );

        virtual const std::vector<suids::suid>&         GetReachableNodes( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypes( Gender::Enum gender = Gender::MALE ) const override;

        std::vector<std::vector<MigrationRateData>> m_RateData;
        std::vector<suids::suid>                    m_ReachableNodesFemale;
        std::vector<MigrationType::Enum>            m_MigrationTypesFemale;
    };

    // -----------------------------
    // --- MigrationMetadata
    // -----------------------------

    // MigrationInfoFileMetadata is the "Metadata" contained in the XXX.bin.json file.  This metadata is used
    // to tell EMOD how to read and interpret the data in the XXX.bin file.
    class MigrationMetadata : public JsonConfigurable
    {
    public:
        MigrationMetadata( MigrationType::Enum expectedMigType, 
                           int defaultDestinationsPerNode );
        virtual ~MigrationMetadata();

        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual bool Configure( const Configuration* config ) override;

        void SetExpectedIdReference( const std::string& rExpectedIdReference );

        int                       GetNumNodes()            const { return m_NumNodes;              }
        MigrationType::Enum       GetMigrationType()       const { return m_ExpectedMigrationType; }
        GenderDataType::Enum      GetGenderDataType()      const { return m_GenderDataType;        }
        InterpolationType::Enum   GetInterpolationType()   const { return m_InterpolationType;     }
        const std::vector<float>& GetAgesYears()           const { return m_AgesYears;             }
        uint32_t                  GetGenderDataSize()      const { return m_GenderDataSize;        }
        uint32_t                  GetAgeDataSize()         const { return m_AgeDataSize;           }
        int                       GetDestinationsPerNode() const { return m_DestinationsPerNode;   }

        uint32_t CalculateExpectedBinaryFileSize();
        bool IsFixedRate() const;

    protected:
        virtual void CheckGenderDataType( const Configuration* config );
        virtual void ConfigDatavalueCount( const Configuration* config );
        virtual void ConfigInterpolationType( const Configuration* config );
        virtual void ConfigMigrationType( const Configuration* config, MigrationType::Enum& file_migration_type );
        virtual void CheckMigrationType( const Configuration* config, const MigrationType::Enum file_migration_type );
        uint32_t GetNumGenderDataChunks() const;

        std::string             m_ExpectedIdReference;
        MigrationType::Enum     m_ExpectedMigrationType;
        int                     m_DestinationsPerNode;
        int                     m_NumNodes;
        GenderDataType::Enum    m_GenderDataType;
        InterpolationType::Enum m_InterpolationType;
        std::vector<float>      m_AgesYears;
        uint32_t                m_GenderDataSize;
        uint32_t                m_AgeDataSize;
    };

    // -----------------------------
    // --- MigrationOffsetsData
    // -----------------------------
    
    class MigrationOffsetsData : public JsonConfigurable
    {
    public:
        MigrationOffsetsData( MigrationMetadata* pMetadata );
        MigrationOffsetsData( MigrationType::Enum expectedMigType, 
                              int defaultDestinationsPerNode );
        virtual ~MigrationOffsetsData();

        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual bool Configure( const Configuration* config ) override;

        void SetExpectedIdReference( const std::string& rExpectedIdReference );

        const std::vector<float>& GetAgesYears()                const;
        int                       GetDestinationsPerNode()      const;
        MigrationType::Enum       GetMigrationType()            const;
        InterpolationType::Enum   GetInterpolationType()        const;
        bool                      IsFixedRate()                 const;
        bool                      HasDataFor( uint32_t nodeID ) const;

        uint32_t CalculateExpectedBinaryFileSize();
        std::streamoff GetOffset( int indexGender, int indexAge, uint32_t nodeID ) const;

    protected:
        MigrationMetadata* m_pMetadata;
        std::unordered_map< ExternalNodeId_t, uint32_t > m_Offsets;
    };

    // ----------------------
    // --- MigrationInfoFile
    // ----------------------

    // MigrationInfoFile is responsible for reading the migration data from files.
    // This object assumes that for one file name there is one binary file containing the
    // "to-node" and rate data while json-formatted metadata file contains extra information
    // about the data in the binary file.  The factory uses this object to create
    // the IMigrationInfo objects.
    class IDMAPI MigrationInfoFile
    {
    public:
        // These are public so that the factory can put these variables into initConfig() statements
        std::string m_Filename ;
        bool m_IsEnabled ;
        float m_xModifier ;

        MigrationInfoFile( MigrationMetadata* pMetaData, const std::string& rFilename, float xModifier );
        MigrationInfoFile( MigrationType::Enum migType, 
                           int defaultDestinationsPerNode );
        virtual ~MigrationInfoFile();

        virtual void Initialize( const std::string& idreference );
        virtual bool IsInitialized() const;

        virtual void SetEnableParameterName( const std::string& rName );
        virtual void SetFilenameParameterName( const std::string& rName );

        virtual bool ReadData( ExternalNodeId_t                                   fromNodeID, 
                               const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeidSuidMap,
                               std::vector<std::vector<MigrationRateData>>&       rRateData );

        MigrationType::Enum  GetMigrationType()  const { return m_pOffsetsData->GetMigrationType(); }


    protected:
        // Returns the expected size of the binary file
        uint32_t ParseMetadataForFile( const std::string& data_filepath, const std::string& idreference );
        void     OpenMigrationFile( const std::string& filepath, uint32_t expected_binary_file_size );

        std::string               m_ParameterNameEnable;
        std::string               m_ParameterNameFilename;
        std::ifstream             m_FileStream;
        bool                      m_IsInitialized;
        MigrationOffsetsData*     m_pOffsetsData;
    };

    // ----------------------------------
    // --- MigrationInfoFactoryFile
    // ----------------------------------

    // MigrationInfoFactoryFile is an IMigrationInfoFactory that creates IMigrationInfo objects based
    // on data found in migration input files.  It can create one IMigrationInfo object for each node
    // in the simulation.
    class IDMAPI MigrationInfoFactoryFile : public JsonConfigurable, virtual public IMigrationInfoFactory
    {
    public:
        // for JsonConfigurable stuff...
        GET_SCHEMA_STATIC_WRAPPER(MigrationInfoFactoryFile)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        MigrationInfoFactoryFile();
        virtual ~MigrationInfoFactoryFile();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration* config ) override;

        // IMigrationInfoFactory methods
        virtual void Initialize( const ::Configuration *config, const std::string& idreference ) override;
        virtual bool IsAtLeastOneTypeConfiguredForIndividuals() const override;
        virtual bool IsEnabled( MigrationType::Enum mt ) const override;

        virtual IMigrationInfo* CreateMigrationInfo( INodeContext *parent_node, 
                                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;

        static std::vector<std::vector<MigrationRateData>> GetRateData( INodeContext *parent_node, 
                                                                        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                        std::vector<MigrationInfoFile*>& infoFileList,
                                                                        bool* pIsFixedRate );
    protected:
        virtual void CreateInfoFileList();
        virtual void InitializeInfoFileList( const Configuration* config );

        std::vector<MigrationInfoFile*> m_InfoFileList ;
        bool m_IsHeterogeneityEnabled;
    private:
    };

    // ----------------------------------
    // --- MigrationInfoFactoryDefault
    // ----------------------------------

    // MigrationInfoFactoryDefault is used when the user is running the default/internal scenario.
    // This assumes that there are at least 3-rows and 3-columns of nodes and that the set of nodes is square.
    class IDMAPI MigrationInfoFactoryDefault : public JsonConfigurable, virtual public IMigrationInfoFactory
    {
    public:
        // for JsonConfigurable stuff...
        GET_SCHEMA_STATIC_WRAPPER(MigrationInfoFactoryDefault)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        MigrationInfoFactoryDefault( int torusSize );
        MigrationInfoFactoryDefault();
        virtual ~MigrationInfoFactoryDefault();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration* config ) override;

        // IMigrationInfoFactory methods
        virtual void Initialize( const ::Configuration *config, const std::string& idreference ) override;
        virtual IMigrationInfo* CreateMigrationInfo( INodeContext *parent_node, 
                                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;
        virtual bool IsAtLeastOneTypeConfiguredForIndividuals() const override;
        virtual bool IsEnabled( MigrationType::Enum mt ) const override;

        static std::vector<std::vector<MigrationRateData>> GetRateData( INodeContext *parent_node, 
                                                                        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                        int torus_size,
                                                                        float modifier );
    protected:

        bool  m_IsHeterogeneityEnabled;
        float m_xLocalModifier;
        int   m_TorusSize;
    private:
        void InitializeParameters(); // just used in multiple constructors
    };
}
