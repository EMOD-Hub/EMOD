
#include "stdafx.h"
#include "GeneticProbability.h"

#include <bitset>

#include "Common.h"
#include "Debug.h"
#include "IVectorGenomeNames.h"
#include "VectorSpeciesParameters.h" //MAX_SPECIES

// The maximum number of AlleleCombos per species in m_AlleleCombosPerSpeices.
// This is used to allocate static memory that is used in the GeneticMath() method.
#define MAX_ALLELE_COMBOS (1000)

#define ALL_BITS (0xFFFFFFFFFFFFFFFF)

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- AlleleComboInner
    // ------------------------------------------------------------------------

    AlleleComboInner::AlleleComboInner()
        : m_SpeciesIndex(-1)
        , m_NumLoci(0)
        , m_BitMask(0)
        , m_PossibleGenomes()
        , m_RefCount(0)
    {
    }

    AlleleComboInner::AlleleComboInner( int speciesIndex,
                                        const VectorGameteBitPair_t& rBitMask,
                                        const std::vector<VectorGameteBitPair_t>& rPossibleGenomes )
        : m_SpeciesIndex( speciesIndex )
        , m_NumLoci(0)
        , m_BitMask( rBitMask )
        , m_PossibleGenomes( rPossibleGenomes )
        , m_RefCount(0)
    {
        // Some of the logic in checking of subset and combining depends on this list being sorted.
        std::sort( m_PossibleGenomes.begin(), m_PossibleGenomes.end() );

        // ---------------------------------------------------------------------------
        // --- Determine the number of loci involved in a gamete.
        // --- This will be used in sorting the combos in order of genetic complexity.
        // ---------------------------------------------------------------------------
        std::pair<VectorGamete, VectorGamete> vg_pair = VectorGamete::Convert( m_BitMask );
        VectorGameteBits_t bits_for_half = vg_pair.first.GetBits();

        for( uint8_t locus_index = 0; locus_index < VectorGamete::MAX_LOCI; ++locus_index )
        {
            VectorGameteBits_t bits_for_locus = bits_for_half & VectorGamete::ALLELE_BITS[ locus_index ];
            if( bits_for_locus > 0 )
            {
                ++m_NumLoci;
            }
        }
    }

    void AlleleComboInner::AddRef()
    {
        ++m_RefCount;
    }

    void AlleleComboInner::Release()
    {
        --m_RefCount;
        if( m_RefCount == 0 )
        {
            delete this;
        }
        else
        {
            release_assert( m_RefCount > 0 );
        }
    }

    // ------------------------------------------------------------------------
    // --- AlleleCombo
    // ------------------------------------------------------------------------

    AlleleCombo AlleleCombo::Combine( const AlleleCombo& rLeft, const AlleleCombo& rRight )
    {
        // ---------------------------------------------------------------------
        // --- We will use the common bits to only keep the genomes that have
        // --- common alleles between them.  Notice that zero common bits will
        // --- result in selecting all of the genomes.
        // ---------------------------------------------------------------------
        VectorGameteBitPair_t common_bit_mask =  rLeft.m_pAlleleCombo->m_BitMask
                                              & rRight.m_pAlleleCombo->m_BitMask;

        std::vector<VectorGameteBitPair_t> new_possible_genomes;
        new_possible_genomes.reserve( rLeft.m_pAlleleCombo->m_PossibleGenomes.size() );
        for( auto left_pg : rLeft.m_pAlleleCombo->m_PossibleGenomes )
        {
            VectorGameteBitPair_t left_common_bits = common_bit_mask & left_pg;
            for( auto right_pg : rRight.m_pAlleleCombo->m_PossibleGenomes )
            {
                VectorGameteBitPair_t right_common_bits = common_bit_mask & right_pg;
                if( left_common_bits == right_common_bits )
                {
                    // 'or' to get collective set of bits.
                    VectorGameteBitPair_t new_pg = left_pg | right_pg;
                    new_possible_genomes.push_back( new_pg );
                }
            }
        }

        if( new_possible_genomes.size() == 0 )
        {
            // return a null combo if the genomes do not interect
            return AlleleCombo();
        }

        // ----------------------------------------------------------------------------------------
        // --- I thought about using set to help with the uniqueness but this post
        // --- https://stackoverflow.com/questions/1041620/whats-the-most-efficient-way-to-erase-duplicates-and-sort-a-vector
        // --- makes the vector solution look best when the number of duplicates is small
        // ----------------------------------------------------------------------------------------
        std::sort( new_possible_genomes.begin(), new_possible_genomes.end() );
        new_possible_genomes.erase( std::unique( new_possible_genomes.begin(), new_possible_genomes.end() ), new_possible_genomes.end() );

        // -----------------------------------------------------------------
        // --- Since they didn't share the same bits, we want to 'or' them
        // --- to get the collective set of bits.
        // -----------------------------------------------------------------
        VectorGameteBitPair_t new_bit_mask =  rLeft.m_pAlleleCombo->m_BitMask 
                                           | rRight.m_pAlleleCombo->m_BitMask;

        AlleleCombo new_combo( rLeft.GetSpeciesIndex(), new_bit_mask, new_possible_genomes );
        return new_combo;
    }

    AlleleCombo AlleleCombo::CombineSubset( const AlleleCombo& rLeft, const AlleleCombo& rRight )
    {
        VectorGameteBitPair_t common_bit_mask =  rLeft.m_pAlleleCombo->m_BitMask 
                                              & rRight.m_pAlleleCombo->m_BitMask;


        std::vector<VectorGameteBitPair_t> new_possible_genomes;
        new_possible_genomes.reserve( rLeft.m_pAlleleCombo->m_PossibleGenomes.size() );
        for( auto left_pg : rLeft.m_pAlleleCombo->m_PossibleGenomes )
        {
            VectorGameteBitPair_t left_common_bits = common_bit_mask & left_pg;
            for( auto right_pg : rRight.m_pAlleleCombo->m_PossibleGenomes )
            {
                VectorGameteBitPair_t right_common_bits = common_bit_mask & right_pg;
                if( left_common_bits == right_common_bits )
                {
                    // only keep the Possible Genomes in the left that have common bits in the right.
                    new_possible_genomes.push_back( left_pg );
                    break;
                }
            }
        }
        release_assert( new_possible_genomes.size() > 0 );

        // ---------------------------
        // --- See note in Combine()
        // ---------------------------
        std::sort( new_possible_genomes.begin(), new_possible_genomes.end() );
        new_possible_genomes.erase( std::unique( new_possible_genomes.begin(), new_possible_genomes.end() ), new_possible_genomes.end() );

        AlleleCombo new_combo( rLeft.GetSpeciesIndex(),  rLeft.m_pAlleleCombo->m_BitMask, new_possible_genomes );
        return new_combo;
    }

    AlleleCombo::AlleleCombo()
        : m_pAlleleCombo( nullptr )
    {
    }

    AlleleCombo::AlleleCombo( int speciesIndex,
                              const VectorGameteBitPair_t& rBitMask,
                              const std::vector<VectorGameteBitPair_t>& rPossibleGenomes )
        : m_pAlleleCombo( new AlleleComboInner( speciesIndex, rBitMask, rPossibleGenomes ) )
    {
        m_pAlleleCombo->AddRef();
    }

    AlleleCombo::AlleleCombo( const AlleleCombo& rMaster )
        : m_pAlleleCombo( rMaster.m_pAlleleCombo )
    {
        if( m_pAlleleCombo != nullptr )
        {
            m_pAlleleCombo->AddRef();
        }
    }

    AlleleCombo::~AlleleCombo()
    {
        if( m_pAlleleCombo != nullptr )
        {
            m_pAlleleCombo->Release();
        }
    }

    AlleleCombo& AlleleCombo::operator=( const AlleleCombo& rhs )
    {
        if( this->m_pAlleleCombo != rhs.m_pAlleleCombo )
        {
            if( this->m_pAlleleCombo != nullptr )
            { 
                this->m_pAlleleCombo->Release();
            }

            this->m_pAlleleCombo = rhs.m_pAlleleCombo;

            if( this->m_pAlleleCombo != nullptr )
            {
                this->m_pAlleleCombo->AddRef();
            }
        }
        return *this;
    }

    bool AlleleCombo::operator==( const AlleleCombo& rThat ) const
    {
        if( this->m_pAlleleCombo == rThat.m_pAlleleCombo ) return true;

        if( (this->m_pAlleleCombo == nullptr) && (rThat.m_pAlleleCombo != nullptr) ) return false;
        if( (this->m_pAlleleCombo != nullptr) && (rThat.m_pAlleleCombo == nullptr) ) return false;
        if( (this->m_pAlleleCombo != nullptr) && (rThat.m_pAlleleCombo != nullptr) )
        {
            if( this->m_pAlleleCombo->m_SpeciesIndex != rThat.m_pAlleleCombo->m_SpeciesIndex ) return false;
            if( this->m_pAlleleCombo->m_BitMask != rThat.m_pAlleleCombo->m_BitMask ) return false;
            if( this->m_pAlleleCombo->m_NumLoci != rThat.m_pAlleleCombo->m_NumLoci ) return false;

            if( this->m_pAlleleCombo->m_PossibleGenomes.size() != rThat.m_pAlleleCombo->m_PossibleGenomes.size() ) return false;
            for( int i = 0; i < this->m_pAlleleCombo->m_PossibleGenomes.size(); ++i )
            {
                if( this->m_pAlleleCombo->m_PossibleGenomes[ i ] != rThat.m_pAlleleCombo->m_PossibleGenomes[ i ] ) return false;
            }
        }
        return true;
    }

    bool AlleleCombo::operator!=( const AlleleCombo& rThat ) const
    {
        return !operator==( rThat );
    }

    bool AlleleCombo::Compare( const AlleleCombo& rLeft, const AlleleCombo& rRight )
    {
        if( rLeft.GetNumLoci() < rRight.GetNumLoci() )
        {
            return true;
        }
        else if( rLeft.GetNumLoci() > rRight.GetNumLoci() )
        {
            return false;
        }
        else
        {
            if( rLeft.GetNumPossibleGenomes() == rRight.GetNumPossibleGenomes() )
            {
                // If they have the same number of possible genomes, we need some way to sort them so that the order
                // is more consistent.  For example, we always want (a1,*) to come after (a0,*).
                return ( rLeft.GetPossibleGenome( 0 ) < rRight.GetPossibleGenome( 0 ) );
            }
            else
            {
                // more possible genomes means more options so less specificity
                return ( rLeft.GetNumPossibleGenomes() > rRight.GetNumPossibleGenomes() );
            }
        }
    }

    bool AlleleCombo::IsNull() const
    {
        return (m_pAlleleCombo == nullptr);
    }

    int AlleleCombo::GetSpeciesIndex() const
    {
        release_assert( m_pAlleleCombo != nullptr );
        return m_pAlleleCombo->m_SpeciesIndex;
    }

    uint8_t AlleleCombo::GetNumLoci() const
    {
        if( m_pAlleleCombo == nullptr )
            return 0;
        else
            return m_pAlleleCombo->m_NumLoci;
    }

    VectorGameteBitPair_t AlleleCombo::GetBitMask() const
    {
        if( m_pAlleleCombo == nullptr )
            return 0;
        else
            return m_pAlleleCombo->m_BitMask;
    }

    int AlleleCombo::GetNumPossibleGenomes() const
    {
        if( m_pAlleleCombo == nullptr )
            return 0;
        else
            return m_pAlleleCombo->m_PossibleGenomes.size();
    }

    VectorGameteBitPair_t AlleleCombo::GetPossibleGenome( uint32_t index ) const
    {
        if( m_pAlleleCombo == nullptr )
            return 0;
        else
            return m_pAlleleCombo->m_PossibleGenomes[ index ];
    }

    bool AlleleCombo::HasAlleles( int speciesIndex, const VectorGenome& rGenome ) const
    {
        if( m_pAlleleCombo == nullptr )
        {
            return true;
        }

        if( speciesIndex != m_pAlleleCombo->m_SpeciesIndex )
        {
            return false;
        }

        VectorGameteBitPair_t genome_bits = m_pAlleleCombo->m_BitMask & rGenome.GetBits();

        for( auto possible_bits : m_pAlleleCombo->m_PossibleGenomes )
        {
            if( genome_bits == possible_bits )
            {
                return true;
            }
        }
        return false;
    }

    bool AlleleCombo::IsSubset( const AlleleCombo& right ) const
    {
        if( right.m_pAlleleCombo == nullptr )
            return true;

        if( this->m_pAlleleCombo == nullptr )
            return false;

        // -----------------------------------------------------------------
        // --- If we 'or' the bits together and the result has the same bits
        // --- as the left, then the right has the same loci as the left.
        // --- If the right has all the bits set in the mask, then we want to
        // --- check the possible combos.
        // -----------------------------------------------------------------
        VectorGameteBitPair_t ored_bits = this->m_pAlleleCombo->m_BitMask
                                        | right.m_pAlleleCombo->m_BitMask;
        if( (ored_bits != this->m_pAlleleCombo->m_BitMask) &&
            (ALL_BITS  != right.m_pAlleleCombo->m_BitMask) )
        {
            return false;
        }

        // ----------------------------------------------------------------------
        // --- Now we determine the bits of the common loci and then determine if
        // --- all of the right possible genomes are in the left.
        // ----------------------------------------------------------------------
        VectorGameteBitPair_t common_loci_bits = this->m_pAlleleCombo->m_BitMask
                                               & right.m_pAlleleCombo->m_BitMask;

        for( auto right_bits : right.m_pAlleleCombo->m_PossibleGenomes )
        {
            auto right_common_bits = common_loci_bits & right_bits;

            bool found_right_bits_in_left = false;
            for( auto left_bits :this->m_pAlleleCombo->m_PossibleGenomes )
            {
                auto left_common_bits = common_loci_bits & left_bits;
                if( right_common_bits == left_common_bits )
                {
                    found_right_bits_in_left = true;
                    break;
                }
            }
            if( !found_right_bits_in_left )
            {
                return false;
            }
        }
        return true;
    }

    // needed for testing
    int AlleleCombo::GetRefCount() const
    {
        if( m_pAlleleCombo == nullptr )
        {
            return -1;
        }
        else
        {
            return m_pAlleleCombo->m_RefCount;
        }
    }

    // needed for testing
    std::string AlleleCombo::ToString( const IVectorGenomeNames& rGenes, bool debug ) const
    {
        std::stringstream ss;
        if( m_pAlleleCombo == nullptr )
        {
            ss << "Default";
        }
        else
        {
            if( debug )
            {
                std::bitset<64> bs = m_pAlleleCombo->m_BitMask;
                ss << bs.to_string() << "=BitMask,\n";
            }

            for( int i = 0; i < m_pAlleleCombo->m_PossibleGenomes.size(); ++i )
            {
                if( debug )
                {
                    std::bitset<64> bs = m_pAlleleCombo->m_PossibleGenomes[ i ];
                    ss << bs.to_string() << "=";
                }
                ss << rGenes.GetGenomeName( VectorGenome( m_pAlleleCombo->m_PossibleGenomes[ i ] ) );
                if( i < (m_pAlleleCombo->m_PossibleGenomes.size() - 1) )
                {
                    ss << ", ";
                }
                ss << "\n";
            }
        }
        return ss.str();
    }


    void AlleleCombo::serialize( Kernel::IArchive& ar, AlleleCombo& rac )
    {
        ar.startObject();

        int species_index = -1;
        if( ar.IsWriter() && (rac.m_pAlleleCombo != nullptr) )
        {
            species_index = rac.m_pAlleleCombo->m_SpeciesIndex;
        }
        ar.labelElement( "m_SpeciesIndex" ) & species_index;

        if( ar.IsReader() )
        {
            release_assert( rac.m_pAlleleCombo == nullptr );
            if( species_index == -1 )
            {
                // we want rac.m_pAlleleCombo to stay nullptr
            }
            else
            {
                rac.m_pAlleleCombo = new AlleleComboInner();
                rac.m_pAlleleCombo->m_RefCount = 1;
                rac.m_pAlleleCombo->m_SpeciesIndex = species_index;
            }
        }

        if( rac.m_pAlleleCombo != nullptr )
        {
            ar.labelElement( "m_NumLoci"         ) & rac.m_pAlleleCombo->m_NumLoci;
            ar.labelElement( "m_BitMask"         ) & rac.m_pAlleleCombo->m_BitMask;
            ar.labelElement( "m_PossibleGenomes" ) & rac.m_pAlleleCombo->m_PossibleGenomes;
        }
        ar.endObject();
    }

    // ------------------------------------------------------------------------
    // --- AlleleComboProbability
    // ------------------------------------------------------------------------

    AlleleComboProbability::AlleleComboProbability( float val )
        : m_AlleleCombo()
        , m_Value( val )
    {
    }

    AlleleComboProbability::AlleleComboProbability( const AlleleCombo& racp, float val )
        : m_AlleleCombo( racp )
        , m_Value( val )
    {
    }

    AlleleComboProbability::AlleleComboProbability( int speciesIndex,
                                                    const VectorGameteBitPair_t& rBitMask,
                                                    const std::vector<VectorGameteBitPair_t>& rPossibleGenomes,
                                                    float value )
        : AlleleComboProbability( AlleleCombo( speciesIndex, rBitMask, rPossibleGenomes ), value )
    {
    }

    AlleleComboProbability::AlleleComboProbability( const AlleleComboProbability& rMaster )
        : m_AlleleCombo( rMaster.m_AlleleCombo )
        , m_Value( rMaster.m_Value )
    {
    }

    AlleleComboProbability::~AlleleComboProbability()
    {
    }

    bool AlleleComboProbability::operator==( const AlleleComboProbability& rThat ) const
    {
        if( this->m_AlleleCombo != rThat.m_AlleleCombo ) return false;

        if( fabs( this->m_Value - rThat.m_Value ) > FLT_EPSILON ) return false;

        return true;
    }

    bool AlleleComboProbability::operator!=( const AlleleComboProbability& rThat ) const
    {
        return !(this->operator==( rThat ));
    }

    AlleleComboProbability& AlleleComboProbability::operator=( const AlleleComboProbability& rhs )
    {
        if( this != &rhs )
        {
            this->m_AlleleCombo = rhs.m_AlleleCombo;
            this->m_Value = rhs.m_Value;
        }
        return *this;
    }

    AlleleComboProbability& AlleleComboProbability::operator=( float rhs )
    {
        if( rhs < 0.0 )
        {
            rhs = 0.0;
        }
        else if( 1.0 < rhs )
        {
            rhs = 1.0;
        }

        m_Value = rhs;

        return *this;
    }

    bool AlleleComboProbability::IsSubset( const AlleleComboProbability& rhs ) const
    {
        return m_AlleleCombo.IsSubset( rhs.m_AlleleCombo );
    }

    bool AlleleComboProbability::HasAlleles( int speciesIndex, const VectorGenome& rGenome ) const
    {
        if( m_AlleleCombo.IsNull() )
            return true;

        return m_AlleleCombo.HasAlleles( speciesIndex, rGenome );
    }

    const AlleleCombo& AlleleComboProbability::GetAlleleCombo() const
    {
        return m_AlleleCombo;
    }

    float AlleleComboProbability::GetValue() const
    {
        return m_Value;
    }

    std::string AlleleComboProbability::ToString( const IVectorGenomeNames& rGenes ) const
    {
        std::stringstream ss;
        ss << m_AlleleCombo.ToString( rGenes ) << " = " << m_Value;

        return ss.str();
    }

    void AlleleComboProbability::serialize( Kernel::IArchive& ar, AlleleComboProbability& racp )
    {
        ar.startObject();
        ar.labelElement( "m_AlleleCombo" ) & racp.m_AlleleCombo;
        ar.labelElement( "m_Value"       ) & racp.m_Value;
        ar.endObject();
    }

    // ------------------------------------------------------------------------
    // --- GeneticProbability
    // ------------------------------------------------------------------------

    GeneticProbability::GeneticProbability( float val )
        : m_pAlleleCombosPerSpecies(nullptr)
        , m_DefaultValue( val )
    {
    }

    GeneticProbability::GeneticProbability( const AlleleComboProbability& racp )
        : GeneticProbability()
    {
        Add( racp );
    }

    GeneticProbability::GeneticProbability( int speciesIndex, const VectorGenome& rGenome, float amount )
        : GeneticProbability(0.0f)
    {
        std::vector<VectorGameteBitPair_t> genomes;
        genomes.push_back( rGenome.GetBits() );
        AlleleCombo ac( speciesIndex, ALL_BITS, genomes );
        AlleleComboProbability acp( ac, amount );
        Add( acp );
    }

    GeneticProbability::GeneticProbability( const GeneticProbability& rMaster )
        : m_pAlleleCombosPerSpecies( nullptr )
        , m_DefaultValue( rMaster.m_DefaultValue )
    {
        if( rMaster.m_pAlleleCombosPerSpecies != nullptr )
        {
            m_pAlleleCombosPerSpecies = new std::vector<std::vector<AlleleComboProbability>>( *(rMaster.m_pAlleleCombosPerSpecies) );
        }
    }

    GeneticProbability::~GeneticProbability()
    {
        delete m_pAlleleCombosPerSpecies;
    }

    bool GeneticProbability::operator==( const GeneticProbability& rThat ) const
    {
        if( this->m_DefaultValue != rThat.m_DefaultValue ) return false;

        if( (this->m_pAlleleCombosPerSpecies != nullptr) && (rThat.m_pAlleleCombosPerSpecies == nullptr) ) return false;
        if( (this->m_pAlleleCombosPerSpecies == nullptr) && (rThat.m_pAlleleCombosPerSpecies != nullptr) ) return false;

        if( (this->m_pAlleleCombosPerSpecies != nullptr) && (rThat.m_pAlleleCombosPerSpecies != nullptr) )
        {
            const std::vector<std::vector<AlleleComboProbability>>& r_this = *(this->m_pAlleleCombosPerSpecies);
            const std::vector<std::vector<AlleleComboProbability>>& r_that = *(rThat.m_pAlleleCombosPerSpecies);

            if( r_this.size() != r_that.size() ) return false;
            for( int i = 0; i < r_this.size(); ++i )
            {
                if( r_this[i].size() != r_that[i].size() ) return false;
                for( int j = 0; j < r_this[ i ].size(); ++j )
                {
                    const AlleleComboProbability& r_this_acp = r_this[ i ][ j ];
                    const AlleleComboProbability& r_that_acp = r_that[ i ][ j ];
                    if( r_this_acp != r_that_acp ) return false;
                }
            }
        }
        return true;
    }

    bool GeneticProbability::operator!=( const GeneticProbability& rThat ) const
    {
        return !(this->operator==( rThat ));
    }

    GeneticProbability& GeneticProbability::operator=( const GeneticProbability& rhs )
    {
        if( this != &rhs )
        {
            if( (this->m_pAlleleCombosPerSpecies != nullptr) && (rhs.m_pAlleleCombosPerSpecies == nullptr) )
            {
                delete m_pAlleleCombosPerSpecies;
                m_pAlleleCombosPerSpecies = nullptr;
            }
            else if( (this->m_pAlleleCombosPerSpecies == nullptr) && (rhs.m_pAlleleCombosPerSpecies != nullptr) )
            {
                m_pAlleleCombosPerSpecies = new std::vector<std::vector<AlleleComboProbability>>( *rhs.m_pAlleleCombosPerSpecies );
            }
            else if( (this->m_pAlleleCombosPerSpecies != nullptr) && (rhs.m_pAlleleCombosPerSpecies != nullptr) )
            {
                *m_pAlleleCombosPerSpecies = *(rhs.m_pAlleleCombosPerSpecies);
            }
            m_DefaultValue = rhs.m_DefaultValue;
        }
        return *this;
    }

    GeneticProbability& GeneticProbability::operator=( float rhs )
    {
        m_DefaultValue = rhs;
        if( m_pAlleleCombosPerSpecies != nullptr )
        {
            for( auto& r_species_combos : *m_pAlleleCombosPerSpecies )
            {
                for( auto& racp : r_species_combos )
                {
                    racp = rhs;
                }
            }
        }
        return *this;
    }

    GeneticProbability& GeneticProbability::operator+=( const GeneticProbability& right )
    {
        if( (this->m_pAlleleCombosPerSpecies == nullptr) && (right.m_pAlleleCombosPerSpecies == nullptr) )
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! This may seem unnessary to what is in GeneticMath (ScalarMath), but for the
            // !!! case where a user is not using insecticides, this is significantly faster (+10%)
            // !!! This reduces the creating and deletion of GeneticProbability objects and reduces
            // !!! the time required to follow the function pointer to the MathOperation.
            // !!! They seem small but makes a big difference in the math in VectorInterventionsContainer
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            this->m_DefaultValue += right.m_DefaultValue;
        }
        else
        {
            GeneticMath( *this, right, &AdditionOperation );
        }
        return *this;
    }

    GeneticProbability& GeneticProbability::operator*=( const GeneticProbability& right )
    {
        if( (this->m_pAlleleCombosPerSpecies == nullptr) && (right.m_pAlleleCombosPerSpecies == nullptr) )
        {
            // See GeneticProbability::operator+=
            this->m_DefaultValue *= right.m_DefaultValue;
        }
        else
        {
            GeneticMath( *this, right, &MultiplicationOperation );
        }
        return *this;
    }

    GeneticProbability& GeneticProbability::operator*=( float right )
    {
        // See GeneticProbability::operator+=
        this->m_DefaultValue *= right;

        if( this->m_pAlleleCombosPerSpecies != nullptr )
        {
            ScalarMath( *this, right, &MultiplicationOperation );
        }
        return *this;
    }

    GeneticProbability& GeneticProbability::operator/=( float right )
    {
        // See GeneticProbability::operator+=
        this->m_DefaultValue /= right;

        if( this->m_pAlleleCombosPerSpecies != nullptr )
        {
            ScalarMath( *this, right, &DivisionOperation );
        }
        return *this;
    }

    GeneticProbability GeneticProbability::operator+( const GeneticProbability& right ) const
    {
        GeneticProbability result = *this;
        if( (result.m_pAlleleCombosPerSpecies == nullptr) && (right.m_pAlleleCombosPerSpecies == nullptr) )
        {
            // See GeneticProbability::operator+=
            result.m_DefaultValue += right.m_DefaultValue;
        }
        else
        {
            GeneticMath( result, right, &AdditionOperation );
        }
        return result;
    }

    GeneticProbability GeneticProbability::operator-( const GeneticProbability& right ) const
    {
        GeneticProbability result = *this;
        if( (result.m_pAlleleCombosPerSpecies == nullptr) && (right.m_pAlleleCombosPerSpecies == nullptr) )
        {
            // See GeneticProbability::operator+=
            result.m_DefaultValue -= right.m_DefaultValue;
        }
        else
        {
            GeneticMath( result, right, &SubtractionOperation );
        }
        return result;
    }

    GeneticProbability GeneticProbability::operator*( const GeneticProbability& right ) const
    {
        GeneticProbability result = *this;
        if( (result.m_pAlleleCombosPerSpecies == nullptr) && (right.m_pAlleleCombosPerSpecies == nullptr) )
        {
            // See GeneticProbability::operator+=
            result.m_DefaultValue *= right.m_DefaultValue;
        }
        else
        {
            GeneticMath( result, right, &MultiplicationOperation );
        }
        return result;
    }

    // Don't need this
    //GeneticProbability GeneticProbability::operator/( const GeneticProbability& rhs ) const;

    GeneticProbability GeneticProbability::operator+( float rhs ) const
    {
        GeneticProbability result = *this;

        // See GeneticProbability::operator+=
        result.m_DefaultValue += rhs;

        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            ScalarMath( result, rhs, &AdditionOperation );
        }
        return result;
    }

    GeneticProbability GeneticProbability::operator-( float rhs ) const
    {
        GeneticProbability result = *this;

        // See GeneticProbability::operator+=
        result.m_DefaultValue -= rhs;

        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            ScalarMath( result, rhs, &SubtractionOperation );
        }
        return result;
    }

    GeneticProbability GeneticProbability::operator*( float rhs ) const
    {
        GeneticProbability result = *this;

        // See GeneticProbability::operator+=
        result.m_DefaultValue *= rhs;

        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            ScalarMath( result, rhs, &MultiplicationOperation );
        }
        return result;
    }

    GeneticProbability GeneticProbability::operator/( float rhs ) const
    {
        GeneticProbability result = *this;

        // See GeneticProbability::operator+=
        result.m_DefaultValue /= rhs;

        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            ScalarMath( result, rhs, &DivisionOperation );
        }
        return result;
    }

    GeneticProbability::operator float() const
    {
        return GetSum();
    }

    GeneticProbability GeneticProbability::expcdf( float dt ) const
    {
        GeneticProbability result = *this;

        result.m_DefaultValue = EXPCDF( -dt * result.m_DefaultValue );

        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            for( int si = 0; si < result.m_pAlleleCombosPerSpecies->size(); ++si )
            {
                for( auto& r_acp : (*result.m_pAlleleCombosPerSpecies)[ si ] )
                {
                    r_acp = EXPCDF( -dt * r_acp.GetValue() );
                }
            }
        }
        return result;
    }

    void GeneticProbability::CombineProbabilities( const GeneticProbability& right )
    {
        *this = 1.0f - ( 1.0f - *this ) * ( 1.0f - right );
    } 

    void GeneticProbability::Add( const AlleleComboProbability& racp )
    {
        if( m_pAlleleCombosPerSpecies == nullptr )
        {
            m_pAlleleCombosPerSpecies = new std::vector<std::vector<AlleleComboProbability>>( MAX_SPECIES );
        }
        int species_index = racp.GetAlleleCombo().GetSpeciesIndex();
        std::vector<AlleleComboProbability>& r_species_combos = (*m_pAlleleCombosPerSpecies)[ species_index ];
        r_species_combos.push_back( racp );
        std::sort( r_species_combos.begin(), r_species_combos.end(), compareACP );
    }

    std::vector<AlleleCombo> GeneticProbability::FindMissingAlleleCombos( const GeneticProbability& rExpected ) const
    {
        std::vector<AlleleCombo> missing;
        if( m_pAlleleCombosPerSpecies == nullptr )
        {
            return missing;
        }

        // See how this' combos compare with the expected
        for( int species_index = 0; species_index < m_pAlleleCombosPerSpecies->size(); ++species_index )
        {
            const std::vector<AlleleComboProbability>& r_this_combos = (*m_pAlleleCombosPerSpecies)[ species_index ];
            const std::vector<AlleleComboProbability>& r_exp_combos  = (*rExpected.m_pAlleleCombosPerSpecies)[ species_index ];

            for( auto& r_exp_acp : r_exp_combos )
            {
                bool found = false;
                for( auto& r_this_acp : r_this_combos )
                {
                    if( r_exp_acp.GetAlleleCombo() == r_this_acp.GetAlleleCombo() )
                    {
                        found = true;
                    }
                }
                if( !found )
                {
                    missing.push_back( r_exp_acp.GetAlleleCombo() );
                }
            }
        }
        return missing;
    }

    float GeneticProbability::GetValue( int speciesIndex, const VectorGenome& rGenome ) const
    {
        release_assert( speciesIndex < MAX_SPECIES );

        // -----------------------------------------------------------------------------
        // --- This assumes that the combos are sorted such that the most specific
        // --- combos are at the end of the list.  We try the more specific ones first
        // --- so that if the combos have stuff in common, we'll take those first and
        // --- the combos with less in common later.
        // -----------------------------------------------------------------------------
        if( m_pAlleleCombosPerSpecies != nullptr )
        {
            const std::vector<AlleleComboProbability>& r_species_combos = (*m_pAlleleCombosPerSpecies)[ speciesIndex ];
            for( int i = r_species_combos.size() - 1; i >= 0; --i )
            {
                const AlleleComboProbability& r_acp = r_species_combos[ i ];
                if( r_acp.HasAlleles( speciesIndex, rGenome ) )
                {
                    return r_acp.GetValue();
                }
            }
        }
        return m_DefaultValue;
    }

    float GeneticProbability::GetDefaultValue( ) const
    {
        return m_DefaultValue;
    }

    void GeneticProbability::SetDefaultValue( float value )
    {
        release_assert( value >= 0.0 );
        m_DefaultValue = value;
    }

    // See header file for more info.
    float GeneticProbability::GetSum() const
    {
        float sum = m_DefaultValue;
        if( m_pAlleleCombosPerSpecies != nullptr )
        {
            for( int index = 0; index < MAX_SPECIES; ++index )
            {
                if( (*m_pAlleleCombosPerSpecies)[ index ].size() > 0 )
                {
                    for( auto& racp : (*m_pAlleleCombosPerSpecies)[ index ] )
                    {
                        sum += racp.GetValue();
                    }
                }
            }
        }
        return sum;
    }

    int GeneticProbability::GetNumAlleleComboProbabilities() const
    {
        int num = 0;
        if( m_pAlleleCombosPerSpecies != nullptr )
        {
            for( int index = 0; index < MAX_SPECIES; ++index )
            {
                num += (*m_pAlleleCombosPerSpecies)[ index ].size();
            }
        }
        return num;
    }

    std::string GeneticProbability::ToString( int speciesIndex, const IVectorGenomeNames& rGenes ) const
    {
        std::stringstream ss;
        ss << "Default=" << m_DefaultValue << "\n";
        if( m_pAlleleCombosPerSpecies != nullptr )
        {
            for( auto& racp : (*m_pAlleleCombosPerSpecies)[ speciesIndex ] )
            {
                ss << racp.ToString( rGenes ) << "\n";
            }
            ss << "\n";
        }
        return ss.str();
    }

    void GeneticProbability::GeneticMath( GeneticProbability& result,
                                          const GeneticProbability& right,
                                          float( *MathOperation)(float lhs, float rhs) ) const
    {
        if( result.m_pAlleleCombosPerSpecies == nullptr )
        {
            if( right.m_pAlleleCombosPerSpecies != nullptr )
            {
                result.m_pAlleleCombosPerSpecies = new std::vector< std::vector<AlleleComboProbability> >( MAX_SPECIES );
            }
            else
            {
                // neither have combos so just return the result for the default
                result.m_DefaultValue = MathOperation( result.m_DefaultValue, right.m_DefaultValue );
            }
        }

        // If the 'right' doesn't have species specific values, 
        // this allows the right combos to point at something
        static std::vector< std::vector<AlleleComboProbability> > tmp_right( MAX_SPECIES );

        for( int i = 0; i < MAX_SPECIES; ++i )
        {
            std::vector<AlleleComboProbability>& r_result_combos = (*result.m_pAlleleCombosPerSpecies)[ i ];
            const std::vector<AlleleComboProbability>* p_right_combos = &(tmp_right[ i ]);
            if( right.m_pAlleleCombosPerSpecies != nullptr )
            {
                p_right_combos = &(*right.m_pAlleleCombosPerSpecies)[ i ];
            }

            // ------------------------------------------------------------
            // --- Temporary arrays of flags to be used during processing.
            // --- Make static so we only create memory once.
            // ------------------------------------------------------------
            static bool* right_was_combined = nullptr;
            if( right_was_combined == nullptr )
            {
                right_was_combined = (bool*)malloc( sizeof( bool ) * MAX_ALLELE_COMBOS );
            }
            memset( right_was_combined, false, sizeof( bool ) * MAX_ALLELE_COMBOS );

            // -----------------------------------------------------------------------------------------
            // --- We do reverse order because we keep the list of combos in order by complexity.
            // --- The combos at the end are the most complex and will combine with fewer other combos.
            // -----------------------------------------------------------------------------------------
            bool need_to_sort = false;
            for( int iLeft = r_result_combos.size() - 1; iLeft >= 0; --iLeft )
            {
                bool left_was_combined = false;
                for( int iRight = p_right_combos->size() - 1; iRight >= 0; --iRight )
                {
                    if( left_was_combined || right_was_combined[ iRight ] ) continue;

                    // put "left" inside the right loop incase we add to r_result_combos
                    AlleleComboProbability& racp_left = r_result_combos[ iLeft ];
                    const AlleleComboProbability& racp_right = (*p_right_combos)[ iRight ];

                    if( racp_left.IsSubset( racp_right ) )
                    {
                        AlleleCombo new_combo;
                        if( racp_right.GetAlleleCombo().GetBitMask() == ALL_BITS )
                        {
                            // I wish we didn't need this special condition, but if the ACP is from vector deposit,
                            // then we just want to update it.  We don't want it to create a new possiblity.
                            new_combo = racp_right.GetAlleleCombo();
                        }
                        else
                        {
                            new_combo = AlleleCombo::CombineSubset( racp_left.GetAlleleCombo(), racp_right.GetAlleleCombo() );
                        }
                        if( racp_left.GetAlleleCombo() == new_combo )
                        {
                            left_was_combined = true;

                            racp_left.m_Value = MathOperation( racp_left.m_Value, racp_right.GetValue() ); // updates entry in "result"
                        }
                        else
                        {
                            AlleleComboProbability new_acp( new_combo, racp_left.GetValue() );
                            new_acp.m_Value = MathOperation( new_acp.m_Value, racp_right.GetValue() );
                            r_result_combos.push_back( new_acp );
                            need_to_sort = true;
                        }
                        right_was_combined[ iRight ] = (racp_right.GetAlleleCombo() == new_combo);  // if not same combo, then combine with default
                    }
                    else if( racp_right.IsSubset( racp_left ) )
                    {
                        AlleleCombo new_combo;
                        if( racp_left.GetAlleleCombo().GetBitMask() == ALL_BITS )
                        {
                            // I wish we didn't need this special condition, but if the ACP is from vector deposit,
                            // then we just want to update it.  We don't want it to create a new possiblity.
                            new_combo = racp_left.GetAlleleCombo();
                        }
                        else
                        {
                            new_combo = AlleleCombo::CombineSubset( racp_right.GetAlleleCombo(), racp_left.GetAlleleCombo() );
                        }
                        AlleleComboProbability new_acp( new_combo, racp_left.GetValue() );
                        new_acp.m_Value = MathOperation( new_acp.m_Value, racp_right.GetValue() );

                        right_was_combined[ iRight ] = (racp_right.GetAlleleCombo() == new_combo);
                        left_was_combined            = (racp_left.GetAlleleCombo()  == new_combo);

                        if( left_was_combined )
                        {
                            r_result_combos[ iLeft ] = new_acp;
                        }
                        else
                        {
                            r_result_combos.push_back( new_acp );
                        }
                        need_to_sort = true;
                    }
                    else
                    {
                        // ------------------------------------------------------------------------------
                        // --- If neither is a subset of the other, then they usually don't intersect
                        // --- and we need to combine them.  However, sometimes there will be a previous
                        // --- entry that is this combination so only add it if the combination is new.
                        // --- For example, a2-a2 and b1:b1 don't intersect and need to be combined.
                        // --- However, you sometimes have already combined say b1:b1 with a2-b1:a2-b1
                        // --- so we don't want to do it again.  There can be interesecting cases like
                        // --- a2-b1:a1-b1 with b1-c3:b1-c3.
                        // ------------------------------------------------------------------------------
                        AlleleCombo new_combo = AlleleCombo::Combine( racp_left.GetAlleleCombo(), racp_right.GetAlleleCombo() );
                        if( !new_combo.IsNull() && !HasCombo( r_result_combos, new_combo ) )
                        {
                            AlleleComboProbability new_acp( new_combo, racp_left.GetValue() );
                            new_acp.m_Value = MathOperation( new_acp.m_Value, racp_right.GetValue() );
                            r_result_combos.push_back( new_acp );
                            need_to_sort = true;
                        }
                    }
                }
                if( !left_was_combined )
                {
                    // Left was not combined with any of the entries on the right so add the default
                    r_result_combos[ iLeft ].m_Value = MathOperation( r_result_combos[ iLeft ].m_Value,right.m_DefaultValue );
                    left_was_combined = true;
                }
            }

            // --------------------------------------------------------------------------
            // --- If an entry on the right did NOT combine with an entry on the left,
            // --- then we need to create a new entry that has been added to the default
            // --- of the left.  If it didn't combine, it means that there wasn't an entry
            // --- on the left that was related to the entry on the right.
            // --------------------------------------------------------------------------
            release_assert( p_right_combos->size() <= MAX_ALLELE_COMBOS );
            for( int iRight = p_right_combos->size() - 1; iRight >= 0; --iRight )
            {
                if( !right_was_combined[ iRight ] )
                {
                    // ------------------------------------------------------------------------------
                    // --- Notice that we take the combo from the right and the value from the left.
                    // --- This is done so that the left value stays on the left in the MathOperation
                    // ------------------------------------------------------------------------------
                    AlleleCombo new_combo = (*p_right_combos)[ iRight ].GetAlleleCombo();
                    AlleleComboProbability new_acp( new_combo, result.m_DefaultValue );
                    new_acp.m_Value = MathOperation( new_acp.m_Value, (*p_right_combos)[ iRight ].GetValue() );
                    r_result_combos.push_back( new_acp );
                    need_to_sort = true;
                }
            }

            // --------------------------------------------------------------------------------------
            // --- Keep the list sorted from least complex (fewest Loci) to most complex (most Loci)
            // --- This will help in calculating and in GetValue().
            // --------------------------------------------------------------------------------------
            if( need_to_sort )
            {
                std::sort( r_result_combos.begin(), r_result_combos.end(), compareACP );
            }
        }

        // Combine the defaults
        result.m_DefaultValue = MathOperation( result.m_DefaultValue, right.m_DefaultValue );
    }

    void GeneticProbability::ScalarMath( GeneticProbability& result, float rhs, 
                                         float( *MathOperation )(float lhs, float rhs) ) const
    {
        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            for( auto& r_species_combos : *result.m_pAlleleCombosPerSpecies )
            {
                for( auto& racp : r_species_combos )
                {
                    racp.m_Value = MathOperation( racp.m_Value, rhs );
                }
            }
        }
    }

    GeneticProbability GeneticProbability::RightHandSubtraction( float lhs ) const
    {
        GeneticProbability result = *this;

        result.m_DefaultValue = lhs - result.m_DefaultValue;
        if( result.m_DefaultValue < 0.0f )
        {
            result.m_DefaultValue = 0.0f;
        }

        if( result.m_pAlleleCombosPerSpecies != nullptr )
        {
            for( auto& r_species_combos : *result.m_pAlleleCombosPerSpecies )
            {
                for( auto& racp : r_species_combos )
                {
                    racp = lhs - racp.GetValue();
                }
            }
        }
        return result;
    }

    // static methods

    bool GeneticProbability::HasCombo( const std::vector<AlleleComboProbability>& rCombos, const AlleleCombo& rac )
    {
        for( auto& racp : rCombos )
        {
            if( racp.GetAlleleCombo() == rac )
            {
                return true;
            }
        }
        return false;
    }

    bool GeneticProbability::compareACP( const AlleleComboProbability& rLeft, const AlleleComboProbability& rRight )
    {
        return rLeft.GetAlleleCombo().Compare( rLeft.GetAlleleCombo(), rRight.GetAlleleCombo());
    }

    float GeneticProbability::AdditionOperation( float lhs, float rhs )
    {
        float result = lhs + rhs;
        if( result < 0.0f )
        {
            result = 0.0f;
        }
        return result;
    }

    float GeneticProbability::SubtractionOperation( float lhs, float rhs )
    {
        float result = lhs - rhs;
        if( result < 0.0f )
        {
            result = 0.0f;
        }
        return result;
    }

    float GeneticProbability::MultiplicationOperation( float lhs, float rhs )
    {
        float result = lhs * rhs;
        return result;
    }

    float GeneticProbability::DivisionOperation( float lhs, float rhs )
    {
        release_assert( rhs != 0.0 );
        float result = lhs / rhs;
        return result;
    }

    void GeneticProbability::serialize( Kernel::IArchive& ar, GeneticProbability& rgp )
    {
        std::vector<std::vector<AlleleComboProbability>> tmp;
        if( ar.IsWriter() && (rgp.m_pAlleleCombosPerSpecies != nullptr) )
        {
            tmp = *rgp.m_pAlleleCombosPerSpecies;
        }
        ar.startObject();
        ar.labelElement( "m_DefaultValue" ) & rgp.m_DefaultValue;
        ar.labelElement( "m_AlleleCombosPerSpecies" ) & tmp;
        ar.endObject();

        if( ar.IsReader() && (tmp.size() > 0) )
        {
            for( auto& r_combos : tmp )
            {
                for( auto& racp : r_combos )
                {
                    rgp.Add( racp );
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // --- Operator functions for when the scalar is on the left hand side
    // ---------------------------------------------------------------------

    GeneticProbability operator+( float lhs, const GeneticProbability& rhs )
    {
        return rhs + lhs;
    }

    GeneticProbability operator-( float lhs, const GeneticProbability& rhs )
    {
        return rhs.RightHandSubtraction( lhs );
    }

    GeneticProbability operator*( float lhs, const GeneticProbability& rhs )
    {
        return rhs * lhs;
    }

    // I don't think we need this
    //GeneticProbability operator/( float lhs, const GeneticProbability& rhs );

}
