
#pragma once

#include "stdafx.h"
#include "EventTrigger.h"
#include "BaseEventTriggerTemplates.h"

namespace Kernel
{
    // -----------------------------------------------------------------------------------------------
    // --- These built-in triggers are static variables that need to be in sync with the 
    // --- same static varaibles in the EXE and DLLs.  To do this, they are first created
    // --- such that the variable is a reference to an empty/uninitialized EventTrigger
    // --- that is maintained in m_VectorBuiltIn.  During Configure(), the EventTriggerInternal
    // --- is created for each of these built-in triggers.  Hence, one must call the Configure()
    // --- before attempting to use any triggers and it should only be called in the EXE.
    // ---
    // --- The DLLs get the EventTriggerFactory instance via the Environment instance being
    // --- set in DllInterfaceHelper::GetEModuleVersion().  This method calls Environment::setInstance()
    // --- which calls EventTriggerFactory::SetBuiltIn().  SetBuiltIn() assigns the
    // --- EventTriggerInternal's of these statics with those from the EXE.
    // -----------------------------------------------------------------------------------------------
    std::vector<std::pair<std::string,EventTrigger*>> EventTriggerFactory::m_VectorBuiltIn;

    const EventTrigger& EventTrigger::Births                         = EventTriggerFactory::CreateBuiltInEventTrigger( "Births"                         );
    const EventTrigger& EventTrigger::EveryUpdate                    = EventTriggerFactory::CreateBuiltInEventTrigger( "EveryUpdate"                    );
    const EventTrigger& EventTrigger::NewInfectionEvent              = EventTriggerFactory::CreateBuiltInEventTrigger( "NewInfectionEvent"              );
    const EventTrigger& EventTrigger::NewClinicalCase                = EventTriggerFactory::CreateBuiltInEventTrigger( "NewClinicalCase"                );
    const EventTrigger& EventTrigger::NewSevereCase                  = EventTriggerFactory::CreateBuiltInEventTrigger( "NewSevereCase"                  );
    const EventTrigger& EventTrigger::DiseaseDeaths                  = EventTriggerFactory::CreateBuiltInEventTrigger( "DiseaseDeaths"                  );
    const EventTrigger& EventTrigger::NonDiseaseDeaths               = EventTriggerFactory::CreateBuiltInEventTrigger( "NonDiseaseDeaths"               );
    const EventTrigger& EventTrigger::PropertyChange                 = EventTriggerFactory::CreateBuiltInEventTrigger( "PropertyChange"                 );
    const EventTrigger& EventTrigger::STIDebut                       = EventTriggerFactory::CreateBuiltInEventTrigger( "STIDebut"                       );
    const EventTrigger& EventTrigger::StartedART                     = EventTriggerFactory::CreateBuiltInEventTrigger( "StartedART"                     );
    const EventTrigger& EventTrigger::StoppedART                     = EventTriggerFactory::CreateBuiltInEventTrigger( "StoppedART"                     );
    const EventTrigger& EventTrigger::InterventionDisqualified       = EventTriggerFactory::CreateBuiltInEventTrigger( "InterventionDisqualified"       );
    const EventTrigger& EventTrigger::HIVNewlyDiagnosed              = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVNewlyDiagnosed"              );
    const EventTrigger& EventTrigger::GaveBirth                      = EventTriggerFactory::CreateBuiltInEventTrigger( "GaveBirth"                      );
    const EventTrigger& EventTrigger::Pregnant                       = EventTriggerFactory::CreateBuiltInEventTrigger( "Pregnant"                       );
    const EventTrigger& EventTrigger::Emigrating                     = EventTriggerFactory::CreateBuiltInEventTrigger( "Emigrating"                     );
    const EventTrigger& EventTrigger::Immigrating                    = EventTriggerFactory::CreateBuiltInEventTrigger( "Immigrating"                    );
    const EventTrigger& EventTrigger::HIVTestedNegative              = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVTestedNegative"              );
    const EventTrigger& EventTrigger::HIVTestedPositive              = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVTestedPositive"              );
    const EventTrigger& EventTrigger::NewlySymptomatic               = EventTriggerFactory::CreateBuiltInEventTrigger( "NewlySymptomatic"               );
    const EventTrigger& EventTrigger::SymptomaticCleared             = EventTriggerFactory::CreateBuiltInEventTrigger( "SymptomaticCleared"             );
    const EventTrigger& EventTrigger::TwelveWeeksPregnant            = EventTriggerFactory::CreateBuiltInEventTrigger( "TwelveWeeksPregnant"            );
    const EventTrigger& EventTrigger::FourteenWeeksPregnant          = EventTriggerFactory::CreateBuiltInEventTrigger( "FourteenWeeksPregnant"          );
    const EventTrigger& EventTrigger::SixWeeksOld                    = EventTriggerFactory::CreateBuiltInEventTrigger( "SixWeeksOld"                    );
    const EventTrigger& EventTrigger::EighteenMonthsOld              = EventTriggerFactory::CreateBuiltInEventTrigger( "EighteenMonthsOld"              );
    const EventTrigger& EventTrigger::STIPreEmigrating               = EventTriggerFactory::CreateBuiltInEventTrigger( "STIPreEmigrating"               );
    const EventTrigger& EventTrigger::STIPostImmigrating             = EventTriggerFactory::CreateBuiltInEventTrigger( "STIPostImmigrating"             );
    const EventTrigger& EventTrigger::STINewInfection                = EventTriggerFactory::CreateBuiltInEventTrigger( "STINewInfection"                ); 
    const EventTrigger& EventTrigger::STIExposed                     = EventTriggerFactory::CreateBuiltInEventTrigger( "STIExposed"                     );
    const EventTrigger& EventTrigger::NodePropertyChange             = EventTriggerFactory::CreateBuiltInEventTrigger( "NodePropertyChange"             );
    const EventTrigger& EventTrigger::HappyBirthday                  = EventTriggerFactory::CreateBuiltInEventTrigger( "HappyBirthday"                  );
    const EventTrigger& EventTrigger::EnteredRelationship            = EventTriggerFactory::CreateBuiltInEventTrigger( "EnteredRelationship"            );
    const EventTrigger& EventTrigger::ExitedRelationship             = EventTriggerFactory::CreateBuiltInEventTrigger( "ExitedRelationship"             );
    const EventTrigger& EventTrigger::FirstCoitalAct                 = EventTriggerFactory::CreateBuiltInEventTrigger( "FirstCoitalAct"                 );
    const EventTrigger& EventTrigger::ExposureComplete               = EventTriggerFactory::CreateBuiltInEventTrigger( "ExposureComplete"               );
    const EventTrigger& EventTrigger::VectorToHumanTransmission      = EventTriggerFactory::CreateBuiltInEventTrigger( "VectorToHumanTransmission"      );
    const EventTrigger& EventTrigger::HumanToVectorTransmission      = EventTriggerFactory::CreateBuiltInEventTrigger( "HumanToVectorTransmission"      );
    const EventTrigger& EventTrigger::ReceivedInfectiousBites        = EventTriggerFactory::CreateBuiltInEventTrigger( "ReceivedInfectiousBites"        );
    const EventTrigger& EventTrigger::InfectionCleared               = EventTriggerFactory::CreateBuiltInEventTrigger( "InfectionCleared"               );
    const EventTrigger& EventTrigger::WouldHaveDied                  = EventTriggerFactory::CreateBuiltInEventTrigger( "WouldHaveDied"                  );
    const EventTrigger& EventTrigger::WouldHaveHadAIDS               = EventTriggerFactory::CreateBuiltInEventTrigger( "WouldHaveHadAIDS"               );
    const EventTrigger& EventTrigger::WouldHaveEnteredLatentStage    = EventTriggerFactory::CreateBuiltInEventTrigger( "WouldHaveEnteredLatentStage"    );
    const EventTrigger& EventTrigger::HIVInfectionStageEnteredLatent = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVInfectionStageEnteredLatent" );
    const EventTrigger& EventTrigger::HIVInfectionStageEnteredAIDS   = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVInfectionStageEnteredAIDS"   );
    const EventTrigger& EventTrigger::HIVInfectionStageEnteredOnART  = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVInfectionStageEnteredOnART"  );
    const EventTrigger& EventTrigger::NewMalariaInfectionObject      = EventTriggerFactory::CreateBuiltInEventTrigger( "NewMalariaInfectionObject"      );
    const EventTrigger& EventTrigger::PFA_Entered                    = EventTriggerFactory::CreateBuiltInEventTrigger( "PFA_Entered"                    );
    const EventTrigger& EventTrigger::PFA_Exited                     = EventTriggerFactory::CreateBuiltInEventTrigger( "PFA_Exited"                     );
    const EventTrigger& EventTrigger::PFA_SeekingPartner             = EventTriggerFactory::CreateBuiltInEventTrigger( "PFA_SeekingPartner"             );
    const EventTrigger& EventTrigger::PFA_FoundPartner               = EventTriggerFactory::CreateBuiltInEventTrigger( "PFA_FoundPartner"               );
    const EventTrigger& EventTrigger::PFA_NoPartnerFound             = EventTriggerFactory::CreateBuiltInEventTrigger( "PFA_NoPartnerFound"             );
    const EventTrigger & EventTrigger::MaritalCondomlessSexAct       = EventTriggerFactory::CreateBuiltInEventTrigger( "MaritalCondomlessSexAct"        );
    const EventTrigger & EventTrigger::NonMaritalCondomlessSexAct    = EventTriggerFactory::CreateBuiltInEventTrigger( "NonMaritalCondomlessSexAct"     );


    const EventType::Enum EventTriggerFactory::EVENT_TYPE = EventType::INDIVIDUAL;
    const char* EventTriggerFactory::CONSTRAINT_SCHEMA_STRING = "'Custom_Individual_Events' or Built-in";
    const char* EventTriggerFactory::USER_EVENTS_PARAMETER_NAME = "Custom_Individual_Events";
    const char* EventTriggerFactory::USER_EVENTS_PARAMETER_DESC = Custom_Individual_Events_DESC_TEXT;

    // ------------------------------------------------------------------------
    // --- EventTrigger
    // ------------------------------------------------------------------------

    EventTrigger::EventTrigger()
        : BaseEventTrigger()
    {
    }

    EventTrigger::EventTrigger( const std::string &init_str )
        : BaseEventTrigger( init_str )
    {
    }

    EventTrigger::EventTrigger( const char *init_str )
        : BaseEventTrigger( init_str )
    {
    }

    EventTrigger::EventTrigger( EventTriggerInternal* peti )
        : BaseEventTrigger( peti )
    {
    }

    EventTrigger::~EventTrigger()
    {
    }

    // ------------------------------------------------------------------------
    // --- EventTriggerFactory
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL( EventTriggerFactory, EventTriggerFactory )

    EventTriggerFactory::EventTriggerFactory()
    : BaseEventTriggerFactory()
    {
    }

    EventTriggerFactory::~EventTriggerFactory()
    {
    }

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template bool               BaseEventTrigger<EventTrigger, EventTriggerFactory>::operator==( const EventTrigger& ) const;
    template bool               BaseEventTrigger<EventTrigger, EventTriggerFactory>::operator!=( const EventTrigger& ) const;
    template bool               BaseEventTrigger<EventTrigger, EventTriggerFactory>::IsUninitialized() const;
    template const std::string& BaseEventTrigger<EventTrigger, EventTriggerFactory>::ToString() const;
    template const char*        BaseEventTrigger<EventTrigger, EventTriggerFactory>::c_str() const;
    template int                BaseEventTrigger<EventTrigger, EventTriggerFactory>::GetIndex() const;
    template void               BaseEventTrigger<EventTrigger, EventTriggerFactory>::serialize( IArchive& ar, EventTrigger& obj );

    template void                            BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::DeleteInstance();
    template void                            BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::SetBuiltIn();
    template int                             BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::GetNumEventTriggers() const;
    template std::vector<EventTrigger>       BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::GetAllEventTriggers();
    template bool                            BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::IsValidEvent( const std::string& candidateEvent ) const;
    template const std::string&              BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::GetEventTriggerName( int eventIndex ) const;
    template EventTrigger                    BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::CreateUserEventTrigger( const std::string& name );
    template EventTrigger                    BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::CreateTrigger( const std::string& rParamName, const std::string& rTriggerName );
    template std::vector<EventTrigger>       BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::CreateTriggerList( const std::string& rParamName, const std::vector<std::string>& rTriggerNameList );
    template const std::vector<std::string>& BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>::GetBuiltInNames();
}
