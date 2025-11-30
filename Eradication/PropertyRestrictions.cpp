
#include "stdafx.h"
#ifndef WIN32
#include <cxxabi.h>
#endif

#include "PropertyRestrictions.h"

SETUP_LOGGING( "PropertyRestrictions" )

namespace Kernel
{
    template<class Key, class KeyValue, class Container>
    PropertyRestrictions<Key,KeyValue,Container>::PropertyRestrictions()
        : JsonConfigurable()
        , _restrictions()
    {
    }

    template<class Key, class KeyValue, class Container>
    int PropertyRestrictions<Key, KeyValue, Container>::Size() const
    {
        return _restrictions.size();
    }

    template<class Key, class KeyValue, class Container>
    void PropertyRestrictions<Key, KeyValue, Container>::ConfigureFromJsonAndKey( const Configuration * inputJson, const std::string& key )
    {
        // Make this optional.
        if( inputJson->Exist( key ) == false )
        {
            return;
        }

        // We have a list of json objects. The format/logic is as follows. For input:
        // [
        //  { "Character": "Good", "Income": "High" },
        //  { "Character": "Bad", "Income": "Low" }
        // ]
        // We give the intervention if the individuals has
        // Good Character AND High Income OR Bad Character AND Low Income.
        // So we AND together the elements of each json object and OR together these 
        // calculated truth values of the elements of the json array.
        json::QuickInterpreter pr_qi = (*inputJson)[ key ];

        if( pr_qi.operator const json::Element &().Type() != json::ARRAY_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << key << "' is NOT an ARRAY in <" << inputJson->GetDataLocation() << ">.";
            throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), *inputJson, ss.str().c_str() );
        }

        json::QuickInterpreter s2sarray = (*inputJson)[key].As<json::Array>();
        for( int idx=0; idx < (*inputJson)[key].As<json::Array>().Size(); idx++ )
        {
            Container container;

            if( s2sarray[ idx ].operator const json::Element &().Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream ss;
                ss << "'" << key << "' is NOT an ARRAY of OBJECTs in <" << inputJson->GetDataLocation() << ">.";
                throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), *inputJson, ss.str().c_str() );
            }

            auto json_map = s2sarray[idx].As<json::Object>();
            for( auto data = json_map.Begin();
                      data != json_map.End();
                      ++data )
            {
                std::string prop_key = data->name;

                if( s2sarray[ idx ][ prop_key ].operator const json::Element &().Type() != json::STRING_ELEMENT )
                {
                    std::stringstream ss;
                    ss << "'" << key << "' has an OBJECT where the parameter '" << prop_key << "' is NOT a STRING in <" << inputJson->GetDataLocation() << ">.";
                    throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), *inputJson, ss.str().c_str() );
                }
                std::string value = (std::string)s2sarray[ idx ][ prop_key ].As< json::String >();
                KeyValue kv( prop_key, value );
                container.Add( kv );
            }
            _restrictions.push_back( container );
        }
    }

    template<class Key, class KeyValue, class Container>
    json::QuickBuilder PropertyRestrictions<Key, KeyValue, Container>::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();

        std::string key_name = typeid(Key).name();
#ifdef WIN32
        key_name = key_name.substr( 14 ); // remove "class Kernel::"
#else
        key_name = abi::__cxa_demangle( key_name.c_str(), 0, 0, nullptr );
        key_name = key_name.substr( 8 ); // remove "Kernel::"
#endif

        // this is kind of hacky, but there only two types right now.
        if( key_name == "IPKey" )
        {
            schema[ tn ] = json::String( "idmType:PropertyRestrictions" );
        }
        else
        {
            schema[ tn ] = json::String( "idmType:NodePropertyRestrictions" );
        }

        schema[ ts ] = json::Object();
        schema[ ts ][ "default" ] = json::Array();
        schema[ ts ][ "description" ] = json::String(PR_Schema_Type_DESC_TEXT);
        schema[ ts ][ "type" ] = json::String("Vector PropertyRestrictions");

        return schema;
    }

    template<class Key, class KeyValue, class Container>
    void PropertyRestrictions<Key, KeyValue, Container>::Add( std::map< std::string, std::string >& rMap )
    {
        Container container;
        for( auto& entry : rMap )
        {
            KeyValue kv( entry.first, entry.second );
            container.Add( kv );
        }
        _restrictions.push_back( container );
    }

    template<class Key, class KeyValue, class Container>
    void PropertyRestrictions<Key, KeyValue, Container>::Add(KeyValue& kv)
    {
        Container container;
        container.Add(kv);
        _restrictions.push_back(container);
    }

    template<class Key, class KeyValue, class Container>
    bool PropertyRestrictions<Key, KeyValue, Container>::Qualifies( const Container& rPropertiesContainer ) const
    {
        bool qualifies = true;

        // individual has to have one of these properties
        for( const Container& container : _restrictions)
        {
            qualifies = false;
            bool meets_property_restriction_criteria = true;
            for( auto kv : container )
            {
                if( rPropertiesContainer.Contains( kv ) )
                {
                    LOG_DEBUG_F( "Partial property restriction: constraint is %s.\n", kv.ToString().c_str() );
                    continue; // we're good
                }
                else
                {
                    meets_property_restriction_criteria = false;
#ifdef WIN32
                    LOG_DEBUG_F( "Person does not get the intervention because the allowed property is %s and the person is %s.\n", 
                                 kv.ToString().c_str(), rPropertiesContainer.Get( kv.GetKey<Key>() ).ToString().c_str() );
#endif
                    break;
                }
            }
            // If verified, we're done since these are OR-ed together
            if( meets_property_restriction_criteria )
            {
                qualifies = true;
                LOG_DEBUG_F( "Individual meets at least 1 of the OR-ed together property restriction conditions. Not checking the rest.\n" );
                break;
            }
        }

        return qualifies;
    }

    template<class Key, class KeyValue, class Container>
    std::string PropertyRestrictions<Key, KeyValue, Container>::GetAsString() const
    {
        std::string restriction_str ;
        if( _restrictions.size() > 0 )
        {
            for( Container container : _restrictions )
            {
                restriction_str += "[ ";
                for( KeyValue kv : container )
                {
                    std::string prop = kv.ToString();
                    restriction_str += "'"+ prop +"', " ;
                }
                restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
                restriction_str += " ], ";
            }
            restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
        }
        return restriction_str;
    }

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template class PropertyRestrictions<IPKey, IPKeyValue, IPKeyValueContainer>;
    template class PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer>;

}
