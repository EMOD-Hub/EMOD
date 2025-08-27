
#pragma once
#include "stdafx.h"

#include <map>
#include <cstring>

#include "Sugar.h"

namespace Logger
{
    typedef enum {
        CRITICAL = 0,
        _ERROR, // ERROR breaks on msvc!
        WARNING,
        INFO,
        DEBUG,
        VALIDATION
    } tLevel;
};

#define NUM_LOG_LEVELS (6)

// EVIL MACROS COMING UP! Idea here is that folks can log with 1 parameter (the string).

#define SETUP_LOGGING(moduleName)\
static const char * _module = moduleName;\
static bool* _log_level_enabled_array = nullptr;


#define LOG_LVL(lvl, x)          do { if( SimpleLogger::IsLoggingEnabled( Logger::lvl, _module, _log_level_enabled_array ) )  EnvPtr->Log->Log( Logger::lvl, _module, x); } while(0)
#define LOG_LVL_F(lvl, x, ...)   do { if( SimpleLogger::IsLoggingEnabled( Logger::lvl, _module, _log_level_enabled_array ) )  EnvPtr->Log->Log(Logger::lvl, _module, x, ##__VA_ARGS__); } while(0)

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

namespace json
{
    class QuickInterpreter;
}

struct LogTimeInfo
{
    time_t hours;
    time_t mins;
    time_t secs;
};

struct cmp_str
{
   bool operator()(char const *a, char const *b) const { return std::strcmp(a, b) < 0; }
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
    void Init( const json::QuickInterpreter * configJson );

    bool CheckLogLevel( Logger::tLevel log_level, const char* module );
    virtual void Log( Logger::tLevel log_level, const char* module, const char* msg, ...);
    virtual void Flush();

    void GetLogInfo(LogTimeInfo &tInfo );

protected:
    typedef std::map< const char*, Logger::tLevel, cmp_str > module_loglevel_map_t;
    module_loglevel_map_t _logLevelMap;

    Logger::tLevel _systemLogLevel;

    bool _throttle;
    bool _initialized;
    bool _flush_all;
    bool _warnings_are_fatal;

    time_t _initTime;
    int _rank;
};

struct LogLevel // for holding tags we will use in the log itself to indicate the level
{
    static const char * Valid;
    static const char * Debug;
    static const char * Info;
    static const char * Warning;
    static const char * Error;
};
