
#pragma once

#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>

#include "Climate.h"

namespace Kernel
{
    class RANDOMBASE;

    class ClimateConstant : public Climate
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER(ClimateConstant)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        virtual bool Configure( const Configuration* config ) override;
        static ClimateConstant * CreateClimate( ClimateUpdateResolution::Enum update_resolution,
                                                INodeContext * _parent,
                                                float start_time,
                                                RANDOMBASE* pRNG );

        virtual void UpdateWeather( float, float, RANDOMBASE* pRNG, bool initialization = false ) override;

    protected:
        ClimateConstant();
        ClimateConstant(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent);

        virtual bool IsPlausible();
    };
}
