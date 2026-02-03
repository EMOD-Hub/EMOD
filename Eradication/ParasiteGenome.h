
#pragma once

#include <string>
#include <vector>
#include <list>
#include "ISerializable.h"
#include "Configure.h"
#include "JsonConfigurableCollection.h"

namespace Kernel
{
    struct IArchive;
    class RANDOMBASE;

    ENUM_DEFINE( GenomeLocationType,
                 ENUM_VALUE_SPEC( BARCODE, 0 )
                 ENUM_VALUE_SPEC( DRUG_RESISTANCE, 1 )
                 ENUM_VALUE_SPEC( HRP, 2 )
                 ENUM_VALUE_SPEC( MSP, 3 )
                 ENUM_VALUE_SPEC( PfEMP1_MAJOR, 4 )
    )

    // This class represents one allele in a ParasiteGenome.  It is intended to be
    // used by classes that need the user to specify a genome location and value.
    // Once a class has this user specified genome location and value, they can
    // then ask the genome if it has this allele.
    class ParasiteGenomeAllele
    {
    public:
        ParasiteGenomeAllele( GenomeLocationType::Enum locType,
                              int32_t location=1,
                              int32_t index=1,
                              int32_t val=0 );
        virtual ~ParasiteGenomeAllele();

        int32_t GetGenomeLocation() const;
        int32_t GetSequenceIndex() const;
        int32_t GetSequenceValue() const;

    private:
        GenomeLocationType::Enum  m_LocationType;
        int32_t m_GenomeLocation;
        int32_t m_SequenceIndex;
        int32_t m_SequenceValue;
    };

    typedef std::vector<ParasiteGenomeAllele> ParasiteGenomeAlleleCollection;

    // This structure is used to define a crossover point between two chromatids
    // during recombination.  The structure assumes that there are a pair of chromatids
    // that come from the female gametocyte and a pair from the male gametocyte.
    struct Crossover
    {
        int32_t genome_location;  // 1 - MAX_LOCATIONS
        int16_t chromatid_female; // 0 or 1 - to indicate which of the sister chromatids
        int16_t chromatid_male;   // 0 or 1 - to indicate which of the sister chromatids
        bool    is_obligate;

        Crossover(int16_t femaleChromatid = 0, int16_t maleChromatid = 0, int32_t loc = 0, bool isObligate = false)
            : genome_location(loc)
            , chromatid_female(femaleChromatid)
            , chromatid_male(maleChromatid)
            , is_obligate(isObligate)
        {}
    };

    // The "inner" class is the class that actually contains the data about the particular
    // genome.  ParasiteGenome wraps one of these objects so that we can do reference counting
    // and limit the number of occurrences.  There can still be multiple occurrences of the same
    // data, but this will because it occurred from recombination.
    class ParasiteGenomeInner : public ISerializable
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        friend class ParasiteGenome;
        friend class ParasiteGenetics;

        static void CalculateHashcodes( ParasiteGenomeInner* pInner );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! I don't really want these public but I want to be able to use them during testing
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        ParasiteGenomeInner();
        ParasiteGenomeInner( const std::vector<int32_t>& rNucleotideSequence );
        ParasiteGenomeInner( const std::vector<int32_t>& rNucleotideSequence, const std::vector<int32_t>& rAlleleRoots );
        ParasiteGenomeInner( const ParasiteGenomeInner* pOldInner, uint32_t infectionID );
        ParasiteGenomeInner( const ParasiteGenomeInner& rMaster );
        ~ParasiteGenomeInner();

        void Initialize( const ParasiteGenomeInner* pInput );

        // Again, I don't want to make these public, but they are needed for testing.
        const std::vector<int32_t>& GetNucleotideSequence() const;
        const std::vector<int32_t>& GetAlleleRoots() const;


    protected:
        // The unique ID of the genome.  Different ID's implies different contents.
        uint32_t m_ID;

        // A hashcode based on the algorithm of Josh Bloch's Effective Java in Item 8 
        // This was intended to be the unique ID but we needed to go to 64-bits to avoid
        // hash collitions.  Hence, we use the hash code to tell if two genomes have the
        // same contents, but we use a seperate ID that is only 32-bits.
        int64_t m_HashCode;

        // A unique ID for the barcode of this genome.  This is different than the hashcode for the genome
        // since it is only the barcode and does not include the allele roots.
        int64_t m_BarcodeHashcode;

        // This is a list of integer values where the meaning of each value depends on its position in the array.
        // Each position in the array is determined by the genome locations defined in ParasiteGenetics.
        // There are Barcode, Drug Resistant, HRP, and Major Var Gene locations.  The sequence assumes that all
        // those locations are sorted into ascending order.
        // NOTE: The minor epitopes (Non-Specific Types) will be randomly determined for each new infection.
        //       This is useful to account for the degree of cross reactive (strain-transcendent) immune
        //       recognition across var gene types. 
        std::vector<int32_t> m_NucleotideSequence;

        // For each value in the Nucleotide Sequence, there will be a parallel value that is the root Infection ID
        // (the originating infection ID from which this allele is inherited).  This is intended to allow for
        // Identity-by-Descent measurements so that ancestry of all remaining genotypes can be traced back to when
        // they initialized in the sim.
        std::vector<int32_t> m_AlleleRoots;

        // Keeping track of genome crossover locations that happened to this genome during its creation.
        std::vector<uint32_t> m_GenomeCrossoverLocations;

        // I think we need to generate "m_nonspectype" when a unique genome is created and then keep it
        // so that we can generate new minor epitopes that are in a similar range/set.
        // This is a number between zero and Falciparum_Nonspecific_Types.  It is used with MINOR_EPITOPE_VARS_PER_SET
        // and MINOR_EPITOPE_VARS_PER_SET to determine the minor epitopoes.
        // EXAMPLE:  m_minor_epitope_type[i] = parent->GetRng()->uniformZeroToN16(MINOR_EPITOPE_VARS_PER_SET) + MINOR_EPITOPE_VARS_PER_SET * m_nonspectype;
        //int32_t m_nonspectype;

        DECLARE_SERIALIZABLE(ParasiteGenomeInner);
    };


    // This class represents the genetic data for the Plasmodium Falciparum parasite when it is
    // in haploid form - a single set of chromosomes.  It is assumed that the only state being
    // modeled where the parasite is diploid (two sets of chromosomes) is when it is in the oocyst
    // state.  When it is in that state, the ParasiteCohort will need to have two instances of
    // this class.
    // NOTE: Instances of this class are immutable.
    // NOTE2: This class implements reference counting to reduce copies of the data and to ensure
    //        that instance get deleted appropriately.
    class ParasiteGenome
    {
    public:
        static void TEST_SetCrossovers( const std::list<Crossover>& rCrossovers );
        static ParasiteGenome TEST_CreateGenome( ParasiteGenomeInner* pInner, bool wrapperOnly );

        // Return the number of active instances of this class on this core.
        static int32_t GetNumActive();

        static void ClearStatics();

        static void Recombination( RANDOMBASE* pRNG,
                                   const ParasiteGenome& rGenomeFemale,
                                   const ParasiteGenome& rGenomeMale,
                                   std::vector<ParasiteGenome>& rNewGenomes );

        // These are public just so I can test them
        static void AddGenome( ParasiteGenomeInner* pInnerOrig,
                               ParasiteGenomeInner* pInnerNew,
                               std::vector<ParasiteGenome>& rNewGenome );

        static void FindCrossovers( RANDOMBASE* pRNG,
                                    uint32_t iChromosome,
                                    std::list<Crossover>& rCrossovers );

        static void IndependentAssortment( RANDOMBASE* pRNG,
                                                           int32_t iChromosome,
                                                           ParasiteGenomeInner* pFemale0,
                                                           ParasiteGenomeInner* pFemale1,
                                                           ParasiteGenomeInner* pMale0,
                                                           ParasiteGenomeInner* pMale1,
                                                           const std::list<Crossover>& rCrossovers );


        ParasiteGenome();
        ParasiteGenome( const ParasiteGenome& rMaster );
        ~ParasiteGenome();

        ParasiteGenome& operator=( const ParasiteGenome& rhs );

        bool operator==( const ParasiteGenome& rThat ) const;
        bool operator!=( const ParasiteGenome& rThat ) const;

        // Return true if this genome is essentially a nullptr / not a valid genome
        bool IsNull() const;

        // Return a unique ID for this genome
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! The challenge with using the 64-bit hash for the ID is that we also use this ID
        // !!! as the GeneticID in the IStrainIdentity interface.  It is a 32-bit integer and
        // !!! changing it would be non-trivial.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        uint32_t GetID() const;

        // Return true if the allele roots have been set for this genome.
        // This is needed because genomes created in OutbreakIndividualMalariaGenetics
        // don't know the the ID of the infection when the genome is created.
        bool HasAlleleRoots() const;

        // Return a human readable representation of this genome
        std::string ToString() const;

        // Return a string representing a human readable version of the locations that represent the "barcode"
        std::string GetBarcode() const;

        // Return a string representing a human readable version of the locations that represent the drug resistant markers
        std::string GetDrugResistantString() const;

        // Return a string representing a human readable version of the locations that represent the HRP markers
        std::string GetHrpString() const;

        // Return true if at least one of the values at the HRP genome locations is 'A' - 'A' is the HRP marker value
        bool HasHrpMarker() const;
        
        // the unique number representing the nucleotide sequence and allele roots.
        // this could be the unique ID of the genome, but want to keep that ID as a 32-bit integer
        int64_t GetHashcode() const;

        // Return a unique ID for the barcode of this genome.  This is different than the hashcode for the genome
        // since it is only the barcode and does not include the allele roots.
        int64_t GetBarcodeHashcode() const;

        // Return the identifier of the merozoite surface protein (MSP)
        // This will be a value between zero and the parameter 'Falciparum_MSP_Variants'
        int32_t GetMSP() const;

        // Return the set of identifiers used by the DTK's Malaria Model 1.0
        // to model the PfEMP1 antigens.
        std::vector<int32_t> GetPfEMP1EpitopesMajor() const; // return must be of length CLONAL_PfEMP1_VARIANTS

        // Return the entire list of values of the genome.  There should be one value for each location defined
        // in the configuration.  This includes barcode SNPs, drug markers, HRP markers, and var genes.
        const std::vector<int32_t>& GetNucleotideSequence() const;

        // Return the entire list of allele roots for each value of the nucleotide sequence.
        // The allele root is the originating infection ID of that value in the genome.
        // This list is parallele to the nucleotide squence and must be the same length.
        const std::vector<int32_t>& GetAlleleRoots() const;

        // return the list of genome crossover locations that happened during the creation of this genome.
        const std::vector<uint32_t>& GetCrossovers() const;

        // Return true if this genome has this allele
        bool HasAllele( const ParasiteGenomeAllele& rAllele ) const;

        // Return true if this genome has all of the alleles defined in the collection
        bool HasAllOfTheAlleles( const ParasiteGenomeAlleleCollection& rAlleleCollection ) const;


        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // FPG-TODO - We will probably need to add something for drug resistance and HPR.
        //            I just don't know what the using code needs right now.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        static void serialize( IArchive& ar, ParasiteGenome& pg );


    protected:
        friend class ParasiteGenetics;

        ParasiteGenome( const std::vector<int32_t>& rNucleotideSequence );
        ParasiteGenome( const std::vector<int32_t>& rNucleotideSequence, const std::vector<int32_t>& rRoots );
        ParasiteGenome( const ParasiteGenome& rGenome, uint32_t infectionID );
        ParasiteGenome( ParasiteGenomeInner* pInner, bool wrapperOnly );

        std::string ConvertToString( const std::vector<int32_t>& rIndexes ) const;


        ParasiteGenomeInner* m_pInner;


        static void LogBarcodes( const char* name,
                                 int iChromosome,
                                 ParasiteGenomeInner* pChild0,
                                 ParasiteGenomeInner* pChild1,
                                 ParasiteGenomeInner* pChild2,
                                 ParasiteGenomeInner* pChild3 );

        static std::vector<int32_t> STATIC_SWAP_ns;
        static std::vector<int32_t> STATIC_SWAP_ar;
        static bool                 STATIC_SWAP_Initialized;
        
        static std::list<Crossover> STATIC_TEST_CROSSOVERS;

        static void TrackCrossoversInGenomes( const std::list<Crossover>& rCrossovers,
                                              const std::vector<int32_t>& rNewOrder,
                                              ParasiteGenomeInner* pFemale0,
                                              ParasiteGenomeInner* pFemale1,
                                              ParasiteGenomeInner* pMale0,
                                              ParasiteGenomeInner* pMale1 );

        static void InitializeStaticSwap();

        static void Swap( int32_t indexFirst,
                          int32_t indexLast,
                          ParasiteGenomeInner* pInner1,
                          ParasiteGenomeInner* pInner2 );

        static void Swap( int32_t indexFirst,
                          int32_t indexLast,
                          ParasiteGenomeInner* pInner1,
                          ParasiteGenomeInner* pInner2,
                          ParasiteGenomeInner* pInner3 );

        static void Swap( int32_t indexFirst,
                          int32_t indexLast,
                          const std::vector<int32_t>&rInnerListIndexes,
                          ParasiteGenomeInner* pInnerList[] );
    };
}
