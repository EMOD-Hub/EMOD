
#include "stdafx.h"
#include "NodeInfo.h"
#include "JsonObject.h"
#include "Serializer.h"
#include "INodeContext.h"

namespace Kernel
{
    NodeInfo::NodeInfo()
        : m_Suid( suids::nil_suid() )
        , m_ExternalId(0)
        , m_Rank(0)
        , m_Population( 0 )
        , m_LongitudeDegrees( 0.0 )
        , m_LatitudeDegrees( 0.0 )
    {
    }

    NodeInfo::NodeInfo( int rank, INodeContext* pNC )
        : m_Suid( pNC->GetSuid() )
        , m_ExternalId( pNC->GetExternalID() )
        , m_Rank( rank )
        , m_Population( pNC->GetStatPop() )
        , m_LongitudeDegrees( pNC->GetLongitudeDegrees() )
        , m_LatitudeDegrees( pNC->GetLatitudeDegrees() )
    {
    }

    NodeInfo::~NodeInfo()
    {
    }

    void NodeInfo::Update( INodeContext* pNC )
    {
        m_Population = pNC->GetStatPop() ;
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! In a scenario like 25_Madagascar that has 30000+ nodes,
    // !!! we really have to minimize the data that is being shared
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void NodeInfo::serialize( IArchive& ar, bool firstTime )
    {
        if( firstTime )
        {
            ar.labelElement("m_Suid"             ) & m_Suid;
            ar.labelElement("m_ExternalId"       ) & m_ExternalId;
            ar.labelElement("m_Rank"             ) & m_Rank;
            ar.labelElement("m_LongitudeDegrees" ) & m_LongitudeDegrees;
            ar.labelElement("m_LatitudeDegrees"  ) & m_LatitudeDegrees;
        }
        ar.labelElement("m_Population" ) & m_Population;
    }
}
