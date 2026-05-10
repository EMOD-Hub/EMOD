
#include "stdafx.h"
#include "Environment.h"
#include "Debug.h"
#include "Infection.h"
#include "InterventionsContainer.h"
#include "ISusceptibilityContext.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"
#include "StrainIdentity.h"
#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "IdmDateTime.h"
#include "IDistribution.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "Infection" )

namespace Kernel
{
    // static initializers for config base class
    MortalityTimeCourse::Enum  InfectionConfig::mortality_time_course   =  MortalityTimeCourse::DAILY_MORTALITY;
    IDistribution* InfectionConfig::infectious_distribution = nullptr;
    IDistribution* InfectionConfig::incubation_distribution = nullptr;
    float InfectionConfig::base_infectivity = 1.0f;
    float InfectionConfig::base_mortality = 0.0f;
    bool  InfectionConfig::enable_disease_mortality = false;
    unsigned int InfectionConfig::number_basestrains = 1;
    unsigned int InfectionConfig::number_substrains = 1;
    
    // symptomatic
    float InfectionConfig::symptomatic_infectious_offset = FLT_MAX; //disabled

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Infection,InfectionConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionConfig)
    END_QUERY_INTERFACE_BODY(InfectionConfig)


    bool InfectionConfig::Configure(const Configuration* config)
    {
        initConfigTypeMap("Enable_Disease_Mortality", &enable_disease_mortality, Enable_Disease_Mortality_DESC_TEXT, false, "Simulation_Type", "GENERIC_SIM,VECTOR_SIM,STI_SIM,MALARIA_SIM");
        initConfig( "Mortality_Time_Course", mortality_time_course, config, MetadataDescriptor::Enum("mortality_time_course", Mortality_Time_Course_DESC_TEXT, MDD_ENUM_ARGS(MortalityTimeCourse)), "Enable_Disease_Mortality" );
        initConfigTypeMap("Base_Mortality", &base_mortality, Base_Mortality_DESC_TEXT, 0.0f, 1000.0f, 0.001f, "Enable_Disease_Mortality"); // should default change depending on disease?
        initConfigTypeMap("Base_Infectivity", &base_infectivity, Base_Infectivity_DESC_TEXT, 0.0f, 1000.0f, 0.3f, "Simulation_Type", "GENERIC_SIM,VECTOR_SIM,STI_SIM,HIV_SIM");// should default change depending on disease?
       
        // Configure incubation period
        DistributionFunction::Enum incubation_period_function(DistributionFunction::CONSTANT_DISTRIBUTION);
        initConfig("Incubation_Period_Distribution", incubation_period_function, config, MetadataDescriptor::Enum("Incubation_Period_Distribution", Incubation_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)));
        incubation_distribution = DistributionFactory::CreateDistribution( this, incubation_period_function, "Incubation_Period", config );
        
        // Configure infectious duration using depends-on.
        DistributionFunction::Enum infectious_distribution_function(DistributionFunction::CONSTANT_DISTRIBUTION);
        initConfig("Infectious_Period_Distribution", infectious_distribution_function, config, MetadataDescriptor::Enum("Infectious_Period_Distribution", Infectious_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)), "Simulation_Type", "GENERIC_SIM, VECTOR_SIM, STI_SIM");
        infectious_distribution = DistributionFactory::CreateDistribution( this, infectious_distribution_function, "Infectious_Period", config );

        // Symptomatic
        initConfigTypeMap( "Symptomatic_Infectious_Offset", &symptomatic_infectious_offset, Symptomatic_Infectious_Offset_DESC_TEXT, -FLT_MAX, FLT_MAX, FLT_MAX, "Simulation_Type", "GENERIC_SIM" ); //FLT_MAX Individual never becomes symptomatic

        initConfigTypeMap( "Number_Basestrains", &number_basestrains, Number_Basestrains_DESC_TEXT, 1,       10, 1, "Simulation_Type", "GENERIC_SIM,VECTOR_SIM,MALARIA_SIM" );
        initConfigTypeMap( "Number_Substrains",  &number_substrains,  Number_Substrains_DESC_TEXT,  1, 16777216, 1, "Simulation_Type", "GENERIC_SIM" );

        return JsonConfigurable::Configure( config );
    }

    Infection::Infection()
        : parent(nullptr)
        , suid(suids::nil_suid())
        , duration(0.0f)
        , total_duration(0.0f)
        , incubation_timer(0.0f)
        , infectious_timer(0.0f)
        , infectiousness(0.0f)
        , infectiousnessByRoute()
        , StateChange(InfectionStateChange::None)
        , infection_strain(nullptr)
        , m_is_symptomatic( false )
        , m_is_newly_symptomatic( false )
    {
    }

    BEGIN_QUERY_INTERFACE_BODY(Infection)
        HANDLE_INTERFACE(IInfection)
        HANDLE_ISUPPORTS_VIA(IInfection)
    END_QUERY_INTERFACE_BODY(Infection)

    Infection::Infection(IIndividualHumanContext *context)
        : parent(context)
        , suid(suids::nil_suid())
        , sim_time_created(-1.0f)
        , duration(0.0f)
        , total_duration(0.0f)
        , incubation_timer(0.0f)
        , infectious_timer(0.0f)
        , infectiousness(0.0f)
        , infectiousnessByRoute()
        , StateChange(InfectionStateChange::None)
        , infection_strain(nullptr)
        , m_is_symptomatic( false )
        , m_is_newly_symptomatic( false )
    {
        if( parent != nullptr )
        {
            sim_time_created = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        }
    }

    void Infection::Initialize(suids::suid _suid)
    {
        suid = _suid;
    }

    Infection *Infection::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        Infection *newinfection = _new_ Infection(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    Infection::~Infection()
    {
        delete infection_strain;
    }

    void Infection::SetParameters( const IStrainIdentity* infstrain, int incubation_period_override ) // or something
    {
        // Set up infection strain
        CreateInfectionStrain(infstrain);

        if( incubation_period_override != -1 )
        {
            incubation_timer = float(incubation_period_override);
        }
        else
        {
            incubation_timer = InfectionConfig::incubation_distribution->Calculate( parent->GetRng() );
            LOG_DEBUG_F( "incubation_timer initialized to %f for individual %d\n", incubation_timer, parent->GetSuid().data );
        }

        infectious_timer = InfectionConfig::infectious_distribution->Calculate( parent->GetRng() );
        
        LOG_DEBUG_F( "infectious_timer = %f\n", infectious_timer );

        total_duration = incubation_timer + infectious_timer;
        infectiousness = 0;
        StateChange    = InfectionStateChange::None;

        if (incubation_timer <= 0)
        {
            infectiousness = InfectionConfig::base_infectivity;
        }
    }

    void Infection::InitInfectionImmunology(ISusceptibilityContext* _immunity)
    {
    }

    // TODO future : grant access to the susceptibility object by way of the host context and keep the update call neutral
    void Infection::Update( float currentTime, float dt, ISusceptibilityContext* immunity )
    {
        StateChange = InfectionStateChange::None;
        duration += dt;

        if (duration > incubation_timer)
        {
            infectiousness = InfectionConfig::base_infectivity;

            // Used to have a release_assert( infectiousness ) here to make sure infectiousness was not zero, 
            // but setting infectiousness to zero can be a valid use case (e.g., while applying external incidence only)
        }

        // To query for mortality-reducing effects of drugs or vaccines
        IDrugVaccineInterventionEffects* idvie = nullptr;

        // if disease has a daily mortality rate, and disease mortality is on, then check for death. mortality_time_course depends-on enable_disease_mortality BUT DAILY_MORTALITY is default
        if (InfectionConfig::enable_disease_mortality && (InfectionConfig::mortality_time_course == MortalityTimeCourse::DAILY_MORTALITY) && (duration > incubation_timer))
        {
            if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext" );
            }
            float prob = InfectionConfig::base_mortality * dt * immunity->getModMortality() * idvie->GetInterventionReducedMortality();
            if( parent->GetRng()->SmartDraw( prob ) )
            { 
                StateChange = InfectionStateChange::Fatal; 
            }
        }

        if (duration > total_duration)
        {
            // disease mortality active and is accounted for at end of infectious period. mortality_time_course depends-on enable_disease_mortality
            if (InfectionConfig::enable_disease_mortality && InfectionConfig::mortality_time_course == MortalityTimeCourse::MORTALITY_AFTER_INFECTIOUS )
            {
                if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext" );
                }
                float prob = InfectionConfig::base_mortality * immunity->getModMortality() * idvie->GetInterventionReducedMortality();
                if( parent->GetRng()->SmartDraw( prob ) )
                {
                    StateChange = InfectionStateChange::Fatal;
                }
                else
                {
                    StateChange = InfectionStateChange::Cleared;
                }//  For debugging only  (1-base_mortality) recover, rest chance die at end of infection, modified by mortality immunity
            }
            else
            {
                StateChange = InfectionStateChange::Cleared;
            }
        }

        UpdateSymptomatic( duration, incubation_timer );
        
        EvolveStrain(immunity, dt); // genomic modifications
    }

    void Infection::CreateInfectionStrain( const IStrainIdentity* infstrain )
    {
        release_assert( infstrain != nullptr );
        release_assert( infection_strain == nullptr );
        infection_strain = infstrain->Clone();
    }

    void Infection::EvolveStrain(ISusceptibilityContext* immunity, float dt)
    {
        // genetic evolution happens here.
        // infection_strain
    }

    const IStrainIdentity& Infection::GetInfectiousStrainID() const 
    {
        release_assert( infection_strain != nullptr );
        return *infection_strain;
    }

    void Infection::SetContextTo(IIndividualHumanContext* context) { parent = context; }

    IIndividualHumanContext* Infection::GetParent() { return parent; }

    suids::suid Infection::GetSuid() const { return suid; }

    const SimulationConfig* Infection::params() { return GET_CONFIGURABLE(SimulationConfig); } // overridden in derived classes but with different return types to hide the casting operation

    InfectionStateChange::_enum Infection::GetStateChange() const { return StateChange; }

    float Infection::GetInfectiousness() const { return infectiousness; }

    float Infection::GetInfectiousnessByRoute( const string& route ) const {
        if( infectiousnessByRoute.find( route ) == infectiousnessByRoute.end() )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "infectiousnesssByRoute", route.c_str() );
        }
        return infectiousnessByRoute.at(route); 
    }

    // Created for TB, but makes sense to be in base class, but no-one else is using yet, placeholder functionality
    bool Infection::IsActive() const
    {
        return false;
    }

    NonNegativeFloat
    Infection::GetDuration()
    const
    {
        return duration;
    }

    bool
    Infection::StrainMatches( IStrainIdentity * pStrain )
    {
        return(infection_strain->GetAntigenID() == pStrain->GetAntigenID());
    }

    bool Infection::IsNewlySymptomatic() const
    {
        return  m_is_newly_symptomatic;
    }

    bool Infection::IsSymptomatic() const
    {
        return m_is_symptomatic;
    }

    void Infection::UpdateSymptomatic( float const duration, float const incubation_timer )
    {
        bool prev_symptomatic = m_is_symptomatic;
        m_is_symptomatic = DetermineSymptomatology( duration, incubation_timer );
        m_is_newly_symptomatic = ( m_is_symptomatic && !prev_symptomatic );
    }

    bool Infection::DetermineSymptomatology( float const duration, float const incubation_timer )
    {
        return ( ( duration - incubation_timer ) > InfectionConfig::symptomatic_infectious_offset );
    }

    float Infection::GetSimTimeCreated() const
    {
        return sim_time_created;
    }

    REGISTER_SERIALIZABLE(Infection);

    void Infection::serialize(IArchive& ar, Infection* obj)
    {
        Infection& infection = *obj;
        ar.labelElement( "suid"                   ) & infection.suid;
        ar.labelElement( "sim_time_created"       ) & infection.sim_time_created;
        ar.labelElement( "duration"               ) & infection.duration;
        ar.labelElement( "total_duration"         ) & infection.total_duration;
        ar.labelElement( "incubation_timer"       ) & infection.incubation_timer;
        ar.labelElement( "infectious_timer"       ) & infection.infectious_timer;
        ar.labelElement( "infectiousness"         ) & infection.infectiousness;
        ar.labelElement( "infectiousnessByRoute"  ) & infection.infectiousnessByRoute;
        ar.labelElement( "StateChange"            ) & (uint32_t&)infection.StateChange;
        ar.labelElement( "infection_strain"       ) & infection.infection_strain;
        ar.labelElement( "m_is_symptomatic"       ) & infection.m_is_symptomatic;
        ar.labelElement( "m_is_newly_symptomatic" ) & infection.m_is_newly_symptomatic;
    }
}
