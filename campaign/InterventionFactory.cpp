
#include "stdafx.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "Log.h"
#include "ObjectFactoryTemplates.h"

SETUP_LOGGING( "InterventionFactory" )

namespace Kernel
{
    bool InterventionFactory::m_UseDefaults = false;

    IDistributableIntervention* InterventionFactory::CreateIntervention( const json::Element& rJsonElement,
                                                                         const std::string& rDataLocation,
                                                                         const char* parameterName,
                                                                         bool throwIfNull )
    {
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = m_UseDefaults;

        bool valid_input = IndividualIVFactory::getInstance()->ElementIsValid( rJsonElement, rDataLocation, parameterName );
        if(!valid_input && !throwIfNull)
        {
            return nullptr;
        }

        IDistributableIntervention* p_di = nullptr;
        p_di = IndividualIVFactory::getInstance()->CreateInstance( rJsonElement, rDataLocation, parameterName );
        if( p_di )
        {
            IndividualIVFactory::getInstance()->CheckSimType( p_di );
        }

        JsonConfigurable::_useDefaults = reset;

        return p_di;
    }

    void InterventionFactory::CreateInterventionList( const json::Element& rJsonElement,
                                                      const std::string& rDataLocation,
                                                      const char* parameterName,
                                                      std::vector<IDistributableIntervention*>& interventionsList )
    {
        if( rJsonElement.Type() == json::NULL_ELEMENT )
        {
            std::stringstream ss;
            ss << "'InterventionFactory' found the element to be NULL for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rJsonElement.Type() != json::ARRAY_ELEMENT )
        {
            std::stringstream ss;
            ss << "'InterventionFactory' found the element specified by '" << parameterName << "'\n"
               << "to NOT be a JSON ARRAY in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        const json::Array & interventions_array = json::QuickInterpreter(rJsonElement).As<json::Array>();

        if( interventions_array.Size() == 0 )
        {
            std::stringstream ss;
            ss << "'InterventionFactory' found zero elements in JSON for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( int idx = 0; idx < interventions_array.Size(); ++idx )
        {
            std::stringstream param_name;
            param_name << parameterName << "[" << idx << "]";

            const json::Element& r_array_element = interventions_array[ idx ];
            if( r_array_element.Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream ss;
                ss << "'InterventionFactory' found the element specified by '" << param_name.str() << "'\n"
                   << "to NOT be a JSON OBJECT in <" << rDataLocation << ">.";
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            const json::Object& json_obj = json_cast<const json::Object&>(interventions_array[idx]);

            // Instantiate and distribute interventions
            IDistributableIntervention* di = InterventionFactory::CreateIntervention( json_obj, rDataLocation, param_name.str().c_str(), true );
            interventionsList.push_back( di );
        }
    }

    INodeDistributableIntervention* InterventionFactory::CreateNDIIntervention( const json::Element& rJsonElement,
                                                                                const std::string& rDataLocation,
                                                                                const char* parameterName,
                                                                                bool throwIfNull )
    {
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = m_UseDefaults;

        bool valid_input = NodeIVFactory::getInstance()->ElementIsValid( rJsonElement, rDataLocation, parameterName );
        if(!valid_input && !throwIfNull)
        {
            return nullptr;
        }

        INodeDistributableIntervention* p_ndi = nullptr;
        p_ndi = NodeIVFactory::getInstance()->CreateInstance( rJsonElement, rDataLocation, parameterName );
        if( p_ndi )
        {
            NodeIVFactory::getInstance()->CheckSimType( p_ndi );
        }

        JsonConfigurable::_useDefaults = reset;

        return p_ndi;
    }

    void InterventionFactory::CreateNDIInterventionList( const json::Element& rJsonElement,
                                                         const std::string& rDataLocation,
                                                         const char* parameterName,
                                                         std::vector<INodeDistributableIntervention*>& interventionsList )
    {
        if( rJsonElement.Type() == json::NULL_ELEMENT )
        {
            std::stringstream ss;
            ss << "'InterventionFactory' found the element to be NULL for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rJsonElement.Type() != json::ARRAY_ELEMENT )
        {
            std::stringstream ss;
            ss << "'InterventionFactory' found the element specified by '" << parameterName << "'\n"
               << "to NOT be a JSON ARRAY in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        const json::Array & interventions_array = json::QuickInterpreter(rJsonElement).As<json::Array>();

        if( interventions_array.Size() == 0 )
        {
            std::stringstream ss;
            ss << "'InterventionFactory' found zero elements in JSON for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( int idx = 0; idx < interventions_array.Size(); ++idx )
        {
            std::stringstream param_name;
            param_name << parameterName << "[" << idx << "]";

            const json::Element& r_array_element = interventions_array[ idx ];
            if( r_array_element.Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream ss;
                ss << "'InterventionFactory' found the element specified by '" << param_name.str() << "'\n"
                   << "to NOT be a JSON OBJECT in <" << rDataLocation << ">.";
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            const json::Object& json_obj = json_cast<const json::Object&>(interventions_array[idx]);

            // Instantiate and distribute interventions
            INodeDistributableIntervention* di = InterventionFactory::CreateNDIIntervention( json_obj, rDataLocation, param_name.str().c_str(), true );
            interventionsList.push_back( di );
        }
    }

    void InterventionFactory::SetUseDefaults( bool useDefaults )
    {
        m_UseDefaults = useDefaults;
    }

    bool InterventionFactory::IsUsingDefaults()
    {
        return m_UseDefaults;
    }

    // Individual IV Factory
    IndividualIVFactory::IndividualIVFactory()
        : ObjectFactory<IDistributableIntervention, IndividualIVFactory>()
    {
    }

    IndividualIVFactory* IndividualIVFactory::_instance = nullptr;

    template IndividualIVFactory* ObjectFactory<IDistributableIntervention, IndividualIVFactory>::getInstance();

    // Node IV Factory
    NodeIVFactory::NodeIVFactory()
        : ObjectFactory<INodeDistributableIntervention, NodeIVFactory>()
    {
    }

    NodeIVFactory* NodeIVFactory::_instance = nullptr;

    template NodeIVFactory* ObjectFactory<INodeDistributableIntervention, NodeIVFactory>::getInstance();
}
