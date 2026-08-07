
#pragma once

#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>

#include "Climate.h"

namespace Kernel
{
    class RANDOMBASE;

    class ClimateByData : public Climate
    {
        GET_SCHEMA_STATIC_WRAPPER(ClimateByData)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        virtual bool Configure( const Configuration* config ) override;

    protected:
        int num_datapoints;
        int num_years;

        // vectors containing the entire timeline of data
        std::vector<float> airtemperature_data;
        std::vector<float> landtemperature_data;
        std::vector<float> rainfall_data;
        std::vector<float> humidity_data;

    public:
        static ClimateByData * CreateClimate( ClimateUpdateResolution::Enum update_resolution,
                                              INodeContext * _parent,
                                              int datapoints,
                                              std::ifstream& airtemperature_file,
                                              std::ifstream& landtemperature_file,
                                              std::ifstream& rainfall_file,
                                              std::ifstream& humidity_file,
                                              float start_time,
                                              RANDOMBASE* pRNG );

        virtual void UpdateWeather( float, float, RANDOMBASE* pRNG, bool initialization = false ) override;
        virtual ~ClimateByData();

    private:
        ClimateByData();
        ClimateByData(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent);

        virtual bool IsPlausible();

        void ReadDataFromFiles(int datapoints, std::ifstream& airtemperature_file, std::ifstream& landtemperature_file, std::ifstream& rainfall_file, std::ifstream& humidity_file);
    };
}
