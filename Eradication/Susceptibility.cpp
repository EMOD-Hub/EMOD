
#include "stdafx.h"

#include "Environment.h"
#include "Susceptibility.h"
#include "SimulationConfig.h"
#include "RapidJsonImpl.h"
#include "Common.h"
#include "IIndividualHumanContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "Susceptibility" )

namespace Kernel
{
    // Maternal protection
    MaternalProtectionType::Enum SusceptibilityConfig::maternal_protection_type = MaternalProtectionType::NONE;
    bool  SusceptibilityConfig::maternal_protection = false;
    float SusceptibilityConfig::matlin_slope        = 1.0f;
    float SusceptibilityConfig::matlin_suszero      = 1.0f;
    float SusceptibilityConfig::matsig_steepfac     = 1.0f;
    float SusceptibilityConfig::matsig_halfmax      = 1.0f;
    float SusceptibilityConfig::matsig_susinit      = 1.0f;

    // Post-infection immunity
    float SusceptibilityConfig::baseacqupdate  = 1.0f;
    float SusceptibilityConfig::basetranupdate = 1.0f;
    float SusceptibilityConfig::basemortupdate = 1.0f;

    // Decay of post-infection immunity
    bool SusceptibilityConfig::enable_immune_decay = false;
    float SusceptibilityConfig::acqdecayrate       = 0.0f;
    float SusceptibilityConfig::trandecayrate      = 0.0f;
    float SusceptibilityConfig::mortdecayrate      = 0.0f;
    float SusceptibilityConfig::baseacqoffset      = 0.0f;
    float SusceptibilityConfig::basetranoffset     = 0.0f;
    float SusceptibilityConfig::basemortoffset     = 0.0f;

    SusceptibilityType::Enum SusceptibilityConfig::susceptibility_type = SusceptibilityType::FRACTIONAL;

    // Initialization of susceptibility at simulation start
    bool                   SusceptibilityConfig::enable_initial_susceptibility_distribution      = false;
    DistributionType::Enum SusceptibilityConfig::susceptibility_initialization_distribution_type = DistributionType::DISTRIBUTION_OFF;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Susceptibility,SusceptibilityConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityConfig)

    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_BODY(Susceptibility)
        HANDLE_INTERFACE(ISusceptibilityContext)
        HANDLE_ISUPPORTS_VIA(ISusceptibilityContext)
    END_QUERY_INTERFACE_BODY(Susceptibility)

    bool SusceptibilityConfig::Configure(const Configuration* config)
    {
        // Infection derived immunity
        // Maximum values were revised down to 1.0 from 1000.0. Values > 1.0 imply greater than 
        // baseline susceptibility post infection, which is almost certainly not the desired behavior.
        // If that IS the desired behavior, then consider a disease specific build.
        initConfigTypeMap( "Post_Infection_Acquisition_Multiplier",  &baseacqupdate,  Post_Infection_Acquisition_Multiplier_DESC_TEXT,  0.0f, 1.0f, 0.0f, "Enable_Immunity" );
        initConfigTypeMap( "Post_Infection_Transmission_Multiplier", &basetranupdate, Post_Infection_Transmission_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.0f, "Enable_Immunity" );
        initConfigTypeMap( "Post_Infection_Mortality_Multiplier",    &basemortupdate, Post_Infection_Mortality_Multiplier_DESC_TEXT,    0.0f, 1.0f, 0.0f, "Enable_Immunity" );

        // Decay of infection derived immunity
        // Maximum values were revised down to 1.0 from 1000.0. Values > 1.0 imply waning rate >100%/day.
        // If that IS the desired behavior, consider turning off immunity.
        initConfigTypeMap( "Enable_Immune_Decay", &enable_immune_decay, Enable_Immune_Decay_DESC_TEXT, true, "Enable_Immunity");
        initConfigTypeMap( "Acquisition_Blocking_Immunity_Decay_Rate",  &acqdecayrate,  Acquisition_Blocking_Immunity_Decay_Rate_DESC_TEXT,  0.0f, 1.0f, 0.001f, "Enable_Immune_Decay" );
        initConfigTypeMap( "Transmission_Blocking_Immunity_Decay_Rate", &trandecayrate, Transmission_Blocking_Immunity_Decay_Rate_DESC_TEXT, 0.0f, 1.0f, 0.001f, "Enable_Immune_Decay" );
        initConfigTypeMap( "Mortality_Blocking_Immunity_Decay_Rate",    &mortdecayrate, Mortality_Blocking_Immunity_Decay_Rate_DESC_TEXT,    0.0f, 1.0f, 0.001f, "Enable_Immune_Decay" );
        initConfigTypeMap( "Acquisition_Blocking_Immunity_Duration_Before_Decay",  &baseacqoffset,  Acquisition_Blocking_Immunity_Duration_Before_Decay_DESC_TEXT,  0.0f, MAX_HUMAN_LIFETIME, 0.0f, "Enable_Immune_Decay" );
        initConfigTypeMap( "Transmission_Blocking_Immunity_Duration_Before_Decay", &basetranoffset, Transmission_Blocking_Immunity_Duration_Before_Decay_DESC_TEXT, 0.0f, MAX_HUMAN_LIFETIME, 0.0f, "Enable_Immune_Decay" );
        initConfigTypeMap( "Mortality_Blocking_Immunity_Duration_Before_Decay",    &basemortoffset, Mortality_Blocking_Immunity_Duration_Before_Decay_DESC_TEXT,    0.0f, MAX_HUMAN_LIFETIME, 0.0f, "Enable_Immune_Decay" );

        // Maternal protection options (does not require infection derived immunity; does not decay)
        initConfigTypeMap("Enable_Maternal_Protection", &maternal_protection, Enable_Maternal_Protection_DESC_TEXT, false, "Enable_Birth");
        initConfig("Maternal_Protection_Type", maternal_protection_type, config, MetadataDescriptor::Enum("Maternal_Protection_Type", Maternal_Protection_Type_DESC_TEXT, MDD_ENUM_ARGS(MaternalProtectionType)),"Enable_Maternal_Protection");
        initConfigTypeMap("Maternal_Linear_Slope",       &matlin_slope,    Maternal_Linear_Slope_DESC_TEXT,          0.0001f, 1.0f,   0.01f, "Maternal_Protection_Type", "LINEAR");
        initConfigTypeMap("Maternal_Linear_SusZero",     &matlin_suszero,  Maternal_Linear_SusZero_DESC_TEXT,        0.0f,    1.0f,   0.2f,  "Maternal_Protection_Type", "LINEAR");
        initConfigTypeMap("Maternal_Sigmoid_SteepFac",   &matsig_steepfac, Maternal_Sigmoid_SteepFac_DESC_TEXT,      0.1f, 1000.0f,  30.0f,  "Maternal_Protection_Type", "SIGMOID");
        initConfigTypeMap("Maternal_Sigmoid_HalfMaxAge", &matsig_halfmax,  Maternal_Sigmoid_HalfMaxAge_DESC_TEXT, -270.0f, 3650.0f, 180.0f,  "Maternal_Protection_Type", "SIGMOID");
        initConfigTypeMap("Maternal_Sigmoid_SusInit",    &matsig_susinit,  Maternal_Sigmoid_SusInit_DESC_TEXT,       0.0f,    1.0f,   0.0f,  "Maternal_Protection_Type", "SIGMOID");

        // Implementation of an individual's susceptibility
        // Currently (May2018) implemented for maternal protection only, but other functionality is expected to follow.
        initConfig( "Susceptibility_Type", susceptibility_type, config, MetadataDescriptor::Enum("Susceptibility_Type", Susceptibility_Type_DESC_TEXT, MDD_ENUM_ARGS(SusceptibilityType)), "Enable_Maternal_Protection");

        initConfigTypeMap( "Enable_Initial_Susceptibility_Distribution", &enable_initial_susceptibility_distribution, Enable_Initial_Susceptibility_Distribution_DESC_TEXT, false, "Enable_Immunity" );
        initConfig( "Susceptibility_Initialization_Distribution_Type",
                    susceptibility_initialization_distribution_type,
                    config,
                    MetadataDescriptor::Enum("Susceptibility_Initialization_Distribution_Type", Susceptibility_Initialization_Distribution_Type_DESC_TEXT, MDD_ENUM_ARGS(DistributionType)),
                    "Enable_Initial_Susceptibility_Distribution");

        bool bRet = JsonConfigurable::Configure( config );

        return bRet;
    }


    Susceptibility::Susceptibility()
        : mod_acquire( 0.0f )
        , mod_transmit( 0.0f )
        , mod_mortality( 0.0f )
        , age ( 0.0f )   // REMOVE NEXT VERSION
        , acqdecayoffset( 0.0f )
        , trandecayoffset( 0.0f )
        , mortdecayoffset( 0.0f )
        , immune_failage( 0.0f )
        , parent(nullptr)
    { }

    Susceptibility::Susceptibility(IIndividualHumanContext* context)
        : mod_acquire( 0.0f )
        , mod_transmit( 0.0f )
        , mod_mortality( 0.0f )
        , age( 0.0f )   // REMOVE NEXT VERSION
        , acqdecayoffset( 0.0f )
        , trandecayoffset( 0.0f )
        , mortdecayoffset( 0.0f )
        , immune_failage( 0.0f )
        , parent(context)
    { }

    Susceptibility::~Susceptibility()
    {
    }

    Susceptibility* Susceptibility::CreateSusceptibility(IIndividualHumanContext* context, float immmod, float riskmod)
    {
        Susceptibility* newsusceptibility = _new_ Susceptibility(context);
        newsusceptibility->Initialize(immmod, riskmod);

        return newsusceptibility;
    }

    void Susceptibility::Initialize(float immmod, float riskmod)
    {
        // immune modifiers
        mod_acquire   = immmod;
        mod_transmit  = 1.0f;
        mod_mortality = 1.0f;

        if(SusceptibilityConfig::maternal_protection && SusceptibilityConfig::susceptibility_type == SusceptibilityType::BINARY)
        {
            float rDraw = parent->GetRng()->e();

            if(SusceptibilityConfig::maternal_protection_type == MaternalProtectionType::LINEAR)
            {
                if (rDraw == 0.0f)
                {
                    immune_failage = 0.0f;
                }
                else
                {
                    immune_failage = (rDraw-SusceptibilityConfig::matlin_suszero)
                                           /SusceptibilityConfig::matlin_slope;
                }
            }

            else if(SusceptibilityConfig::maternal_protection_type == MaternalProtectionType::SIGMOID)
            {
                if (rDraw <= SusceptibilityConfig::matsig_susinit)
                {
                    immune_failage = 0.0f;
                }
                else
                {
                    // The value 0.001f is a small positive constant that avoids division by zero.
                    immune_failage = SusceptibilityConfig::matsig_halfmax -
                                       SusceptibilityConfig::matsig_steepfac*
                                       log(( 1.0f-SusceptibilityConfig::matsig_susinit)/
                                           (rDraw-SusceptibilityConfig::matsig_susinit)-1.0f+0.001f);
                }
            }
        }
    }

    void Susceptibility::SetContextTo(IIndividualHumanContext* context)
    {
        parent = context;
    }

    IIndividualHumanContext* Susceptibility::GetParent()
    {
        return parent;
    }

    const SimulationConfig* Susceptibility::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    float Susceptibility::getModAcquire() const
    {
        float susceptibility_correction = 1.0f;

        if(SusceptibilityConfig::maternal_protection && SusceptibilityConfig::susceptibility_type == SusceptibilityType::FRACTIONAL)
        {
            if(SusceptibilityConfig::maternal_protection_type == MaternalProtectionType::LINEAR)
            {
                susceptibility_correction *= SusceptibilityConfig::matlin_slope*parent->GetAge()+
                                             SusceptibilityConfig::matlin_suszero;
            }

            else if(SusceptibilityConfig::maternal_protection_type == MaternalProtectionType::SIGMOID)
            {
                susceptibility_correction *= SusceptibilityConfig::matsig_susinit +
                                             (1.0f - SusceptibilityConfig::matsig_susinit)/
                                             (1.0f + exp((SusceptibilityConfig::matsig_halfmax-parent->GetAge())/
                                                           SusceptibilityConfig::matsig_steepfac));
            }
        }

        // Reduces mod_acquire to zero if age is less than calculated immunity failure day.
        if(parent->GetAge() < immune_failage)
        {
            susceptibility_correction = 0.0f;
        }

        BOUND_RANGE(susceptibility_correction, 0.0f, 1.0f);

        LOG_VALID_F("id = %d, age = %f, mod_acquire = %f, immune_failage = %f\n", parent->GetSuid().data, parent->GetAge(), mod_acquire*susceptibility_correction, immune_failage);

        return mod_acquire * susceptibility_correction;
    }

    float Susceptibility::getModTransmit() const
    {
        return mod_transmit;
    }

    float Susceptibility::getModMortality() const
    {
        return mod_mortality;
    }

    float Susceptibility::getImmuneFailage() const
    {
        return immune_failage;
    }

    void Susceptibility::Update( float dt )
    {

        // Immunity decay calculations
        // Logic was revised to eliminate oscillations and ensure decay works correctly even if
        // for mod_XX > 1.0. (As of Feb2018, no simulations are able to specify mod_XX > 1.0)
        if(SusceptibilityConfig::enable_immune_decay)
        {
            // Acquisition immunity decay
            if (acqdecayoffset > 0.0f)
            {
                acqdecayoffset -= dt;
            }
            else
            {
                float net_change = SusceptibilityConfig::acqdecayrate * dt;
                mod_acquire += (1.0f - mod_acquire) * (net_change < 1.0f ? net_change : 1.0f);
            }

            // Transmission immunity decay
            if (trandecayoffset > 0.0f)
            {
                trandecayoffset -= dt;
            }
            else
            {
                float net_change = SusceptibilityConfig::trandecayrate * dt;
                mod_transmit += (1.0f - mod_transmit) * (net_change < 1.0f ? net_change : 1.0f);
            }

            // Disease mortality immunity decay
            if (mortdecayoffset > 0.0f)
            {
                mortdecayoffset -= dt;
            }
            else
            {
                float net_change = SusceptibilityConfig::mortdecayrate * dt;
                mod_mortality += (1.0f - mod_mortality) * (net_change < 1.0f ? net_change : 1.0f);
            }
        }
    }

    void Susceptibility::updateModAcquire(float updateVal)
    {
        mod_acquire *= updateVal;
    }

    void Susceptibility::updateModTransmit(float updateVal)
    {
        mod_transmit *= updateVal;
    }

    void Susceptibility::updateModMortality(float updateVal)
    {
        mod_mortality *= updateVal;
    }

    void Susceptibility::setImmuneFailage(float newFailage)
    {
        immune_failage = newFailage;
    }
    
    void Susceptibility::UpdateInfectionCleared()
    {
        mod_acquire   *= SusceptibilityConfig::baseacqupdate;
        mod_transmit  *= SusceptibilityConfig::basetranupdate;
        mod_mortality *= SusceptibilityConfig::basemortupdate;

        acqdecayoffset  = SusceptibilityConfig::baseacqoffset;
        trandecayoffset = SusceptibilityConfig::basetranoffset;
        mortdecayoffset = SusceptibilityConfig::basemortoffset;
    }

    bool Susceptibility::IsImmune() const
    {
        // Overriden as needed
        release_assert(false);
        return false;
    }

    void Susceptibility::InitNewInfection()
    {
        // Overriden as needed
        release_assert(false);
    }

    REGISTER_SERIALIZABLE(Susceptibility);

    void Susceptibility::serialize(IArchive& ar, Susceptibility* obj)
    {
        Susceptibility& susceptibility = *obj;

        ar.labelElement("age")                        & susceptibility.age;   // REMOVE NEXT VERSION
        
        ar.labelElement("mod_acquire")                & susceptibility.mod_acquire;
        ar.labelElement("mod_transmit")               & susceptibility.mod_transmit;
        ar.labelElement("mod_mortality")              & susceptibility.mod_mortality;

        ar.labelElement("acqdecayoffset")             & susceptibility.acqdecayoffset;
        ar.labelElement("trandecayoffset")            & susceptibility.trandecayoffset;
        ar.labelElement("mortdecayoffset")            & susceptibility.mortdecayoffset;

        ar.labelElement("immune_failage")             & susceptibility.immune_failage;
    }
}
