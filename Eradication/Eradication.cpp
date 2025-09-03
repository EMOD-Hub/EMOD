
#include "stdafx.h"

#pragma warning(disable:4996)

#include <iostream>
#include <fstream>
#include <sstream> // ostringstream
#include <csignal>

#include <math.h>
#include <stdio.h>

#include "Environment.h"
#include "FileSystem.h"

#ifdef WIN32
#include <windows.h>
#else
#include <fenv.h>
#endif

#include "ProgramOptions.h"
#include "Controller.h"
#include "Debug.h"
#include "ControllerFactory.h"
#include "StatusReporter.h"
#include "PythonSupport.h"
#include "Memory.h"
#include "IdmMpi.h"
#include "IdmString.h"
#include "Schema.h"
#include "SimulationConfig.h"
#include "ConfigParams.h"

#include "Exceptions.h"

// Version info extraction for both program and dlls
#include "ProgVersion.h"
#include "DllLoader.h"

#include "InstructionSetInfo.h"


using namespace std;

int MPIInitWrapper(int argc, char* argv[]);
bool ControllerInitWrapper(int argc, char *argv[], IdmMpi::MessageInterface* pMpi ); // returns false if something broke


#ifndef WIN32
void setStackSize();
#define sprintf_s sprintf
void setStackSize();
#endif

SETUP_LOGGING( "Eradication" )


void FPE_SignalHandler( int signal )
{
    // From https://en.cppreference.com/w/cpp/utility/program/signal, "On entry to the signal handler, the state of the floating-point environment
    // and the values of all objects is unspecified... ". Value of control word seems to be reset to _MCW_EM

    std::stringstream ss;
    ss << "Floating Point Exception, signal: " << signal << ".";

    Kernel::IllegalOperationException exp(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
    std::cerr << exp.GetMsg() << "\n\n";
    std::cerr << exp.GetStackTrace() << "\n\n";
    fflush( stderr );

    std::cout << "\n\n";
    std::cout << exp.GetMsg() << "\n\n";
    std::cout << exp.GetStackTrace() << "\n\n";
    fflush( stdout );
    EnvPtr->MPI.p_idm_mpi->Abort( -1 );
    exit(-1);
}

void SetFloatingPointSignalHandler()
{
#ifdef WIN32
    unsigned int currentlControl = 0;
    _controlfp_s( &currentlControl, 0, 0 );   //read current FPE control word
    _controlfp_s(&currentlControl, ~(_EM_ZERODIVIDE | _EM_OVERFLOW | _EM_INVALID), _MCW_EM);   // /0 | +/-inf | NaN
    _controlfp_s(&currentlControl, 0, 0);     //read FPE control word after clearing the bits
    LOG_DEBUG_F("Signal Handler control word: %u \n", currentlControl);
#else
    feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);   // /0 | +/-inf | NaN
    LOG_DEBUG_F("Signal Handler control word: %d \n", fegetexcept() );
#endif

    // Install a signal handler
    if( std::signal(SIGFPE, FPE_SignalHandler) == SIG_ERR )
    {
        LOG_WARN("Could not install Floating Point Exception signal handler\n");
    }
}

void DisableFloatingPointSignalHandler()
{
#ifdef WIN32
    unsigned int currentlControl = 0;
    _controlfp_s(&currentlControl, 0, 0);   //read current control word
    _controlfp_s(&currentlControl, currentlControl | _EM_ZERODIVIDE | _EM_INVALID , _MCW_EM);   // disable FPE signals by setting bits
#else
    fedisableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);   // disable FPE signals
#endif
    std::signal(SIGFPE, SIG_DFL);   //set default signal handler for SIGFPE
}

int main(int argc, char* argv[])
{    
    // First thing to do when app launches
    Environment::setLogger(new SimpleLogger());
    if (argc < 2)
    {
        LOG_INFO_F("For full usage, run: %s --help\n", argv[0]);
        exit(1);
    }

#ifndef WIN32
    setStackSize();
#endif

    ProgDllVersion pv;

    std::stringstream output;
    output << "EMOD Disease Transmission Kernel " << pv.getVersion() << std::endl
           << "Built on " << pv.getBuildDate() 
           << " by " <<      pv.getBuilderName() 
           << " from " <<    pv.getSccsBranch() 
           << " checked in on " << pv.getSccsDate() << std::endl;
    
    output << "Supports Simulation_Type(s): " << getSupportedSimsString() << "." << std::endl;

#ifdef ENABLE_LOG_VALID
    output << "TestSugar Enabled" << std::endl;
#endif
    Kernel::InstructionSetInfo info;
    output << info.GetSupportString() << std::endl;

    output << std::endl;
    LOG_INFO_F( output.str().c_str() );
    EnvPtr->Log->Flush();
 
/* // for debugging all kernel allocations, use inner block around controller lifetime to ignore some environment and mpi stuff
#ifdef WIN32
#ifdef _DEBUG
    // In release builds, _CrtMemCheckpoint and _CrtMemDumpAllObjectsSince below are no-ops
    _CrtMemState initial_state;
#endif
    _CrtMemCheckpoint(&initial_state);

    //_crtBreakAlloc = 10242; // set this to alloc number to break on
#endif
*/
    int ret = MPIInitWrapper(argc, argv);
/*
#ifdef WIN32
    _CrtMemDumpAllObjectsSince(&initial_state);
#endif
*/
    return ret;
}

#ifndef WIN32

#define sprintf_s sprintf   

// hacks to give us some more stack space size since there isn't a compiler/linker option
#include <sys/resource.h>
void setStackSize()
{
    const rlim_t kStackSize = 32 * 1024 * 1024;   // min stack size = 32 MB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }
}

#endif


int MPIInitWrapper( int argc, char* argv[])
{
    try 
    {
        bool fSuccessful = false;
        IdmMpi::MessageInterface* p_mpi = IdmMpi::MessageInterface::Create( argc, argv );

        try 
        {
            fSuccessful = ControllerInitWrapper( argc, argv, p_mpi );
        }
        catch( std::exception& e) 
        {
            LOG_ERR_F("Error in ControllerInitWrapper: %s\n", e.what());
        }

        // If ControllerInitWrapper() returned false (error) or an exception was
        // caught, tear everything down, decisively.
        if (!fSuccessful)
        {
            if( (EnvPtr != nullptr) && (EnvPtr->Log != nullptr) )
            {
                EnvPtr->Log->Flush();
            }
            p_mpi->Abort(-1);
        }

        p_mpi->Finalize();
        delete p_mpi;
        p_mpi = nullptr;

        // Shouldn't get here unless ControllerInitWrapper() returned true (success).
        // MPI_Abort() implementations generally exit the process. If not, return a
        // useful value.
        return fSuccessful ? 0 : -1;
    }
    catch( std::exception &e )
    {
        LOG_ERR_F("Error in MPIInitWrapper: %s\n", e.what());
        return -1;
    }
}


bool ControllerInitWrapper( int argc, char *argv[], IdmMpi::MessageInterface* pMpi )
{
    using namespace std;

    /////////////////////////////////////////////////////////////
    // parse input arguments

    ProgramOptions po("Recognized options");
    try
    {
        po.AddOption(          "help",         "Show this help message." );
        po.AddOption(          "version",      "v",          "Get version info." );
        po.AddOption(          "get-schema",   "Request the kernel to write all its input definition schema json to the current working directory and exit." );
        po.AddOptionWithValue( "schema-path",  "stdout",     "Path to write schema(s) to instead of writing to stdout." );
        po.AddOptionWithValue( "config",       "C",          "config.json",         "Name of config.json file to use" );
        po.AddOptionWithValue( "input-path",   "I",          ".",                   "Relative or absolute path to location of model input files" );       
        po.AddOptionWithValue( "output-path",  "O",          "output",              "Relative or absolute path for output files" );
        po.AddOptionWithValue( "dll-path",     "D",          "",                    "Relative (to the executable) or absolute path for EMODule (dll/shared object) root directory" );
        po.AddOptionWithValue( "monitor_host", "none",       "IP of commissioning/monitoring host" );
        po.AddOptionWithValue( "monitor_port", 0,            "port of commissioning/monitoring host" );
        po.AddOptionWithValue( "sim_id",       "none",       "Unique id of this simulation, formerly sim_guid. Needed for self-identification to UDP host" );
        po.AddOption(          "progress",     "Send updates on the progress of the simulation to the HPC job scheduler." );
#ifdef ENABLE_PYTHON
        po.AddOptionWithValue( "python-script-path", "P", "", "Path to python scripts." );
#endif

        std::string errmsg = po.ParseCommandLine( argc, argv );
        if( !errmsg.empty() )
        {
            LOG_ERR_F("Error parsing command line: %s\n", errmsg.c_str() );
            return false;
        }

        if( po.CommandLineHas( "help" ) )
        {
            std::ostringstream oss;
            po.Print( oss );
            LOG_INFO(oss.str().c_str());
            return true;
        }

        if( po.CommandLineHas( "version" ) )
        {
            ProgDllVersion pv;
            std::ostringstream oss;
            oss << std::endl;
            oss << argv[0] << " version: " << pv.getVersion()  << std::endl;
            std::list<string> dllNames;
            std::list<string> dllVersions;
#ifdef WIN32
            if( po.CommandLineHas( "dll-path" ) )
            {
                DllLoader dllLoader;
                if( dllLoader.GetEModulesVersion( po.GetCommandLineValueString( "dll-path" ).c_str(), dllNames, dllVersions ) )
                {
                    std::list<string>::iterator iter1;
                    std::list<string>::iterator iter2;
                    for(iter1 = dllNames.begin(),iter2 = dllVersions.begin(); 
                        iter1 != dllNames.end();
                        ++iter1, ++iter2)
                    {
                        oss << *iter1 << " version: " << *iter2 << std::endl;
                    }
                }
            }
            else
            {
                LOG_DEBUG("The EModule root path is not given, so nothing to get version from.\n");
            }
            Kernel::InstructionSetInfo inst_set;
            oss << inst_set.GetSupportString() << std::endl;
#endif
            LOG_INFO(oss.str().c_str());
            return true;
        }

        //////////////////////////////////////////
        if( po.CommandLineHas( "config" ) )
        {
            LOG_INFO_F( "Using config file: %s\n", po.GetCommandLineValueString( "config" ).c_str() );
        }
        else
        {
            LOG_WARN("No config file specified.\n");
        }

        if( po.CommandLineHas( "input-path" ) )
        {
            LOG_INFO_F( "Using input path: %s\n", po.GetCommandLineValueString( "input-path" ).c_str() );
        }
        else
        {
            LOG_WARN("Input path not specified, assuming current directory.\n"); // in reality these never happen since default values are supplied by the program options
        }

        if( po.CommandLineHas( "output-path" ) )
        {
            LOG_INFO_F( "Using output path: %s\n", po.GetCommandLineValueString( "output-path" ).c_str() );
        }
        else
        {
            LOG_WARN("Output path not specified, using current directory.\n");
        }

        if( po.CommandLineHas( "dll-path" ) )
        {
            LOG_INFO_F( "Using dll path: %s\n", po.GetCommandLineValueString( "dll-path" ).c_str() );
        }
        else
        {
            LOG_WARN("Dll path not specified.\n");
        }
    }
    catch( std::exception& e) 
    {
        LOG_ERR_F("Error parsing command line: %s\n", e.what());
        return false;
    }
    catch(...) 
    {
        LOG_ERR("Exception of unknown type!\n");
        return false;
    }
    EnvPtr->Log->Flush();


    /////////////////////////////////////////////////////////////////////////////////////////
    // Run model

    // handle run modes

    bool status = false;
    ostringstream exceptionErrorReport;
    try
    {
        if( po.CommandLineHas( "progress" ) )
        {
            StatusReporter::updateScheduler = true;
        }

        bool is_getting_schema = po.CommandLineHas( "get-schema" );
        auto schema_path       = po.GetCommandLineValueString("schema-path");
        auto configFileName    = po.GetCommandLineValueString( "config" );

#ifdef ENABLE_PYTHON
        // Start python interpreter if python-script-path is not empty; default value for python-script-path option is empty string
        if ( pMpi->GetRank() == 0 )
        {
            Kernel::PythonSupport::SetupPython(po.GetCommandLineValueString( "python-script-path" ));
        }
#endif

        // Exceptions for misconfiguration of schema
        if( Kernel::PythonSupport::IsPythonInitialized() && is_getting_schema && schema_path == "stdout")
        {
            std::stringstream msg;
            msg << "--schema-path=stdout and --python-script-path defined:  Post processing only works on a file.  "
                << "Please define --schema-path as a filename.";
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Run python pre-process script; does nothing and returns configFileName if no python.
        configFileName = Kernel::PythonSupport::RunPyFunction( configFileName, Kernel::PythonSupport::SCRIPT_PRE_PROCESS );

        // Ensure all processes have the same value for configFileName
        int fname_tmp_size = configFileName.size() + 1;
        pMpi->BroadcastInteger(&fname_tmp_size, 1, 0);
        char* fname_tmp = static_cast<char*>(malloc(fname_tmp_size*sizeof(char)));
        std::strcpy(fname_tmp, configFileName.c_str());
        pMpi->BroadcastChar(fname_tmp, fname_tmp_size, 0);
        configFileName = fname_tmp;
        free(fname_tmp); fname_tmp = nullptr;

        // Exceptions for missing simulation configuration file
        if( !is_getting_schema && !FileSystem::FileExists( configFileName ) )
        {
            std::string msg;
            msg += "The configuration file '" + configFileName + "' could not be found.";
            msg += "Did you forget to define the configuration file on the command line with --config or -C?" ;
            LOG_ERR_F( msg.c_str() );
            return false;
        }

        // set up the environment
        EnvPtr->Log->Flush();
        LOG_INFO("Initializing environment...\n");
        bool env_ok = Environment::Initialize(
            pMpi,
            configFileName,
            po.GetCommandLineValueString( "input-path"  ),
            po.GetCommandLineValueString( "output-path" ),
            po.GetCommandLineValueString( "dll-path"    ),
            is_getting_schema
            );

        if (!env_ok)
        {
            LOG_ERR("Failed to initialize environment, exiting\n");
            return false;
        }
        else
        {
            LOG_INFO( "Environment initialized successfully.\n" );
        }

        if( is_getting_schema )
        {
            writeInputSchemas( po.GetCommandLineValueString( "schema-path" ).c_str() );
            return true;
        }

        // Process configuration file
        Kernel::ConfigParams config_obj;
        config_obj.Configure(EnvPtr->Config);

        // Update logger with params from configuration file
        EnvPtr->initLogger();

        // UDP-enabled StatusReporter needs host and sim unique id.
        if( po.GetCommandLineValueString( "monitor_host" ) != "none" )
        {
            LOG_INFO_F("Status monitor host: %s\n", po.GetCommandLineValueString( "monitor_host" ).c_str() );
            EnvPtr->getStatusReporter()->SetHost( po.GetCommandLineValueString( "monitor_host" ) );
        }

        if( po.GetCommandLineValueInt( "monitor_port" ) != 0 )
        {
            LOG_INFO_F("Status monitor port: %d \n",  po.GetCommandLineValueInt( "monitor_port" )  );
            EnvPtr->getStatusReporter()->SetMonitorPort(  po.GetCommandLineValueInt( "monitor_port" ) );
        }

        if( po.GetCommandLineValueString( "sim_id" ) != "none" )
        {
            LOG_INFO_F("Status monitor sim id: %s\n", po.GetCommandLineValueString( "sim_id" ).c_str() );
            EnvPtr->getStatusReporter()->SetHPCId( po.GetCommandLineValueString( "sim_id" ) );
        }


#ifdef _DEBUG
    //#define DEBUG_MEMORY_LEAKS // uncomment this to run an extra warm-up pass and enable dumping objects 
#endif 

#ifdef WIN32

    #ifdef DEBUG_MEMORY_LEAKS
        _CrtMemState initial_state;

        for (int run = 0; run < 2; run++)
        {
            if (run == 1)
            {
                _RPT0(_CRT_WARN,"Beginning check pass...\n");

                _CrtMemCheckpoint(&initial_state);
                int *intential_leak_marker = _new_ int[250]; // help us locate beginning of allocations during the check pass because DumpAllObjectsSince is not reliable and sometimes dumps everything

            //    _crtBreakAlloc = 106768; // break on this alloc number; get this from the object dump
            }
    #endif

#endif

        LOG_INFO( "Loaded Configuration...\n" ); 
        //LOG_INFO_F( "Name: %s\n", GET_CONFIGURABLE(SimulationConfig)->ConfigName.c_str() );  // can't get ConfigName because we haven't initialized SimulationConfig yet...
        LOG_INFO_F( "%d parameters found.\n", (EnvPtr->Config)->As<json::Object>().Size() );
        EnvPtr->Log->Flush();

        Kernel::SimulationConfig* SimConfig = Kernel::SimulationConfigFactory::CreateInstance(EnvPtr->Config);
        if (SimConfig)
        {
            Environment::setSimulationConfig(SimConfig);
        }
        else
        {
            throw Kernel::InitializationException(__FILE__, __LINE__, __FUNCTION__, "Failed to create SimulationConfig instance");
        }
        LOG_DEBUG("Initialize with config\n");

        // override controller selection if unit tests requested on command line
        LOG_INFO("Initializing Controller...\n");
        EnvPtr->Log->Flush();
        IController* controller = ControllerFactory::CreateController(EnvPtr->Config);

        if (controller)
        {
            SetFloatingPointSignalHandler();        // Enable floating point signal handler while controller is running
            status = controller->Execute();
            DisableFloatingPointSignalHandler();    // Prevent external programs from triggering fpe, e.g. Python dll

            if (status)
            {

                release_assert( EnvPtr );

                // Run python post-process script; does nothing if no python.
                Kernel::PythonSupport::RunPyFunction( EnvPtr->OutputPath, Kernel::PythonSupport::SCRIPT_POST_PROCESS );

                LOG_INFO( "Controller executed successfully.\n" );
            }
            else
            {
                LOG_INFO( "Controller execution failed, exiting.\n" );
            }
            delete controller;
        }

#ifdef WIN32

    #ifdef DEBUG_MEMORY_LEAKS
        if (run == 1)
            _CrtMemDumpAllObjectsSince(&initial_state);
    }
    #endif

#endif

    }
    catch( Kernel::GeneralConfigurationException &e )
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.GetMsg() << std::endl;

        if(Kernel::JsonConfigurable::_possibleNonflatConfig && 
            Kernel::JsonConfigurable::missing_parameters_set.size() != 0)
        {
            exceptionErrorReport << "Presence of \"Default_Config_Path\" detected in config-file may indicate a problem; make sure you're using a flattened config." << std::endl;
        }
        exceptionErrorReport << std::endl << e.GetStackTrace() << std::endl;
    }
    catch( Kernel::DetailedException &e )
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.GetMsg() << std::endl << std::endl;
        exceptionErrorReport << e.GetStackTrace() << std::endl;
    }
    catch (std::bad_alloc &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.what() << std::endl;
        exceptionErrorReport << "Memory allocation failure: try reducing the memory footprint of your simulation or using more cores.\n"; 
    }
    catch (json::Exception &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << "Caught json::Exception: " << e.what() << std::endl;

        if(Kernel::JsonConfigurable::_possibleNonflatConfig)
        {
            exceptionErrorReport << "Presence of \"Default_Config_Path\" detected in config-file may indicate a problem; make sure you're using a flattened config." << std::endl;
        }
    }
    catch (std::runtime_error &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << "Caught std::runtime_error: " << e.what() << std::endl;
    }
    catch (std::exception &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.what() << std::endl;
    } 

    // no finally in c++
    if( exceptionErrorReport.str() != "" )
    {
        LOG_ERR( exceptionErrorReport.str().c_str() );
#ifdef WIN32
        std::string msg = exceptionErrorReport.str();
        std::wstring w_msg ;
        w_msg.assign(msg.begin(), msg.end());
        OutputDebugStringW( w_msg.c_str() );
#endif
    }

    if( EnvPtr != nullptr )
    {
        EnvPtr->Log->Flush();
    }

    Environment::Finalize();
    return status;
}
