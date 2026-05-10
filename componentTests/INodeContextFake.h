
#pragma once

#include "ISimulationContext.h"
#include "NodeEventContext.h"
#include "VectorContexts.h"
#include "VectorProbabilities.h"
#include "VectorHabitat.h"
#include "RANDOM.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "INodeSTI.h"
#include "TransmissionGroupMembership.h"
#include "IVectorPopulation.h"

using namespace Kernel;

struct INodeContextFake : INodeContext, INodeVector, IVectorNodeContext, INodeSTI, IActionStager
{
private:

    suids::suid m_suid ;
    INodeEventContext* m_pNEC;
    NPKeyValueContainer m_NodeProperties;
    VectorProbabilities* m_pVectorProbabilities;
    VectorHabitatList_t m_HabitatList;
    PSEUDO_DES m_RNG;
    IdmDateTime m_Time;
    uint32_t m_NextVectorID;
    std::vector<std::pair<IIndividualHumanEventContext*, EventTrigger>> m_StagedEvents;
    std::vector<IVectorPopulation*>  m_vectorpopulations;
    VectorPopulationReportingList_t m_VectorPopulationReportingList;


public:
    INodeContextFake( int id = 1, INodeEventContext* pNEC = nullptr )
    : m_suid()
    , m_pNEC( pNEC )
    , m_NodeProperties()
    , m_pVectorProbabilities( VectorProbabilities::CreateVectorProbabilities() )
    , m_HabitatList()
    , m_RNG( 42 )
    , m_Time()
    , m_NextVectorID(0)
    , m_StagedEvents()
    , m_vectorpopulations()
    , m_VectorPopulationReportingList()
    {
        m_suid.data = id ;
        json::Object fake_json;
        fake_json[ "Habitat_Type" ] = json::String( "CONSTANT" );
        fake_json[ "Max_Larval_Capacity" ] = json::Number( 10000000000 );
        Configuration * fake_config = Configuration::CopyFromElement( fake_json );
        m_HabitatList.push_back( VectorHabitat::CreateHabitat( VectorHabitatType::CONSTANT, fake_config ) );
        delete fake_config;

        if( m_pNEC != nullptr )
        {
            m_pNEC->SetContextTo( this );
        }
    }

    INodeContextFake( const suids::suid& rSuid, INodeEventContext* pNEC = nullptr )
    : m_suid(rSuid)
    , m_pNEC(pNEC)
    , m_NodeProperties()
    , m_pVectorProbabilities( nullptr )
    , m_HabitatList()
    , m_RNG( 42 )
    , m_Time()
    , m_NextVectorID(0)
    , m_StagedEvents()
    {
        if( m_pNEC != nullptr )
        {
            m_pNEC->SetContextTo( this );
        }
    }

    ~INodeContextFake()
    {
        for( auto habitat : m_HabitatList )
        {
            delete habitat;
        }
        delete m_pVectorProbabilities;
    }

    virtual bool operator==( const INodeContext& rThat ) const override
    {
        const INodeContextFake* pThat = dynamic_cast<const INodeContextFake*>(&rThat) ;
        if( pThat == nullptr ) return false ;

        if( this->m_suid.data != pThat->m_suid.data ) return false ;

        return true ;
    }

    virtual ISimulationContext* GetParent() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual suids::suid GetSuid() const override
    {
        return m_suid ;
    }

    virtual suids::suid GetNextInfectionSuid() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual RANDOMBASE* GetRng() override
    {
        return &m_RNG;
    }

    virtual void SetRng( RANDOMBASE* prng ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual void ExposeIndividual( IInfectable* candidate, TransmissionGroupMembership_t individual, float dt ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs, float contagion_quantity, TransmissionGroupMembership_t individual, TransmissionRoute::Enum route ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void GetGroupMembershipForIndividual( const RouteList_t& route, const IPKeyValueContainer& properties, TransmissionGroupMembership_t& membershipOut ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void UpdateTransmissionGroupPopulation( const IPKeyValueContainer& properties, float size_changes, float mc_weight ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual std::map< std::string, float > GetContagionByRoute() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetTotalContagion( void ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual RouteList_t& GetTransmissionRoutes() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetContagionByRouteAndProperty( const std::string& route, const IPKeyValue& property_value )
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual float getSinusoidalCorrection(float sinusoidal_amplitude, float sinusoidal_phase) const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float getBoxcarCorrection(float boxcar_amplitude, float boxcar_start_time, float boxcar_end_time) const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual IMigrationInfo* GetMigrationInfo() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual const NodeDemographics* GetDemographics() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual NPKeyValueContainer& GetNodeProperties() override
    {
        return m_NodeProperties;
    }

    virtual const IdmDateTime& GetTime() const override
    {
        if( m_pNEC != nullptr )
        {
            return m_pNEC->GetTime();
        }
        else
        {
            static IdmDateTime time(0);
            return time;
        }
    }

    virtual float GetInfected() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetSymptomatic() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual float GetNewlySymptomatic() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual float GetStatPop() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetBirths() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetCampaignCost() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetInfectivity() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual const Climate* GetLocalWeather() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void SetupEventContextHost() override
    {
        // current code does not expect the test code to call this method
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual INodeEventContext* GetEventContext() override
    {
        release_assert( m_pNEC );
        return m_pNEC;
    }

    virtual void AddEventsFromOtherNodes( const std::vector<EventTrigger>& rTriggerList ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual QueryResult QueryInterface( iid_t iid, void** ppvObject ) override
    {
        *ppvObject = nullptr;
        if( iid == GET_IID( INodeContext ) )
            *ppvObject = static_cast<INodeContext*>(this);
        else if( iid == GET_IID( INodeVector ) )
            *ppvObject = static_cast<INodeVector*>(this);
        else if( iid == GET_IID( IVectorNodeContext ) )
            *ppvObject = static_cast<IVectorNodeContext*>(this);
        else if( iid == GET_IID( INodeSTI ) )
            *ppvObject = static_cast<INodeSTI*>(this);
        else if( iid == GET_IID( IActionStager ) )
            *ppvObject = static_cast<IActionStager*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK;
        }
        else
            return QueryResult::e_NOINTERFACE;
    }

    virtual int32_t AddRef() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual int32_t Release() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual long int GetPossibleMothers() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual bool IsEveryoneHome() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void SetWaitingForFamilyTrip( suids::suid migrationDestination, MigrationType::Enum migrationType, float timeUntilTrip, float timeAtDestination, bool isDestinationNewHome ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetMeanAgeInfection() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }
    virtual const NodeDemographicsDistribution* GetImmunityDistribution()        const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution* GetFertilityDistribution()       const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution* GetMortalityDistribution()       const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution* GetMortalityDistributionMale()   const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution* GetMortalityDistributionFemale() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution* GetAgeDistribution()             const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual std::vector<bool> GetMigrationTypeEnabledFromDemographics() const override
    {
        std::vector<bool> enabled_list;

        enabled_list.push_back( true );
        enabled_list.push_back( true );
        enabled_list.push_back( true );
        enabled_list.push_back( true );
        enabled_list.push_back( true );

        return enabled_list;
    }

    virtual ExternalNodeId_t GetExternalID() const override
    {
        return m_suid.data ;
    }

    virtual float GetLatitudeDegrees() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float GetLongitudeDegrees() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void SetupMigration( IMigrationInfoFactory * migration_factory, 
                                 const std::string& idreference,
                                 MigrationStructure::Enum ms,
                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override
    { 
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented.");
    }

    virtual void SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented.");
    }

    virtual void SetContextTo(ISimulationContext*)                                                             override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented."); }
    virtual void PopulateFromDemographics(NodeDemographicsFactory *demographics_factory)                       override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented."); }
    virtual void InitializeTransmissionGroupPopulations()                                                      override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented."); }
    virtual void Update(float)                                                                                 override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented."); }
    virtual IIndividualHuman* processImmigratingIndividual(IIndividualHuman*)                                  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented."); }
    virtual void SortHumans()                                                                                  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method is not implemented."); }

    virtual float GetBasePopulationScaleFactor() const
    {
        return 1.0;
    }

    virtual ProbabilityNumber GetProbMaternalTransmission() const
    {
        return 1.0;
    }
    virtual float GetMaxInfectionProb( TransmissionRoute::Enum route ) const
    {
        return 0.0f;
    }


    virtual float GetNonDiseaseMortalityRateByAgeAndSex( float age, Gender::Enum sex ) const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual float initiatePregnancyForIndividual( int individual_id, float dt ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual bool updatePregnancyForIndividual( int individual_id, float duration ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void populateNewIndividualsByBirth(int count_new_individuals = 100) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    // ---------------
    // --- INodeVector
    // ---------------
    virtual suids::suid GetNextVectorSuid() override
    {
        suids::suid id;
        id.data = ++m_NextVectorID;
        return id;
    }

    virtual const VectorPopulationReportingList_t& GetVectorPopulationReporting() const override
    {
        return m_VectorPopulationReportingList;
    }

    virtual void AddVectors( const std::string& releasedSpecies,
                             const VectorGenome& rGenome,
                             const VectorGenome& rMateGenome,
                             bool isFraction,
                             uint32_t releasedNumber,
                             float releasedFraction,
                             float releasedInfectious ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual void processImmigratingVector( IVectorCohort* immigrant ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    // --- INodeSTI -----
    virtual IRelationshipManager* GetRelationshipManager() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual ISociety* GetSociety() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void DepositFromIndividual( const IStrainIdentity& rStrain, const CoitalAct& rCoitalAct ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs,
                                        const GeneticProbability& contagion_quantity,
                                        TransmissionRoute::Enum route ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual void ExposeVector( IInfectable* candidate, float dt ) override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual GeneticProbability GetTotalContagionGP( TransmissionRoute::Enum route ) const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    // -----------------------
    // --- IVectorNodeContext
    // -----------------------
    virtual VectorProbabilities* GetVectorLifecycleProbabilities() override
    {
        return m_pVectorProbabilities;
    }

    virtual void AddHabitat( const std::string& species, IVectorHabitat* pHabitat ) override
    {
        // do nothing
    }

    virtual IVectorHabitat* GetVectorHabitatBySpeciesAndType( const std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson ) override
    {
        return VectorHabitat::CreateHabitat( type, inputJson );
    }

    virtual VectorHabitatList_t* GetVectorHabitatsBySpecies( const std::string& species ) override
    {
        return &m_HabitatList;
    }

    virtual float GetLarvalHabitatMultiplier( VectorHabitatType::Enum type, const std::string& species ) const override
    {
        return 1.0;
    }

    virtual IActionStager* GetActionStager() override
    {
        return this;
    }

    // IActionStager
    virtual void StageIntervention( IIndividualHumanEventContext* pHuman, IDistributableIntervention* pIntervention ) override
    {
    }

    virtual void StageEvent( IIndividualHumanEventContext* pHuman, const EventTrigger& rTrigger ) override
    {
        m_StagedEvents.push_back( std::make_pair( pHuman, rTrigger ) );
    }

    void PostUpdate()
    {
        // Broadcast the events that were staged until all of the individuals have been updated.
        IIndividualEventBroadcaster* broadcaster = GetEventContext()->GetIndividualEventBroadcaster();
        for( auto event_pair : m_StagedEvents )
        {
            broadcaster->TriggerObservers( event_pair.first, event_pair.second );
        }
        m_StagedEvents.clear();
    }

    void AddVectorPopulationToNode(IVectorPopulation* vp) override
    {

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE: m_vector_populations, m_VectorPopulationReportingList and VectorParameters.vector_species
        // !!! need to all be in the same order.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        m_vectorpopulations.push_back(vp);

        IVectorPopulationReporting* p_ivpr = nullptr;
        if (vp->QueryInterface(GET_IID(IVectorPopulationReporting), (void**)&p_ivpr) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "vp", "IVectorPopulationReporting", "IVectorPopulation");
        }
        m_VectorPopulationReportingList.push_back(p_ivpr);
    }

};
