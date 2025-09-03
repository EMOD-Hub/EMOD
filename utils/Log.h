
#pragma once
#include "stdafx.h"

#include <map>
#include <cstring>

#include "Sugar.h"

namespace Logger
{
    typedef enum {
        _ERROR = 0, // ERROR breaks on msvc!
        WARNING,
        INFO,
        DEBUG,
        VALIDATION
    } tLevel;
};

#define NUM_LOG_LEVELS    (5)
#define LOG_NAME_PREFIX   ("logLevel_")
#define DEFAULT_LOG_NAME  ("default")

// EVIL MACROS COMING UP! Idea here is that folks can log with 1 parameter (the string).

#define SETUP_LOGGING(moduleName)                                   \
static const char*  _module                   = moduleName;         \
static       bool*  _log_level_enabled_array  = nullptr;            \
static DummyLogger  _tmp_log_obj(moduleName);

#define LOG_LVL(lvl, x)          { if( SimpleLogger::IsLoggingEnabled( Logger::lvl, _module, _log_level_enabled_array ) )  EnvPtr->Log->Log(Logger::lvl, _module, x               ); }
#define LOG_LVL_F(lvl, x, ...)   { if( SimpleLogger::IsLoggingEnabled( Logger::lvl, _module, _log_level_enabled_array ) )  EnvPtr->Log->Log(Logger::lvl, _module, x, ##__VA_ARGS__); }

#define LOG_LEVEL(lvl)          ((EnvPtr != nullptr) ? EnvPtr->Log->CheckLogLevel(Logger::lvl, _module) : false)

#define LOG_ERR(x)            LOG_LVL( _ERROR, x )
#define LOG_ERR_F(x, ...)     LOG_LVL_F( _ERROR, x, ##__VA_ARGS__ )
#define LOG_WARN(x)           LOG_LVL( WARNING, x )
#define LOG_WARN_F(x, ...)    LOG_LVL_F( WARNING, x, ##__VA_ARGS__ )
#define LOG_INFO(x)           LOG_LVL( INFO, x )
#define LOG_INFO_F(x, ...)    LOG_LVL_F( INFO, x, ##__VA_ARGS__ )

// NOTE: LOG_DEBUG is disabled with LOG_VALID for performance reasons - 2-4%.
#if defined(_DEBUG) || defined(ENABLE_LOG_VALID)
    #define LOG_DEBUG(x)          LOG_LVL( DEBUG, x )
    #define LOG_DEBUG_F(x, ...)   LOG_LVL_F( DEBUG, x, ##__VA_ARGS__ )
    #define LOG_VALID(x)          LOG_LVL( VALIDATION, x )
    #define LOG_VALID_F(x, ...)   LOG_LVL_F( VALIDATION, x, ##__VA_ARGS__ )
#else
    #define LOG_DEBUG(X)
    #define LOG_DEBUG_F(X, ...)
    #define LOG_VALID(X)
    #define LOG_VALID_F(X, ...)
#endif // _DEBUG


struct LogTimeInfo
{
    time_t hours;
    time_t mins;
    time_t secs;
};

// DummyLogger was created so the SETUP_LOGGING macro records a list of module names and the
// module names can be included in the schema. The static vector of module names in SimpleLogger
// is a pointer to avoid initialization order conflicts. Customs reporters are loaded after
// simulation creation, so don't get config params. Should add to custom reports file if logging
// control is needed there. Reports as interventions would also resolve the problem.
class DummyLogger
{
public:
    DummyLogger( std::string module_name );
};


class SimpleLogger
{
public:
    static inline bool IsLoggingEnabled( Logger::tLevel log_level, const char* module, bool*& logLevelEnabledArray )
    {
        if( logLevelEnabledArray == nullptr )
        {
            if( EnvPtr == nullptr ) return false;
            if( EnvPtr->Log == nullptr ) return false;

            logLevelEnabledArray = (bool*)malloc( sizeof( bool )*NUM_LOG_LEVELS );

            for( int i = 0 ; i < NUM_LOG_LEVELS ; ++i )
            {
                Logger::tLevel lvl = Logger::tLevel( i );
                logLevelEnabledArray[ i ] = EnvPtr->Log->CheckLogLevel( lvl, module );
            }
        }

        return logLevelEnabledArray[ log_level ];
    }

    SimpleLogger();
    SimpleLogger( Logger::tLevel syslevel );

    void Init();
    bool CheckLogLevel( Logger::tLevel log_level, const char* module );
    virtual void Log( Logger::tLevel log_level, const char* module, const char* msg, ...);
    virtual void Flush();

    void GetLogInfo(LogTimeInfo &tInfo );

    static const std::vector<std::string>& GetModuleNames();
    static void                            AddModuleName(std::string);

protected:
    static std::vector<std::string>* module_names;

    std::map< std::string, Logger::tLevel > _logLevelMap;

    Logger::tLevel _systemLogLevel;

    bool _throttle;
    bool _initialized;
    bool _flush_all;
    bool _warnings_are_fatal;

    time_t _initTime;
    int    _rank;
};
