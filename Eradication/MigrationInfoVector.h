
#pragma once

#include "stdafx.h"
#include "IMigrationInfoVector.h"
#include "Migration.h"
#include "EnumSupport.h"
#include "VectorEnums.h"
#include "SimulationEnums.h"

namespace Kernel
{
    struct IVectorSimulationContext;

    ENUM_DEFINE(ModifierEquationType,
        ENUM_VALUE_SPEC(LINEAR       , 1)
        ENUM_VALUE_SPEC(EXPONENTIAL  , 2))

     // ---------------------------
     // --- MigrationInfoFileVector
     // ---------------------------

     // MigrationInfoFileVector is responsible for reading the vector-specific migration data from files.
     // This object assumes that for one file name there is one binary file containing the
     // "to-node" and rate data while json-formatted metadata file contains extra information
     // about the data in the binary file. The factory uses this object to create
     // the IMigrationInfo objects.
        class MigrationInfoFileVector : public MigrationInfoFile
    {
    public:
        MigrationInfoFileVector( MigrationType::Enum migType,
                                 int defaultDestinationsPerNode );
        virtual ~MigrationInfoFileVector();

        const std::vector<std::pair<AlleleCombo, int>>& GetAlleleComboMapList() { return m_allele_combos_index_map_list; }
        void SetSpeciesParameters( const VectorSpeciesParameters* pSpeciesParameters ){ m_pSpeciesParameters = pSpeciesParameters; }


    protected:
        // Returns the expected size of the binary file
        virtual uint32_t ParseMetadataForFile( const std::string& data_filepath, const std::string& idreference);

        static bool AlleleComboIntCompare( const std::pair<AlleleCombo, int>& rLeft, const std::pair<AlleleCombo, int>& rRight );
        std::vector<std::pair<AlleleCombo, int>> m_allele_combos_index_map_list;
        const VectorSpeciesParameters* m_pSpeciesParameters;
    };


    // ----------------------------------
    // --- MigrationInfoNullVector
    // ----------------------------------
    class MigrationInfoNullVector : public MigrationInfoNull, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
            DECLARE_QUERY_INTERFACE()

    public:
        //IMigrationInfoVector
        virtual void UpdateRates( const suids::suid& rThisNodeId,
                                  const std::string& rSpeciesID,
                                  IVectorSimulationContext* pivsc ) {};

        virtual Gender::Enum             ConvertVectorGender( VectorGender::Enum vector_gender ) const
        {
            return ( vector_gender == VectorGender::VECTOR_FEMALE ? Gender::FEMALE : Gender::MALE );
        };
        virtual const std::vector<float>& GetFractionTraveling( VectorGender::Enum vector_gender, int by_genome_index ) 
        {  
            static std::vector<float> fraction_traveling;
            return fraction_traveling;
        };
        virtual bool                     CanTravel() { return false; }
        virtual bool                     TravelByAlleles() { return false; }
        virtual int                      GetMigrationDataIndex( int species_index, VectorGenome& rGenome ) { return 0; }




    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoNullVector();
        virtual ~MigrationInfoNullVector();
    };

    // ----------------------------------
    // --- MigrationInfoAgeAndGenderVector
    // ----------------------------------

    class MigrationInfoAgeAndGenderVector : public MigrationInfoAgeAndGender, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoAgeAndGenderVector();

        // IMigrationInfoVector
        virtual void                      UpdateRates( const suids::suid& rThisNodeId,
                                                       const std::string& rSpeciesID,
                                                       IVectorSimulationContext* pivsc ) override;

        virtual Gender::Enum              ConvertVectorGender (VectorGender::Enum gender ) const override;
        virtual void                      CalculateRates( Gender::Enum gender, float ageYears) override;
        virtual float                     GetTotalRate( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<float>& GetCumulativeDistributionFunction( Gender::Enum gender = Gender::MALE ) const override;
        const std::vector<suids::suid>&   GetReachableNodes( Gender::Enum gender = Gender::MALE ) const override;
        const std::vector<float>&         GetFractionTraveling( VectorGender::Enum vector_gender, int by_genome_index ) override;
        virtual bool                      CanTravel() { return true; }
        bool                              TravelByAlleles() { return (m_allele_combos_index_map_list.size() > 0  ?  true: false); }
        virtual int                       GetMigrationDataIndex( int species_index, VectorGenome& rGenome ) override;


    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoAgeAndGenderVector( INodeContext* _parent,
                                         ModifierEquationType::Enum equation,
                                         float habitatModifier,
                                         float foodModifier,
                                         float stayPutModifier );

        virtual void Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData ) override;
        virtual void SaveRawRates( std::vector<float>& r_rate_cdf, Gender::Enum gender )  override;

        float CalculateModifiedRate( const suids::suid& rNodeId,
                                     float rawRate,
                                     float populationRatio,
                                     float habitatRatio);

        typedef std::function<int( const suids::suid& rNodeId,
                                   const std::string& rSpeciesID,
                                   IVectorSimulationContext* pivsc)> tGetValueFunc;

        std::vector<float> GetRatios( const std::vector<suids::suid>& rReachableNodes,
                                      const std::string& rSpeciesID,
                                      IVectorSimulationContext* pivsc,
                                      tGetValueFunc getValueFunc);


    private:
        std::vector<std::pair<AlleleCombo, int>> m_allele_combos_index_map_list;
        std::vector<std::vector<float>> m_RawMigrationRateFemaleByIndex;
        std::vector<std::vector<float>> m_FractionTravelingMaleByIndex;
        std::vector<std::vector<float>> m_FractionTravelingFemaleByIndex;
        std::vector<float>              m_TotalRateFemaleByIndex;
        std::vector<float>              m_TotalRateMaleByIndex;
        std::vector<std::vector<float>> m_RateCDFFemaleByIndex;
        float                       m_TotalRateFemale;
        std::vector<float>          m_RateCDFFemale;
        suids::suid                 m_ThisNodeId;
        ModifierEquationType::Enum  m_ModifierEquation;
        float                       m_ModifierHabitat;
        float                       m_ModifierFood;
        float                       m_ModifierStayPut;
    };



    // ----------------------------------
    // --- MigrationInfoFactoryVector
    // ----------------------------------

    class MigrationInfoFactoryVector : public IMigrationInfoFactoryVector
    {
    public:
        MigrationInfoFactoryVector( bool enableVectorMigration );
        virtual ~MigrationInfoFactoryVector();

        // IMigrationInfoFactoryVector
        virtual void                  ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config ) override;
        virtual IMigrationInfoVector* CreateMigrationInfoVector( const std::string& idreference,
                                                                 INodeContext *parent_node, 
                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                 const VectorSpeciesParameters* m_species_params ) override;

    private:
        MigrationInfoFileVector    m_InfoFileVector;
        ModifierEquationType::Enum m_ModifierEquation;
        float                      m_ModifierHabitat;
        float                      m_ModifierFood;
        float                      m_ModifierStayPut;
    };

    // ----------------------------------
    // --- MigrationInfoFactoryVectorDefault
    // ----------------------------------

    class MigrationInfoFactoryVectorDefault : public IMigrationInfoFactoryVector
    {
    public:
        MigrationInfoFactoryVectorDefault( bool enableVectorMigration, int defaultTorusSize );
        virtual ~MigrationInfoFactoryVectorDefault();

        // IMigrationInfoFactoryVector
        virtual void                  ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config ) {};
        virtual IMigrationInfoVector* CreateMigrationInfoVector( const std::string& idreference,
                                                                 INodeContext *parent_node, 
                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                 const VectorSpeciesParameters* m_species_params ) override;

    protected: 

    private: 
        bool m_IsVectorMigrationEnabled;
        int  m_TorusSize;
    };
}
