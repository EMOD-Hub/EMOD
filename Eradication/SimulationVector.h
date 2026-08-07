
#pragma once

#include <map>
#include "Simulation.h"
#include "VectorContexts.h"
#include "IVectorCohort.h"


namespace Kernel
{
    struct IVectorMigrationReporting;

    class SimulationVector : public Simulation, public IVectorSimulationContext
    {
        GET_SCHEMA_STATIC_WRAPPER( SimulationVector )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static SimulationVector *CreateSimulation();
        static SimulationVector *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationVector();

        virtual bool Configure( const Configuration* inputJson ) override;

        // IVectorSimulationContext methods
        virtual void  PostMigratingVector( const suids::suid& nodeSuid, IVectorCohort* ind ) override;
        virtual float GetNodePopulation( const suids::suid& nodeSuid ) override;
        virtual float GetAvailableLarvalHabitat( const suids::suid& nodeSuid, const std::string& rSpeciesID ) override;

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory ) override;
        virtual int  populateFromDemographics(const char* campaign_filename, const char* loadbalance_filename) override;

        // Creates reporters.  Specifies vector-species-specific reporting in addition to base reporting
        virtual void Reports_CreateBuiltIn() override;

        // INodeInfoFactory
        virtual INodeInfo* CreateNodeInfo() override;
        virtual INodeInfo* CreateNodeInfo( int rank, INodeContext* pNC ) override;

    protected:
        SimulationVector();

        virtual void Initialize(const ::Configuration *config) override;

        static bool ValidateConfiguration(const ::Configuration *config);

        // holds a vector of migrating vectors for each node rank
        vector<vector<IVectorCohort*>> migratingVectorQueues;
        vector< IVectorMigrationReporting* > vector_migration_reports;
        std::map<suids::suid, float> node_populations_map;

        virtual void resolveMigration() override;
        virtual void setupMigrationQueues() override;

        DECLARE_SERIALIZABLE(SimulationVector);

    private:

        virtual ISimulationContext *GetContextPointer() override;
    };
}
