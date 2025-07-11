
#pragma once

#include "stdafx.h"
#include "EventTriggerNode.h"
#include "BaseEventTriggerTemplates.h"

namespace Kernel
{
    // -----------------------------------------------------------------------------------------------
    // --- These built-in triggers are static variables that need to be in sync with the 
    // --- same static varaibles in the EXE and DLLs.  To do this, they are first created
    // --- such that the variable is a reference to an empty/uninitialized EventTriggerNode
    // --- that is maintained in m_VectorBuiltIn.  During Configure(), the EventTriggerInternal
    // --- is created for each of these built-in triggers.  Hence, one must call the Configure()
    // --- before attempting to use any triggers and it should only be called in the EXE.
    // ---
    // --- The DLLs get the EventTriggerNodeFactory instance via the Environment instance being
    // --- set in DllInterfaceHelper::GetEModuleVersion().  This method calls Environment::setInstance()
    // --- which calls EventTriggerNodeFactory::SetBuiltIn().  SetBuiltIn() assigns the
    // --- EventTriggerInternal's of these statics with those from the EXE.
    // -----------------------------------------------------------------------------------------------
    std::vector<std::pair<std::string,EventTriggerNode*>> EventTriggerNodeFactory::m_VectorBuiltIn;
    //const EventTriggerNode& EventTriggerNode::SheddingComplete                     = EventTriggerNodeFactory::CreateBuiltInEventTrigger( "SheddingComplete"                    );

    const EventType::Enum EventTriggerNodeFactory::EVENT_TYPE = EventType::NODE;

    const char* EventTriggerNodeFactory::CONSTRAINT_SCHEMA_STRING = "'Custom_Node_Events' or Built-in";
    const char* EventTriggerNodeFactory::USER_EVENTS_PARAMETER_NAME = "Custom_Node_Events";
    const char* EventTriggerNodeFactory::USER_EVENTS_PARAMETER_DESC = Custom_Node_Events_DESC_TEXT;

    // ------------------------------------------------------------------------
    // --- EventTriggerNode
    // ------------------------------------------------------------------------

    EventTriggerNode::EventTriggerNode()
        : BaseEventTrigger()
    {
    }

    EventTriggerNode::EventTriggerNode( const std::string &init_str )
        : BaseEventTrigger( init_str )
    {
    }

    EventTriggerNode::EventTriggerNode( const char *init_str )
        : BaseEventTrigger( init_str )
    {
    }

    EventTriggerNode::EventTriggerNode( EventTriggerInternal* peti )
        : BaseEventTrigger( peti )
    {
    }

    EventTriggerNode::~EventTriggerNode()
    {
    }

    // ------------------------------------------------------------------------
    // --- EventTriggerNodeFactory
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL( EventTriggerNodeFactory, EventTriggerNodeFactory )

    EventTriggerNodeFactory::EventTriggerNodeFactory()
    : BaseEventTriggerFactory()
    {
    }

    EventTriggerNodeFactory::~EventTriggerNodeFactory()
    {
    }

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template bool               BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::operator==( const EventTriggerNode& ) const;
    template bool               BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::operator!=( const EventTriggerNode& ) const;
    template bool               BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::IsUninitialized() const;
    template const std::string& BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::ToString() const;
    template const char*        BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::c_str() const;
    template int                BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::GetIndex() const;
    template void               BaseEventTrigger<EventTriggerNode, EventTriggerNodeFactory>::serialize( IArchive& ar, EventTriggerNode& obj );

    template void                            BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::DeleteInstance();
    template void                            BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::SetBuiltIn();
    template int                             BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::GetNumEventTriggers() const;
    template std::vector<EventTriggerNode>   BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::GetAllEventTriggers();
    template bool                            BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::IsValidEvent( const std::string& candidateEvent ) const;
    template const std::string&              BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::GetEventTriggerName( int eventIndex ) const;
    template EventTriggerNode                BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::CreateUserEventTrigger( const std::string& name );
    template EventTriggerNode                BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::CreateTrigger( const std::string& rParamName, const std::string& rTriggerName );
    template std::vector<EventTriggerNode>   BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::CreateTriggerList( const std::string& rParamName, const std::vector<std::string>& rTriggerNameList );
    template const std::vector<std::string>& BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>::GetBuiltInNames();
}
