
#include "stdafx.h"

#include "AdditionalRestrictions.h"
#include "IndividualEventContext.h"
#include "Interventions.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HasIntervention)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasIntervention)

    IMPLEMENT_FACTORY_REGISTERED(HasIntervention)
    REGISTER_SERIALIZABLE(HasIntervention)

    HasIntervention::HasIntervention()
        : AdditionalRestrictionsAbstract()
        , m_InterventionName()
    {
        initSimTypes( 1, "*");
    }

    bool HasIntervention::Configure(const Configuration* config)
    {
        std::string tmp_name;
        initConfigTypeMap("Intervention_Name", &tmp_name, AR_Intervention_Name_DESC_TEXT, "");

        bool ret = AdditionalRestrictionsAbstract::Configure(config);

        if( ret && !JsonConfigurable::_dryrun ) 
        {
            m_InterventionName = tmp_name;
            if( m_InterventionName.empty() )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'Intervention_Name' must be provided with a non-empty string.");
            }
        }

        return ret;
    }

    bool HasIntervention::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        return (pContext->GetInterventionsContext()->ContainsExistingByName(m_InterventionName) == m_CompareTo);
    }

    void HasIntervention::serialize(IArchive& ar, HasIntervention* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(HasIP)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasIP)

    IMPLEMENT_FACTORY_REGISTERED(HasIP)
    REGISTER_SERIALIZABLE(HasIP)

    HasIP::HasIP() 
        : AdditionalRestrictionsAbstract()
        , m_IPKeyValue()
    {
        initSimTypes( 1, "*");
    }

    bool HasIP::Configure(const Configuration* config)
    {
        IPKeyValueParameter ip_key_value;
        initConfigTypeMap("IP_Key_Value", &ip_key_value, AR_IP_Key_Value_DESC_TEXT);

        bool ret = AdditionalRestrictionsAbstract::Configure(config);
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_IPKeyValue = ip_key_value;
            if( !m_IPKeyValue.IsValid() )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "'IP_Key_Value' must be defined and cannot be empty string.");
            }
        }
        return ret;
    }

    bool HasIP::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        return (pContext->GetProperties()->Contains( m_IPKeyValue ) == m_CompareTo);
    }

    void HasIP::serialize(IArchive& ar, HasIP* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(IsPregnant)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(IsPregnant)

    IMPLEMENT_FACTORY_REGISTERED(IsPregnant)
    REGISTER_SERIALIZABLE(IsPregnant)

    IsPregnant::IsPregnant() 
        : AdditionalRestrictionsAbstract()
    {
        initSimTypes( 1, "*");
    }

    bool IsPregnant::Configure(const Configuration* config)
    {
        bool ret = AdditionalRestrictionsAbstract::Configure(config);
        if( ret && !JsonConfigurable::_dryrun )
        {
        }
        return ret;
    }

    bool IsPregnant::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        return (pContext->IsPregnant() == m_CompareTo);
    }

    void IsPregnant::serialize(IArchive& ar, IsPregnant* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

}