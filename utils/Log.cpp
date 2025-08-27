
#include "stdafx.h"
#include <stdarg.h>
#include <iostream>
#include "Log.h"
#include "Environment.h"
#include "Exceptions.h"
#include <map>

using namespace std;
#pragma warning(disable : 4996)

SETUP_LOGGING("Log")

// standard tags
const char * LogLevel::Valid =      "V";
const char * LogLevel::Debug =      "D";
const char * LogLevel::Info =       "I";
const char * LogLevel::Warning =    "W";
const char * LogLevel::Error =      "E";
typedef std::map< Logger::tLevel, std::string > tLogLevel;

tLogLevel init_loglevel_map()
{
    tLogLevel returnThis;
    returnThis[Logger::VALIDATION] = LogLevel::Valid;
    returnThis[Logger::DEBUG     ] = LogLevel::Debug;
    returnThis[Logger::INFO      ] = LogLevel::Info;
    returnThis[Logger::WARNING   ] = LogLevel::Warning;
    returnThis[Logger::_ERROR    ] = LogLevel::Error;
    return returnThis;
}

tLogLevel init_full_string_loglevel_map()
{
    tLogLevel returnThis;
    returnThis[Logger::VALIDATION] = "VALID";
    returnThis[Logger::DEBUG     ] = "DEBUG";
    returnThis[Logger::INFO      ] = "INFO" ;
    returnThis[Logger::WARNING   ] = "WARNING";
    returnThis[Logger::_ERROR    ] = "ERROR";
    return returnThis;
}

static std::map< Logger::tLevel, std::string > logLevelStrMap     = init_loglevel_map();
static std::map< Logger::tLevel, std::string > logLevelFullStrMap = init_full_string_loglevel_map();


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

void
SimpleLogger::Init(
    const json::QuickInterpreter * configJson
)
{
    // Only throttle if config.json says to.
    if( (*configJson)["parameters"].Exist( "Enable_Log_Throttling" ) && (*configJson)["parameters"]["Enable_Log_Throttling"].As<json::Number>() == 1 )
    {
        _throttle = true;
    }

    if( (*configJson)["parameters"].Exist( "Enable_Continuous_Log_Flushing" ) && (*configJson)["parameters"]["Enable_Continuous_Log_Flushing"].As<json::Number>() == 1 )
    {
        _flush_all = true;
    }

    _logLevelMap["Eradication"] = Logger::INFO; // Default Eradication to INFO, config.json can override.

    // Iterate through config.json looking for any keys with logLevel prefix.
    for( json::Object::Members::const_iterator it = (*configJson)["parameters"].As<json::Object>().Begin();
                                               it != (*configJson)["parameters"].As<json::Object>().End();
                                             ++it )
    {
        const std::string& key = it->name;
        if( key.substr(0,9) == "logLevel_" ) // 9 is the length of "logLevel_"
        {
            const std::string& moduleName = key.substr(9);
            const json::QuickInterpreter elemConfigJson = json::QuickInterpreter( it->element );
            const std::string& value = elemConfigJson.As<json::String>();
            Logger::tLevel moduleLogLevel = Logger::_ERROR;
            // Brute force this (instead of map) since only used here.
            if( value == logLevelFullStrMap[Logger::_ERROR] )
            {
                moduleLogLevel = Logger::_ERROR;
            }
            else if( value == logLevelFullStrMap[Logger::WARNING] )
            {
                moduleLogLevel = Logger::WARNING;
            }
            else if( value == logLevelFullStrMap[Logger::INFO] )
            {
                moduleLogLevel = Logger::INFO;
            }
            else if( value == logLevelFullStrMap[Logger::DEBUG] )
            {
                moduleLogLevel = Logger::DEBUG;
            }
            else if( value == logLevelFullStrMap[Logger::VALIDATION] )
            {
                moduleLogLevel = Logger::VALIDATION;
            }
            else
            {
                std::string msg = "Unknown log level ("+value+") for " + key + ".  Acceptable values are: " ;
                for( tLogLevel::iterator jt = logLevelFullStrMap.begin() ; jt != logLevelFullStrMap.end() ; ++jt )
                {
                    msg += jt->second + ", " ;
                }
                msg = msg.substr(0, msg.size()-2) ; //remove trailing comma and space
                throw Kernel::InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
            }

            if( moduleName == "default" )
            {
                _systemLogLevel = moduleLogLevel;
            }
            else
            {
                // Copy module-names to char*'s, otherwise searching in the map later
                // results in poor performance because of lots of string::assign()...
                char * tmp = new char[moduleName.size()+1];
                strcpy(tmp, moduleName.c_str());
                _logLevelMap[tmp/*moduleName*/] = moduleLogLevel;
            }
        }
    }

    _rank = EnvPtr->MPI.Rank;

    std::cout << "Log-levels:" << std::endl ;
    std::cout << "\tDefault -> " <<  logLevelFullStrMap[_systemLogLevel] << std::endl ;
    for (auto& loglevelpair : _logLevelMap)
    {
        std::cout << "\t" << loglevelpair.first << " -> " << logLevelFullStrMap[ loglevelpair.second ] << std::endl ;
    }

    if( (*configJson)["parameters"].Exist( "Warnings_Are_Fatal" ) && (*configJson)["parameters"]["Warnings_Are_Fatal"].As<json::Number>() == 1 )
    {
        _warnings_are_fatal = true;
    }

    _initialized = true;
}

static map< std::string, std::string > _logShortHistory; // module name to log line map

bool SimpleLogger::CheckLogLevel( Logger::tLevel log_level, const char* module )
{
    // We use standard 0-based priority sequence (0 is the highest level priority)
    // Check if module is in our map
    module_loglevel_map_t::const_iterator findIt;

    if( _logLevelMap.size() > 0 && 
        (findIt = _logLevelMap.find( module )) != _logLevelMap.end() )
    {
        // We have this module in our map, check the priority.
        Logger::tLevel moduleLogLevel = findIt->second;
        if( log_level > moduleLogLevel )
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

// Either we have a log_level for this module in the map (intialized at init time), or we use the system log level.
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

        fprintf(stdout, "%02d:%02d:%02d [%d] [%s] [%s] ", static_cast<int>(tInfo.hours), static_cast<int>(tInfo.mins), static_cast<int>(tInfo.secs), _rank, logLevelStrMap[log_level].c_str(), module);
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! GH-2693 - Don't write warning messages to StdErr until the issue with how
        // !!! MPI causes the sim to slow down when writing to StdErr is resolved.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (log_level == Logger::_ERROR /*|| log_level == Logger::WARNING*/)
        {
            // Yes, this line is mostly copy-pasted from above. Yes, I'm comfortable with that. :)
            fprintf(stderr, "%02d:%02d:%02d [%d] [%s] [%s] ", static_cast<int>(tInfo.hours), static_cast<int>(tInfo.mins), static_cast<int>(tInfo.secs), _rank, logLevelStrMap[log_level].c_str(), module);
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
        Flush();

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
