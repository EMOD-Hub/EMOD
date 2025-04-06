
#include "stdafx.h"

#include "ParasiteCohort.h"
#include "IStrainIdentity.h"
#include "RANDOM.h"
#include "Debug.h"
#include "Log.h"
#include "IArchive.h"
#include "ParasiteGenetics.h"
#include "StrainIdentityMalariaGenetics.h"

SETUP_LOGGING( "ParasiteCohort" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( ParasiteCohort )
    END_QUERY_INTERFACE_BODY( ParasiteCohort )

    ParasiteCohort::ParasiteCohort()
        : m_ID( 0 )
        , m_State( ParasiteState::GAMETOCYTE_FEMALE )
        , m_pStrainIdentity( nullptr )
        , m_MaleGametocyteGenome()
        , m_AgeDays( 0.0f )
        , m_Progress( 0.0f )
        , m_OocystDuration( 0.0f )
        , m_Population( 0 )
    {
    }

    ParasiteCohort::ParasiteCohort( uint32_t parasiteID,
                                    ParasiteState::Enum state,
                                    const IStrainIdentity* pStrain,
                                    float ageDays,
                                    uint32_t pop )
        : m_ID( parasiteID )
        , m_State( state )
        , m_pStrainIdentity( nullptr )
        , m_MaleGametocyteGenome()
        , m_AgeDays( ageDays )
        , m_Progress( 0.0f )
        , m_OocystDuration( 0.0f )
        , m_Population( pop )
    {
        release_assert( pStrain != nullptr );
        m_pStrainIdentity = pStrain->Clone();
    }

    ParasiteCohort::ParasiteCohort( const ParasiteCohort& rMaster, uint32_t parasiteID )
        : m_ID( parasiteID )
        , m_State( rMaster.m_State )
        , m_pStrainIdentity( rMaster.m_pStrainIdentity->Clone() )
        , m_MaleGametocyteGenome( rMaster.m_MaleGametocyteGenome )
        , m_AgeDays( rMaster.m_AgeDays )
        , m_Progress( rMaster.m_Progress )
        , m_OocystDuration( rMaster.m_OocystDuration )
        , m_Population( rMaster.m_Population )
    {
    }

    ParasiteCohort::~ParasiteCohort()
    {
        delete m_pStrainIdentity;
    }

    uint32_t ParasiteCohort::GetID() const
    {
        return m_ID;
    }

    ParasiteState::Enum ParasiteCohort::GetState() const
    {
        return m_State;
    }

    const IStrainIdentity& ParasiteCohort::GetStrainIdentity() const
    {
        return *m_pStrainIdentity;
    }

    const ParasiteGenome& ParasiteCohort::GetGenome() const
    {
        release_assert( m_pStrainIdentity != nullptr );
        StrainIdentityMalariaGenetics* p_si_genetics = static_cast<StrainIdentityMalariaGenetics*>(m_pStrainIdentity);
        return p_si_genetics->GetGenome();
    }

    const ParasiteGenome& ParasiteCohort::GetMaleGametocyteGenome() const
    {
        return m_MaleGametocyteGenome;
    }

    float ParasiteCohort::GetAge() const
    {
        return m_AgeDays;
    }

    uint32_t ParasiteCohort::GetPopulation() const
    {
        return m_Population;
    }

    void ParasiteCohort::Update( RANDOMBASE* pRNG,
                                 float dt,
                                 float infectedProgressThisTimestep,
                                 float sporozoiteMortalityModifier )
    {
        m_AgeDays += dt;
        m_Progress += infectedProgressThisTimestep;
        if( m_State == ParasiteState::OOCYST )
        {
            m_OocystDuration += dt;
        }
        if (EnvPtr->Log->CheckLogLevel(Logger::VALIDATION, _module))
        {
            const char* state = ParasiteState::pairs::lookup_key( m_State );
            LOG_VALID_F("state %s progress %f  population %u  id %u  age %f \n", state, m_Progress, m_Population, m_ID, m_AgeDays);
        }
        if( m_Progress >= 1.0 )
        {
            if( m_State == ParasiteState::OOCYST )
            {
                m_State = ParasiteState::SPOROZOITE;
                // -----------------------------------------------------------
                // --- m_Population is number of oocysts until after next line
                // --- when it becomes the number of sporozoites
                // -----------------------------------------------------------
                m_Population = ParasiteGenetics::GetInstance()->ConvertOocystsToSporozoites( pRNG, dt, m_Population );
            }
            else if( m_State == ParasiteState::SPOROZOITE )
            {
                m_OocystDuration = 0.0;
                uint32_t old_pop = m_Population;
                m_Population = ParasiteGenetics::GetInstance()->ReduceSporozoitesDueToDeath( pRNG, dt, m_Population, sporozoiteMortalityModifier );
                LOG_VALID_F("Sporozoites cohort id %u initial %u remaining %u genome %s\n", m_ID, old_pop, m_Population, GetGenome().GetBarcode().c_str());
            }
        }
    }

    bool ParasiteCohort::Merge( const IParasiteCohort& rCohortToAdd )
    {
        bool can_merge = true;

        can_merge &= (this->GetAge() == rCohortToAdd.GetAge());
        can_merge &= (this->GetState() == rCohortToAdd.GetState());

        if( !ParasiteGenetics::GetInstance()->IsFPGSimulatingBaseModel() )
        {
            can_merge &= (this->GetMaleGametocyteGenome() == rCohortToAdd.GetMaleGametocyteGenome());
            can_merge &= (this->GetStrainIdentity().GetGeneticID() == rCohortToAdd.GetStrainIdentity().GetGeneticID());

            // ------------------------------------------------------------------------------------------
            // --- We are not merging cohorts together if they have different lineage so we can track it.
            // --- However, this can change behavior.  People could get more infection objects than if
            // --- we only cared about the genome.
            // ------------------------------------------------------------------------------------------
            const StrainIdentityMalariaGenetics* p_si_this = static_cast<const StrainIdentityMalariaGenetics*>(m_pStrainIdentity);
            const StrainIdentityMalariaGenetics* p_si_that = static_cast<const StrainIdentityMalariaGenetics*>(&rCohortToAdd.GetStrainIdentity());
            can_merge &= (p_si_this->GetFemaleGametocyteInfo() == p_si_that->GetFemaleGametocyteInfo());
            can_merge &= (p_si_this->GetMaleGametocyteInfo()   == p_si_that->GetMaleGametocyteInfo()  );
        }

        if( can_merge )
        {
            m_Population += rCohortToAdd.GetPopulation();
        }
        return can_merge;
    }

    void ParasiteCohort::Mate( RANDOMBASE* pRNG, const IParasiteCohort& rMaleGametocytes )
    {
        release_assert( m_State == ParasiteState::GAMETOCYTE_FEMALE );
        release_assert( rMaleGametocytes.GetState() == ParasiteState::GAMETOCYTE_MALE );

        m_State = ParasiteState::OOCYST;

        const StrainIdentityMalariaGenetics* p_si_male_const = static_cast<const StrainIdentityMalariaGenetics*>(&rMaleGametocytes.GetStrainIdentity());
        const InfectionSourceInfo& r_male_info = p_si_male_const->GetMaleGametocyteInfo();

        StrainIdentityMalariaGenetics* p_si_female = static_cast<StrainIdentityMalariaGenetics*>(m_pStrainIdentity);
        p_si_female->SetMaleGametocyteInfo( r_male_info );
        p_si_female->SetSporozoiteVectorID( p_si_female->GetFemaleGametocyteInfo().GetVectorID() );

        release_assert( p_si_male_const != nullptr );
        m_MaleGametocyteGenome = p_si_male_const->GetGenome();
    }

    void ParasiteCohort::Recombination( RANDOMBASE* pRNG, IParasiteIdGenerator* pIdGen, std::vector<IParasiteCohort*>& rNewCohorts )
    {
        ParasiteGenome genome_female = GetGenome();

        std::vector<ParasiteGenome> recombination_genomes;
        ParasiteGenome::Recombination( pRNG, genome_female, m_MaleGametocyteGenome, recombination_genomes );

        if( recombination_genomes.size() == 4 )
        {
            const std::vector<int32_t>& r_ns_female = genome_female.GetNucleotideSequence();
            const std::vector<int32_t>& r_ns_male  = m_MaleGametocyteGenome.GetNucleotideSequence();

            ParasiteCohort* p_new_2 = new ParasiteCohort( *this, pIdGen->GetNextParasiteSuid().data );
            ParasiteCohort* p_new_3 = new ParasiteCohort( *this, pIdGen->GetNextParasiteSuid().data );
            ParasiteCohort* p_new_4 = new ParasiteCohort( *this, pIdGen->GetNextParasiteSuid().data );

            static_cast<StrainIdentityMalariaGenetics*>(   this->m_pStrainIdentity)->SetGenome( recombination_genomes[ 0 ] );
            static_cast<StrainIdentityMalariaGenetics*>(p_new_2->m_pStrainIdentity)->SetGenome( recombination_genomes[ 1 ] );
            static_cast<StrainIdentityMalariaGenetics*>(p_new_3->m_pStrainIdentity)->SetGenome( recombination_genomes[ 2 ] );
            static_cast<StrainIdentityMalariaGenetics*>(p_new_4->m_pStrainIdentity)->SetGenome( recombination_genomes[ 3 ] );

            static std::vector<float> fraction_in_cohorts = { 0.25, 0.25, 0.25, 0.25 };
            std::vector<uint64_t> number_in_cohort = pRNG->multinomial_approx( m_Population, fraction_in_cohorts );

            uint32_t prev = m_Population;
            this->m_Population    = number_in_cohort[ 0 ];
            p_new_2->m_Population = number_in_cohort[ 1 ];
            p_new_3->m_Population = number_in_cohort[ 2 ];
            p_new_4->m_Population = number_in_cohort[ 3 ];

            rNewCohorts.push_back( p_new_2 );
            rNewCohorts.push_back( p_new_3 );
            rNewCohorts.push_back( p_new_4 );
        }
        else
        {
            release_assert( recombination_genomes.size() == 1 );
            release_assert( recombination_genomes[0].GetID() == genome_female.GetID() );
        }
    }

    IParasiteCohort* ParasiteCohort::Split( uint32_t newCohortID, uint32_t numLeaving )
    {
        m_Population = (numLeaving > m_Population) ? 0 : (m_Population - numLeaving);

        ParasiteCohort* p_pc = new ParasiteCohort( newCohortID,
                                                   m_State,
                                                   m_pStrainIdentity,
                                                   m_AgeDays,
                                                   numLeaving );

        return p_pc;
    }

    void ParasiteCohort::SetSporozoiteBiteID( uint32_t biteID )
    {
        release_assert( m_pStrainIdentity != nullptr );
        StrainIdentityMalariaGenetics* p_si_genetics = static_cast<StrainIdentityMalariaGenetics*>(m_pStrainIdentity);
        p_si_genetics->SetSporozoiteBiteID( biteID );
    }

    float ParasiteCohort::GetOocystDuration() const
    {
        return m_OocystDuration;
    }

    REGISTER_SERIALIZABLE( ParasiteCohort );

    void ParasiteCohort::serialize( IArchive& ar, ParasiteCohort* obj )
    {
        ParasiteCohort& cohort = *obj;

        ar.labelElement( "m_ID"                   ) & cohort.m_ID;
        ar.labelElement( "m_State"                ) & (uint32_t&)cohort.m_State;
        ar.labelElement( "m_pStrainIdentity"      ) & cohort.m_pStrainIdentity;
        ar.labelElement( "m_MaleGametocyteGenome" ) & cohort.m_MaleGametocyteGenome;
        ar.labelElement( "m_AgeDays"              ) & cohort.m_AgeDays;
        ar.labelElement( "m_Progress"             ) & cohort.m_Progress;
        ar.labelElement( "m_OocystDuration"       ) & cohort.m_OocystDuration;
        ar.labelElement( "m_Population"           ) & cohort.m_Population;
    }
}
