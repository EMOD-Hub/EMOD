
#include "stdafx.h"

#include "VectorCohortIndividualMalariaGenetics.h"
#include "IParasiteCohort.h"
#include "ParasiteGenetics.h"
#include "MalariaGeneticsContexts.h"
#include "StrainIdentityMalariaGenetics.h"
#include "Vector.h"
#include "VectorTraitModifiers.h"
#include "Log.h"
#include "Debug.h"
#include "VectorGene.h"


SETUP_LOGGING( "VectorCohortIndividualMalariaGenetics" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( VectorCohortIndividualMalariaGenetics, VectorCohortIndividual )
        HANDLE_INTERFACE( IVectorCohortIndividualMalariaGenetics )
    END_QUERY_INTERFACE_DERIVED( VectorCohortIndividualMalariaGenetics, VectorCohortIndividual )

    VectorCohortIndividualMalariaGenetics::VectorCohortIndividualMalariaGenetics()
        : VectorCohortIndividual()
        , m_NewOocystCohorts()
        , m_OocystCohorts()
        , m_SporozoiteCohorts()
        , m_InfectiousToAdult( false )
        , m_InfectiousToInfected( false )
        , m_NumMaturingOccysts( 0 )
        , m_SumOccystDuration( 0.0 )
        , m_pParasiteIdGenerator( nullptr )
    {
    }

    VectorCohortIndividualMalariaGenetics::VectorCohortIndividualMalariaGenetics( uint32_t vectorID,
                                                                                  VectorStateEnum::Enum _state,
                                                                                  float _age,
                                                                                  float _progress,
                                                                                  float microsporidiaDuration,
                                                                                  uint32_t _initial_population,
                                                                                  const VectorGenome& rGenome,
                                                                                  int speciesIndex )
        : VectorCohortIndividual( vectorID,
                                  _state,
                                  _age,
                                  _progress,
                                  microsporidiaDuration,
                                  _initial_population,
                                  rGenome,
                                  speciesIndex )
        , m_NewOocystCohorts()
        , m_OocystCohorts()
        , m_SporozoiteCohorts()
        , m_InfectiousToAdult( false )
        , m_InfectiousToInfected( false )
        , m_NumMaturingOccysts( 0 )
        , m_SumOccystDuration( 0.0 )
        , m_pParasiteIdGenerator( nullptr )
    {
    }

    VectorCohortIndividualMalariaGenetics::~VectorCohortIndividualMalariaGenetics()
    {
        for( auto p_parasite_cohort : m_NewOocystCohorts )
        {
            delete p_parasite_cohort;
        }
        m_NewOocystCohorts.clear();
        for( auto p_parasite_cohort : m_OocystCohorts )
        {
            delete p_parasite_cohort;
        }
        m_OocystCohorts.clear();
        for( auto p_parasite_cohort : m_SporozoiteCohorts )
        {
            delete p_parasite_cohort;
        }
        m_SporozoiteCohorts.clear();
    }

    VectorCohortIndividualMalariaGenetics* 
        VectorCohortIndividualMalariaGenetics::CreateCohort( uint32_t vectorID,
                                                             VectorStateEnum::Enum _state,
                                                             float _age,
                                                             float _progress,
                                                             float microsporidiaDuration,
                                                             uint32_t _initial_population,
                                                             const VectorGenome& rGenome,
                                                             int speciesIndex )
    {
        VectorCohortIndividualMalariaGenetics* newqueue = nullptr;

        if( !_supply || (_supply->size() == 0) )
        {
            newqueue = _new_ VectorCohortIndividualMalariaGenetics( vectorID,
                                                                    _state,
                                                                    _age,
                                                                    _progress,
                                                                    microsporidiaDuration,
                                                                    _initial_population,
                                                                    rGenome,
                                                                    speciesIndex );
        }
        else
        {
            VectorCohortIndividualMalariaGenetics* ptr = static_cast<VectorCohortIndividualMalariaGenetics*>( _supply->back() );
            _supply->pop_back();
            newqueue = new(ptr) VectorCohortIndividualMalariaGenetics( vectorID,
                                                                       _state,
                                                                       _age,
                                                                       _progress,
                                                                       microsporidiaDuration,
                                                                       _initial_population,
                                                                       rGenome,
                                                                       speciesIndex );
        }
        newqueue->Initialize();

        return newqueue;
    }

    void VectorCohortIndividualMalariaGenetics::Update( RANDOMBASE* pRNG,
                                                        float dt,
                                                        const VectorTraitModifiers& rTraitModifiers,
                                                        float infectedProgressThisTimestep,
                                                        bool hadMicrosporidiaPreviously )
    {
        VectorCohortIndividual::Update( pRNG,
                                        dt,
                                        rTraitModifiers,
                                        infectedProgressThisTimestep,
                                        hadMicrosporidiaPreviously );

        UpdateSporozoites( pRNG, dt, rTraitModifiers, infectedProgressThisTimestep );
        UpdateOocysts(     pRNG, dt, rTraitModifiers, infectedProgressThisTimestep );

        UpdateVectorState();
    }

    void VectorCohortIndividualMalariaGenetics::UpdateSporozoites( RANDOMBASE* pRNG,
                                                                   float dt,
                                                                   const VectorTraitModifiers& rTraitModifiers,
                                                                   float infectedProgressThisTimestep )
    {
        for( int i = 0; i < m_SporozoiteCohorts.size(); )
        {
            IParasiteCohort* p_sporozoite_cohort = m_SporozoiteCohorts[ i ];

            float mortality_modifier = GetSporozoiteMortalityModifier( rTraitModifiers, p_sporozoite_cohort );
            p_sporozoite_cohort->Update( pRNG, dt, infectedProgressThisTimestep, mortality_modifier );

            if( p_sporozoite_cohort->GetPopulation() == 0 )
            {
                m_SporozoiteCohorts[ i ] = m_SporozoiteCohorts.back();
                m_SporozoiteCohorts.pop_back();

                delete p_sporozoite_cohort;
            }
            else
            {
                ++i;
            }
        }
    }

    void VectorCohortIndividualMalariaGenetics::UpdateOocysts( RANDOMBASE* pRNG,
                                                               float dt,
                                                               const VectorTraitModifiers& rTraitModifiers,
                                                               float infectedProgressThisTimestep )
    {
        m_NumMaturingOccysts = 0;
        m_SumOccystDuration = 0.0;

        for( int i = 0; i < m_OocystCohorts.size(); )
        {
            IParasiteCohort* p_oocyst = m_OocystCohorts[ i ];

            float modifier = GetOocystProgressModifier(rTraitModifiers, p_oocyst);
            float infected_progress = infectedProgressThisTimestep * modifier;
            p_oocyst->Update( pRNG, dt, infected_progress, 1.0 );

            if( p_oocyst->GetPopulation() == 0 )
            {
                m_OocystCohorts[ i ] = m_OocystCohorts.back();
                m_OocystCohorts.pop_back();

                delete p_oocyst;
            }
            else if( p_oocyst->GetState() == ParasiteState::SPOROZOITE )
            {
                m_OocystCohorts[ i ] = m_OocystCohorts.back();
                m_OocystCohorts.pop_back();

                m_NumMaturingOccysts += 1;
                m_SumOccystDuration += p_oocyst->GetOocystDuration();

                std::vector<IParasiteCohort*> new_parasite_cohorts;
                // ------------------------------------------------------------------------------------------------------------------------------
                // --- This ParasiteCohort will be the first of the cohorts created during recombination and so its genome may change.
                // --- Hence, we get a copy of the genome before it is changed.
                // ------------------------------------------------------------------------------------------------------------------------------
                const ParasiteGenome r_pg_mom = p_oocyst->GetGenome();
                p_oocyst->Recombination( pRNG, m_pParasiteIdGenerator,  new_parasite_cohorts );
                if (EnvPtr->Log->SimpleLogger::IsLoggingEnabled(Logger::VALIDATION, _module, _log_level_enabled_array))
                {
                    const ParasiteGenome& r_pg_kid = p_oocyst->GetGenome();
                    const ParasiteGenome& r_pg_dad = p_oocyst->GetMaleGametocyteGenome();
                    LOG_VALID_F( "created %d new sporozoite genome %s %u - mom genome %s %u dad genome %s %u\n",
                                 p_oocyst->GetPopulation(),
                                 r_pg_kid.GetBarcode().c_str(), r_pg_kid.GetID(),
                                 r_pg_mom.GetBarcode().c_str(), r_pg_mom.GetID(),
                                 r_pg_dad.GetBarcode().c_str(), r_pg_dad.GetID() );
                    for( auto p_new_pc : new_parasite_cohorts )
                    {
                        LOG_VALID_F( "created %d new sporozoite genome %s %u - mom genome %s %u dad genome %s %u\n",
                                     p_new_pc->GetPopulation(),
                                     p_new_pc->GetGenome().GetBarcode().c_str(), p_new_pc->GetGenome().GetID(),
                                     r_pg_mom.GetBarcode().c_str(), r_pg_mom.GetID(),
                                     r_pg_dad.GetBarcode().c_str(), r_pg_dad.GetID() );
                    }
                }

                if( p_oocyst->GetPopulation() == 0 )
                {
                    delete p_oocyst;
                }
                else
                {
                    m_SporozoiteCohorts.push_back( p_oocyst );
                }
                for( auto p_new_pc : new_parasite_cohorts )
                {
                    if( p_new_pc->GetPopulation() == 0 )
                    {
                        delete p_new_pc;
                    }
                    else
                    {
                        bool merged = false;
                        for( auto p_existing_pc : m_SporozoiteCohorts )
                        {
                            if( p_existing_pc->Merge( *p_new_pc ) )
                            {
                                merged = true;
                                delete p_new_pc;
                                break;
                            }
                        }
                        if( !merged )
                        {
                            m_SporozoiteCohorts.push_back( p_new_pc );
                        }
                    }
                }
            }
            else
            {
                ++i;
            }
        }

        m_OocystCohorts.insert( m_OocystCohorts.end(), m_NewOocystCohorts.begin(), m_NewOocystCohorts.end() );
        m_NewOocystCohorts.clear();
    }

    void VectorCohortIndividualMalariaGenetics::UpdateVectorState()
    {
        VectorStateEnum::Enum prev_state = state;

        state = VectorStateEnum::STATE_ADULT;

        if( m_OocystCohorts.size() > 0 )
        {
            state = VectorStateEnum::STATE_INFECTED;
        }

        if( m_SporozoiteCohorts.size() > 0 )
        {
            state = VectorStateEnum::STATE_INFECTIOUS;
        }

        m_InfectiousToAdult = false;
        m_InfectiousToInfected = false;
        if( (prev_state == VectorStateEnum::STATE_INFECTIOUS) && (state == VectorStateEnum::STATE_ADULT) )
        {
            m_InfectiousToAdult = true;
        }
        else if( (prev_state == VectorStateEnum::STATE_INFECTIOUS) && (state == VectorStateEnum::STATE_INFECTED) )
        {
            m_InfectiousToInfected = true;
        }
    }

    std::vector<IParasiteCohort*> VectorCohortIndividualMalariaGenetics::GetSporozoitesForBite( RANDOMBASE* pRNG,
                                                                                                IParasiteIdGenerator* pIdGenerator,
                                                                                                float transmissionModifier )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // FPG-TODO: figure out how to sue the "transmissionModifier"
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        std::vector<IParasiteCohort*> sporozoites_in_bite;
        if( m_SporozoiteCohorts.size() == 0 )
        {
            return sporozoites_in_bite;
        }

        // ----------------------------------------------------------------------------------------------
        // --- Sporozoite Transmission by Anopheles Freeborni and Anopheles Gambiae Experimentally Infected With Plasmodium Falciparum
        // --- by J C Beier 1, M S Beier, J A Vaughan, C B Pumpuni, J R Davis, B H Noden
        // --- "The GM number of sporozoites transmitted was 4.5 for An.gambiae and 5.4 for An.stephensi.
        // --- Overall, 86.9% of the mosquitoes transmitted from one to 25 sporozoites, and
        // --- only 6.6% transmitted over 100 sporozoites( maximum = 369 )."
        // ----------------------------------------------------------------------------------------------
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! FPG TODO - Jon found a paper that talked about sporozoite load vs propbability of infection.
        // !!! The paper involved infecting mice.
        // !!! We don't take into account what happens if the vector has a small number of sporozoites.
        // !!! For example, how is the number_in_bite impacted withn the total number of sporozoites is < 10.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        uint32_t number_in_bite = ParasiteGenetics::GetInstance()->GetNumSporozoitesInBite( pRNG );

        // -------------------------------------------------------------------
        // --- Determine the fraction sporozoites of the total in each cohort
        // -------------------------------------------------------------------
        float total_sporozites = 0.0;
        for( auto p_ipc : m_SporozoiteCohorts )
        {
            total_sporozites += p_ipc->GetPopulation();
        }

        std::vector<float> fraction_in_cohorts;
        for( auto p_ipc : m_SporozoiteCohorts )
        {
            float fraction = float(p_ipc->GetPopulation()) / total_sporozites;
            fraction_in_cohorts.push_back( fraction );
        }

        // ---------------------------------------------------------------
        // --- Use the multinomial to determine the number of sporozoites
        // --- from each cohort in the bite
        // ---------------------------------------------------------------
        std::vector<uint64_t> number_in_cohort = pRNG->multinomial_approx( number_in_bite, fraction_in_cohorts );

        // ---------------------------------------------------------------
        // --- Extract and determine the number of sporozites from each
        // --- cohort that are in the bite.  Remember each cohort probably
        // --- has a different genome.
        // ---------------------------------------------------------------
        for( int i = 0; i < m_SporozoiteCohorts.size(); ++i )
        {
            if( number_in_cohort[ i ] > 0 )
            {
                IParasiteCohort* p_ipc_in_bite = m_SporozoiteCohorts[ i ]->Split( pIdGenerator->GetNextParasiteSuid().data, number_in_cohort[ i ] );
                sporozoites_in_bite.push_back( p_ipc_in_bite );

                LOG_VALID_F("sporozoites in bite cohort id %u  genome %s  population %u \n",
                             m_SporozoiteCohorts[i]->GetID(),
                             m_SporozoiteCohorts[i]->GetGenome().GetBarcode().c_str(),
                             p_ipc_in_bite->GetPopulation() );
            }
        }
        return sporozoites_in_bite;
    }

    void GetPopulations( const std::vector<IParasiteCohort*>& rParasites, std::vector<uint64_t>& rPops, uint64_t& rTotalGams )
    {
        for( int i = 0; i < rParasites.size(); ++i )
        {
            uint32_t pop = rParasites[ i ]->GetPopulation();
            rPops[ i ] = pop;
            rTotalGams += pop;
        }
    }

    void CreateListOfCohortsForPairing( const std::vector<IParasiteCohort*>& rCohorts,
                                        const std::vector<uint32_t>& numInCohortMating,
                                        std::vector<IParasiteCohort*>& cohortsToPair )
    {
        for( int i = 0; i < numInCohortMating.size(); ++i )
        {
            IParasiteCohort* p_ipc = rCohorts.at( i );
            for( uint32_t j = 0; j < numInCohortMating[ i ]; ++j )
            {
                cohortsToPair.push_back( p_ipc );
            }
        }
    }

    void CalculateFraction( uint64_t totalGams, const std::vector<uint64_t>& rGams, std::vector<float>& rFractionGams )
    {
        for( auto num_gams : rGams )
        {
            rFractionGams.push_back( float(num_gams) / float(totalGams) );
        }
    }

    bool VectorCohortIndividualMalariaGenetics::ExtractGametocytes( RANDOMBASE* pRNG,
                                                                    float currentTime,
                                                                    ExternalNodeId_t nodeId,
                                                                    uint32_t vectorId,
                                                                    uint32_t biteId,
                                                                    IParasiteIdGenerator* pIdGenerator,
                                                                    const GametocytesInPerson& rGametocytesInPerson )
    {
        // ------------------------------------------------------------------------------------------
        // --- Assuming that if this method is called that the person being bit is infectious enough
        // --- to cause the vector to get infected and that the vector should become infected.
        // ------------------------------------------------------------------------------------------
        if( ( (rGametocytesInPerson.p_mature_gametocytes_female->size() == 0) &&
              (rGametocytesInPerson.p_mature_gametocytes_male->size()   == 0)) ||
            (rGametocytesInPerson.infectiousness == 0.0) )
        {
            release_assert( false );
        }

        // ----------------------------------------------------------------------------------
        // --- Determine the percentage of each group of gametocytes (i.e. different genome)
        // ----------------------------------------------------------------------------------
        uint64_t total_gams = 0;
        std::vector<uint64_t> female_gams_in_person( rGametocytesInPerson.p_mature_gametocytes_female->size(), 0 );
        std::vector<uint64_t>   male_gams_in_person( rGametocytesInPerson.p_mature_gametocytes_male->size(), 0 );
        GetPopulations( *(rGametocytesInPerson.p_mature_gametocytes_female), female_gams_in_person, total_gams );
        GetPopulations( *(rGametocytesInPerson.p_mature_gametocytes_male  ),   male_gams_in_person, total_gams );

        std::vector<float> fraction_gams;
        fraction_gams.reserve( female_gams_in_person.size() + male_gams_in_person.size() );
        CalculateFraction( total_gams, female_gams_in_person, fraction_gams );
        CalculateFraction( total_gams, male_gams_in_person,   fraction_gams );

        // --------------------------------------------------------------------------------------
        // --- Determine the number of gametocytes that the vector got in the blood meal.
        // --- We are not really concerned about modeling this since there is no data to
        // --- fit this to.  However, we do care that the number of gametocytes in the bloodmeal
        // --- are proportional to the number in the person.  And we REALLY care about the
        // --- the type of gametocytes going into the oocysts.
        // --------------------------------------------------------------------------------------
        uint64_t num_gametocytes_in_bloodmeal = uint64_t( MICROLITERS_PER_BLOODMEAL *
                                                          rGametocytesInPerson.inv_microliters_blood *
                                                          float(total_gams) );
        if( num_gametocytes_in_bloodmeal < 2 )
        {
            // Since we assume the vector got infected, she had to get at least two gametocytes
            // (one male and one female - they have to mate to create an oocyst)
            num_gametocytes_in_bloodmeal = 2;
        }

        // -----------------------------------------------------------------------------------
        // --- Use the multinomial to determine from which groups/cohort that the gametocytes
        // --- in the blood meal came from.
        // -----------------------------------------------------------------------------------
        // ????????????????????????????????????????????????????????????????????????????????????
        // ??? FPG-TODO - Can't decide if we really want to enforce that a male and female get
        // ??? selected. Infectiousness is based on female density, but if there are no males,
        // ??? then the vector really shouldn't be getting infected.
        // ????????????????????????????????????????????????????????????????????????????????????
        std::vector<uint64_t> gams_in_bloodmeal_per_cohort = pRNG->multinomial_approx( num_gametocytes_in_bloodmeal, fraction_gams );

        // --------------------------------------------------------------------------
        // --- Separate out into male and female lists and get totals of each gender
        // --------------------------------------------------------------------------
        uint32_t total_female_gams_in_bloodmeal = 0;
        std::vector<uint32_t> female_games_in_bloodmeal( female_gams_in_person.size(), 0 );
        for( int i = 0; i < female_gams_in_person.size(); ++i )
        {
            female_games_in_bloodmeal[ i ] = uint32_t( gams_in_bloodmeal_per_cohort[ i ] );
            total_female_gams_in_bloodmeal += female_games_in_bloodmeal[ i ];
        }
        uint32_t total_male_gams_in_bloodmeal = 0;
        std::vector<uint32_t> male_games_in_bloodmeal( male_gams_in_person.size(), 0 );
        for( int i = female_gams_in_person.size(); i < gams_in_bloodmeal_per_cohort.size(); ++i )
        {
            int j = i - female_gams_in_person.size();
            male_games_in_bloodmeal[ j ] = uint32_t( gams_in_bloodmeal_per_cohort[ i ] );
            total_male_gams_in_bloodmeal += male_games_in_bloodmeal[ j ];
        }

        // -----------------------------------------------------------------------------------
        // --- If missing one gender (probably males), then return and don't cause the vector
        // --- to get infected / get new oocysts.
        // -----------------------------------------------------------------------------------
        if( (total_female_gams_in_bloodmeal == 0) || (total_male_gams_in_bloodmeal == 0) )
        {
            return false;
        }

        // ---------------------------------------------------------------------------------------------
        // --- "Predicting the likelihood and intensity of mosquito infection from sex specific
        // --- Plasmodium falciparum gametocyte density" by John Bradley, Will Stone, Dari F Da, et al
        // ---
        // --- The data to go directly from gametocyte densities to oocysts is limited and not very good
        // --- so we need to stay with just using the Negative Binomial.  However, the reduction of num_oocysts
        // --- due to the sampled gametocytes intuitively should capture the dependency where people of low
        // --- densities.  In any case, sticking with it for now but may have to change it later if we learn
        // --- something new.
        // ---------------------------------------------------------------------------------------------
        uint32_t num_oocysts = ParasiteGenetics::GetInstance()->GetNumOocystsFromBite( pRNG );
        LOG_VALID_F("num_oocysts = %u\n", num_oocysts);

        // ---------------------------------------------------------------------------------
        // --- Since each oocyst requires a male-female pair of gametocytes to mate, then
        // --- X oocysts implies we need X female gametocytes and X male gametocytes.
        // --- We use the multivariate hypergeometric distribution to select the number of
        // --- gametocytes from each cohort.  We then convert those numbers to a list of
        // --- cohorts so we can then pair the cohorts and have them mate.
        // --- NOTE: We use the hypergeometric because we assume the numbers of gametocytes
        // ---       are small and need to select without replacement.
        // ---------------------------------------------------------------------------------
        std::vector<uint32_t> female_gams_in_oocysts = pRNG->multivariate_hypergeometric( female_games_in_bloodmeal, num_oocysts );
        std::vector<uint32_t>   male_gams_in_oocysts = pRNG->multivariate_hypergeometric(   male_games_in_bloodmeal, num_oocysts );

        std::vector<IParasiteCohort*> females_to_pair, males_to_pair;
        CreateListOfCohortsForPairing( *(rGametocytesInPerson.p_mature_gametocytes_female),
                                       female_gams_in_oocysts,
                                       females_to_pair );
        CreateListOfCohortsForPairing( *(rGametocytesInPerson.p_mature_gametocytes_male),
                                       male_gams_in_oocysts,
                                       males_to_pair );

        // -------------------------------------------------------------
        // --- We use random_suffle() so we get random pair mating.
        // --- Randomizing one list should cause enough mixing
        // -------------------------------------------------------------
        int min_num_to_pair = min( females_to_pair.size(), males_to_pair.size() );

        if (EnvPtr->Log->SimpleLogger::IsLoggingEnabled(Logger::VALIDATION, _module, _log_level_enabled_array))
        {
            int difference = int(num_oocysts) - min_num_to_pair;
            if (difference > 0)
            {
                LOG_VALID_F("num_oocysts %d difference %d \n", int(num_oocysts), difference);
            }
            else
            {
                LOG_VALID("boom infected \n");
            }
        }

        auto myran = [ pRNG ]( int i ) { return pRNG->uniformZeroToN32( i ); };
        std::random_shuffle( females_to_pair.begin(), (females_to_pair.begin()+min_num_to_pair), myran );

        // -------------------------------------------------------------------------
        // --- Create pairs and mate - We might not have enough of the right gender
        // --- but we make as many as we can.
        // -------------------------------------------------------------------------
        bool infected = false;
        for( int i = 0; i < min_num_to_pair; ++i )
        {
            IParasiteCohort* p_gametocyte_female = females_to_pair[ i ];
            IParasiteCohort* p_gametocyte_male   = males_to_pair[ i ];

            // extract data for reporting
            const StrainIdentityMalariaGenetics* p_si_female_const = static_cast<const StrainIdentityMalariaGenetics*>(&(p_gametocyte_female->GetStrainIdentity()));
            StrainIdentityMalariaGenetics* p_si_female = const_cast<StrainIdentityMalariaGenetics*>(p_si_female_const);
            p_si_female->SetFemaleGametocyteInfo( InfectionSourceInfo( currentTime,
                                                                       nodeId,
                                                                       vectorId,
                                                                       biteId,
                                                                       p_si_female->GetFemaleGametocyteInfo().GetHumanID(),
                                                                       p_si_female->GetFemaleGametocyteInfo().GetInfectionID(),
                                                                       p_si_female->GetGenome().GetID() ) );

            const StrainIdentityMalariaGenetics* p_si_male_const = static_cast<const StrainIdentityMalariaGenetics*>(&(p_gametocyte_male->GetStrainIdentity()));
            StrainIdentityMalariaGenetics* p_si_male = const_cast<StrainIdentityMalariaGenetics*>(p_si_male_const);
            p_si_male->SetMaleGametocyteInfo( InfectionSourceInfo( currentTime,
                                                                   nodeId,
                                                                   vectorId,
                                                                   biteId,
                                                                   p_si_male->GetMaleGametocyteInfo().GetHumanID(),
                                                                   p_si_male->GetMaleGametocyteInfo().GetInfectionID(),
                                                                   p_si_male->GetGenome().GetID() ) );

            // split out 1 gametocyte because that is all we need to make the oocyst
            IParasiteCohort* p_oocyst = p_gametocyte_female->Split( pIdGenerator->GetNextParasiteSuid().data, 1 );
            p_oocyst->Mate( pRNG, *p_gametocyte_male );

            // put into "temporary" list so that these oocysts don't get updated
            // until the next timestep
            bool merged = false;
            for( auto p_existing_oocyst : m_NewOocystCohorts )
            {
                if( p_existing_oocyst->Merge( *p_oocyst ) )
                {
                    delete p_oocyst;
                    merged = true;
                    break;
                }
            }
            if( !merged )
            {
                m_NewOocystCohorts.push_back( p_oocyst );
            }
            infected = true;
        }
        return infected;
    }

    bool VectorCohortIndividualMalariaGenetics::HasStrain( const IStrainIdentity& rStrain ) const
    {
        for( auto p_ipc : m_OocystCohorts )
        {
            if( p_ipc->GetStrainIdentity().GetGeneticID() == rStrain.GetGeneticID() )
            {
                return true;
            }
        }

        for( auto p_ipc : m_SporozoiteCohorts )
        {
            if( p_ipc->GetStrainIdentity().GetGeneticID() == rStrain.GetGeneticID() )
            {
                return true;
            }
        }

        return false;
    }

    void VectorCohortIndividualMalariaGenetics::SetParasiteIdGenderator( IParasiteIdGenerator* pIdGen )
    {
        m_pParasiteIdGenerator = pIdGen;
    }

    void VectorCohortIndividualMalariaGenetics::CountSporozoiteBarcodeHashcodes( std::map<int64_t, int32_t>& rSporozoiteBarcodeHashcodeToCountMap )
    {
        for( auto p_pc : m_SporozoiteCohorts )
        {
            rSporozoiteBarcodeHashcodeToCountMap[ p_pc->GetGenome().GetBarcodeHashcode() ] += 1;
        }
    }

    uint32_t VectorCohortIndividualMalariaGenetics::GetNumParasiteCohortsOocysts() const
    {
        for( auto pc : m_OocystCohorts )
        {
            release_assert( pc->GetState() == ParasiteState::OOCYST );
            release_assert( pc->GetPopulation() > 0 );
        }
        return m_OocystCohorts.size();
    }

    uint32_t VectorCohortIndividualMalariaGenetics::GetNumParasiteCohortsSporozoites() const
    {
        for( auto pc : m_SporozoiteCohorts )
        {
            release_assert( pc->GetState() == ParasiteState::SPOROZOITE );
            release_assert( pc->GetPopulation() > 0 );
        }
        return m_SporozoiteCohorts.size();
    }

    uint32_t VectorCohortIndividualMalariaGenetics::GetNumOocysts() const
    {
        uint32_t num_oocysts = 0;
        for( auto p_pc : m_OocystCohorts )
        {
            num_oocysts += p_pc->GetPopulation();
        }
        return num_oocysts;
    }

    uint32_t VectorCohortIndividualMalariaGenetics::GetNumSporozoites() const
    {
        uint32_t num_sporozoites = 0;
        for( auto p_pc : m_SporozoiteCohorts )
        {
            num_sporozoites += p_pc->GetPopulation();
        }
        return num_sporozoites;
    }

    bool VectorCohortIndividualMalariaGenetics::ChangedFromInfectiousToAdult() const
    {
        return m_InfectiousToAdult;
    }

    bool VectorCohortIndividualMalariaGenetics::ChangedFromInfectiousToInfected() const
    {
        return m_InfectiousToInfected;
    }

    int32_t VectorCohortIndividualMalariaGenetics::GetNumMaturingOocysts() const
    {
        return m_NumMaturingOccysts;
    }

    float VectorCohortIndividualMalariaGenetics::GetSumOfDurationsOfMaturingOocysts() const
    {
        return m_SumOccystDuration;
    }

    float VectorCohortIndividualMalariaGenetics::GetOocystProgressModifier( const VectorTraitModifiers& rTraitModifiers,
                                                                            IParasiteCohort* pOocystCohort ) const
    {
        int64_t barcode_hash_A = pOocystCohort->GetGenome().GetBarcodeHashcode();
        int64_t barcode_hash_B = pOocystCohort->GetMaleGametocyteGenome().GetBarcodeHashcode();
        float modifier = rTraitModifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION,
                                                      this->GetGenome(), 
                                                      barcode_hash_A,
                                                      barcode_hash_B );
        return modifier;
    }

    float VectorCohortIndividualMalariaGenetics::GetSporozoiteMortalityModifier( const VectorTraitModifiers& rTraitModifiers,
                                                                                 IParasiteCohort* pSporoziteCohort ) const
    {
        int64_t barcode_hash = pSporoziteCohort->GetGenome().GetBarcodeHashcode();
        float modifier = rTraitModifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, this->GetGenome(), barcode_hash );
        return modifier;
    }

    std::string VectorCohortIndividualMalariaGenetics::GetLogSporozoiteInfo( const VectorGeneCollection& rGenes )
    {
        std::string return_string;
        if (m_SporozoiteCohorts.size() > 0)
        {
            std::stringstream ss;
            ss << "Vector " << GetID() << " with " << rGenes.GetGenomeName(GetGenome()) << " has sporozoites = ";
            for(int i = 0; i < m_SporozoiteCohorts.size(); ++i )
            {
                IParasiteCohort* p_sporozoite_cohort = m_SporozoiteCohorts[i];

                ss << p_sporozoite_cohort->GetID() << " " << p_sporozoite_cohort->GetGenome().GetBarcode() << " has " << p_sporozoite_cohort->GetPopulation();
                if ((i + 1) < m_SporozoiteCohorts.size())
                {
                    ss << ", ";
                }
            }
            ss << "\n";
            return_string = ss.str();
        }
        return return_string;
    }

    REGISTER_SERIALIZABLE( VectorCohortIndividualMalariaGenetics );

    void VectorCohortIndividualMalariaGenetics::serialize( IArchive& ar, VectorCohortIndividualMalariaGenetics* obj )
    {
        VectorCohortIndividual::serialize( ar, obj );
        VectorCohortIndividualMalariaGenetics& cohort = *obj;

        ar.labelElement( "m_NewOocystCohorts"  ) & cohort.m_NewOocystCohorts;
        ar.labelElement( "m_OocystCohorts"     ) & cohort.m_OocystCohorts;
        ar.labelElement( "m_SporozoiteCohorts" ) & cohort.m_SporozoiteCohorts;

        // These get reset at the beginning of eacht time step so
        // we don't need to serialize them
        //bool m_InfectiousToAdult;
        //bool m_InfectiousToInfected;
        //m_NumMaturingOccysts
        //m_SumOccystDuration
    }
}
