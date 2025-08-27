
#include <cmath>

#include "stdafx.h"
#include "ConfigParams.h"


namespace Kernel
{
    // Static param structures
    LoggingParams    LoggingConfig::logging_params;


    // ConfigParams Methods
    bool ConfigParams::Configure(Configuration* config)
    {
        // Configs with static param structures
        LoggingConfig      logging_config_obj;

        // Process configuration
        bool bRet = true;

        bRet &= logging_config_obj.Configure(config);

        return bRet;
    }

    // *****************************************************************************

    LoggingParams::LoggingParams()
        : enable_continuous_log_flushing(false)
        , enable_log_throttling(false)
        , enable_warnings_are_fatal(false)
        , log_levels()
    {}

    // *****************************************************************************

    // LoggingConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(LoggingConfig,LoggingConfig)
    BEGIN_QUERY_INTERFACE_BODY(LoggingConfig)
    END_QUERY_INTERFACE_BODY(LoggingConfig)

    bool LoggingConfig::Configure(const Configuration* config)
    {
        // Enable defaults
        JsonConfigurable::_useDefaults = true;

        // Logging parameters
        initConfigTypeMap("Enable_Continuous_Log_Flushing",  &logging_params.enable_continuous_log_flushing,  Enable_Continuous_Log_Flushing_DESC_TEXT,  false);
        initConfigTypeMap("Enable_Log_Throttling",           &logging_params.enable_log_throttling,           Enable_Log_Throttling_DESC_TEXT,           false);
        initConfigTypeMap("Enable_Warnings_Are_Fatal",       &logging_params.enable_warnings_are_fatal,       Enable_Warnings_Are_Fatal_DESC_TEXT,       false);

        // Process configuration
        bool bRet = JsonConfigurable::Configure(config);

        // Logging levels; constrained string because "ERROR" cannot be enumerated
        jsonConfigurable::ConstrainedString  log_config_str("INFO");
        jsonConfigurable::tStringSet         log_opt_set{"ERROR","WARNING","INFO","DEBUG","VALID"};
        std::string                          log_default_val("INFO");

        log_config_str.constraints           = "ERROR,WARNING,INFO,DEBUG,VALID";
        log_config_str.constraint_param      = &log_opt_set;

        // Configure default logging
        std::string default_log_name(DEFAULT_LOG_NAME);
        std::string log_name_param(LOG_NAME_PREFIX+default_log_name);
        initConfigTypeMap(log_name_param.c_str(), &log_config_str, logLevel_default_DESC_TEXT, log_default_val);
        bRet &= JsonConfigurable::Configure(config);
        log_default_val = static_cast<std::string>(log_config_str);
        logging_params.log_levels[default_log_name] = log_default_val;

        for (auto& mod_name : SimpleLogger::GetModuleNames())
        {
            log_name_param = LOG_NAME_PREFIX+mod_name;
            initConfigTypeMap(log_name_param.c_str(), &log_config_str, logLevel_MODULE_DESC_TEXT, log_default_val);
            bRet &= JsonConfigurable::Configure(config);
            logging_params.log_levels[mod_name] = static_cast<std::string>(log_config_str);
        }

        JsonConfigurable::_useDefaults = false;
        return bRet;
    }

    const LoggingParams* LoggingConfig::GetLoggingParams()
    {
        return &logging_params;
    }

}
