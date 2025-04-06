
#include "stdafx.h"

#include "StrainIdentityMalariaGenetics.h"
#include "Exceptions.h"
#include "Debug.h"
#include "Log.h"
#include "IArchive.h"

SETUP_LOGGING( "StrainIdentityMalariaGenetics" )


namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- InfectionSourceInfo
    // ------------------------------------------------------------------------
    InfectionSourceInfo::InfectionSourceInfo()
        : m_Time( 0.0f )
        , m_NodeID( 0 )
        , m_VectorID( 0 )
        , m_BiteID( 0 )
        , m_HumanID( 0 )
        , m_InfectionID( 0 )
        , m_GenomeID( 0 )
    {
    }

    InfectionSourceInfo::InfectionSourceInfo( float biteTime,
                                              uint32_t nodeID,
                                              uint32_t vectorID,
                                              uint32_t biteID,
                                              uint32_t humanID,
                                              uint32_t infectionID,
                                              uint32_t genomeID )
        : m_Time( biteTime )
        , m_NodeID( nodeID )
        , m_VectorID( vectorID )
        , m_BiteID( biteID )
        , m_HumanID( humanID )
        , m_InfectionID( infectionID )
        , m_GenomeID( genomeID )
    {
    }

    InfectionSourceInfo::~InfectionSourceInfo()
    {
    }

    bool InfectionSourceInfo::operator==( const InfectionSourceInfo& rThat ) const
    {
        if( this->m_Time        != rThat.m_Time        ) return false;
        if( this->m_NodeID      != rThat.m_NodeID      ) return false;
        if( this->m_VectorID    != rThat.m_VectorID    ) return false;
        if( this->m_BiteID      != rThat.m_BiteID      ) return false;
        if( this->m_HumanID     != rThat.m_HumanID     ) return false;
        if( this->m_InfectionID != rThat.m_InfectionID ) return false;
        if( this->m_GenomeID    != rThat.m_GenomeID    ) return false;

        return true;
    }

    bool InfectionSourceInfo::operator!=( const InfectionSourceInfo& rThat ) const
    {
        return !this->operator==( rThat );
    }

    void InfectionSourceInfo::SetVectorID( uint32_t vectorID )
    {
        m_VectorID = vectorID;
    }

    void InfectionSourceInfo::SetBiteID( uint32_t biteID )
    {
        m_BiteID = biteID;
    }

    void InfectionSourceInfo::serialize(IArchive& ar, InfectionSourceInfo& rInfo)
    {
        ar.startObject();
        ar.labelElement( "m_Time"        ) & rInfo.m_Time;
        ar.labelElement( "m_NodeID"      ) & rInfo.m_NodeID;
        ar.labelElement( "m_VectorID"    ) & rInfo.m_VectorID;
        ar.labelElement( "m_BiteID"      ) & rInfo.m_BiteID;
        ar.labelElement( "m_HumanID"     ) & rInfo.m_HumanID;
        ar.labelElement( "m_InfectionID" ) & rInfo.m_InfectionID;
        ar.labelElement( "m_GenomeID"    ) & rInfo.m_GenomeID;
        ar.endObject();
    }

    // ------------------------------------------------------------------------
    // --- StrainIdentityMalariaGenetics
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY( StrainIdentityMalariaGenetics )
        HANDLE_INTERFACE(IStrainIdentity)
    END_QUERY_INTERFACE_BODY( StrainIdentityMalariaGenetics )

    StrainIdentityMalariaGenetics::StrainIdentityMalariaGenetics()
        : m_Genome()
        , m_SporozoiteInfo()
        , m_FemaleGametocyteInfo()
        , m_MaleGametocyteInfo()
    {
    }

    StrainIdentityMalariaGenetics::StrainIdentityMalariaGenetics( const ParasiteGenome& rGenome )
        : m_Genome( rGenome )
        , m_SporozoiteInfo()
        , m_FemaleGametocyteInfo()
        , m_MaleGametocyteInfo()
    {
    }

    StrainIdentityMalariaGenetics::StrainIdentityMalariaGenetics( const StrainIdentityMalariaGenetics& rMaster )
        : m_Genome( rMaster.m_Genome )
        , m_SporozoiteInfo(       rMaster.m_SporozoiteInfo )
        , m_FemaleGametocyteInfo( rMaster.m_FemaleGametocyteInfo )
        , m_MaleGametocyteInfo(   rMaster.m_MaleGametocyteInfo )
    {
    }

    StrainIdentityMalariaGenetics::~StrainIdentityMalariaGenetics( void )
    {
    }

    IStrainIdentity* StrainIdentityMalariaGenetics::Clone() const
    {
        return new StrainIdentityMalariaGenetics( *this );
    }

    int  StrainIdentityMalariaGenetics::GetAntigenID(void) const
    {
        return 0;
    }

    int  StrainIdentityMalariaGenetics::GetGeneticID(void) const
    {
        release_assert( !m_Genome.IsNull() );
        return m_Genome.GetID();
    }

    void StrainIdentityMalariaGenetics::SetAntigenID(int in_antigenID)
    {
        // do nothing because it is fixed at zero
        release_assert( false );
    }

    void StrainIdentityMalariaGenetics::SetGeneticID(int in_geneticID)
    {
        release_assert( false );
        // do nothing because it is fixed in the genome
    }

    const ParasiteGenome& StrainIdentityMalariaGenetics::GetGenome() const
    {
        release_assert( !m_Genome.IsNull() );
        return m_Genome;
    }

    void StrainIdentityMalariaGenetics::SetGenome( const ParasiteGenome& rGenome )
    {
        m_Genome = rGenome;
    }

    uint32_t StrainIdentityMalariaGenetics::GetSporozoiteVectorID() const
    {
        return m_SporozoiteInfo.GetVectorID();
    }

    void StrainIdentityMalariaGenetics::SetSporozoiteVectorID( uint32_t vectorID )
    {
        m_SporozoiteInfo.SetVectorID( vectorID );
    }

    uint32_t StrainIdentityMalariaGenetics::GetSporozoiteBiteID() const
    {
        return m_SporozoiteInfo.GetBiteID();
    }

    void StrainIdentityMalariaGenetics::SetSporozoiteBiteID( uint32_t biteID )
    {
        m_SporozoiteInfo.SetBiteID( biteID );
    }

    void StrainIdentityMalariaGenetics::SetSporozoiteInfo( const InfectionSourceInfo& rInfo )
    {
        m_SporozoiteInfo = rInfo;
    }

    void StrainIdentityMalariaGenetics::SetFemaleGametocyteInfo( const InfectionSourceInfo& rInfo )
    {
        m_FemaleGametocyteInfo = rInfo;
    }

    void StrainIdentityMalariaGenetics::SetMaleGametocyteInfo( const InfectionSourceInfo& rInfo )
    {
        m_MaleGametocyteInfo = rInfo;
    }

    const InfectionSourceInfo& StrainIdentityMalariaGenetics::GetSporozoiteInfo() const
    {
        return m_SporozoiteInfo;
    }

    const InfectionSourceInfo& StrainIdentityMalariaGenetics::GetFemaleGametocyteInfo() const
    {
        return m_FemaleGametocyteInfo;
    }

    const InfectionSourceInfo& StrainIdentityMalariaGenetics::GetMaleGametocyteInfo() const
    {
        return m_MaleGametocyteInfo;
    }

    REGISTER_SERIALIZABLE(StrainIdentityMalariaGenetics);

    void StrainIdentityMalariaGenetics::serialize(IArchive& ar, StrainIdentityMalariaGenetics* obj)
    {
        StrainIdentityMalariaGenetics& strain = *obj;

        ar.labelElement( "m_Genome"               ) & strain.m_Genome;
        ar.labelElement( "m_SporozoiteInfo"       ) & strain.m_SporozoiteInfo;
        ar.labelElement( "m_FemaleGametocyteInfo" ) & strain.m_FemaleGametocyteInfo;
        ar.labelElement( "m_MaleGametocyteInfo"   ) & strain.m_MaleGametocyteInfo;
    }
}