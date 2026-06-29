
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

    struct AgentParams
    {
    public:
        AgentParams();
    };

    struct ClimateParams
    {
    public:
        ClimateParams();
    };

    struct LoggingParams
    {
    public:
        LoggingParams();

        bool enable_continuous_log_flushing;
        bool enable_log_throttling;
        bool enable_warnings_are_fatal;

        std::map<std::string, std::string> module_name_to_level_map;
    };

    struct MigrationParams
    {
    public:
        MigrationParams();
    };

    struct NodeParams
    {
    public:
        NodeParams();
    };

    struct SimParams
    {
    public:
        SimParams();
    };

    // *****************************************************************************

    class AgentConfig : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER(AgentConfig)

    public:
        virtual bool Configure(const Configuration* config) override;

        static const AgentParams&     GetAgentParams();

    protected:
        static       AgentParams      agent_params;
    };

    class ClimateConfig : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER(ClimateConfig)

    public:
        virtual bool Configure(const Configuration* config) override;

        static const ClimateParams&     GetClimateParams();

    protected:
        static       ClimateParams      climate_params;
    };

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

    class MigrationConfig : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER(MigrationConfig)

    public:
        virtual bool Configure(const Configuration* config) override;

        static const MigrationParams&   GetMigrationParams();

    protected:
        static       MigrationParams    migration_params;
    };

    class NodeConfig : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER(NodeConfig)

    public:
        virtual bool Configure(const Configuration* config) override;

        static const NodeParams&        GetNodeParams();

    protected:
        static       NodeParams         node_params;
    };

    class SimConfig : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER(SimConfig)

    public:
        virtual bool Configure(const Configuration* config) override;

        static const SimParams&   GetSimParams();

    protected:
        static       SimParams    sim_params;
    };
}
