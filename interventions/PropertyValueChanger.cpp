
#include "stdafx.h"
#include "PropertyValueChanger.h"
#include "Debug.h" // for release_assert
#include "RANDOM.h"
#include "Common.h"             // for INFINITE_TIME
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "InterventionsContainer.h"
#include "DistributionFactory.h" // for Probability && DistributionFunction
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "EventTrigger.h"

SETUP_LOGGING( "PropertyValueChanger" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(PropertyValueChanger)

    BEGIN_QUERY_INTERFACE_BODY(PropertyValueChanger)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(PropertyValueChanger)

    PropertyValueChanger::PropertyValueChanger()
        : BaseIntervention()
        , target_property_key()
        , target_property_value()
        , probability(1.0)
        , revert(0.0f)
        , max_duration(0.0f)
        , action_timer(0.0f)
        , reversion_timer(0.0f)
    {
        //std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
        // done in base class ctor, not needed here.
        // primary_decay_time_constant...
        // cost_per_unit...
    }

    PropertyValueChanger::PropertyValueChanger( const PropertyValueChanger& rThat )
        : BaseIntervention( rThat )
        , target_property_key( rThat.target_property_key )
        , target_property_value( rThat.target_property_value )
        , probability( rThat.probability )
        , revert( rThat.revert )
        , max_duration( rThat.max_duration )
        , action_timer( 0.0f )
        , reversion_timer( rThat.reversion_timer )
    {
    }


    PropertyValueChanger::~PropertyValueChanger()
    {
        LOG_DEBUG("PropertyValueChanger destructor \n");
    }

    bool
    PropertyValueChanger::Configure(
        const Configuration * inputJson
    )
    {
        target_property_key.constraints   = IPKey::GetConstrainedStringConstraintKey();
        target_property_value.constraints = IPKey::GetConstrainedStringConstraintValue();

        initConfigTypeMap("Target_Property_Key", &target_property_key, PC_Target_Property_Key_DESC_TEXT );
        initConfigTypeMap("Target_Property_Value", &target_property_value, PC_Target_Property_Value_DESC_TEXT );
        initConfigTypeMap("Daily_Probability", &probability, PC_Daily_Probability_DESC_TEXT, 0.0f, 1.0f );
        initConfigTypeMap("Maximum_Duration", &max_duration, PC_Maximum_Duration_DESC_TEXT, -1.0f, FLT_MAX, FLT_MAX);
        initConfigTypeMap("Revert", &revert, PC_Revert_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            std::set<std::string> key_set = IPFactory::GetInstance()->GetKeysAsStringSet();
            if( key_set.find( target_property_key ) == key_set.end() )
            {
                std::stringstream ss;
                ss << "'Target_Property_Key' has an invalid value.  It must be one of the Individual Property keys/names." << std::endl;
                ss << "Possible values are: " << IPFactory::GetInstance()->GetKeysAsString();
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            std::set<std::string> value_set = IPFactory::GetInstance()->GetIP( target_property_key )->GetValues<IPKeyValueContainer>().GetValuesToStringSet();
            if( value_set.find( target_property_value ) == value_set.end() )
            {
                std::stringstream ss;
                ss << "'Target_Property_Value'(='" << target_property_value << "') has an invalid value for 'Target_Property_Key'='" << target_property_key << "'." << std::endl;
                ss << "The Individual Property '" << target_property_key << "' has the following values: " << std::endl;
                ss << IPFactory::GetInstance()->GetIP( target_property_key )->GetValues<IPKeyValueContainer>().GetValuesToString();
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return ret;
    }

    bool
    PropertyValueChanger::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if( probability < 1.0 )
        {
            std::unique_ptr<IDistribution> distribution( DistributionFactory::CreateDistribution( DistributionFunction::EXPONENTIAL_DISTRIBUTION ) );
            distribution->SetParameters( (double) probability, 0, 0 );
            action_timer = distribution->Calculate( context->GetParent()->GetRng() );
            
            if( action_timer > max_duration )
            {
                action_timer = FLT_MAX;
            }
            LOG_DEBUG_F( "Time until property change occurs = %f\n", action_timer );
        }
        return BaseIntervention::Distribute( context, pCCO );
    }

    void PropertyValueChanger::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        release_assert( expired == false );

        std::string current_prop_value = "";
        if( reversion_timer > 0 )
        {
            reversion_timer -= dt;
            if( reversion_timer <= 0 )
            {
                LOG_DEBUG_F( "Time to revert PropertyValueChanger.\n" );
                probability = 1.0;
            }
        }
        if( probability == 1.0 || action_timer < 0 )
        {
            if( revert )
            {
                // Need to ask individual (parent's parent) for current value of this property
                auto props = parent->GetEventContext()->GetProperties();
                current_prop_value = props->Get( IPKey( target_property_key ) ).GetValueAsString();
            }
            parent->GetInterventionsContext()->ChangeProperty( target_property_key.c_str(), target_property_value.c_str() );

            if( revert )
            {
                target_property_value = current_prop_value;
                probability = 0.0; // keep it simple for now, reversion is guaranteed
                reversion_timer = revert;
                action_timer= FLT_MAX;
                LOG_DEBUG_F( "Initializing reversion timer to %f\n", reversion_timer );
                revert = 0; // no more reversion from reversion
            }
            else
            {
                expired = true;
            }
        }
        action_timer -= dt;
    }

    void PropertyValueChanger::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );
    }

    int
    PropertyValueChanger::AddRef()
    {
        return BaseIntervention::AddRef();
    }

    int
    PropertyValueChanger::Release()
    {
        return BaseIntervention::Release();
    }

    REGISTER_SERIALIZABLE(PropertyValueChanger);

    void PropertyValueChanger::serialize(IArchive& ar, PropertyValueChanger* obj)
    {
        BaseIntervention::serialize( ar, obj );
        PropertyValueChanger& changer = *obj;

        ar.labelElement("target_property_key"  ) & changer.target_property_key;
        ar.labelElement("target_property_value") & changer.target_property_value;
        ar.labelElement("probability"          ) & changer.probability;
        ar.labelElement("revert"               ) & changer.revert;
        ar.labelElement("max_duration"         ) & changer.max_duration;
        ar.labelElement("action_timer"         ) & changer.action_timer;
        ar.labelElement("reversion_timer"      ) & changer.reversion_timer;

        if( !ar.IsWriter() )
        {
            changer.target_property_key.constraints   = IPKey::GetConstrainedStringConstraintKey();
            changer.target_property_value.constraints = IPKey::GetConstrainedStringConstraintValue();

            //TODO - Need to actual use the constrained string
        }
    }
}
