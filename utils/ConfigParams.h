
#pragma once

#include "Common.h"
#include "Configure.h"


namespace Kernel
{
    // ConfigParams is a convenience wrapper for initializing other configuration parameter classes.
    // Each of the other parameter classes has a static struct with themed-parameters; the ConfigParams
    // class itself has no static members / parameters and doesn't inherit from JsonConfigurable.
    class ConfigParams
    {
    public:
        bool Configure(Configuration* config);
    };

    // *****************************************************************************

    struct LoggingParams
    {
    public:
        LoggingParams();

        bool enable_continuous_log_flushing;
        bool enable_log_throttling;
        bool enable_warnings_are_fatal;

        std::map<std::string, std::string> module_name_to_level_map;
    };

    // *****************************************************************************

    class LoggingConfig : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER(LoggingConfig)

    public:
        virtual bool Configure(const Configuration* config) override;

        static const LoggingParams&   GetLoggingParams();

    protected:
        static       LoggingParams    logging_params;
    };

}
