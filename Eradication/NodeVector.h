
#pragma once

#include <map>

#include "Node.h"
#include "VectorHabitat.h"
#include "IVectorPopulation.h"
#include "LarvalHabitatMultiplier.h"
#include "VectorContexts.h"

class ReportVector;
class VectorSpeciesReport;

namespace Kernel
{
    class SpatialReportVector;
    class StrainAwareTransmissionGroupsGP;

    class NodeVector : public Node, public IVectorNodeContext, public INodeVector
    {
        GET_SCHEMA_STATIC_WRAPPER(NodeVector)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        friend class ::ReportVector;
        friend class ::VectorSpeciesReport;
        friend class SpatialReportVector;

    public:
        static NodeVector *CreateNode(ISimulationContext *context, ExternalNodeId_t externalNodeId, suids::suid suid);
        virtual ~NodeVector();

        virtual bool Configure( const Configuration* config ) override;
        virtual void InitSuidGenerator(int, int) override;

        virtual IIndividualHuman* processImmigratingIndividual(IIndividualHuman*) override;
        virtual IIndividualHuman* addNewIndividual(float = 1.0f, float = 0.0f, int = 0, int = 0, float = 1.0f, float = 1.0f, float = 1.0f) override;

        virtual void PopulateFromDemographics() override;
        virtual void SetupIntranodeTransmission() override;
        virtual void BuildTransmissionRoutes( float ) override;
        virtual void SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory ) override;
        virtual void updateInfectivity(float dt = 0.0f) override;
        virtual void updatePopulationStatistics(float dt = 1.0f) override;
        virtual void accumulateIndividualPopulationStatistics(float dt, IIndividualHuman* individual) override;
        virtual void GetGroupMembershipForIndividual(const RouteList_t& route, const IPKeyValueContainer& properties, TransmissionGroupMembership_t& membershipOut) override;
        virtual void clearTransmissionGroups() override;

        virtual void SetupMigration( IMigrationInfoFactory * migration_factory, 
                                     const std::string& idreference,
                                     MigrationStructure::Enum ms,
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;

        // IVectorNodeContext methods
        virtual VectorProbabilities* GetVectorLifecycleProbabilities() override;
        virtual void                 AddHabitat( const std::string& species, IVectorHabitat* pHabitat ) override;
        virtual IVectorHabitat*      GetVectorHabitatBySpeciesAndType( const std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson) override;
        virtual VectorHabitatList_t* GetVectorHabitatsBySpecies( const std::string& species ) override;
        virtual float                GetLarvalHabitatMultiplier(VectorHabitatType::Enum type, const std::string& species ) const override;

        // INodeContext methods
        virtual void SetupEventContextHost() override;
        virtual ITransmissionGroups* CreateTransmissionGroups() override;
        virtual void UpdateTransmissionGroupPopulation(const IPKeyValueContainer& properties, float size_changes,float mc_weight) override;
        virtual void ExposeIndividual(IInfectable* candidate, TransmissionGroupMembership_t individual, float dt) override;
        // these should throw an exception because the GeneticProbability versions should be used
        virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs,
                                            float contagion_quantity,
                                            TransmissionGroupMembership_t individual,
                                            TransmissionRoute::Enum route) override;
        virtual float GetTotalContagion( void ) override;

        // INodeVector
        virtual suids::suid GetNextVectorSuid() override;
        virtual const VectorPopulationReportingList_t& GetVectorPopulationReporting() const override;
        virtual void AddVectors( const std::string& releasedSpecies,
                                 const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious ) override;
        virtual void processImmigratingVector( IVectorCohort* immigrant ) override;
        virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs,
                                            const GeneticProbability& contagion_quantity,
                                            TransmissionRoute::Enum route ) override;
        virtual void ExposeVector( IInfectable* candidate, float dt ) override;
        virtual GeneticProbability GetTotalContagionGP( TransmissionRoute::Enum route ) const override;

        void SetSortingVectors();
        void SortVectors();

    protected:
        static TransmissionGroupMembership_t human_indoor;
        static TransmissionGroupMembership_t human_outdoor;
        static TransmissionGroupMembership_t vector_indoor;
        static TransmissionGroupMembership_t vector_outdoor;

        static IndividualProperty* p_internal_IP_indoor;
        static IndividualProperty* p_internal_IP_outdoor;
        static IPKeyValueContainer vector_properties;
        static IPKeyValueContainer human_properties;

        std::map<std::string, VectorHabitatList_t> m_larval_habitats;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE: m_vector_populations, m_VectorPopulationReportingList and VectorParameters.vector_species
        // !!! need to all be in the same order.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        std::vector<IVectorPopulation*>  m_vectorpopulations;
        VectorPopulationReportingList_t m_VectorPopulationReportingList;

        VectorProbabilities* m_vector_lifecycle_probabilities;

        LarvalHabitatMultiplier larval_habitat_multiplier;

        bool vector_mortality;
        int32_t mosquito_weight;

        StrainAwareTransmissionGroupsGP* txIndoor;
        StrainAwareTransmissionGroupsGP* txOutdoor;
        suids::distributed_generator m_VectorCohortSuidGenerator;

        NodeVector();
        NodeVector(ISimulationContext *context, ExternalNodeId_t externalNodeId, suids::suid node_suid);
        virtual void Initialize() override;

        void SetVectorPopulations(void);    //default--1 population as before

        virtual void AddVectorPopulationToNode( IVectorPopulation* vp ) override;
        void VectorMigrationBasedOnFiles( float dt );
        void processEmigratingVectors( float dt );
            
        virtual IIndividualHuman *createHuman( suids::suid id, float MCweight, float init_age, int gender) override;

        /* clorton virtual */ const SimulationConfig *params() /* clorton override */;
        IVectorSimulationContext *context() const; // N.B. this is returning a non-const context because of the PostMigratingVector function

        virtual IVectorPopulation* CreateVectorPopulation( VectorSamplingType::Enum vectorSamplingType,
                                                           int speciesIndex,
                                                           int32_t populationPerSpecies );
                                                           
        DECLARE_SERIALIZABLE(NodeVector);

    private:
        virtual INodeContext *getContextPointer() override { return static_cast<INodeContext*>(this); }
        virtual void propagateContextToDependents() override;
    };
} // end namespace Kernel
