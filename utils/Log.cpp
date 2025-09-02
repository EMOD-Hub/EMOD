
#include "stdafx.h"
#include <stdarg.h>
#include <iostream>
#include <ctime>
#include "ConfigParams.h"
#include "Log.h"
#include "Environment.h"
#include "Exceptions.h"
#include <map>

using namespace std;

SETUP_LOGGING("Log")

static std::map< std::string, Logger::tLevel > logLevelLookup   = {{"VALID",   Logger::VALIDATION},
                                                                   {"DEBUG",   Logger::DEBUG     },
                                                                   {"INFO",    Logger::INFO      },
                                                                   {"WARNING", Logger::WARNING   },
                                                                   {"ERROR",   Logger::_ERROR    }};
static std::map< std::string,    std::string > _logShortHistory;

DummyLogger::DummyLogger( std::string module_name )
{
    SimpleLogger::AddModuleName(module_name);
}

SimpleLogger::SimpleLogger()
    : _systemLogLevel(Logger::INFO)
    , _throttle(false)
    , _initialized(false)
    , _flush_all(false)
    , _warnings_are_fatal(false)
    , _rank(0)
{
    _initTime = time(nullptr);
}

SimpleLogger::SimpleLogger( Logger::tLevel syslevel )
    : _systemLogLevel(syslevel)
    , _throttle(false)
    , _initialized(false)
    , _flush_all(false)
    , _warnings_are_fatal(false)
    , _rank(0)
{
    _initTime = time(nullptr);
}

std::vector<std::string>* SimpleLogger::module_names;

const std::vector<std::string>& SimpleLogger::GetModuleNames()
{
    release_assert(module_names);
    return *module_names;
}

void SimpleLogger::AddModuleName( std::string module_name )
{
    if(!module_names)
    {
        module_names = new std::vector<std::string>();
    }
    module_names->push_back(module_name);
}

void SimpleLogger::Init()
{
    const Kernel::LoggingParams lp = Kernel::LoggingConfig::GetLoggingParams();

    _rank                    = EnvPtr->MPI.Rank;
    _throttle                = lp.enable_log_throttling;
    _flush_all               = lp.enable_continuous_log_flushing;
    _warnings_are_fatal      = lp.enable_warnings_are_fatal;
    _systemLogLevel          = logLevelLookup[lp.module_name_to_level_map.at(DEFAULT_LOG_NAME)];

    for(auto log_module : lp.module_name_to_level_map)
    {
        if(logLevelLookup[log_module.second] != _systemLogLevel)
        {
            _logLevelMap[log_module.first] = logLevelLookup[log_module.second];
        }
    }

    std::cout << "Log-levels:" << std::endl;
    std::cout << "    Default -> " << lp.module_name_to_level_map.at(DEFAULT_LOG_NAME) << std::endl;
    for(auto loglevelpair : _logLevelMap)
    {
        std::cout << "    " << loglevelpair.first << " -> " 
                  << lp.module_name_to_level_map.at(loglevelpair.first) << std::endl;
    }

    _initialized = true;
}

bool SimpleLogger::CheckLogLevel( Logger::tLevel log_level, const char* module )
{
    // We use standard 0-based priority sequence (0 is the highest level priority)
    // Check if module is in our map
    if( _logLevelMap.count(module) )
    {
        // We have this module in our map, check the priority.
        if( log_level > _logLevelMap[module] )
        {
            return false;
        }
    }
    else if( log_level > _systemLogLevel )
    {
        return false;
    }

    return true;
}

// A log_level for this module is in the map (intialized at init time), or use the system log level.
// Either way, if the requested level is >= the setting (in terms of criticality), log it.
void SimpleLogger::Log( Logger::tLevel log_level, const char* module, const char* msg, ...)
{
    if(_throttle)
    {
        if(_logShortHistory.find( module ) != _logShortHistory.end() && _logShortHistory[ module ] == msg ) // FIX THIS
        {
            // Throttling because we just saw this message for this module && throttling is on.
            return;
        }
        _logShortHistory[ module ] = msg;
    }

    if(_initialized)
    {
        LogTimeInfo tInfo;
        GetLogInfo(tInfo);

        int t_hrs = static_cast<int>(tInfo.hours);
        int t_min = static_cast<int>(tInfo.mins);
        int t_sec = static_cast<int>(tInfo.secs);

        const char* log_char;
        switch(log_level)
        {
            case Logger::VALIDATION:
                log_char = "V";
                break;
            case Logger::DEBUG:
                log_char = "D";
                break;
            case Logger::INFO:
                log_char = "I";
                break;
            case Logger::WARNING:
                log_char = "W";
                break;
            case Logger::_ERROR:
                log_char = "E";
                break;
            default:
                release_assert(false);
                break;
        }

        fprintf(stdout, "%02d:%02d:%02d [%d] [%s] [%s] ", t_hrs, t_min, t_sec, _rank, log_char, module);

        if(log_level == Logger::_ERROR)
        {
            fprintf(stderr, "%02d:%02d:%02d [%d] [%s] [%s] ", t_hrs, t_min, t_sec, _rank, log_char, module);
            va_list args;
            va_start(args, msg);
            vfprintf(stderr, msg, args);
            va_end(args);
        }
    }

    va_list args;
    va_start(args,msg);
    vfprintf(stdout, msg, args);
    va_end(args);

    if(_flush_all)
    {
        Flush();
    }

    if( log_level == Logger::WARNING && _warnings_are_fatal )
    {
        throw Kernel::WarningException( __FILE__, __LINE__, __FUNCTION__ );
    }
}

void SimpleLogger::Flush()
{
    std::cout.flush();
    std::cerr.flush();
}

void SimpleLogger::GetLogInfo( LogTimeInfo &tInfo )
{
    // Need timestamp
    time_t now = time(nullptr);
    time_t sim_time = now - _initTime;
    tInfo.hours = sim_time/3600;
    tInfo.mins = (sim_time - (tInfo.hours*3600))/60;
    tInfo.secs = (sim_time - (tInfo.hours*3600)) - tInfo.mins*60;
}
