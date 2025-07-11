
#pragma once

#include "stdafx.h"
#include "EventTriggerCoordinator.h"
#include "BaseEventTriggerTemplates.h"

namespace Kernel
{
    // -----------------------------------------------------------------------------------------------
    // --- These built-in triggers are static variables that need to be in sync with the 
    // --- same static varaibles in the EXE and DLLs.  To do this, they are first created
    // --- such that the variable is a reference to an empty/uninitialized EventTriggerCoordinator
    // --- that is maintained in m_VectorBuiltIn.  During Configure(), the EventTriggerInternal
    // --- is created for each of these built-in triggers.  Hence, one must call the Configure()
    // --- before attempting to use any triggers and it should only be called in the EXE.
    // ---
    // --- The DLLs get the EventTriggerCoordinatorFactory instance via the Environment instance being
    // --- set in DllInterfaceHelper::GetEModuleVersion().  This method calls Environment::setInstance()
    // --- which calls EventTriggerCoordinatorFactory::SetBuiltIn().  SetBuiltIn() assigns the
    // --- EventTriggerInternal's of these statics with those from the EXE.
    // -----------------------------------------------------------------------------------------------
    std::vector<std::pair<std::string,EventTriggerCoordinator*>> EventTriggerCoordinatorFactory::m_VectorBuiltIn;

    const EventType::Enum EventTriggerCoordinatorFactory::EVENT_TYPE = EventType::COORDINATOR;
    const char* EventTriggerCoordinatorFactory::CONSTRAINT_SCHEMA_STRING = "'Custom_Coordinator_Events' or Built-in";
    const char* EventTriggerCoordinatorFactory::USER_EVENTS_PARAMETER_NAME = "Custom_Coordinator_Events";
    const char* EventTriggerCoordinatorFactory::USER_EVENTS_PARAMETER_DESC = Custom_Coordinator_Events_DESC_TEXT;

    // ------------------------------------------------------------------------
    // --- EventTriggerCoordinator
    // ------------------------------------------------------------------------

    EventTriggerCoordinator::EventTriggerCoordinator()
        : BaseEventTrigger()
    {
    }

    EventTriggerCoordinator::EventTriggerCoordinator( const std::string &init_str )
        : BaseEventTrigger( init_str )
    {
    }

    EventTriggerCoordinator::EventTriggerCoordinator( const char *init_str )
        : BaseEventTrigger( init_str )
    {
    }

    EventTriggerCoordinator::EventTriggerCoordinator( EventTriggerInternal* peti )
        : BaseEventTrigger( peti )
    {
    }

    EventTriggerCoordinator::~EventTriggerCoordinator()
    {
    }

    // ------------------------------------------------------------------------
    // --- EventTriggerCoordinatorFactory
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL( EventTriggerCoordinatorFactory, EventTriggerCoordinatorFactory )

    EventTriggerCoordinatorFactory::EventTriggerCoordinatorFactory()
    : BaseEventTriggerFactory()
    {
    }

    EventTriggerCoordinatorFactory::~EventTriggerCoordinatorFactory()
    {
    }

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template bool               BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::operator==( const EventTriggerCoordinator& ) const;
    template bool               BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::operator!=( const EventTriggerCoordinator& ) const;
    template bool               BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::IsUninitialized() const;
    template const std::string& BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::ToString() const;
    template const char*        BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::c_str() const;
    template int                BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::GetIndex() const;
    template void               BaseEventTrigger<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::serialize( IArchive& ar, EventTriggerCoordinator& obj );

    template void                                 BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::DeleteInstance();
    template void                                 BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::SetBuiltIn();
    template int                                  BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::GetNumEventTriggers() const;
    template std::vector<EventTriggerCoordinator> BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::GetAllEventTriggers();
    template bool                                 BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::IsValidEvent( const std::string& candidateEvent ) const;
    template const std::string&                   BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::GetEventTriggerName( int eventIndex ) const;
    template EventTriggerCoordinator              BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::CreateUserEventTrigger( const std::string& name );
    template EventTriggerCoordinator              BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::CreateTrigger( const std::string& rParamName, const std::string& rTriggerName );
    template std::vector<EventTriggerCoordinator> BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::CreateTriggerList( const std::string& rParamName, const std::vector<std::string>& rTriggerNameList );
    template const std::vector<std::string>&      BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>::GetBuiltInNames();
}
