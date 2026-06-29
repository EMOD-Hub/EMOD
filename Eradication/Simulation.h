
#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include "suids.hpp"

#include "IdmDateTime.h"
#include "IIndividualHuman.h"
#include "ISimulation.h"
#include "ISimulationContext.h"
#include "ExternalNodeId.h"
#include "NodeRankMap.h"
#include "INodeInfo.h"
#include "IReport.h"
#include "Configure.h"
#include "IdmApi.h"
#include "SerializationParameters.h"
#include "EventsForOtherNodes.h"
#include "Memory.h"


namespace Kernel
{
    class Simulation;
}

// declare friend functions 
namespace SerializedState {
    Kernel::NodeMap_t& GetNodes( Kernel::Simulation* sim );
}

#define ENABLE_DEBUG_MPI_TIMING 0  // TODO: could make this an environment setting so we don't have to recompile

namespace Kernel
{
    class RandomNumberGeneratorFactory;
    class RANDOMBASE;
    class  CampaignEvent;
    struct INodeContext;
    struct IEventCoordinator;
    struct SimulationEventContext;
    class  SimulationEventContextHost;
    struct IMigrationInfoFactory;
    class NodeDemographicsFactory;
    class ClimateFactory;
    class Node;
    class Infection;
    class IndividualHuman;

    class IDMAPI Simulation : public ISimulation, public ISimulationContext, public INodeInfoFactory, public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(Simulation)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:

        static Simulation *CreateSimulation();  // for serialization
        static Simulation *CreateSimulation(const ::Configuration *config);
        virtual ~Simulation();

        static float base_year;

        virtual bool Configure( const ::Configuration *json ) override;
        virtual bool TimeToStop() override;

        // ISimulation methods
        virtual bool  Populate() override;
        virtual void  Update(float dt) override;
        virtual int   GetSimulationTimestep() const override;
        virtual const IdmDateTime& GetSimulationTime() const override;
        virtual void CheckMemoryFailure( bool onlyCheckForFailure ) override;
        virtual const ProcessMemoryInfo& GetProcessMemory() override;
        virtual const SystemMemoryInfo& GetSystemMemory() override;
        virtual void  RegisterNewNodeObserver(void* id, Kernel::ISimulation::callback_t observer) override;
        virtual void  UnregisterNewNodeObserver(void* id) override;
        virtual void  WriteReportsData() override;

        virtual const DemographicsContext* GetDemographicsContext() const override;

        // Migration
        virtual void PostMigratingIndividualHuman(IIndividualHuman *i) override;
        virtual bool CanSupportFamilyTrips() const override;

        virtual void DistributeEventToOtherNodes( const EventTrigger& rEventTrigger, INodeQualifier* pQualifier ) override;
        virtual void UpdateNodeEvents() override;
        virtual ISimulationEventContext* GetSimulationEventContext() override;

        // Unique ID services
        virtual suids::suid GetNextInfectionSuid() override;
        virtual suids::suid GetNodeSuid( ExternalNodeId_t external_node_id ) override;
        virtual ExternalNodeId_t GetNodeExternalID( const suids::suid& rNodeSuid ) override;
        virtual uint32_t    GetNodeRank( const suids::suid& rNodeSuid ) override;

        virtual RANDOMBASE* GetRng() override; //should only be accessed by Node

        // Reporting
        virtual std::vector<IReport*>& GetReports() override;
        virtual std::vector<IReport*>& GetReportsNeedingIndividualData() override;
        virtual uint32_t GetNumNodesInSim() const override;

        // INodeInfoFactory
        virtual INodeInfo* CreateNodeInfo() override;
        virtual INodeInfo* CreateNodeInfo( int rank, INodeContext* pNC ) override;

        virtual void Initialize(const ::Configuration *config) override;
        typedef NodeMap_t::value_type NodeMapEntry_t;

    protected:
        Simulation();

        virtual void Initialize();  // for serialization

        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void Reports_CreateBuiltIn();
        virtual void Reports_CreateCustom();

        // Initialization
        virtual IMigrationInfoFactory* CreateMigrationInfoFactory ( const std::string& idreference,
                                                                    MigrationStructure::Enum ms,
                                                                    int torusSize );

        virtual void setupEventContextHost();
        virtual void setupMigrationQueues();
        void setupRng();
        void setParams( const ::Configuration *config );
        void initSimulationState();

        // Node initialization
        virtual void LoadInterventions(const char * campaignfilename, const std::vector<ExternalNodeId_t>& demographic_node_ids);
        virtual int  populateFromDemographics(const char* campaign_filename, const char* loadbalance_filename); // creates nodes from demographics input file data
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory, 
                                                 ClimateFactory *climate_factory ); // For derived Simulation classes to add correct node type
        void addNode_internal( INodeContext *node, 
                               NodeDemographicsFactory *nodedemographics_factory, 
                               ClimateFactory *climate_factory ); // Helper to add Nodes
        void initializeNode( INodeContext* node, 
                             NodeDemographicsFactory* nodedemographics_factory, 
                             ClimateFactory* climate_factory );
        int  getInitialRankFromNodeId( ExternalNodeId_t node_id ); // Need in MPI implementation

        // Migration
        virtual void resolveMigration(); // derived classes override this...

        // Campaign input file parsing
        virtual void notifyNewNodeObservers(INodeContext*);
        virtual void loadCampaignFromFile(const std::string & campaignfilename, const std::vector<ExternalNodeId_t>& demographic_node_ids);

        friend NodeMap_t& SerializedState::GetNodes( Kernel::Simulation* sim );

        SerializationBitMask_t serializationFlags;

        // Nodes
        NodeMap_t nodes;
        NodeRankMap nodeRankMap;

        std::map<int,EventsForOtherNodes> node_events_added ; // map of rank to EventsForOtherNodes
        std::map<suids::suid,std::vector<EventTrigger>> node_events_to_be_processed ; // map of node suids to list of events

        std::vector<INodeEventContext*> node_event_context_list ;

        typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
        typedef nodeid_suid_map_t::value_type nodeid_suid_pair;
        nodeid_suid_map_t nodeid_suid_map;
        nodeid_suid_map_t nodeid_suid_map_full;

        // Migration
        std::vector<std::vector<IIndividualHuman*>> migratingIndividualQueues;

        // Master copies of contained-class flags are maintained here so that they only get serialized once
        // TODO: deprecate and use SimulationConfig everywhere
        const SimulationConfig*     m_simConfigObj;
        const DemographicsContext *demographicsContext;

        // Simulation-unique ID generators for each type of child object that might exist in our system
        suids::distributed_generator infectionSuidGenerator;
        suids::distributed_generator nodeSuidGenerator;

        // Input files
        std::string campaignFilename;
        std::string loadBalanceFilename;

        // RNG services
        RANDOMBASE* rng;

        // Reporting
        std::vector<IReport*> reports;                          // Reporter container
        std::vector<IReport*> individual_data_reports;          // subset of 'reports' vector

        typedef IReport* (*tReportClassCreator)();
        tReportClassCreator reportClassCreator;                 // Reporting class factory/creator function pointer.
        tReportClassCreator binnedReportClassCreator;
        tReportClassCreator spatialReportClassCreator;
        tReportClassCreator propertiesReportClassCreator;
        tReportClassCreator demographicsReportClassCreator;
        tReportClassCreator eventReportClassCreator;
        tReportClassCreator nodeEventReportClassCreator;
        tReportClassCreator coordinatorEventReportClassCreator;
        tReportClassCreator surveillanceEventReportClassCreator;

        // Coordination of events for campaign intervention events
        std::list<IEventCoordinator*> event_coordinators;
        std::list<CampaignEvent*>     campaign_events;

        friend class SimulationEventContextHost;
        friend class Node;
        SimulationEventContextHost *event_context_host;

        // Counters
        IdmDateTime currentTime;

        // JsonConfigurable variables
        SimType::Enum sim_type;

        bool demographic_tracking;
        bool enable_spatial_output;
        bool enable_property_output;
        bool enable_default_report;
        bool enable_event_report;
        bool enable_node_event_report;
        bool enable_coordinator_event_report;
        bool enable_surveillance_event_report;
        bool enable_termination_on_zero_total_infectivity;
        std::string campaign_filename;
        std::string custom_reports_filename;
        std::string loadbalance_filename;
        bool can_support_family_trips;

        NodeDemographicsFactory* demographics_factory;
        RandomNumberGeneratorFactory* m_pRngFactory;

        MemoryGauge m_MemoryGauge;
        float min_sim_endtime;

    protected:

        void MergeNodeIdSuidBimaps( nodeid_suid_map_t&, nodeid_suid_map_t& );

        map<void*, Kernel::ISimulation::callback_t> new_node_observers;

        DECLARE_SERIALIZABLE(Simulation);
        static void serialize(IArchive&, NodeMap_t&);

    private:
        void Reports_ConfigureBuiltIn();
        void Reports_FindReportsCollectingIndividualData( float currentTime, float dt );
        void Reports_UpdateEventRegistration( float _currentTime, float dt );
        void Reports_BeginTimestep();
        void Reports_EndTimestep( float _currentTime, float dt );
        void Reports_LogNodeData( INodeContext* n );
        void PrintTimeAndPopulation();

        // Handling of passing "contexts" down to nodes, individuals, etc.
        virtual ISimulationContext *GetContextPointer();
        virtual void PropagateContextToDependents();
    };
}
