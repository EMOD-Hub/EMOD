
#pragma once

#include "Configure.h"

namespace Kernel
{
    // This template allows one to have a collection/vector of JsonConfigurable objects.
    // The object created from this will need to use the initConfigComplexType() since this
    // is a complex object.  The JSON will be an array of the JsonConfigurable object used
    // in the template, i.e. JsonConfigurableClass.
    template<class JsonConfigurableClass>
    class JsonConfigurableCollection : public JsonConfigurable, public IComplexJsonConfigurable
    {
    public:
        JsonConfigurableCollection( const std::string& rIdmTypeName )
            : JsonConfigurable()
            , m_IdmTypeName( rIdmTypeName )
            , m_Collection()
        { }

        JsonConfigurableCollection( const JsonConfigurableCollection& rMaster )
            : JsonConfigurable( rMaster )
            , m_IdmTypeName( rMaster.m_IdmTypeName )
            , m_Collection() // DO NOT COPY
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Do not copy the elements of m_Collection.  They are pointers to objects owned by rMaster.
            // !!! Implementers of the template must implement their own copy constructor so that they
            // !!! can copy the elements.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }

        virtual ~JsonConfigurableCollection()
        {
            for( JsonConfigurableClass* p_jcc : m_Collection )
            {
                delete p_jcc;
            }
        }

        //JsonConfigurable methods
        virtual int32_t AddRef() { return 1; }
        virtual int32_t Release() { return 1; }
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return QueryResult::e_NOINTERFACE; }

        // IComplexJsonConfigurable methods
        virtual bool  HasValidDefault() const override
        {
            return false;
        }

        virtual json::QuickBuilder GetSchema() override
        {
            JsonConfigurableClass* p_jcc = CreateObject();
            if( JsonConfigurable::_dryrun )
            {
                p_jcc->Configure( nullptr );
            }

            std::string object_schema_name = p_jcc->GetTypeName();
            std::string idm_type_schema = "idmType:" + object_schema_name;

            json::QuickBuilder schema( GetSchemaBase() );
            auto tn = JsonConfigurable::_typename_label();
            auto ts = JsonConfigurable::_typeschema_label();
            schema[ tn ] = json::String( idm_type_schema );
            schema[ "item_type" ] = json::String( object_schema_name );

            // Add the schema for the objects to be retrieved by JsonConfigurable::Configure()
            schema[ ts ] = p_jcc->GetSchema();

            delete p_jcc;

            return schema;
        }

        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override
        {
            // Exist(key) should have been called before calling this
            Configuration* p_config = Configuration::CopyFromElement( (*inputJson)[ key ], inputJson->GetDataLocation() );

            if( p_config->GetElement().Type() != json::ARRAY_ELEMENT )
            {
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*inputJson)[key], "Expected ARRAY of OBJECTs" );
            }

            const auto& json_array = json_cast<const json::Array&>((*p_config));
            for( auto data = json_array.Begin(); data != json_array.End(); ++data )
            {
                Configuration* p_object_config = Configuration::CopyFromElement( *data, inputJson->GetDataLocation() );

                JsonConfigurableClass* p_jcc = CreateObject();

                if( p_object_config->GetElement().Type() != json::OBJECT_ELEMENT )
                {
                    std::stringstream ss;
                    ss << "Expected ARRAY of OBJECTs of type '" << p_jcc->GetTypeName() << "'";
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*inputJson)[key], ss.str().c_str() );
                }

                try
                {
                    p_jcc->Configure( p_object_config );
                }
                catch( json::Exception& )
                {
                    std::stringstream ss;
                    ss << "JSON Error while reading OBECT of type '" << p_jcc->GetTypeName() << "'";
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*p_object_config), ss.str().c_str() );
                }
                Add( p_jcc );

                delete p_object_config;
            }
            delete p_config;
        }

        // Other methods
        virtual void CheckConfiguration()
        {
        }

        const std::string& GetCollectionName() const
        {
            return m_IdmTypeName;
        }

        virtual void Add( JsonConfigurableClass* pJcc )
        {
            m_Collection.push_back( pJcc );
        }

        int Size() const
        {
            return m_Collection.size();
        }

        void Clear()
        {
            m_Collection.clear();
        }

        JsonConfigurableClass* operator[]( int index )
        {
            return m_Collection[ index ];
        }

        const JsonConfigurableClass* operator[]( int index ) const
        {
            return m_Collection[ index ];
        }

        static void serialize( IArchive& ar, JsonConfigurableCollection<JsonConfigurableClass>& rJcc )
        {
            size_t count = ar.IsWriter() ? rJcc.m_Collection.size() : -1;

            ar.startArray( count );
            if( ar.IsWriter() )
            {
                for( JsonConfigurableClass* p_obj : rJcc.m_Collection )
                {
                    ar.startObject();
                        ar & *p_obj;
                    ar.endObject();
                }
            }
            else
            {
                for( size_t i = 0; i < count; ++i )
                {
                    JsonConfigurableClass* p_obj = rJcc.CreateObject();
                    ar.startObject();
                        ar & *p_obj;
                    ar.endObject();
                    rJcc.m_Collection.push_back( p_obj );
                }
            }
            ar.endArray();
        }

    protected:
        virtual JsonConfigurableClass* CreateObject() = 0;

        std::string m_IdmTypeName;
        std::vector<JsonConfigurableClass*> m_Collection;
    };
}