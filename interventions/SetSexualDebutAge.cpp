
#include "stdafx.h"
#include "SetSexualDebutAge.h"
#include "InterventionEnums.h"
#include "IIndividualHumanContext.h"
#include "NodeEventContext.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHuman.h"


SETUP_LOGGING( "SetSexualDebugAge" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( SetSexualDebutAge )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( SetSexualDebutAge )

    IMPLEMENT_FACTORY_REGISTERED( SetSexualDebutAge )
    
    SetSexualDebutAge::SetSexualDebutAge()
    : BaseIntervention()
    , m_setting_type( SettingType::CURRENT_AGE )
    , m_age( -1.0 )
    , m_DistrbutedEventTrigger()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    bool SetSexualDebutAge::Configure( const Configuration * inputJson )
    {
        initConfig( "Setting_Type", m_setting_type, inputJson, MetadataDescriptor::Enum( "setting_type", SSDA_Setting_Type_DESC_TEXT, MDD_ENUM_ARGS( SettingType ) ) );
        initConfigTypeMap( "Age_Years", &m_age, SSDA_Age_Years_DESC_TEXT, 0.0f, FLT_MAX, MAX_HUMAN_AGE, "Setting_Type", "USER_SPECIFIED" );
        initConfigTypeMap( "Distributed_Event_Trigger", &m_DistrbutedEventTrigger, Distributed_Event_Trigger_DESC_TEXT );

        bool is_configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && is_configured )
        {
            // convert age from years to days
            m_age = m_age * DAYSPERYEAR;
        }

        return is_configured;
    }

    bool SetSexualDebutAge::Distribute( IIndividualHumanInterventionsContext *context,
                                       ICampaignCostObserver * const pCCO )
    {
        bool ret = BaseIntervention::Distribute( context, pCCO );

        if( ret && !m_DistrbutedEventTrigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = context->GetParent()->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( context->GetParent()->GetEventContext(), m_DistrbutedEventTrigger );
        }
        return ret;
    }

    void SetSexualDebutAge::Update( float dt )
    {       
        IIndividualHumanSTI* individual;
        if( s_OK != parent->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&individual ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanSTI", "IIndividualHumanInterventionsContext" );
        }
        
        if( m_setting_type == SettingType::CURRENT_AGE )
        {
            IIndividualHuman* ih = nullptr;
            if( s_OK != parent->QueryInterface( GET_IID( IIndividualHuman ), (void**)&ih ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanSTI", "IIndividualHumanInterventionsContext" );
            }
            m_age = ih->GetAge();
        }
        
        individual->SetSexualDebutAge( m_age );
        expired = true;
    }

    REGISTER_SERIALIZABLE( SetSexualDebutAge );

    void SetSexualDebutAge::serialize( IArchive& ar, SetSexualDebutAge* obj )
    {
        BaseIntervention::serialize( ar, obj );
        SetSexualDebutAge& mc = *obj;
        ar.labelElement( "m_setting_type"           ) & (uint32_t&)mc.m_setting_type;
        ar.labelElement( "m_age"                    ) & mc.m_age;
        ar.labelElement( "m_DistrbutedEventTrigger" ) & mc.m_DistrbutedEventTrigger;
    }
}
