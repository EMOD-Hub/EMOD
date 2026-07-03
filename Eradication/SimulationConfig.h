
#pragma once

#include <string>
#include <list>
#include <map>
#include <functional>

#include "IdmApi.h"
#include "Configuration.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "SimulationEnums.h"

// This macro, GET_CONFIGURABLE(Category), is the convenient and fast way to access the SimulationConfig parameters
// within monolithic executable Eradication.exe.  This can be used in DLL's as well since we include the pointer
// to the SimulationConfig object in Environment.
#define GET_CONFIGURABLE(Category)  ((Kernel::Category*)(Environment::get##Category()))

using namespace std;

namespace Kernel
{
    struct VectorParameters;

    class SimulationConfig;

    class ISimulationConfigFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
    };

    class IDMAPI SimulationConfigFactory : public ISimulationConfigFactory
    {
    public:
        static ISimulationConfigFactory * getInstance();

        static SimulationConfig* CreateInstance(Configuration * config);
        void Register(string classname, instantiator_function_t _if);

    protected:
        static support_spec_map_t& getRegisteredClasses();

    private:
        static ISimulationConfigFactory * _instance;
    };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SimulationConfig Class Layout Special Notes
// Note 1:
// All non-primitive objects on the stack (like STL or user-defined type) have to be declared at the end
// for the primitive objects in the SimulationConfig object crossing the DLL/EModule boundary
// without affecting the memory image and therefore its member values
// Note 2:
// All the non-primitive _static_ objects will not be able to cross DLL/EModule boundary
// so their values will be different on two sides of memory space even for the same SimulationConfig object
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! DMB 9/7/2016 - This would be a bigger issue if we were supporting the ability of users to create their DLL to
// !!! work with our EXE.  Since we only support the situation where the user builds both the EXE and DLL,
// !!! the memory layout and boundary issues are not really a concern.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    class IDMAPI SimulationConfig : public JsonConfigurable
    {

        DECLARE_FACTORY_REGISTERED(SimulationConfigFactory, SimulationConfig, IConfigurable)

    public:
        DECLARE_CONFIGURED(SimulationConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        // Enum type name                                    Enum variable name                                name in config.json
        MigrationStructure::Enum                             migration_structure;                              // MIGRATION_STRUCTURE
        SimType::Enum                                        sim_type;                                         // Sim_Type


        bool demographics_initial;
        int default_torus_size;
        int default_node_population;

        float lloffset; // half the size of a grid edge in degrees, set by SetFlags()

        // parameters for individual
        bool interventions;

        // constant climate params
        float          airtemperature;
        float          landtemperature;
        float          rainfall;
        float          humidity;

        // scale params
        float          airtemperature_offset;
        float          landtemperature_offset;
        float          rainfall_scale_factor;
        float          humidity_scale_factor;

        // stochasticity params
        float          airtemperature_variance;
        float          landtemperature_variance;
        bool           rainfall_variance_enabled;
        float          humidity_variance;

        // flag for heterogeneity in mixing (true) or uniform mixing (false)
        bool heterogeneous_intranode_transmission_enabled;

        float Sim_Duration;
        float Sim_Tstep;
        float starttime;
        float node_grid_size;
        int Run_Number;

        std::string ConfigName;
        std::string airmig_filename;
        std::string campaign_filename;
        std::string climate_airtemperature_filename;
        std::string climate_koppen_filename;
        std::string climate_landtemperature_filename;
        std::string climate_rainfall_filename;
        std::string climate_relativehumidity_filename;
        std::string loadbalance_filename;
        std::string localmig_filename;
        std::string regionmig_filename;
        std::string seamig_filename;

        VectorParameters*  vector_params;

        ///////////////////////////////////////////
        SimulationConfig();
        virtual ~SimulationConfig();

        virtual QuickBuilder SimulationConfig::GetSchema() override;

        const Configuration* GetJsonConfigObj() const { return m_jsonConfig; }

        void SetFixedParameters(Configuration * inputJson);

    private: // for serialization to work
        const Configuration* m_jsonConfig;

        void VectorInitConfig( const Configuration* inputJson );
        void VectorCheckConfig( const Configuration* inputJson );
    };
}
