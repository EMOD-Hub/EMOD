
#include "stdafx.h"
#include <limits>
#include "SusceptibilityHIV.h"
#include "IIndividualHumanContext.h"
#include "IIndividualHumanHIV.h"
#include "InfectionHIV.h"
#include "IHIVInterventionsContainer.h"
#include "Common.h"
#include "Debug.h"
#include "RANDOM.h"
#include "math.h"
#include "NodeEventContext.h"   // Needed for parent->GetEventContext()->GetNodeEventContext()
#include "SimulationConfig.h"
#include "EventTrigger.h"

SETUP_LOGGING( "SusceptibilityHIV" )

#define CD4_MINIMUM (50.0f)
#define CD4_RECONSTITUTION_COEFF_PER_SQ_MONTHS_ON_ART (-0.2113f)
#define CD4_RECONSTITUTION_COEFF_PER_MONTHS_ON_ART (15.584f)
#define CD4_RECONSTITUTION_SATURATION_ON_ART (287.3415f)
#define MONTHS_ON_ART_UNTIL_CD4_RECONSTITUTION_SATURATES (3.073f*MONTHSPERYEAR)
#define UNINITIALIZED_RATE (99999.0f)

namespace Kernel
{
#define MAX_CD4 (2500.0f)
#define CUM_PROB_CUTOFF (0.99f)
 
    GET_SCHEMA_STATIC_WRAPPER_IMPL(HIV.SusceptibilityHIV,SusceptibilityHIVConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityHIVConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityHIVConfig)

    float SusceptibilityHIVConfig::post_infection_CD4_lambda = 0.0f;
    float SusceptibilityHIVConfig::post_infection_CD4_inverse_kappa = 0.0f;
    float SusceptibilityHIVConfig::disease_death_CD4_alpha = 0.0f;
    float SusceptibilityHIVConfig::disease_death_CD4_inverse_beta = 0.0f;
    float SusceptibilityHIVConfig::days_between_symptomatic_and_death_lambda = 183.0f;
    float SusceptibilityHIVConfig::days_between_symptomatic_and_death_inv_kappa = 1.0f;

    bool
    SusceptibilityHIVConfig::Configure(
        const Configuration* config
    )
    {
        initConfigTypeMap( "CD4_Post_Infection_Weibull_Scale",         &post_infection_CD4_lambda, CD4_Post_Infection_Weibull_Scale_DESC_TEXT,                1.0f, 1000.0f, 560.4319099584783f );
        initConfigTypeMap( "CD4_Post_Infection_Weibull_Heterogeneity", &post_infection_CD4_inverse_kappa, CD4_Post_Infection_Weibull_Heterogeneity_DESC_TEXT, 0.0f, 100.0f, 1/3.627889600819898f ); 
        initConfigTypeMap( "CD4_At_Death_LogLogistic_Scale",         &disease_death_CD4_alpha, CD4_At_Death_LogLogistic_Scale_DESC_TEXT,                1.0f, 1000.0f, 31.63f );
        initConfigTypeMap( "CD4_At_Death_LogLogistic_Heterogeneity", &disease_death_CD4_inverse_beta, CD4_At_Death_LogLogistic_Heterogeneity_DESC_TEXT, 0.0f, 100.0f, 0.0f );
        initConfigTypeMap( "Days_Between_Symptomatic_And_Death_Weibull_Scale",         &days_between_symptomatic_and_death_lambda,    Days_Between_Symptomatic_And_Death_Weibull_Scale_DESC_TEXT,         1, 3650.0f, 183.0f );
        initConfigTypeMap( "Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity", &days_between_symptomatic_and_death_inv_kappa, Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity_DESC_TEXT, 0.1f, 10.0f, 1.0f );

        bool ret = JsonConfigurable::Configure( config );

        if( !JsonConfigurable::_dryrun &&
            (post_infection_CD4_inverse_kappa == 0) &&
            (disease_death_CD4_inverse_beta   == 0) &&
            (post_infection_CD4_lambda == disease_death_CD4_alpha) )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                    "CD4_Post_Infection_Weibull_Scale (with CD4_Post_Infection_Weibull_Heterogeneity == 0) ", post_infection_CD4_lambda,
                                                    "CD4_At_Death_LogLogistic_Scale (with CD4_At_Death_LogLogistic_Heterogeneity == 0) ", disease_death_CD4_alpha );
        }

        // If beta is zero, make sure alpha is below MAX CD4
        if(!JsonConfigurable::_dryrun &&
            disease_death_CD4_inverse_beta   == 0 &&
            disease_death_CD4_alpha > MAX_CD4 ) 
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                    "CD4_At_Death_LogLogistic_Scale", disease_death_CD4_alpha,
                    "CD4_At_Death_LogLogistic_Heterogeneity", disease_death_CD4_inverse_beta,
                    (std::string("Because there is no heterogeneity and the scale is large, all samples will exceed the maximum allowable CD4 of ") + std::to_string(MAX_CD4) + std::string(".")).c_str() );
        }

        if( post_infection_CD4_inverse_kappa > 0 )
        {
            // Check cumulative Weibull distribution to ensure that CUM_PROB_CUTOFF (e.g. 99%) of the mass is below MAX_CD4 so we can safely truncate if needed
            float post_infection_CD4_cumprob_at_max = 1 - exp( - pow( (MAX_CD4/post_infection_CD4_lambda), 1/post_infection_CD4_inverse_kappa) );
            if( !JsonConfigurable::_dryrun && 
                post_infection_CD4_cumprob_at_max < CUM_PROB_CUTOFF )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                    "CD4_Post_Infection_Weibull_Scale", post_infection_CD4_lambda,
                    "CD4_Post_Infection_Weibull_Heterogeneity", post_infection_CD4_inverse_kappa,
                    (std::string("The probability of sampling a CD4 value above ") + std::to_string(MAX_CD4) + std::string(" exceeds ") + std::to_string(CUM_PROB_CUTOFF) + std::string(".")).c_str() );
            }
        }

        if( disease_death_CD4_inverse_beta > 0 )
        {
            // Check cumulative Log Logistic distribution to ensure that CUM_PROB_CUTOFF (e.g. 99.9%) of the mass is below MAX_CD4 so we can safely truncate if needed
            float disease_death_CD4_cumprob_at_max = 1 / (1 + pow(MAX_CD4/disease_death_CD4_alpha, -1/disease_death_CD4_inverse_beta));
            if( !JsonConfigurable::_dryrun && 
                disease_death_CD4_cumprob_at_max < CUM_PROB_CUTOFF )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                    "CD4_At_Death_LogLogistic_Scale", disease_death_CD4_alpha,
                    "CD4_At_Death_LogLogistic_Heterogeneity", disease_death_CD4_inverse_beta,
                    (std::string("The probability of sampling a CD4 value above ") + std::to_string(MAX_CD4) + std::string(" exceeds ") + std::to_string(CUM_PROB_CUTOFF) + std::string(".")).c_str() );
            }
        }

        return ret;
    }

    //---------------------------- SusceptibilityHIV ----------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityHIV)
        HANDLE_INTERFACE(ISusceptibilityHIV)
    END_QUERY_INTERFACE_BODY(SusceptibilityHIV)

    Susceptibility *SusceptibilityHIV::CreateSusceptibility(IIndividualHumanContext *context, float immmod, float riskmod)
    {
        SusceptibilityHIV *newsusceptibility = _new_ SusceptibilityHIV(context);
        newsusceptibility->Initialize(immmod, riskmod);

        return newsusceptibility;
    }

    void SusceptibilityHIV::SetContextTo(IIndividualHumanContext* context)
    {
        SusceptibilitySTI::SetContextTo( context );

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IndividualHuman" );
        }
    }

    void SusceptibilityHIV::setCD4Rate(const IInfectionHIV * const pInf)
    {
        release_assert( pInf );

        // Initialize sqrtCD4_Rate using GetPrognosis() from InfectionHIV
        float prognosis = pInf->GetPrognosis();
        release_assert( prognosis != 0.0 );

        sqrtCD4_Rate = (sqrtCD4_AtDiseaseDeath - sqrtCD4_PostInfection) / prognosis;

        LOG_DEBUG_F("Individual %d CD4: sqrtCD4_PostInfection=%f, sqrtCD4_AtDiseaseDeath=%f, Prognosis=%f --> sqrtCD4_Rate=%f\n",
                     parent->GetSuid().data, sqrtCD4_PostInfection, sqrtCD4_AtDiseaseDeath, prognosis, sqrtCD4_Rate);
    }

    // The purpose of this function is to reduce the individual's CD4 count all at once
    // as dictated by the value of dt passed in. This does not represent a real-world 
    // behaviour. Rather it is for Outbreaks where we are pretending time has elapsed.
    void SusceptibilityHIV::FastForward( const IInfectionHIV* const pInf, float dt )
    {
        if( sqrtCD4_Rate == UNINITIALIZED_RATE )
        {
           setCD4Rate(pInf);
        }

        sqrtCD4_Current += sqrtCD4_Rate * dt;

        release_assert( sqrtCD4_Current >= sqrtCD4_AtDiseaseDeath);
        release_assert( sqrtCD4_Current <= sqrtCD4_PostInfection );
    }

    // The Update function primarily increases or decreases the value of CD4count depending
    // on whether or not the individual has untreated HIV -- decreases -- or is on a viral 
    // suppression intervention (e.g., ART) -- increases. 
    void SusceptibilityHIV::Update(float dt)
    {
        Susceptibility::Update(dt);

        release_assert( hiv_parent );
        if ( !hiv_parent->HasHIV() )
        {
            return;
        }

        if( sqrtCD4_Rate == UNINITIALIZED_RATE )
        {
            IInfectionHIV * pHIVInfection = hiv_parent->GetHIVInfection();
            setCD4Rate(pHIVInfection);
        }

        if( hiv_parent->GetHIVInterventionsContainer()->ShouldReconstituteCD4() )
        {
            // Adding dt because the interventions container updates after susceptibility.  Fixes rare assertion failure.
            float months_since_starting_ART = (hiv_parent->GetHIVInterventionsContainer()->GetDurationSinceLastStartingART() + dt) / IDEALDAYSPERMONTH;
            NO_MORE_THAN( months_since_starting_ART, MONTHS_ON_ART_UNTIL_CD4_RECONSTITUTION_SATURATES )
            release_assert( months_since_starting_ART >= 0 );

            if( sqrtCD4_Current < sqrtCD4_PostInfection )
            {
                float increase_in_CD4 = CD4_RECONSTITUTION_COEFF_PER_SQ_MONTHS_ON_ART * pow( months_since_starting_ART, 2)
                                      + CD4_RECONSTITUTION_COEFF_PER_MONTHS_ON_ART * months_since_starting_ART;

                NO_MORE_THAN( increase_in_CD4, CD4_RECONSTITUTION_SATURATION_ON_ART );
                sqrtCD4_Current = sqrt(CD4count_at_ART_start + increase_in_CD4);
                NO_MORE_THAN( sqrtCD4_Current, sqrtCD4_PostInfection );

                LOG_DEBUG_F( "Individual %d increased CD4 because of ART.  CD4=%f, PostInfection=%f, AtDiseaseDeath=%f\n",
                             parent->GetSuid().data, GetCD4count(), pow(sqrtCD4_PostInfection,2), pow(sqrtCD4_AtDiseaseDeath,2) );

                release_assert( sqrtCD4_Current >= sqrtCD4_AtDiseaseDeath);
                release_assert( sqrtCD4_Current <= sqrtCD4_PostInfection );
            }
        }
        else // not on art, regular.  DJK: could FastForward by dt to avoid code redundancy
        {
            float delta = sqrtCD4_Rate * dt;
            sqrtCD4_Current += delta;
            LOG_DEBUG_F( "delta CD4 for individual %d = %f\n", parent->GetSuid().data, delta );

            // Large DT make CD4count negative
            if( sqrtCD4_Current < sqrtCD4_AtDiseaseDeath )
            {
                sqrtCD4_Current = sqrtCD4_AtDiseaseDeath;
            }

            LOG_DEBUG_F( "Individual %d decreased CD4 because of HIV\n", parent->GetSuid().data, GetCD4count() );
            if( GetCD4count() < 30 )
            {
                LOG_DEBUG_F( "Individual %d, CD4 dropped below 30, prognosis = %f\n", parent->GetSuid().data, hiv_parent->GetHIVInfection()->GetPrognosis() );
            }

            IInfectionHIV *hiv_infection = hiv_parent->GetHIVInfection();

            float days_till_death = hiv_infection->GetDaysTillDeath();

            if( days_till_death <= days_between_symptomatic_and_death ) 
            {
                is_symptomatic = true;
                days_between_symptomatic_and_death = -FLT_MAX;
            }
        }

        LOG_DEBUG_F( "Individual %d has CD4 count of %f\n", parent->GetSuid().data, GetCD4count() );
    }

    void SusceptibilityHIV::ApplyARTOnset()
    {
        CD4count_at_ART_start = GetCD4count();
        LOG_DEBUG_F( "CD4count_at_ART_start = %f for individual %d\n", CD4count_at_ART_start, parent->GetSuid().data );
    }

    void SusceptibilityHIV::UpdateInfectionCleared()
    {
        //placeholder, in TB it counts the number of simultaneous infections you have
    }

    void SusceptibilityHIV::Initialize(float _immmod, float _riskmod)
    {
        Susceptibility::Initialize(_immmod, _riskmod);

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IndividualHuman" );
        }

        // Calculate CD4 post-infection and at HIV-cause death
        float CD4_PostInfection = parent->GetRng()->Weibull2(SusceptibilityHIVConfig::post_infection_CD4_lambda, SusceptibilityHIVConfig::post_infection_CD4_inverse_kappa );
        CD4_PostInfection = (std::min)(CD4_PostInfection, MAX_CD4);
        sqrtCD4_PostInfection = sqrt(CD4_PostInfection);

        float CD4_AtDiseaseDeath = SusceptibilityHIVConfig::disease_death_CD4_alpha;
        if( SusceptibilityHIVConfig::disease_death_CD4_inverse_beta != 0.0f )
        {
            CD4_AtDiseaseDeath = parent->GetRng()->LogLogistic( SusceptibilityHIVConfig::disease_death_CD4_alpha, 1.0f/SusceptibilityHIVConfig::disease_death_CD4_inverse_beta );
        }
        CD4_AtDiseaseDeath = (std::min)(CD4_AtDiseaseDeath, MAX_CD4);
        sqrtCD4_AtDiseaseDeath = sqrt( CD4_AtDiseaseDeath );

        if( sqrtCD4_PostInfection < sqrtCD4_AtDiseaseDeath)
        {
            LOG_WARN_F( "Initial CD4 %f is less than final CD4 %f, swapping low and high!\n", pow(sqrtCD4_PostInfection,2), pow(sqrtCD4_AtDiseaseDeath,2) );
            float tmp = sqrtCD4_PostInfection;
            sqrtCD4_PostInfection = sqrtCD4_AtDiseaseDeath;
            sqrtCD4_AtDiseaseDeath = tmp;
        }

        // By chance or by configuration sqrtCD4_PostInfection could equal sqrtCD4_AtDiseaseDeath
        // To map from CD4 to prognosis fraction complete and back, CD4 must decline over time
        // To work around non-declining CD4 in this rare case, we slightly increase sqrtCD4_PostInfection
        if( sqrtCD4_PostInfection == sqrtCD4_AtDiseaseDeath) {
            sqrtCD4_PostInfection = sqrt( pow(sqrtCD4_PostInfection,2) + 1 );
            LOG_WARN_F( "Increased initial CD4 to %f because it equaled final CD4 of %f\n", pow(sqrtCD4_PostInfection,2), pow(sqrtCD4_AtDiseaseDeath,2) );
        }

        sqrtCD4_Current = sqrtCD4_PostInfection;

        UpdateSymptomaticPresentationTime();

        LOG_DEBUG_F( "Individual %d will be symptomatic %f days before death\n", parent->GetSuid().data, days_between_symptomatic_and_death );
    }

    void SusceptibilityHIV::UpdateSymptomaticPresentationTime()
    {
        // Calculate symptomatic presentation time 
        is_symptomatic = false;
        days_between_symptomatic_and_death = parent->GetRng()->Weibull2( SusceptibilityHIVConfig::days_between_symptomatic_and_death_lambda,
                                                                         SusceptibilityHIVConfig::days_between_symptomatic_and_death_inv_kappa );

        LOG_DEBUG_F( "Individual %d will be symptomatic %f days before death\n", parent->GetSuid().data, days_between_symptomatic_and_death );
    }

    ProbabilityNumber
    SusceptibilityHIV::GetPrognosisCompletedFraction()
    const
    {
        release_assert( sqrtCD4_PostInfection != sqrtCD4_AtDiseaseDeath );

        float fraction = max(0.0f, (sqrtCD4_PostInfection - sqrtCD4_Current)) / (sqrtCD4_PostInfection - sqrtCD4_AtDiseaseDeath);
        NO_MORE_THAN( fraction, 1.0f )
        ProbabilityNumber fraction_completed = fraction;

        LOG_DEBUG_F( "Individual %d in GetPrognosisCompletedFraction: sqrtCD4_PostInfection=%f, sqrtCD4_AtDiseaseDeath=%f, sqrtCD4_Current=%f --> fraction=%f\n", 
                     parent->GetSuid().data, sqrtCD4_PostInfection, sqrtCD4_AtDiseaseDeath, sqrtCD4_Current, float(fraction_completed) );

        return fraction_completed;
    }

    void
    SusceptibilityHIV::TerminateSuppression( float days_remaining )
    {
        if( days_remaining > 0 )
        {
            sqrtCD4_Rate = (sqrtCD4_AtDiseaseDeath - sqrtCD4_Current) / days_remaining;
        }
        else
        {
            sqrtCD4_Rate = 0.0;
        }

        // Prepare to be NewlySymptomatic again
        UpdateSymptomaticPresentationTime();
        LOG_DEBUG_F( "After terminating suppression, individual %d will be symptomatic %f days before death\n", parent->GetSuid().data, days_between_symptomatic_and_death );
    }

    float SusceptibilityHIV::GetDaysBetweenSymptomaticAndDeath() const
    {
        return days_between_symptomatic_and_death;
    }

    bool SusceptibilityHIV::IsSymptomatic() const
    {
        return is_symptomatic;
    }

    float SusceptibilityHIV::GetCD4count() const
    {
        return pow(sqrtCD4_Current, 2);
    }

    SusceptibilityHIV::~SusceptibilityHIV(void) { }

    SusceptibilityHIV::SusceptibilityHIV()
        : SusceptibilitySTI()
        , hiv_parent( nullptr )
        , is_symptomatic( false )
        , days_between_symptomatic_and_death(0)
        , sqrtCD4_Current( 0 )
        , sqrtCD4_Rate( UNINITIALIZED_RATE )
        , sqrtCD4_PostInfection( 0 )
        , sqrtCD4_AtDiseaseDeath( 0 )
        , CD4count_at_ART_start( 0 )
    { 
    }

    SusceptibilityHIV::SusceptibilityHIV(IIndividualHumanContext *context)
        : SusceptibilitySTI(context)
        , hiv_parent( nullptr )
        , is_symptomatic( false )
        , days_between_symptomatic_and_death(0)
        , sqrtCD4_Current( 0 )
        , sqrtCD4_Rate( UNINITIALIZED_RATE )
        , sqrtCD4_PostInfection( 0 )
        , sqrtCD4_AtDiseaseDeath( 0 )
        , CD4count_at_ART_start( 0 )
        { }

    REGISTER_SERIALIZABLE(SusceptibilityHIV);

    void SusceptibilityHIV::serialize(IArchive& ar, SusceptibilityHIV* obj)
    {
        SusceptibilitySTI::serialize( ar, obj );
        SusceptibilityHIV& suscep = *obj;
        ar.labelElement("is_symptomatic"                    ) & suscep.is_symptomatic;
        ar.labelElement("days_between_symptomatic_and_death") & suscep.days_between_symptomatic_and_death;
        ar.labelElement("sqrtCD4_Current"                   ) & suscep.sqrtCD4_Current;
        ar.labelElement("sqrtCD4_Rate"                      ) & suscep.sqrtCD4_Rate;
        ar.labelElement("sqrtCD4_PostInfection"             ) & suscep.sqrtCD4_PostInfection;
        ar.labelElement("sqrtCD4_AtDiseaseDeath"            ) & suscep.sqrtCD4_AtDiseaseDeath;
        ar.labelElement("CD4count_at_ART_start"             ) & suscep.CD4count_at_ART_start;

        // hiv_parent; - Updated in SetContextTo
    }
}
