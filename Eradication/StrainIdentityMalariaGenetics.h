
#pragma once

#include "IStrainIdentity.h"
#include "ParasiteGenome.h"

namespace Kernel
{
    class InfectionSourceInfo
    {
    public:
        InfectionSourceInfo();
        InfectionSourceInfo( float biteTime,
                             uint32_t nodeID,
                             uint32_t vectorID,
                             uint32_t biteID,
                             uint32_t humanID,
                             uint32_t infectionID,
                             uint32_t genomeID );
        ~InfectionSourceInfo();

        bool operator==( const InfectionSourceInfo& rThat ) const;
        bool operator!=( const InfectionSourceInfo& rThat ) const;

        float    GetTime()        const { return m_Time; }
        uint32_t GetNodeID()      const { return m_NodeID; }
        uint32_t GetVectorID()    const { return m_VectorID; }
        uint32_t GetBiteID()      const { return m_BiteID; }
        uint32_t GetHumanID()     const { return m_HumanID; }
        uint32_t GetInfectionID() const { return m_InfectionID; }
        uint32_t GetGenomeID()    const { return m_GenomeID; }

        void SetVectorID( uint32_t vectorID );
        void SetBiteID( uint32_t biteID );

        static void serialize( IArchive& ar, InfectionSourceInfo& rInfo );

    private:
        float    m_Time;
        uint32_t m_NodeID;
        uint32_t m_VectorID;
        uint32_t m_BiteID;
        uint32_t m_HumanID;
        uint32_t m_InfectionID;
        uint32_t m_GenomeID;
    };

    class StrainIdentityMalariaGenetics : public IStrainIdentity
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        StrainIdentityMalariaGenetics();
        StrainIdentityMalariaGenetics( const ParasiteGenome& rGenome );
        StrainIdentityMalariaGenetics( const StrainIdentityMalariaGenetics& rMaster );
        virtual ~StrainIdentityMalariaGenetics(void);

        // IStrainIdentity methods
        virtual IStrainIdentity* Clone() const override;
        virtual int  GetAntigenID(void) const override;
        virtual int  GetGeneticID(void) const override;
        virtual void SetAntigenID( int in_antigenID ) override;
        virtual void SetGeneticID( int in_geneticID ) override;

        // other
        const ParasiteGenome& GetGenome() const;
        void SetGenome( const ParasiteGenome& rGenome );
        uint32_t GetSporozoiteVectorID() const;
        void SetSporozoiteVectorID( uint32_t vectorID );
        uint32_t GetSporozoiteBiteID() const;
        void SetSporozoiteBiteID( uint32_t biteID );

        void SetSporozoiteInfo( const InfectionSourceInfo& rInfo );
        void SetFemaleGametocyteInfo( const InfectionSourceInfo& rInfo );
        void SetMaleGametocyteInfo( const InfectionSourceInfo& rInfo );
        const InfectionSourceInfo& GetSporozoiteInfo() const;
        const InfectionSourceInfo& GetFemaleGametocyteInfo() const;
        const InfectionSourceInfo& GetMaleGametocyteInfo() const;

        DECLARE_SERIALIZABLE(StrainIdentityMalariaGenetics);

    protected:
        ParasiteGenome m_Genome;
        InfectionSourceInfo m_SporozoiteInfo;
        InfectionSourceInfo m_FemaleGametocyteInfo;
        InfectionSourceInfo m_MaleGametocyteInfo;
    };
}
