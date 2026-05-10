
#pragma once
#include "IdmApi.h"
#include "ISerializable.h"
#include "suids.hpp"
#include "TransmissionGroupMembership.h"
#include "IntranodeTransmissionTypes.h"
#include "Properties.h"
#include "SimulationEnums.h"
#include "EventTrigger.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    struct IdmDateTime;
    class RANDOMBASE;
    struct IInfectable;
    struct IMigrationInfo;
    struct IMigrationInfoFactory;
    struct NodeDemographics;
    class NodeDemographicsFactory;
    struct NodeDemographicsDistribution;
    class  Climate;
    class  ClimateFactory;
    struct INodeEventContext;
    struct IIndividualHuman;
    struct ISimulationContext;
    class NPKeyValueContainer;
    struct IStrainIdentity;

    typedef std::vector<std::string> RouteList_t;

    struct IDMAPI INodeContext : ISerializable
    {
        // TODO/OPTION:
        // could have an PostMigratingIndividual interface too if individuals will call back to do migration....

        // DMB 9-9-2014 May need to consider implementing the equality operator for ISupports
        virtual bool operator==( const INodeContext& rThat ) const
        {
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "INodeContect::operator==() needs to be implemented by subclass.");
        } ;

        virtual bool operator!=( const INodeContext& rThat ) const
        { 
            return !(*this == rThat);
        } ;

        virtual ISimulationContext* GetParent() = 0;

        //individual can get an id of their parent to compare against, for instance, their home node id
        virtual suids::suid GetSuid() const = 0;

        virtual void SetupMigration( IMigrationInfoFactory * migration_factory, 
                                     const std::string& idreference,
                                     MigrationStructure::Enum ms,
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) = 0;

        virtual void SetupEventContextHost() = 0;
        virtual void SetContextTo( ISimulationContext* ) = 0;
        virtual void SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory ) = 0;
        virtual void PopulateFromDemographics (NodeDemographicsFactory *demographics_factory ) = 0;
        virtual void InitializeTransmissionGroupPopulations() = 0;

        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual RANDOMBASE* GetRng() = 0; 
        virtual void SetRng( RANDOMBASE* prng ) = 0;

        virtual void Update(float dt) = 0;
        virtual IIndividualHuman* processImmigratingIndividual( IIndividualHuman* ) = 0;
        virtual void SortHumans() = 0;

        // heterogeneous intra-node transmission
        virtual void ExposeIndividual(IInfectable* candidate, TransmissionGroupMembership_t individual, float dt) = 0;
        virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs, float contagion_quantity, TransmissionGroupMembership_t individual, TransmissionRoute::Enum route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT) = 0;
        virtual void GetGroupMembershipForIndividual(const RouteList_t& route, const IPKeyValueContainer& properties, TransmissionGroupMembership_t& membershipOut ) = 0;
        virtual void UpdateTransmissionGroupPopulation(const IPKeyValueContainer& properties, float size_changes,float mc_weight) = 0;
        virtual std::map< std::string, float > GetContagionByRoute() const = 0; // developed for Typhoid/Environmental
        virtual float GetTotalContagion( void ) = 0;
        virtual const RouteList_t& GetTransmissionRoutes( ) const = 0;
        virtual float GetContagionByRouteAndProperty( const std::string& route, const IPKeyValue& property_value ) = 0;

        virtual float getSinusoidalCorrection(float sinusoidal_amplitude, float sinusoidal_phase) const = 0;
        virtual float getBoxcarCorrection(float boxcar_amplitude, float boxcar_start_time, float boxcar_end_time) const = 0;

        virtual IMigrationInfo* GetMigrationInfo() = 0;
        virtual const NodeDemographics* GetDemographics() const = 0;
        virtual std::vector<bool> GetMigrationTypeEnabledFromDemographics() const = 0 ;
        virtual NPKeyValueContainer& GetNodeProperties() = 0;

        // reporting interfaces
        virtual const IdmDateTime& GetTime()   const = 0;
        virtual float       GetInfected()      const = 0;
        virtual float       GetSymptomatic()   const = 0;
        virtual float       GetNewlySymptomatic()     const = 0;
        virtual float       GetStatPop()       const = 0;
        virtual float       GetBirths()        const = 0;
        virtual float       GetCampaignCost()  const = 0;
        virtual float       GetInfectivity()   const = 0;
        virtual const Climate* GetLocalWeather() const = 0;
        virtual long int GetPossibleMothers()  const = 0;
        virtual float GetMeanAgeInfection()    const = 0;
        virtual float GetNonDiseaseMortalityRateByAgeAndSex( float age, Gender::Enum sex ) const = 0;

        // These methods are not const because they will extract the value from the demographics
        // if it has not been done yet.
        virtual float GetLatitudeDegrees() = 0;
        virtual float GetLongitudeDegrees() = 0;

        // This method will ONLY be used for reporting by input node ID, don't use it elsewhere!
        virtual ExternalNodeId_t GetExternalID() const = 0;

        // for interventions
        virtual INodeEventContext* GetEventContext() = 0;
        virtual void AddEventsFromOtherNodes( const std::vector<EventTrigger>& rTriggerList ) = 0;

        virtual bool IsEveryoneHome() const = 0;
        virtual void SetWaitingForFamilyTrip( suids::suid migrationDestination, 
                                              MigrationType::Enum migrationType, 
                                              float timeUntilTrip, 
                                              float timeAtDestination,
                                              bool isDestinationNewHome ) = 0;

        virtual float GetBasePopulationScaleFactor() const = 0;
        virtual ProbabilityNumber GetProbMaternalTransmission() const = 0;

        virtual float GetMaxInfectionProb( TransmissionRoute::Enum tx_route )        const = 0;

        virtual float initiatePregnancyForIndividual( int individual_id, float dt ) = 0;
        virtual bool updatePregnancyForIndividual( int individual_id, float duration ) = 0;
        virtual void populateNewIndividualsByBirth(int count_new_individuals = 100) = 0;

        virtual const NodeDemographicsDistribution* GetImmunityDistribution()        const = 0;
        virtual const NodeDemographicsDistribution* GetFertilityDistribution()       const = 0;
        virtual const NodeDemographicsDistribution* GetMortalityDistribution()       const = 0;
        virtual const NodeDemographicsDistribution* GetMortalityDistributionMale()   const = 0;
        virtual const NodeDemographicsDistribution* GetMortalityDistributionFemale() const = 0;
        virtual const NodeDemographicsDistribution* GetAgeDistribution()             const = 0;

    };
}

