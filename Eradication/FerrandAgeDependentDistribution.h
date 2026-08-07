
#pragma once

#include "Configure.h"

namespace Kernel
{
    class FerrandAgeDependentDistribution : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(FerrandAgeDependentDistribution)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        FerrandAgeDependentDistribution();

        float pdf(float x, float param) { return -1.0f; }
   
        //Param is the age of individual at time of infection, 
        //It should be in the interval [0,100]
        float invcdf(float y, float age_param);

        // TODO pass in the kappa and lambda values and slopes as a fn of age rather
        // than hard-coding them

        // JsonConfigurable Methods
        virtual bool Configure(const Configuration *config) override;

    private:    
        static float DistributionInvCDF(float rand, float kappa, float lambda);

        // PARAMETERS DEFINING PROBABILITY DISTRIBUTION OF PROGNOSIS FOR UNTREATED HIV INFECTION:

        float m_child_age_years ;
        float m_max_age_years ;

        // Childhood survival probability is bimodal, represented as the sum of an exponential and a weibull:
        // The weighting of the two distributions is alpha for the first and 1-alpha for the second.
        float m_child_beta;
        float m_child_s;
        float m_child_mu;
        float m_child_alpha;

        // Adult survival Weibull distribution is unimodal, defined by a single Weibull distribution.
        // Kappa and lambda change linearly as a function of age at time of infection.
        float m_adult_lambda_slope;
        float m_adult_lambda_intercept;
        float m_adult_kappa_intercept; 

    };
}
