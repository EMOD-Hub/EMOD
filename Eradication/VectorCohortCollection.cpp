
#include "stdafx.h"
#include "VectorCohortCollection.h"
#include "VectorContexts.h"
#include "VectorCohort.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"

SETUP_LOGGING( "VectorCohortCollection" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- VectorCohortCollectionAbstract::Iterator
    // ------------------------------------------------------------------------
    VectorCohortCollectionAbstract::Iterator::Iterator( const std::map<GenomeKey, std::vector<IVectorCohort*>>::const_iterator& it_begin,
                                                       const std::map<GenomeKey, std::vector<IVectorCohort*>>::const_iterator& it_end )
        : m_MapIterator( it_begin )
        , m_MapIteratorEnd( it_end )
        , m_pCurrentVector( nullptr )
        , m_VectorIndex( 0 )
        , m_IsStdVector( false )
    {
        bool done = false;
        while( !done )
        {
            if( m_MapIterator != m_MapIteratorEnd )
            {
                m_pCurrentVector = const_cast<std::vector<IVectorCohort*>*>(&(m_MapIterator->second));
                while( (m_VectorIndex < m_pCurrentVector->size()) && ((*m_pCurrentVector)[ m_VectorIndex ] == nullptr) )
                {
                    ++m_VectorIndex;
                }
                done = (m_VectorIndex < m_pCurrentVector->size()) && ((*m_pCurrentVector)[ m_VectorIndex ] != nullptr);
            }
            else
            {
                m_pCurrentVector = nullptr;
                done = true;
            }
            if( !done )
            {
                ++m_MapIterator;
                m_VectorIndex = 0;
            }
        }
    }

    VectorCohortCollectionAbstract::Iterator::Iterator( std::vector<IVectorCohort*>* pStdVector, size_t startIndex )
        : m_MapIterator()
        , m_MapIteratorEnd()
        , m_pCurrentVector( pStdVector )
        , m_VectorIndex( startIndex )
        , m_IsStdVector( true )
    {
    }

    VectorCohortCollectionAbstract::Iterator::~Iterator()
    {
    }

    bool VectorCohortCollectionAbstract::Iterator::operator==( const Iterator& rThat ) const
    {
        if( this->m_IsStdVector != rThat.m_IsStdVector )
        {
            return false;
        }
        else if( !this->m_IsStdVector && !rThat.m_IsStdVector )
        {
            return (this->m_MapIterator    == rThat.m_MapIterator   ) &&
                   (this->m_pCurrentVector == rThat.m_pCurrentVector) &&
                   (this->m_VectorIndex    == rThat.m_VectorIndex   );
        }
        else
        {
            if( this->m_pCurrentVector != rThat.m_pCurrentVector )
            {
                return false;
            }
            else if( (this->m_VectorIndex < this->m_pCurrentVector->size()) &&
                     (rThat.m_VectorIndex < rThat.m_pCurrentVector->size()) )
            {
                return (this->m_VectorIndex == rThat.m_VectorIndex);
            }
            else if( (this->m_VectorIndex >= this->m_pCurrentVector->size()) &&
                     (rThat.m_VectorIndex >= rThat.m_pCurrentVector->size()) )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    bool VectorCohortCollectionAbstract::Iterator::operator!=( const Iterator& rThat ) const
    {
        return !operator==( rThat );
    }

    VectorCohortCollectionAbstract::Iterator& VectorCohortCollectionAbstract::Iterator::operator++()
    {
        if( m_IsStdVector && (m_VectorIndex < int32_t(m_pCurrentVector->size())) )
        {
            ++m_VectorIndex;
        }
        else if( m_MapIterator != m_MapIteratorEnd )
        {
            do
            {
                ++m_VectorIndex;
                if( m_VectorIndex >= m_pCurrentVector->size() )
                {
                    bool done = false;
                    while( !done )
                    {
                        ++m_MapIterator;
                        if( m_MapIterator != m_MapIteratorEnd )
                        {
                            m_pCurrentVector = const_cast<std::vector<IVectorCohort*>*>(&(m_MapIterator->second));
                            done = (m_pCurrentVector->size() > 0);
                        }
                        else
                        {
                            m_pCurrentVector = nullptr;
                            done = true;
                        }
                    }
                    m_VectorIndex = 0;
                }
            } while( (m_pCurrentVector != nullptr) && ((*m_pCurrentVector)[ m_VectorIndex ] == nullptr) );
        }
        return *this;
    }

    IVectorCohort* VectorCohortCollectionAbstract::Iterator::operator*()
    {
        release_assert( m_pCurrentVector );
        release_assert( m_pCurrentVector->size() > 0 );
        return m_pCurrentVector->at( m_VectorIndex );
    }

    void VectorCohortCollectionAbstract::Iterator::remove( bool usingFixedSize )
    {
        release_assert( m_pCurrentVector );
        release_assert( m_pCurrentVector->size() > 0 );
        release_assert( (0 <= m_VectorIndex) && (m_VectorIndex < m_pCurrentVector->size()) );
        if( usingFixedSize )
        {
            (*m_pCurrentVector)[ m_VectorIndex ] = nullptr;
        }
        else
        {
            (*m_pCurrentVector)[ m_VectorIndex ] = m_pCurrentVector->back();
            m_pCurrentVector->pop_back();
            --m_VectorIndex;
        }
    }

    // ------------------------------------------------------------------------
    // --- VectorCohortCollectionAbstract
    // ------------------------------------------------------------------------

    VectorCohortCollectionAbstract::VectorCohortCollectionAbstract()
        : m_pNodeVector( nullptr )
        , m_AgingEnabled( false )
    {
    }

    VectorCohortCollectionAbstract::~VectorCohortCollectionAbstract()
    {
    }

    void VectorCohortCollectionAbstract::initialize( INodeVector* pNodeVector, bool agingEnabled )
    {
        m_pNodeVector = pNodeVector;
        m_AgingEnabled = agingEnabled;
    }

    void VectorCohortCollectionAbstract::serialize( IArchive& ar, VectorCohortCollectionAbstract& collection )
    {
        std::vector<IVectorCohort*> cohort_vector;

        if( ar.IsWriter() )
        {
            for( auto pvc : collection )
            {
                cohort_vector.push_back( pvc );
            }
        }

        ar.startObject();
        ar.labelElement( "m_AgingEnabled" ) & collection.m_AgingEnabled;
        ar.labelElement( "collection" ) & cohort_vector;
        ar.endObject();

        if( ar.IsReader() )
        {
            for( auto pvc : cohort_vector )
            {
                collection.add( pvc, 0.0, false );
            }
        }
    }

    // ------------------------------------------------------------------------
    // --- VectorCohortCollectionStdVector
    // ------------------------------------------------------------------------

    VectorCohortCollectionStdVector::VectorCohortCollectionStdVector()
        : VectorCohortCollectionAbstract()
        , m_StdVector()
    {
        m_StdVector.reserve( 100000 );
    }

    VectorCohortCollectionStdVector::~VectorCohortCollectionStdVector()
    {
        clear();
    }

    VectorCohortCollectionStdVector::Iterator VectorCohortCollectionStdVector::begin() const
    {
        return Iterator( const_cast<std::vector<IVectorCohort*>*>(&m_StdVector), 0 );
    }

    VectorCohortCollectionStdVector::Iterator VectorCohortCollectionStdVector::end() const
    {
        return Iterator( const_cast<std::vector<IVectorCohort*>*>(&m_StdVector), m_StdVector.size() );
    }

    size_t VectorCohortCollectionStdVector::size() const
    {
        return m_StdVector.size();
    }

    void VectorCohortCollectionStdVector::clear()
    {
        for( auto p_cohort : m_StdVector )
        {
            delete p_cohort;
        }
        m_StdVector.clear();
    }

    void VectorCohortCollectionStdVector::add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge )
    {
        m_StdVector.push_back( pCohort );
    }

    void VectorCohortCollectionStdVector::remove( Iterator& it )
    {
        it.remove( false );
    }

    void VectorCohortCollectionStdVector::compact()
    {
    }

    void VectorCohortCollectionStdVector::updateAge( float dt )
    {
    }

    // ------------------------------------------------------------------------
    // --- VectorCohortCollectionStdMap
    // ------------------------------------------------------------------------

    VectorCohortCollectionStdMap::VectorCohortCollectionStdMap()
        : VectorCohortCollectionAbstract()
        , m_MapOfVectors()
        , m_Size( 0 )
    {
    }

    VectorCohortCollectionStdMap::~VectorCohortCollectionStdMap()
    {
        clear();
    }

    VectorCohortCollectionStdMap::Iterator VectorCohortCollectionStdMap::begin() const
    {
        return Iterator( m_MapOfVectors.begin(), m_MapOfVectors.end() );
    }

    VectorCohortCollectionStdMap::Iterator VectorCohortCollectionStdMap::end() const
    {
        return Iterator( m_MapOfVectors.end(), m_MapOfVectors.end() );
    }

    size_t VectorCohortCollectionStdMap::size() const
    {
        return m_Size;
    }

    void VectorCohortCollectionStdMap::clear()
    {
        for( Iterator it = begin(); it != end(); ++it )
        {
            const IVectorCohort* p_cohort = *it;
            delete p_cohort;
        }
        m_MapOfVectors.clear();
        m_Size = 0;
    }

    void VectorCohortCollectionStdMap::compact()
    {
        for( auto it = m_MapOfVectors.begin(); it != m_MapOfVectors.end(); )
        {
            if( it->second.size() == 0 )
            {
                m_MapOfVectors.erase( it++ );
            }
            else
            {
                ++it;
            }
        }
    }

    void VectorCohortCollectionStdMap::updateAge( float dt )
    {
    }

    // ------------------------------------------------------------------------
    // --- VectorCohortCollectionStdMapWithProgress
    // ------------------------------------------------------------------------

    VectorCohortCollectionStdMapWithProgress::VectorCohortCollectionStdMapWithProgress()
        : VectorCohortCollectionStdMap()
    {
    }

    VectorCohortCollectionStdMapWithProgress::~VectorCohortCollectionStdMapWithProgress()
    {
    }

    void VectorCohortCollectionStdMapWithProgress::add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge )
    {
        GenomeKey key( pCohort->GetGenome(), pCohort->GetMateGenome() );
        std::vector<IVectorCohort*>& r_cohort_vector = m_MapOfVectors[ key ];

        IVectorCohort *p_existing = nullptr;
        if( attemptMerge )
        {
            // -----------------------------------------------------------------------
            // --- Select the cohort that is within a time step of an existing cohort.
            // --- i.e. +/- half a day
            // -----------------------------------------------------------------------
            float delta_progress = progressThisTimestep * 0.5;

            VectorCohortVector_t::iterator it = std::find_if( r_cohort_vector.begin(),
                                                              r_cohort_vector.end(),
                                                              [ pCohort, delta_progress ]( IVectorCohort* cohort )
            {
                return (cohort->GetState() == pCohort->GetState()) &&
                       (cohort->GetAge() == pCohort->GetAge()) &&
                       ((cohort->GetProgress() - delta_progress) <= pCohort->GetProgress()) &&
                        (pCohort->GetProgress() <= (cohort->GetProgress() + delta_progress));
            } );

            if( it != r_cohort_vector.end() )
            {
                p_existing = *it;
            }
        }

        if( p_existing != nullptr )
        {
            p_existing->Merge( pCohort );
            delete pCohort;
        }
        else
        {
            r_cohort_vector.push_back( pCohort );
            ++m_Size;
        }
    }

    void VectorCohortCollectionStdMapWithProgress::remove( Iterator& it )
    {
        it.remove( false );
        --m_Size;
    }

    // ------------------------------------------------------------------------
    // --- VectorCohortCollectionStdMapWithAging
    // ------------------------------------------------------------------------

    VectorCohortCollectionStdMapWithAging::VectorCohortCollectionStdMapWithAging()
        : VectorCohortCollectionStdMap()
    {
    }

    VectorCohortCollectionStdMapWithAging::~VectorCohortCollectionStdMapWithAging()
    {
    }

    void VectorCohortCollectionStdMapWithAging::add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge )
    {
        GenomeKey key( pCohort->GetGenome(), pCohort->GetMateGenome() );
        std::vector<IVectorCohort*>& r_cohort_vector = m_MapOfVectors[ key ];

        if( r_cohort_vector.size() == 0 )
        {
            if( m_AgingEnabled )
            {
                r_cohort_vector.resize( VectorCohortAbstract::I_MAX_AGE, nullptr );
            }
            else
            {
                r_cohort_vector.resize( 1, nullptr );
            }
        }

        //release_assert( !attemptMerge );

        uint32_t i_age = uint32_t( pCohort->GetAge() );
        release_assert( i_age < r_cohort_vector.size() );
        IVectorCohort *p_existing = r_cohort_vector[ i_age ];

        if( p_existing != nullptr )
        {
            p_existing->Merge( pCohort );
            delete pCohort;
        }
        else
        {
            release_assert( r_cohort_vector[ i_age ] == nullptr );
            r_cohort_vector[ i_age ] = pCohort;
            ++m_Size;
        }
    }

    void VectorCohortCollectionStdMapWithAging::remove( Iterator& it )
    {
        it.remove( true );
        --m_Size;
    }

    void VectorCohortCollectionStdMapWithAging::updateAge( float dt )
    {
        if( m_AgingEnabled )
        {
            for( auto it = m_MapOfVectors.begin(); it != m_MapOfVectors.end(); ++it )
            {
                release_assert( it->second.size() == VectorCohort::I_MAX_AGE );
                for( int i = (it->second.size() - 1); i > 0; --i )
                {
                    if( (i == (it->second.size() - 1)) &&
                        (it->second[ i ] != nullptr) &&
                        (it->second[ i - 1 ] != nullptr) )
                    {
                        it->second[ i ]->Merge( it->second[ i - 1 ] );
                        delete it->second[ i - 1 ];
                        it->second[ i - 1 ] = nullptr;
                        --m_Size;
                    }
                    else if( (i == (it->second.size() - 1)) &&
                             (it->second[ i ] != nullptr) &&
                             (it->second[ i - 1 ] == nullptr) )
                    {
                        // do nothing
                    }
                    else
                    {
                        release_assert( it->second[ i ] == nullptr );
                        it->second[ i ] = it->second[ i - 1 ];
                        it->second[ i - 1 ] = nullptr;

                        if( it->second[ i ] != nullptr )
                        {
                            it->second[ i ]->IncreaseAge( dt );
                            release_assert( i == int( it->second[ i ]->GetAge() ) );
                        }
                    }
                }
            }
        }
    }
}
