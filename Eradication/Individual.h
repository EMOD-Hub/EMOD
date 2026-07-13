
#pragma once

#include <list>
#include <map>
#include <string>

#include "Common.h"
#include "Configure.h"
#include "IIndividualHumanContext.h"
#include "IInfectable.h"
#include "IMigrate.h"
#include "IndividualEventContext.h"
#include "InterventionsContainer.h" // for serialization code to compile
#include "TransmissionGroupMembership.h"
#include "SimulationEnums.h"
#include "suids.hpp"
#include "IContagionPopulation.h"
#include "IIndividualHuman.h"
#include "Types.h" // for ProbabilityNumber

class Configuration;

namespace Kernel
{
    class RANDOMBASE;
    struct INodeContext;
    struct IMigrationInfoFactory;
    struct IIndividualHumanInterventionsContext;
    class  Infection;
    class  InterventionsContainer;
    struct ISusceptibilityContext;
    class  NodeNetwork;
    class  StrainIdentity;
    class  Susceptibility;

    class IndividualHumanConfig : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(IndividualHumanConfig)
        friend class Simulation;
        friend class IndividualHuman;
        friend class Node;
        friend class IndividualHumanMalariaConfig;

    public:
        static bool IsAdultAge( float years );
        static bool CanSupportFamilyTrips( IMigrationInfoFactory* pmi );
        virtual bool Configure( const Configuration* config ) override; // public for pymod

        static int max_ind_inf;
        static bool superinfection;
        static MigrationPattern::Enum migration_pattern;
        static float infection_timestep;

    protected:
        static bool aging;
        static float min_adult_age_years ;

        // migration parameters
        static int roundtrip_waypoints;
        static float local_roundtrip_prob;
        static float air_roundtrip_prob;
        static float region_roundtrip_prob;
        static float sea_roundtrip_prob;
        static float family_roundtrip_prob;

        // duration rates
        static float local_roundtrip_duration_rate;
        static float air_roundtrip_duration_rate;
        static float region_roundtrip_duration_rate;
        static float sea_roundtrip_duration_rate;
        static float family_roundtrip_duration_rate;

        static int infection_updates_per_tstep;
        static bool enable_immunity;
        
        // From SimConfig
        static MigrationStructure::Enum                             migration_structure;                              // MIGRATION_STRUCTURE

        static bool  enable_skipping;

        void PrintConfigs() const;

        void RegisterRandomWalkDiffusionParameters();
        void RegisterSingleRoundTripsParameters();
        void RegisterWaypointsHomeParameters();

        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    };

    class IndividualHuman : public IIndividualHuman,
                            public IIndividualHumanContext,
                            public IIndividualHumanEventContext,
                            public IInfectable,
                            public IInfectionAcquirable,
                            public IMigrate
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static IndividualHuman *CreateHuman();
        static IndividualHuman *CreateHuman(INodeContext *context, suids::suid id, float MCweight = 1.0f, float init_age = 0.0f, int gender = 0);
        virtual ~IndividualHuman();
        virtual void InitializeHuman() override;

        virtual void Update(float currenttime, float dt) override;

        virtual IMigrate* GetIMigrate() override;

        // IIndividualHumanContext
        virtual suids::suid GetSuid() const override;
        virtual suids::suid GetNextInfectionSuid() override;
        virtual RANDOMBASE* GetRng() override;

        virtual IIndividualHumanInterventionsContext* GetInterventionsContext()                                  const   override;
        virtual IIndividualHumanInterventionsContext* GetInterventionsContextbyInfection(IInfection* infection)          override;
        virtual const IIndividualHuman*               GetIndividualHumanConst()                                  const   override { return this; };
        virtual IIndividualHumanEventContext*         GetEventContext()                                                  override;
        virtual ISusceptibilityContext*               GetSusceptibilityContext()                                 const   override;

        virtual const Kernel::NodeDemographics*     GetDemographics() const override;

        // IIndividualHumanEventContext methods
        virtual bool              IsPregnant()                     const override { return is_pregnant; };
        virtual float             GetAge()                         const override { return m_age; }
        virtual float             GetImmuneFailage()               const override;
        inline  float             getAgeInYears()                  const          { return floor(GetAge()/DAYSPERYEAR);}
        virtual int               GetGender()                      const override { return m_gender; }
        virtual float             GetMonteCarloWeight()            const override { return m_mc_weight; }
        virtual bool              IsPossibleMother()               const override;
        virtual bool              IsInfected()                     const override { return m_is_infected; }
        virtual float             GetAcquisitionImmunity()         const override; // KM: For downsampling based on immune status.  For now, just takes perfect immunity; can be updated to include a threshold.  Unclear how to work with multiple strains or waning immunity.
        virtual HumanStateChange  GetStateChange()                 const override { return StateChange; }
        virtual void              Die( HumanStateChange )                override;

        virtual INodeEventContext*        GetNodeEventContext()          override; // for campaign cost reporting in e.g. HealthSeekingBehavior
        virtual IPKeyValueContainerFull*  GetProperties()                override;
        virtual bool AtHome() const override;
        virtual void SetHome( const suids::suid& rHomeNodeID ) override;

        virtual bool IsAdult() const override;
        virtual bool IsDead() const override;

        // IMigrate
        virtual void ImmigrateTo(INodeContext* destination_node) override;
        virtual void SetMigrating( suids::suid destination, 
                                   MigrationType::Enum type, 
                                   float timeUntilTrip, 
                                   float timeAtDestination,
                                   bool isDestinationNewHome ) override;
        virtual const suids::suid& GetMigrationDestination() override;
        virtual MigrationType::Enum GetMigrationType() const override { return migration_type; }
        virtual bool IsOnFamilyTrip() const override { return is_on_family_trip; } ;
        virtual const suids::suid& GetHomeNodeId() const override { return home_node_id ; } ;

        // Migration
        virtual bool IsMigrating() override;
        virtual void CheckForMigration(float currenttime, float dt);
        void SetNextMigration();

        // Heterogeneous intra-node transmission
        virtual void UpdateGroupMembership() override;
        virtual void UpdateGroupPopulation(float size_changes) override;

        // Initialization
        virtual void SetInitialInfections(int init_infs) override;
        virtual void SetParameters( INodeContext* pParent, float infsample, float imm_mod, float risk_mod, float mig_mod) override; // specify each parameter, default version of SetParams()
        virtual void CreateSusceptibility(float susceptibility_mod=1.0, float risk_mod=1.0);
        virtual void setupMaternalAntibodies(IIndividualHumanContext* mother, INodeContext* node) override;
        virtual void SetMigrationModifier( float modifier ) override { migration_mod = modifier; }

        // Infections
        virtual void ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership);
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT ) override;
        virtual void AcquireNewInfection( const IStrainIdentity* infstrain, int incubation_period_override = -1) override;

        virtual const infection_list_t &GetInfections() const override;
        virtual IInfection* GetNewInfection() const override;
        virtual bool IsSymptomatic() const override;
        virtual bool IsNewlySymptomatic() const override;
        virtual void UpdateInfectiousness(float dt) override;
        virtual bool InfectionExistsForThisStrain(IStrainIdentity* check_strain_id);
        virtual void ClearNewInfectionState() override;
        virtual NewInfectionState::_enum GetNewInfectionState() const override { return m_new_infection_state; }
        virtual inline float GetInfectiousness() const override { return infectiousness; }

        virtual float GetImmunityReducedAcquire() const override;
        virtual float GetInterventionReducedAcquire() const override;

        // Births and deaths
        virtual bool UpdatePregnancy(float dt=1) override; // returns true if birth happens this time step and resets is_pregnant to false
        virtual void InitiatePregnancy(float duration = (DAYSPERWEEK * WEEKS_FOR_GESTATION)) override;
        virtual float GetBirthRateMod() const override;
        virtual void CheckVitalDynamics(float currenttime, float dt=1.0); // non-disease mortality
        // update and set dynamic MC weight
        virtual void UpdateMCSamplingRate(float current_sampling_rate) override;

        //Broadcast event
        void BroadcastEvent( const EventTrigger& event_trigger ) override;

        // Assorted getters and setters
        virtual void SetContextTo(INodeContext* context) override;
        virtual INodeContext* GetParent() const override;
        virtual inline Kernel::suids::suid GetParentSuid() const override;
        virtual ProbabilityNumber getProbMaternalTransmission() const override;

        virtual void SetGoingOnFamilyTrip( suids::suid migrationDestination, 
                                           MigrationType::Enum migrationType, 
                                           float timeUntilTrip, 
                                           float timeAtDestination,
                                           bool isDestinationNewHome ) override;
        virtual void SetWaitingToGoOnFamilyTrip() override;
        virtual void GoHome() override;

        static void InitializeStatics( const Configuration* config );

    protected:
        // Core properties
        suids::suid suid;
        float m_age;
        int   m_gender;
        float m_mc_weight;
        float m_daily_mortality_rate;
        bool  is_pregnant;      // pregnancy variables for vital_birth_dependence==INDIVIDUAL_PREGNANCIES
        float pregnancy_timer;

        // Immune system, infection(s), intervention(s), and transmission properties
        Susceptibility*               susceptibility;   // individual susceptibility (i.e. immune system)
        infection_list_t              infections;
        InterventionsContainer*       interventions;

        TransmissionGroupMembership_t transmissionGroupMembership;

        // Infections
        bool  m_is_infected;    // TODO: replace with more sophisticated strain-tracking capability
        float infectiousness;   // infectiousness calculated over all Infections and passed back to Node
        float Inf_Sample_Rate;  // EAW: unused currently
        int   cumulativeInfs;   // counter of total infections over individual's history
        IInfection* m_pNewInfection; //only non-null during the timestep of the infection occurred

        NewInfectionState::_enum  m_new_infection_state; // to flag various types of infection state changes
        HumanStateChange          StateChange;           // to flag that the individual has migrated or died

        // Migration
        float               migration_mod;
        MigrationType::Enum migration_type;
        suids::suid         migration_destination;
        float               migration_time_until_trip;
        float               migration_time_at_destination;
        bool                migration_is_destination_new_home;
        bool                migration_will_return;
        bool                migration_outbound;
        int                              max_waypoints;    // maximum waypoints a trip can have before returning home
        std::vector<suids::suid>         waypoints;
        std::vector<MigrationType::Enum> waypoints_trip_type;

        bool                waiting_for_family_trip ;
        bool                leave_on_family_trip ;
        bool                is_on_family_trip ;
        suids::suid         family_migration_destination ;
        MigrationType::Enum family_migration_type ;
        float               family_migration_time_until_trip ;
        float               family_migration_time_at_destination ;
        bool                family_migration_is_destination_new_home;

        suids::suid home_node_id ;

        IPKeyValueContainerFull Properties;

        INodeContext* parent;   // Access back to node/simulation methods

        IndividualHuman(INodeContext *context);
        IndividualHuman(suids::suid id = suids::nil_suid(), float MCweight = 1.0f, float init_age = 0.0f, int gender = 0);

        virtual IInfection* createInfection(suids::suid _suid); // factory method (overridden in derived classes)
        virtual void setupInterventionsContainer();            // derived classes can customize the container, and hence the interventions supported, by overriding this method
        virtual void UpdateAge( float dt );

        float GetRoundTripDurationRate( MigrationType::Enum trip_type );

        // Infection updating
        virtual bool SetNewInfectionState(InfectionStateChange::_enum inf_state_change);
        virtual void ReportInfectionState();

        virtual void PropagateContextToDependents();
        virtual bool ImmunityEnabled() const;
        IIndividualEventBroadcaster* broadcaster;

    private:
        bool m_newly_symptomatic;

        virtual IIndividualHumanContext* GetContextPointer();

        DECLARE_SERIALIZABLE(IndividualHuman);
    };
}
