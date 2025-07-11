
#pragma once

#include <string>
#include "VectorGenome.h"
#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "IVectorGenomeNames.h"

namespace Kernel
{
    // This is a Germline Mutation where an allele can mutate into another allele
    // during the process of creating gamete cells.
    class VectorAlleleMutation : public JsonConfigurable
    {
    public:
        VectorAlleleMutation( const std::set<std::string>* pAlleleNameSet );
        ~VectorAlleleMutation();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()

        virtual bool Configure( const Configuration* config ) override;

        const std::string& GetAlleleNameFrom() const;
        const std::string& GetAlleleNameTo() const;
        void SetAlleleIndexTo( uint8_t indexTo );
        uint8_t GetAlleleIndexTo() const;
        float   GetFrequency() const;

    private:
        const std::set<std::string>* m_pAlleleNameSet; // used to ensure Mutate_From/To names are valid
        std::string m_MutateFrom;
        std::string m_MutateTo;
        uint8_t m_AlleleIndexTo;
        float   m_Frequency;
    };

    // The DNA that appears at a particular Locus and is typically related
    // to a specific trait.
    class VectorAllele : public JsonConfigurable
    {
    public:
        VectorAllele( uint8_t index );
        VectorAllele( const std::string& rName, uint8_t index, float frequency, bool isMaleAllele=false );
        ~VectorAllele();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()

        virtual bool Configure( const Configuration* config ) override;

        void SetIndex( uint8_t index );

        const std::string& GetName() const;
        uint8_t GetIndex() const;
        float GetFrequency() const;

        void AddMutation( VectorAlleleMutation* pMutation );
        const std::vector<VectorAlleleMutation*>& GetMutations() const;

        bool IsFemale() const;
        bool IsMale() const;

    private:
        std::string m_Name;
        uint8_t m_Index;
        float m_Frequency;
        bool m_IsMaleAllele;
        std::vector<VectorAlleleMutation*> m_Mutations;
    };

    // This class contains the collection of alleles for a particular gene/locus
    class VectorAlleleCollection : public JsonConfigurableCollection<VectorAllele>
    {
    public:
        VectorAlleleCollection();
        virtual ~VectorAlleleCollection();

        virtual void CheckConfiguration() override;

        const std::set<std::string>* GetAlleleNameSet() const;

    protected:
        virtual VectorAllele* CreateObject() override;

        std::set<std::string> m_AlleleNameSet; // to be used with ConstrainedString
    };

    // This class contains the collection of mutations for a particular allele
    class VectorAlleleMutationCollection : public JsonConfigurableCollection<VectorAlleleMutation>
    {
    public:
        VectorAlleleMutationCollection( VectorAlleleCollection& rPossibleAlleles );
        virtual ~VectorAlleleMutationCollection();

        virtual void CheckConfiguration() override;

    protected:
        virtual VectorAlleleMutation* CreateObject() override;

        VectorAlleleCollection& m_rPossibleAlleles;
    };

    // This class contains information about a specific gene.  It knows what 
    // the possible alleles are and their expected frequency.  It also knows
    // about how alleles of the gene mutate into other alleles.  All alleles in
    // the simulation must be pre-defined - this includes mutations.
    class VectorGene : public JsonConfigurable
    {
    public:
        // We treat gender as a "gene" where the X & Y chromosomes are allele
        VectorGene( bool isGenderGene=false );
        ~VectorGene();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        // Other
        void ConfigureGenderAlleles();
        bool IsGenderGene() const;
        void SetLocusIndex( uint8_t locusIndex );
        uint8_t GetLocusIndex() const;
        uint8_t GetNumAllele() const;
        const VectorAllele* GetAllele( uint8_t alleleIndex ) const;
        VectorAllele& GetAllele( const std::string& rName );
        void AddAllele( const std::string& rName, float frequency, bool isMaleAllele );
        bool IsValidAlleleName( const std::string& rName );
        std::string GetPossibleAlleleNames() const;

    private:
        VectorAllele* GetAllelePointer( const std::string& rName );

        bool m_IsGenderGene;
        uint8_t m_LocusIndex;
        VectorAlleleCollection m_PossibleAllele;
    };

    // This collection of genes are all of the possible genes for vectors
    // in the simulation.  No new genes are created on the fly.
    class VectorGeneCollection : public JsonConfigurableCollection<VectorGene>
                               , public IVectorGenomeNames
    {
    public:
        VectorGeneCollection();
        virtual ~VectorGeneCollection();

        virtual void CheckConfiguration() override;

        bool IsValidAlleleName( const std::string& rAlleleName ) const;

        uint8_t GetLocusIndex( const std::string& rAlleleName ) const;
        uint8_t GetAlleleIndex( const std::string& rAlleleName ) const;
        const std::set< std::string >& GetDefinedAlleleNames() const;
        const std::set< std::string >& GetGenderGeneAlleleNames() const;

        void ConvertAlleleCombinationsStrings( const std::string& rParameterName,
                                               const std::vector<std::vector<std::string>>& rComboStrings,
                                               VectorGameteBitPair_t* pBitMask,
                                               std::vector<VectorGameteBitPair_t>* pPossibleGenomes ) const;

        VectorGenome CreateGenome( const std::string& rParameterName,
                                   const std::vector<std::vector<std::string>>& rComboStrings ) const;

        const std::string& GetAlleleName( uint8_t locusIndex, uint8_t alleleIndex ) const;

        // IVectorGenomeNames
        virtual std::string GetGenomeName( const VectorGenome& rGenome ) const override; 
        static constexpr const char* GENE_CONSTRAINTS = "Vector_Species_Params[X].Genes";

    protected:
        virtual VectorGene* CreateObject() override;

        struct AlleleIndexes
        {
            uint8_t locus_index;
            uint8_t allele_index_1;
            uint8_t allele_index_2;

            AlleleIndexes()
                : locus_index(0)
                , allele_index_1(0)
                , allele_index_2(0)
            {
            }
        };

        std::vector<AlleleIndexes> ConvertStringsToIndexes( const std::string& rParameterName,
                                                            int allelePairIndex,
                                                            const std::set<uint8_t>& rLocusIndexesUsed,
                                                            const std::vector<std::string>& rAllelePairStrings ) const;

    private:
        std::set<std::string> m_AlleleNamesDefined;
        std::set<std::string> m_GenderGeneAlleleNames;
        std::map<std::string,VectorGene*> m_AlleleNameToGeneMap;
    };
}
