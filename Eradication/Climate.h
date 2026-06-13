
#pragma once

#include "stdafx.h"
#include <fstream>
#include <math.h>
#include <unordered_map>

#include "Sugar.h"
#include "CajunIncludes.h"
#include "SimulationEnums.h"
#include "Configure.h"
#include "ExternalNodeId.h"

#ifdef __GNUC__
namespace std
{
     using namespace __gnu_cxx;
}
#endif

class Configuration;

#define PI 3.141593

namespace Kernel
{
    class RANDOMBASE;
    struct INodeContext;

    class Climate : public JsonConfigurable
    {
    public:
        // for JsonConfigurable stuff...
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        bool Configure( const Configuration* config );
        ClimateUpdateResolution::Enum   climate_update_resolution;

        float          base_airtemperature;
        float          base_landtemperature;
        float          base_rainfall;
        float          base_humidity;

        // scale params
        float          airtemperature_offset;
        float          landtemperature_offset;
        float          rainfall_scale_factor;
        float          humidity_scale_factor;

        // stochasticity params
        bool           enable_climate_stochasticity;
        float          airtemperature_variance;
        float          landtemperature_variance;
        bool           rainfall_variance_enabled;
        float          humidity_variance;

        inline float airtemperature()       const { return m_airtemperature; }
        inline float landtemperature()      const { return m_landtemperature; }
        inline float accumulated_rainfall() const { return m_accumulated_rainfall; }
        inline float humidity()             const { return m_humidity; }

    protected:
        float m_airtemperature;
        float m_landtemperature;
        float m_accumulated_rainfall;
        float m_humidity;
        float resolution_correction;

        static const float min_airtemp;
        static const float max_airtemp;
        static const float min_landtemp;
        static const float max_landtemp;
        static const float max_rainfall;

        INodeContext* parent;

    public:
        virtual ~Climate();
        void SetContextTo(INodeContext* _parent);

        //  Updates weather based on the time step.  If the time step is long, then it adjusts.
        //  For instance, is rainfall over an hour or over a week?
        virtual void UpdateWeather( float time, float dt, RANDOMBASE* pRNG, bool initialization=false );

    protected:
        friend class ClimateFactory;

        Climate(ClimateUpdateResolution::Enum update_resolution = ClimateUpdateResolution::CLIMATE_UPDATE_DAY, INodeContext * _parent = nullptr);

        virtual void AddStochasticity( RANDOMBASE* pRNG,
                                       float airtemp_variance,
                                       float landtemp_variance,
                                       bool rainfall_variance_enabled,
                                       float humidity_variance );

        virtual bool IsPlausible() = 0;
    };


    class ClimateFactory : public JsonConfigurable
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER(ClimateFactory)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        bool Configure( const Configuration* config );

        static ClimateFactory* CreateClimateFactory(boost::bimap<ExternalNodeId_t, suids::suid> * nodeid_suid_map, const ::Configuration *config, const std::string idreference);
        ~ClimateFactory();

        Climate* CreateClimate( INodeContext *parent_node, float altitude, float latitude, RANDOMBASE* pRNG );

        static ClimateStructure::Enum climate_structure;

    private:
        ClimateFactory(boost::bimap<ExternalNodeId_t, suids::suid> * nodeid_suid_map);
        ClimateFactory(){} // just for GetSchema
        bool Initialize(const ::Configuration *config, const std::string idreference);
        bool ParseMetadataForFile(std::string data_filepath, std::string idreference, ClimateUpdateResolution::Enum * const update_resolution, int * const num_datavalues, int * const num_nodes, std::unordered_map<uint32_t, uint32_t> &node_offsets);
        bool OpenClimateFile(std::string filepath, uint32_t expected_size, std::ifstream &file);

        boost::bimap<ExternalNodeId_t, suids::suid> * nodeid_suid_map;

        std::string climate_airtemperature_filename;
        std::string climate_koppen_filename;
        std::string climate_landtemperature_filename;
        std::string climate_rainfall_filename;
        std::string climate_relativehumidity_filename;

        // data file stream handles for ClimateByData
        std::ifstream climate_landtemperature_file;
        std::ifstream climate_airtemperature_file;
        std::ifstream climate_rainfall_file;
        std::ifstream climate_humidity_file;

        // data file stream handle for ClimateKoppen
        std::ifstream climate_koppentype_file;

        // node offsets in ClimateByData files
        std::unordered_map<uint32_t, uint32_t> landtemperature_offsets;
        std::unordered_map<uint32_t, uint32_t> airtemperature_offsets;
        std::unordered_map<uint32_t, uint32_t> rainfall_offsets;
        std::unordered_map<uint32_t, uint32_t> humidity_offsets;

        // node offsets in ClimateKoppen file
        std::unordered_map<uint32_t, uint32_t> koppentype_offsets;

        // metadata info
        ClimateUpdateResolution::Enum climate_update_resolution;
        int num_datavalues;
        int num_nodes;

        int num_badnodes;
        float start_time;
    };
}
