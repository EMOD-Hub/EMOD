
#include "stdafx.h"
#include "ReportEventCounter.h"
#include "Environment.h"
#include "IndividualEventContext.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#endif

SETUP_LOGGING( "ReportEventCounter" )

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "*", nullptr };// <<< Types of simulation the report is to be used with

instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportEventCounter()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char*
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char *
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void
GetReportInstantiator( Kernel::instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif
#endif //_REPORT_DLL

    BEGIN_QUERY_INTERFACE_DERIVED( ReportEventCounter, BaseEventReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportEventCounter, BaseEventReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportEventCounter )
#endif

    ReportEventCounter::ReportEventCounter()
        : BaseEventReport( "ReportEventCounter", true, true )
        , channelDataMap()
        , unitsMap()
        , event_trigger_index_to_channel_id()
    {
        initSimTypes( 1, "*" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportEventCounter::~ReportEventCounter()
    {
    }

    bool ReportEventCounter::Configure( const Configuration * inputJson )
    {
        bool ret = BaseEventReport::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            int num_triggers = EventTriggerFactory::GetInstance()->GetNumEventTriggers();
            event_trigger_index_to_channel_id.resize( num_triggers );

            const std::vector< EventTrigger >& trigger_list = GetEventTriggerList();
            for( auto trigger : trigger_list )
            {
                unitsMap[ trigger.ToString() ] = "" ;
                ChannelID id = channelDataMap.AddChannel( trigger.ToString() );
                event_trigger_index_to_channel_id[ trigger.GetIndex() ] = id;
            }
        }
        return ret;
    }

    void ReportEventCounter::BeginTimestep()
    {
        if( HaveRegisteredAllEvents() && !HaveUnregisteredAllEvents() )
        {
            channelDataMap.IncreaseChannelLength( 1 );
        }
    }

    void ReportEventCounter::Reduce()
    {
        channelDataMap.Reduce();
        BaseEventReport::Reduce();
    }

    void ReportEventCounter::Finalize()
    {
        std::string output_fn = GetBaseOutputFilename() + ".json" ;
        channelDataMap.WriteOutput( output_fn, unitsMap );
        BaseEventReport::Finalize();
    }

    bool ReportEventCounter::notifyOnEvent( IIndividualHumanEventContext *context, 
                                            const EventTrigger& trigger )
    {
        if( HaveRegisteredAllEvents() &&
            !HaveUnregisteredAllEvents() &&
            report_filter.IsValidHuman( context->GetIndividualHumanConst() ) )
        {
            channelDataMap.Accumulate( event_trigger_index_to_channel_id[ trigger.GetIndex() ], 1.0);
            return true;
        }
        else
        {
            return false;
        }
    }
}
