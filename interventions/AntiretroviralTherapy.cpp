
#include "stdafx.h"

#include "AntiretroviralTherapy.h"

#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "IHIVInterventionsContainer.h"  // for IHIVDrugEffectsApply methods
#include "IInfectionHIV.h"
#include "ISusceptibilityHIV.h"
#include "IIndividualHumanHIV.h"
#include "SimulationEnums.h"
#include "RANDOM.h"
#include "Debug.h"

SETUP_LOGGING( "AntiretroviralTherapy" )

#define AGE_40_YRS_IN_DAYS (40.0f*DAYSPERYEAR)

#define DEFAULT_MULTIPLIER_ON_TRANSMISSION (0.08f)
#define DEFAULT_DAYS_TO_ACHIEVE_SUPPRESSION (183.0f)
#define DEFAULT_MAX_COX_CD4 (350.0f)
#define DEFAULT_IEDEA_KAPPA (0.34f)
#define DEFAULT_IEDEA_LAMBDA_BASE_IN_YRS (123.83f)
#define DEFAULT_IEDEA_FEMALE_MULTIPLIER (0.6775f)
#define DEFAULT_IEDEA_OVER_AGE_40Y_MULTIPLIER (1.4309f)
#define DEFAULT_WHO_STAGE_THRESHOLD_FOR_COX (3)
#define DEFAULT_IEDEA_STAGE_3_PLUS_MULTIPLIER (2.7142f)
#define DEFAULT_COX_PROP_CONSTANT_1 (-0.00758256281931556f)
#define DEFAULT_COX_PROP_CONSTANT_2 (0.282851687024819f)
#define DEFAULT_COX_PROP_CONSTANT_3 (-0.0731529900006081f)
#define DEFAULT_COX_PROP_CONSTANT_4 (3.05043211490958f)

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( AntiretroviralTherapy )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( AntiretroviralTherapy )

    IMPLEMENT_FACTORY_REGISTERED(AntiretroviralTherapy)

    AntiretroviralTherapy::AntiretroviralTherapy()
    : BaseIntervention()
    , m_MultiplierOnTransmission( DEFAULT_MULTIPLIER_ON_TRANSMISSION )
    , m_IsActiveViralSuppression(true)
    , m_DaysToAchieveSuppression( DEFAULT_DAYS_TO_ACHIEVE_SUPPRESSION )
    , m_MaxCoxCD4(            DEFAULT_MAX_COX_CD4 )
    , m_WeibullShape(         DEFAULT_IEDEA_KAPPA )
    , m_WeibullScale(         DEFAULT_IEDEA_LAMBDA_BASE_IN_YRS )
    , m_FemaleMultiplier(     DEFAULT_IEDEA_FEMALE_MULTIPLIER )
    , m_Over40Multipiler(     DEFAULT_IEDEA_OVER_AGE_40Y_MULTIPLIER )
    , m_WhoStageCoxThreshold( DEFAULT_WHO_STAGE_THRESHOLD_FOR_COX )
    , m_Stage3Multiplier(     DEFAULT_IEDEA_STAGE_3_PLUS_MULTIPLIER )
    , m_CD4Slope(             DEFAULT_COX_PROP_CONSTANT_1 )
    , m_CD4Intercept(         DEFAULT_COX_PROP_CONSTANT_2 )
    , m_WeightSlope(          DEFAULT_COX_PROP_CONSTANT_3 )
    , m_WeightIntercept(      DEFAULT_COX_PROP_CONSTANT_4 )
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    AntiretroviralTherapy::AntiretroviralTherapy( const AntiretroviralTherapy& rMaster)
    : BaseIntervention( rMaster )
    , m_MultiplierOnTransmission( rMaster.m_MultiplierOnTransmission )
    , m_IsActiveViralSuppression( rMaster.m_IsActiveViralSuppression )
    , m_DaysToAchieveSuppression( rMaster.m_DaysToAchieveSuppression )
    , m_MaxCoxCD4(                rMaster.m_MaxCoxCD4 )
    , m_WeibullShape(             rMaster.m_WeibullShape )
    , m_WeibullScale(             rMaster.m_WeibullScale )
    , m_FemaleMultiplier(         rMaster.m_FemaleMultiplier )
    , m_Over40Multipiler(         rMaster.m_Over40Multipiler )
    , m_WhoStageCoxThreshold(     rMaster.m_WhoStageCoxThreshold )
    , m_Stage3Multiplier(         rMaster.m_Stage3Multiplier )
    , m_CD4Slope(                 rMaster.m_CD4Slope )
    , m_CD4Intercept(             rMaster.m_CD4Intercept )
    , m_WeightSlope(              rMaster.m_WeightSlope )
    , m_WeightIntercept(          rMaster.m_WeightIntercept )
    {
    }

    AntiretroviralTherapy::~AntiretroviralTherapy()
    {
    }

    bool
    AntiretroviralTherapy::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 99999, 1 );
        initConfigTypeMap( "ART_Multiplier_On_Transmission_Prob_Per_Act",             &m_MultiplierOnTransmission, ART_Multiplier_On_Transmission_Prob_Per_Act_DESC_TEXT,             0.0f,     1.0f,     DEFAULT_MULTIPLIER_ON_TRANSMISSION    );
        initConfigTypeMap( "ART_Is_Active_Against_Mortality_And_Transmission",        &m_IsActiveViralSuppression, ART_Is_Active_Against_Mortality_And_Transmission_DESC_TEXT, true );
        initConfigTypeMap( "Days_To_Achieve_Viral_Suppression",                       &m_DaysToAchieveSuppression, ART_Days_To_Achieve_Viral_Suppression_DESC_TEXT,                   0.0f,      FLT_MAX,   DEFAULT_DAYS_TO_ACHIEVE_SUPPRESSION   , "ART_Is_Active_Against_Mortality_And_Transmission" );

        ConfigureMortalityParams(inputJson);

        bool result = BaseIntervention::Configure(inputJson);

        if (result && !JsonConfigurable::_dryrun)
        {
            ValidateParams();
        }

        return result;
    }

    void AntiretroviralTherapy::ConfigureMortalityParams(const Configuration * inputJson)
    {
        initConfigTypeMap("ART_CD4_at_Initiation_Saturating_Reduction_in_Mortality", &m_MaxCoxCD4, ART_CD4_at_Initiation_Saturating_Reduction_in_Mortality_DESC_TEXT, 0.0f, FLT_MAX, DEFAULT_MAX_COX_CD4, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Baseline_Hazard_Weibull_Shape", &m_WeibullShape, ART_Survival_Baseline_Hazard_Weibull_Shape_DESC_TEXT, 0.0f, 10.0f, DEFAULT_IEDEA_KAPPA, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Baseline_Hazard_Weibull_Scale", &m_WeibullScale, ART_Survival_Baseline_Hazard_Weibull_Scale_DESC_TEXT, 1.00E-06f, 1.00E+06f, DEFAULT_IEDEA_LAMBDA_BASE_IN_YRS, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_Female", &m_FemaleMultiplier, ART_Survival_Hazard_Ratio_Female_DESC_TEXT, 1.00E-06f, 1.00E+06f, DEFAULT_IEDEA_FEMALE_MULTIPLIER, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_Age_Over_40Yr", &m_Over40Multipiler, ART_Survival_Hazard_Ratio_Age_Over_40Yr_DESC_TEXT, 1.00E-06f, 1.00E+06f, DEFAULT_IEDEA_OVER_AGE_40Y_MULTIPLIER, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_WHO_Stage_Threshold_For_Cox", &m_WhoStageCoxThreshold, ART_Survival_WHO_Stage_Threshold_For_Cox_DESC_TEXT, 0.0f, 5.0f, DEFAULT_WHO_STAGE_THRESHOLD_FOR_COX, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_WHO_Stage_3Plus", &m_Stage3Multiplier, ART_Survival_Hazard_Ratio_WHO_Stage_3Plus_DESC_TEXT, 1.00E-06f, 1.00E+06f, DEFAULT_IEDEA_STAGE_3_PLUS_MULTIPLIER, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_CD4_Slope", &m_CD4Slope, ART_Survival_Hazard_Ratio_CD4_Slope_DESC_TEXT, -1.00E+06f, 1.00E+06f, DEFAULT_COX_PROP_CONSTANT_1, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_CD4_Intercept", &m_CD4Intercept, ART_Survival_Hazard_Ratio_CD4_Intercept_DESC_TEXT, -1.00E+06f, 1.00E+06f, DEFAULT_COX_PROP_CONSTANT_2, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_Body_Weight_Kg_Slope", &m_WeightSlope, ART_Survival_Hazard_Ratio_Body_Weight_Kg_Slope_DESC_TEXT, -1.00E+06f, 1.00E+06f, DEFAULT_COX_PROP_CONSTANT_3, "ART_Is_Active_Against_Mortality_And_Transmission");
        initConfigTypeMap("ART_Survival_Hazard_Ratio_Body_Weight_Kg_Intercept", &m_WeightIntercept, ART_Survival_Hazard_Ratio_Body_Weight_Kg_Intercept_DESC_TEXT, -1.00E+06f, 1.00E+06f, DEFAULT_COX_PROP_CONSTANT_4, "ART_Is_Active_Against_Mortality_And_Transmission");
    }

    void AntiretroviralTherapy::ValidateParams()
    {
    }

    bool AntiretroviralTherapy::CanDistribute(IIndividualHumanHIV* p_hiv_human)
    {
        return !p_hiv_human->GetHIVInterventionsContainer()->OnArtQuery();
    }

    bool
    AntiretroviralTherapy::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            IIndividualHumanHIV* p_hiv_human = nullptr;
            if( s_OK != context->GetParent()->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&p_hiv_human ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetParent()", "IIndividualHumanHIV", "IIndividualHumanContext" );
            }

            // -----------------------------------------------------------------------------------
            // --- If the person is NOT infected or already on ART, then we allow the intervention
            // --- to be distributed to the individual but do not apply it,  The internal application
            // --- of being OnART requires the individual to be infected.
            // -----------------------------------------------------------------------------------
            if( p_hiv_human->HasHIV() && CanDistribute(p_hiv_human))
            {
                LOG_DEBUG_F( "AntiretroviralTherapy distributed to individual %d.\n", context->GetParent()->GetSuid().data );
                IHIVDrugEffectsApply* itbda = nullptr;
                if (s_OK != context->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
                } 

                float duration = 0.0;
                if( m_IsActiveViralSuppression )
                {
                    duration = ComputeDurationFromEnrollmentToArtAidsDeath( context->GetParent(), p_hiv_human );
                }

                itbda->GoOnART( m_IsActiveViralSuppression, m_DaysToAchieveSuppression, duration, m_MultiplierOnTransmission );
            }
        }
        return distributed;
    }

    void AntiretroviralTherapy::Update( float dt )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Don't disqualify because the action was in the Distribute() method.
        // !!! When we move the action (GoOnART) to the Update() method, we want to
        // !!! check the Disqualifying_Properties, but not now.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if( !BaseIntervention::UpdateIndividualsInterventionStatus( false ) ) return;

        SetExpired( true );
    }

    // static: doesn't use any member variables, just lives in this class for convenience.
    // Purpose: This function converts a WHO HIV stage (1-4 really, but 1.0-4.99 for us)
    // into a human body weight in kg. It does a linear interpolation on the hardcoded array.
    float AntiretroviralTherapy::GetWeightInKgFromWHOStage( float whoStage )
    {
        if( whoStage < MIN_WHO_HIV_STAGE || whoStage > MAX_WHO_HIV_STAGE )
        {
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "whoStage", whoStage, (whoStage < MIN_WHO_HIV_STAGE ? MIN_WHO_HIV_STAGE : MAX_WHO_HIV_STAGE) );
        }

        // All hardcoded here because these are the weight boundaries for WHO HIV stages.
        // !!! SEE BELOW FOR EXPLANATION !!!
        float weights[] = { 65.0f, 62.1f, 57.0f, 50.0f, 40.1f }; // Kg

        int left = int( floor( whoStage ) ) - 1;
        int right = int( ceil( whoStage ) ) - 1;

        float weightInKilograms = weights[ left ] + (weights[ right ] - weights[ left ]) * (whoStage - floor( whoStage ));
        LOG_DEBUG_F( "%s returning %f for stage %f\n", __FUNCTION__, weightInKilograms, whoStage );
        return weightInKilograms;
    }

    // -------------------------------------------------------------------------------------------------------
    // !!! WEIGHTS EXPLANATION - Values used above. !!! 5/14/2015 AB
    // The goal of the weight model was to combine information about WHO guideline and clinical progression 
    // in order to estimate body weight over the course of untreated HIV infection. This in turn is used to 
    // estimate body weight at treatment initiation, which is used to compute survivorship based to reflect
    // that of the IeDEA cohorts. The WHO guidelines for clinical staging (revised 2005) specify that WHO 
    // stage I/II is defined as <10% loss in body weight and WHO stage III/IV is a >10% loss in body weight. 
    // Thus, we placed the transition between stages II and III at approximately 10% loss of body weight. 
    // IeDEA uses absolute weight rather than percentage change from pre-HIV baseline or BMI, because 
    // pre-HIV weight and height are often unavailable in patient records. We therefore assumed an intial
    // weight of 65kg. Clinically, we know that weight loss during AIDS is not linear: in Stage I/II there 
    // is usually relatively slow weight loss, whereas in Stage IV weight loss can be rapid due to the 
    // compounding effects of malabsorption, difficulty eating/loss of appetite, the metabolic demands of
    // opportunistic infections and viral replication. This is why the rate of decline in weight increases
    // as the clinical stages advance.
    // -------------------------------------------------------------------------------------------------------


    // This is an implementation of the Cox Proportional Model (e.g., http://en.wikipedia.org/wiki/Cox_proportional_hazards_model)
    // The values are taken from the IeDEA website (http://www.iedea-sa.org/index.php?id=2856)
    float AntiretroviralTherapy::ComputeDurationFromEnrollmentToArtAidsDeath( IIndividualHumanContext* pPersonGoingOnArt,
                                                                 IIndividualHumanHIV* pPersonGoingOnArtHIV ) const
    {
        float whoStageContinuous = pPersonGoingOnArtHIV->GetHIVInfection()->GetWHOStage();
        float weight = GetWeightInKgFromWHOStage( whoStageContinuous );

        Gender::Enum gender = Gender::Enum(pPersonGoingOnArt->GetEventContext()->GetGender());
        float age_days = pPersonGoingOnArt->GetEventContext()->GetAge();
        uint32_t person_id = pPersonGoingOnArt->GetSuid().data;

        float cd4AtArtEnrollment = pPersonGoingOnArtHIV->GetHIVSusceptibility()->GetCD4count();
        if( cd4AtArtEnrollment < 30.0f )
        {
            LOG_WARN_F( "Individual %d had low CD4 at ART enrollment: %f\n", person_id, cd4AtArtEnrollment );
        }
        else if( cd4AtArtEnrollment > 850.0f )
        {
            LOG_WARN_F( "Individual %d had high CD4 at ART enrollment: %f\n", person_id, cd4AtArtEnrollment );
        }
        float multiplier = float( exp( m_CD4Slope * min( m_MaxCoxCD4, cd4AtArtEnrollment ) + m_CD4Intercept ) ); // Stop at 350
        release_assert( multiplier > 0.0f );
        //release_assert( multiplier < 10.0f );

        if( gender == Gender::FEMALE )
        {
            multiplier *= m_FemaleMultiplier;
        }

        if( age_days > AGE_40_YRS_IN_DAYS )
        {
            multiplier *= m_Over40Multipiler;
        }

        if( whoStageContinuous >= m_WhoStageCoxThreshold )
        {
            multiplier *= m_Stage3Multiplier;
        }

        multiplier *= float( exp( m_WeightSlope * weight + m_WeightIntercept ) );
        float lambda_divisor = pow( multiplier, 1.0f / m_WeibullShape );

        float lambda_corrected = m_WeibullScale / lambda_divisor;
        float ret = DAYSPERYEAR * lambda_corrected * pow( -log( 1.0f - pPersonGoingOnArt->GetRng()->e() ), 1.0f / m_WeibullShape );

        // This might be a little too verbose for production.
        LOG_DEBUG_F( "%s returning %f from WHO stage of %f, age of %f, weight of %f, and gender %s: multiplier was %f, lambda divisor = %f, lambda (actual) %f.\n",
                     __FUNCTION__,
                     ret,
                     whoStageContinuous,
                     age_days / DAYSPERYEAR,
                     weight,
                     (gender == Gender::MALE ? "MALE" : "FEMALE"),
                     multiplier,
                     lambda_divisor,
                     lambda_corrected
        );

        return ret;
    }

    REGISTER_SERIALIZABLE(AntiretroviralTherapy);

    void AntiretroviralTherapy::serialize(IArchive& ar, AntiretroviralTherapy* obj)
    {
        BaseIntervention::serialize( ar, obj );
        AntiretroviralTherapy& art = *obj;
        ar.labelElement( "m_MultiplierOnTransmission" ) & art.m_MultiplierOnTransmission;
        ar.labelElement( "m_IsActiveViralSuppression" ) & art.m_IsActiveViralSuppression;
        ar.labelElement( "m_DaysToAchieveSuppression" ) & art.m_DaysToAchieveSuppression;
        ar.labelElement( "m_MaxCoxCD4"                ) & art.m_MaxCoxCD4;
        ar.labelElement( "m_WeibullShape"             ) & art.m_WeibullShape;
        ar.labelElement( "m_WeibullScale"             ) & art.m_WeibullScale;
        ar.labelElement( "m_FemaleMultiplier"         ) & art.m_FemaleMultiplier;
        ar.labelElement( "m_Over40Multipiler"         ) & art.m_Over40Multipiler;
        ar.labelElement( "m_WhoStageCoxThreshold"     ) & art.m_WhoStageCoxThreshold;
        ar.labelElement( "m_Stage3Multiplier"         ) & art.m_Stage3Multiplier;
        ar.labelElement( "m_CD4Slope"                 ) & art.m_CD4Slope;
        ar.labelElement( "m_CD4Intercept"             ) & art.m_CD4Intercept;
        ar.labelElement( "m_WeightSlope"              ) & art.m_WeightSlope;
        ar.labelElement( "m_WeightIntercept"          ) & art.m_WeightIntercept;
    }
}
