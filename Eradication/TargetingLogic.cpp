
#include "stdafx.h"

#include "TargetingLogic.h"
#include "AdditionalRestrictionsFactory.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- OrAndCollection
    // ------------------------------------------------------------------------

    OrAndCollection::OrAndCollection()
        : m_Restrictions()
        , m_JsonSchemaBase()
    {
    }

    OrAndCollection::~OrAndCollection()
    {
        for( auto& r_outer : m_Restrictions )
        {
            for( auto p_ar : r_outer )
            {
                delete p_ar;
            }
        }
        m_Restrictions.clear();
    }

    void OrAndCollection::ConfigureFromJsonAndKey(const Configuration* inputJson, const std::string& key)
    {
        json::QuickInterpreter qi_2d( (*inputJson)[ key ] );
        if( qi_2d.operator const json::Element &().Type() != json::ARRAY_ELEMENT )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                          key.c_str(), (*inputJson)[key], "Expected 2D ARRAY of OBJECTs of type 'AdditionalTargetingConfig'" );
        }

        const auto& json_array = json_cast<const json::Array&>((*inputJson)[key]);
        for (auto data = json_array.Begin(); data != json_array.End(); ++data)
        {
            if( data->Type() != json::ARRAY_ELEMENT )
            {
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                              key.c_str(), (*inputJson)[key], "Expected 2D ARRAY of OBJECTs of type 'AdditionalTargetingConfig'" );

            }
            const auto& json_array_inner = json_cast<const json::Array&>((*data));

            vector<IAdditionalRestrictions*> innerm_Restrictions;
            for (auto data_inner = json_array_inner.Begin(); data_inner != json_array_inner.End(); ++data_inner)
            {
                if( data_inner->Type() != json::OBJECT_ELEMENT )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                                  key.c_str(), (*inputJson)[key], "Expected 2D ARRAY of OBJECTs of type 'AdditionalTargetingConfig'" );

                }

                Configuration* p_object_config = Configuration::CopyFromElement(*data_inner, inputJson->GetDataLocation());

                AdditionalTargetingConfig targeting_config(p_object_config);
                IAdditionalRestrictions* additionalm_Restrictions = AdditionalRestrictionsFactory::getInstance()->CreateInstance( targeting_config._json,
                                                                                                                                  inputJson->GetDataLocation(),
                                                                                                                                  key.c_str(),
                                                                                                                                  false );
                innerm_Restrictions.push_back(additionalm_Restrictions);

                delete p_object_config;
            }
            m_Restrictions.push_back(innerm_Restrictions);
        }
    }

    void OrAndCollection::CheckConfiguration( const char* parameterName )
    {
        if( m_Restrictions.size() == 0 )
        {
            std::stringstream ss;
            ss << "'" << parameterName << "' cannot be an empty array.\n";
            ss << "The user must define restrictions in this 2D array.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        for( auto& r_outer : m_Restrictions )
        {
            if( r_outer.size() == 0 )
            {
                std::stringstream ss;
                ss << "'" << parameterName << "' cannot have an empty inner array.\n";
                ss << "This is a 2D array and each inner array must not be empty.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    json::QuickBuilder OrAndCollection::GetSchema()
    {
        json::QuickBuilder schema = json::QuickBuilder( m_JsonSchemaBase );
        auto tn = "type_name";// JsonConfigurable::_typename_label();
        auto ts = "type_schema"; // JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "Vector2d idmType:AdditionalRestrictions" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["type"] = json::String( "Vector idmType:AdditionalRestrictions" );
        schema[ ts ][0]["description"] = json::String( "Two dimensional array of AdditionalRestrictions objects." );
        schema[ ts ][0]["default"] = json::Array();
        return schema;
    }

    bool OrAndCollection::HasValidDefault() const
    {
        return false;
    }

    // ------------------------------------------------------------------------
    // --- TargetingLogic
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(TargetingLogic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(TargetingLogic)

    IMPLEMENT_FACTORY_REGISTERED(TargetingLogic)
    REGISTER_SERIALIZABLE(TargetingLogic)

    TargetingLogic::TargetingLogic()
        : JsonConfigurable()
        , m_CompareTo(true)
        , m_OrAndLogic()
    {
        initSimTypes( 1, "*");
    }

    TargetingLogic::~TargetingLogic()
    {
    }

    bool TargetingLogic::Configure(const Configuration* config)
    {
        initConfigTypeMap( "Is_Equal_To", &m_CompareTo, AR_Is_Equal_To_DESC_TEXT, true );
        initConfigComplexType( "Logic", &m_OrAndLogic, AR_Logic_DESC_TEXT );

        bool configured = JsonConfigurable::Configure(config);
        if( configured && !JsonConfigurable::_dryrun )
        {
            m_OrAndLogic.CheckConfiguration( "Logic" );
        }
        return configured;
    }
    
    bool TargetingLogic::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        bool result = false;

        for (auto& outer : m_OrAndLogic.m_Restrictions)
        {
            bool result_inner = true;
            for (auto inner : outer)
            {
                result_inner &= inner->IsQualified( pContext );
            }
            result |= result_inner;
        }

        return (result == m_CompareTo);
    }

    void TargetingLogic::serialize(IArchive& ar, TargetingLogic* obj)
    {
        // TODO: implement me
        release_assert( false );
    }
}