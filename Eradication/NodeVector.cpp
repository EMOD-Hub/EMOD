
#include "stdafx.h"

#include "Debug.h"
#include "Exceptions.h"
#include "NodeVector.h"
#include "NodeVectorEventContext.h"
#include "IndividualVector.h"
#include "VectorPopulation.h"
#include "VectorPopulationIndividual.h"
#include "VectorParameters.h"
#include "Log.h"
#include "SimulationConfig.h"
#include "ISimulationContext.h"
#include "TransmissionGroupsFactory.h"
#include "TransmissionGroupMembership.h"
#include "IMigrationInfoVector.h"
#include "Infection.h"
#include "StrainAwareTransmissionGroupsGP.h"
#include "Memory.h"

SETUP_LOGGING( "NodeVector" )

using namespace std;

#define INDOOR  "indoor"
#define OUTDOOR "outdoor"

#define HUMAN   "human"
#define VECTOR  "vector"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeVector, Node)
        HANDLE_INTERFACE(IVectorNodeContext)
        HANDLE_INTERFACE(INodeVector)
    END_QUERY_INTERFACE_DERIVED(NodeVector, Node)

    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeVector, NodeVector)

    TransmissionGroupMembership_t NodeVector::human_indoor;
    TransmissionGroupMembership_t NodeVector::human_outdoor;
    TransmissionGroupMembership_t NodeVector::vector_indoor;
    TransmissionGroupMembership_t NodeVector::vector_outdoor;

    IndividualProperty* NodeVector::p_internal_IP_indoor = nullptr;
    IndividualProperty* NodeVector::p_internal_IP_outdoor = nullptr;
    IPKeyValueContainer NodeVector::vector_properties;
    IPKeyValueContainer NodeVector::human_properties;

    NodeVector::NodeVector() 
        : Node()
        , m_larval_habitats()
        , m_vectorpopulations()
        , m_VectorPopulationReportingList()
        , m_vector_lifecycle_probabilities( VectorProbabilities::CreateVectorProbabilities() )
        , larval_habitat_multiplier()
        , vector_mortality( true )
        , mosquito_weight( 0 )
        , txIndoor( nullptr )
        , txOutdoor( nullptr )
        , m_VectorCohortSuidGenerator(0,0)
    {
        serializationFlagsDefault.set( SerializationFlags::LarvalHabitats );
        serializationFlagsDefault.set( SerializationFlags::VectorPopulation );
    }

    NodeVector::NodeVector(ISimulationContext *context, ExternalNodeId_t externalNodeId, suids::suid _suid) 
        : Node(context, externalNodeId, _suid)
        , m_larval_habitats()
        , m_vectorpopulations()
        , m_VectorPopulationReportingList()
        , m_vector_lifecycle_probabilities( VectorProbabilities::CreateVectorProbabilities() )
        , larval_habitat_multiplier()
        , vector_mortality( true )
        , mosquito_weight( 1 )
        , txIndoor( nullptr )
        , txOutdoor( nullptr )
        , m_VectorCohortSuidGenerator(0,0)
    {
        serializationFlagsDefault.set( SerializationFlags::LarvalHabitats );
        serializationFlagsDefault.set( SerializationFlags::VectorPopulation );
    }

    bool
    NodeVector::Configure(
        const Configuration * config
    )
    {
        larval_habitat_multiplier.SetExternalNodeId(externalId);

        initConfigTypeMap( "Enable_Vector_Mortality", &vector_mortality, Enable_Vector_Mortality_DESC_TEXT, true );
        initConfigTypeMap( "Mosquito_Weight", &mosquito_weight, Mosquito_Weight_DESC_TEXT, 1, 1e4, 1, "Vector_Sampling_Type", "SAMPLE_IND_VECTORS" ); // should this be renamed vector_weight?

        bool configured = Node::Configure( config );

        return configured;
    }

    void NodeVector::Initialize()
    {
        Node::Initialize();

        // when creating nodes from scratch (not from serialization)
        larval_habitat_multiplier.Initialize(); 

        if (ClimateFactory::climate_structure == ClimateStructure::CLIMATE_OFF)
        {
            // This could be either a vector sim or a malaria sim. Let's get the correct sim type for the error message.
            const char* simulation_type_name = SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type );
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_OFF", "Simulation_Type", simulation_type_name );
        }
    }

    void NodeVector::SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory )
    {
        Node::SetParameters( demographics_factory, climate_factory );

        if (demographics["NodeAttributes"].Contains("LarvalHabitatMultiplier") )
        {
            // if there are already m_larval_habitats, that means we created them via de-serialization and it's too late for this.
            if( !m_larval_habitats.empty() )
            {
                // Larval habitats already set up from de-serialization, LarvalHabitatiMultiplier will not be used.
                return;
            }
            std::istringstream config_string(demographics["NodeAttributes"].GetJsonObject().ToString());
            Configuration* config = Configuration::Load(config_string, std::string(""));
            larval_habitat_multiplier.Configure(config);
        }
        else
        {
            LOG_DEBUG("Did not find the LarvalHabitatMultiplier property in NodeAttributes.\n");
        }
    }

    void NodeVector::SetupEventContextHost()
    {
        event_context_host = _new_ NodeVectorEventContextHost(this);
    }

    NodeVector *NodeVector::CreateNode(ISimulationContext *context, ExternalNodeId_t externalNodeId, suids::suid suid)
    {
        NodeVector *newnode = _new_ NodeVector(context, externalNodeId, suid);
        newnode->Initialize();

        return newnode;
    }

    NodeVector::~NodeVector()
    {
        for (auto population : m_vectorpopulations)
        {
            delete population;
        }
        m_vectorpopulations.clear();
        m_VectorPopulationReportingList.clear();

        for (auto& entry : m_larval_habitats)
        {
            LOG_DEBUG_F("%s: Cleaning up habitats for species '%s'.\n", __FUNCTION__, entry.first.c_str() );
            for (auto& habitat : entry.second)
            {
                delete habitat;
                habitat = nullptr;
            }
        }
        m_larval_habitats.clear();

        delete txIndoor;
        delete txOutdoor;
        delete m_vector_lifecycle_probabilities;
    }

    IIndividualHuman* NodeVector::createHuman( suids::suid id, float MCweight, float init_age, int gender)
    {
        return IndividualHumanVector::CreateHuman(getContextPointer(), id, MCweight, init_age, gender);
    }

    IIndividualHuman* NodeVector::processImmigratingIndividual( IIndividualHuman* movedind)
    {
        Node::processImmigratingIndividual(movedind);

        return movedind;
    }

    IIndividualHuman* NodeVector::addNewIndividual(float MCweight, float init_age, int gender, int init_infs, float immparam, float riskparam, float mighet)
    {
        // just the base class for now
        return Node::addNewIndividual(MCweight, init_age, gender, init_infs, immparam, riskparam, mighet);
    }

    void NodeVector::SetupMigration( IMigrationInfoFactory * migration_factory, 
                               const std::string& idreference,
                               MigrationStructure::Enum ms,
                               const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        Node::SetupMigration( migration_factory, idreference, ms, rNodeIdSuidMap );

        for( auto p_vp : m_vectorpopulations )
        {
            p_vp->SetupMigration( idreference, rNodeIdSuidMap );
        }
    }

    void NodeVector::SetupIntranodeTransmission()
    {
        // create two routes (indoor and outdoor), and two groups for each route (human, mosquito).
        // future expansion of heterogeneous intra-node transmission could start here.
        CreateTransmissionGroups();

        // Making this nullptr to ensure it is not used, but txIndoor & txOutdoor
        transmissionGroups = nullptr;

        ScalingMatrix_t scalingMatrix{
            { 0.0f, 1.0f },
            { 1.0f, 0.0f } };
        txIndoor->AddProperty( /*property*/ INDOOR,  { HUMAN, VECTOR }, scalingMatrix );
        txOutdoor->AddProperty(/*property*/ OUTDOOR, { HUMAN, VECTOR }, scalingMatrix );

        LOG_DEBUG("groups added.\n");

        VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_sampling_type;
        if ( (vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_NUMBER || vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_PERCENT) &&
              InfectionConfig::number_basestrains > 1 &&
              InfectionConfig::number_substrains  > 1 )
        {
            // Ideally error messages would all be in Exceptions.cpp
            std::ostringstream msg;
            msg << "Strain tracking is only fully supported for the individual (not cohort) vector model."
                << " Simulations will run for cohort models, but all vector-to-human transmission defaults to strain=(0,0)." 
                << " Specified values for InfectionConfig::number_basestrains = " << InfectionConfig::number_basestrains << " and number_substrains = " << InfectionConfig::number_substrains
                << " are not allowed. They may only be set to 1 for the cohort model.  To use the individual vector model, switch vector sampling type to TRACK_ALL_VECTORS or SAMPLE_IND_VECTORS."
                << std::endl;
            LOG_ERR( msg.str().c_str() );
            std::ostringstream err_msg;
            err_msg << InfectionConfig::number_basestrains << " and " << InfectionConfig::number_substrains;
            throw IncoherentConfigurationException(
                __FILE__, __LINE__, __FUNCTION__,
                "InfectionConfig::number_basestrains > 1 and number_substrains > 1",
                err_msg.str().c_str(),
                "vector_sampling_type",
                VectorSamplingType::pairs::lookup_key( vector_sampling_type )
            );
        }

        LOG_DEBUG("Building indoor/outdoor human-vector groups.\n");

        BuildTransmissionRoutes( 1.0f );

        // Do once for all nodes
        if( p_internal_IP_indoor == nullptr )
        {
            std::map<std::string, float> in_out_values;
            in_out_values.insert( std::make_pair( HUMAN, 0.5f ) );
            in_out_values.insert( std::make_pair( VECTOR, 0.5f ) );

            p_internal_IP_indoor  = IPFactory::GetInstance()->AddIP( this->externalId, INDOOR,  in_out_values, false );
            p_internal_IP_outdoor = IPFactory::GetInstance()->AddIP( this->externalId, OUTDOOR, in_out_values, false );

            vector_properties.Add( p_internal_IP_indoor->GetValues<IPKeyValueContainer>().Get( "indoor:vector" ) );
            vector_properties.Add( p_internal_IP_outdoor->GetValues<IPKeyValueContainer>().Get( "outdoor:vector" ) );

            human_properties.Add( p_internal_IP_indoor->GetValues<IPKeyValueContainer>().Get( "indoor:human" ) );
            human_properties.Add( p_internal_IP_outdoor->GetValues<IPKeyValueContainer>().Get( "outdoor:human" ) );
        }

        // Note, this works, but has potential for a problem - these static membership
        // variables are shared by all (demographic) nodes on the same compute core. It is
        // just maybe possible that the HINT configuration is different for different nodes
        // and should not all be using the same static variables.
        txIndoor->GetGroupMembershipForProperties(  human_properties, NodeVector::human_indoor  );
        txOutdoor->GetGroupMembershipForProperties( human_properties, NodeVector::human_outdoor );

        txIndoor->GetGroupMembershipForProperties(  vector_properties, NodeVector::vector_indoor  );
        txOutdoor->GetGroupMembershipForProperties( vector_properties, NodeVector::vector_outdoor );
    }

    void NodeVector::GetGroupMembershipForIndividual( const RouteList_t& route,
                                                      const IPKeyValueContainer& properties,
                                                      TransmissionGroupMembership_t& membershipOut )
    {
        // do nothing because Vector sims don't support HINT
    }

    void NodeVector::clearTransmissionGroups()
    {
        txIndoor->ClearPopulationSize();
        txOutdoor->ClearPopulationSize();
    }

    void NodeVector::updateInfectivity(float dt)
    {
        clearTransmissionGroups();

        // update population stats
        updatePopulationStatistics(dt);

        // changes in larval capacities.
        // drying of larval habitat, function of temperature and humidity
        for ( auto& entry : m_larval_habitats )
        {
            LOG_DEBUG_F( "%s: Updating habitats for mosquito species %s.\n", __FUNCTION__, entry.first.c_str() );

            for ( auto habitat : entry.second )
            {
                release_assert( habitat );
                habitat->Update( dt, getContextPointer(), entry.first );
            }
        }

        // finish processing human-to-mosquito infectiousness
        txIndoor->EndUpdate();
        txOutdoor->EndUpdate();

        // don't need to update the vector populations before the first timestep
        if ( dt <= 0 ) 
            return;

        // process vector populations and get infection rate
        for (auto population : m_vectorpopulations)
        {
            release_assert(population);
            population->UpdateVectorPopulation(dt);
        }

        // do again so that humans bitten this time step get infected
        // finish processing mosquito-to-human infectiousness
        txIndoor->EndUpdate();
        txOutdoor->EndUpdate();

        // Now process the node's emigrating mosquitoes
        processEmigratingVectors( dt );
    }

    void NodeVector::accumulateIndividualPopulationStatistics( float dt, IIndividualHuman* individual )
    {
        Node::accumulateIndividualPopulationStatistics( dt, individual );

        //IIndividualHumanVectorContext* host_individual = static_cast<IndividualHumanVector*>(individual);
        IndividualHumanVector* host_individual = static_cast<IndividualHumanVector*>(individual);
        IVectorInterventionsEffects* ivie = host_individual->GetVectorInterventionEffects();

        // Host vector weight includes heterogeneous biting
        // effect of heterogeneous biting explored in Smith, D. L., F. E. McKenzie, et al. (2007). "Revisiting the basic reproductive number for malaria and its implications for malaria control." PLoS Biol 5(3): e42.
        // also in Smith, D. L., J. Dushoff, et al. (2005). "The entomological inoculation rate and Plasmodium falciparum infection in African children." Nature 438(7067): 492-495.
        float host_vector_weight = float(individual->GetMonteCarloWeight() * host_individual->GetRelativeBitingRate());

        // Accumulate the probabilities corresponding to individual vector intervention effects
        m_vector_lifecycle_probabilities->AccumulateIndividualProbabilities(ivie, host_vector_weight);
    }

    void NodeVector::updatePopulationStatistics(float dt)
    {
        // Reset vector lifecycle transition probabilities
        m_vector_lifecycle_probabilities->ResetProbabilities();

        // Update population statistics as in the base class
        Node::updatePopulationStatistics(dt);

        // Additionally normalize "Human Infectious Reservoir" reporting channel variable
        // to be consistent with previous behavior, where it is the human-to-vector infection probability
        mInfectivity = (statPop > 0) ? mInfectivity/statPop : 0;

        // Normalize lifecycle probabilities by population weight
        m_vector_lifecycle_probabilities->NormalizeIndividualProbabilities();

        // Query for node vector intervention effects interface
        INodeVectorInterventionEffects* invie = nullptr;
        if (s_OK !=  GetEventContext()->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&invie))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetEventContext()", "INodeVectorInterventionEffects", "INodeEventContext" );
        }

        // Get effects of node-level interventions on vector lifecycle probabilities
        m_vector_lifecycle_probabilities->SetNodeProbabilities(invie, dt);
    }

    void NodeVector::PopulateFromDemographics( NodeDemographicsFactory *demographics_factory )
    {
        m_VectorCohortSuidGenerator = suids::distributed_generator( GetSuid().data, demographics_factory->GetNodeIDs().size() );

        // Add the vector populations
        SetVectorPopulations();

        // Populate the people as in the base class
        Node::PopulateFromDemographics( demographics_factory );
    }

    void NodeVector::SetVectorPopulations()
    {
        if (suid.data <= 0)
        {
            int _id = suid.data;
            LOG_WARN_F("Node suid value is %d\n", _id); // EAW: throw exception?
            return;
        }
        VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE( SimulationConfig )->vector_params->vector_sampling_type;

        for( int species_index = 0; species_index < params()->vector_params->vector_species.Size(); ++species_index )
        {
            const VectorSpeciesParameters* p_vsp = params()->vector_params->vector_species[ species_index ];

            // parameter description in demographics_params.rc - InitialVectorsPerSpecies_DESC_TEXT
            int32_t population_per_species = DEFAULT_VECTOR_POPULATION_SIZE;
            if( demographics[ "NodeAttributes" ].Contains( "InitialVectorsPerSpecies" ) )
            {
                if( demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ].IsObject() )
                {
                    if( demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ].Contains( p_vsp->name ) )
                    {
                        population_per_species = demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ][ p_vsp->name ].AsInt();
                    }
                }
                else
                {
                    population_per_species = demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ].AsInt();
                }
            }
            LOG_DEBUG_F( "population_per_species = %f.\n", population_per_species );

            IVectorPopulation *vectorpopulation = CreateVectorPopulation( vector_sampling_type, species_index, population_per_species );

            release_assert( vectorpopulation );
            AddVectorPopulationToNode( vectorpopulation );

            parent->CheckMemoryFailure( false );
        }
    }

    IVectorPopulation* NodeVector::CreateVectorPopulation( VectorSamplingType::Enum vector_sampling_type,
                                                           int species_index,
                                                           int32_t population_per_species )
    {
        IVectorPopulation *vectorpopulation = nullptr;
            if (vector_sampling_type == VectorSamplingType::TRACK_ALL_VECTORS || 
                vector_sampling_type == VectorSamplingType::SAMPLE_IND_VECTORS)
            {
                // Individual mosquito model
                LOG_DEBUG( "Creating VectorPopulationIndividual instance(s).\n" );
                vectorpopulation = VectorPopulationIndividual::CreatePopulation(getContextPointer(), species_index, population_per_species, mosquito_weight);
            }
            else
            {
                // Cohort model
                LOG_DEBUG( "Creating VectorPopulation (regular) instance(s).\n" );

                vectorpopulation = VectorPopulation::CreatePopulation( getContextPointer(), species_index, population_per_species );
            }
        return vectorpopulation;
    }

    void NodeVector::AddVectors( const std::string& releasedSpecies,
                                 const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious )
    {
        bool found_existing_vector_population = false;
        for (auto population : m_vectorpopulations)
        {
            if(population->get_SpeciesID() == releasedSpecies)
            {
                found_existing_vector_population = true;
                population->AddVectors( rGenome, rMateGenome, isFraction, releasedNumber, releasedFraction, releasedInfectious );
                break;
            }
        }

        if ( !found_existing_vector_population )
        {
            throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__, "Adding vectors (e.g. through a MosquitoRelease intervention) with a species name other than an existing VectorPopulation is not yet supported." );
        }

        return;
    }

    void NodeVector::processImmigratingVector( IVectorCohort* immigrant )
    {
        release_assert( (0 <= immigrant->GetSpeciesIndex()) && (immigrant->GetSpeciesIndex() < m_vectorpopulations.size()) );

        m_vectorpopulations[ immigrant->GetSpeciesIndex() ]->AddImmigratingVector( immigrant );
    }

    void NodeVector::processEmigratingVectors( float dt )
    {
        VectorMigrationBasedOnFiles( dt );
    }

    void NodeVector::VectorMigrationBasedOnFiles( float dt )
    {
        IVectorSimulationContext* ivsc = context();

        VectorCohortVector_t migrating_vectors;
        for( auto vp : m_vectorpopulations )
        {
            migrating_vectors.clear();
            vp->Vector_Migration( dt, &migrating_vectors, false );
            for (auto p_vc : migrating_vectors)
            {
                ivsc->PostMigratingVector(this->GetSuid(), p_vc);
            }
        }
    }

    void NodeVector::SetSortingVectors()
    {
        for( auto p_vp : this->m_vectorpopulations )
        {
            p_vp->SetSortingVectors();
        }
    }

    void NodeVector::SortVectors()
    {
        for( auto p_vp : this->m_vectorpopulations )
        {
            p_vp->SortImmigratingVectors();
        }
    }

    IVectorSimulationContext* NodeVector::context() const
    {
        IVectorSimulationContext *ivsc;
        if (s_OK != parent->QueryInterface(GET_IID(IVectorSimulationContext), (void**)&ivsc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IVectorSimulationContext", "IVectorSimulationContext" );
        }

        return ivsc;
    }

    void NodeVector::propagateContextToDependents()
    {
        Node::propagateContextToDependents();

        if( !larval_habitat_multiplier.WasInitialized() )
        {
            // For nodes that are created from serialization file
            larval_habitat_multiplier.Initialize();
        }

        for (auto population : m_vectorpopulations)
        {
            population->SetContextTo( getContextPointer() );
        }
    }

    const SimulationConfig*
    NodeVector::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    VectorProbabilities* NodeVector::GetVectorLifecycleProbabilities()
    {
        return m_vector_lifecycle_probabilities;
    }

    void NodeVector::AddHabitat( const std::string& species, IVectorHabitat* pHabitat )
    {
        VectorHabitatType::Enum type = pHabitat->GetVectorHabitatType();

        VectorHabitatList_t& habitats = m_larval_habitats[ species ];
        VectorHabitatList_t::iterator it = std::find_if( habitats.begin(), habitats.end(), [type](IVectorHabitat* entry){ return entry->GetVectorHabitatType() == type; } );
        if ( it != habitats.end() )
        {
            std::stringstream ss;
            ss << "Duplicate habitat type being added = '" << VectorHabitatType::pairs::lookup_key( type ) << "' being added for species = '" << species << "'";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        habitats.push_back( pHabitat );
    }

    IVectorHabitat* NodeVector::GetVectorHabitatBySpeciesAndType( const std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson )
    {
        VectorHabitatList_t& habitats = m_larval_habitats[ species ];
        VectorHabitatList_t::iterator it = std::find_if( habitats.begin(), habitats.end(), [type](IVectorHabitat* entry){ return entry->GetVectorHabitatType() == type; } );
        if ( it == habitats.end() )
        {
            std::ostringstream msg;
            msg << "The current configuration of 'Vector_Species_Params' does not comply with the serialized state of the simulation. \n";
            msg << "There is a habitat of type '" << VectorHabitatType::pairs::lookup_key( type ) << "' in the simulation but the configured species '" << species << "' does not populate it."; 
            msg << "The species that originally populated the habitat has to be added to 'Vector_Species_Params'. \n";
            msg << "Types of Larval habitats defined for a species in the configuration that was used for the serialized population cannot be removed. \n";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        LOG_DEBUG_F( "%s: Found larval habitat with type = %s\n", __FUNCTION__, VectorHabitatType::pairs::lookup_key( type ) );

        return *it;
    }

    VectorHabitatList_t* NodeVector::GetVectorHabitatsBySpecies( const std::string& species )
    {
        // This will create an empty habitat list if necessary, which is okay.
        return &(m_larval_habitats[ species ]);
    }

    float NodeVector::GetLarvalHabitatMultiplier(VectorHabitatType::Enum type, const std::string& species) const
    {
        return larval_habitat_multiplier.GetMultiplier( type, species ) *
               larval_habitat_multiplier.GetMultiplier( VectorHabitatType::ALL_HABITATS, species );
    }

    void NodeVector::AddVectorPopulationToNode( IVectorPopulation* vp )
    {
        vp->SetVectorMortality( vector_mortality );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE: m_vector_populations, m_VectorPopulationReportingList and VectorParameters.vector_species
        // !!! need to all be in the same order.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        m_vectorpopulations.push_back(vp);

        IVectorPopulationReporting* p_ivpr = nullptr;
        if( vp->QueryInterface( GET_IID( IVectorPopulationReporting ), (void**)&p_ivpr ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "vp", "IVectorPopulationReporting", "IVectorPopulation" );
        }
        m_VectorPopulationReportingList.push_back( p_ivpr );
    }

    suids::suid NodeVector::GetNextVectorSuid()
    {
        return m_VectorCohortSuidGenerator();
    }

    const VectorPopulationReportingList_t& NodeVector::GetVectorPopulationReporting() const
    {
        return m_VectorPopulationReportingList;
    }

    void NodeVector::UpdateTransmissionGroupPopulation(const IPKeyValueContainer& properties, float size_changes, float mc_weight)
    {
        txIndoor->UpdatePopulationSize(human_indoor, size_changes, mc_weight);
        txOutdoor->UpdatePopulationSize(human_outdoor, size_changes, mc_weight);
    }

    ITransmissionGroups* NodeVector::CreateTransmissionGroups()
    {
        txOutdoor = new StrainAwareTransmissionGroupsGP( GetRng() );
        txOutdoor->SetTag( OUTDOOR );
        txIndoor = new StrainAwareTransmissionGroupsGP( GetRng() );
        txIndoor->SetTag( INDOOR );
        return nullptr; // Don't want Node::transmissionGroups used
    }

    void NodeVector::BuildTransmissionRoutes( float /* contagionDecayRate */ )
    {
        txIndoor->Build( 1.0f, InfectionConfig::number_basestrains, InfectionConfig::number_substrains );
        txOutdoor->Build( 1.0f, InfectionConfig::number_basestrains, InfectionConfig::number_substrains );
    }

    void NodeVector::DepositFromIndividual(
        const IStrainIdentity& strainIDs,
        float contagion_quantity,
        TransmissionGroupMembership_t shedder,
        TransmissionRoute::Enum route)
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should be calling GeneticProbability version." );
    }

    float NodeVector::GetTotalContagion( void )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should be calling GeneticProbability version." );
    }

    void NodeVector::ExposeIndividual( IInfectable* candidate, TransmissionGroupMembership_t individual, float dt )
    {
        txIndoor->ExposeToContagion( candidate, human_indoor, dt, TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR );
        txOutdoor->ExposeToContagion( candidate, human_outdoor, dt, TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR );
    }

    void NodeVector::ExposeVector( IInfectable* candidate, float dt )
    {
        txIndoor->ExposeToContagion(  candidate, vector_indoor,  dt, TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR );
        txOutdoor->ExposeToContagion( candidate, vector_outdoor, dt, TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR );
    }

    void NodeVector::DepositFromIndividual( const IStrainIdentity& strainIDs,
                                            const GeneticProbability& contagion_quantity,
                                            TransmissionRoute::Enum route )
    {
        LOG_DEBUG_F( "deposit from individual: antigen index =%d, substain index = %d, quantity = %f, route = %d\n",
                     strainIDs.GetAntigenID(), strainIDs.GetGeneticID(), contagion_quantity.GetSum(), uint32_t(route) );

        switch (route)
        {
            case TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR:
                txIndoor->DepositContagionGP( strainIDs, contagion_quantity, NodeVector::human_indoor );
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR:
                txOutdoor->DepositContagionGP( strainIDs, contagion_quantity, NodeVector::human_outdoor );
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR:
                txIndoor->DepositContagionGP( strainIDs, contagion_quantity, NodeVector::vector_indoor );
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR:
                txOutdoor->DepositContagionGP( strainIDs, contagion_quantity, NodeVector::vector_outdoor );
                break;

            default:
                // TODO - try to get proper string from route enum
                throw new BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "route", uint32_t(route), "???" );
        }
    }

    GeneticProbability NodeVector::GetTotalContagionGP( TransmissionRoute::Enum route ) const
    {
        GeneticProbability contagion;
        switch (route)
        {
            case TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR:
            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR:
                contagion = txIndoor->GetTotalContagionGP();
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR:
            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR:
                contagion = txOutdoor->GetTotalContagionGP();
                break;

            default:
                // TODO - try to get proper string from route enum
                throw new BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "route", uint32_t(route), "???" );
        }

        return contagion;
    }

    REGISTER_SERIALIZABLE(NodeVector);

    void NodeVector::serialize(IArchive& ar, NodeVector* obj)
    {
        Node::serialize(ar, obj);
        NodeVector& node = *obj;

        if( node.serializationFlags.test( SerializationFlags::LarvalHabitats ) ) {
            ar.labelElement("m_larval_habitats") & node.m_larval_habitats;
        }
        else{
            // Let the user know we are getting larval habitats from config
            if( ar.IsReader() ) {
                LOG_INFO( "Loading larval habitats from config instead of serialized data. 'LarvalHabitatMultiplier' in demographics will not be applied.\n" );
            }
            else{
                LOG_INFO( "Not serializing larval habitats, they will have to be loaded from config instead of serialized data.\n" );
            }
        }

        if( node.serializationFlags.test( SerializationFlags::VectorPopulation ) ) {
            ar.labelElement("m_vectorpopulations") & node.m_vectorpopulations;
            ar.labelElement("m_vector_lifecycle_probabilities"); VectorProbabilities::serialize( ar, node.m_vector_lifecycle_probabilities );
            ar.labelElement("m_VectorCohortSuidGenerator") & node.m_VectorCohortSuidGenerator;

            if( ar.IsReader() )
            {
                for (const auto vector_population : node.m_vectorpopulations)
                {
                    IVectorPopulationReporting* p_ivpr = nullptr;
                    if( vector_population->QueryInterface( GET_IID( IVectorPopulationReporting ), (void**)&p_ivpr ) != s_OK )
                    {
                        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "vector_population", "IVectorPopulationReporting", "IVectorPopulation" );
                    }
                    node.m_VectorPopulationReportingList.push_back( p_ivpr );
                }
            }
        }

        if( node.serializationFlags.test( SerializationFlags::Parameters ) ) {
// clorton            ar.labelElement("larval_habitat_multiplier") & node.larval_habitat_multiplier;
            ar.labelElement("vector_mortality") & node.vector_mortality;
            ar.labelElement("mosquito_weight") & node.mosquito_weight;
        }

        if( node.serializationFlags.test( SerializationFlags::Properties ) ) {
        }
    }
} // end namespace Kernel
