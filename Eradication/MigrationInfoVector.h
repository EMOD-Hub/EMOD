
#pragma once

#include "stdafx.h"
#include "IMigrationInfoVector.h"
#include "Migration.h"
#include "EnumSupport.h"
#include "VectorEnums.h"
#include "SimulationEnums.h"
#include "GeneticProbability.h"

namespace Kernel
{
    struct IVectorSimulationContext;
    class VectorGeneCollection;

    ENUM_DEFINE(ModifierEquationType,
        ENUM_VALUE_SPEC(LINEAR       , 1)
        ENUM_VALUE_SPEC(EXPONENTIAL  , 2))

    // ---------------------------
    // --- MigrationInfoFileVector
    // ---------------------------
    // Need to extend MigrationMetadata to add the ability for allele combinations
    // i.e. migration by genome
    class MigrationMetadataVector : public MigrationMetadata
    {
    public:
        MigrationMetadataVector( int speciesIndex,
                                 const VectorGeneCollection* pGenes,
                                 MigrationType::Enum expectedMigType,
                                 int defaultDestinationsPerNode );
        virtual ~MigrationMetadataVector();

        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual bool Configure( const Configuration* config ) override;

        const std::vector<std::pair<AlleleCombo, int>>& GetAlleleComboMapList() { return m_AlleleCombosIndexMapList; }

    protected:
        virtual void CheckGenderDataType( const Configuration* config ) override { /* don't need to check it */ };
        virtual void ConfigInterpolationType( const Configuration* config ) override { /* don't need to check it */ };
        virtual void ConfigMigrationType( const Configuration* config, MigrationType::Enum& file_migration_type ) override { /* don't need to get it */ };
        virtual void CheckMigrationType( const Configuration* config, const MigrationType::Enum file_migration_type ) override { /* don't need to check it */ };
        virtual void ConfigDatavalueCount( const Configuration* config ) override;
        static bool AlleleComboIntCompare( const std::pair<AlleleCombo, int>& rLeft, const std::pair<AlleleCombo, int>& rRight );

        int m_SpeciesIndex;
        const VectorGeneCollection* m_pGenes;
        std::vector<std::pair<AlleleCombo, int>> m_AlleleCombosIndexMapList;
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

        virtual Gender::Enum              ConvertVectorGender( VectorGender::Enum vector_gender ) const
        {
            return ( vector_gender == VectorGender::VECTOR_FEMALE ? Gender::FEMALE : Gender::MALE );
        };
        const std::vector<float>*         GetFractionTraveling( const IVectorCohort* this_vector )
        {  
            return nullptr;
        };
        virtual bool                      MightTravel( VectorGender::Enum vector_gender ) { return false; }
        virtual bool                      IsMigrationByAlleles() { return false; }


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
        const std::vector<float>*         GetFractionTraveling( const IVectorCohort* this_vector ) override;
        virtual bool                      MightTravel( VectorGender::Enum vector_gender ) override;
        virtual bool                      IsMigrationByAlleles() override;

    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoAgeAndGenderVector( INodeContext* _parent,
                                         ModifierEquationType::Enum equation,
                                         float habitatModifier,
                                         float foodModifier,
                                         float stayPutModifier,
                                         const std::vector<std::pair<AlleleCombo, int>>& rAlleleComboIndexList );

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
        bool                        m_MigrationByAlleles;
        bool                        m_DoingMigration;
    };



    // ----------------------------------
    // --- MigrationInfoFactoryVector
    // ----------------------------------

    class MigrationInfoFactoryVector : public IMigrationInfoFactoryVector
    {
    public:
        MigrationInfoFactoryVector();
        virtual ~MigrationInfoFactoryVector();

        // IMigrationInfoFactoryVector
        virtual void                  ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config ) override;
        virtual IMigrationInfoVector* CreateMigrationInfoVector( const std::string& idreference,
                                                                 INodeContext *pParentNode, 
                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                 int speciesIndex,
                                                                 const VectorGeneCollection* pGenes ) override;

    private:
        MigrationMetadataVector*    m_pMetaData;
        MigrationInfoFile*          m_pInfoFile;
        ModifierEquationType::Enum  m_ModifierEquation;
        float                       m_ModifierHabitat;
        float                       m_ModifierFood;
        float                       m_ModifierStayPut;
        std::string                 m_Filename ;
        float                       m_xModifier ;
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
                                                                 INodeContext *pParentNode, 
                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                 int speciesIndex,
                                                                 const VectorGeneCollection* pGenes ) override;

    protected: 

    private: 
        bool m_IsVectorMigrationEnabled;
        int  m_TorusSize;
    };
}
