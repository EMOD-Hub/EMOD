
#include "stdafx.h"

#include "DemographicRestrictions.h"
#include "IndividualEventContext.h"
#include "IIndividualHuman.h"
#include "AdditionalRestrictionsFactory.h"

SETUP_LOGGING( "DemographicRestrictions" )

#define DEFAULT_DEMOGRAPHIC_COVERAGE (1.0)

namespace Kernel
{
    DemographicRestrictions::DemographicRestrictions( bool age_restrictions,
                                                      TargetDemographicType::Enum defaultTargetDemographic,
                                                      bool use_coverage,
                                                      const char* pDemographicCoverageDescText )
    : allow_age_restrictions(age_restrictions)
    , use_demographic_coverage( use_coverage )
    , demographic_coverage(DEFAULT_DEMOGRAPHIC_COVERAGE)
    , p_demographic_coverage_desc_text(pDemographicCoverageDescText)
    , default_target_demographic(defaultTargetDemographic)
    , target_demographic(default_target_demographic)
    , target_age_min_years(0)
    , target_age_max_years(FLT_MAX/ DAYSPERYEAR)
    , target_age_min_days(0)
    , target_age_max_days(FLT_MAX)
    , target_gender(TargetGender::All)
    , property_restrictions_set()
    , property_restrictions()
    , target_residents_only( false )
    , targeting_config()
    , additional_restrictions(nullptr)
    {
    }

    void DemographicRestrictions::ConfigureRestrictions( JsonConfigurable* pParent, const Configuration * inputJson )
    {
        if( use_demographic_coverage )
        {
            pParent->initConfigTypeMap( "Demographic_Coverage", 
                                        &demographic_coverage,
                                        p_demographic_coverage_desc_text,
                                        0.0, 
                                        1.0, 
                                        DEFAULT_DEMOGRAPHIC_COVERAGE/*, 
                                        "Intervention_Config.*.iv_type", 
                                        "IndividualTargeted"*/ );
        }

        release_assert( default_target_demographic == TargetDemographicType::Everyone );
        if( JsonConfigurable::_dryrun ||
            (default_target_demographic == TargetDemographicType::Everyone) || 
            inputJson->Exist("Target_Demographic") )
        {
            pParent->initConfig( "Target_Demographic", 
                                 target_demographic, 
                                 inputJson,
                                 MetadataDescriptor::Enum( "target_demographic", 
                                                           Target_Demographic_DESC_TEXT, 
                                                           MDD_ENUM_ARGS(TargetDemographicType))/*,
                                 "Intervention_Config.*.iv_type", 
                                 "IndividualTargeted"*/);
        }
        LOG_DEBUG_F( "Target_Demographic configured as = %s\n", TargetDemographicType::pairs::lookup_key( target_demographic ) );
        
        if( (target_demographic == TargetDemographicType::ExplicitAgeRanges         ) || 
            (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) ||
            JsonConfigurable::_dryrun )
        {
            if( !allow_age_restrictions && !JsonConfigurable::_dryrun )
            {
                std::ostringstream msg;
                msg << typeid(*pParent).name() << " does not allow age restrictions.  'Target_Demographic' cannot be 'ExplicitAgeRanges' or 'ExplicitAgeRangesAndGender'";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            pParent->initConfigTypeMap( "Target_Age_Min", &target_age_min_years, Target_Age_Min_DESC_TEXT, 0.0f, FLT_MAX/DAYSPERYEAR,                0.0f, "Target_Demographic", "ExplicitAgeRanges,ExplicitAgeRangesAndGender" );
            pParent->initConfigTypeMap( "Target_Age_Max", &target_age_max_years, Target_Age_Max_DESC_TEXT, 0.0f, FLT_MAX/DAYSPERYEAR, FLT_MAX/DAYSPERYEAR, "Target_Demographic", "ExplicitAgeRanges,ExplicitAgeRangesAndGender" );

            if( (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) || JsonConfigurable::_dryrun)
            {
                pParent->initConfig( "Target_Gender", target_gender, inputJson, MetadataDescriptor::Enum("target_gender", Target_Gender_DESC_TEXT, MDD_ENUM_ARGS(TargetGender)) ); 
            }
        }
        else if( (target_demographic == TargetDemographicType::ExplicitGender) || JsonConfigurable::_dryrun )
        {
            pParent->initConfig( "Target_Gender", target_gender, inputJson, MetadataDescriptor::Enum("target_gender", Target_Gender_DESC_TEXT, MDD_ENUM_ARGS(TargetGender)) ); 
        }

        // xpath-y way of saying that the possible values for prop restrictions comes from demographics file IP's.
        property_restrictions_set.value_source = IPKey::GetConstrainedStringConstraintKeyValue(); 
        pParent->initConfigTypeMap("Property_Restrictions", &property_restrictions_set, Property_Restrictions_DESC_TEXT /*, "Intervention_Config.*.iv_type", "IndividualTargeted"*/ );

        pParent->initConfigComplexType("Property_Restrictions_Within_Node", &property_restrictions, Property_Restrictions_Within_Node_DESC_TEXT /*, "Intervention_Config.*.iv_type", "IndividualTargeted"*/ );

        pParent->initConfigTypeMap( "Target_Residents_Only", &target_residents_only, Target_Residents_Only_DESC_TEXT, false );

        if (JsonConfigurable::_dryrun || inputJson->Exist("Targeting_Config"))
        {
            pParent->initConfigComplexType("Targeting_Config", &targeting_config, Targeting_Config_DESC_TEXT);
        }
    }

    void DemographicRestrictions::CheckConfiguration()  
    {
        if( target_age_min_years >= target_age_max_years )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Target_Age_Min", target_age_min_years, "Target_Age_Max", target_age_max_years, "Max must be > than Min" );
        }
        target_age_min_days = target_age_min_years * DAYSPERYEAR;
        target_age_max_days = target_age_max_years * DAYSPERYEAR;

        if( property_restrictions_set.size() > 0 )
        {
            if( property_restrictions.Size() > 0 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Cannot have both 'Property_Restrictions' and 'Property_Restrictions_Within_Node'." );
            }

            std::map< std::string, std::string > restriction_map;

            std::set< std::string > dupKeyHelperSet;
            for (const auto& prop : property_restrictions_set)
            {
                // parse, pre-colon is prop key, post is value
                size_t sep = prop.find( ':' );
                const std::string& szKey = prop.substr( 0, sep );
                const std::string& szVal = prop.substr( sep+1, prop.size()-sep );
                if( dupKeyHelperSet.count( szKey ) != 0 )
                {
                    std::string msg;
                    msg += "Duplicate keys in 'Property_Restrictions'. Since entries are AND-ed together, ";
                    msg += "this will always be an empty set since an individual can only have a single ";
                    msg += "value of a given key. Use 'Property_Restrictions_Within_Node' instead.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
                }
                dupKeyHelperSet.insert( szKey );
                restriction_map.insert( make_pair( szKey, szVal ) );
            }
            property_restrictions.Add( restriction_map );
        }

        bool valid_input = false;
        if(targeting_config._json.Type() != json::NULL_ELEMENT)
        {
            valid_input = AdditionalRestrictionsFactory::getInstance()->ElementIsValid(targeting_config._json,
                                                                                       "campaign.json",
                                                                                       "Targeting_Config");
        }
        if(valid_input)
        {
            additional_restrictions = AdditionalRestrictionsFactory::getInstance()->CreateInstance( targeting_config._json,
                                                                                                    "campaign.json",
                                                                                                    "Targeting_Config" );
        }
    }

    bool DemographicRestrictions::HasDefaultRestrictions() const
    {
        if( demographic_coverage         != DEFAULT_DEMOGRAPHIC_COVERAGE    ) return false;
        if( target_demographic           != TargetDemographicType::Everyone ) return false;
        if( target_residents_only        != false                           ) return false;
        if( property_restrictions.Size() >  0                               ) return false;
        if( additional_restrictions      != nullptr                         ) return false;

        return true;
    }

    bool DemographicRestrictions::IsQualified( const IIndividualHumanEventContext* pIndividual )
    {
        bool retQualifies = true;

        if( target_residents_only )
        {
            const IIndividualHuman* p_human = pIndividual->GetIndividualHumanConst();
            if( !p_human->AtHome() )
            {
                return false ;
            }
        }

        if( (target_demographic == TargetDemographicType::PossibleMothers) && !pIndividual->IsPossibleMother() )
        {
            LOG_DEBUG("Individual not given intervention because not possible mother\n");
            retQualifies = false;
        }
        else if( (target_demographic == TargetDemographicType::ExplicitAgeRanges         ) ||
                 (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) )
        {
            if( !allow_age_restrictions )
            {
                std::ostringstream msg;
                msg << "Age Restrictions are not allowed.  'Target_Demographic' cannot be 'ExplicitAgeRanges' or 'ExplicitAgeRangesAndGender'";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if( pIndividual->GetAge() < target_age_min_days )
            {
                LOG_DEBUG_F("Individual %lu not given intervention because too young (age=%f) for intervention min age (%f)\n", 
                            pIndividual->GetSuid().data, pIndividual->GetAge(), target_age_min_days);
                retQualifies = false;
            }
            else if( pIndividual->GetAge() > target_age_max_days )
            {
                LOG_DEBUG_F("Individual %lu not given intervention because too old (age=%f) for intervention max age (%f)\n", 
                    pIndividual->GetSuid().data, pIndividual->GetAge(), target_age_max_days);
                retQualifies = false;
            }

            if( retQualifies && (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) )
            {
                // Gender = 0 is MALE, Gender = 1 is FEMALE.  Should use Gender::Enum throughout code
                if( (pIndividual->GetGender() == 0) && (target_gender == TargetGender::Female) )
                {
                    retQualifies = false;
                }
                else if( (pIndividual->GetGender() == 1) && (target_gender == TargetGender::Male) )
                {
                    retQualifies = false;
                }
            }
        }
        else if( target_demographic == TargetDemographicType::ExplicitGender )
        {
            // Gender = 0 is MALE, Gender = 1 is FEMALE.  Should use Gender::Enum throughout code
            if( (pIndividual->GetGender() == 0) && (target_gender == TargetGender::Female) )
            {
                retQualifies = false;
            }
            else if( (pIndividual->GetGender() == 1) && (target_gender == TargetGender::Male) )
            {
                retQualifies = false;
            }
        }
        else if( target_demographic == TargetDemographicType::Pregnant )
        {
            if( !pIndividual->IsPregnant() )
            {
                retQualifies = false;
            }
        }

        if( retQualifies && (property_restrictions.Size() > 0) )
        {
            retQualifies = property_restrictions.Qualifies( *(const_cast<IIndividualHumanEventContext*>(pIndividual)->GetProperties()) );
        }
        else
        {
            LOG_DEBUG( "No property restrictions in event coordiantor applied.\n" );
        }

        if (retQualifies && additional_restrictions != nullptr)
        {
            retQualifies = additional_restrictions->IsQualified(const_cast<IIndividualHumanEventContext*>(pIndividual));
        }
        
        LOG_DEBUG_F( "Returning %d from %s\n", retQualifies, __FUNCTION__ );
        return retQualifies;
    }

    void DemographicRestrictions::SetDemographicCoverage( float coverage )
    {
        demographic_coverage = coverage;
    }

    float DemographicRestrictions::GetDemographicCoverage() const
    {
        return demographic_coverage;
    }

    TargetDemographicType::Enum DemographicRestrictions::GetTargetDemographic() const
    {
        return target_demographic;
    }

    TargetGender::Enum DemographicRestrictions::GetTargetGender() const
    {
        return target_gender;
    }

    float DemographicRestrictions::GetMinimumAge() const
    {
        return target_age_min_years;
    }

    float DemographicRestrictions::GetMaximumAge() const
    {
        return target_age_max_years;
    }

    std::string DemographicRestrictions::GetPropertyRestrictionsAsString() const
    {
        return property_restrictions.GetAsString();
    }
}
