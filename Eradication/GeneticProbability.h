
#pragma once

#include "VectorGenome.h"

namespace Kernel
{
    struct IVectorGenomeNames;

    // AlleleComboInner is the actual memory used for an AlleleCombo.  It is a smart pointer
    // that keeps track of how many references it has and when it doesn't have any more
    // references it deletes itself.  A smart pointer concept was used to reduce the amount
    // of memory thrashing.  The user of these classes wants to think of these as value objects
    // and can't worry about deleting them.
    class AlleleComboInner
    {
    public:
        friend class AlleleCombo;

        AlleleComboInner( int speciesIndex,
                          const VectorGameteBitPair_t& rBitMask,
                          const std::vector<VectorGameteBitPair_t>& rPossibleGenomes );

    protected:
        AlleleComboInner();

        void AddRef();
        void Release();

    private:
        int                                m_SpeciesIndex;
        uint8_t                            m_NumLoci;
        VectorGameteBitPair_t              m_BitMask;
        std::vector<VectorGameteBitPair_t> m_PossibleGenomes;
        int m_RefCount;
    };

    // AlleleCombo represents a subset of all genomes that have a particular subset of alleles.
    // Internally, it is a wrapper around a smart pointer.  Users of AlleleCombo can use it as
    // a value object and don't need to worry about pointers or deleting.
    class AlleleCombo
    {
    public:
        // Create a new object that is a combination of both left and right combos.
        // We want to get the alleles for the loci that we don't share but only the alleles
        // we share of the loci we share.  This will be used with the input combos
        // are not a subset of the other (i.e. when they don't intersect).
        static AlleleCombo Combine( const AlleleCombo& rLeft, const AlleleCombo& rRight );

        // Create a new object that is the portion of the left combo that is common in the right.
        // It is the stuff that is in the left that has common alleles in the right.
        // The subset of the left that is in the right.
        static AlleleCombo CombineSubset( const AlleleCombo& rLeft, const AlleleCombo& rRight );

        AlleleCombo();
        AlleleCombo( int speciesIndex,
                     const VectorGameteBitPair_t& rBitMask,
                     const std::vector<VectorGameteBitPair_t>& rPossibleGenomes );
        AlleleCombo( const AlleleCombo& rMaster );
        ~AlleleCombo();

        AlleleCombo& operator=( const AlleleCombo& rhs );

        bool operator==( const AlleleCombo& rThat ) const;
        bool operator!=( const AlleleCombo& rThat ) const;
        static bool Compare( const AlleleCombo& rLeft, const AlleleCombo& rRight );

        bool IsNull() const;
        int GetSpeciesIndex() const;
        uint8_t GetNumLoci() const;
        VectorGameteBitPair_t GetBitMask() const;
        int GetNumPossibleGenomes() const;
        VectorGameteBitPair_t GetPossibleGenome( uint32_t index ) const;
        bool HasAlleles( int speciesIndex, const VectorGenome& rGenome ) const;
        bool IsSubset( const AlleleCombo& right ) const;

        // needed for testing
        int GetRefCount() const;
        std::string ToString( const IVectorGenomeNames& rGenes, bool debug=false ) const;

        static void serialize( Kernel::IArchive& ar, AlleleCombo& rf );

    private:
        AlleleComboInner* m_pAlleleCombo;
    };

    // AlleleComboProbability is the combination of an AlleleCombo and a single probability value.
    // This is basically the collection of genomes with this specific probability.
    class AlleleComboProbability
    {
    public:
        friend class GeneticProbability;

        AlleleComboProbability( float val = 0.0f );
        AlleleComboProbability( const AlleleCombo& racp, float val = 0.0f );
        AlleleComboProbability( int speciesIndex,
                                const VectorGameteBitPair_t& rBitMask,
                                const std::vector<VectorGameteBitPair_t>& rPossibleGenomes,
                                float value = 0.0 );
        AlleleComboProbability( const AlleleComboProbability& rMaster );
        ~AlleleComboProbability();

        bool operator==( const AlleleComboProbability& rThat ) const;
        bool operator!=( const AlleleComboProbability& rThat ) const;

        AlleleComboProbability& operator=( const AlleleComboProbability& rhs );
        AlleleComboProbability& operator=( float rhs );

        bool IsSubset( const AlleleComboProbability& rhs ) const;
        bool HasAlleles( int speciesIndex, const VectorGenome& rGenome ) const;
        const AlleleCombo& GetAlleleCombo() const;
        float GetValue() const;
        std::string ToString( const IVectorGenomeNames& rGenes ) const;

        static void serialize( Kernel::IArchive& ar, AlleleComboProbability& rf );

    protected:
        AlleleCombo m_AlleleCombo;
        float       m_Value;
    };

    // GeneticProbability represents a a probability for a collection of possible genomes.
    // It allows for mathematical operations to be performed with the probabilities
    // while maintaining the genomes that are associated with these probabilites.
    // For example, an insecticide spray may have a general probability of kill
    // but there could be some vectors with alleles that are resistant to the spray.
    // This probability object can maintain the different values for this probability
    // of kill for the different genomes.
    class GeneticProbability
    {
    public:
        friend GeneticProbability operator-( float lhs, const GeneticProbability& rhs );

        GeneticProbability( float val = 0.0f );
        GeneticProbability( const AlleleComboProbability& racp );
        GeneticProbability( int speciesIndex, const VectorGenome& rGenome, float amount );
        GeneticProbability( const GeneticProbability& rMaster );
        ~GeneticProbability();

        bool operator==( const GeneticProbability& rThat ) const;
        bool operator!=( const GeneticProbability& rThat ) const;

        GeneticProbability& operator=( const GeneticProbability& rhs );
        GeneticProbability& operator=( float rhs );

        GeneticProbability& operator+=( const GeneticProbability& right );
        GeneticProbability& operator*=( const GeneticProbability& right );
        GeneticProbability& operator*=( float right );
        GeneticProbability& operator/=( float right );

        GeneticProbability operator+( const GeneticProbability& right ) const;
        GeneticProbability operator-( const GeneticProbability& right ) const;
        GeneticProbability operator*( const GeneticProbability& right ) const;
        //GeneticProbability operator/( const GeneticProbability& rhs ) const; We shouldn't need this

        GeneticProbability operator+( float rhs ) const;
        GeneticProbability operator-( float rhs ) const;
        GeneticProbability operator*( float rhs ) const;
        GeneticProbability operator/( float rhs ) const;

        explicit operator float() const;

        GeneticProbability expcdf( float dt ) const;

        void Add( const AlleleComboProbability& racp );
        std::vector<AlleleCombo> FindMissingAlleleCombos( const GeneticProbability& rgp ) const;
        float GetValue( int speciesIndex, const VectorGenome& rGenome ) const;
        float GetDefaultValue() const;
        void SetDefaultValue( float value );
        std::string ToString( int speciesIndex, const IVectorGenomeNames& rGenes ) const;

        // This method attempts to provide a single representation of the set of probabilities
        // that are present.  It sums all of the probabilities contributed by each species and
        // genome.  This is a good method to use when wanting to make sure the probability is
        // not zero.  It is also helpful when selecting a strain in ResolveInfectingStrain().
        float GetSum() const;

        int GetNumAlleleComboProbabilities() const; // needed for testing

        static void serialize( Kernel::IArchive& ar, GeneticProbability& rf );

    protected:
        void GeneticMath( GeneticProbability& result,
                          const GeneticProbability& right,
                          float( *MathOperation )(float lhs, float rhs) ) const;
        void ScalarMath( GeneticProbability& result, float rhs,
                         float( *MathOperation )(float lhs, float rhs) ) const;

        // This is needed by the methods where the float is on the left hand side and
        // the GeneticProbability is son the right.
        GeneticProbability RightHandSubtraction( float lhs ) const;

        static bool HasCombo( const std::vector<AlleleComboProbability>& rCombos, const AlleleCombo& rac );
        static bool compareACP( const AlleleComboProbability& rLeft, const AlleleComboProbability& rRight );

        // These operations are passed to GeneticMath and ScalarMath as arguments
        static float AdditionOperation(       float lhs, float rhs );
        static float SubtractionOperation(    float lhs, float rhs );
        static float MultiplicationOperation( float lhs, float rhs );
        static float DivisionOperation(       float lhs, float rhs );

    private:

        // The outer vector/array is automaticall allocated for MAX_SPECIES and is indexed
        // by the VectorSpecies ID.  This allows us to ensure that every GeneticProbability object
        // always have a default value for every species which ensure we can do the math correctly.
        // We also use the VectorSpecies ID to index the outer array.
        // The inner vector/array is a list of AlleleComboProbabilities that must be sorted in
        // ascending order of complexity (i.e. number of loci).  GeneticMath() and GetValue()
        // work on these lists in reverse order - they start with the most complex and work to
        // the lest complex.  NOTE: This keeps us from needing to ensure the AlleleCombos don't
        // intersect..If the most complex applies, use it first.
        std::vector<std::vector<AlleleComboProbability>>*  m_pAlleleCombosPerSpecies;
        float m_DefaultValue;
    };

    // ---------------------------------------------------------------------------------
    // --- Right Hand Side Methods - These methods are used when we have a float on the
    // --- left hand side and a GeneticProbability on the right hand side.
    // ---------------------------------------------------------------------------------
    GeneticProbability operator+( float lhs, const GeneticProbability& rhs );
    GeneticProbability operator-( float lhs, const GeneticProbability& rhs );
    GeneticProbability operator*( float lhs, const GeneticProbability& rhs );
    //GeneticProbability operator/( float lhs, const GeneticProbability& rhs ); we don't need this

}
