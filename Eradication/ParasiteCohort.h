
#pragma once

#include "IParasiteCohort.h"
#include "ParasiteGenome.h"

namespace Kernel
{
    class ParasiteCohort : public IParasiteCohort
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        ParasiteCohort();
        ParasiteCohort( uint32_t parasiteID,
                        ParasiteState::Enum state,
                        const IStrainIdentity* pStrain,
                        float ageDays,
                        uint32_t pop );
        ParasiteCohort( const ParasiteCohort& rMaster, uint32_t parasiteID );
        virtual ~ParasiteCohort();

        // IParasiteCohort
        virtual uint32_t GetID() const override;
        virtual ParasiteState::Enum GetState() const override;
        virtual const IStrainIdentity& GetStrainIdentity() const override;
        virtual const ParasiteGenome& GetGenome() const override;
        virtual const ParasiteGenome& GetMaleGametocyteGenome() const override;
        virtual float GetAge() const override; // in days
        virtual uint32_t GetPopulation() const override;
        virtual void Update( RANDOMBASE* pRNG,
                             float dt,
                             float progressThisTimestep,
                             float sporozoiteMortalityModifier ) override;
        virtual bool Merge( const IParasiteCohort& rCohortToAdd ) override; // caller is responsible for deleting
        virtual void Mate( RANDOMBASE* pRNG, const IParasiteCohort& rMaleGametocytes ) override;
        virtual void Recombination( RANDOMBASE* pRNG, IParasiteIdGenerator* pIdGen, std::vector<IParasiteCohort*>& rNewCohorts ) override;
        virtual IParasiteCohort* Split( uint32_t newCohortID, uint32_t numLeaving ) override;
        virtual void SetSporozoiteBiteID( uint32_t biteID ) override;
        virtual float GetOocystDuration() const override;

    protected:
        uint32_t m_ID;
        ParasiteState::Enum m_State;
        IStrainIdentity* m_pStrainIdentity;
        ParasiteGenome m_MaleGametocyteGenome;
        float m_AgeDays;
        float m_Progress;
        float m_OocystDuration;
        uint32_t m_Population;

        ParasiteCohort( const ParasiteCohort& rMaster ) = delete;

        DECLARE_SERIALIZABLE( ParasiteCohort );
    };
}
