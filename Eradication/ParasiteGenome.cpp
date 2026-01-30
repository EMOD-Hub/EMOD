
#include "stdafx.h"

#include <numeric>

#include "ParasiteGenome.h"
#include "ParasiteGenetics.h"
#include "RANDOM.h"
#include "IArchive.h"
#include "Debug.h"
#include "Log.h"

SETUP_LOGGING( "ParasiteGenome" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- Testing Support
    // ------------------------------------------------------------------------

    static int32_t num_active = 0;

    int32_t ParasiteGenome::GetNumActive()
    {
        return num_active;
    }

    // ------------------------------------------------------------------------
    // --- ParasiteGenomeAllele
    // ------------------------------------------------------------------------

    ParasiteGenomeAllele::ParasiteGenomeAllele( GenomeLocationType::Enum locType,
                                                int32_t location,
                                                int32_t index,
                                                int32_t val )
        : m_LocationType( locType )
        , m_GenomeLocation( location )
        , m_SequenceIndex( index )
        , m_SequenceValue( val )
    {
    }

    ParasiteGenomeAllele::~ParasiteGenomeAllele()
    {
    }

    int32_t ParasiteGenomeAllele::GetGenomeLocation() const
    {
        return m_GenomeLocation;
    }

    int32_t ParasiteGenomeAllele::GetSequenceIndex() const
    {
        return m_SequenceIndex;
    }

    int32_t ParasiteGenomeAllele::GetSequenceValue() const
    {
        return m_SequenceValue;
    }

    // ------------------------------------------------------------------------
    // --- ParasiteGenomeInner
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(ParasiteGenomeInner)
    END_QUERY_INTERFACE_BODY(ParasiteGenomeInner)

    void ParasiteGenomeInner::CalculateHashcodes( ParasiteGenomeInner* pInner )
    {
        pInner->m_HashCode = 17;
        if( pInner->m_AlleleRoots.size() == pInner->m_NucleotideSequence.size() )
        {
            // ---------------------------------------------------------------------------
            // --- The following commented out code is the basic algorithm.  However,
            // --- the live code is 4-5 times faster.  This is important because computing
            // --- the hashcode can take a large amount of time when there are a lot of
            // --- barcode locations.
            // ---------------------------------------------------------------------------
            //for( int i = 0; i < pInner->m_NucleotideSequence.size(); ++i )
            //{
            //    pInner->m_HashCode = 31 * pInner->m_HashCode + pInner->m_NucleotideSequence[ i ];
            //    pInner->m_HashCode = 31 * pInner->m_HashCode + pInner->m_AlleleRoots[ i ];
            //}
            // ---------------------------------------------------------------------------
            int len =  pInner->m_NucleotideSequence.size();
            int i = 0;
            for( ; (i+3) < len; i+=4 )
            {
                // the hashcode calculation normally can overflow, so I'm pretty certain this is ok
                #pragma warning( push )
                #pragma warning( disable: 4307 ) // warning C4307: '*': integral constant overflow
                pInner->m_HashCode = 31 * 31 * 31 * 31 * 31 * 31 * 31 * 31 * pInner->m_HashCode
                                   + 31 * 31 * 31 * 31 * 31 * 31 * 31 *      pInner->m_NucleotideSequence[ i ]
                                   + 31 * 31 * 31 * 31 * 31 * 31 *           pInner->m_AlleleRoots[ i ]
                                   + 31 * 31 * 31 * 31 * 31 *                pInner->m_NucleotideSequence[ i+1 ]
                                   + 31 * 31 * 31 * 31 *                     pInner->m_AlleleRoots[ i+1 ]
                                   + 31 * 31 * 31 *                          pInner->m_NucleotideSequence[ i+2 ]
                                   + 31 * 31 *                               pInner->m_AlleleRoots[ i+2 ]
                                   + 31*                                     pInner->m_NucleotideSequence[ i+3 ]
                                   +                                         pInner->m_AlleleRoots[ i+3 ];
                #pragma warning( pop )
            }
            for( ; i < len; ++i )
            {
                pInner->m_HashCode = 31 * (31 * pInner->m_HashCode + pInner->m_NucleotideSequence[ i ]) + pInner->m_AlleleRoots[ i ];
            }
        }
        else
        {
            // we don't include the allele roots when they have not been defined
            for( int i = 0; i < pInner->m_NucleotideSequence.size(); ++i )
            {
                pInner->m_HashCode = 31 * pInner->m_HashCode + pInner->m_NucleotideSequence[ i ];
            }
        }

        const std::vector<int32_t>& r_barcode_indexes = ParasiteGenetics::GetInstance()->GetIndexesBarcode();

        pInner->m_BarcodeHashcode = 17;
        int len =  r_barcode_indexes.size();
        int i = 0;
        for( ; (i+3) < len; i+=4 )
        {
            pInner->m_BarcodeHashcode = 31 * 31 * 31 * 31 * pInner->m_BarcodeHashcode
                                      + 31 * 31 * 31 *      pInner->m_NucleotideSequence[ r_barcode_indexes[ i   ] ]
                                      + 31 * 31 *           pInner->m_NucleotideSequence[ r_barcode_indexes[ i+1 ] ]
                                      + 31 *                pInner->m_NucleotideSequence[ r_barcode_indexes[ i+2 ] ]
                                      +                     pInner->m_NucleotideSequence[ r_barcode_indexes[ i+3 ] ];
        }
        for( ; i < len; ++i )
        {
            pInner->m_BarcodeHashcode = 31 * pInner->m_BarcodeHashcode + pInner->m_NucleotideSequence[ r_barcode_indexes[ i ] ];
        }
    }

    ParasiteGenomeInner::ParasiteGenomeInner()
        : m_refcount(0)
        , m_ID(0)
        , m_HashCode(0)
        , m_BarcodeHashcode(0)
        , m_NucleotideSequence()
        , m_AlleleRoots()
        , m_GenomeCrossoverLocations()
    {
        ++num_active;
    }

    ParasiteGenomeInner::ParasiteGenomeInner( const std::vector<int32_t>& rNucleotideSequence )
        : m_refcount(0)
        , m_ID( ParasiteGenetics::CreateInstance()->GetNextGenomeID() )
        , m_HashCode(0)
        , m_BarcodeHashcode(0)
        , m_NucleotideSequence(rNucleotideSequence)
        , m_AlleleRoots()
        , m_GenomeCrossoverLocations()
    {
        CalculateHashcodes( this );
        ++num_active;
    }

    ParasiteGenomeInner::ParasiteGenomeInner( const std::vector<int32_t>& rNucleotideSequence,
                                              const std::vector<int32_t>& rAlleleRoots )
        : m_refcount(0)
        , m_ID( ParasiteGenetics::CreateInstance()->GetNextGenomeID() )
        , m_HashCode(0)
        , m_BarcodeHashcode(0)
        , m_NucleotideSequence(rNucleotideSequence)
        , m_AlleleRoots(rAlleleRoots)
        , m_GenomeCrossoverLocations()
    {
        release_assert( rNucleotideSequence.size() == rAlleleRoots.size() );
        release_assert( rNucleotideSequence.size() == ParasiteGenetics::CreateInstance()->GetNumBasePairs() );

        CalculateHashcodes( this );
        ++num_active;
    }

    ParasiteGenomeInner::ParasiteGenomeInner( const ParasiteGenomeInner* pOldInner, uint32_t infectionID )
        : m_refcount(0)
        , m_ID( ParasiteGenetics::CreateInstance()->GetNextGenomeID() )
        , m_HashCode(0)
        , m_BarcodeHashcode(0)
        , m_NucleotideSequence(pOldInner->m_NucleotideSequence)
        , m_AlleleRoots( pOldInner->m_NucleotideSequence.size(), infectionID )
    {
        release_assert( m_NucleotideSequence.size() == m_AlleleRoots.size() );
        CalculateHashcodes( this );
        ++num_active;
    }

    ParasiteGenomeInner::ParasiteGenomeInner( const ParasiteGenomeInner& rMaster )
        : m_refcount(0)
        , m_ID( rMaster.m_ID )
        , m_HashCode( rMaster.m_HashCode )
        , m_BarcodeHashcode( rMaster.m_BarcodeHashcode )
        , m_NucleotideSequence( rMaster.m_NucleotideSequence )
        , m_AlleleRoots( rMaster.m_AlleleRoots )
        , m_GenomeCrossoverLocations(rMaster.m_GenomeCrossoverLocations)
    {
        // This should really only occur in recombination where rMaster is a local copy
        ++num_active;
    }

    ParasiteGenomeInner::~ParasiteGenomeInner()
    {
        --num_active;
    }

    void ParasiteGenomeInner::Initialize( const ParasiteGenomeInner* pInput )
    {
        m_HashCode           = 0;
        m_BarcodeHashcode    = 0;
        m_NucleotideSequence = pInput->m_NucleotideSequence;
        m_AlleleRoots        = pInput->m_AlleleRoots;
        m_GenomeCrossoverLocations = {};
    }

    const std::vector<int32_t>& ParasiteGenomeInner::GetNucleotideSequence() const
    {
        return m_NucleotideSequence;
    }

    const std::vector<int32_t>& ParasiteGenomeInner::GetAlleleRoots() const
    {
        return m_AlleleRoots;
    }


    REGISTER_SERIALIZABLE( ParasiteGenomeInner );

    void ParasiteGenomeInner::serialize( IArchive& ar, ParasiteGenomeInner* pInner )
    {
        ar.labelElement("m_ID"                ) & pInner->m_ID;
        ar.labelElement("m_HashCode"          ) & pInner->m_HashCode;
        ar.labelElement("m_BarcodeHashcode"   ) & pInner->m_BarcodeHashcode;
        ar.labelElement("m_NucleotideSequence") & pInner->m_NucleotideSequence;
        ar.labelElement("m_AlleleRoots"       ) & pInner->m_AlleleRoots;
        // Not serializing m_GenomeCrossoverLocations to keep serialization file the same
        // This parameter exists only for ReportFPGNewInfections and not for anything else. 
    }

    // ------------------------------------------------------------------------
    // --- ParasiteGenome
    // ------------------------------------------------------------------------
    ParasiteGenome ParasiteGenome::TEST_CreateGenome( ParasiteGenomeInner* pInner, bool wrapperOnly )
    {
        return ParasiteGenome( pInner, wrapperOnly );
    }

    ParasiteGenome::ParasiteGenome()
        : m_pInner( nullptr )
    {
    }

    ParasiteGenome::ParasiteGenome( ParasiteGenomeInner* pInner, bool wrapperOnly )
        : m_pInner( pInner )
    {
        if( m_pInner != nullptr )
        {
            if( wrapperOnly )
            {
                m_pInner = pInner;
            }
            else
            {
                m_pInner = ParasiteGenetics::CreateInstance()->AddParasiteGenomeInner( m_pInner );
            }
            m_pInner->AddRef();
        }
    }

    ParasiteGenome::ParasiteGenome( const ParasiteGenome& rMaster )
        : m_pInner( rMaster.m_pInner )
    {
        if( m_pInner != nullptr )
        {
            m_pInner->AddRef();
        }
    }

    ParasiteGenome::ParasiteGenome( const std::vector<int32_t>& rNucleotideSequence )
        : m_pInner( new ParasiteGenomeInner( rNucleotideSequence ) )
    {
        m_pInner->AddRef();
    }

    ParasiteGenome::ParasiteGenome( const std::vector<int32_t>& rNucleotideSequence,
                                    const std::vector<int32_t>& rRoots )
        : m_pInner( new ParasiteGenomeInner( rNucleotideSequence, rRoots ) )
    {
        m_pInner = ParasiteGenetics::CreateInstance()->AddParasiteGenomeInner( m_pInner );
        m_pInner->AddRef();
    }

    ParasiteGenome::ParasiteGenome( const ParasiteGenome& rGenome, uint32_t infectionID )
        : m_pInner( new ParasiteGenomeInner( rGenome.m_pInner, infectionID ) )
    {
        m_pInner = ParasiteGenetics::CreateInstance()->AddParasiteGenomeInner( m_pInner );
        m_pInner->AddRef();
    }

    ParasiteGenome::~ParasiteGenome()
    {
        if( m_pInner != nullptr )
        {
            m_pInner->Release();
        }
    }

    ParasiteGenome& ParasiteGenome::operator=( const ParasiteGenome& rhs )
    {
        if( this->m_pInner != rhs.m_pInner )
        {
            if( this->m_pInner != nullptr )
            { 
                this->m_pInner->Release();
            }

            this->m_pInner = rhs.m_pInner;

            if( this->m_pInner != nullptr )
            {
                this->m_pInner->AddRef();
            }
        }
        return *this;
    }

    bool ParasiteGenome::operator==( const ParasiteGenome& rThat ) const
    {
        if( this->m_pInner == rThat.m_pInner ) return true;

        if( (this->m_pInner == nullptr) && (rThat.m_pInner != nullptr) ) return false;
        if( (this->m_pInner != nullptr) && (rThat.m_pInner == nullptr) ) return false;
        if( (this->m_pInner != nullptr) && (rThat.m_pInner != nullptr) )
        {
            if( this->m_pInner->m_HashCode != rThat.m_pInner->m_HashCode ) return false;

            // ----------------------------------------------------------------------------
            // --- Since ParasiteGenome is immutable, the hashcode should always represent
            // --- the rest of the contents.
            // ----------------------------------------------------------------------------
        }
        return true;
    }

    bool ParasiteGenome::operator!=( const ParasiteGenome& rThat ) const
    {
        return !operator==( rThat );
    }

    bool ParasiteGenome::IsNull() const
    {
        return (m_pInner == nullptr);
    }

    uint32_t ParasiteGenome::GetID() const
    {
        release_assert( m_pInner != nullptr );
        return uint32_t(m_pInner->m_ID);
    }

    bool ParasiteGenome::HasAlleleRoots() const
    {
        return (m_pInner->m_AlleleRoots.size() == m_pInner->m_NucleotideSequence.size());
    }

    std::string ParasiteGenome::ToString() const
    {
        // ------------
        // --- FPG-TODO
        // ------------
        release_assert( false );
        return "not implemented";
    }

    std::string ParasiteGenome::ConvertToString( const std::vector<int32_t>& rIndexes ) const
    {
        static const char NucleotideBaseLetters[] = { 'A', 'C', 'G', 'T' };

        std::string str( rIndexes.size(), ' ' );
        int i = 0;
        for( auto index : rIndexes )
        {
            str[ i ] = NucleotideBaseLetters[ m_pInner->m_NucleotideSequence[ index ] ];
            ++i;
        }
        return str;
    }

    std::string ParasiteGenome::GetBarcode() const
    {
        release_assert( m_pInner != nullptr );

        const std::vector<int32_t>& r_barcode_indexes = ParasiteGenetics::GetInstance()->GetIndexesBarcode();

        return ConvertToString( r_barcode_indexes );
    }

    std::string ParasiteGenome::GetDrugResistantString() const
    {
        release_assert( m_pInner != nullptr );

        const std::vector<int32_t>& r_drug_indexes = ParasiteGenetics::GetInstance()->GetIndexesDrugResistant();

        return ConvertToString( r_drug_indexes );
    }

    std::string ParasiteGenome::GetHrpString() const
    {
        release_assert( m_pInner != nullptr );

        const std::vector<int32_t>& r_hrp_indexes = ParasiteGenetics::GetInstance()->GetIndexesHRP();

        return ConvertToString( r_hrp_indexes );
    }

    bool ParasiteGenome::HasHrpMarker() const
    {
        const std::vector<int32_t>& r_hrp_indexes = ParasiteGenetics::GetInstance()->GetIndexesHRP();
        if( r_hrp_indexes.size() == 0 )
        {
            return true;
        }
        for( auto index : r_hrp_indexes )
        {
            if( m_pInner->m_NucleotideSequence[ index ] == 0 ) // 0 = 'A'
            {
                return true;
            }
        }
        return false;
    }
      
    int64_t ParasiteGenome::GetHashcode() const
    {
        return m_pInner->m_HashCode;
    }

    int64_t ParasiteGenome::GetBarcodeHashcode() const
    {
        return m_pInner->m_BarcodeHashcode;
    }

    int32_t ParasiteGenome::GetMSP() const
    {
        release_assert( m_pInner != nullptr );
        return m_pInner->m_NucleotideSequence[ ParasiteGenetics::GetInstance()->GetIndexMSP() ];
    }

    std::vector<int32_t> ParasiteGenome::GetPfEMP1EpitopesMajor() const
    {
        release_assert( m_pInner != nullptr );
        const std::vector<int32_t>& r_major_indexes = ParasiteGenetics::GetInstance()->GetIndexesPfEMP1Major();

        std::vector<int32_t> majors;
        for( auto index : r_major_indexes )
        {
            majors.push_back( m_pInner->m_NucleotideSequence[ index ] );
        }

        return majors;
    }

    const std::vector<int32_t>& ParasiteGenome::GetNucleotideSequence() const
    {
        release_assert( m_pInner != nullptr );
        return m_pInner->m_NucleotideSequence;
    }

    const std::vector<int32_t>& ParasiteGenome::GetAlleleRoots() const
    {
        release_assert( m_pInner != nullptr );
        return m_pInner->m_AlleleRoots;
    }

    const std::vector<uint32_t>& ParasiteGenome::GetCrossovers() const
    {
        release_assert( m_pInner != nullptr );
        return m_pInner->m_GenomeCrossoverLocations;
    }

    bool ParasiteGenome::HasAllele( const ParasiteGenomeAllele& rAllele ) const
    {
        if( ParasiteGenetics::GetInstance()->GetIndexesDrugResistant().size() == 0 )
        {
            // not using drug resistance
            return false;
        }

        return(m_pInner->m_NucleotideSequence[ rAllele.GetSequenceIndex() ] == rAllele.GetSequenceValue());
    }

    bool ParasiteGenome::HasAllOfTheAlleles( const ParasiteGenomeAlleleCollection& rAlleleCollection ) const
    {
        if( ParasiteGenetics::GetInstance()->GetIndexesDrugResistant().size() == 0 )
        {
            // not using drug resistance
            return false;
        }

        for( int i = 0; i < rAlleleCollection.size(); ++i )
        {
            if( !HasAllele( rAlleleCollection[ i ] ) )
            {
                return false;
            }
        }
        return true;
    }

    void ParasiteGenome::serialize(IArchive& ar, ParasiteGenome& pg)
    {
        ar.startObject();
        if( ar.IsWriter() )
        {
            ar.labelElement("m_pInner" ) & pg.m_pInner;
        }
        else
        {
            ParasiteGenomeInner* p_inner_tmp = nullptr;
            int32_t hashcode = 0;
            ar.labelElement("m_pInner" ) & p_inner_tmp;

            pg = ParasiteGenome( p_inner_tmp, false );
        }
        ar.endObject();
    }

    // ------------------------------------------------------------------------
    // --- Recombination
    // ------------------------------------------------------------------------

    void ParasiteGenome::AddGenome( ParasiteGenomeInner* pInnerOrig,
                                    ParasiteGenomeInner* pInnerNew,
                                    std::vector<ParasiteGenome>& rNewGenomes )
    {
        ParasiteGenomeInner::CalculateHashcodes( pInnerNew );
        if( pInnerOrig->m_HashCode == pInnerNew->m_HashCode )
        {
            rNewGenomes.push_back( ParasiteGenome( pInnerOrig, true ) );
        }
        else
        {
            // make a copy of pInnerNew because it is static memory
            ParasiteGenomeInner* p_inner = new ParasiteGenomeInner( *pInnerNew );
            p_inner->m_ID = ParasiteGenetics::CreateInstance()->GetNextGenomeID();
            rNewGenomes.push_back( ParasiteGenome( p_inner, false ) );
        }
    }

    static Crossover CHROMATID_PAIR_CROSSOVERS[] = { Crossover( 0 ,0 ), Crossover( 1, 0 ), Crossover( 0, 1 ), Crossover( 1, 1 ) };

    bool DetermineCrossover( RANDOMBASE* pRNG,
                             int32_t minLocation,
                             int32_t maxLocation,
                             bool isGoingLeftOfObligate,
                             bool isObligate,
                             Crossover& rCrossover )
    {
        int32_t location = 0;
        bool done_jumping = false;
        if( isObligate )
        {
            location = minLocation + pRNG->uniformZeroToN32( maxLocation - minLocation + 1 );
            done_jumping = false;  // it should never be out of range
            if (minLocation == 1) {
                LOG_VALID_F("obligate %d\n", location);
            }
        }
        else
        {
            int32_t amount_to_jump = ParasiteGenetics::GetInstance()->GetSecondaryCrossoverDistance( pRNG );
            if( isGoingLeftOfObligate )
            {
                location = maxLocation - amount_to_jump;
            }
            else
            {
                location = minLocation + amount_to_jump;
            }
            done_jumping = ((location < minLocation) || (maxLocation < location));
        }
        if ((minLocation == 1 || maxLocation == 643000) && !isObligate)  //minLocation == 1
        {
            LOG_VALID_F("First chromosome: crossover location %d going %s location: %s\n", location, isGoingLeftOfObligate ? "left" : "right", done_jumping ? "bad" : "good");
        }
        if( !done_jumping ) // information was exchanged
        {
            int32_t i_chromatid_pair = pRNG->uniformZeroToN16( 4 ); // 4 possible chromatid pairs
            rCrossover = CHROMATID_PAIR_CROSSOVERS[ i_chromatid_pair ];
            rCrossover.genome_location = location;
            rCrossover.is_obligate = isObligate;
        }
        return done_jumping;
    }

    //NOTE: we use a std::list here so that we can keep
    //the list sorted as we add crossovers.
    void ParasiteGenome::FindCrossovers( RANDOMBASE* pRNG,
                                         uint32_t iChromosome,
                                         std::list<Crossover>& rCrossovers )
    {
        if( STATIC_TEST_CROSSOVERS.size() > 0 )
        {
            rCrossovers = STATIC_TEST_CROSSOVERS;
            return;
        }

        // NOTE: locations are from 1 to MAX_LOCATIONS.  This makes looking at the values easier.
        int32_t min_chromosome_loc = (iChromosome == 0) ? 1 : ParasiteGenetics::CHROMOSOME_ENDS[ iChromosome-1 ] + 1;
        int32_t max_chromosome_loc = ParasiteGenetics::CHROMOSOME_ENDS[ iChromosome ];

        Crossover obligate_co;
        DetermineCrossover( pRNG, min_chromosome_loc, max_chromosome_loc, true, true, obligate_co );
        // add to the list after the ones that come before are added

        // -------------------------------------------------------------------
        // --- Find the other crossovers that are to the LEFT of the obligate
        // -------------------------------------------------------------------
        int32_t max_loc = obligate_co.genome_location - 1;
        bool done = false;
        while( !done )
        {
            Crossover co;
            done = DetermineCrossover( pRNG, min_chromosome_loc, max_loc, true, false, co );

            if( !done )
            {
                rCrossovers.push_front( co ); // put at front so list is sorted
                max_loc = co.genome_location - 1;
            }
        }

        // Add the obligate crossover now that the crossovers that happen
        // before are in the list
        rCrossovers.push_back( obligate_co );

        // -------------------------------------------------------------------
        // --- Find the other crossovers that are to the RIGHT of the obligate
        // -------------------------------------------------------------------
        int32_t min_loc = obligate_co.genome_location + 1;
        done = false;
        while( !done )
        {
            Crossover co;
            done = DetermineCrossover( pRNG, min_loc, max_chromosome_loc, false, false, co );
            if( !done )
            {
                rCrossovers.push_back( co ); // add to end so list stays sorted
                min_loc = co.genome_location + 1;
            }
        }
    }

    // ------------------------------------------------------------------------------
    // --- These statics are moved globally so that they are allocated before "main"
    // --- is called. This gives us 4 active ParasiteGenomeInner objects that are
    // --- active all the time.  If they were in Recombination(), the would get
    // --- allocated once - the first time Recombination() is called - and then
    // --- you can get rid of them.  Better they are there allthe time.
    // ------------------------------------------------------------------------------
    static ParasiteGenomeInner female_0, female_1, male_0, male_1;

    void ParasiteGenome::LogBarcodes( const char* name,
                                      int iChromosome,
                                      ParasiteGenomeInner* pChild0,
                                      ParasiteGenomeInner* pChild1,
                                      ParasiteGenomeInner* pChild2,
                                      ParasiteGenomeInner* pChild3 )
    {
        if( EnvPtr->Log->SimpleLogger::IsLoggingEnabled( Logger::VALIDATION, _module, _log_level_enabled_array ) )
        {
            if( pChild0 == &female_0 )
            {
                // make increase reverence counting to ensure that Release() doesn't try to delete these inner objects
                female_0.AddRef();
                female_1.AddRef();
                male_0.AddRef();
                male_1.AddRef();
            }
            ParasiteGenome genome_f0( pChild0, true );
            ParasiteGenome genome_f1( pChild1, true );
            ParasiteGenome genome_m0( pChild2, true );
            ParasiteGenome genome_m1( pChild3, true );

            if (genome_f0.GetBarcode() != genome_m0.GetBarcode())
            {
                LOG_VALID_F("name(%s)  chromo(%d)  Child_0=%s  Child_1=%s  Child_2=%s  Child_3=%s\n",
                    name,
                    iChromosome,
                    genome_f0.GetBarcode().c_str(),
                    genome_f1.GetBarcode().c_str(),
                    genome_m0.GetBarcode().c_str(),
                    genome_m1.GetBarcode().c_str());
            }
        }
    }

    void ParasiteGenome::Recombination( RANDOMBASE* pRNG,
                                        const ParasiteGenome& rGenomeFemale,
                                        const ParasiteGenome& rGenomeMale,
                                        std::vector<ParasiteGenome>& rNewGenomes )
    {
        release_assert( rNewGenomes.size() == 0 );

        // -----------------------------------------------------------------------------------
        // --- The section of crossovers is based on the following paper:
        // ---    "Modeling the genetic relatedness of Plasmodium falciparum parasites following
        // ---    meiotic recombination and cotransmission" by Wong, Wenger, Hartl, Wirth 2018
        // --- From the paper, we selected the Obligate Chiasma Model
        // -----------------------------------------------------------------------------------

        if( (rGenomeFemale.GetID() == rGenomeMale.GetID()) || ParasiteGenetics::GetInstance()->IsFPGSimulatingBaseModel() )
        {
            // if the genomes are the same, then recombination will not change anything.
            // I think this is called "selfing"
            rNewGenomes.push_back( rGenomeFemale );

            return;
        }

        // Keep one set of memory to do all of the work in.  Only create new memory
        // when the new genomes are not one of the parents.
        static ParasiteGenomeInner* female_inners[] = { &female_0, &female_1 };
        static ParasiteGenomeInner* male_inners[] = { &male_0, &male_1 };

        female_0.Initialize( rGenomeFemale.m_pInner );
        female_1.Initialize( rGenomeFemale.m_pInner );
        male_0.Initialize( rGenomeMale.m_pInner );
        male_1.Initialize( rGenomeMale.m_pInner );

        for( int32_t i_chromosome = 0; i_chromosome < ParasiteGenetics::NUM_CHROMOSOMES; ++i_chromosome )
        {
            //if( r_female_chromosome_hashcodes[ i_chromosome ] == r_male_chromosome_hashcodes[ i_chromosome ] ) continue;

            if( !ParasiteGenetics::GetInstance()->ChromosomeHasLocationsOfInterest( i_chromosome ) ) continue;

            int first_index_on_chromosome = ParasiteGenetics::GetInstance()->GetFirstIndexOnChromosome( i_chromosome );

            static std::list<Crossover> crossovers;
            crossovers.clear();
            FindCrossovers( pRNG, i_chromosome, crossovers );

            for( auto co : crossovers )
            {
                int32_t index = ParasiteGenetics::GetInstance()->ConvertCrossoverLocationToIndex( i_chromosome, co.genome_location );
                if( index < 0 ) continue;

                ParasiteGenomeInner* p_female = female_inners[ co.chromatid_female ];
                ParasiteGenomeInner* p_male   =   male_inners[ co.chromatid_male   ];
                
                LOG_VALID_F("Female chromatid %d male chromatid %d \n", co.chromatid_female, co.chromatid_male);
                // ------------------------------------------------------------------------------
                // --- We need to make sure that the chromatids selected in FindCrossovers()
                // --- are still pointing to the same alleles.  Hence, if we are processing the
                // --- locations from left-to-right (ascending order), then we want to swap the
                // --- locations that are to the left of the crossover.  This leaves the alleles
                // --- to the right as what was pointed to by the next crossover.
                // ------------------------------------------------------------------------------
                Swap( first_index_on_chromosome, index - 1, p_female, p_male );
            }

            // ----------------------------------------------------------------------------------
            // --- Independent Assortment - Randomly shuffle this chromosome between the genomes.
            // ----------------------------------------------------------------------------------
            LogBarcodes( "BeforeIA", i_chromosome, &female_0, &female_1, &male_0, &male_1 );
            std::vector<int32_t> new_order = IndependentAssortment( pRNG, i_chromosome, &female_0, &female_1, &male_0, &male_1 );
            std::vector<ParasiteGenomeInner*> p_all = { &female_0, &female_1, &male_0, &male_1 };
            release_assert( new_order.size() == p_all.size() );
            for(auto co : crossovers)
            {
                uint32_t male_offset = 2;
                uint32_t male_idx = co.chromatid_male + male_offset;  //offset 0->2, 1->3
                bool other_found = false;
                for(int i = 0; i < p_all.size(); i++)
                {
                    // co.chromatid_female = 0 - > index 0
                    // co.chromatid_female = 1 - > index 1
                    // co.chromatid_male   = 0 - > index 2
                    // co.chromatid_male   = 1 - > index 3
                    if(new_order[i] == male_idx || new_order[i] == co.chromatid_female)
                    {
                        p_all[i]->m_GenomeCrossoverLocations.push_back( co.genome_location );
                        if(other_found)
                        {
                            break; 
                        }
                        else
                        {
                            other_found = true;
                        }
                    }
                }
            }
            LogBarcodes( "AfterIA ", i_chromosome, &female_0, &female_1, &male_0, &male_1 );
        }

        AddGenome( rGenomeFemale.m_pInner, &female_0, rNewGenomes );
        AddGenome( rGenomeFemale.m_pInner, &female_1, rNewGenomes );
        AddGenome( rGenomeMale.m_pInner,   &male_0,   rNewGenomes );
        AddGenome( rGenomeMale.m_pInner,   &male_1,   rNewGenomes );

        release_assert( rNewGenomes.size() == 4 );

        if( EnvPtr->Log->SimpleLogger::IsLoggingEnabled( Logger::VALIDATION, _module, _log_level_enabled_array ) )
        {
            int32_t num_locs = rGenomeFemale.GetNucleotideSequence().size();
            int32_t all_A = num_locs * 0; // 0=A
            int32_t all_C = num_locs * 1; // 1=C

            int32_t sum_mom = std::accumulate( rGenomeFemale.GetNucleotideSequence().begin(), rGenomeFemale.GetNucleotideSequence().end(), 0 );
            int32_t sum_dad = std::accumulate( rGenomeMale.GetNucleotideSequence().begin(),   rGenomeMale.GetNucleotideSequence().end(),   0 );

            if( ((sum_mom == all_A) && (sum_dad == all_C)) || ((sum_mom == all_C) && (sum_dad == all_A)) )
            {
                LogBarcodes( "FinalRecombination", -1, rNewGenomes[ 0 ].m_pInner, rNewGenomes[ 1 ].m_pInner, rNewGenomes[ 2 ].m_pInner, rNewGenomes[ 3 ].m_pInner );
            }
        }
    }

    // IndependentAssortment copy algorithms
    enum SwapType
    {
        TWO,
        THREE,
        TWO_TWO,
        FOUR
    };

    // structure for indicating how independent assortment is to occur for
    // the current chromosome
    struct Assortment
    {
        SwapType swap_type; // swap algorithm
        std::vector<int32_t> inner_indexes; // index of the genome
        std::vector<int32_t> new_order;     // new order of the genomes after swap

        Assortment( SwapType st, int32_t index0, int32_t index1, std::vector<int32_t> new_order )
            : swap_type( SwapType::TWO ) // purposely ignore input
            , inner_indexes()
            , new_order( new_order )
        {
            inner_indexes.push_back( index0 );
            inner_indexes.push_back( index1 );
        }

        Assortment( SwapType st, int32_t index0, int32_t index1, int32_t index2, std::vector<int32_t> new_order )
            : swap_type( SwapType::THREE ) // purposely ignore input
            , inner_indexes()
            , new_order( new_order )
        {
            inner_indexes.push_back( index0 );
            inner_indexes.push_back( index1 );
            inner_indexes.push_back( index2 );
        }

        Assortment( SwapType st, int32_t index0, int32_t index1, int32_t index2, int32_t index3 , std::vector<int32_t> new_order )
            : swap_type( st )
            , inner_indexes()
            , new_order( new_order )
        {
            inner_indexes.push_back( index0 );
            inner_indexes.push_back( index1 );
            inner_indexes.push_back( index2 );
            inner_indexes.push_back( index3 );
        }
    };

    // ------------------------------------------------------------------------------------
    // --- For each chromosome, there are 4 possible chromatids.  This implies
    // --- that there are 24 possible chromatid-to-chromosome assignment combinations.
    // --- The numbers below are the indexes of the different genomes where:
    // ---    0 => female 0
    // ---    1 => female 1
    // ---    2 => male 0
    // ---    3 => male 1
    // --- The numbers in the comments to the right indicate how the swapping of data
    // --- should end up where the first column is for the first genome, the second column
    // --- is for the second genome, and so on.  The numbers in the Assortment objects
    // --- are chosen so that the swap algorithm will copy things correctly.
    // ------------------------------------------------------------------------------------

    std::vector<Assortment> ASSORTMENTS = {
        Assortment( SwapType::TWO,     0, 0       ,{0, 1, 2, 3} ), // 0- 0, 1, 2, 3 - Do nothing
        Assortment( SwapType::TWO,     2, 3       ,{0, 1, 3, 2} ), // 1- 0, 1, 3, 2 - TWO
        Assortment( SwapType::TWO,     1, 2       ,{0, 2, 1, 3} ), // 2- 0, 2, 1, 3 - TWO
        Assortment( SwapType::THREE,   1, 2, 3    ,{0, 2, 3, 1} ), // 3- 0, 2, 3, 1 - THREE
        Assortment( SwapType::THREE,   2, 1, 3    ,{0, 3, 1, 2} ), // 4- 0, 3, 1, 2 - THREE
        Assortment( SwapType::TWO,     1, 3       ,{0, 3, 2, 1} ), // 5- 0, 3, 2, 1 - TWO
        Assortment( SwapType::TWO,     0, 1       ,{1, 0, 2, 3} ), // 6- 1, 0, 2, 3 - TWO
        Assortment( SwapType::TWO_TWO, 0, 1, 2, 3 ,{1, 0, 3, 2} ), // 7- 1, 0, 3, 2 - TWO(0,1), TWO(2,3)
        Assortment( SwapType::THREE,   0, 1, 2    ,{1, 2, 0, 3} ), // 8- 1, 2, 0, 3 - THREE
        Assortment( SwapType::FOUR,    1, 2, 3, 0 ,{1, 2, 3, 0} ), // 9- 1, 2, 3, 0 - FOUR
        Assortment( SwapType::FOUR,    1, 3, 0, 2 ,{1, 3, 0, 2} ), // 10 1, 3, 0, 2 - FOUR
        Assortment( SwapType::THREE,   0, 1, 3    ,{1, 3, 2, 0} ), // 11 1, 3, 2, 0 - THREE
        Assortment( SwapType::THREE,   0, 2, 1    ,{2, 0, 1, 3} ), // 12 2, 0, 1, 3 - THREE
        Assortment( SwapType::FOUR,    2, 0, 3, 1 ,{2, 0, 3, 1} ), // 13 2, 0, 3, 1 - FOUR
        Assortment( SwapType::TWO,     0, 2       ,{2, 1, 0, 3} ), // 14 2, 1, 0, 3 - TWO
        Assortment( SwapType::THREE,   0, 2, 3    ,{2, 1, 3, 0} ), // 15 2, 1, 3, 0 - THREE
        Assortment( SwapType::TWO_TWO, 0, 2, 1, 3 ,{2, 3, 0, 1} ), // 16 2, 3, 0, 1 - TWO(0,2) TWO(1,3)
        Assortment( SwapType::FOUR,    2, 3, 1, 0 ,{2, 3, 1, 0} ), // 17 2, 3, 1, 0 - FOUR
        Assortment( SwapType::FOUR,    3, 0, 1, 2 ,{3, 0, 1, 2} ), // 18 3, 0, 1, 2 - THREE
        Assortment( SwapType::THREE,   0, 3, 1    ,{3, 0, 2, 1} ), // 19 3, 0, 2, 1 - THREE
        Assortment( SwapType::THREE,   0, 3, 2    ,{3, 1, 0, 2} ), // 20 3, 1, 0, 2 - THREE
        Assortment( SwapType::TWO,     0, 3       ,{3, 1, 2, 0} ), // 23 3, 1, 2, 0 - TWO
        Assortment( SwapType::FOUR,    3, 2, 0, 1 ,{3, 2, 0, 1} ), // 24 3, 2, 0, 1 - FOUR
        Assortment( SwapType::TWO_TWO, 0, 3, 2, 1 ,{3, 2, 1, 0} ), // 25 3, 2, 1, 0 - TWO(0,3) TWO(2,1)
    };

    // ----------------------------------------------------------------------------------
    // --- The idea with this algorithm is to draw one random number, look up in a table
    // --- how to arrange the chromosomes into the different genomes, and do this with
    // --- a minimal amount of copying and random numbers.
    // ----------------------------------------------------------------------------------
    std::vector<int32_t> ParasiteGenome::IndependentAssortment( RANDOMBASE* pRNG,
                                                                int32_t iChromosome,
                                                                ParasiteGenomeInner* pFemale0,
                                                                ParasiteGenomeInner* pFemale1,
                                                                ParasiteGenomeInner* pMale0,
                                                                ParasiteGenomeInner* pMale1 )
    {
        int16_t assortment_index = pRNG->uniformZeroToN16( ASSORTMENTS.size() );

        if(assortment_index == 0) return {0, 1, 2, 3};

        const Assortment& rAssortment = ASSORTMENTS[ assortment_index ];

        int first_index = ParasiteGenetics::GetInstance()->GetFirstIndexOnChromosome( iChromosome );
        int last_index  = ParasiteGenetics::GetInstance()->GetLastIndexOnChromosome( iChromosome );

        ParasiteGenomeInner* pInners[] = { pFemale0, pFemale1, pMale0, pMale1 };

        switch( rAssortment.swap_type )
        {
            case SwapType::TWO:
                Swap( first_index,
                      last_index,
                      pInners[ rAssortment.inner_indexes[ 0 ] ],
                      pInners[ rAssortment.inner_indexes[ 1 ] ] );
                break;

            case SwapType::THREE:
                Swap( first_index,
                      last_index,
                      pInners[ rAssortment.inner_indexes[ 0 ] ],
                      pInners[ rAssortment.inner_indexes[ 1 ] ],
                      pInners[ rAssortment.inner_indexes[ 2 ] ] );
                break;

            case SwapType::TWO_TWO:
                Swap( first_index,
                      last_index,
                      pInners[ rAssortment.inner_indexes[ 0 ] ],
                      pInners[ rAssortment.inner_indexes[ 1 ] ] );
                Swap( first_index,
                      last_index,
                      pInners[ rAssortment.inner_indexes[ 2 ] ],
                      pInners[ rAssortment.inner_indexes[ 3 ] ] );
                break;

            case SwapType::FOUR:
                Swap( first_index,
                      last_index,
                      rAssortment.inner_indexes,
                      pInners );
                break;

            default:
                release_assert( false ); // shouldn't get here
        }

        return rAssortment.new_order;
    }

    std::vector<int32_t> ParasiteGenome::STATIC_SWAP_ns;
    std::vector<int32_t> ParasiteGenome::STATIC_SWAP_ar;
    bool                 ParasiteGenome::STATIC_SWAP_Initialized = false;
    std::list<Crossover> ParasiteGenome::STATIC_TEST_CROSSOVERS;
    
    void ParasiteGenome::TEST_SetCrossovers( const std::list<Crossover>& rCrossovers )
    {
        STATIC_TEST_CROSSOVERS = rCrossovers;
    }

    void ParasiteGenome::ClearStatics()
    {
        STATIC_SWAP_Initialized = false;
        STATIC_SWAP_ns.clear();
        STATIC_SWAP_ar.clear();
        STATIC_TEST_CROSSOVERS.clear();
    }

    void ParasiteGenome::InitializeStaticSwap()
    {
        if( !STATIC_SWAP_Initialized )
        {
            STATIC_SWAP_Initialized = true;
            int32_t num_bp = ParasiteGenetics::GetInstance()->GetNumBasePairs();
            STATIC_SWAP_ns.resize( num_bp, 0 );
            STATIC_SWAP_ar.resize( num_bp, 0 );
        }
    }

    // ------------------------------------------------------------
    // --- This is really just your basic swap between two objects.
    // ------------------------------------------------------------
    void ParasiteGenome::Swap( int32_t indexFirst,
                               int32_t indexLast,
                               ParasiteGenomeInner* pInner1,
                               ParasiteGenomeInner* pInner2 )
    {
        InitializeStaticSwap();

        if( (indexLast+1) < indexFirst ) return;

        size_t num_bytes_to_copy = sizeof( int32_t ) * (indexLast - indexFirst + 1);

        memcpy( &(STATIC_SWAP_ns[ indexFirst ]),                &(pInner2->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(STATIC_SWAP_ar[ indexFirst ]),                &(pInner2->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        memcpy( &(pInner2->m_NucleotideSequence[ indexFirst ]), &(pInner1->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(pInner2->m_AlleleRoots[ indexFirst ]),        &(pInner1->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        memcpy( &(pInner1->m_NucleotideSequence[ indexFirst ]), &(STATIC_SWAP_ns[ indexFirst ]),                num_bytes_to_copy );
        memcpy( &(pInner1->m_AlleleRoots[ indexFirst ]),        &(STATIC_SWAP_ar[ indexFirst ]),                num_bytes_to_copy );
    }

    // ------------------------------------------------------------------------
    // --- This is a 3-way swap.  The algorithm is circular:
    // ---    tmp<-1
    // ---    1 <- 2
    // ---    2 <- 3
    // ---    3 <- tmp
    // ------------------------------------------------------------------------
    void ParasiteGenome::Swap( int32_t indexFirst,
                               int32_t indexLast,
                               ParasiteGenomeInner* pInner1,
                               ParasiteGenomeInner* pInner2,
                               ParasiteGenomeInner* pInner3 )
    {
        InitializeStaticSwap();

        size_t num_bytes_to_copy = sizeof( int32_t ) * (indexLast - indexFirst + 1);

        memcpy( &(STATIC_SWAP_ns[ indexFirst ]),                &(pInner1->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(STATIC_SWAP_ar[ indexFirst ]),                &(pInner1->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        memcpy( &(pInner1->m_NucleotideSequence[ indexFirst ]), &(pInner2->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(pInner1->m_AlleleRoots[ indexFirst ]),        &(pInner2->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        memcpy( &(pInner2->m_NucleotideSequence[ indexFirst ]), &(pInner3->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(pInner2->m_AlleleRoots[ indexFirst ]),        &(pInner3->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        memcpy( &(pInner3->m_NucleotideSequence[ indexFirst ]), &(STATIC_SWAP_ns[ indexFirst ]),                num_bytes_to_copy );
        memcpy( &(pInner3->m_AlleleRoots[ indexFirst ]),        &(STATIC_SWAP_ar[ indexFirst ]),                num_bytes_to_copy );
    }

    // ------------------------------------------------------------------------
    // --- This is a 4-way swap.  The key to this algorithm is the use of the
    // --- index values.  It is "circular" in that you first copy the data in the
    // --- first index into tmp, then copy the data from the genome indicated by
    // --- the index.  The genome we copied 'from' becomes the one we want to copy 'to'.
    // --- Continue this until all four indexes are used.
    // --- Note: Unlike the circular algorithm for Swap::THREE, {3, 2, 0, 1} 
    // --- does NOT mean: 2 replaces 3, 0 replaces 2, 1 replaces 0, and 3 replaces 1
    // ---  with final indecies being {1, 3, 0, 2}
    // --- it means: 3 is now in 0th index, 2 in 1st index, 0 in 2nd index, and 1 in last index
    // --- with final indecies being {3, 2, 0, 1} <---- same as argument
    // ------------------------------------------------------------------------
    void ParasiteGenome::Swap( int32_t indexFirst,
                               int32_t indexLast,
                               const std::vector<int32_t>&rInnerListIndexes,
                               ParasiteGenomeInner* pInnerList[] )
    {
        InitializeStaticSwap();

        size_t num_bytes_to_copy = sizeof( int32_t ) * (indexLast - indexFirst + 1);

        memcpy( &(STATIC_SWAP_ns[ indexFirst ]),                              &(pInnerList[0]->m_NucleotideSequence[ indexFirst ]),           num_bytes_to_copy );
        memcpy( &(STATIC_SWAP_ar[ indexFirst ]),                              &(pInnerList[0]->m_AlleleRoots[ indexFirst ]),                  num_bytes_to_copy );

        int32_t copy_to = 0;
        int32_t copy_from = rInnerListIndexes[ 0 ];
        memcpy( &(pInnerList[ copy_to ]->m_NucleotideSequence[ indexFirst ]), &(pInnerList[ copy_from ]->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(pInnerList[ copy_to ]->m_AlleleRoots[ indexFirst ]),        &(pInnerList[ copy_from ]->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        copy_to = copy_from;
        copy_from = rInnerListIndexes[ copy_from ];
        memcpy( &(pInnerList[ copy_to ]->m_NucleotideSequence[ indexFirst ]), &(pInnerList[ copy_from ]->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(pInnerList[ copy_to ]->m_AlleleRoots[ indexFirst ]),        &(pInnerList[ copy_from ]->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        copy_to = copy_from;
        copy_from = rInnerListIndexes[ copy_from ];
        memcpy( &(pInnerList[ copy_to ]->m_NucleotideSequence[ indexFirst ]), &(pInnerList[ copy_from ]->m_NucleotideSequence[ indexFirst ]), num_bytes_to_copy );
        memcpy( &(pInnerList[ copy_to ]->m_AlleleRoots[ indexFirst ]),        &(pInnerList[ copy_from ]->m_AlleleRoots[ indexFirst ]),        num_bytes_to_copy );

        copy_to = copy_from;
        memcpy( &(pInnerList[ copy_to ]->m_NucleotideSequence[ indexFirst ]), &(STATIC_SWAP_ns[ indexFirst ]),                                num_bytes_to_copy );
        memcpy( &(pInnerList[ copy_to ]->m_AlleleRoots[ indexFirst ]),        &(STATIC_SWAP_ar[ indexFirst ]),                                num_bytes_to_copy );
    }
}
