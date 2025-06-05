
#pragma once

#include "ObjectFactory.h"
#ifndef WIN32
#include <cxxabi.h>
#endif

namespace Kernel
{
    template<class IObject, class Factory>
    Factory * ObjectFactory<IObject,Factory>::getInstance()
    {
        return (_instance != nullptr) ? _instance : _instance = new Factory();
    }

    template<class IObject, class Factory>
    ObjectFactory<IObject,Factory>::ObjectFactory( bool queryForReturnInterface )
        : m_RegisteredClasses()
        , m_FactorySchema()
        , m_QueryForReturnInterface( queryForReturnInterface )
    {
    }

    template<class IObject, class Factory>
    ObjectFactory<IObject,Factory>::~ObjectFactory()
    {
    }

    template<class IObject, class Factory>
    void ObjectFactory<IObject,Factory>::Register( const char *classname, instantiator_function_t _if )
    {
        std::string classnameString( classname );
        m_RegisteredClasses[ classnameString ] = _if;
    }

    template<class IObject, class Factory>
    json::QuickBuilder ObjectFactory<IObject,Factory>::GetSchema()
    {
        JsonConfigurable::_dryrun = true;
        for( auto& entry : m_RegisteredClasses )
        {
            // --------------------------------------------------------------
            // --- Create a fake JSON object with the "class" element defined
            // --- so the CreateInstance() method will construct the object
            // --------------------------------------------------------------
            const std::string& class_name = entry.first;
            json::Object fakeJson;
            fakeJson["class"] = json::String(class_name);
            instantiator_function_t creator = entry.second;

            // ----------------------
            // --- Create the object
            // ----------------------
            ISupports* pForSchema = creator();
            release_assert( pForSchema );

            // --------------------------------
            // --- Get the schema for the class
            // --------------------------------
            IConfigurable* p_config = nullptr;
            if( s_OK != pForSchema->QueryInterface( GET_IID( IConfigurable ), (void**)&p_config ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ret", "IConfigurable", "ISupports" );
            }

            Configuration* fakeConfig = Configuration::CopyFromElement( fakeJson );
            p_config->Configure( fakeConfig );
            json::QuickBuilder schema = p_config->GetSchema();

            // -------------------------------------------------------
            // --- Add the schema to the schema for the whole factory
            // -------------------------------------------------------
            ModifySchema( schema, pForSchema );
            schema[std::string("class")] = json::String(class_name);
            m_FactorySchema[ class_name ] = schema;

            delete fakeConfig;
            fakeConfig = nullptr;
        }

        json::QuickBuilder retSchema = json::QuickBuilder(m_FactorySchema);
        return retSchema;
    }

    template<class IObject, class Factory>
    IObject* ObjectFactory<IObject, Factory>::CreateInstance( const json::Element& rJsonElement,
                                                              const std::string& rDataLocation,
                                                              const char* parameterName )
    {
        ElementIsValid( rJsonElement, rDataLocation, parameterName, false );

        Configuration* pConfig = Configuration::CopyFromElement( rJsonElement, rDataLocation );
        IObject* obj = CreateInstanceFromSpecs<IObject>( pConfig, m_RegisteredClasses, m_QueryForReturnInterface );

        if( !obj )
        {
            std::stringstream ss;
            ss << "Error loading '" << GET_CONFIG_STRING( pConfig, "class" ) << "' via "
               << "'" << GetFactoryName() << "' for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        delete pConfig;
        return obj;
    }

    template<class IObject, class Factory>
    bool ObjectFactory<IObject, Factory>::ElementIsValid( const json::Element& rJsonElement,
                                                          const std::string& rDataLocation,
                                                          const char* parameterName,
                                                          bool skip_exceptions )
    {
        if( rJsonElement.Type() == json::NULL_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName() << "' found the element to be NULL for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rJsonElement.Type() != json::OBJECT_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName() << "' found the element specified by '" << parameterName << "'\n"
               << "to NOT be a JSON object in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        json::QuickInterpreter json_obj( rJsonElement );

        if( !json_obj.Exist( "class" ) )
        {
            if( !skip_exceptions )
            {
                std::stringstream json_text;
                json::Writer::Write( rJsonElement, json_text );

                std::stringstream ss;
                ss << "'"
                    << GetFactoryName()
                    << "' could not instantiate object from json.\n"
                    << "The 'class' parameter was not specified in parameter '" << parameterName << "'\n"
                    << "that has the following JSON:\n"
                    << json_text.str();
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            else
            {
                return false;
            }
        }

        std::string class_name = static_cast<std::string>(json_obj["class"].As<json::String>());

        if( m_RegisteredClasses.find( class_name ) == m_RegisteredClasses.end() )
        {
            if( !skip_exceptions )
            {
                std::stringstream json_text;
                json::Writer::Write( rJsonElement, json_text );

                std::stringstream ss;
                ss << "'" << GetFactoryName() << "' could not find class '" << class_name << "'.\n"
                   << "It was specified in parameter '" << parameterName << "' in <" << rDataLocation << ">.\n"
                   << "This parameter had the following JSON:\n"
                   << json_text.str() << "\n"
                   << "Valid classes for this parameter are:\n";
                for( auto entry : m_RegisteredClasses )
                {
                    ss << entry.first << "\n";
                }
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    template<class IObject, class Factory>
    std::string ObjectFactory<IObject, Factory>::GetFactoryName()
    {
        std::string factory_name = typeid(*this).name();
#ifdef WIN32
        factory_name = factory_name.substr( 14 ); // remove "class Kernel::"
#else
        factory_name = abi::__cxa_demangle( factory_name.c_str(), 0, 0, nullptr );
        factory_name = factory_name.substr( 8 ); // remove "Kernel::"
#endif
        return factory_name;
    }

    template<class IObject, class Factory>
    void ObjectFactory<IObject, Factory>::CheckSimType( ISupports* pObject )
    {
        if( JsonConfigurable::_dryrun )
        {
            return;
        }
        release_assert(pObject);

        std::string sim_type_str = GET_CONFIG_STRING( EnvPtr->Config, "Simulation_Type" );

        IConfigurable* p_config = nullptr;
        if( s_OK != pObject->QueryInterface( GET_IID( IConfigurable ), (void**)&p_config ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pObject", "IConfigurable", "IObject" );
        }


        json::Array sim_type_array = p_config->GetSimTypes();

        int num_types = sim_type_array.Size();
        assert( num_types > 0 );

        std::string first_sim_type = std::string( json::QuickInterpreter( sim_type_array[ 0 ] ).As<json::String>() );
        if( first_sim_type == std::string( "*" ) )
        {
            // wild card means it is valid for any simulation type

            if( !JsonConfigurable::_dryrun )
            {
                //clear the object of sim types to reduce memory
                p_config->ClearSchema();
            }
            return;
        }

        std::string supported;
        for( int i = 0; i < num_types; i++ )
        {
            std::string supported_sim_type = std::string(  json::QuickInterpreter( sim_type_array[ i ] ).As<json::String>() );
            if( sim_type_str == supported_sim_type )
            {
                if( !JsonConfigurable::_dryrun )
                {
                    //clear the object of sim types to reduce memory
                    p_config->ClearSchema();
                }
                return;
            }
            supported += "'" + supported_sim_type + "', ";
        }
        supported = supported.substr( 0, supported.length() - 2 );

        std::stringstream ss;
        ss << "The '" << typeid(*pObject).name() << "' object is not valid with the current 'Simulation_Type' (='" << sim_type_str << "').  ";
        ss << "This object is only supported for the following simulation types: " << supported;
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

}
