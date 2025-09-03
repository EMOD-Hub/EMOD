
#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <sstream> // ostringstream

#ifdef WIN32
#include <windows.h>
#endif

#include "Schema.h"
#include "IdmString.h"
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


const std::string getSupportedSimsString()
{
    const auto sims = getSimTypeList();
    std::string sim_types_str;
    sim_types_str.reserve( sims.size() * 20 ); // Reserve space to avoid multiple reallocations
    for( size_t i = 0; i < sims.size(); ++i ) {
        const std::string sim_type_prefix = IdmString( sims[i] ).split( '_' )[0];
        sim_types_str += sim_type_prefix;

        if( i < sims.size() - 1 ) // Avoids trailing comma
        {
            sim_types_str += ", ";
        }
    }
    return sim_types_str;
}

void writeInputSchemas( const char* output_path )
{
    json::Object jsonRoot;
    json::QuickBuilder total_schema( jsonRoot );
    Kernel::JsonConfigurable::_dryrun = true;

    // --- Create Metadata Schema
    json::Object vsRoot;
    json::QuickBuilder versionSchema( vsRoot );

    ProgDllVersion pv;
    versionSchema["DTK_Version"] = json::String( pv.getVersion() );
    versionSchema["DTK_Branch"] = json::String( pv.getSccsBranch() );
    versionSchema["DTK_Build_Date"] = json::String( pv.getBuildDate() );
    versionSchema["Supported_Simulation_Types"] = json::String( getSupportedSimsString() );
    total_schema["Version"] = versionSchema.As<json::Object>();

    // --- Create Config Schema
    json::Object configSchemaAll;

    for (auto& sim_type : getSimTypeList())
    {
        json::Object fakeSimTypeConfigJson;
        fakeSimTypeConfigJson["Simulation_Type"] = json::String(sim_type);
        Configuration* fakeConfig = Configuration::CopyFromElement( fakeSimTypeConfigJson );
        Kernel::SimulationConfig* pConfig = Kernel::SimulationConfigFactory::CreateInstance(fakeConfig);
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

    // --- Create Campaign Schema
    json::Object camp_defaults_root;
    json::QuickBuilder camp_ud_schema( camp_defaults_root );

    camp_ud_schema["type"] = json::String( "bool" );
    camp_ud_schema["default"] = json::Number( 0 );
    camp_ud_schema["description"] = json::String( CAMP_Use_Defaults_DESC_TEXT );

    json::Object camp_root;
    json::QuickBuilder camp_schema( camp_root );

    camp_schema["Events"][0] = json::String( "idmAbstractType:CampaignEvent" );
    camp_schema["Use_Defaults"] = camp_ud_schema.As<json::Object>();

    total_schema["interventions"] = camp_schema.As<json::Object>();

    // --- Create Reports Schema
    json::Object reports_defaults_root;
    json::QuickBuilder report_ud_schema( reports_defaults_root );

    report_ud_schema["type"] = json::String( "bool" );
    report_ud_schema["default"] = json::Number( 0 );
    report_ud_schema["description"] = json::String( REPORTS_Use_Defaults_DESC_TEXT );

    json::Object reports_root;
    json::QuickBuilder reports_schema( reports_root );

    reports_schema["Reports"][0] = json::String( "idmType:IReport" );
    reports_schema["Use_Defaults"] = report_ud_schema.As<json::Object>();

    total_schema["reports"] = reports_schema.As<json::Object>();

    // --- Create idmTypes Schema
    json::Object ivtypes_root;
    json::QuickBuilder ivt_schema( ivtypes_root );

    json::QuickBuilder ivs_schema = Kernel::IndividualIVFactory::getInstance()->GetSchema();
    json::QuickBuilder nvs_schema = Kernel::NodeIVFactory::getInstance()->GetSchema();

    ivt_schema["idmAbstractType:IndividualIntervention"] = ivs_schema.As<json::Object>();
    ivt_schema["idmAbstractType:NodeIntervention"]       = nvs_schema.As<json::Object>();

    json::Object idmtypes_root;
    json::QuickBuilder idmtypes_schema( idmtypes_root );

    json::QuickBuilder ces_schema = Kernel::CampaignEventFactory::getInstance()->GetSchema();
    json::QuickBuilder ecs_schema = Kernel::EventCoordinatorFactory::getInstance()->GetSchema();
    json::QuickBuilder nds_schema = Kernel::NodeSetFactory::getInstance()->GetSchema();
    json::QuickBuilder rpt_schema = Kernel::ReportFactory::getInstance()->GetSchema();
    json::QuickBuilder we_schema = Kernel::WaningEffectFactory::getInstance()->GetSchema();
    json::QuickBuilder ar_schema = Kernel::AdditionalRestrictionsFactory::getInstance()->GetSchema();

    idmtypes_schema["idmAbstractType:CampaignEvent"] = ces_schema.As<json::Object>();
    idmtypes_schema["idmAbstractType:Intervention"] = ivt_schema.As<json::Object>();
    idmtypes_schema["idmAbstractType:EventCoordinator"] = ecs_schema.As<json::Object>();
    idmtypes_schema["idmAbstractType:NodeSet"] = nds_schema.As<json::Object>();
    idmtypes_schema["idmType:IReport"] = rpt_schema.As<json::Object>();  // Should be abstract type!
    idmtypes_schema["idmType:WaningEffect"] = we_schema.As<json::Object>();  // Should be abstract type!
    idmtypes_schema["idmType:AdditionalRestrictions"] = ar_schema.As<json::Object>();

    total_schema[ "idmTypes" ] = idmtypes_schema.As<json::Object>();

    // --- Post-processing schema
    json::Object idmtypes_addl;
    json::SchemaUpdater updater(idmtypes_addl);
    jsonRoot.Accept(updater);

    json::Object::iterator it(idmtypes_addl.Begin());
    json::Object::iterator itEnd(idmtypes_addl.End());

    while (it != itEnd)
    {
        total_schema["idmTypes"][it->name] = it->element;
        it++;
    }

    // --- Write Schema to output
    std::ofstream schema_ostream_file;
    std::string szOutputPath = std::string( output_path );
    if( szOutputPath != "stdout" )
    {
        // Attempt to open output path
        FileSystem::OpenFileForWriting( schema_ostream_file, output_path );
    }
    std::ostream &schema_ostream = ( ( szOutputPath == "stdout" ) ? std::cout : schema_ostream_file );

    json::Writer::Write( total_schema, schema_ostream, "    ", true, true );
    schema_ostream_file.close();

    // PythonSupportPtr can be null during componentTests
    if( szOutputPath != "stdout" )
    {
        Kernel::PythonSupport::RunPyFunction( output_path, Kernel::PythonSupport::SCRIPT_POST_PROCESS_SCHEMA );
        std::cout << "Created schema file " << output_path << ". " << std::endl;
    }
}


namespace json
{
    void SchemaUpdater::Visit(Array& array)
    {
        Array::iterator it(array.Begin());
        Array::iterator itEnd(array.End());

        // Iterate over elements
        while (it != itEnd)
        {
            // No action; 
            it->Accept(*this); 
            it++;
        }
    }

    void SchemaUpdater::Visit(Object& object)
    {
        Object::iterator it(object.Begin());
        Object::iterator itEnd(object.End());

        // Iterate over elements
        while (it != itEnd)
        {
            it->element.Accept(*this);

            // Aggregate idmTypes to separate object
            std::string key_str = it->name;
            if(key_str.rfind("idmType:", 0) == 0)
            {
                if(!m_idmt_schema.Exist(it->name))
                {
                    Object::Member obj_copy(key_str, Element(it->element));
                    m_idmt_schema.Insert(obj_copy);
                }
                it = object.Erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void SchemaUpdater::Visit(String& stringElement)
    {
        // No action;
    }

    void SchemaUpdater::Visit(Number& number)
    {
        // No action;
    }

    void SchemaUpdater::Visit(Uint64& number)
    {
        // No action;
    }

    void SchemaUpdater::Visit(Boolean& booleanElement)
    {
        // No action;
    }

    void SchemaUpdater::Visit(Null& nullElement)
    {
        // No action;
    }
}
