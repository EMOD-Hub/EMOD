
#pragma once

#include <string>
#include <map>
#include "VectorGenome.h"
#include "Configure.h"
#include "JsonConfigurableCollection.h"

namespace Kernel
{
    ENUM_DEFINE( VectorGeneDriverType,
        ENUM_VALUE_SPEC( CLASSIC,             0 )
        ENUM_VALUE_SPEC( INTEGRAL_AUTONOMOUS, 1 )
        ENUM_VALUE_SPEC( X_SHRED,             2 )
        ENUM_VALUE_SPEC( Y_SHRED,             3 )
        ENUM_VALUE_SPEC( DAISY_CHAIN,         4 ))

    class VectorGeneCollection;
    class VectorTraitModifiers;

    class CopyToAlleleLikelihood : public JsonConfigurable
    {
    public:
        explicit CopyToAlleleLikelihood( const VectorGeneCollection* pGenes );

        CopyToAlleleLikelihood( const VectorGeneCollection* pGenes,
                                const std::string& rAlleleName,
                                uint8_t alleleIndex,
                                float likelihood );

        virtual ~CopyToAlleleLikelihood();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()

        virtual bool Configure( const Configuration* config ) override;

        const std::string& GetCopyToAlleleName() const;
        uint8_t            GetCopyToAlleleIndex() const;
        float              GetLikelihood() const;

    protected:
        // Protected constructor for derived classes to customize config names
        CopyToAlleleLikelihood( const VectorGeneCollection* pGenes,
                                const char* alleleConfigName,
                                const char* alleleDescText,
                                const char* likelihoodDescText );

        // Protected constructor with initial values
        CopyToAlleleLikelihood( const VectorGeneCollection* pGenes,
                                const std::string& rAlleleName,
                                uint8_t alleleIndex,
                                float likelihood,
                                const char* alleleConfigName,
                                const char* alleleDescText,
                                const char* likelihoodDescText );

        const VectorGeneCollection* m_pGenes;
        std::string m_CopyToAlleleName;
        uint8_t     m_CopyToAlleleIndex;
        float       m_Prob;

    private:
        const char* m_AlleleConfigName;
        const char* m_AlleleDescText;
        const char* m_LikelihoodDescText;

    };


    class CopyToAlleleLikelihoodCollection : public JsonConfigurableCollection<CopyToAlleleLikelihood>
    {
    public:
        CopyToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes );
        ~CopyToAlleleLikelihoodCollection();

        virtual void CheckConfiguration() override;

    protected:
        CopyToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes, 
                                          std::string idmTypeName);
        virtual CopyToAlleleLikelihood* CreateObject() override;

        const VectorGeneCollection* m_pGenes;
    };

    class AlleleDriven : public JsonConfigurable
    {
    public:
        AlleleDriven( const VectorGeneCollection* pGenes );
        AlleleDriven( const VectorGeneCollection* pGenes,
                      uint8_t locusIndex,
                      uint8_t alleleIndexToCopy,
                      uint8_t alleleIndexToReplace );
        AlleleDriven( const AlleleDriven& rMaster );
        ~AlleleDriven();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        // Other
        bool AreTheSame( const AlleleDriven& rthat ) const;
        uint8_t GetLocusIndex() const;
        uint8_t GetAlleleIndexToCopy() const;
        uint8_t GetAlleleIndexToReplace() const;
        const CopyToAlleleLikelihoodCollection& GetCopyToAlleleLikelihoods() const;

        void AddCopyToLikelhood( uint8_t alleleIndex, float likelihood );

        float GetProbabilityOfFailure() const;

    private:
        const VectorGeneCollection* m_pGenes;
        uint8_t m_LocusIndex;
        uint8_t m_AlleleIndexToCopy;
        uint8_t m_AlleleIndexToReplace;
        float   m_ProbabilityOfFailure;
        CopyToAlleleLikelihoodCollection m_CopyToAlleleLikelihoods;
    };


    class AlleleDrivenCollection : public JsonConfigurableCollection<AlleleDriven>
    {
    public:
        AlleleDrivenCollection( const VectorGeneCollection* pGenes );
        ~AlleleDrivenCollection();

        virtual void CheckConfiguration() override;

    protected:
        virtual AlleleDriven* CreateObject() override;

        const VectorGeneCollection* m_pGenes;
    };

    class ShreddingAlleles : public JsonConfigurable
    {
    public:
        ShreddingAlleles( const VectorGeneCollection* pGenes );
        virtual ~ShreddingAlleles();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        AlleleDriven* ConvertToAlleleDriven() const;

        uint8_t GetLocusIndex() const;
        uint8_t GetAlleleIndexRequired() const;
        uint8_t GetAlleleIndexToShred() const;
        uint8_t GetAlleleIndexToShredTo() const;
        float   GetShreddingSurvivingFraction() const;

    protected:
        const VectorGeneCollection* m_pGenes;
        uint8_t m_LocusIndex;
        uint8_t m_AlleleIndexRequired;
        uint8_t m_AlleleIndexToShred;
        uint8_t m_AlleleIndexToShredTo;
        float   m_ShreddingFraction;
        float   m_SurvivingFraction;
    };

    class VectorGeneDriver : public JsonConfigurable
    {
    public:
        VectorGeneDriver( const VectorGeneCollection* pGenes, VectorTraitModifiers* pTraitModifiers );
        ~VectorGeneDriver();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        // Other
        bool CanBeDriven( const VectorGenome& rGenome ) const;
        GenomeProbPairVector_t VectorGeneDriver::DriveGenes( const VectorGenome& rGenome ) const;

        VectorGeneDriverType::Enum GetDriverType() const;
        uint8_t GetDriverLocusIndex() const;
        uint8_t GetDriverAlleleIndex() const;
        int GetNumLociDriven() const;
        int GetNumAlleleToCopy( uint8_t locusIndex ) const;
        const AlleleDriven* GetAlleleDriven( uint8_t locusIndex ) const;

    protected:
        void CheckAllelesDrivenDrivingAllele();
        void ConvertShreddingAlleles( const ShreddingAlleles& rShreddingAlleles );

        GenomeProbPairVector_t ClassicDrive( VectorGenomeGameteIndex::Enum fromGameteIndex,
                                             VectorGenomeGameteIndex::Enum toGameteIndex,
                                             const VectorGenome& rGenome ) const;

        GenomeProbPairVector_t AutonomousDrive( const VectorGenome& rGenome ) const;

    private:
        const VectorGeneCollection* m_pGenes;
        VectorTraitModifiers* m_pTraitModifiers;
        VectorGeneDriverType::Enum m_DriverType;
        uint8_t m_DriverLocusIndex;
        uint8_t m_DriverAlleleIndex;
        AlleleDrivenCollection m_AllelesDriven;
        std::vector<AlleleDriven*> m_AllelesDrivenByLocus; // num genes/loci by num alleles driven for locus
    };

    class VectorGeneDriverCollection : public JsonConfigurableCollection<VectorGeneDriver>
    {
    public:
        VectorGeneDriverCollection( const VectorGeneCollection* pGenes, VectorTraitModifiers* pTraitModifiers );
        virtual ~VectorGeneDriverCollection();

        virtual void CheckConfiguration() override;

        virtual GenomeProbPairVector_t DriveGenes( const VectorGenome& rGenome ) const;

        bool HasDriverAndHeterozygous( const VectorGenome& rGenome ) const;

    protected:
        bool AreAllelesDrivenTheSame( const VectorGeneDriver* pVgdA, const VectorGeneDriver* pVgdB ) const;
        bool DoAllelesDrivenHaveSameLocus( const VectorGeneDriver* pVgdA, const VectorGeneDriver* pVgdB ) const;
        void CheckDriverOverlap( const VectorGeneDriver* pVgdA, const VectorGeneDriver* pVgdB ) const;
        virtual VectorGeneDriver* CreateObject() override;

        const VectorGeneCollection* m_pGenes;
        VectorTraitModifiers* m_pTraitModifiers;
    };
}
