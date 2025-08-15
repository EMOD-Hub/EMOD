
#include "stdafx.h"

#include "ReportPluginAgeAtInfection.h"
#include "Environment.h"
#include "IIndividualHuman.h"
#include "FileSystem.h"
#include "ProgVersion.h"


#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "FactorySupport.h" // for DTK_DLLEXPORT
#else
#include "RandomNumberGeneratorFactory.h"
#include "RANDOM.h"
#endif

using namespace std;
using namespace json;

SETUP_LOGGING( "ReportPluginAgeAtInfection" ) // <<< Name of this file

static const std::string _report_name = "AgeAtInfectionReport.json";

static const char * _age_at_infection_label = "AgeAtInfection";
static const char * _time_of_infection_label = "TimeOfInfection";
static const char * _weight_label = "MCWeight";

#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with NULL
// Refer to DllLoader.h for current SIMTYPES_MAXNUM
// but we don't include DllLoader.h to avoid compiling redefinition errors.
static const char * _sim_types[] = {"GENERIC_SIM", "VECTOR_SIM", "MALARIA_SIM", NULL};

Kernel::instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new Kernel::ReportPluginAgeAtInfection()); // <<< Report to create
};

Kernel::DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

//
// This is the interface function from the DTK.
//
DTK_DLLEXPORT
char*
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT void
GetReportInstantiator( Kernel::instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

DTK_DLLEXPORT
const char *
GetType()
{
    std::ostringstream oss;
    oss << "GetType called for " << _module << std::endl;
    LOG_INFO( oss.str().c_str() );
    return _module;
}

DTK_DLLEXPORT void
InitReportEnvironment(
    const Environment * pEnv
)
{
    Environment::setInstance(const_cast<Environment*>(pEnv));
}


#ifdef __cplusplus
}
#endif
#endif //_REPORT_DLL
/////////////////////////
// Initialization methods
/////////////////////////
namespace Kernel
{

BEGIN_QUERY_INTERFACE_DERIVED( ReportPluginAgeAtInfection, BaseChannelReport )
    HANDLE_INTERFACE( IReport )
    HANDLE_INTERFACE( IConfigurable )
END_QUERY_INTERFACE_DERIVED( ReportPluginAgeAtInfection, BaseChannelReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportPluginAgeAtInfection )
#endif


ReportPluginAgeAtInfection::ReportPluginAgeAtInfection()
: BaseChannelReport( _report_name )
, sampling_ratio(0.75f)
, m_pRNG( nullptr )
{
    initSimTypes( 1, "*" );
    LOG_DEBUG( "ReportPluginAgeAtInfection ctor\n" );
#ifdef _REPORT_DLL
    m_pRNG = DLL_HELPER.CreateRandomNumberGenerator( EnvPtr );
#else
    m_pRNG = RandomNumberGeneratorFactory::CreateRandomNumberGeneratorForReport();
#endif
}

ReportPluginAgeAtInfection::~ReportPluginAgeAtInfection()
{
    delete m_pRNG;
}

/////////////////////////
// steady-state methods
/////////////////////////
void
ReportPluginAgeAtInfection::EndTimestep(float currentTime, float dt)
{
    time_age_map.emplace((unsigned int)time_age_map.size()+1u, ages);
    ages.clear();
}

void
ReportPluginAgeAtInfection::LogNodeData(
    Kernel::INodeContext * const pNode
)
{
}

void 
ReportPluginAgeAtInfection::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    LOG_DEBUG( "LogIndividualData\n" );

    if (individual->GetNewInfectionState() == NewInfectionState::NewAndDetected || individual->GetNewInfectionState() == NewInfectionState::NewInfection)
    {
        LOG_DEBUG_F("Individual's New infection state is %d\n", (int)individual->GetNewInfectionState());
        if( m_pRNG->e() < sampling_ratio)
        {
            ages.push_back(individual->GetAge());
        }
    }
}


// Just calling the base class but for demo purposes leaving in because I can imagine wanting to do this custom.
void
ReportPluginAgeAtInfection::Finalize()
{
    LOG_INFO( "WriteData\n" );
    postProcessAccumulatedData();

    std::map<std::string, std::string> units_map;
    populateSummaryDataUnitsMap(units_map);

    time_t now = time(0);
#ifdef WIN32
    tm now2;
    localtime_s(&now2,&now);
    char timebuf[26];
    asctime_s(timebuf,26,&now2);
    std::string now3 = std::string(timebuf);
#else
    tm* now2 = localtime(&now);
    std::string now3 = std::string(asctime(now2));
#endif

    Element elementRoot = String();
    QuickBuilder qb(elementRoot);
    qb["Header"]["DateTime"] = String(now3.substr(0,now3.length()-1)); // have to remove trailing '\n'

    ProgDllVersion pv;
    ostringstream dtk_ver;
    dtk_ver << pv.getVersion() << " " << pv.getSccsBranch() << " " << pv.getBuildDate();
    qb["Header"]["DTK_Version"] = String(dtk_ver.str());
    qb["Header"]["Report_Version"] = String("3");
    qb["Header"]["Timesteps"] = String("3");
    qb["Channels"]["Ages"]["Units"] = String("Days");
    int timestep_ind = 0;
    for (auto& timestep_entry : time_age_map)
    {
        const float curr_timestep = timestep_entry.first;
        const vector<float> curr_ages = timestep_entry.second;

        int age_ind = 0;
        if (curr_ages.size() == 0)
        {
            qb["Channels"]["Ages"]["Data"][timestep_ind][0] = Number(-1);
        }
        else
        {
            for (auto& age_entry : curr_ages)
            {
                qb["Channels"]["Ages"]["Data"][timestep_ind][age_ind] = Number(age_entry);
                age_ind++;
            } 
        }
        timestep_ind++;

    }
    qb["Header"]["Timesteps"] = Number(timestep_ind);

    // write to an internal buffer first... if we write directly to the network share, performance is slow
    // (presumably because it's doing a bunch of really small writes of all the JSON elements instead of one
    // big write)
    ostringstream oss;
    Writer::Write(elementRoot, oss);

    ofstream inset_chart_json;
    FileSystem::OpenFileForWriting( inset_chart_json, FileSystem::Concat( EnvPtr->OutputPath, report_name ).c_str() );

    inset_chart_json << oss.str();
    inset_chart_json.close();
}

void 
ReportPluginAgeAtInfection::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    LOG_INFO( "populateSummaryDataUnitsMap\n" );
    units_map[_age_at_infection_label]                        = "Age at Infection";
    units_map[_time_of_infection_label]                       = "Time of Infection";
    units_map[_weight_label]                                  = "MC Weight";
}

// not sure whether to leave this in custom demo subclass
void
ReportPluginAgeAtInfection::postProcessAccumulatedData()
{
    LOG_DEBUG( "getSummaryDataCustomProcessed\n" );
}

void
ReportPluginAgeAtInfection::Reduce()
{
    LOG_INFO( "Reduce\n" );
    
}

}