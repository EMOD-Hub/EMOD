
#include "stdafx.h"

#include "ReportVectorStats.h"
#include "report_params.rc"
#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "ReportUtilities.h"
#include "INodeContext.h"
#include "IVectorCohort.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "ReportVectorStats" )

namespace Kernel
{
    // ----------------------------------------
    // --- VectorStats Methods
    // ----------------------------------------
    VectorStats::VectorStats()
        : m_IncludeWolbachia(false)
        , m_IncludeMicrosporidia(false)
        , m_IncludeGestation( false )
        , m_IncludeDeathByState( false )
        , state_counts()
        , death_counts()
        , sum_age_at_death()
        , num_gestating_queue(8,0)
        , larvae_to_immature_counts(0)
        , sum_dur_lar_to_imm(0.0)
        , num_gestating_begin( 0 )
        , num_gestating_end( 0 )
        , num_looking_to_feed( 0 )
        , num_fed_count( 0 )
        , num_attempt_feed_indoor( 0 )
        , num_attempt_feed_outdoor( 0 )
        , num_attempt_but_not_feed( 0 )
        , new_eggs_count( 0 )
        , indoor_bites_count(0)
        , indoor_bites_count_infectious(0)
        , outdoor_bites_count(0)
        , outdoor_bites_count_infectious(0)
        , new_adults(0)
        , unmated_adults(0)
        , dead_before(0)
        , dead_indoor(0)
        , dead_outdoor(0)
        , available_habitat_per_species()
        , egg_crowding_correction_per_species()
        , wolbachia_counts()
        , microsporidia_counts_by_state()
    {
        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            state_counts.push_back( 0 );
            death_counts.push_back( 0 );
            sum_age_at_death.push_back( 0.0 );
            microsporidia_counts_by_state.push_back( 0 );
        }
    }

    VectorStats::~VectorStats()
    {
    }

    void VectorStats::Initialize( const std::vector<std::string>&  rSpeciesList,
                                  bool includeWolbachia,
                                  bool includeMicrosporidia,
                                  bool includeGestation,
                                  bool includeDeathByState )
    {
        for( auto sp : rSpeciesList )
        {
            available_habitat_per_species.insert( std::make_pair( sp, 0.0f ) );
            egg_crowding_correction_per_species.insert( std::make_pair( sp, 0.0f ) );
        }
        m_IncludeWolbachia = includeWolbachia;
        m_IncludeMicrosporidia = includeMicrosporidia;
        if( m_IncludeWolbachia )
        {
            for( int i = 0; i < VectorWolbachia::pairs::count(); ++i )
            {
                wolbachia_counts.push_back(0);
            }
        }
        m_IncludeGestation = includeGestation;
        m_IncludeDeathByState = includeDeathByState;
    }

    void VectorStats::ResetCounters()
    {
        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            state_counts[ i ] = 0;
            death_counts[ i ] = 0;
            sum_age_at_death[ i ] = 0.0f;
            microsporidia_counts_by_state[ i ] = 0;
        }
        larvae_to_immature_counts = 0;
        sum_dur_lar_to_imm = 0.0;

        if( m_IncludeGestation )
        {
            for( int i = 0; i < num_gestating_queue.size(); ++i )
            {
                num_gestating_queue[ i ] = 0;
            }
            num_gestating_begin = 0;
            num_gestating_end = 0;
            num_looking_to_feed = 0;
            num_fed_count = 0;
            num_attempt_feed_indoor = 0;
            num_attempt_feed_outdoor = 0;
            num_attempt_but_not_feed = 0;
        }
        new_eggs_count = 0;
        indoor_bites_count = 0;
        indoor_bites_count_infectious = 0;
        outdoor_bites_count = 0;
        outdoor_bites_count_infectious = 0;
        new_adults = 0;
        unmated_adults = 0;
        dead_before = 0;
        dead_indoor = 0;
        dead_outdoor = 0;

        for( auto& entry : available_habitat_per_species )
        {
            entry.second = 0.0;
        }
        for( auto& entry : egg_crowding_correction_per_species )
        {
            entry.second = 0.0;
        }
        if( m_IncludeWolbachia )
        {
            for( int i = 0; i < VectorWolbachia::pairs::count(); ++i )
            {
                wolbachia_counts[ i ] = 0;
            }
        }
    }

    void VectorStats::CollectData( const IVectorPopulationReporting* vp )
    {
        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            state_counts[ i ] += vp->getCount( VectorStateEnum::Enum( i ) );
        }

        if( m_IncludeDeathByState )
        {
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                VectorStateEnum::Enum state = VectorStateEnum::Enum( i );
                death_counts[ i ] += vp->getDeathCount( state );
                sum_age_at_death[ i ] += vp->getSumAgeAtDeath( state );
            }
            larvae_to_immature_counts = vp->getProgressFromLarvaeToImmatureNum();
            sum_dur_lar_to_imm        = vp->getProgressFromLarvaeToImmatureSumDuration();
        }

        if( m_IncludeGestation )
        {
            const std::vector<uint32_t>& r_queue = vp->getGestatingQueue();
            for( int i = 0; i < r_queue.size(); ++i )
            {
                num_gestating_queue[i] += r_queue[i];
            }
            num_gestating_begin      += vp->getNumGestatingBegin();
            num_gestating_end        += vp->getNumGestatingEnd();
            num_looking_to_feed      += vp->getNumLookingToFeed();
            num_fed_count            += vp->getNumFed();
            num_attempt_feed_indoor  += vp->getNumAttemptFeedIndoor();
            num_attempt_feed_outdoor += vp->getNumAttemptFeedOutdoor();
            num_attempt_but_not_feed += vp->getNumAttemptButNotFeed();
        }

        indoor_bites_count             += vp->getIndoorBites();
        indoor_bites_count_infectious  += vp->getInfectiousIndoorBites();
        outdoor_bites_count            += vp->getOutdoorBites();
        outdoor_bites_count_infectious += vp->getInfectiousOutdoorBites();

        new_eggs_count      += vp->getNewEggsCount();
        new_adults          += vp->getNewAdults();
        unmated_adults      += vp->getUnmatedAdults();
        dead_before         += vp->getNumDiedBeforeFeeding();
        dead_indoor         += vp->getNumDiedDuringFeedingIndoor();
        dead_outdoor        += vp->getNumDiedDuringFeedingOutdoor();

        const std::string& species = vp->get_SpeciesID();
        for( auto habitat : vp->GetHabitats() )
        {
            available_habitat_per_species[ species ] += habitat->GetCurrentLarvalCapacity() - habitat->GetTotalLarvaCount( CURRENT_TIME_STEP );
            egg_crowding_correction_per_species[ species ] += habitat->GetEggCrowdingCorrection();
        }
        if( m_IncludeWolbachia )
        {
            for( int i = 0; i < VectorWolbachia::pairs::count(); ++i )
            {
                wolbachia_counts[ i ] += vp->getWolbachiaCount( VectorWolbachia::Enum(VectorWolbachia::pairs::get_values()[i]) );
            }
        }
        if( m_IncludeMicrosporidia )
        {
            // start strain_index at 1 to skip those without microsporidia

            std::vector<std::string> strain_names = vp->GetMicrosporidiaStrainNames();
            for( int strain_index = 1; strain_index < strain_names.size(); ++strain_index )
            {
                for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
                {
                    VectorStateEnum::Enum state = VectorStateEnum::Enum( i );
                    microsporidia_counts_by_state[ i ] += vp->getMicrosporidiaCount( strain_index, state );
                }
            }
        }
    }

    std::string VectorStats::GetHeader( bool stratifyBySpecies, const std::vector<std::string>& rSpeciesList ) const
    {
        std::stringstream header;

        header << "VectorPopulation" ;
        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            header << "," << VectorStateEnum::pairs::get_keys()[ i ];
        }
        if( m_IncludeGestation )
        {
            header
                   << "," << "NumLookingToFeed"
                   << "," << "NumFedCount"
                   << "," << "NumGestatingBegin"
                   << "," << "NumGestatingEnd"
                   << "," << "NumAttemptFeedIndoor"
                   << "," << "NumAttemptFeedOutdoor"
                   << "," << "NumAttemptButNotFeed"
                ;
        }
        header << "," << "NewEggsCount" 
               << "," << "IndoorBitesCount"
               << "," << "IndoorBitesCount-Infectious"
               << "," << "OutdoorBitesCount"
               << "," << "OutdoorBitesCount-Infectious"
               << "," << "NewAdults"
               << "," << "UnmatedAdults"
               << "," << "DiedBeforeFeeding"
               << "," << "DiedDuringFeedingIndoor"
               << "," << "DiedDuringFeedingOutdoor"
            ;

        if( m_IncludeDeathByState )
        {
            header 
                   << ",NumDiedInfectious"
                   << ",NumDiedInfected"
                   << ",NumDiedAdults"
                   << ",NumDiedMale"
                   << ",AvgAgeAtDeathInfectious"
                   << ",AvgAgeAtDeathInfected"
                   << ",AvgAgeAtDeathAdults"
                   << ",AvgAgeAtDeathMale"
                   << ",AvgDurationLarvaeToImmature ";
        }

        if( m_IncludeGestation )
        {
            for( int i = 0; i < num_gestating_queue.size(); ++i )
            {
                header << ",NumGestatingOnDay_" << i;
            }
        }

        if( m_IncludeWolbachia )
        {
            for( int i = 0; i < VectorWolbachia::pairs::count(); ++i )
            {
                header << ", " << VectorWolbachia::pairs::get_keys()[i];
            }
        }
        if( m_IncludeMicrosporidia )
        {
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                header << ",HasMicrosporidia-" << VectorStateEnum::pairs::get_keys()[ i ];
            }
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                header << ",NoMicrosporidia-" << VectorStateEnum::pairs::get_keys()[ i ];
            }
        }

        if( stratifyBySpecies )
        {
            header << "," << "AvailableHabitat";
            header << "," << "EggCrowdingCorrection";
        }
        else
        {
            // -------------------------------------------------------------------------------
            // --- Since we are NOT stratifying by species, we want a column for each species
            // -------------------------------------------------------------------------------
            for( auto sp : rSpeciesList )
            {
                header << "," << sp << "_AvailableHabitat";
            }
            for( auto sp : rSpeciesList )
            {
                header << "," << sp << "_EggCrowdingCorrection";
            }
        }

        return header.str();
    }

    void VectorStats::WriteBaseData( std::stringstream& output )
    {
        int total_count = state_counts[ VectorStateEnum::STATE_ADULT ]
                        + state_counts[ VectorStateEnum::STATE_INFECTED ]
                        + state_counts[ VectorStateEnum::STATE_INFECTIOUS ];

        output << "," << total_count;

        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            output << "," << state_counts[ i ];
        }
        if( m_IncludeGestation )
        {
            output
                << "," << num_looking_to_feed
                << "," << num_fed_count
                << "," << num_gestating_begin
                << "," << num_gestating_end
                << "," << num_attempt_feed_indoor
                << "," << num_attempt_feed_outdoor
                << "," << num_attempt_but_not_feed
                ;
        }
        output
            << "," << new_eggs_count
            << "," << indoor_bites_count
            << "," << indoor_bites_count_infectious
            << "," << outdoor_bites_count
            << "," << outdoor_bites_count_infectious
            << "," << new_adults
            << "," << unmated_adults
            << "," << dead_before
            << "," << dead_indoor
            << "," << dead_outdoor
            ;

        if( m_IncludeDeathByState )
        {
            uint32_t num_died_infectious = death_counts[ VectorStateEnum::STATE_INFECTIOUS ];
            uint32_t num_died_infected   = death_counts[ VectorStateEnum::STATE_INFECTED   ];
            uint32_t num_died_adult      = death_counts[ VectorStateEnum::STATE_ADULT      ];
            uint32_t num_died_male       = death_counts[ VectorStateEnum::STATE_MALE       ];
            uint32_t num_lar_to_imm      = larvae_to_immature_counts ;

            float sum_age_infectious = sum_age_at_death[ VectorStateEnum::STATE_INFECTIOUS ];
            float sum_age_infected   = sum_age_at_death[ VectorStateEnum::STATE_INFECTED   ];
            float sum_age_adult      = sum_age_at_death[ VectorStateEnum::STATE_ADULT      ];
            float sum_age_male       = sum_age_at_death[ VectorStateEnum::STATE_MALE       ];
            //float sum_dur_lar_to_imm = sum_dur_lar_to_imm;

            float avg_age_infectious = (num_died_infectious > 0) ? (sum_age_infectious / float( num_died_infectious )) : 0.0;
            float avg_age_infected   = (num_died_infected   > 0) ? (sum_age_infected   / float( num_died_infected   )) : 0.0;
            float avg_age_adult      = (num_died_adult      > 0) ? (sum_age_adult      / float( num_died_adult      )) : 0.0;
            float avg_age_male       = (num_died_male       > 0) ? (sum_age_male       / float( num_died_male       )) : 0.0;
            float avg_dur_lar_to_imm = (num_lar_to_imm      > 0) ? (sum_dur_lar_to_imm / float( num_lar_to_imm      )) : 0.0;

            output
                << "," << num_died_infectious
                << "," << num_died_infected
                << "," << num_died_adult
                << "," << num_died_male
                << "," << avg_age_infectious
                << "," << avg_age_infected
                << "," << avg_age_adult
                << "," << avg_age_male
                << "," << avg_dur_lar_to_imm;
        }

        if( m_IncludeGestation )
        {
            for( int i = 0; i < num_gestating_queue.size(); ++i )
            {
                output << "," << num_gestating_queue[ i ];
            }
        }
        if( m_IncludeWolbachia )
        {
            for( int i = 0; i < VectorWolbachia::pairs::count(); ++i )
            {
                output << "," << wolbachia_counts[ i ];
            }
        }
        if( m_IncludeMicrosporidia )
        {
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                output << "," << microsporidia_counts_by_state[ i ];
            }
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                output << "," << (state_counts[ i ] - microsporidia_counts_by_state[ i ]);
            }
        }
    }

    void VectorStats::WriteData( const suids::suid& node_suid, const std::string& species, std::stringstream& output )
    {
        WriteBaseData( output );

        // -------------------------------------------------------------------------------
        // --- Since we ARE stratifying by species, we write one column for each of these
        // -------------------------------------------------------------------------------
        output << "," << available_habitat_per_species[ species ]
               << "," << egg_crowding_correction_per_species[ species ];
    }

    void VectorStats::WriteData( const suids::suid& node_suid,
                                 const std::vector<std::string>& rSpeciesList,
                                 std::stringstream& output )
    {
        WriteBaseData( output );

        // ------------------------------------------------------------------------------
        // --- since were not stratifying by species, we write a column for each species.
        // ------------------------------------------------------------------------------
        for( auto sp : rSpeciesList )
        {
            output << "," << available_habitat_per_species[ sp ];
        }
        for( auto sp : rSpeciesList )
        {
            output << "," << egg_crowding_correction_per_species[ sp ];
        }
    }

    uint32_t VectorStats::GetNumInfectiousBitesGivenIndoor() const
    {
        return indoor_bites_count_infectious;
    }

    uint32_t VectorStats::GetNumInfectiousBitesGivenOutdoor() const
    {
        return outdoor_bites_count_infectious;
    }

    // ----------------------------------------
    // --- ReportVectorStats Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportVectorStats, BaseTextReportEvents )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportVectorStats, BaseTextReportEvents )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportVectorStats )
#endif

    ReportVectorStats::ReportVectorStats()
        : ReportVectorStats( "ReportVectorStats.csv" )
    {
    }

    ReportVectorStats::ReportVectorStats( const std::string& rReportName )
        : BaseTextReportEvents( rReportName )
        , species_list()
        , stratify_by_species(false)
        , include_wolbachia( false )
        , include_gestation( false )
        , stats()
        , num_infectious_bites_received(0)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );

        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportVectorStats::~ReportVectorStats()
    {
    }

    bool ReportVectorStats::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Species_List",                   &species_list,           RVS_Species_List_DESC_TEXT );
        initConfigTypeMap( "Stratify_By_Species",            &stratify_by_species,    RVS_Stratify_By_Species_DESC_TEXT, false );
        initConfigTypeMap( "Include_Wolbachia_Columns",      &include_wolbachia,      RVS_Include_Wolbachia_Columns_DESC_TEXT, false );
        initConfigTypeMap( "Include_Microsporidia_Columns",  &include_microsporidia,  RVS_Include_Microsporidia_Columns_DESC_TEXT, false );
        initConfigTypeMap( "Include_Gestation_Columns",      &include_gestation,      RVS_Include_Gestation_Columns_DESC_TEXT, false );
        initConfigTypeMap( "Include_Death_By_State_Columns", &include_death_by_state, RVS_Include_Death_By_State_Columns_DESC_TEXT, false );

        bool ret = JsonConfigurable::Configure( inputJson );

        // Manually push required events into the eventTriggerList
        //eventTriggerList.push_back( EventTrigger::HIVNewlyDiagnosed );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( species_list.size() == 0 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Species_List cannot be empty." );
            }
            stats.Initialize( species_list, include_wolbachia, include_microsporidia, include_gestation, include_death_by_state );

            eventTriggerList.push_back( EventTrigger::ReceivedInfectiousBites );
        }
        return ret;
    }

    void ReportVectorStats::UpdateEventRegistration( float currentTime, 
                                                     float dt, 
                                                     std::vector<INodeEventContext*>& rNodeEventContextList,
                                                     ISimulationEventContext* pSimEventContext )
    {
        BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
    }

    std::string ReportVectorStats::GetHeader() const
    {
        std::stringstream header ;

        header << "Time"
               << "," << "NodeID";

        if( stratify_by_species )
        {
            header << "," << "Species";
        }
        header << "," << "Population";

        header << "," << stats.GetHeader( stratify_by_species, species_list );

        if( !stratify_by_species )
        {
            header << ",NumInfectousBitesGiven";
            header << ",NumInfectousBitesReceived";
            header << ",InfectiousBitesGivenMinusReceived";
        }

        return header.str();
    }

    void ReportVectorStats::LogNodeData( Kernel::INodeContext* pNC )
    {
        auto time      = pNC->GetTime().time ;
        auto nodeId    = pNC->GetExternalID();
        auto node_suid = pNC->GetSuid();
        auto pop       = pNC->GetStatPop();
        
        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }

        stats.ResetCounters();
        ResetOtherCounters();

        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
        // ---------------------------------------------------------------------
        // --- if the list is empty, then we want to include all of the species
        // ---------------------------------------------------------------------
        if( species_list.empty() )
        {
            for( auto vp : vector_pop_list )
            {
                species_list.push_back( vp->get_SpeciesID() );
            }
        }

        int species_index = 0;
        for( auto vp : vector_pop_list )
        {
            const std::string& species = vp->get_SpeciesID();

            if( std::find( species_list.begin(), species_list.end(), species ) == species_list.end() )
            {
                // -------------------------------------------------------------
                // --- If the current species is not in the list, then skip it.
                // -------------------------------------------------------------
                continue;
            }

            if( stratify_by_species )
            {
                stats.ResetCounters();
                ResetOtherCounters();
            }

            stats.CollectData( vp );
            CollectOtherData( vp );

            if( stratify_by_species )
            {
                WriteData( time, nodeId, node_suid, species, pop );
            }
            ++species_index;
        }

        if( !stratify_by_species )
        {
            WriteData( time, nodeId, node_suid, "", pop );
        }
    }

    void ReportVectorStats::WriteData( float time,
                                       ExternalNodeId_t nodeId,
                                       const suids::suid& rNodeSuid,
                                       const std::string& rSpecies,
                                       float humanPop )
    {
        GetOutputStream()
                   << time
            << "," << nodeId;

        if( stratify_by_species )
        {
            GetOutputStream() << "," << rSpecies;
        }

        GetOutputStream() << "," << humanPop;

        if( stratify_by_species )
        {
            stats.WriteData( rNodeSuid, rSpecies, GetOutputStream() );
        }
        else
        {
            stats.WriteData( rNodeSuid, species_list, GetOutputStream() );

            uint32_t bites_given = stats.GetNumInfectiousBitesGivenIndoor() + stats.GetNumInfectiousBitesGivenOutdoor();
            int64_t diff = int64_t( bites_given ) - int64_t( num_infectious_bites_received );

            GetOutputStream() << "," << bites_given;
            GetOutputStream() << "," << num_infectious_bites_received;
            GetOutputStream() << "," << diff;

            num_infectious_bites_received = 0;
        }


        WriteOtherData();

        GetOutputStream() << endl;
    }

    bool ReportVectorStats::notifyOnEvent( IIndividualHumanEventContext *context, 
                                            const EventTrigger& trigger )
    {
        IIndividualHumanVectorContext* p_ind_vector = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanVectorContext), (void**)&p_ind_vector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanVectorContext", "IIndividualHumanEventContext");
        }

        num_infectious_bites_received += p_ind_vector->GetNumInfectiousBites();

        return true;
    }
}