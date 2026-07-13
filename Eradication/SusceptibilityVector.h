
#pragma once

#include "VectorContexts.h"
#include "Common.h"
#include "Susceptibility.h"

namespace Kernel
{
    class SimulationConfig;

    class SusceptibilityVectorConfig : public SusceptibilityConfig
    {
        friend class IndividualHumanVector;
    public:
        virtual bool Configure( const Configuration* config ) override;

    protected:
        friend class SusceptibilityVector;

        static AgeDependentBitingRisk::Enum age_dependent_biting_risk_type;
        static float m_newborn_biting_risk;

        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityVectorConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    class SusceptibilityVector : public Susceptibility, public IVectorSusceptibilityContext
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static SusceptibilityVector *CreateSusceptibility(IIndividualHumanContext *context, float immmod = 1.0f, float riskmod = 1.0f);
        virtual ~SusceptibilityVector();

        virtual void Update(float dt=0.0) override;

        virtual void  SetRelativeBitingRate( float rate ) override;
        virtual float GetRelativeBitingRate(void) const override;

        static float LinearBitingFunction(float);

        // The code for this function is in the header file so that it can be easily used by DLLs
        static float SurfaceAreaBitingFunction(float input_age)
        {
            // piecewise linear rising from birth to age 2
            // and then shallower slope to age 20
            float newborn_risk = 0.07f;
            float two_year_old_risk = 0.23f;
            if( input_age < 2 * DAYSPERYEAR )
            {
                return newborn_risk + input_age * (two_year_old_risk - newborn_risk) / (2 * DAYSPERYEAR);
            }

            if( input_age < 20 * DAYSPERYEAR )
            {
                return two_year_old_risk + (input_age - 2 * DAYSPERYEAR) * (1 - two_year_old_risk) / ((20 - 2) * DAYSPERYEAR);
            }

            return 1.0f;
        }


    protected:
        SusceptibilityVector();
        SusceptibilityVector(IIndividualHumanContext *context);
        virtual void Initialize(float immmod, float riskmod) override;
        float BitingRiskAgeFactor();

        // effect of heterogeneous biting explored in Smith, D. L., F. E. McKenzie, et al. (2007). "Revisiting the basic reproductive number for malaria and its implications for malaria control." PLoS Biol 5(3): e42.
        // also in Smith, D. L., J. Dushoff, et al. (2005). "The entomological inoculation rate and Plasmodium falciparum infection in African children." Nature 438(7067): 492-495.
        float m_relative_biting_rate;
        float m_age_dependent_biting_risk;

        DECLARE_SERIALIZABLE(SusceptibilityVector);
    };
}
