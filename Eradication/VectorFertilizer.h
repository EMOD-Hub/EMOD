
#pragma once

#include "VectorGenome.h"

namespace Kernel
{
    class RANDOMBASE;
    class VectorGeneCollection;
    class VectorTraitModifiers;
    class VectorGeneDriverCollection;
    class VectorMaternalDepositionCollection;

    struct MatedGenomeCount
    {
        VectorGenome female;
        VectorGenome male;
        uint32_t count;
    };

    struct InitialGenomeData
    {
        std::vector<MatedGenomeCount> mated_females;
        GenomeCountPairVector_t males;
    };

    class VectorFertilizer
    {
    public:
        VectorFertilizer();
        ~VectorFertilizer();

        void Initialize( const VectorGeneCollection* pGenes,
                         const VectorTraitModifiers* pTraitModifiers,
                         const VectorGeneDriverCollection* pGeneDrivers,
                         const VectorMaternalDepositionCollection* pMaternalDepositions );

        InitialGenomeData DetermineInitialGenomeData( RANDOMBASE* pRNG, uint32_t total );

        GenomeCountPairVector_t DetermineInitialGenomes( RANDOMBASE* pRNG, uint32_t total );

        GenomeCountPairVector_t DetermineFertilizedEggs( RANDOMBASE* pRNG,
                                                         const VectorGenome& rFemale,
                                                         const VectorGenome& rMale, 
                                                         uint32_t totalEggs );

        GenomeNamePairVector_t CreateAllPossibleGenomesWithNames() const;

        GenomeProbPairVector_t DriveGenes( const VectorGenome& rGenome ) const;

    protected:
        GenomeProbPairVector_t CreatePossibleGenomes( const GameteProbPairVector_t& rGametesFemale,
                                                      const GameteProbPairVector_t& rGametesMale ) const;

        GenomeCountPairVector_t DetermineNumberOfGenomes( RANDOMBASE* pRNG,
                                                          const GenomeProbPairVector_t& rPossibilities,
                                                          uint32_t total );

        GameteProbPairVector_t CreateInitialGametes( bool femaleOnly, bool useFrequencies ) const;

        GameteProbPairVector_t CreateGametes( float isFemaleModifier,
                                              const GenomeProbPairVector_t& rGPPV );

        GameteProbPairVector_t CreateGametes( float isFemaleModifier,
                                              const VectorGenome& rGenome );

        void GermlineMutation( GameteProbPairVector_t& rGametes );
        void DoMaternalDepostion( const VectorGenome& rMomGenome, GameteProbPairVector_t& rGametes ) const;
        void AdjustForNonFertileEggs( GenomeProbPairVector_t& rPossibilities ) const;


    private:
        const VectorGeneCollection*       m_pGenes;
        const VectorTraitModifiers*       m_pTraitModifiers;
        const VectorGeneDriverCollection* m_pGeneDrivers;
        const VectorMaternalDepositionCollection* m_pMaternalDepositions;
    };
}
