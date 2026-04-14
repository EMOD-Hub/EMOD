
#include "stdafx.h"
#include <iomanip>
#include "InfectionHIV.h"

#include "Interventions.h"
#include "RANDOM.h"
#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanHIV.h"
#include "HIVInterventionsContainer.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "NodeEventContext.h"

#include "Types.h" // for ProbabilityNumber and NaturalNumber
#include "Debug.h" // for release_assert
#include "DistributionFactory.h"

SETUP_LOGGING( "InfectionHIV" )

namespace Kernel
{
#define INITIAL_VIRAL_LOAD (10000)

    // Initialization params here 
    float InfectionHIVConfig::acute_duration_in_months = 0.0f;
    float InfectionHIVConfig::AIDS_duration_in_months = 0.0f;
    float InfectionHIVConfig::acute_stage_infectivity_multiplier = 0.0f;
    float InfectionHIVConfig::AIDS_stage_infectivity_multiplier = 0.0f;
    float InfectionHIVConfig::personal_infectivity_scale = 0.0f;
    FerrandAgeDependentDistribution InfectionHIVConfig::mortality_distribution_by_age;
    IDistribution* InfectionHIVConfig::p_hetero_infectivity_distribution = nullptr;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(HIV.Infection,InfectionHIVConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionHIVConfig)
    END_QUERY_INTERFACE_BODY(InfectionHIVConfig)

    BEGIN_QUERY_INTERFACE_BODY(InfectionHIV)
        HANDLE_INTERFACE(IInfectionHIV)
    END_QUERY_INTERFACE_BODY(InfectionHIV)

    bool
    InfectionHIVConfig::Configure(
        const Configuration * config
    )
    {
        //read in configs here   
        initConfigTypeMap( "Acute_Duration_In_Months", &acute_duration_in_months, Acute_Duration_In_Months_DESC_TEXT, 0.0f, 10.0f, 2.9f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "AIDS_Duration_In_Months", &AIDS_duration_in_months, AIDS_Duration_In_Months_DESC_TEXT, 7.0f, 12.0f, 9.0f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "Acute_Stage_Infectivity_Multiplier", &acute_stage_infectivity_multiplier, Acute_Stage_Infectivity_Multiplier_DESC_TEXT, 1.0f, 100.0f, 26.0f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "AIDS_Stage_Infectivity_Multiplier", &AIDS_stage_infectivity_multiplier, AIDS_Stage_Infectivity_Multiplier_DESC_TEXT, 1.0f, 100.0f, 10.0f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "Heterogeneous_Infectiousness_LogNormal_Scale", &personal_infectivity_scale, Heterogeneous_Infectiousness_LogNormal_Scale_DESC_TEXT, 0.0f, 10.0f, 0.0f, "Simulation_Type", "HIV_SIM" );

        bool ret = JsonConfigurable::Configure( config );
        if( ret || JsonConfigurable::_dryrun )
        {
            ret = mortality_distribution_by_age.Configure( config );
            
            p_hetero_infectivity_distribution = DistributionFactory::CreateDistribution( DistributionFunction::LOG_NORMAL_DISTRIBUTION );
            
            // mu = -sigma^2/2
            const float personal_infectivity_mu = (-InfectionHIVConfig::personal_infectivity_scale * InfectionHIVConfig::personal_infectivity_scale / 2.0);
        
            // First argument is the median = exp(mu), where mu=-sigma^2/2, second arg is scale
            p_hetero_infectivity_distribution->SetParameters( personal_infectivity_mu, personal_infectivity_scale, 0.0 );
        }

        return ret ;
    }

    InfectionHIV *InfectionHIV::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionHIV *newinfection = _new_ InfectionHIV(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    void InfectionHIV::Initialize(suids::suid _suid)
    {
        InfectionSTI::Initialize(_suid);

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IndividualHuman" );
        }

        // Initialization of HIV infection members, start with constant-ish values
        ViralLoad = INITIAL_VIRAL_LOAD;

        m_has_been_suppressed = false;
        SetupNonSuppressedDiseaseTimers();

        SetupWouldHaveTimers();

        // calculate individual infectivity multiplier based on Log-Normal draw.
        m_hetero_infectivity_multiplier = 1;
        if (InfectionHIVConfig::personal_infectivity_scale > 0)
        {
            m_hetero_infectivity_multiplier = InfectionHIVConfig::p_hetero_infectivity_distribution->Calculate( parent->GetRng() );
        }

        LOG_DEBUG_F( "Individual %d just entered (started) HIV Acute stage, heterogeneity multiplier = %f.\n", parent->GetSuid().data, m_hetero_infectivity_multiplier );
    }

    void InfectionHIV::SetupWouldHaveTimers()
    {
        // save/start timers
        m_duration_until_would_have_entered_latent = m_acute_duration;                        // WouldHaveEnteredLatentStage
        m_duration_until_would_have_entered_aids   = m_acute_duration + m_latent_duration;    // WouldHaveHadAids
        m_duration_until_would_have_died           = HIV_duration_until_mortality_without_TB; // WouldHaveDied
    }

    void InfectionHIV::UpdateWouldHaveTimers( float dt )
    {
        m_duration_until_would_have_entered_latent -= dt;
        m_duration_until_would_have_entered_aids   -= dt;
        m_duration_until_would_have_died           -= dt;

        // ----------------------------------------------------------------------------
        // --- Only broadcast WouldHaveXXX events if the events would have happened
        // --- to the person if interventions like ART hadn't suppressed the infection.
        // --- Make sure not to broadcast the events if the person is in the process
        // --- of dying.  Also, don't broadcast the event if the duration is more than
        // --- dt old.  This means that the infection was fast forwarded and these
        // --- events occured in the past.
        // ----------------------------------------------------------------------------
        if( m_has_been_suppressed && (StateChange != InfectionStateChange::Fatal) ) // not going to die right now
        {
            IIndividualEventBroadcaster* p_broadcaster = nullptr;
            INodeEventContext* p_nec = parent->GetEventContext()->GetNodeEventContext();
            auto signal = EventTrigger::Births;

            if( !m_would_have_died && (-dt <= m_duration_until_would_have_died) && (m_duration_until_would_have_died <= 0.0) )
            {
                m_would_have_died = true;
                signal = EventTrigger::WouldHaveDied;
            }
            else if( !m_would_have_entered_aids && (-dt <= m_duration_until_would_have_entered_aids) && (m_duration_until_would_have_entered_aids <= 0.0) )
            {
                m_would_have_entered_aids = true;
                signal = EventTrigger::WouldHaveHadAIDS;
            }
            else if( !m_would_have_entered_latent && (-dt <= m_duration_until_would_have_entered_latent) && (m_duration_until_would_have_entered_latent <= 0.0) )
            {
                m_would_have_entered_latent = true;
                signal = EventTrigger::WouldHaveEnteredLatentStage;
            }
            if( p_nec != nullptr )
            {
                p_broadcaster = p_nec->GetIndividualEventBroadcaster();
                if( p_broadcaster && signal != EventTrigger::Births )
                {
                    p_broadcaster->TriggerObservers( parent->GetEventContext(), signal  );
                }
            }
        }
    }

    void InfectionHIV::SetContextTo( IIndividualHumanContext* context )
    {
        InfectionSTI::SetContextTo( context );

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IndividualHuman" );
        }
    }

#define WHO_STAGE_KAPPA_0 (0.9664f)
#define WHO_STAGE_KAPPA_1 (0.9916f)
#define WHO_STAGE_KAPPA_2 (0.9356f)
#define WHO_STAGE_LAMBDA_0 (0.26596f)
#define WHO_STAGE_LAMBDA_1 (0.19729f)
#define WHO_STAGE_LAMBDA_2 (0.34721f)

    void
    InfectionHIV::SetupNonSuppressedDiseaseTimers()
    {
        try
        {
            // These two constants will both need to be configurable in next checkin.
            m_acute_duration    = InfectionHIVConfig::acute_duration_in_months * DAYSPERYEAR / float(MONTHSPERYEAR);
            m_aids_duration     = InfectionHIVConfig::AIDS_duration_in_months * DAYSPERYEAR / float(MONTHSPERYEAR);

            IIndividualHumanEventContext* HumanEventContextWhoKnowsItsAge  = parent->GetEventContext();
            float age_at_HIV_infection = HumanEventContextWhoKnowsItsAge->GetAge();
            // |------------|----------------|--------------*
            //   (acute)         (latent)        (aids)     (death)
            // Note that these two 'timers' are apparently identical at this point. Maybe HIV-xxx is redundant?
            HIV_duration_until_mortality_without_TB = InfectionHIVConfig::mortality_distribution_by_age.invcdf( parent->GetRng()->e(), age_at_HIV_infection );
            //HIV_duration_until_mortality_without_TB /= 2; // test hack
            infectious_timer = HIV_duration_until_mortality_without_TB;
            NO_LESS_THAN( HIV_duration_until_mortality_without_TB, (float)DAYSPERWEEK ); // no less than 7 days prognosis
            NO_MORE_THAN( m_acute_duration, HIV_duration_until_mortality_without_TB ); // Acute stage time can't be longer than total prognosis!
            NO_MORE_THAN( m_aids_duration, HIV_duration_until_mortality_without_TB-m_acute_duration ); // AIDS can't be longer than prognosis - acute
            NO_LESS_THAN( m_aids_duration, 0.0f ); // can't be negative either!
            // Latent gets what's left
            m_latent_duration  = HIV_duration_until_mortality_without_TB - m_acute_duration - m_aids_duration;
            NO_LESS_THAN( m_latent_duration, 0.0f );  // days -- can't be negative
            LOG_DEBUG_F( "HIV Infection initialized for individual %d with prognosis %f, acute_duration %f, latent_duration %f, and aids_duration %f (all in years)\n", 
                         parent->GetSuid().data, HIV_duration_until_mortality_without_TB/DAYSPERYEAR, m_acute_duration/DAYSPERYEAR, m_latent_duration/DAYSPERYEAR, m_aids_duration/DAYSPERYEAR );

            // Pre-calculate for WHO stage: ///////////////////////////////////////////////////////////////
            float kappas [ NUM_WHO_STAGES-1 ]    = { WHO_STAGE_KAPPA_0, WHO_STAGE_KAPPA_1, WHO_STAGE_KAPPA_2 };
            float lambdas[ NUM_WHO_STAGES-1 ]    = { WHO_STAGE_LAMBDA_0, WHO_STAGE_LAMBDA_1, WHO_STAGE_LAMBDA_2 };

            float remaining_fraction = 1.0f;
            for(int stage=0; stage<NUM_WHO_STAGES-1; ++stage)      // NOTE: stage=0 indicates WHO stage 1 (and so forth)
            {
                // Note that these "fractions" do not add to 1.  In particular, any one value could be greater than 1, indicating that 
                // the individual never leaves this WHO stage.
                m_fraction_of_prognosis_spent_in_stage[ stage ] = parent->GetRng()->Weibull(lambdas[stage], kappas[stage]); //  lambdas[stage]*pow(-log(1-parent->GetRng()->e()), 1/kappas[stage]);
                remaining_fraction -= m_fraction_of_prognosis_spent_in_stage[ stage ];

                release_assert( m_fraction_of_prognosis_spent_in_stage[ stage ] != 0.0 ); // used as a divisor later so can't allow to be zero
            }
            //NO_LESS_THAN( remaining_fraction, 0 );

            m_fraction_of_prognosis_spent_in_stage[ NUM_WHO_STAGES-1 ] = std::max<float>( 0.0f, float(remaining_fraction) );

            std::ostringstream msg;
            for( int idx=0; idx<=NUM_WHO_STAGES-1; idx++ )
            {
                msg << "m_fraction_of_prognosis_spent_in_stage[" << idx << "] = " << m_fraction_of_prognosis_spent_in_stage[idx] << " - ";
            }
            msg << std::endl;
            LOG_DEBUG_F( msg.str().c_str() );
            duration = 0.0f;
            ///////////////////////////////////////////////////////////////////////////////////////////////
            // SCHEDULE NATURAL DISEASE PROGRESSION, TODO put parameters into config //////////////////////
            SetStage( HIVInfectionStage:: ACUTE );
        }
        catch( DetailedException &exc )
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, exc.GetMsg() );
        }
    }

    void
    InfectionHIV::SetupSuppressedDiseaseTimers( float durationFromEnrollmentToArtAidsDeath )
    {
        m_has_been_suppressed = true;
        m_acute_duration  = INACTIVE_DURATION;
        m_latent_duration = INACTIVE_DURATION;
        m_aids_duration   = INACTIVE_DURATION;

        // store HIV (relative) prognosis just in case
        HIV_natural_duration_until_mortality = HIV_duration_until_mortality_without_TB;
        // Clear HIV natural duration so it's clear we're not using this
        HIV_duration_until_mortality_without_TB = INACTIVE_DURATION;
        // Set our new prognosis based on being on ART
        HIV_duration_until_mortality_with_viral_suppression = durationFromEnrollmentToArtAidsDeath;
        // There are redundant variables that now need to be set also (TBD: let's use just 1 soon)
        total_duration = HIV_duration_until_mortality_with_viral_suppression;

        // This is risky. Reset our infection duration so other code works But we have lost our history.
        // Maybe store duration in another variable?
        duration = 0;
        LOG_DEBUG_F( "Replaced hiv natural mortality timer of %f with ART-based mortality timer of %f for individual %d\n",
                     HIV_natural_duration_until_mortality,
                     HIV_duration_until_mortality_with_viral_suppression,
                     parent->GetSuid().data
                   );
        SetStage( HIVInfectionStage::ON_ART );
    }

    void InfectionHIV::SetParameters( const IStrainIdentity* infstrain, int incubation_period_override )
    {
        LOG_DEBUG_F( "New HIV infection for individual %d; incubation_period_override = %d.\n", parent->GetSuid().data, incubation_period_override );
        // Don't call down into baseclass. Copied two lines below to repro required functionality.
        incubation_timer = incubation_period_override;
        CreateInfectionStrain(infstrain);

        if( incubation_period_override == 0 )
        {
            // This means we're part of an outbreak. Fastforward infection. 
            float fast_forward = HIV_duration_until_mortality_without_TB * parent->GetRng()->e(); // keep this as member?
            duration += fast_forward;
            m_time_infected -= fast_forward;    // Move infection time backwards
            hiv_parent->GetHIVSusceptibility()->FastForward( this, fast_forward );

            // The infection is not suppressed so WouldHaveXXX events would not fire
            // so just reduct timers.
            m_duration_until_would_have_entered_latent -= fast_forward;
            m_duration_until_would_have_entered_aids   -= fast_forward;
            m_duration_until_would_have_died           -= fast_forward;

            LOG_DEBUG_F( "Individual is outbreak seed, fast forward infection by %f.\n", fast_forward );

            SetStageFromDuration();
        }

        total_duration = HIV_duration_until_mortality_without_TB;
        // now we have 3 variables doing the same thing?
        infectiousness = InfectionConfig::base_infectivity;
        StateChange    = InfectionStateChange::None;
    }

    void InfectionHIV::SetStageFromDuration()
    {
        // DJK*: This function updates stage rather than sets it.  
        //  E.g. if someone is in ACUTE or ON_ART stage, their stage is unchanged
        // DJK*: Note, this function _also_ sets StateChange to Fatal.

        LOG_DEBUG_F( "Individual %d, duration = %f, prognosis = %f, m_infection_stage = %d\n",
                     parent->GetSuid().data,
                     duration,
                     GetPrognosis()/DAYSPERYEAR,
                     m_infection_stage );

        if (duration >= GetPrognosis() )
        {
            StateChange = InfectionStateChange::Fatal;
            LOG_DEBUG_F( "Individual %d just died of HIV in stage %s with CD4 count of %f.\n",
                         parent->GetSuid().data,
                         HIVInfectionStage::pairs::lookup_key(m_infection_stage),
                         hiv_parent->GetHIVSusceptibility()->GetCD4count() );
        }
        else if ( ((m_infection_stage == HIVInfectionStage::ACUTE) || (m_infection_stage == HIVInfectionStage::LATENT)) &&
                  (duration >= (m_acute_duration + m_latent_duration)) )
        {
            SetStage( HIVInfectionStage::AIDS );
        }
        else if( (m_infection_stage == HIVInfectionStage::ACUTE) && (duration >= m_acute_duration) )
        {
            SetStage( HIVInfectionStage::LATENT );
        }
    }

    void InfectionHIV::ApplySuppressionDropout()
    {
        LOG_DEBUG_F( "Looks like ART dropout for %d, recalc all timers for non-ART HIV progression.\n", parent->GetSuid().data );
        // draw new prognosis
        SetupNonSuppressedDiseaseTimers();
        // But we have to fast-forward based on current CD4. 
        //
        // Let's say our individual starts with CD4=550 and a prognosis of 15 years, with CD4=50 at death.
        // Then he drops to CD4=350 over time, then gets ART, then ART brings CD4 up to, say, 450 before drop out.
        // Then we draw a new prognosis, say 12 years. But we "fast-forward" that 12 years based on their current 
        // CD4 count of 450. And we figure out how much percentage to fast-forward based on the fact that 450 is 
        // 20% of the way from from 550 to 50, so we fast-forward 20% of 12 years or 2.4 years, for a resulting 
        // prognosis of 9.6 years.
        ProbabilityNumber fraction_prognosis_completed = hiv_parent->GetHIVSusceptibility()->GetPrognosisCompletedFraction();
        hiv_parent->GetHIVSusceptibility()->TerminateSuppression( GetPrognosis() * (1-fraction_prognosis_completed) );

        HIV_duration_until_mortality_without_TB *= (1.0 - fraction_prognosis_completed); // if 0 completed, prog is unchanged. If 1.0 completed, prog = 0.
        infectious_timer = HIV_duration_until_mortality_without_TB;
        total_duration = HIV_duration_until_mortality_without_TB;
        m_acute_duration = 0;   // DJK*: With this, acute+latent+AIDS durations will not sum to prognosis!

        // Initially, set stage to LATENT, but then possibly
        // switch to AIDS if the latent duration has expired
        SetStage( HIVInfectionStage::LATENT );
        SetStageFromDuration();

        LOG_DEBUG_F( "Post-dropout prognosis for individual %d = %f\n",
                     parent->GetSuid().data,
                     HIV_duration_until_mortality_without_TB/DAYSPERYEAR );
    }

    void InfectionHIV::ApplySuppressionFailure()
    {
        LOG_DEBUG_F( "Looks like ART failure for %d, recalc timers for AIDS-stage progression.\n", parent->GetSuid().data );
        // Should be just 9 months to live. Timers shouldn't need to be reset.
        SetStage( HIVInfectionStage::AIDS );
    }

    void InfectionHIV::Update( float currenttime, float dt, ISusceptibilityContext* immunity )
    {
        InfectionSTI::Update( currenttime, dt, immunity );

        SetStageFromDuration();

        // update viral load per timestep
        ViralLoad = INITIAL_VIRAL_LOAD; 

        ApplyDrugEffects(dt, immunity);

        // TODO: there aren't any strains yet, but any differences between strains 
        // (especially drug resistance) can be handled this way
        // EvolveStrain(immunity, dt);

        // Make sure we didn't accidentally clear.  Remove this line for sterilizing cure.
        release_assert( StateChange != InfectionStateChange::Cleared );

        UpdateWouldHaveTimers( dt );
    }

    float
    InfectionHIV::GetInfectiousness() const
    {
        float retInf = InfectionSTI::GetInfectiousness();
        LOG_DEBUG_F( "infectiousness from STI = %f\n", retInf );
        // TBD: a STATE is not an EVENT. Split up this enum.
        if( m_infection_stage == HIVInfectionStage::ACUTE )
        {
            retInf *= InfectionHIVConfig::acute_stage_infectivity_multiplier; //  26.0f;
        }
        else if( m_infection_stage == HIVInfectionStage::AIDS )
        {
            retInf *= InfectionHIVConfig::AIDS_stage_infectivity_multiplier;
        }

        // ART reduces infectivity, but we don't want to put ART-specific knowledge and code in the infection object.
        // Prefer instead to apply the _effects_ of art, and prefer to keep all ART knowledge encapsulated inside
        // the HIVInterventionsContainer.
        // TBD: Use QI instead of cast
        IHIVInterventionsContainer * pHIC = nullptr;
        if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVInterventionsContainer), (void**)&pHIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IHIVInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }
        retInf *= pHIC->GetInfectivitySuppression();
        retInf *= m_hetero_infectivity_multiplier;
        
        LOG_DEBUG_F( "infectiousness from HIV = %f\n", retInf );
        return retInf;
    }

    NaturalNumber InfectionHIV::GetViralLoad()
    const
    {
        return ViralLoad;
    }

    float
    InfectionHIV::GetPrognosis()
    const
    {
        // These may be the WRONG tests. Timers can be sub-zero but really one should die immediately after that update occurs.
        // Works now but possibly very sensitive to code flow that could change.
        if( HIV_duration_until_mortality_without_TB >= 0.0f )
        {
            return HIV_duration_until_mortality_without_TB;
        }
        else if( HIV_duration_until_mortality_with_viral_suppression >= 0.0f )
        {
            return HIV_duration_until_mortality_with_viral_suppression;
        }
        else
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "No valid mortality timers!" );
        }
    }


    float InfectionHIV::GetTimeInfected() const
    {
        return m_time_infected;
    }

    float InfectionHIV::GetDaysTillDeath() const
    {
        return GetPrognosis() - duration;
    }

    const HIVInfectionStage::Enum&
    InfectionHIV::GetStage()
    const
    {
        return m_infection_stage;
    }

    void InfectionHIV::SetStage( HIVInfectionStage::Enum stage )
    {
        m_infection_stage = stage;

        EventTrigger stage_trigger;
        switch( m_infection_stage )
        {
            case HIVInfectionStage::ACUTE:
                // do nothing
                break;

            case HIVInfectionStage::LATENT:
                stage_trigger = EventTrigger::HIVInfectionStageEnteredLatent;
                break;

            case HIVInfectionStage::AIDS:
                stage_trigger = EventTrigger::HIVInfectionStageEnteredAIDS;
                break;

            case HIVInfectionStage::ON_ART:
                stage_trigger = EventTrigger::HIVInfectionStageEnteredOnART;
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "m_infection_stage",
                                                         m_infection_stage,
                                                         HIVInfectionStage::pairs::lookup_key( m_infection_stage ) );
        }

        if( !stage_trigger.IsUninitialized() )
        {
            INodeEventContext* p_nec = parent->GetEventContext()->GetNodeEventContext();
            IIndividualEventBroadcaster* p_broadcaster = p_nec->GetIndividualEventBroadcaster();

            p_broadcaster->TriggerObservers( parent->GetEventContext(), stage_trigger );

            LOG_DEBUG_F( "Individual %d just entered of HIV %s stage.\n",
                         parent->GetSuid().data,
                         HIVInfectionStage::pairs::lookup_key( m_infection_stage ) );
        }
    }

    bool InfectionHIV::ApplyDrugEffects(float dt, ISusceptibilityContext* immunity)
    {
        // Check for valid inputs
        if (dt <= 0 || !immunity) 
        {
            LOG_WARN("ApplyDrugEffects takes the following arguments: time step (dt), Susceptibility (immunity)\n");
            return false;
        }

        // Query for drug effects
        IIndividualHumanContext *patient = GetParent();
        IHIVDrugEffects *ihivde = nullptr;

        if (!patient->GetInterventionsContext()) 
        {
            if ( s_OK != patient->GetInterventionsContextbyInfection(this)->QueryInterface(GET_IID(IHIVDrugEffects), (void**)&ihivde) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContextbyInfection()", "IHIVDrugEffects", "IIndividualHumanInterventionsContext" );
            }
        }
        else
        {
            if ( s_OK != patient->GetInterventionsContext()->QueryInterface(GET_IID(IHIVDrugEffects), (void **)&ihivde) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IHIVDrugEffects", "IIndividualHumanInterventionsContext" );
            }
        }

        // float HIV_drug_inactivation_rate = ihivde->GetDrugInactivationRate(); //no action for Drug Inactivation for now
        float HIV_drug_clearance_rate    = ihivde->GetDrugClearanceRate();

        if( parent->GetRng()->SmartDraw( dt * HIV_drug_clearance_rate ) )
        {
            // Infection cleared. InfectionStateChange reporting (in SetNewInfectionState) is TB specific, but this allows us to delete the HIV infections if we get drugs and are cleared
            StateChange = InfectionStateChange::Cleared;
            LOG_DEBUG("HIV drugs cured my HIV infection \n");
            return true;
        }

        // Otherwise drug did not have an effect in this timestep
        return false;
    }

    InfectionHIV::~InfectionHIV(void) { }
    InfectionHIV::InfectionHIV()
        : ViralLoad(0)
        , HIV_duration_until_mortality_without_TB(INACTIVE_DURATION )
        , HIV_natural_duration_until_mortality(INACTIVE_DURATION ) // need some value that says Ignore-Me
        , HIV_duration_until_mortality_with_viral_suppression(INACTIVE_DURATION )
        , m_time_infected ( 0 )
        , m_infection_stage( HIVInfectionStage::ACUTE )
        //, m_fraction_of_prognosis_spent_in_stage[ NUM_WHO_STAGES ]
        , m_acute_duration( 0.0 )
        , m_latent_duration( 0.0 )
        , m_aids_duration( 0.0 )
        , m_duration_until_would_have_entered_latent( 0.0 )
        , m_duration_until_would_have_entered_aids( 0.0 )
        , m_duration_until_would_have_died( 0.0 )
        , m_would_have_entered_latent( false )
        , m_would_have_entered_aids( false )
        , m_would_have_died( false )
        , m_has_been_suppressed( false )
        , m_hetero_infectivity_multiplier( 0.0 )
        , hiv_parent( nullptr )
    {
    }

    InfectionHIV::InfectionHIV(IIndividualHumanContext *context)
        : InfectionSTI(context)
        , HIV_duration_until_mortality_without_TB(INACTIVE_DURATION )
        , HIV_natural_duration_until_mortality(INACTIVE_DURATION ) // need some value that says Ignore-Me
        , HIV_duration_until_mortality_with_viral_suppression(INACTIVE_DURATION )
        , m_time_infected( 0 )
        , m_infection_stage( HIVInfectionStage::ACUTE )
        //, m_fraction_of_prognosis_spent_in_stage[ NUM_WHO_STAGES ]
        , m_acute_duration( 0.0 )
        , m_latent_duration( 0.0 )
        , m_aids_duration( 0.0 )
        , m_duration_until_would_have_entered_latent( 0.0 )
        , m_duration_until_would_have_entered_aids( 0.0 )
        , m_duration_until_would_have_died( 0.0 )
        , m_would_have_entered_latent( false )
        , m_would_have_entered_aids( false )
        , m_would_have_died( false )
        , m_has_been_suppressed( false )
        , m_hetero_infectivity_multiplier( 0.0 )
        , hiv_parent( nullptr )
    {
        // TODO: Consider moving m_time_infected to Infection layer.  Alternatively, track only in reporters.
        m_time_infected = parent->GetEventContext()->GetIndividualHumanConst()->GetParent()->GetTime().time;
    }

    float
    InfectionHIV::GetWHOStage()
    const
    {
        // DJK TODO: Needs to return float that interpolates stages, e.g. for weight.  partial not used. <ERAD-1860>
        float ret = MAX_WHO_HIV_STAGE-1;
        double fractionOfPrognosisCompleted = hiv_parent->GetHIVSusceptibility()->GetPrognosisCompletedFraction();
        double cum_stage = 0;        // Could be greater than one
        double partial = 0;
        // |__________________|
        //   1  ^ 2 ^  3 ^  4
        for( NaturalNumber stage=MIN_WHO_HIV_STAGE; // from 1
             stage < MAX_WHO_HIV_STAGE-1; // to 3
             stage++)
        {
            NaturalNumber stage_idx = stage-1;
            cum_stage += m_fraction_of_prognosis_spent_in_stage[ stage_idx ];
            if( cum_stage >= fractionOfPrognosisCompleted )
            {
                auto partial_raw = ( fractionOfPrognosisCompleted - ( cum_stage - m_fraction_of_prognosis_spent_in_stage[ stage_idx ] ) ) /m_fraction_of_prognosis_spent_in_stage[ stage_idx ];
                if( partial_raw > 1.0f )
                {
                    std::ostringstream msg;
                    msg << "partial_raw calculated to greater than 1.0f: " << std::setprecision(12) << partial_raw << " with divisor " << m_fraction_of_prognosis_spent_in_stage[ stage_idx ] << std::endl;
                    LOG_WARN_F( msg.str().c_str() );
                    partial_raw = 1.0f;
                }
                partial = partial_raw;
                ret = stage; // max 2
                break;
            }
            release_assert( int(stage_idx) < NUM_WHO_STAGES-1 );
            //LOG_DEBUG_F( "cum_stage now = %f after adding %f because fraction_completed = %f, stage_idx = %d\n", (float) cum_stage, (float) m_fraction_of_prognosis_spent_in_stage[ stage_idx ], (float) fractionOfPrognosisCompleted, (int) stage_idx );
        }
        
        ret += partial;

#if 0
        std::ostringstream msg;
        msg << "Looks like we are in WHO stage "
            << (int) ret 
            << " with remainder "
            << (float) partial
            << " based on prognosis fraction of "
            << (float) fractionOfPrognosisCompleted
            << " and stage fractions/cum fractions of: ";
        float cum = 0.0f;
        for( int idx=0; idx<3; idx++ )
        {
            msg << m_fraction_of_prognosis_spent_in_stage[idx] << "/";
            cum += m_fraction_of_prognosis_spent_in_stage[idx];
            msg << cum << " -- ";
        }
        msg << std::endl;
        //LOG_DEBUG_F( "Looks like we are in WHO stage %d with remainder %f based on prognosis fraction of %f.\n", (int) stage, (float) partial, (float) fractionOfPrognosisCompleted );
        LOG_DEBUG_F( msg.str().c_str() );
#endif
        LOG_DEBUG_F( "%s returning %f for individual %d\n", __FUNCTION__, ret, parent->GetSuid().data );
        return ret;
    }

    REGISTER_SERIALIZABLE(InfectionHIV);

    void InfectionHIV::serialize(IArchive& ar, InfectionHIV* obj)
    {
        InfectionSTI::serialize( ar, obj );
        InfectionHIV& inf_hiv = *obj;
        ar.labelElement("ViralLoad"                                          ) & inf_hiv.ViralLoad;
        ar.labelElement("HIV_duration_until_mortality_without_TB"            ) & inf_hiv.HIV_duration_until_mortality_without_TB;
        ar.labelElement("HIV_natural_duration_until_mortality"               ) & inf_hiv.HIV_natural_duration_until_mortality;
        ar.labelElement("HIV_duration_until_mortality_with_viral_suppression") & inf_hiv.HIV_duration_until_mortality_with_viral_suppression;
        ar.labelElement("m_time_infected"                                    ) & inf_hiv.m_time_infected;
        ar.labelElement("m_infection_stage"                                  ) & (uint32_t&)inf_hiv.m_infection_stage;
        ar.labelElement("m_fraction_of_prognosis_spent_in_stage"             ); ar.serialize( inf_hiv.m_fraction_of_prognosis_spent_in_stage, NUM_WHO_STAGES );
        ar.labelElement("m_acute_duration"                                   ) & inf_hiv.m_acute_duration;
        ar.labelElement("m_latent_duration"                                  ) & inf_hiv.m_latent_duration;
        ar.labelElement("m_aids_duration"                                    ) & inf_hiv.m_aids_duration;
        ar.labelElement("m_duration_until_would_have_entered_latent"         ) & inf_hiv.m_duration_until_would_have_entered_latent;
        ar.labelElement("m_duration_until_would_have_entered_aids"           ) & inf_hiv.m_duration_until_would_have_entered_aids;
        ar.labelElement("m_duration_until_would_have_died"                   ) & inf_hiv.m_duration_until_would_have_died;
        ar.labelElement("m_would_have_entered_latent"                        ) & inf_hiv.m_would_have_entered_latent;
        ar.labelElement("m_would_have_entered_aids"                          ) & inf_hiv.m_would_have_entered_aids;
        ar.labelElement("m_would_have_died"                                  ) & inf_hiv.m_would_have_died;
        ar.labelElement("m_has_been_suppressed"                              ) & inf_hiv.m_has_been_suppressed;
        ar.labelElement("m_hetero_infectivity_multiplier"                    ) & inf_hiv.m_hetero_infectivity_multiplier;

        //hiv_parent assigned in SetContextTo()
    }
}
