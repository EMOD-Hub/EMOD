
#pragma once

#include "ISerializable.h"
#include "suids.hpp"
#include "Common.h"
#include "IInfection.h"
#include "SimulationEnums.h"
#include "EventTrigger.h"

namespace Kernel
{
    struct INodeContext;
    struct IIndividualHumanContext;
    struct IIndividualHumanEventContext;
    struct IIndividualHumanInterventionsContext;
    struct IMigrate;
    class IPKeyValueContainerFull;

    // Interface for controlling objects (e.g. Node)
    struct IIndividualHuman : ISerializable
    {
        // Setup
        virtual void setupMaternalAntibodies(IIndividualHumanContext* mother, INodeContext* node) = 0;
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override = -1) = 0;
        virtual void SetInitialInfections(int init_infs) = 0;
        virtual void SetParameters( INodeContext* pParent, float infsample, float imm_mod, float risk_mod, float mig_mod) = 0;
        virtual void InitializeHuman() = 0;
        virtual void SetMigrationModifier( float modifier ) = 0;
        virtual void SetHome( const suids::suid& rHomeNodeID ) = 0;

        // Control
        virtual void Update(float current_time, float dt) = 0;
        virtual void UpdateInfectiousness(float dt) = 0;
        virtual void UpdateGroupMembership() = 0;
        virtual void UpdateGroupPopulation(float size_changes) = 0;

        // Inspection
        virtual suids::suid GetSuid() const = 0;
        virtual float GetAge() const = 0;
        virtual float GetImmuneFailage() const = 0;
        virtual bool IsAdult() const = 0;
        virtual int GetGender() const = 0;
        virtual float GetMonteCarloWeight() const = 0;
        virtual bool IsInfected() const = 0;
        virtual bool AtHome() const = 0;
        virtual bool IsOnFamilyTrip() const = 0 ;
        virtual const suids::suid& GetHomeNodeId() const = 0 ;
        virtual bool IsDead() const = 0;
        virtual bool IsSymptomatic() const = 0;
        virtual bool IsNewlySymptomatic() const = 0;

        virtual NewInfectionState::_enum GetNewInfectionState() const = 0;
        virtual HumanStateChange GetStateChange() const = 0;

        virtual IMigrate*                               GetIMigrate()                       = 0;
        virtual IIndividualHumanInterventionsContext*   GetInterventionsContext()     const = 0;
        virtual IIndividualHumanEventContext*           GetEventContext()                   = 0;
        virtual ISusceptibilityContext*                 GetSusceptibilityContext()    const = 0;
        virtual INodeContext*                           GetParent()                   const = 0;

        virtual IPKeyValueContainerFull* GetProperties() = 0;

        virtual float GetAcquisitionImmunity() const = 0;               // KM: For downsampling based on immune status.  For now, just takes perfect immunity; can be updated to include a threshold.  Unclear how to work with multiple strains or waning immunity.
        virtual bool IsPossibleMother() const = 0;
        virtual void UpdateMCSamplingRate(float current_sampling_rate) = 0;
        virtual bool IsPregnant() const = 0;
        virtual bool UpdatePregnancy(float dt = 1) = 0; // returns true if birth happens this time step and resets is_pregnant to false
        virtual void InitiatePregnancy(float duration = (DAYSPERWEEK * WEEKS_FOR_GESTATION)) = 0;
        virtual float GetBirthRateMod() const = 0;
        virtual float GetInfectiousness() const = 0;
        virtual ProbabilityNumber getProbMaternalTransmission() const = 0;
        virtual inline Kernel::suids::suid GetParentSuid() const = 0;
        virtual bool IsMigrating() = 0;
        virtual void ClearNewInfectionState() = 0;
        virtual const infection_list_t& GetInfections() const = 0;
        virtual IInfection* GetNewInfection() const = 0;
        virtual float GetImmunityReducedAcquire() const = 0;
        virtual float GetInterventionReducedAcquire() const = 0;
        virtual const suids::suid& GetMigrationDestination() = 0;

        // Migration
        virtual void SetContextTo( INodeContext* ) = 0;
        virtual void SetGoingOnFamilyTrip( suids::suid migrationDestination, 
                                           MigrationType::Enum migrationType, 
                                           float timeUntilTrip, 
                                           float timeAtDestination,
                                           bool isDestinationNewHome ) = 0;
        virtual void SetWaitingToGoOnFamilyTrip() = 0;
        virtual void GoHome() = 0;

        // Broadcast
        virtual void BroadcastEvent( const EventTrigger& event_trigger ) = 0;

        virtual ~IIndividualHuman() {}
    };
}
