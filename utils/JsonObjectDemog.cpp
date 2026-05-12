#include <stdafx.h>
#include <cstdio>
#include <string>
#include <assert.h>

#include <sstream>
#include <functional>

#include "Log.h"
#include "Exceptions.h"
#include "FileSystem.h"

#include "JsonObjectDemog.h"

#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#pragma warning(disable : 4996) //ignore depracation warning about strcpy

SETUP_LOGGING( "JsonObjectDemog" )

using namespace std;

namespace Kernel {

    JsonObjectDemog::JsonObjectDemog( JsonObjectType jot )
        : m_Key()
        , m_pDocument()
        , m_pValue(nullptr)
    {
        if( jot != JSON_OBJECT_NULL )
        {
            rapidjson::Document* p_doc = new rapidjson::Document();
            shared_ptr<rapidjson::Document> shared_doc( p_doc ) ;
            m_pDocument = shared_doc ;
            m_pValue = p_doc ;
            m_Key = "root" ;
            if( jot == JSON_OBJECT_OBJECT )
            {
                p_doc->SetObject();
            }
            else if( jot == JSON_OBJECT_ARRAY )
            {
                p_doc->SetArray();
            }
            else
            {
                ostringstream s;
                s << "Unknown JsonObject enum = " << jot ;
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
            }
        }
    }

    JsonObjectDemog::JsonObjectDemog( const std::string& rKey, void* pValue, std::shared_ptr<void> pDocument )
        : m_Key(rKey)
        , m_pDocument(pDocument)
        , m_pValue( pValue )
    {
    }

    JsonObjectDemog::JsonObjectDemog( const JsonObjectDemog& rThat )
        : m_Key( rThat.m_Key )
        , m_pDocument( rThat.m_pDocument )
        , m_pValue( rThat.m_pValue )
    {
    }

    JsonObjectDemog::~JsonObjectDemog()
    {
    }

    JsonObjectDemog& JsonObjectDemog::operator=( const JsonObjectDemog& rThat )
    {
        if( this != &rThat )
        {
            this->m_Key           = rThat.m_Key ;
            this->m_pDocument     = rThat.m_pDocument ;
            this->m_pValue        = rThat.m_pValue ;
        }
        return *this ;
    }

    bool JsonObjectDemog::operator==( const JsonObjectDemog& rThat ) const 
    {
        if( this->m_Key != rThat.m_Key ) return false ;

        if( (this->m_pDocument == nullptr) && (rThat.m_pDocument != nullptr) ) return false ;
        if( (this->m_pDocument != nullptr) && (rThat.m_pDocument == nullptr) ) return false ;
        if( (this->m_pDocument != nullptr) && (rThat.m_pDocument != nullptr) )
        {
            // -----------------------------------------------------------------------
            // --- RapidJson does not support the equality operator.  Fake it for now
            // -----------------------------------------------------------------------
            if( this->m_pDocument.get() != rThat.m_pDocument.get() ) return false ;
        }

        if( (this->m_pValue == nullptr) && (rThat.m_pValue != nullptr) ) return false ;
        if( (this->m_pValue != nullptr) && (rThat.m_pValue == nullptr) ) return false ;
        if( (this->m_pValue != nullptr) && (rThat.m_pValue != nullptr) )
        {
            // -----------------------------------------------------------------------
            // --- RapidJson does not support the equality operator.  Fake it for now
            // -----------------------------------------------------------------------
            if( this->m_pValue != rThat.m_pValue ) return false ;
        }

        return true ;
    }

    bool JsonObjectDemog::operator!=( const JsonObjectDemog& rThat ) const
    {
        return !(*this == rThat);
    }

    void JsonObjectDemog::WriteToFile( const char* filename )
    {
        JsonWriterDemog writer( true ) ;
        writer << *this ;

        std::string text = writer.PrettyText();

        std::ofstream json_file;
        FileSystem::OpenFileForWriting( json_file, filename );
        json_file << text ;
        json_file.close();
    }

    // ------------------------------------------------------
    // --- Methods for creating an object from a string and
    // --- for creating the string from the object
    // ------------------------------------------------------

    void JsonObjectDemog::Parse( const char* jsBuffer, const char* filename )
    {
        rapidjson::Document* p_doc = new rapidjson::Document();
        shared_ptr<rapidjson::Document> shared_doc( p_doc ) ;
        m_pDocument = shared_doc ;
        m_pValue = p_doc ;
        m_Key = "root" ;

        p_doc->Parse<0>(jsBuffer);

        bool success = !p_doc->HasParseError();

        if (!success)
        {
            std::ofstream json_file;
            FileSystem::OpenFileForWriting( json_file, "invalid.json" );
            json_file << jsBuffer;
            json_file.close();

            ostringstream s;
            if( filename != nullptr )
            {
                s << filename << ": ";
            }
            s << "Failed to parse incoming text. " << p_doc->GetParseError() << " at character=" << p_doc->GetErrorOffset() << " / line number=" << p_doc->GetLineNumber() << endl;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }
    }

    void JsonObjectDemog::ParseFile(const char* filename)
    {
        std::string* p_buffer = FileSystem::ReadFile( filename );
        try
        {
            Parse( p_buffer->c_str(), filename );
        }
        catch( DetailedException& )
        {
            delete p_buffer ;
            throw ;
        }
        delete p_buffer ;
    }

    std::string JsonObjectDemog::ToString() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::GenericStringBuffer< rapidjson::UTF8<> > buffer;
        rapidjson::Writer< rapidjson::GenericStringBuffer< rapidjson::UTF8<> > > writer(buffer);

        r_value.Accept(writer);

        std::string ret_str( buffer.GetString() );

        return ret_str ;
    }

    // ------------------------------------------------------
    // --- Methods for querying information about the object
    // ------------------------------------------------------

    std::string JsonObjectDemog::GetKey() const
    {
        return m_Key ;
    }

    bool JsonObjectDemog::Contains(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        return r_value.HasMember(key);
    }

    bool JsonObjectDemog::IsNull() const
    {
        if( m_pValue == nullptr )
        {
            return true ;
        }
        else
        {
            rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

            return r_value.IsNull();
        }
    }

    bool JsonObjectDemog::IsArray() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        return r_value.IsArray();
    }

    bool JsonObjectDemog::IsObject() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        return r_value.IsObject();
    }

    std::string JsonObjectDemog::GetTypeName() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        std::string type_name ;
        rapidjson::Type type = r_value.GetType();
        switch( type )
        {
            case rapidjson::kNullType:
                type_name = "Null" ;
                break;
            case rapidjson::kFalseType:
                type_name = "Bool" ;
                break;
            case rapidjson::kTrueType:
                type_name = "Bool" ;
                break;
            case rapidjson::kObjectType:
                type_name = "Object" ;
                break;
            case rapidjson::kArrayType:
                type_name = "Array" ;
                break;
            case rapidjson::kStringType:
                type_name = "String" ;
                break;
            case rapidjson::kNumberType:
                type_name = "Number" ;
                break;
            default:
                ostringstream s;
                s << "The type of element for '" << m_Key <<"' is unknown." ;
                throw SerializationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }
        return type_name ;
    }

    // ----------------------------------------------
    // --- Methods for getting values from an object
    // ----------------------------------------------

    JsonObjectDemog JsonObjectDemog::operator[]( const char* key ) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        if( !r_value.IsObject() )
        {
            ostringstream s;
            s << "The '" << m_Key << "' element is not an object.  One cannot get key-based value from something that is not an object." ;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }
        else if( !r_value.HasMember( key ) )
        {
            ostringstream s;
            s << "The '" << m_Key << "' element does not contain an element with name '" << key << "'." ;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }

        JsonObjectDemog obj( key, &r_value[ key ], m_pDocument );
        return obj;
    }

    void CheckElement( const char* pElementKey,
                       const char* pGetKey, 
                       const rapidjson::Value& rValue, 
                       std::function<bool( const rapidjson::Value& )>&& func,
                       const char* pTypeName, 
                       int linenumber,
                       const char* funcName )
    {
        if( rValue.IsNull() )
        {
            ostringstream s;
            s << "'" << pElementKey << "' does not contain an element named '" << pGetKey << "'" ;
            throw SerializationException( __FILE__, linenumber, funcName, s.str().c_str() );
        }

        if( !func( rValue ) )
        {
            ostringstream s;
            s << "'" << pElementKey << "' has an element named '" << pGetKey << "' but it is not a '" << pTypeName << "'." ;
            throw SerializationException( __FILE__, linenumber, funcName, s.str().c_str() );
        }
    }

    JsonObjectDemog JsonObjectDemog::GetJsonObject(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsObject(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Object", __LINE__, __FUNCTION__ );

        JsonObjectDemog obj( key, &r_key_value, m_pDocument );
        return obj;
    }

    JsonObjectDemog JsonObjectDemog::GetJsonArray(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsArray(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Array", __LINE__, __FUNCTION__ );

        JsonObjectDemog obj( key, &r_key_value, m_pDocument );

        return obj;
    }

    const char* JsonObjectDemog::GetString(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsString(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "String", __LINE__, __FUNCTION__ );

        return r_key_value.GetString();
    }

    int32_t JsonObjectDemog::GetInt(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsInt(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Int", __LINE__, __FUNCTION__ );

        return r_key_value.GetInt();
    }

    uint32_t JsonObjectDemog::GetUint(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsUint(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Uint", __LINE__, __FUNCTION__ );

        return r_key_value.GetUint();
    }

    int64_t JsonObjectDemog::GetInt64(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsInt64(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Int64", __LINE__, __FUNCTION__ );

        return r_key_value.GetInt64();
    }

    uint64_t JsonObjectDemog::GetUint64(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsUint64(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Uint64", __LINE__, __FUNCTION__ );

        return r_key_value.GetUint64();
    }

    float JsonObjectDemog::GetFloat(const char* key) const
    {
        double val = GetDouble( key );
        float f_val = float(val) ;
        return f_val ;
    }

    double JsonObjectDemog::GetDouble(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsNumber(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Number", __LINE__, __FUNCTION__ );

        return r_key_value.GetDouble();
    }

    bool JsonObjectDemog::GetBool(const char* key) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        rapidjson::Value& r_key_value = r_value[key] ;

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsBool(); };
        CheckElement( m_Key.c_str(), key, r_key_value, func, "Bool", __LINE__, __FUNCTION__ );

        return r_key_value.GetBool();
    }

    // -------------------------------------------
    // --- Methods for getting values from values
    // -------------------------------------------

    void CheckValue( const char* pElementKey,
                     const rapidjson::Value& rValue, 
                     std::function<bool( const rapidjson::Value& )>&& func,
                     const char* pTypeName, 
                     int linenumber,
                     const char* funcName )
    {
        if( !func( rValue ) )
        {
            ostringstream s;
            s << "The '" << pElementKey <<"' element is not a '" << pTypeName << "'." ;
            throw SerializationException( __FILE__, linenumber, funcName, s.str().c_str() );
        }
    }

    const char* JsonObjectDemog::AsString() const
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsString(); };
        CheckValue( m_Key.c_str(), r_value, func, "String", __LINE__, __FUNCTION__ );

        return r_value.GetString();
    }

    int32_t JsonObjectDemog::AsInt()    const
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsInt(); };
        CheckValue( m_Key.c_str(), r_value, func, "Int", __LINE__, __FUNCTION__ );

        return r_value.GetInt(); 
    }

    uint32_t JsonObjectDemog::AsUint() const 
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsUint(); };
        CheckValue( m_Key.c_str(), r_value, func, "Uint", __LINE__, __FUNCTION__ );

        return r_value.GetUint(); 
    }

    int64_t JsonObjectDemog::AsInt64() const 
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsInt64(); };
        CheckValue( m_Key.c_str(), r_value, func, "Int64", __LINE__, __FUNCTION__ );

        return r_value.GetInt64(); 
    }

    uint64_t JsonObjectDemog::AsUint64() const 
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsUint64(); };
        CheckValue( m_Key.c_str(), r_value, func, "Uint64", __LINE__, __FUNCTION__ );

        return r_value.GetUint64(); 
    }

    float JsonObjectDemog::AsFloat() const 
    { 
        return float(AsDouble());
    }

    double JsonObjectDemog::AsDouble() const 
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsNumber(); };
        CheckValue( m_Key.c_str(), r_value, func, "Number", __LINE__, __FUNCTION__ );

        return r_value.GetDouble(); 
    }

    bool JsonObjectDemog::AsBool() const 
    { 
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        auto func = [] ( const rapidjson::Value& rValue ) { return rValue.IsBool(); };
        CheckValue( m_Key.c_str(), r_value, func, "Bool", __LINE__, __FUNCTION__ );

        return r_value.GetBool(); 
    }

    JsonObjectDemog::operator const char*() const { return AsString(); }
    JsonObjectDemog::operator int32_t()     const { return AsInt(); }
    JsonObjectDemog::operator float()       const { return AsFloat(); }
    JsonObjectDemog::operator bool()        const { return AsBool(); }

    // ---------------------------------------------------------
    // --- Methods for Adding and Removing items from an Object
    // ---------------------------------------------------------

    void DeepCopy( rapidjson::Value& rOrig, rapidjson::Value& rCopy, rapidjson::Document& rDoc )
    {
        if( rOrig.IsObject() )
        {
            rCopy.SetObject();
            for( rapidjson::Value::MemberIterator it = rOrig.MemberBegin() ;
                it != rOrig.MemberEnd() ; ++it )
            {
                rapidjson::Value member_value_to_add ;
                DeepCopy( it->value, member_value_to_add, rDoc );
                rCopy.AddMember( it->name.GetString(), rDoc.GetAllocator(), member_value_to_add, rDoc.GetAllocator() );
            }
        }
        else if( rOrig.IsArray() )
        {
            rCopy.SetArray();
            for( rapidjson::Value::ValueIterator it = rOrig.Begin() ;
                it != rOrig.End() ; ++it )
            {
                rapidjson::Value value_to_add ;
                DeepCopy( *it, value_to_add, rDoc );
                rCopy.PushBack( value_to_add, rDoc.GetAllocator() );
            }
        }
        else if( rOrig.IsString() )
        {
            rCopy.SetString( rOrig.GetString(), rDoc.GetAllocator() );
        }
        else if( rOrig.IsBool() )
        {
            rCopy.SetBool( rOrig.GetBool() );
        }
        else if( rOrig.IsInt() )
        {
            rCopy.SetInt( rOrig.GetInt() );
        }
        else if( rOrig.IsUint() )
        {
            rCopy.SetUint( rOrig.GetUint() );
        }
        else if( rOrig.IsInt64() )
        {
            rCopy.SetInt64( rOrig.GetInt64() );
        }
        else if( rOrig.IsUint64() )
        {
            rCopy.SetUint64( rOrig.GetUint64() );
        }
        else if( rOrig.IsDouble() )
        {
            rCopy.SetDouble( rOrig.GetDouble() );
        }
        else
        {
            assert( false );
        }
    }

    void JsonObjectDemog::Add( const std::string& rKey, const JsonObjectDemog& rValue )
    {
        Add( rKey.c_str(), rValue );
    }

    void JsonObjectDemog::Add( const char* pKey, const JsonObjectDemog& rValue )
    {
        assert(m_pDocument.get());
        rapidjson::Document& r_this_doc = *((rapidjson::Document*)m_pDocument.get());

        assert(m_pValue);
        rapidjson::Value& r_this_value = *((rapidjson::Value*)m_pValue);

        assert(rValue.m_pValue);
        rapidjson::Value& r_that_value = *((rapidjson::Value*)rValue.m_pValue);

        if( r_this_value.HasMember( pKey ) )
        {
            r_this_value.RemoveMember( pKey );
        }

        rapidjson::Value value_to_add ;
        DeepCopy( r_that_value, value_to_add, r_this_doc );

        r_this_value.AddMember( pKey, r_this_doc.GetAllocator(), value_to_add, r_this_doc.GetAllocator() );
    }

    void JsonObjectDemog::Add( const std::string& rKey, const std::string& rStrValue )
    {
        Add( rKey.c_str(), rStrValue.c_str() );
    }

    void JsonObjectDemog::Add( const char* pKey, const char* pStrValue )
    {
        assert(m_pDocument.get());
        rapidjson::Document& r_doc = *((rapidjson::Document*)m_pDocument.get());

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsObject() );

        if( r_value.HasMember( pKey ) )
        {
            r_value.RemoveMember( pKey );
        }

        rapidjson::Value new_val( pStrValue, r_doc.GetAllocator() );

        r_value.AddMember( pKey, r_doc.GetAllocator(), new_val, r_doc.GetAllocator() );
    }

    void JsonObjectDemog::Add( const std::string& rKey, double val )
    {
        Add( rKey.c_str(), val );
    }

    void JsonObjectDemog::Add( const char* pKey, double val )
    {
        assert(m_pDocument.get());
        rapidjson::Document& r_doc = *((rapidjson::Document*)m_pDocument.get());

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsObject() );

        if( r_value.HasMember( pKey ) )
        {
            r_value.RemoveMember( pKey );
        }

        rapidjson::Value new_val( val );

        r_value.AddMember( pKey, r_doc.GetAllocator(), new_val, r_doc.GetAllocator() );
    }

    void JsonObjectDemog::Add( const std::string& rKey, int val )
    {
        Add( rKey.c_str(), val );
    }

    void JsonObjectDemog::Add( const char* pKey, int val )
    {
        assert(m_pDocument.get());
        rapidjson::Document& r_doc = *((rapidjson::Document*)m_pDocument.get());

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsObject() );

        if( r_value.HasMember( pKey ) )
        {
            r_value.RemoveMember( pKey );
        }

        rapidjson::Value new_val( val );

        r_value.AddMember( pKey, r_doc.GetAllocator(), new_val, r_doc.GetAllocator() );
    }

    void JsonObjectDemog::Remove( const JsonObjectDemog::Iterator& rIterator )
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsObject() );

        assert( rIterator.m_pData );
        rapidjson::Value::Member* p_member = (rapidjson::Value::Member*)rIterator.m_pData ;

        r_value.RemoveMember( p_member->name.GetString() );
    }

    // -------------------------------------
    // --- Methods for manipulating an Array
    // -------------------------------------

    size_t JsonObjectDemog::size() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        if( !r_value.IsArray() )
        {
            ostringstream s;
            s << "The '" << m_Key <<"' element is not an 'Array'." ;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }

        return r_value.Size();
    }

    JsonObjectDemog JsonObjectDemog::operator[](IndexType index) const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        if( !r_value.IsArray() )
        {
            ostringstream s;
            s << "The '" << m_Key <<"' element is not an 'Array'." ;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }

        ostringstream s_key;
        s_key << m_Key << "[" << index << "]" ;
        JsonObjectDemog obj( s_key.str(), &r_value[ (rapidjson::SizeType)index ], m_pDocument );
        return obj;
    }

    void JsonObjectDemog::PushBack( const JsonObjectDemog& rNewValue )
    {
        assert(m_pDocument.get());
        rapidjson::Document& r_doc = *((rapidjson::Document*)m_pDocument.get());

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsArray() );

        assert(rNewValue.m_pValue);
        rapidjson::Value& r_new_value = *((rapidjson::Value*)rNewValue.m_pValue);

        rapidjson::Value value_to_add ;
        DeepCopy( r_new_value, value_to_add, r_doc );

        r_value.PushBack( value_to_add, r_doc.GetAllocator() );
    }

    void JsonObjectDemog::PushBack( const std::string& rStr )
    {
        assert(m_pDocument.get());
        rapidjson::Document& r_doc = *((rapidjson::Document*)m_pDocument.get());

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsArray() );

        rapidjson::Value value_to_add;
        value_to_add.SetString( rStr.c_str(), rapidjson::SizeType(rStr.length()), r_doc.GetAllocator() );

        r_value.PushBack( value_to_add, r_doc.GetAllocator() );
    }

    //void JsonObjectDemog::PopBack() 
    //{
    //    assert(false);
    //}

    // ----------------------------------------------------
    // --- Methods for getting an Iterator from an Object
    // ----------------------------------------------------

    JsonObjectDemog::Iterator JsonObjectDemog::Begin() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsObject() );

        Iterator it( r_value.MemberBegin(), m_pDocument, m_pValue );
        return it ;
    }

    JsonObjectDemog::Iterator JsonObjectDemog::End() const
    {
        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);
        assert( r_value.IsObject() );

        Iterator it( r_value.MemberEnd(), m_pDocument, m_pValue );
        return it ;
    }

    // -----------------------------
    // --- JsonObjectDemog::Iterator
    // -----------------------------

    JsonObjectDemog::Iterator::Iterator( void* pData, std::shared_ptr<void> pDocument, void* pValue )
        : m_pData( pData )
        , m_pDocument( pDocument )
        , m_pValue( pValue )
    {
    }

    JsonObjectDemog::Iterator::~Iterator()
    {
    }

    bool JsonObjectDemog::Iterator::operator==( const JsonObjectDemog::Iterator& rThat ) const
    {
        if( this->m_pData     != rThat.m_pData     ) return false ;
        if( this->m_pDocument != rThat.m_pDocument ) return false ;
        if( this->m_pValue    != rThat.m_pValue    ) return false ;

        return true ;
    }

    bool JsonObjectDemog::Iterator::operator!=( const JsonObjectDemog::Iterator& rThat ) const
    {
        return !( *this == rThat );
    }

    std::string JsonObjectDemog::Iterator::GetKey() const
    {
        assert( m_pData );
        rapidjson::Value::Member* p_member = (rapidjson::Value::Member*)m_pData ;

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        // iterator must not be past the end of the array
        assert( p_member < r_value.MemberEnd() );

        return std::string(p_member->name.GetString()) ;
    }

    JsonObjectDemog JsonObjectDemog::Iterator::GetValue() const
    {
        assert( m_pData );
        rapidjson::Value::Member* p_member = (rapidjson::Value::Member*)m_pData ;

        assert(m_pValue);
        rapidjson::Value& r_value = *((rapidjson::Value*)m_pValue);

        // iterator must not be past the end of the array
        assert( p_member < r_value.MemberEnd() );

        JsonObjectDemog json( p_member->name.GetString(), (void*)(&(p_member->value)), m_pDocument );
        return json ;
    }

    JsonObjectDemog::Iterator& JsonObjectDemog::Iterator::operator++()
    {
        assert( m_pData );
        rapidjson::Value::Member* p_member = (rapidjson::Value::Member*)m_pData ;

        p_member++ ;

        m_pData = p_member ;

        return *this ;
    }

    // ------------------------
    // --- JsonWriterDemog
    // ------------------------

    JsonWriterDemog::JsonWriterDemog(bool pretty)
        : m_pBuffer(new rapidjson::StringBuffer())
        , m_pWriter(nullptr)
    {
        m_pWriter = new rapidjson::Writer<rapidjson::StringBuffer>( *((rapidjson::StringBuffer*)m_pBuffer) ) ;
        pretty;
    }

    JsonWriterDemog::~JsonWriterDemog()
    {
        delete m_pWriter;
        delete m_pBuffer;
    }

    const char* JsonWriterDemog::Text() const
    {
        assert( m_pBuffer );
        rapidjson::StringBuffer& r_buffer = *((rapidjson::StringBuffer*)m_pBuffer);
        return r_buffer.GetString();
    }

    struct OutputStringStream : public std::ostringstream 
    {
        typedef char Ch;

        void Put(char c) 
        {
            put(c);
        }
    };

    char* JsonWriterDemog::PrettyText() const
    {
        char* prettyString = nullptr;
        rapidjson::Document doc;
        rapidjson::StringBuffer& r_buffer = *((rapidjson::StringBuffer*)m_pBuffer);

        if (doc.Parse<0>(r_buffer.GetString()).HasParseError())
        {
            const char* err = doc.GetParseError();
            LOG_ERR_F("%s: Parse json error: %s\n", __FUNCTION__, err);
            return prettyString;
        }

        OutputStringStream os;
        rapidjson::PrettyWriter<OutputStringStream> writer(os);
        doc.Accept(writer);
        size_t len = os.str().size();
        if (len > 0)
        {
            prettyString = (char*) malloc(len + 1);
            strcpy(prettyString, os.str().c_str());
        }

        return prettyString;
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const char bracket)
    {
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);
        switch (bracket)
        {
        case '{':
            r_writer.StartObject();
            break;

        case '}':
            r_writer.EndObject();
            break;

        case '[':
            r_writer.StartArray();
            break;

        case ']':
            r_writer.EndArray();
            break;

        default:
            ostringstream s ;
            s << "Not supported character = " << bracket << ".  Only '{', '}', '[', ']' are supported." ;
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }

        return (*this);
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const char* value)
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.String(value); 

        return *this;
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const int32_t value)  
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Int(value);    

        return *this; 
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const uint32_t value) 
    {
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Uint(value);   

        return *this;
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const int64_t value)  
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Int64(value);  

        return *this; 
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const uint64_t value) 
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Uint64(value);

        return *this; 
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const float value)    
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Double(value); 

        return *this;
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const double value)   
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Double(value); 

        return *this; 
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const bool value)     
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        r_writer.Bool(value);

        return *this; 
    }

    JsonWriterDemog& JsonWriterDemog::operator <<(const JsonObjectDemog& rValue ) 
    { 
        assert( m_pWriter );
        rapidjson::Writer<rapidjson::StringBuffer>& r_writer = *((rapidjson::Writer<rapidjson::StringBuffer>*)m_pWriter);

        assert(rValue.m_pValue);
        rapidjson::Value& r_rapid_value = *((rapidjson::Value*)rValue.m_pValue);

        r_rapid_value.Accept( r_writer );

        return *this; 
    }
}
