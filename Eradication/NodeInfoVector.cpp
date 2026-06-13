
#include "stdafx.h"
#include "NodeInfoVector.h"
#include "JsonObject.h"
#include "Serializer.h"
#include "Debug.h"
#include "VectorContexts.h"
#include "VectorHabitat.h"
#include "VectorPopulation.h"
#include "INodeContext.h"


namespace Kernel
{
    static void CalculateAvailableLarvalHabitat( INodeContext* pNC, std::vector<std::string>& species, std::vector<float>& capacity_vec )
    {
        INodeVector * pnv = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pnv) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext" );
        }

        for( auto pvp : pnv->GetVectorPopulationReporting() )
        {
            float total_capacity = 0.0 ;
            for( auto pHabitat : pvp->GetHabitats() )
            {
                // -----------------------------------------------------------------------------------------
                // --- DMB 1/29/16 At one time, we subtracted the current larval count (GetTotalLarvaCount)
                // --- but since this data is used in modifying vector migration behavior,
                // --- Jaline requested that it be based on just the capacity.
                // -----------------------------------------------------------------------------------------
                total_capacity += pHabitat->GetCurrentLarvalCapacity();
            }
            auto iter = std::find( species.begin(), species.end(), pvp->get_SpeciesID() );
            if( iter == species.end() )
            {
                species.push_back( pvp->get_SpeciesID() );
                capacity_vec.push_back( total_capacity );
            }
            else
            {
                int index = iter - species.begin();
                if( index < capacity_vec.size() )
                {
                    capacity_vec[ index ] = total_capacity;
                }
                else
                {
                    // it shouldn't get here
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "new species???" );
                }
            }
        }
        release_assert( species.size() == capacity_vec.size() );
    }

    NodeInfoVector::NodeInfoVector()
        : NodeInfo()
        , m_Species()
        , m_AvailableLarvalHabitat()
    {
    }

    NodeInfoVector::NodeInfoVector( int rank, INodeContext* pNC )
        : NodeInfo( rank, pNC )
        , m_Species()
        , m_AvailableLarvalHabitat()
    {
        CalculateAvailableLarvalHabitat( pNC, m_Species, m_AvailableLarvalHabitat );
    }

    NodeInfoVector::~NodeInfoVector()
    {
    }

    void NodeInfoVector::Update( INodeContext* pNC )
    {
        NodeInfo::Update( pNC );
        CalculateAvailableLarvalHabitat( pNC, m_Species, m_AvailableLarvalHabitat );
    }

    float NodeInfoVector::GetAvailableLarvalHabitat( const std::string& rSpeciesID ) const
    { 
        float available = 0.0;
        auto iter = std::find( m_Species.begin(), m_Species.end(), rSpeciesID );
        if( iter == m_Species.end() )
        {
            std::stringstream msg;
            msg << "Unknown species=" << rSpeciesID ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else
        {
            int index = iter - m_Species.begin();
            available = m_AvailableLarvalHabitat[ index ];
        }
        return available;
    }

    float NodeInfoVector::GetTotalAvailableLarvalHabitat() const
    {
        float total_habitat = 0.0 ;
        for( auto habitat : m_AvailableLarvalHabitat )
        {
            total_habitat += habitat;
        }
        return total_habitat;
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! In a scenario like 25_Madagascar that has 30000+ nodes,
    // !!! we really have to minimize the data that is being shared
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void NodeInfoVector::serialize(IArchive& ar, bool firstTime )
    {
        NodeInfo::serialize( ar, firstTime );
        if( firstTime )
        {
            ar.labelElement("m_Species") & m_Species;
        }
        ar.labelElement("m_AvailableLarvalHabitat") & m_AvailableLarvalHabitat;
    }
}
