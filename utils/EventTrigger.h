
#pragma once

#include "BaseEventTrigger.h"

namespace Kernel
{
    class EventTriggerInternal;
    class EventTriggerFactory;
    class EventTrigger;

    // An EventTrigger is the name of an event that occurs to an individual.
    // EventTriggers can be built-in or defined by the user.  They are broadcasted by
    // a IIndividualEventBroadcaster (which is implemented by NodeEventContextHost)
    // to objects that have registered to be notified of these events.
    // This implementation of EventTrigger attempts to reduce string processing and
    // improve performance.
    class EventTrigger : public BaseEventTrigger<EventTrigger,EventTriggerFactory>
    {
    public:
        static const EventTrigger& Births;
        static const EventTrigger& EveryUpdate;
        static const EventTrigger& NewInfectionEvent;
        static const EventTrigger& NewClinicalCase;
        static const EventTrigger& NewSevereCase;
        static const EventTrigger& DiseaseDeaths;
        static const EventTrigger& NonDiseaseDeaths;
        static const EventTrigger& PropertyChange;
        static const EventTrigger& STIDebut;
        static const EventTrigger& StartedART;
        static const EventTrigger& StoppedART;
        static const EventTrigger& InterventionDisqualified;
        static const EventTrigger& HIVNewlyDiagnosed;
        static const EventTrigger& GaveBirth;
        static const EventTrigger& Pregnant;
        static const EventTrigger& Emigrating;
        static const EventTrigger& Immigrating;
        static const EventTrigger& HIVTestedNegative;
        static const EventTrigger& HIVTestedPositive;
        static const EventTrigger& NewlySymptomatic;
        static const EventTrigger& SymptomaticCleared;
        static const EventTrigger& TwelveWeeksPregnant;
        static const EventTrigger& FourteenWeeksPregnant;
        static const EventTrigger& SixWeeksOld;
        static const EventTrigger& EighteenMonthsOld;
        static const EventTrigger& STIPreEmigrating;
        static const EventTrigger& STIPostImmigrating;
        static const EventTrigger& STINewInfection;
        static const EventTrigger& STIExposed;
        static const EventTrigger& NodePropertyChange;
        static const EventTrigger& HappyBirthday;
        static const EventTrigger& EnteredRelationship;
        static const EventTrigger& ExitedRelationship;
        static const EventTrigger& FirstCoitalAct;
        static const EventTrigger& ExposureComplete;
        static const EventTrigger& VectorToHumanTransmission;
        static const EventTrigger& HumanToVectorTransmission;
        static const EventTrigger& ReceivedInfectiousBites;
        static const EventTrigger& InfectionCleared;
        static const EventTrigger& WouldHaveDied;
        static const EventTrigger& WouldHaveHadAIDS;
        static const EventTrigger& WouldHaveEnteredLatentStage;
        static const EventTrigger& HIVInfectionStageEnteredLatent;
        static const EventTrigger& HIVInfectionStageEnteredAIDS;
        static const EventTrigger& HIVInfectionStageEnteredOnART;
        static const EventTrigger& NewMalariaInfectionObject;
        static const EventTrigger& PFA_Entered;
        static const EventTrigger& PFA_Exited;
        static const EventTrigger& PFA_SeekingPartner;
        static const EventTrigger& PFA_FoundPartner;
        static const EventTrigger& PFA_NoPartnerFound;
        static const EventTrigger & MaritalCondomlessSexAct;
        static const EventTrigger & NonMaritalCondomlessSexAct;


    public:
        EventTrigger();
        explicit EventTrigger( const std::string &init_str );
        explicit EventTrigger( const char *init_str );
        ~EventTrigger();

    protected:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        explicit EventTrigger( EventTriggerInternal* peti );
    };

    class EventTriggerFactory : public BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>
    {
        GET_SCHEMA_STATIC_WRAPPER( EventTriggerFactory )
    public:

        virtual ~EventTriggerFactory();

    private:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        EventTriggerFactory();
    };
}
