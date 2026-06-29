
#include <cmath>

#include "stdafx.h"
#include "ConfigParams.h"


namespace Kernel
{
    // Static param structures
    AgentParams      AgentConfig::agent_params;
    ClimateParams    ClimateConfig::climate_params;
    LoggingParams    LoggingConfig::logging_params;
    MigrationParams  MigrationConfig::migration_params;
    NodeParams       NodeConfig::node_params;
    SimParams        SimConfig::sim_params;

    // ConfigParams Methods
    bool ConfigParams::Configure(Configuration* config)
    {
        // Configs with static param structures
        AgentConfig        agent_config_obj;
        ClimateConfig      climate_config_obj;
        LoggingConfig      logging_config_obj;
        MigrationConfig    migration_config_obj;
        NodeConfig         node_config_obj;
        SimConfig          sim_config_obj;

        // Process configuration
        bool bRet = true;

        bRet &= agent_config_obj.Configure(config);
        bRet &= climate_config_obj.Configure(config);
        bRet &= logging_config_obj.Configure(config);
        bRet &= migration_config_obj.Configure(config);
        bRet &= node_config_obj.Configure(config);
        bRet &= sim_config_obj.Configure(config);

        return bRet;
    }

    // *****************************************************************************

    AgentParams::AgentParams()
    {}

    ClimateParams::ClimateParams()
    {}

    LoggingParams::LoggingParams()
        : enable_continuous_log_flushing(false)
        , enable_log_throttling(false)
        , enable_warnings_are_fatal(false)
        , module_name_to_level_map()
    {}

    MigrationParams::MigrationParams()
    {}

    NodeParams::NodeParams()
    {}

    SimParams::SimParams()
    {}

    // *****************************************************************************

    // AgentConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(AgentConfig,AgentConfig)
    BEGIN_QUERY_INTERFACE_BODY(AgentConfig)
    END_QUERY_INTERFACE_BODY(AgentConfig)

    bool AgentConfig::Configure(const Configuration* config)
    {
        // Process configuration
        bool bRet = JsonConfigurable::Configure(config);

        return bRet;
    }

    const AgentParams& AgentConfig::GetAgentParams()
    {
        return agent_params;
    }

    // *****************************************************************************

    // ClimateConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(ClimateConfig,ClimateConfig)
    BEGIN_QUERY_INTERFACE_BODY(ClimateConfig)
    END_QUERY_INTERFACE_BODY(ClimateConfig)

    bool ClimateConfig::Configure(const Configuration* config)
    {
        // Process configuration
        bool bRet = JsonConfigurable::Configure(config);

        return bRet;
    }

    const ClimateParams& ClimateConfig::GetClimateParams()
    {
        return climate_params;
    }

    // *****************************************************************************

    // LoggingConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(LoggingConfig,LoggingConfig)
    BEGIN_QUERY_INTERFACE_BODY(LoggingConfig)
    END_QUERY_INTERFACE_BODY(LoggingConfig)

    bool LoggingConfig::Configure(const Configuration* config)
    {
        // Logging parameters have historically not been required in the config file. In emod-api (2.0.28), as part of the
        // finalize method for the config file, all logLevel_<module_name> parameters are removed if they are equal to
        // the value of logLevel_default. This process simplifies the config file, but requires that absent parameters for
        // logging do not cause an exception.
        bool use_defaults_prior_setting = JsonConfigurable::_useDefaults;
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
        logging_params.module_name_to_level_map[default_log_name] = log_default_val;

        for (auto& mod_name : SimpleLogger::GetModuleNames())
        {
            log_name_param = LOG_NAME_PREFIX+mod_name;
            initConfigTypeMap(log_name_param.c_str(), &log_config_str, logLevel_MODULE_DESC_TEXT, log_default_val);
            bRet &= JsonConfigurable::Configure(config);
            logging_params.module_name_to_level_map[mod_name] = static_cast<std::string>(log_config_str);
        }

        JsonConfigurable::_useDefaults = use_defaults_prior_setting;
        return bRet;
    }

    const LoggingParams& LoggingConfig::GetLoggingParams()
    {
        return logging_params;
    }

    // *****************************************************************************

    // MigrationConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(MigrationConfig,MigrationConfig)
    BEGIN_QUERY_INTERFACE_BODY(MigrationConfig)
    END_QUERY_INTERFACE_BODY(MigrationConfig)

    bool MigrationConfig::Configure(const Configuration* config)
    {
        // Process configuration
        bool bRet = JsonConfigurable::Configure(config);

        return bRet;
    }

    const MigrationParams& MigrationConfig::GetMigrationParams()
    {
        return migration_params;
    }

    // *****************************************************************************

    // NodeConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeConfig,NodeConfig)
    BEGIN_QUERY_INTERFACE_BODY(NodeConfig)
    END_QUERY_INTERFACE_BODY(NodeConfig)

    bool NodeConfig::Configure(const Configuration* config)
    {
        // Process configuration
        bool bRet = JsonConfigurable::Configure(config);

        return bRet;
    }

    const NodeParams& NodeConfig::GetNodeParams()
    {
        return node_params;
    }

    // *****************************************************************************

    // SimConfig Methods
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimConfig,SimConfig)
    BEGIN_QUERY_INTERFACE_BODY(SimConfig)
    END_QUERY_INTERFACE_BODY(SimConfig)

    bool SimConfig::Configure(const Configuration* config)
    {
        // Process configuration
        bool bRet = JsonConfigurable::Configure(config);

        return bRet;
    }

    const SimParams& SimConfig::GetSimParams()
    {
        return sim_params;
    }

    // *****************************************************************************
}
