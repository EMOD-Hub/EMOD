
#pragma once

#include <string>
#include <fstream>
#include <map>

#include "CajunIncludes.h"

// common environment information
// This class is intended to be passed around through const reference, so most members will be read only 
// by default. Output streams and the like are mutable because their logical contents won't change.
// I think this falls into the acceptable category: http://www.highprogrammer.com/alan/rants/mutable.html

class SimpleLogger;
class Configuration;
class StatusReporter;

namespace IdmMpi
{
    class MessageInterface;
}


class Environment
{
public:
    struct _MPI
    {
        int NumTasks;
        int Rank;
        IdmMpi::MessageInterface* p_idm_mpi;
    } MPI;

    SimpleLogger *Log;
    Configuration *Config;
    void* SimConfig;
    void* pIPFactory;
    void* pNPFactory;
    void* pRngFactory;
    std::vector<void*> event_trigger_factories;
    StatusReporter * Status_Reporter;
    
    std::list< std::string > InputPaths;
    std::string OutputPath;
    std::string StatePath;
    std::string DllPath;

    // Sets up the environment for this process. Returns false if something went wrong
    static bool Initialize(
        IdmMpi::MessageInterface* pMpi,
        std::string configFileName,
        std::string inputPath,
        std::string outputPath,
// 2.5        std::string statePath,
        std::string dllPath,
        bool get_schema
        );

    // Cleans up open files, handles, memory, etc held by the environment
    static void Finalize();

    static const Configuration* getConfiguration();
    static Configuration* CopyFromElement( const json::Element& rElement, const std::string& rDataLocation = "Unknown" );
    static Configuration* LoadConfigurationFile( const std::string& rFileName );

    static Environment* getInstance();
    static void setInstance( Environment * env );
    static void setLogger( SimpleLogger* log );
    static void initLogger();
    static void setSimulationConfig( void* SimConfig );
    static const void* getSimulationConfig();
    static StatusReporter * getStatusReporter();
    static void setIPFactory( void* pipf );
    static void* getIPFactory();
    static void setNPFactory( void* pnpf );
    static void* getNPFactory();

    static const void* getEventTriggerFactory( int event_type );
    static void setEventTriggerFactory( int event_type, void* pFactory );

    static const void* getRandomNumberGeneratorFactory();
    static void setRandomNumberGeneratorFactory( void* pFactory );

    // Return path to specified file according to the following order of preference:
    // (1) in current working directory, (2) in specified InputPath
    static std::string FindFileOnPath( const std::string& rFilename );


    virtual ~Environment();

private:
    Environment();

    static Environment * localEnv;
};
