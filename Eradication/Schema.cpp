
#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <sstream> // ostringstream

#ifdef WIN32
#include <windows.h>
#endif

#include "Schema.h"
#include "Log.h"
#include "ProgVersion.h"
#include "DllLoader.h"
#include "EventTrigger.h"
#include "InterventionFactory.h"
#include "ReportFactory.h"
#include "SimulationConfig.h"
#include "CampaignEvent.h"
#include "EventCoordinator.h"
#include "IWaningEffect.h"
#include "AdditionalRestrictionsFactory.h"
#include "PythonSupport.h"
#include "Memory.h"
#include "FileSystem.h"
#include "iv_params.rc"

SETUP_LOGGING( "Schema" )

#define CAMP_Use_Defaults_DESC_TEXT "Set to true (1) if you don't want to have to specify all params for event coordinators and interventions. Use at own risk." 
#define REPORTS_Use_Defaults_DESC_TEXT "Set to true (1) if you don't want to have to specify all params for the reports."

const std::vector<std::string> getSimTypeList()
{
    const char * simTypeListC[] = { "GENERIC_SIM"
#ifndef DISABLE_VECTOR
        , "VECTOR_SIM"
#endif
#ifndef DISABLE_MALARIA
        , "MALARIA_SIM"
#endif
#ifndef DISABLE_STI
        , "STI_SIM"
#endif
#ifndef DISABLE_HIV
        , "HIV_SIM"
#endif
    };

#define KNOWN_SIM_COUNT (sizeof(simTypeListC)/sizeof(simTypeListC[0]))
    std::vector<std::string> simTypeList( simTypeListC, simTypeListC + KNOWN_SIM_COUNT );
    return simTypeList;
}



/////////////////////////////////////////////////////////////////////////////////////////////
// access to input schema embedding

void IDMAPI writeInputSchemas(
    const char* dll_path,
    const char* output_path
)
{
    std::ofstream schema_ostream_file;
    json::Object fakeJsonRoot;
    json::QuickBuilder total_schema( fakeJsonRoot );

    // Get DTK version into schema
    json::Object vsRoot;
    json::QuickBuilder versionSchema( vsRoot );
    ProgDllVersion pv;
    versionSchema["DTK_Version"] = json::String( pv.getVersion() );
    versionSchema["DTK_Branch"] = json::String( pv.getSccsBranch() );
    versionSchema["DTK_Build_Date"] = json::String( pv.getBuildDate() );
    total_schema["Version"] = versionSchema.As<json::Object>();

    std::string szOutputPath = std::string( output_path );
    if( szOutputPath != "stdout" )
    {
        // Attempt to open output path
        FileSystem::OpenFileForWriting( schema_ostream_file, output_path );
    }
    std::ostream &schema_ostream = ( ( szOutputPath == "stdout" ) ? std::cout : schema_ostream_file );

    Kernel::JsonConfigurable::_dryrun = true;
    std::ostringstream oss;

    // ---------------------------
    // --- Create Config Schema
    // ---------------------------
    json::Object configSchemaAll;

    for (auto& sim_type : getSimTypeList())
    {
        json::Object fakeSimTypeConfigJson;
        fakeSimTypeConfigJson["Simulation_Type"] = json::String(sim_type);
        Configuration * fakeConfig = Configuration::CopyFromElement( fakeSimTypeConfigJson );
        Kernel::SimulationConfig * pConfig = Kernel::SimulationConfigFactory::CreateInstance(fakeConfig);
        release_assert( pConfig );

        json::QuickBuilder config_schema = pConfig->GetSchema();
        configSchemaAll[sim_type] = config_schema;

        delete fakeConfig;
        fakeConfig = nullptr;
    }

    for (auto& entry : Kernel::JsonConfigurable::get_registration_map())
    {
        const std::string& classname = entry.first;
        json::QuickBuilder config_schema = ((*(entry.second))());
        configSchemaAll[classname] = config_schema;
    }

    total_schema[ "config" ] = configSchemaAll;

    // ---------------------------
    // --- Create Campaign Schema
    // ---------------------------
    if( !Kernel::InterventionFactory::getInstance() )
    {
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, "Kernel::InterventionFactory::getInstance(" );
    }

    json::QuickBuilder ces_schema = Kernel::CampaignEventFactory::getInstance()->GetSchema();
    json::QuickBuilder ecs_schema = Kernel::EventCoordinatorFactory::getInstance()->GetSchema();
    json::QuickBuilder ivs_schema = Kernel::InterventionFactory::getInstance()->GetSchema();
    json::QuickBuilder ns_schema  = Kernel::NodeSetFactory::getInstance()->GetSchema();
    json::QuickBuilder we_schema  = Kernel::WaningEffectFactory::getInstance()->GetSchema();
    json::QuickBuilder ar_schema  = Kernel::AdditionalRestrictionsFactory::getInstance()->GetSchema();

    json::Object objRoot;
    json::QuickBuilder camp_schema( objRoot );

    json::Object useDefaultsRoot;
    json::QuickBuilder udSchema( useDefaultsRoot );
    udSchema["type"] = json::String( "bool" );
    udSchema["default"] = json::Number( 0 );
    udSchema["description"] = json::String( CAMP_Use_Defaults_DESC_TEXT );
    camp_schema["Use_Defaults"] = udSchema.As<json::Object>();

    camp_schema["Events"][0] = json::String( "idmType:CampaignEvent" );

    camp_schema[ "idmTypes" ][ "idmType:CampaignEvent"          ] = ces_schema.As<json::Object>();
    camp_schema[ "idmTypes" ][ "idmType:EventCoordinator"       ] = ecs_schema.As<json::Object>();
    camp_schema[ "idmTypes" ][ "idmType:Intervention"           ] = ivs_schema.As<json::Object>();
    camp_schema[ "idmTypes" ][ "idmType:NodeSet"                ] = ns_schema.As<json::Object>();
    camp_schema[ "idmTypes" ][ "idmType:WaningEffect"           ] = we_schema.As<json::Object>();
    camp_schema[ "idmTypes" ][ "idmType:AdditionalRestrictions" ] = ar_schema.As<json::Object>();

    total_schema[ "interventions" ] = camp_schema.As< json::Object> ();

    // ---------------------------
    // --- Create Reports Schema
    // ---------------------------
    json::Object reports_objRoot;
    json::QuickBuilder reports_schema( reports_objRoot );

    json::Object reports_useDefaultsRoot;
    json::QuickBuilder reports_udSchema( reports_useDefaultsRoot );
    reports_udSchema["type"] = json::String( "bool" );
    reports_udSchema["default"] = json::Number( 0 );
    reports_udSchema["description"] = json::String( REPORTS_Use_Defaults_DESC_TEXT );
    reports_schema["Use_Defaults"] = reports_udSchema.As<json::Object>();

    reports_schema["Reports"][0] = json::String( "idmType:IReport" );

    json::QuickBuilder rf_schema  = Kernel::ReportFactory::getInstance()->GetSchema();

    reports_schema["idmTypes" ][ "idmType:IReport" ] = rf_schema.As<json::Object>();

    total_schema[ "reports" ] = reports_schema.As< json::Object> ();

    // --------------------------
    // --- Write Schema to output
    // --------------------------
    json::Writer::Write( total_schema, schema_ostream );
    schema_ostream_file.close();

    // PythonSupportPtr can be null during componentTests
    if( szOutputPath != "stdout" )
    {
        std::cout << "Successfully created schema in file " << output_path << ". Attempting to post-process." << std::endl;
        Kernel::PythonSupport::RunPyFunction( output_path, Kernel::PythonSupport::SCRIPT_POST_PROCESS_SCHEMA );
    }
}

