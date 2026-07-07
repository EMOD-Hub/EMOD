
#include "stdafx.h"

#include "SimulationSTI.h"

#include "InfectionSTI.h"
#include "NodeSTI.h"
#include "ReportSTI.h"
#include "BinnedReportSTI.h"
#include "STIReportEventRecorder.h"
#include "SusceptibilitySTI.h"
#include "SimulationConfig.h"
#include "StiObjectFactory.h"
#include "NodeInfoSTI.h"
#include "MpiDataExchanger.h"

SETUP_LOGGING( "SimulationSTI" )

static const float DEFAULT_BASE_YEAR = 2015.0f ;

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimulationSTI,SimulationSTI)

    BEGIN_QUERY_INTERFACE_DERIVED(SimulationSTI, Simulation)
        HANDLE_INTERFACE(IIdGeneratorSTI)
        HANDLE_INTERFACE(ISTISimulationContext)
    END_QUERY_INTERFACE_DERIVED(SimulationSTI, Simulation)

    SimulationSTI::SimulationSTI()
        : relationshipSuidGenerator( EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks )
        , coitalActSuidGenerator( EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks )
        , report_relationship_start(false)
        , report_relationship_end(false)
        , report_relationship_consummated(false)
        , report_transmission(false)
        , nodes_sti()
        , migrating_relationships()
        , migrating_relationships_vecmap()
    {
        initConfigTypeMap( "Report_Relationship_Start",     &report_relationship_start,      Report_Relationship_Start_DESC_TEXT, false);
        initConfigTypeMap( "Report_Relationship_End",       &report_relationship_end,        Report_Relationship_End_DESC_TEXT,   false);
        initConfigTypeMap( "Report_Transmission",           &report_transmission,            Report_Transmission_DESC_TEXT,      false);
        initConfigTypeMap( "Report_Coital_Acts",            &report_relationship_consummated,Report_Coital_Acts_DESC_TEXT,         false);

        reportClassCreator = ReportSTI::CreateReport;
        eventReportClassCreator = STIReportEventRecorder::CreateReport;
        binnedReportClassCreator = BinnedReportSTI::CreateReport;
    }

    SimulationSTI::~SimulationSTI(void)
    {
    }

    SimulationSTI *SimulationSTI::CreateSimulation()
    {
        SimulationSTI *newsimulation = _new_ SimulationSTI();
        newsimulation->Initialize();

        return newsimulation;
    }

    SimulationSTI *SimulationSTI::CreateSimulation(const ::Configuration *config)
    {
        SimulationSTI *newsimulation = nullptr;

        newsimulation = _new_ SimulationSTI();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
            }
        }

        return newsimulation;
    }

    void SimulationSTI::Initialize()
    {
        Simulation::Initialize();
    }

    void SimulationSTI::Initialize( const ::Configuration *config )
    {
        Simulation::Initialize(config);
        IndividualHumanSTI::InitializeStaticsSTI( config );
    }

    bool SimulationSTI::Configure(const Configuration* inputJson)
    {
        initConfigTypeMap( "Base_Year",  &base_year, Base_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, DEFAULT_BASE_YEAR );

        bool ret = Simulation::Configure( inputJson );
        if( ret )
        {
            LOG_INFO_F("Setting Base_Year to %f\n", base_year );
            currentTime.setBaseYear( base_year );
        }

        return ret;
    }

    void
    SimulationSTI::Reports_CreateBuiltIn()
    {
        Simulation::Reports_CreateBuiltIn();

        // ---------------------------------------------------------------------------
        // --- We check the simulation type here because this method could be called
        // --- from a subclass like SimulationHIV.  SimulationHIV will only be calling
        // --- this if the simulation type is HIV_SIM
        // ---------------------------------------------------------------------------
        if (report_relationship_start && (GET_CONFIGURABLE( SimulationConfig )->sim_type == SimType::STI_SIM))
        {
            LOG_INFO( "Using STI RelationshipStartReporter.\n" );
            reports.push_back(StiObjectFactory::CreateRelationshipStartReporter(this));
        }

        if (report_relationship_end)
        {
            LOG_INFO( "Using STI RelationshipEndReporter.\n" );
            reports.push_back(StiObjectFactory::CreateRelationshipEndReporter(this));
        }

        if (report_relationship_consummated)
        {
            LOG_INFO( "Using STI RelationshipConsummatedReporter.\n" );
            reports.push_back(StiObjectFactory::CreateRelationshipConsummatedReporter(this));
        }

        if (report_transmission && (GET_CONFIGURABLE( SimulationConfig )->sim_type == SimType::STI_SIM))
        {
            LOG_INFO( "Using STI TransmissionReporter.\n" );
            reports.push_back(StiObjectFactory::CreateTransmissionReporter(this));
        }

    }

    bool SimulationSTI::ValidateConfiguration(const ::Configuration *config)
    {
        if (!Simulation::ValidateConfiguration(config))
            return false;

        // TODO: any disease-specific validation goes here.
        // Warning: static climate parameters are not configured until after this function is called

        return true;
    }

    void SimulationSTI::addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                    suids::suid node_suid,
                                                    NodeDemographicsFactory *nodedemographics_factory,
                                                    ClimateFactory *climate_factory )
    {
        NodeSTI *node = NodeSTI::CreateNode(this, externalNodeId, node_suid);
        addNode_internal( node, nodedemographics_factory, climate_factory );
        nodes_sti[ node_suid ] = node;
    }

    int SimulationSTI::populateFromDemographics( const char* campaignfilename, const char* loadbalancefilename )
    {
        int num_nodes_this_core = Simulation::populateFromDemographics( campaignfilename, loadbalancefilename );

        // add one since NodeID's start at 1
        migrating_relationships_vecmap.resize( nodeid_suid_map_full.size() + 1 );

        return num_nodes_this_core;
    }

    suids::suid SimulationSTI::GetNextRelationshipSuid()
    {
        return relationshipSuidGenerator();
    }

    suids::suid SimulationSTI::GetNextCoitalActSuid()
    {
        return coitalActSuidGenerator();
    }

    void SimulationSTI::AddTerminatedRelationship( const suids::suid& nodeSuid, const suids::suid& relId )
    {
        INodeInfo& r_ni = nodeRankMap.GetNodeInfo( nodeSuid );
        NodeInfoSTI* p_ni_sti = dynamic_cast<NodeInfoSTI*>(&r_ni);
        p_ni_sti->AddTerminatedRelationship( relId );
    }

    bool SimulationSTI::WasRelationshipTerminatedLastTimestep( const suids::suid& relId ) const
    {
        const NodeRankMap::RankMap_t& rank_map = nodeRankMap.GetRankMap();

        for( auto& entry : rank_map )
        {
            NodeInfoSTI* p_nis = dynamic_cast<NodeInfoSTI*>(entry.second);
            release_assert( p_nis );

            if( p_nis->WasRelationshipTerminatedLastTimestep( relId ) )
            {
                return true;
            }
        }
        return false;
    }

    INodeInfo* SimulationSTI::CreateNodeInfo()
    {
        INodeInfo* pin = new NodeInfoSTI();
        return pin ;
    }

    INodeInfo* SimulationSTI::CreateNodeInfo( int rank, INodeContext* pNC )
    {
        INodeInfo* pin = new NodeInfoSTI( rank, pNC );
        return pin ;
    }

    void SimulationSTI::AddEmigratingRelationship( IRelationship* pRel )
    {
        bool add_relationship = true;

        uint32_t destination_node_id = pRel->GetMigrationDestination().data;

        auto it = migrating_relationships_vecmap[ destination_node_id ].find( pRel->GetSuid() );
        if( it != migrating_relationships_vecmap[ destination_node_id ].end() )
        {
            IRelationship* p_existing_rel = it->second;
            if( p_existing_rel->GetMigrationDestination() == pRel->GetMigrationDestination() )
            {
                // ----------------------------------------------------------------------------------
                // --- If we find an existing relationship that is migrating to the same destination,
                // --- then we want to update the existing relationship so it knows that both partners
                // --- are migrating.
                // ----------------------------------------------------------------------------------
                if( (p_existing_rel->MalePartner() == nullptr) && (pRel->MalePartner() != nullptr) )
                {
                    p_existing_rel->Pause( pRel->MalePartner(), pRel->GetMigrationDestination() );
                    add_relationship = false;
                }
                else if( (p_existing_rel->FemalePartner() == nullptr) && (pRel->FemalePartner() != nullptr) )
                {
                    p_existing_rel->Pause( pRel->FemalePartner(), pRel->GetMigrationDestination() );
                    add_relationship = false;
                }
                else if( ((pRel->MalePartner()           == nullptr) && (pRel->FemalePartner()           == nullptr)) &&
                         ((p_existing_rel->MalePartner() == nullptr) && (p_existing_rel->FemalePartner() == nullptr)) )
                {
                    // the relationship already knows that both partners are migrating so
                    // keep existing relationship and throw away the new one
                    add_relationship = false;
                }
                else
                {
                    release_assert( false );
                }
            }
        }

        if( add_relationship )
        {
            migrating_relationships_vecmap[ destination_node_id ][ pRel->GetSuid() ] = pRel;
        }
    }

    void SimulationSTI::setupMigrationQueues()
    {
        Simulation::setupMigrationQueues();
        migrating_relationships.resize( EnvPtr->MPI.NumTasks );
    }

    void SimulationSTI::resolveMigration()
    {
        WithSelfFunc to_self_func = [ this ]( int myRank )
        {
            // Don't bother to serialize locally
            for( auto iterator = migrating_relationships[ myRank ].rbegin(); iterator != migrating_relationships[ myRank ].rend(); ++iterator )
            {
                // map.at() is faster than map[] since it doesn't optionally create an entry
                IRelationship* p_rel = *iterator;
                release_assert( p_rel->GetMigrationDestination().data > 0 );
                INodeSTI* p_sti_node = nodes_sti.at( p_rel->GetMigrationDestination() );
                p_sti_node->GetRelationshipManager()->Immigrate( p_rel );
            }
        };

        SendToOthersFunc to_others_func = [ this ]( IArchive* writer, int toRank )
        {
            *writer & migrating_relationships[ toRank ];
            for( auto p_rel : migrating_relationships[ toRank ] )
            {
                delete p_rel;
            }
            migrating_relationships[ toRank ].clear();
        };

        ClearDataFunc clear_data_func = [ this ]( int rank )
        {
            migrating_relationships[ rank ].clear();
        };

        ReceiveFromOthersFunc from_others_func = [ this ]( IArchive* reader, int fromRank )
        {
            *reader & migrating_relationships[ fromRank ];
            for( auto p_rel : migrating_relationships[ fromRank ] )
            {
                release_assert( p_rel->GetMigrationDestination().data > 0 );
                INodeSTI* p_sti_node = nodes_sti.at( p_rel->GetMigrationDestination() );
                p_sti_node->GetRelationshipManager()->Immigrate( p_rel );
            }
        };

        // -------------------------------------------------------------------------------------------
        // --- Move the relationships from the vector of maps to the queues used in transfering data.
        // --- We need to check for terminate because one partner could have be migrating and the
        // --- second partner processed died.  If this happens, the relationship was migrating but
        // --- was terminated while in a migration state.
        // -------------------------------------------------------------------------------------------
        for( auto& r_id2rel_map : migrating_relationships_vecmap )
        {
            for( auto it : r_id2rel_map )
            {
                IRelationship* p_rel = it.second;
                if( p_rel->GetState() == RelationshipState::TERMINATED )
                {
                    delete p_rel;
                }
                else
                {
                    suids::suid suid = p_rel->GetMigrationDestination();
                    release_assert( suid.data > 0 );
                    int rank = nodeRankMap.GetRankFromNodeSuid( suid );
                    migrating_relationships.at( rank ).push_back( p_rel );
                }
            }
            r_id2rel_map.clear();
        }

        // --------------------------------------------------------------------------------------------------
        // --- Migrate the relationships before the people.  This allows the RelationshipManager to have all
        // --- of the relationships updated before people are transfered and need the relationships.
        // --------------------------------------------------------------------------------------------------
        MpiDataExchanger exchanger( "RelationshipMigration", to_self_func, to_others_func, from_others_func, clear_data_func );
        exchanger.ExchangeData( this->currentTime );

        Simulation::resolveMigration(); // Take care of the humans
    }
}
