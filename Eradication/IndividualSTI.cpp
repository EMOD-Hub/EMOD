
#include "stdafx.h"

#include <typeinfo>
#include "IndividualSTI.h"

#include "Debug.h"
#include "IIndividualHuman.h"
#include "InfectionSTI.h"
#include "NodeEventContext.h"
#include "Relationship.h"
#include "RelationshipGroups.h"
#include "SusceptibilitySTI.h"
#include "STIInterventionsContainer.h"
#include "INodeSTI.h"
#include "Sigmoid.h"
#include "IPairFormationStats.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationAgent.h"
#include "IConcurrency.h"
#include "StrainIdentity.h"
#include "EventTrigger.h"
#include "RANDOM.h"
#include "INodeContext.h"

SETUP_LOGGING( "IndividualSTI" )

// Assume MAX_SLOTS == 63 => 64-bits are full
#define SLOTS_FILLED (uint64_t(0xFFFFFFFFFFFFFFFF))

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! If you change size of promiscuity_flags (unsigned char) or
// !!! add more RelationshipTypes (>7), change this macro.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define SUPER_SPREADER 0x80

#define IS_SUPER_SPREADER()   ((promiscuity_flags & SUPER_SPREADER) != 0)
#define IS_EXTRA_ALLOWED(rel) ((promiscuity_flags & EXTRA_RELATIONAL_ALLOWED((Kernel::RelationshipType::Enum)rel)) != 0)
#define MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES (MAX_SLOTS)

// -------------------------------------------------------------------
// !!!! Duplicated in ReportRelationshipCensus !!!
#define THREE_MONTHS  ( 91) // ~3 months
#define SIX_MONTHS    (182) // ~6 months
#define NINE_MONTHS   (274) // ~9 months
#define TWELVE_MONTHS (365) // ~12 months

static const float PERIODS[] = { THREE_MONTHS, SIX_MONTHS, NINE_MONTHS, TWELVE_MONTHS };
static std::vector<float> UNIQUE_PARTNER_TIME_PERIODS( PERIODS, PERIODS + sizeof( PERIODS ) / sizeof( PERIODS[ 0 ] ) );
// -------------------------------------------------------------------

namespace Kernel
{
    bool IndividualHumanSTI::needs_census_data = false; // see header for details

    float IndividualHumanSTIConfig::debutAgeYrsMale_inv_kappa = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsMale_lambda = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsFemale_inv_kappa = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsFemale_lambda = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsMin = 13.0f;

    float IndividualHumanSTIConfig::sti_coinfection_trans_mult = 1.0f;
    float IndividualHumanSTIConfig::sti_coinfection_acq_mult = 1.0f;

    float IndividualHumanSTIConfig::min_days_between_adding_relationships = 60.0f;

    float IndividualHumanSTIConfig::condom_transmission_blocking_probability = 0.0f;

    std::vector<float> IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges;
    std::vector<float> IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers;

    bool IndividualHumanSTIConfig::enable_coital_dilution = true;
    float IndividualHumanSTIConfig::coital_dilution_2_partners      = 1.0f;
    float IndividualHumanSTIConfig::coital_dilution_3_partners      = 1.0f;
    float IndividualHumanSTIConfig::coital_dilution_4_plus_partners = 1.0f;

    SexualDebutAgeSettingType::Enum IndividualHumanSTIConfig::sexual_debut_age_setting_type = SexualDebutAgeSettingType::WEIBULL;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(IndividualHumanSTI,IndividualHumanSTIConfig)
    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanSTIConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanSTIConfig)

    bool IndividualHumanSTIConfig::Configure( const Configuration* config )
    {
        LOG_DEBUG( "Configure\n" );

        initConfig( "Sexual_Debut_Age_Setting_Type", sexual_debut_age_setting_type, config, MetadataDescriptor::Enum( "sexual_debut_age_setting_type", Sexual_Debut_Age_Setting_Type_DESC_TEXT, MDD_ENUM_ARGS( SexualDebutAgeSettingType ) ) );
        initConfigTypeMap( "Sexual_Debut_Age_Male_Weibull_Heterogeneity", &debutAgeYrsMale_inv_kappa, Sexual_Debut_Age_Male_Weibull_Heterogeneity_DESC_TEXT, 0.0f, 50.0f, 20.0f, "Sexual_Debut_Age_Setting_Type", "WEIBULL");
        initConfigTypeMap( "Sexual_Debut_Age_Male_Weibull_Scale", &debutAgeYrsMale_lambda, Sexual_Debut_Age_Male_Weibull_Scale_DESC_TEXT, 0.0f, 50.0f, 16.0f, "Sexual_Debut_Age_Setting_Type", "WEIBULL" );     // Min was 13
        initConfigTypeMap( "Sexual_Debut_Age_Female_Weibull_Heterogeneity", &debutAgeYrsFemale_inv_kappa, Sexual_Debut_Age_Female_Weibull_Heterogeneity_DESC_TEXT, 0.0f, 50.0f, 20.0f, "Sexual_Debut_Age_Setting_Type", "WEIBULL" );
        initConfigTypeMap( "Sexual_Debut_Age_Female_Weibull_Scale", &debutAgeYrsFemale_lambda, Sexual_Debut_Age_Female_Weibull_Scale_DESC_TEXT, 0.0f, 50.0f, 16.0f, "Sexual_Debut_Age_Setting_Type", "WEIBULL" );   // Min was 13

        initConfigTypeMap( "Sexual_Debut_Age_Min", &debutAgeYrsMin, Sexual_Debut_Age_Min_DESC_TEXT, 0.0f, FLT_MAX, 13.0f );

        initConfigTypeMap( "STI_Coinfection_Acquisition_Multiplier", &sti_coinfection_acq_mult, STI_Coinfection_Acquisition_Multiplier_DESC_TEXT, 0.0f, 100.0f, 10.0f );
        initConfigTypeMap( "STI_Coinfection_Transmission_Multiplier", &sti_coinfection_trans_mult, STI_Coinfection_Transmission_Multiplier_DESC_TEXT, 0.0f, 100.0f, 10.0f );

        initConfigTypeMap( "Min_Days_Between_Adding_Relationships", &min_days_between_adding_relationships, Min_Days_Between_Adding_Relationships_DESC_TEXT, 0.0f, 365.0f, 60.0f );

        initConfigTypeMap( "Male_To_Female_Relative_Infectivity_Ages", &maleToFemaleRelativeInfectivityAges, Male_To_Female_Relative_Infectivity_Ages_DESC_TEXT, 0.0f, FLT_MAX, false );
        initConfigTypeMap( "Male_To_Female_Relative_Infectivity_Multipliers", &maleToFemaleRelativeInfectivityMultipliers, Male_To_Female_Relative_Infectivity_Multipliers_DESC_TEXT, 0.0f, 25.0f );

        initConfigTypeMap( "Condom_Transmission_Blocking_Probability", &condom_transmission_blocking_probability, Condom_Transmission_Blocking_Probability_DESC_TEXT, 0.0f, 1.0f, 0.9f );

        initConfigTypeMap( "Enable_Coital_Dilution", &enable_coital_dilution, Enable_Coital_Dilution_DESC_TEXT, true );
        initConfigTypeMap( "Coital_Dilution_Factor_2_Partners", &coital_dilution_2_partners, Coital_Dilution_Factor_2_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f );
        initConfigTypeMap( "Coital_Dilution_Factor_3_Partners", &coital_dilution_3_partners, Coital_Dilution_Factor_3_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f);
        initConfigTypeMap( "Coital_Dilution_Factor_4_Plus_Partners", &coital_dilution_4_plus_partners, Coital_Dilution_Factor_4_Plus_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( maleToFemaleRelativeInfectivityAges.size() != maleToFemaleRelativeInfectivityMultipliers.size() )
            {
                std::stringstream ss;
                ss << "ERROR: Invalid array lengths for 'Male_To_Female_Relative_Infectivity_Ages' and 'Male_To_Female_Relative_Infectivity_Multipliers'\n";
                ss << "'Male_To_Female_Relative_Infectivity_Ages' has " << maleToFemaleRelativeInfectivityAges.size() << " elements.\n";
                ss << "'Male_To_Female_Relative_Infectivity_Multipliers' has " << maleToFemaleRelativeInfectivityMultipliers.size() << " elements.\n";
                ss << "They must be the same length.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }

        return ret;
    }

    // The reason for this explicit QI (instead of macro) is to let us resolve 
    // the IJsonSerializable cast which is ambiguous and must be done via
    // a specific intermediate.
    Kernel::QueryResult
    IndividualHumanSTI::QueryInterface( iid_t iid, void **ppinstance )
    {
        if ( !ppinstance )
        {
            return e_NULL_POINTER;
        }

        ISupports* foundInterface = nullptr;

        if ( iid == GET_IID(IIndividualHuman)) 
            foundInterface = static_cast<IIndividualHuman*>(this);
        else if ( iid == GET_IID(IIndividualHumanSTI)) 
            foundInterface = static_cast<IIndividualHumanSTI*>(this);
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<IIndividualHumanSTI*>(this));
        else if( IndividualHuman::QueryInterface( iid, (void**)&foundInterface ) != s_OK )
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    void IndividualHumanSTI::SetConcurrencyParameters( const char *prop, const char* prop_value )
    {
        // Update promiscuity flags.  Begin with a reset.
        if( IS_SUPER_SPREADER() )
            promiscuity_flags = SUPER_SPREADER;
        else
            promiscuity_flags = 0;

        IConcurrency* p_concurrency = p_sti_node->GetSociety()->GetConcurrency();

        promiscuity_flags |= p_concurrency->GetProbExtraRelationalBitMask( GetRng(), prop, prop_value, Gender::Enum(GetGender()), IS_SUPER_SPREADER() );

        // Max allowable relationships
        NaturalNumber totalMax = 0;
        for( int rel = 0; rel < RelationshipType::COUNT; rel++)
        {
            max_relationships[rel] = p_concurrency->GetMaxAllowableRelationships( GetRng(), prop, prop_value, Gender::Enum(GetGender()), RelationshipType::Enum(rel) );
            totalMax += max_relationships[rel];
        }
        if( totalMax > MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "<sum of max number of relationships across types>", totalMax, MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES );
        }
    }

    void IndividualHumanSTI::UpdateSTINetworkParams( const char *prop, const char* new_value )
    {
        IConcurrency* p_concurrency = p_sti_node->GetSociety()->GetConcurrency();

        if( ((prop == nullptr) || p_concurrency->IsConcurrencyProperty( prop )) && (sexual_debut_age >= 0) )
        {
            const char* prop_key = prop;
            if( prop == nullptr )
            {
                prop_key = p_concurrency->GetPropertyKey().c_str();
            }
            const char* prop_value = p_concurrency->GetConcurrencyPropertyValue( *GetProperties(), prop_key, new_value );
            SetConcurrencyParameters( prop_key, prop_value );
        }
    }

    IndividualHumanSTI *IndividualHumanSTI::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender)
    {
        IndividualHumanSTI *newindividual = _new_ IndividualHumanSTI(id, MCweight, init_age, gender);

        newindividual->SetContextTo(context);
        newindividual->InitializeConcurrency();
        LOG_DEBUG_F( "Created human with age=%f and gender=%d\n", newindividual->m_age, gender );

        if( context != nullptr )
        {
            newindividual->sim_day_born = context->GetTime().time - init_age;
        }
        else
        {
            newindividual->sim_day_born -= init_age;
        }

        return newindividual;
    }

    void IndividualHumanSTI::InitializeConcurrency()
    {
        if( IndividualHumanSTIConfig::sexual_debut_age_setting_type == SexualDebutAgeSettingType::FROM_INTERVENTION )
        {
            sexual_debut_age = MAX_HUMAN_AGE * DAYSPERYEAR; // No sexual debut; needs to be set with intervention
        }
        else
        {
            sexual_debut_age = CalculateSexualDebutAgeByWeibull();
        }

        // Promiscuity flags, including behavioral super-spreader
        if( p_sti_node && GetRng()->SmartDraw( p_sti_node->GetSociety()->GetConcurrency()->GetProbSuperSpreader() ) )
        {
            promiscuity_flags |= SUPER_SPREADER;
        }
    }

    void IndividualHumanSTI::SetSexualDebutAge( float age )
    {
        sexual_debut_age = age;
        bool is_post_debut = ( m_age >= sexual_debut_age );

        if( is_post_debut && broadcaster )
        {
            // Broadcast STIDebut
            enter_PFA_now = true;
            enter_relationship_now = true;
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::STIDebut );
        }
    }

    float IndividualHumanSTI::CalculateSexualDebutAgeByWeibull()
    {
        // -------------------------------------------------------------------------------------------------------------
        // --- DMB 6-11-2016  I'm not doing this in InitializeHuman() because it change the random number stream.
        // --- This used to be called in the constructor, but I can't do that because we need p_sti_node to be defined.
        // -------------------------------------------------------------------------------------------------------------

        // Sexual debut
        float min_age_sexual_debut_in_days = IndividualHumanSTIConfig::debutAgeYrsMin * DAYSPERYEAR;
        float debut_lambda = 0;
        float debut_inv_kappa = 0;
        if( GetGender() == Gender::MALE ) 
        {
            debut_inv_kappa = IndividualHumanSTIConfig::debutAgeYrsMale_inv_kappa;
            debut_lambda    = IndividualHumanSTIConfig::debutAgeYrsMale_lambda;
        }
        else
        {
            debut_inv_kappa = IndividualHumanSTIConfig::debutAgeYrsFemale_inv_kappa;
            debut_lambda    = IndividualHumanSTIConfig::debutAgeYrsFemale_lambda;
        }
        float debut_draw = float( DAYSPERYEAR * GetRng()->Weibull2( debut_lambda, debut_inv_kappa ) );
        LOG_DEBUG_F( "debut_draw = %f with lamba %f and kappa %f\n", debut_draw, debut_lambda, debut_inv_kappa );
        float debut_age = (std::max)( min_age_sexual_debut_in_days, debut_draw );

        LOG_DEBUG_F( "Individual ? will debut at age %f (yrs)f.\n", debut_age / DAYSPERYEAR );

        return debut_age;
    }

    void IndividualHumanSTI::InitializeHuman()
    {
        IndividualHuman::InitializeHuman();

        UpdateSTINetworkParams( nullptr, nullptr );
    }

    void IndividualHumanSTI::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility = SusceptibilitySTI::CreateSusceptibility(this, imm_mod, risk_mod);
    }

    IIndividualHuman* IndividualHumanSTI::GetIndividualHuman()
    {
        return this;
    }

    void IndividualHumanSTI::NotifyPotentialExposure()
    {
        potential_exposure_flag = true;
    }

    std::vector<unsigned int> IndividualHumanSTI::GetQueuedRelationships() const
    {
        return std::vector<unsigned int>(queued_relationships, queued_relationships + RelationshipType::Enum::COUNT);
    }

    std::vector<bool> IndividualHumanSTI::GetAvailableRelationships() const
    {        
        return std::vector<bool>( available, available + RelationshipType::Enum::COUNT );
    }

    void IndividualHumanSTI::ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership)
    {
        if( potential_exposure_flag )
        {
            LOG_DEBUG_F( "Exposing individual %d\n", GetSuid().data );
            release_assert( IsInfected() == false );

            parent->ExposeIndividual( this, transmissionGroupMembership, dt );
            potential_exposure_flag = false;
        }
    }

    float IndividualHumanSTI::GetSusceptibilityMultipilierCircumcision() const
    {
        float multiplier = 1.0;
        if( (GetGender() == Gender::MALE) && IsCircumcised() )
        {
            multiplier = (1.0 - m_pSTIInterventionsContainer->GetCircumcisedReducedAcquire());
        }
        return multiplier;
    }

    float IndividualHumanSTI::GetSusceptibilityMultiplierMaleToFemale() const
    {
        float multiplier = 1.0f;
        if( (GetGender() == Gender::FEMALE) && (IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges.size() > 0) )
        {
            // Individual is female. If user specified a male-to-female infectivity multiplier, 
            // we need to apply that to the probability of infection. The base infectivity
            // is for female to male. Male to female can be equal or greater.
            float age_in_yrs = GetAge() / DAYSPERYEAR;
            unsigned int idx = 0;
            while( idx < IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges.size() &&
                   age_in_yrs > IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges[ idx ] )
            {
                idx++;
            }
            // 3 possible cases
            if( idx == 0 )
            {
                multiplier = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers[ 0 ];
                LOG_DEBUG_F( "Using value of %f for age-asymmetric infection multiplier for female age (in yrs) %f.\n", float( multiplier ), (float)age_in_yrs );
            }
            else if( idx == IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges.size() )
            {
                multiplier = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers.back();
                LOG_DEBUG_F( "Using value of %f for age-asymmetric infection multiplier for female age (in yrs) %f.\n", float( multiplier ), (float)age_in_yrs );
            }
            else
            {
                // do some linear interp math.
                float left_age   = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges[ idx - 1 ];
                float right_age  = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges[ idx ];
                float left_mult  = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers[ idx - 1 ];
                float right_mult = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers[ idx ];

                multiplier = left_mult + (age_in_yrs - left_age) / (right_age - left_age) * (right_mult - left_mult);

                LOG_DEBUG_F( "Using interpolated value of %f for age-asymmetric infection multiplier for age (in yrs) %f.\n", float( multiplier ), (float)age_in_yrs );
            }
        }
        return multiplier;
    }

    void IndividualHumanSTI::Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route)
    {
        IContagionInfo * p_info = nullptr;
        if( (const_cast<IContagionPopulation*>(cp))->QueryInterface( GET_IID( IContagionInfo ), (void**)&p_info ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "cp", "IContagionInfo", "IContagionPopulation" );
        }

        // Apply gender-based susceptibility multipliers here
        float susceptibility_multiplier = 1.0;
        susceptibility_multiplier *= GetSusceptibilityMultipilierCircumcision();
        susceptibility_multiplier *= GetSusceptibilityMultiplierMaleToFemale();

        float prob_acquire = p_info->GetInterventionReducedAcquire() * susceptibility_multiplier;

        m_CurrentCoitalAct = p_info->GetCoitalAct();
        m_CurrentCoitalAct.SetAcquisitionProbability( prob_acquire );

        float prob_infection = m_CurrentCoitalAct.GetRiskMultiplier()
                             * m_CurrentCoitalAct.GetTransmissionProbability()
                             * m_CurrentCoitalAct.GetAcquisitionProbability();

        if( !IsInfected() && GetRng()->SmartDraw( prob_infection ) )
        {
            StrainIdentity strainId;
            cp->ResolveInfectingStrain( &strainId ); // get the substrain ID

            // The ContagionProbability object can give us the ID of the Infector (Depositor)
            // We store this as the heretofore unused "Antigen ID" of the Infecting Strain.
            strainId.SetAntigenID( m_CurrentCoitalAct.GetInfectedPartnerID().data );

            m_CurrentCoitalAct.SetWasTransmitted();

            AcquireNewInfection( &strainId );
        }
        else if( IsInfected() )
        {
            m_CurrentCoitalAct.ClearProbabilities();
        }
        if( broadcaster != nullptr )
        {
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::STIExposed );
        }
    }

    IndividualHumanSTI::IndividualHumanSTI(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender)
        : IndividualHuman(_suid, monte_carlo_weight, initial_age, gender)
        , relationships()
        , relationship_ids()
        , max_relationships()
        , queued_relationships()
        , active_relationships()
        , migrating_because_of_partner(false)
        , migrating_rel( nullptr )
        , promiscuity_flags(0)
        , sim_day_born( 0 )
        , sexual_debut_age( -1.0f )
        , age_at_infection( -1.0f )
        , co_infective_factor( 0.0f )
        , has_other_sti_co_infection(false)
        , transmissionInterventionsDisabled(false)
        , relationshipSlots(0)
        , delay_between_adding_relationships_timer(0.0f)
        , potential_exposure_flag(false)
        , m_pSTIInterventionsContainer( nullptr )
        , p_exiting_relationship( nullptr )
        , m_CurrentCoitalAct()
        , available()
        , enter_PFA_now( false )
        , enter_relationship_now( false )
        , relationships_terminated()
        , num_lifetime_relationships()
        , last_6_month_relationships()
        , last_12_month_relationships()
       // , slot2RelationshipDebugMap()
        , p_sti_node(nullptr)
        , m_AssortivityIndex()
        , m_TotalCoitalActs( 0 )
        , num_unique_partners()
    {
        ZERO_ARRAY( queued_relationships );
        ZERO_ARRAY( active_relationships );
        ZERO_ARRAY( num_lifetime_relationships );

        for (int i = 0; i < RelationshipType::COUNT; ++i)
        {
            m_AssortivityIndex[i] = -1;
        }

        if( IndividualHumanSTI::needs_census_data )
        {
            InitializeNumUniquePartners();
        }
    }

    void IndividualHumanSTI::InitializeNumUniquePartners()
    {
        for( int i = 0; i < UNIQUE_PARTNER_TIME_PERIODS.size(); ++i )
        {
            std::vector<PartnerIdToRelEndTimeMap_t> rel_type_vector_of_maps;
            for( int j = 0; j < RelationshipType::COUNT; ++j )
            {
                rel_type_vector_of_maps.push_back( PartnerIdToRelEndTimeMap_t() );
            }
            num_unique_partners.push_back( rel_type_vector_of_maps );
        }
    }

    const IPKeyValueContainer& IndividualHumanSTI::GetPropertiesConst() const
    {
        return Properties;
    }

    suids::suid IndividualHumanSTI::GetNodeSuid() const
    {
        if( parent == nullptr )
        {
            return suids::nil_suid();
        }
        else
        {
            return parent->GetSuid();
        }
    }

    void IndividualHumanSTI::Update(float currenttime, float dt)
    {
        IndividualHuman::Update( currenttime, dt );
    }

    void IndividualHumanSTI::UpdatePausedRelationships( const IdmDateTime& rCurrentTime, float dt )
    {
        // ---------------------------------------------------------------
        // --- Update the individual pointers in the paused relationships.
        // --- This is to help against using invalid pointers.
        // ---------------------------------------------------------------
        for( auto rel : relationships )
        {
            rel->UpdatePaused();
        }
        relationships_terminated.clear();
    }

    void IndividualHumanSTI::UpdateAge( float dt )
    {
        bool was_pre_debut = m_age < sexual_debut_age;

        IndividualHuman::UpdateAge( dt );

        // Check for debut
        bool is_post_debut = m_age >= sexual_debut_age;
        if( was_pre_debut && is_post_debut && broadcaster )
        {
            // Broadcast STIDebut
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::STIDebut );
        }
    }

    bool IndividualHumanSTI::EnterPfaNow() const
    {
        return enter_PFA_now;
    }

    bool IndividualHumanSTI::EnterRelationshipNow() const
    {
        return enter_relationship_now;
    }

    void IndividualHumanSTI::UpdateHistory( const IdmDateTime& rCurrentTime, float dt )
   {
       while( (last_6_month_relationships.size() > 0) && ((rCurrentTime.time - last_6_month_relationships.front().second) > SIX_MONTHS) )
       {
            last_6_month_relationships.pop_front();
       }
       while( (last_12_month_relationships.size() > 0) && ((rCurrentTime.time - last_12_month_relationships.front().second) > TWELVE_MONTHS) )
       {
           last_12_month_relationships.pop_front();
       }

        if( IndividualHumanSTI::needs_census_data )
        {
            if( num_unique_partners.size() == 0 )
            {
                InitializeNumUniquePartners();
            }

            // This took over 8% of processing in some scenarios even when not needed
            for( int itp = 0; itp < UNIQUE_PARTNER_TIME_PERIODS.size(); ++itp )
            {
                float time_period = UNIQUE_PARTNER_TIME_PERIODS[ itp ];
                for( int irel = 0; irel < RelationshipType::COUNT; ++irel )
                {
                    PartnerIdToRelEndTimeMap_t& r_partner_map = num_unique_partners[ itp ][ irel ];
                    for( auto it = r_partner_map.begin(); it != r_partner_map.end();  )
                    {
                        if( (rCurrentTime.time - it->second) > time_period )
                        {
                            it = r_partner_map.erase( it );
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }
        }
    }

    void IndividualHumanSTI::Die(HumanStateChange newState)
    {
        release_assert( (newState == HumanStateChange::DiedFromNaturalCauses) || (newState == HumanStateChange::KilledByInfection) );

        // Remove self from PFAs if queued up
        disengageFromSociety();

        if( p_sti_node != nullptr )
        {
            IRelationshipManager* p_manager = p_sti_node->GetRelationshipManager();

            while( relationships.size() > 0 )
            {
                auto relationship = relationships[ 0 ];
                relationship->Terminate( RelationshipTerminationReason::PARTNER_DIED );
                p_manager->RemoveRelationship( relationship, true, true ); // RM owns deleting the object
                LOG_DEBUG_F("Relationship with %d terminated at time %f due to death\n", GetSuid().data, (float) GetParent()->GetTime().time );
            }

            if( migrating_because_of_partner )
            {
                release_assert( migrating_rel != nullptr );
                migrating_rel->Terminate( RelationshipTerminationReason::PARTNER_DIED );
                p_manager->RemoveRelationship( migrating_rel, true, true ); // RM owns deleting the object
            }
        }

        IndividualHuman::Die( newState );
    }

    IndividualHumanSTI::~IndividualHumanSTI()
    {
        LOG_DEBUG_F( "%lu (STI) destructor.\n", suid.data );
    }

    IInfection* IndividualHumanSTI::createInfection( suids::suid _suid )
    {
        return InfectionSTI::CreateInfection(this, _suid);
    }

    void IndividualHumanSTI::InitializeStaticsSTI( const Configuration* config )
    {
        IndividualHumanSTIConfig individual_config;
        individual_config.Configure( config );
    }

    void IndividualHumanSTI::setupInterventionsContainer()
    {
        m_pSTIInterventionsContainer = _new_ STIInterventionsContainer();
        interventions = m_pSTIInterventionsContainer;
    }

    void IndividualHumanSTI::UpdateGroupMembership()
    {
        // do nothing
    }

    void IndividualHumanSTI::UpdateInfectiousnessSTI( CoitalAct& rCoitalAct )
    {
        infectiousness = 0;

        if( infections.size() > 1 )
        {
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "STI/HIV does not support superinfection yet." );
        }
        else if( infections.size() == 0 )
        {
            // we shouldn't get here right now, but in the future we might.
            release_assert( false );
        }

        release_assert( infections.size() == 1 );
        IInfection*p_infection = *(infections.begin());
        infectiousness += p_infection->GetInfectiousness();
        const IStrainIdentity& r_strain_id = p_infection->GetInfectiousStrainID();

        // For STI/HIV, infectiousness should be the per-act probability of transmission
        // including intrahost factors like stage and ART, but excluding condoms
        // Also excluding STI because they're handled separately
        float tran_prob =  m_mc_weight * infectiousness * susceptibility->getModTransmit() * interventions->GetInterventionReducedTransmit();

        rCoitalAct.SetTransmissionProbability( tran_prob );

        p_sti_node->DepositFromIndividual( r_strain_id, rCoitalAct );

        infectiousness *= susceptibility->getModTransmit() * interventions->GetInterventionReducedTransmit();
    }

    void IndividualHumanSTI::UpdateInfectiousness(float dt)
    {
        if( delay_between_adding_relationships_timer > 0.0f )
        {
            delay_between_adding_relationships_timer -= dt;
        }

        // DJK: Why only consummate relationships of males?  What about MSM?  Designate one "consummator" per rel?  <ERAD-1868>
        if( GetGender() == Gender::MALE )
        {
            for (auto relationship : relationships)
            {
                release_assert( relationship );
                relationship->Consummate( GetRng(), dt, p_sti_node->GetRelationshipManager() );
            }
        }
    }

    void IndividualHumanSTI::AcquireNewInfection( const IStrainIdentity* infstrain, int incubation_period_override )
    {
        int numInfs = int(infections.size());
        if( (numInfs >= IndividualHumanConfig::max_ind_inf) ||
            (!IndividualHumanConfig::superinfection && numInfs > 0 )
          )
        {
            return;
        }

        LOG_DEBUG_F( "(EEL) %s: %s\n", __FUNCTION__, toString().c_str() );

        // Would be nice to log the infecting relationship.

        // Let's just put some paranoid code to make sure that we are in relationship with an infected individual
        if( infstrain )
        {
            LOG_DEBUG_F( "(EEL) individual %lu infected by relationship partner %lu\n", GetSuid().data, infstrain->GetAntigenID() );
        }
        else
        {
            LOG_DEBUG_F( "(EEL) individual %lu infected, but relationship partner cannot be identified\n", GetSuid().data );
        }

        if( infstrain && infstrain->GetAntigenID() == 0 )
        {
            release_assert( infstrain );
            LOG_DEBUG_F( "(EEL) individual %lu infected by relationship partner %lu\n", GetSuid().data, infstrain->GetAntigenID() );
            if( infstrain->GetAntigenID() == 0 )
            {
                // Can't throw exception because there are even non-outbreak cases where this is valid (e.g., infecting relationship ended this time step
                LOG_DEBUG_F( "%s: individual %lu got infected but not in relationship with an infected individual (num relationships=%d). OK if PrevIncrease Outbreak.!!!\n",
                            __FUNCTION__, GetSuid().data, relationships.size() );
            }
        }

        IndividualHuman::AcquireNewInfection( infstrain, incubation_period_override );
        if( (numInfs == 0) && (infections.size() > 0) )
        {
            age_at_infection = GetAge();
        }

        if( broadcaster )
        {
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::STINewInfection );
        }
    }

    void IndividualHumanSTI::UpdateEligibility()
    {
        // DJK: Could return if pre-sexual-debut, related to <ERAD-1869>
        release_assert( p_sti_node );
        ISociety* society = p_sti_node->GetSociety();

        for( int type=0; type<RelationshipType::COUNT; type++ )
        {
            RelationshipType::Enum rel_type = RelationshipType::Enum(type);
            available[type] = AvailableForRelationship( rel_type );
            if( available[type] )
            {
                RiskGroup::Enum risk_group = IS_EXTRA_ALLOWED(rel_type) ? RiskGroup::HIGH : RiskGroup::LOW;
                society->GetStats(rel_type)->UpdateEligible(m_age, m_gender, risk_group, 1);    // DJK: Should use MC weight <ERAD-1870>
            }
        }
    }

    void
    IndividualHumanSTI::ConsiderRelationships(float dt)
    {
        release_assert( p_sti_node );
        ISociety* society = p_sti_node->GetSociety();

        float ellapsed_time = 0;
        bool is_any_available = false;
        float formation_rates[RelationshipType::COUNT];
        ZERO_ARRAY(formation_rates);
        float cumulative_rate = 0.0f;
        int max_rate_type = -1;
        float max_rate = 0.0;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            RelationshipType::Enum rel_type = RelationshipType::Enum(type);

            is_any_available |= available[type];

            if( available[type] )
            {
                RiskGroup::Enum risk_group = IS_EXTRA_ALLOWED(rel_type) ? RiskGroup::HIGH : RiskGroup::LOW;
                // Formation rates will remain constant across the dt timestep
                formation_rates[type] = society->GetRates(rel_type)->GetRateForAgeAndSexAndRiskGroup(m_age, m_gender, risk_group);
                cumulative_rate += formation_rates[type] ;
                if( max_rate < formation_rates[ type ] )
                {
                    max_rate = formation_rates[ type ];
                    max_rate_type = type;
                }
            }
        }

        while( cumulative_rate > 0.0f && is_any_available && ellapsed_time < dt )
        {
            if (LOG_LEVEL(DEBUG))
            {
                std::stringstream ss;
                ss << __FUNCTION__ << "individual " << suid.data << " availability { ";
                for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
                {
                    ss << available[i];
                    if( (i+1) < RelationshipType::COUNT )
                    {
                        ss << ", ";
                    }
                    else
                    {
                        ss << " ";
                    }
                }
                ss << "\n";
                LOG_DEBUG_F( ss.str().c_str() );
            }

            // At least one relationship could be formed
            // Advance total ellapsed time by time to next relationship
            ellapsed_time += GetRng()->expdist(cumulative_rate);
            if (ellapsed_time <= dt)
            {
                float random_draw = GetRng()->e() * cumulative_rate;
                float running_sum = 0.0f;
                int type=0;
                for (type = 0; type < RelationshipType::COUNT; type++)
                {
                    running_sum += formation_rates[type];
                    if (running_sum > random_draw)
                    {
                        break;
                    }
                }
                release_assert(type < RelationshipType::COUNT);

                LOG_DEBUG_F( "%s: individual %d joining %s PFA.\n", __FUNCTION__, suid.data, RelationshipType::pairs::lookup_key(type) );

                society->GetPFA(RelationshipType::Enum(type))->AddIndividual(this);
                enter_PFA_now = false;
                ++queued_relationships[type] ;
            }

            // Update statistics for the next round
            is_any_available = false;
            cumulative_rate = 0.0f;
            for( int type = 0; type < RelationshipType::COUNT; type++ )
            {
                available[type] = AvailableForRelationship( RelationshipType::Enum(type));  // uses queued_relationships what might have been updated
                is_any_available |= available[type];

                if( available[type] )
                {
                    cumulative_rate += formation_rates[type] ;
                }
                else
                {
                    formation_rates[ type ] = 0.0;
                }
            }
        } // while ellapsed_time < dt

        if( enter_PFA_now && (max_rate_type >= 0) )
        {
            society->GetPFA(RelationshipType::Enum(max_rate_type))->AddIndividual(this);
            enter_PFA_now = false;
            ++queued_relationships[max_rate_type] ;
            available[max_rate_type] = AvailableForRelationship( RelationshipType::Enum(max_rate_type));  // uses queued_relationships what might have been updated
        }
    }

    void
    IndividualHumanSTI::AddRelationship(
        IRelationship * pNewRelationship
    )
    {
        relationships.push_back(pNewRelationship);
        relationship_ids.insert( pNewRelationship->GetSuid() );

        RelationshipType::Enum relationship_type = pNewRelationship->GetType();

        if( !pNewRelationship->IsOutsidePFA() )
        {
            --queued_relationships[relationship_type] ;
            ++active_relationships[relationship_type] ;
            enter_relationship_now = false;

            // set slot bit
            uint64_t slot = GetOpenRelationshipSlot();
            uint64_t bitmask = uint64_t(1) << slot;
            relationshipSlots |= bitmask;
            release_assert(relationshipSlots <= SLOTS_FILLED);
            LOG_DEBUG_F( "%s: relationshipSlots = %d, individual = %d\n", __FUNCTION__, relationshipSlots, GetSuid().data );
            //slot2RelationshipDebugMap[ slot ] = pNewRelationship->GetSuid().data;
            LOG_DEBUG_F( "%s: Individual %d gave slot %d to relationship %d\n", __FUNCTION__, GetSuid().data, slot, pNewRelationship->GetSuid().data );

            if(IndividualHumanSTIConfig::min_days_between_adding_relationships > 0)
            {
                delay_between_adding_relationships_timer = IndividualHumanSTIConfig::min_days_between_adding_relationships;
            }
        }

        // DJK: Can these counters live elsewhere?  Either reporter or something parallel to interventions container, e.g. counters container
        num_lifetime_relationships[ int(relationship_type) ]++;
        last_6_month_relationships.push_back( std::make_pair( int( relationship_type ), float( parent->GetTime().time ) ) );
        last_12_month_relationships.push_back( std::make_pair( int( relationship_type ), float( parent->GetTime().time ) ) );

        if( IndividualHumanSTI::needs_census_data )
        {
            release_assert( num_unique_partners.size() > 0 );

            suids::suid partner_id = pNewRelationship->GetPartnerId( GetSuid() );
            for( int itp = 0; itp < UNIQUE_PARTNER_TIME_PERIODS.size(); ++itp )
            {
                PartnerIdToRelEndTimeMap_t& r_partner_map = num_unique_partners[ itp ][ int( relationship_type ) ];
                r_partner_map[ partner_id ] = FLT_MAX;
            }
        }
        if( broadcaster )
        {
            broadcaster->TriggerObservers(GetEventContext(), EventTrigger::EnteredRelationship);
        }
    }

    void
    IndividualHumanSTI::RemoveRelationship(
        IRelationship * pRelationship
    )
    {
        if( relationship_ids.find( pRelationship->GetSuid() ) == relationship_ids.end() )
        {
            return;
        }
        RemoveRelationshipFromList(pRelationship);
        relationship_ids.erase( pRelationship->GetSuid() );
        relationships_terminated.push_back( pRelationship );

        RelationshipType::Enum relationship_type = pRelationship->GetType();

        if( !pRelationship->IsOutsidePFA() )
        {
            // These are unsigned quantities, so we'll loop for wrap-around.
            --active_relationships[relationship_type] ;

            uint64_t slot = pRelationship->GetSlotNumberForPartner( GetGender() == Gender::FEMALE );
            uint64_t bitmask = uint64_t(1) << slot;
            relationshipSlots &= (~bitmask);
            release_assert(relationshipSlots < SLOTS_FILLED);
            LOG_DEBUG_F( "%s: relationshipSlots = %d, individual=%d\n", __FUNCTION__, relationshipSlots, GetSuid().data );
            LOG_DEBUG_F( "%s: individual %d freed up slot %d for relationship %d\n", __FUNCTION__, GetSuid().data, slot, pRelationship->GetSuid().data );
            //release_assert( slot2RelationshipDebugMap[ slot ] == pRelationship->GetSuid().data );
            //slot2RelationshipDebugMap[ slot ] = -1;

            delay_between_adding_relationships_timer = 0.0f;
        }

        if( IndividualHumanSTI::needs_census_data )
        {
            release_assert( num_unique_partners.size() > 0 );

            suids::suid partner_id = pRelationship->GetPartnerId( GetSuid() );
            for( int itp = 0; itp < UNIQUE_PARTNER_TIME_PERIODS.size(); ++itp )
            {
                PartnerIdToRelEndTimeMap_t& r_partner_map = num_unique_partners[ itp ][ int( relationship_type ) ];
                r_partner_map[ partner_id ] = float( parent->GetTime().time );
            }
        }

        // keep track of relationship that is exiting for those listening for the event.
        p_exiting_relationship = pRelationship;

        if( broadcaster )
        {
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::ExitedRelationship );
        }

        p_exiting_relationship = nullptr;
    }

    bool IndividualHumanSTI::IsBehavioralSuperSpreader() const
    {
        return IS_SUPER_SPREADER();
    }

    unsigned int IndividualHumanSTI::GetExtrarelationalFlags() const
    {
        unsigned int bitmask = 0;
        for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
        {
            bitmask |= EXTRA_RELATIONAL_ALLOWED(i);
        }
        unsigned int flags = promiscuity_flags & bitmask;
        return flags;
    }

    void IndividualHumanSTI::SetStiCoInfectionState()
    {
        has_other_sti_co_infection = true;
    }

    void IndividualHumanSTI::ClearStiCoInfectionState()
    {
        has_other_sti_co_infection = false;
    }

    bool IndividualHumanSTI::HasSTICoInfection() const
    {
        return has_other_sti_co_infection;
    }

    float IndividualHumanSTI::GetCoInfectiveAcquisitionFactor() const
    {
        return has_other_sti_co_infection ? IndividualHumanSTIConfig::sti_coinfection_acq_mult : 1.0f;
    }

    float IndividualHumanSTI::GetCoInfectiveTransmissionFactor() const
    {
        if( has_other_sti_co_infection )
        {
            return IndividualHumanSTIConfig::sti_coinfection_trans_mult;
        }
        else
        {
            return 1.0f;
        }
    }

    float IndividualHumanSTI::GetCoitalActRiskAcquisitionFactor() const
    {
        return m_pSTIInterventionsContainer->GetCoitalActRiskAcquisitionFactor();
    }

    float IndividualHumanSTI::GetCoitalActRiskTransmissionFactor() const
    {
        return m_pSTIInterventionsContainer->GetCoitalActRiskTransmissionFactor();
    }

    bool IndividualHumanSTI::IsCircumcised() const
    {
        return m_pSTIInterventionsContainer->IsCircumcised();
    }

    bool IndividualHumanSTI::IsPostDebut() const
    {
        return GetAge() >= GetDebutAge();
    }

    void IndividualHumanSTI::onEmigrating()
    {
        disengageFromSociety();
        relationships_terminated.clear();
        migrating_rel = nullptr;
    }

    void IndividualHumanSTI::onImmigrating()
    {
        LOG_DEBUG_F( "%s()\n", __FUNCTION__ );
        migrating_because_of_partner = false;
        migrating_rel = nullptr;

        release_assert( p_sti_node );
        auto society = p_sti_node->GetSociety();
        auto manager = p_sti_node->GetRelationshipManager();
        for( auto rel_id : relationship_ids )
        {
            IRelationship* p_rel = manager->GetRelationshipById( rel_id.data );
            release_assert( p_rel != nullptr );
            p_rel->Resume( society, this );
            relationships.push_back( p_rel );
            // already in relationship_ids
        }
    }

    bool IndividualHumanSTI::AvailableForRelationship(RelationshipType::Enum relType) const
    {
        unsigned int effective_rels[ RelationshipType::COUNT ];
        bool no_relationships_or_allowed_extra = true;
        unsigned int total_rels = 0;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            effective_rels[ type ] = active_relationships[type] + queued_relationships[type];
            total_rels += effective_rels[ type ];
            no_relationships_or_allowed_extra &= ( (effective_rels[type] == 0) || 
                                                   ( (effective_rels[type] > 0) && IS_EXTRA_ALLOWED(type)));
        }

        // --------------------------------------------------------------------
        // --- An individual is eligible for a relationship of this type if:
        // --- 1) Older than debut age
        // --- 2) Sufficient time has passed since last rel end
        // --- 3) No rels of this type, or extra-relational flag is present
        // --- 4) Num active plus queued rels is less than max
        // --------------------------------------------------------------------
        bool ret = ( (GetAge() >= sexual_debut_age) &&
                     (delay_between_adding_relationships_timer <= 0.0f) &&
                     no_relationships_or_allowed_extra &&
                     (effective_rels[relType] < max_relationships[relType]) &&
                     total_rels <= MAX_SLOTS
                   );


        return ret;
    }

    std::vector<IRelationship*>& IndividualHumanSTI::GetRelationships()
    {
        return relationships;
    }

    const std::vector<IRelationship*>& IndividualHumanSTI::GetRelationshipsTerminated()
    {
        return relationships_terminated;
    }

    IRelationship* IndividualHumanSTI::GetMigratingRelationship()
    {
        return migrating_rel;
    }

    // This method finds the lowest 0 in the relationshipSlots bitmask
    unsigned int IndividualHumanSTI::GetOpenRelationshipSlot() const
    {
        release_assert( relationshipSlots < SLOTS_FILLED );
        uint64_t bit = 1;
        for( unsigned int counter = 0 ; counter <= MAX_SLOTS ; ++counter )
        {
            if( (relationshipSlots & bit) == 0 )
            {
                LOG_DEBUG_F( "%s: Returning %d as first open slot for individual %d\n", __FUNCTION__, counter, suid.data );
                release_assert( counter <= MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES );
                return counter;
            }
            bit <<= 1;
        }
        std::stringstream ss;
        ss << "Cannot be in more than " << MAX_SLOTS << " simultaneous relationship." ;
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    NaturalNumber IndividualHumanSTI::GetLast6MonthRels() const
    {
        return last_6_month_relationships.size();
    }

    NaturalNumber IndividualHumanSTI::GetLast6MonthRels( RelationshipType::Enum ofType ) const
    {
        auto func = [ ofType ]( std::pair<int, float> p ) { return (ofType == p.first); };
        int num = std::count_if( last_6_month_relationships.begin(), last_6_month_relationships.end(), func );
        return num;
    }

    NaturalNumber IndividualHumanSTI::GetLast12MonthRels( RelationshipType::Enum ofType ) const
    {
        auto func = [ ofType ]( std::pair<int, float> p ) { return (ofType == p.first); };
        int num = std::count_if( last_12_month_relationships.begin(), last_12_month_relationships.end(), func );
        return num;
    }

    NaturalNumber IndividualHumanSTI::GetNumUniquePartners( int itp, int irel ) const
    {
        release_assert( IndividualHumanSTI::needs_census_data && (num_unique_partners.size() > 0) );
        return num_unique_partners[ itp ][ irel ].size();
    }

    NaturalNumber IndividualHumanSTI::GetLifetimeRelationshipCount() const
    {
        unsigned int sum = 0;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            sum += num_lifetime_relationships[type];
        }
        return sum;
    }

    NaturalNumber IndividualHumanSTI::GetLifetimeRelationshipCount( RelationshipType::Enum ofType ) const
    {
        return num_lifetime_relationships[ ofType ];
    }

    float IndividualHumanSTI::GetSimDayBorn() const
    {
        return sim_day_born;
    }

    float IndividualHumanSTI::GetDebutAge() const
    {
        return sexual_debut_age;
    }

    float IndividualHumanSTI::GetAgeAtInfection() const
    {
        return age_at_infection;
    }

    IRelationship* IndividualHumanSTI::GetExitingRelationship() const
    {
        return p_exiting_relationship;
    }

    const CoitalAct& IndividualHumanSTI::GetCurrentCoitalAct() const
    {
        return m_CurrentCoitalAct;
    }

    std::string IndividualHumanSTI::toString() const
    {
        std::ostringstream me;
        me << "id="
           << GetSuid().data
           << ",gender="
           << ( GetGender() == Gender::MALE ? "male" : "female" )
           << ",age="
           << GetAge()/DAYSPERYEAR
           << ",num_infections="
           << infections.size()
           << ",num_relationships="
           << relationships.size()
           << ",num_relationships_lifetime="
           << GetLifetimeRelationshipCount()
           << ",num_relationships_last_6_months="
           << last_6_month_relationships.size()
           << ",promiscuity_flags="
           << std::hex << static_cast<unsigned>(promiscuity_flags)
           ;
        return me.str();
    }

    void IndividualHumanSTI::SetContextTo( INodeContext* context )
    {
        LOG_DEBUG_F( "%s: individual %d setting context to 0x%08X.\n", __FUNCTION__, suid.data, context );

        // --------------------------------------------------------------------------
        // --- SetContextTo() is getting called twice in STI.  This is just trying to
        // --- to stop the extra execution.  Should probably fix the calling,
        // --- but I'm out of time.
        // --------------------------------------------------------------------------
        if( parent == context )
        {
            if( p_sti_node == nullptr )
            {
                parent->QueryInterface(GET_IID(INodeSTI), (void**)&p_sti_node);
            }
            return;
        }

        IndividualHuman::SetContextTo(context);
        if( context == nullptr )
        {
            p_sti_node = nullptr;
        }
        else
        {
            if (parent->QueryInterface(GET_IID(INodeSTI), (void**)&p_sti_node) == s_OK)
            {
                release_assert( p_sti_node );
                engageWithSociety();
            }
        }
    }

    void IndividualHumanSTI::CheckForMigration( float currenttime, float dt )
    {
        if( migrating_because_of_partner )
        {
            StateChange = HumanStateChange::Migrating;
        }
        else
        {
            IndividualHuman::CheckForMigration( currenttime, dt );
            if( StateChange == HumanStateChange::Migrating )
            {
                IIndividualEventBroadcaster* p_broadcaster = GetNodeEventContext()->GetIndividualEventBroadcaster();
                p_broadcaster->TriggerObservers( this->GetEventContext(), EventTrigger::STIPreEmigrating );

                release_assert( p_sti_node );
                auto manager = p_sti_node->GetRelationshipManager();

                while( relationships.size() > 0 )
                {
                    auto rel = relationships[ 0 ];
                    RelationshipMigrationAction::Enum migration_action = rel->GetMigrationAction( GetRng() );
                    if( migration_action == RelationshipMigrationAction::MIGRATE )
                    {
                        // -------------------------------
                        // --- Migrating the relationship 
                        // -------------------------------

                        IIndividualHumanSTI* ipartner = rel->GetPartner( this );
                        IndividualHumanSTI* partner = dynamic_cast<IndividualHumanSTI*>(ipartner);
                        release_assert( partner );

                        partner->migrating_because_of_partner = true ;
                        partner->migrating_rel                = rel;

                        partner->StateChange                  = HumanStateChange::Migrating;
                        partner->migration_destination        = this->migration_destination ;
                        partner->migration_type               = this->migration_type;
                        partner->migration_time_until_trip    = this->migration_time_until_trip;
                        partner->migration_time_at_destination= this->migration_time_at_destination;
                        partner->migration_will_return        = this->migration_will_return;
                        partner->migration_outbound           = this->migration_outbound;
                        partner->waypoints                    = this->waypoints;
                        partner->waypoints_trip_type          = this->waypoints_trip_type;

                        p_broadcaster->TriggerObservers( partner->GetEventContext(), EventTrigger::STIPreEmigrating );

                        rel->Migrate( migration_destination );

                        // -------------------------------------------------------------------------------------
                        // --- RM does NOT own deleting object.  Ownership is given to SimulationSTI.
                        // --- If the object just transfers nodes on the same core, then no deletion is required
                        // --- If it transfers cores, SimulationSTI will be responsible for deleting it.
                        // -------------------------------------------------------------------------------------
                        manager->RemoveRelationship( rel, true, false );
                        this->RemoveRelationshipFromList( rel );
                        partner->RemoveRelationshipFromList( rel );
                        manager->Emigrate( rel );

                        while( partner->relationships.size() > 0 )
                        {
                            IRelationship* partner_rel = partner->relationships[ 0 ];
                            partner_rel->Terminate( RelationshipTerminationReason::PARTNER_MIGRATING );
                            manager->RemoveRelationship( partner_rel, true, true ); // RM owns deleting the object
                        }
                    }
                    else if( migration_action == RelationshipMigrationAction::TERMINATE )
                    {
                        // -------------------------------------------------------------------
                        // --- Relationship ends permanently when the person leaves the Node.
                        // -------------------------------------------------------------------
                        rel->Terminate( RelationshipTerminationReason::SELF_MIGRATING );
                        manager->RemoveRelationship( rel, true, true ); // RM owns deleting the object
                    }
                    else if( migration_action == RelationshipMigrationAction::PAUSE )
                    {
                        // --------------------------------------------------------------------------------
                        // --- More permanent relationships can last/stay open until the person comes back
                        // --------------------------------------------------------------------------------
                        RelationshipState::Enum previous_state = rel->GetState();
                        IRelationship* p_leaving_rel = rel->Pause( this, migration_destination );

                        // see note above on Migrate() about relationship object ownership
                        manager->RemoveRelationship( p_leaving_rel, (previous_state == RelationshipState::PAUSED), false );
                        RemoveRelationshipFromList( rel );
                        p_leaving_rel->UpdatePaused();
                        manager->Emigrate( p_leaving_rel );
                    }
                    else
                    {
                        throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, 
                            "migration_action", 
                            migration_action, 
                            RelationshipMigrationAction::pairs::lookup_key( migration_action ) );
                    }
                }
            }
        }
    }

    void IndividualHumanSTI::engageWithSociety()
    {
        if( (p_sti_node != nullptr) && !IsDead() )
        {
            ISociety* society = p_sti_node->GetSociety();

            for( int type = 0; type < RelationshipType::COUNT; type++ )
            {
                society->GetPFA(RelationshipType::Enum(type))->RegisterIndividual( this );
            }
            UpdateSTINetworkParams( nullptr, nullptr );
        }
    }

    void IndividualHumanSTI::disengageFromSociety()
    {
        if( p_sti_node != nullptr )
        {
            ISociety* society = p_sti_node->GetSociety();

            for( int type = 0; type < RelationshipType::COUNT; type++ )
            {
                society->GetPFA(RelationshipType::Enum(type))->UnregisterIndividual(this);
                if (queued_relationships[type] > 0)
                {
                    LOG_DEBUG_F( "%s: individual %lu is in %s PFA - removing.\n", __FUNCTION__, suid.data, RelationshipType::pairs::lookup_key(type) );
                    society->GetPFA(RelationshipType::Enum(type))->RemoveIndividual(this);
                    queued_relationships[type] = 0;
                }
            }
        }
    }

    ProbabilityNumber IndividualHumanSTI::getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const
    {
        const Sigmoid& probs = m_pSTIInterventionsContainer->GetSTIBarrierProbabilitiesByRelType( pRelParams );
        float year = float(GetParent()->GetTime().Year());
        ProbabilityNumber prob = probs.variableWidthAndHeightSigmoid( year );
        //LOG_DEBUG_F( "%s: returning %f from Sigmoid::vWAHS( %f, %f, %f, %f, %f )\n", __FUNCTION__, (float) prob, year, probs.midyear, probs.rate, probs.early, probs.late );
        return prob;
    }

    void IndividualHumanSTI::ClearAssortivityIndexes()
    {
        for (int i = 0; i < RelationshipType::COUNT; ++i)
        {
            m_AssortivityIndex[i] = -1;
        }
    }

    int IndividualHumanSTI::GetAssortivityIndex( RelationshipType::Enum type ) const
    {
        return m_AssortivityIndex[ type ];
    }
    void IndividualHumanSTI::SetAssortivityIndex( RelationshipType::Enum type, int index )
    {
        m_AssortivityIndex[ type ] = index;
    }

    void IndividualHumanSTI::UpdateNumCoitalActs( uint32_t numActs )
    {
        unsigned int prev_total_coital_acts = m_TotalCoitalActs;

        m_TotalCoitalActs += numActs;

        if( (prev_total_coital_acts == 0) && (m_TotalCoitalActs > 0) && broadcaster )
        {
            bool prev_is_infected = IsInfected();

            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::FirstCoitalAct );

            if( !prev_is_infected && IsInfected() )
            {
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__,
                                                 "Do not try to infect people using the FirstCoitalAct event.\nThis can cause problems down stream." );
            }
        }
    }

    uint32_t IndividualHumanSTI::GetTotalCoitalActs() const
    {
        return m_TotalCoitalActs;
    }

    void IndividualHumanSTI::RemoveRelationshipFromList( IRelationship* pRel )
    {
        for( int i = 0; i < relationships.size(); ++i )
        {
            if( relationships[ i ]->GetSuid().data == pRel->GetSuid().data )
            {
                relationships[ i ] = relationships.back();
                relationships.pop_back();
                break;
            }
        }
    }

    void IndividualHumanSTI::StartNonPfaRelationships()
    {
        m_pSTIInterventionsContainer->StartNonPfaRelationships();
    }

    REGISTER_SERIALIZABLE(IndividualHumanSTI);

    void serialize_relationship_ids( IArchive& ar, std::set<suids::suid>& rRelIdSet )
    {
        size_t count = ar.IsWriter() ? rRelIdSet.size() : -1;

        ar.startArray(count);
        if (ar.IsWriter())
        {
            for( suids::suid rel_id : rRelIdSet )
            {
                ar & rel_id;
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                suids::suid rel_id;
                ar & rel_id;
                rRelIdSet.insert( rel_id );
            }
        }
        ar.endArray();
    }

    void serialize_pair_list( IArchive& ar, std::list < std::pair<int, float>>& pairList )
    {
        size_t count = ar.IsWriter() ? pairList.size() : -1;

        ar.startArray( count );
        if( ar.IsWriter() )
        {
            for( auto& entry : pairList )
            {
                int first = entry.first;
                float second = entry.second;
                ar.startObject();
                ar.labelElement( "first" ) & first;
                ar.labelElement( "second" ) & second;
                ar.endObject();
            }
        }
        else
        {
            for( size_t i = 0; i < count; ++i )
            {
                int first = 0;
                float second = 0.0;
                ar.startObject();
                ar.labelElement( "first" ) & first;
                ar.labelElement( "second" ) & second;
                ar.endObject();
                pairList.push_back( std::make_pair( first, second ) );
            }
        }
        ar.endArray();
    }

    void IndividualHumanSTI::serialize(IArchive& ar, IndividualHumanSTI* obj)
    {
        size_t rel_count = RelationshipType::COUNT;

        IndividualHuman::serialize( ar, obj );
        IndividualHumanSTI& human_sti = *obj;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Relationships are migrated per node/Relationshipmanager prior to people being migrated.
        // !!! We keep track of the relationship_ids so that when the person arrives at the new node,
        // !!! they can use the ids to get the relationship from the manager.  The manager is supposed
        // !!! to own the relationships.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //ar.labelElement("relationships"                           ); serialize_relationships( ar, human_sti.relationships );
        ar.labelElement("relationship_ids"                        ); serialize_relationship_ids( ar, human_sti.relationship_ids );
        ar.labelElement("max_relationships"                       ); ar.serialize( human_sti.max_relationships,    rel_count );
        ar.labelElement("queued_relationships"                    ); ar.serialize( human_sti.queued_relationships, rel_count );
        ar.labelElement("active_relationships"                    ); ar.serialize( human_sti.active_relationships, rel_count );
        ar.labelElement("migrating_because_of_partner"            ) & human_sti.migrating_because_of_partner;

        //ar.labelElement("migrating_rel") & human_sti.migrating_rel; temporary incase the person dies while attempting to migrate due to partner

        ar.labelElement("promiscuity_flags"                       ) & human_sti.promiscuity_flags;
        ar.labelElement("sim_day_born"                            ) & human_sti.sim_day_born;
        ar.labelElement("sexual_debut_age"                        ) & human_sti.sexual_debut_age;
        ar.labelElement("age_at_infection"                        ) & human_sti.age_at_infection;
        ar.labelElement("co_infective_factor"                     ) & human_sti.co_infective_factor;
        ar.labelElement("has_other_sti_co_infection"              ) & human_sti.has_other_sti_co_infection;
        ar.labelElement("transmissionInterventionsDisabled"       ) & human_sti.transmissionInterventionsDisabled;
        ar.labelElement("relationshipSlots"                       ) & human_sti.relationshipSlots;
        ar.labelElement("delay_between_adding_relationships_timer") & human_sti.delay_between_adding_relationships_timer;
        ar.labelElement("potential_exposure_flag"                 ) & human_sti.potential_exposure_flag;
        // m_pSTIInterventionsContainer
        // p_exiting_relationship
        // don't serialize m_CurrentCoitalAct because it's valid value is only temporary

        ar.labelElement("available"                              );  ar.serialize<bool>( human_sti.available, RelationshipType::COUNT );
        ar.labelElement("enter_PFA_now"                          ) & human_sti.enter_PFA_now;
        ar.labelElement("enter_relationship_now"                 ) & human_sti.enter_relationship_now;


        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Do not serialize relationships_terminated.  If the person is dead, we are just trying to get them
        // !!! home to clean up who is a resident.  The use of relationships_terminated should have been used already
        // !!! when the death event was broadcasted.  Hence, no one should need this later.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //ar.labelElement("relationships_terminated"                  ); serialize_relationships( ar, human_sti.relationships_terminated );
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        ar.labelElement("num_lifetime_relationships"              ); ar.serialize( human_sti.num_lifetime_relationships, rel_count );
        ar.labelElement("last_6_month_relationships"              ); serialize_pair_list( ar, human_sti.last_6_month_relationships );
        ar.labelElement("last_12_month_relationships"             ); serialize_pair_list( ar, human_sti.last_12_month_relationships );
        //ar.labelElement("slot2RelationshipDebugMap"               ) & human_sti.slot2RelationshipDebugMap;
        ar.labelElement("m_AssortivityIndex"                      ); ar.serialize( human_sti.m_AssortivityIndex, rel_count );
        ar.labelElement("m_TotalCoitalActs"                       ) & human_sti.m_TotalCoitalActs;
        ar.labelElement("num_unique_partners"                     ) & human_sti.num_unique_partners;

        if( human_sti.m_pSTIInterventionsContainer == nullptr )
        {
            human_sti.m_pSTIInterventionsContainer = static_cast<STIInterventionsContainer*>(human_sti.interventions);
        }
    }
}
