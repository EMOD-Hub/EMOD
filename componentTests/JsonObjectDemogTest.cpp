
#include "stdafx.h"
#include <iostream>
#include "UnitTest++.h"
#include "Exceptions.h"
#include "JsonObjectDemog.h"

#include <limits>
#include <functional>

using namespace std; 
using namespace Kernel; 

SUITE(JsonObjectDemogTest)
{
    struct JsonTestFixture
    {
        void TestHelper_FormatException( int lineNumber, const std::string& rBadJsonStr, const std::string& rExpMsg )
        {
            try
            {
                JsonObjectDemog json ;
                json.Parse( rBadJsonStr.c_str() );
                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( SerializationException& re )
            {
                std::string msg = re.GetMsg();
                CHECK_LN( msg.find( rExpMsg ) != string::npos, lineNumber );
            }
        }

        void TestHelper_BadAccessException( int lineNumber, 
                                            JsonObjectDemog& rJson, 
                                            std::function<void( const JsonObjectDemog& )>&& func, 
                                            const std::string& rExpMsg )
        {
            try
            {
                func( rJson );
                CHECK_LN( false, lineNumber ); // shouldn't get here
            }
            catch( SerializationException& re )
            {
                std::string msg = re.GetMsg();
                CHECK_LN( msg.find( rExpMsg ) != string::npos, lineNumber );
            }
        }

    };

    TEST(TestJsonReadWrite)
    {
        JsonWriterDemog writer( true ) ;

        writer << '{' 
               <<     "String"         << "ThisString"
               <<     "Bool"           << true
               <<     "Int32"          << -2147483647
               <<     "Uint32"         << uint32_t(4294967295)
               <<     "Int64"          << int64_t(-9223372036854775807)
               <<     "Uint64"         << uint64_t(18446744073709551615)
               <<     "FloatMin"       << float(1.17549435e-038)
               <<     "FloatMax"       << float(3.40282347e+038)
               <<     "FloatEpsilon1"  << float( 0.00001)
               <<     "FloatEpsilon2"  << float(-0.00001)
               <<     "DoubleMin"      << double(2.225073858507e-308)
               <<     "DoubleMax"      << double(1.797693134862e+308)
               <<     "DoubleEpsilon1" << double( 0.000000001)
               <<     "DoubleEpsilon2" << double(-0.000000001)
               <<     "Object"
               <<     '{'
               <<         "ObjectString" << "String for Object"
               <<         "Int32"        << 777
               <<     '}'
               <<     "ObjectOuter"
               <<     '{'
               <<         "ObjectString" << "String for ObjectOuter"
               <<         "Int32"        << 999
               <<         "ObjectInner"
               <<         '{'
               <<             "Float" << 0.12345
               <<             "Bool"  << false
               <<         '}'
               <<     '}'
               <<     "SingleArray" << '[' << 1 << 2 << 3 << 4 << 5 << ']'
               <<     "DoubleArray" << '[' << '[' << 11 << 12 << 13 << 14 << 15 << ']'
                                           << '[' << 21 << 22 << 23 << 24 << 25 << ']' << ']'
               << '}' ;

        JsonObjectDemog json ;

        json.Parse( writer.Text() );

        CHECK_EQUAL( string("ThisString"),           json.GetString( "String"         ) );
        CHECK_EQUAL( true,                           json.GetBool(   "Bool"           ) );
        CHECK_EQUAL( int32_t(-2147483647),           json.GetInt(    "Int32"          ) );
        CHECK_EQUAL( uint32_t(4294967295),           json.GetUint(   "Uint32"         ) );
        CHECK_EQUAL( int64_t(-9223372036854775807),  json.GetInt64(  "Int64"          ) );
        CHECK_EQUAL( uint64_t(18446744073709551615), json.GetUint64( "Uint64"         ) );
        CHECK_EQUAL( float(1.17549435e-038),         json.GetFloat(  "FloatMin"       ) );
        CHECK_EQUAL( float(3.40282347e+038),         json.GetFloat(  "FloatMax"       ) );
        CHECK_EQUAL( float( 0.00001),                json.GetFloat(  "FloatEpsilon1"  ) );
        CHECK_EQUAL( float(-0.00001),                json.GetFloat(  "FloatEpsilon2"  ) );
        CHECK_EQUAL( double(2.225073858507e-308),    json.GetDouble( "DoubleMin"      ) );
        CHECK_EQUAL( double(1.797693134862e+308),    json.GetDouble( "DoubleMax"      ) );
        CHECK_EQUAL( double(0.000000001),            json.GetDouble( "DoubleEpsilon1" ) );
        CHECK_EQUAL( double(-0.000000001),           json.GetDouble( "DoubleEpsilon2" ) );

        JsonObjectDemog obj = json.GetJsonObject( "Object" );

        CHECK_EQUAL( string("String for Object"), obj.GetString( "ObjectString" ) );
        CHECK_EQUAL( int32_t(777), obj.GetInt( "Int32" ) );

        JsonObjectDemog obj_outer = json.GetJsonObject( "ObjectOuter" );

        CHECK_EQUAL( string("String for ObjectOuter"), obj_outer.GetString( "ObjectString" ) );
        CHECK_EQUAL( int32_t(999), obj_outer.GetInt( "Int32" ) );

        JsonObjectDemog obj_inner = obj_outer.GetJsonObject( "ObjectInner" );

        CHECK_EQUAL( float(0.12345), obj_inner.GetFloat( "Float" ) );
        CHECK_EQUAL( false, obj_inner.GetBool( "Bool" ) );

        JsonObjectDemog array_single = json.GetJsonArray( "SingleArray" );

        CHECK_EQUAL( 5, array_single.size() );
        CHECK_EQUAL( 1, array_single[IndexType(0)].AsInt() );
        CHECK_EQUAL( 2, array_single[IndexType(1)].AsInt() );
        CHECK_EQUAL( 3, array_single[IndexType(2)].AsInt() );
        CHECK_EQUAL( 4, array_single[IndexType(3)].AsInt() );
        CHECK_EQUAL( 5, array_single[IndexType(4)].AsInt() );

        array_single.PushBack( array_single[IndexType(0)] );

        JsonObjectDemog array_double = json.GetJsonArray( "DoubleArray" );

        CHECK_EQUAL( 2, array_double.size() );

        CHECK_EQUAL( 11, array_double[IndexType(0)][IndexType(0)].AsInt() );
        CHECK_EQUAL( 12, array_double[IndexType(0)][IndexType(1)].AsInt() );
        CHECK_EQUAL( 13, array_double[IndexType(0)][IndexType(2)].AsInt() );
        CHECK_EQUAL( 14, array_double[IndexType(0)][IndexType(3)].AsInt() );
        CHECK_EQUAL( 15, array_double[IndexType(0)][IndexType(4)].AsInt() );

        CHECK_EQUAL( 21, array_double[IndexType(1)][IndexType(0)].AsInt() );
        CHECK_EQUAL( 22, array_double[IndexType(1)][IndexType(1)].AsInt() );
        CHECK_EQUAL( 23, array_double[IndexType(1)][IndexType(2)].AsInt() );
        CHECK_EQUAL( 24, array_double[IndexType(1)][IndexType(3)].AsInt() );
        CHECK_EQUAL( 25, array_double[IndexType(1)][IndexType(4)].AsInt() );

//        CHECK_EQUAL( 18, json.size() );

        JsonObjectDemog::Iterator it = json.Begin() ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("String"), it.GetKey() );
        CHECK_EQUAL( string("ThisString"), it.GetValue().AsString() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("Bool"), it.GetKey() );
        CHECK_EQUAL( true, it.GetValue().AsBool() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("Int32"), it.GetKey() );
        CHECK_EQUAL( int32_t(-2147483647), it.GetValue().AsInt() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("Uint32"), it.GetKey() );
        CHECK_EQUAL( uint32_t(4294967295), it.GetValue().AsUint() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("Int64"), it.GetKey() );
        CHECK_EQUAL( int64_t(-9223372036854775807), it.GetValue().AsInt64() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("Uint64"), it.GetKey() );
        CHECK_EQUAL( uint64_t(18446744073709551615), it.GetValue().AsUint64() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("FloatMin"), it.GetKey() );
        CHECK_EQUAL( float(1.17549435e-038), it.GetValue().AsFloat() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("FloatMax"), it.GetKey() );
        CHECK_EQUAL( float(3.40282347e+038), it.GetValue().AsFloat() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("FloatEpsilon1"), it.GetKey() );
        CHECK_EQUAL( float(0.00001), it.GetValue().AsFloat() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("FloatEpsilon2"), it.GetKey() );
        CHECK_EQUAL( float(-0.00001), it.GetValue().AsFloat() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("DoubleMin"), it.GetKey() );
        CHECK_EQUAL( double(2.225073858507e-308), it.GetValue().AsDouble() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("DoubleMax"), it.GetKey() );
        CHECK_EQUAL( double(1.797693134862e+308), it.GetValue().AsDouble() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("DoubleEpsilon1"), it.GetKey() );
        CHECK_EQUAL( double(0.000000001), it.GetValue().AsDouble() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("DoubleEpsilon2"), it.GetKey() );
        CHECK_EQUAL( double(-0.000000001), it.GetValue().AsDouble() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("Object"), it.GetKey() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("ObjectOuter"), it.GetKey() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("SingleArray"), it.GetKey() );

        ++it ;
        CHECK( it != json.End() );
        CHECK_EQUAL( string("DoubleArray"), it.GetKey() );

        ++it ;
        CHECK( it == json.End() );
    }


    TEST_FIXTURE(JsonTestFixture,TestBadJsonErrorHandling)
    {
        string json_str = "{ bad json }" ;
        TestHelper_FormatException( __LINE__, json_str,
            "Failed to parse incoming text. Name of an object member must be a string at character=2 / line number=0" );

        json_str = "{ \"String\" :  }" ; // missing value
        TestHelper_FormatException( __LINE__, json_str,
            "Failed to parse incoming text. Expect a value here. at character=14 / line number=0" );

        json_str = "{ \"String\" : \"ThisString\", \"Int\" : 5, }" ; // extra comma
        TestHelper_FormatException( __LINE__, json_str,
            "Failed to parse incoming text. Name of an object member must be a string at character=38 / line number=0" );

        json_str = "{ \"String\" : \"ThisString, \"Int\" : 5 }" ; // missing double quote
        TestHelper_FormatException( __LINE__, json_str,
            "Failed to parse incoming text. Must be a comma or '}' after an object member at character=28 / line number=0" );
    }

    TEST_FIXTURE(JsonTestFixture,TestGettingDataErrorHandling)
    {
        JsonWriterDemog writer( true ) ;

        writer << '{' 
               <<     "String"         << "ThisString"
               <<     "Bool"           << true
               <<     "Int32"          << -2147483647
               <<     "Uint32"         << uint32_t(4294967295)
               <<     "Int64"          << int64_t(-9223372036854775807)
               <<     "Uint64"         << uint64_t(18446744073709551615)
               <<     "FloatMin"       << float(1.17549435e-038)
               <<     "FloatMax"       << float(3.40282347e+038)
               <<     "FloatEpsilon1"  << float( 0.00001)
               <<     "FloatEpsilon2"  << float(-0.00001)
               <<     "Double"         << double(5.0)
               <<     "DoubleMin"      << double(2.225073858507e-308)
               <<     "DoubleMax"      << double(1.797693134862e+308)
               <<     "DoubleEpsilon1" << double( 0.000000001)
               <<     "DoubleEpsilon2" << double(-0.000000001)
               <<     "Object"
               <<     '{'
               <<         "ObjectString" << "String for Object"
               <<         "Int32"        << 777
               <<     '}'
               <<     "ObjectOuter"
               <<     '{'
               <<         "ObjectString" << "String for ObjectOuter"
               <<         "Int32"        << 999
               <<         "ObjectInner"
               <<         '{'
               <<             "Float" << 0.12345
               <<             "Bool"  << false
               <<         '}'
               <<     '}'
               <<     "SingleArray" << '[' << 1.1 << 2.1 << 3.1 << 4.1 << 5.1 << ']'
               <<     "DoubleArray" << '[' << '[' << 11 << 12 << 13 << 14 << 15 << ']'
                                           << '[' << 21 << 22 << 23 << 24 << 25 << ']' << ']'
               << '}' ;

        JsonObjectDemog json ;

        json.Parse( writer.Text() );

        auto func1 = [] ( const JsonObjectDemog& rJson ) { rJson.GetString("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func1, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func2 = [] ( const JsonObjectDemog& rJson ) { rJson.GetInt("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func2, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func3 = [] ( const JsonObjectDemog& rJson ) { rJson.GetUint("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func3, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func4 = [] ( const JsonObjectDemog& rJson ) { rJson.GetInt64("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func4, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func5 = [] ( const JsonObjectDemog& rJson ) { rJson.GetUint64("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func5, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func6 = [] ( const JsonObjectDemog& rJson ) { rJson.GetFloat("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func6, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func7 = [] ( const JsonObjectDemog& rJson ) { rJson.GetDouble("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func7, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func8 = [] ( const JsonObjectDemog& rJson ) { rJson.GetBool("InvalidName"); };
        TestHelper_BadAccessException( __LINE__, json, func8, 
                                       "'root' does not contain an element named 'InvalidName'" );

        auto func9 = [] ( const JsonObjectDemog& rJson ) { rJson.GetString("Bool"); };
        TestHelper_BadAccessException( __LINE__, json, func9, 
                                       "'root' has an element named 'Bool' but it is not a 'String'." );

        auto func10 = [] ( const JsonObjectDemog& rJson ) { rJson.GetString("Int32"); };
        TestHelper_BadAccessException( __LINE__, json, func10, 
                                       "'root' has an element named 'Int32' but it is not a 'String'." );

        auto func11 = [] ( const JsonObjectDemog& rJson ) { rJson.GetBool("Int32"); };
        TestHelper_BadAccessException( __LINE__, json, func11, 
                                       "'root' has an element named 'Int32' but it is not a 'Bool'." );

        auto func12 = [] ( const JsonObjectDemog& rJson ) { rJson.GetInt("String"); };
        TestHelper_BadAccessException( __LINE__, json, func12, 
                                       "'root' has an element named 'String' but it is not a 'Int'." );

        auto func13 = [] ( const JsonObjectDemog& rJson ) { rJson.GetJsonObject("SingleArray"); };
        TestHelper_BadAccessException( __LINE__, json, func13, 
                                       "'root' has an element named 'SingleArray' but it is not a 'Object'." );

        auto func14 = [] ( const JsonObjectDemog& rJson ) { rJson.GetJsonArray("ObjectOuter"); };
        TestHelper_BadAccessException( __LINE__, json, func14, 
                                       "'root' has an element named 'ObjectOuter' but it is not a 'Array'." );

        auto func15 = [] ( const JsonObjectDemog& rJson ) { rJson.GetJsonArray("SingleArray").AsString(); };
        TestHelper_BadAccessException( __LINE__, json, func15, 
                                       "'SingleArray' element is not a 'String'." );

        auto func16 = [] ( const JsonObjectDemog& rJson ) { rJson.GetJsonArray("SingleArray")[IndexType(0)].AsString(); };
        TestHelper_BadAccessException( __LINE__, json, func16, 
                                       "'SingleArray[0]' element is not a 'String'." );

        auto func17 = [] ( const JsonObjectDemog& rJson ) { rJson.GetJsonArray("DoubleArray")[IndexType(0)][IndexType(1)].AsString(); };
        TestHelper_BadAccessException( __LINE__, json, func17, 
                                       "'DoubleArray[0][1]' element is not a 'String'." );

        auto func18 = [] ( const JsonObjectDemog& rJson ) { rJson.GetJsonArray("SingleArray")[IndexType(3)].AsInt(); };
        TestHelper_BadAccessException( __LINE__, json, func18, 
                                       "'SingleArray[3]' element is not a 'Int'." );
    }

    TEST_FIXTURE(JsonTestFixture,TestGettingDataErrorHandling2)
    {
        std::string json_str ;
        json_str += "{" ;
        json_str +=    "\"String\":\"ThisString\"," ;
        json_str +=    "\"Value1\":1," ;
        json_str +=    "\"Value2\":2.2," ;
        json_str +=    "\"Value3\":3.3e-1," ;
        json_str +=    "\"Value4\":true" ;
        json_str += "}" ;

        JsonObjectDemog json ;

        json.Parse( json_str.c_str() );

        CHECK_EQUAL( std::string("ThisString"), std::string(json.GetString( "String" )) );

        CHECK_EQUAL( 1,    json.GetInt(    "Value1" ) );
        CHECK_EQUAL( 1,    json.GetUint(   "Value1" ) );
        CHECK_EQUAL( 1,    json.GetInt64(  "Value1" ) );
        CHECK_EQUAL( 1,    json.GetUint64( "Value1" ) );
        CHECK_EQUAL( 1.0f, json.GetFloat(  "Value1" ) );
        CHECK_EQUAL( 1.0,  json.GetDouble( "Value1" ) );
        CHECK_EQUAL( 2.2f, json.GetFloat(  "Value2" ) );
        CHECK_EQUAL( 2.2,  json.GetDouble( "Value2" ) );
        CHECK_EQUAL( float(3.3e-1), json.GetFloat( "Value3" ) );
        CHECK_CLOSE( double(3.3e-1), json.GetDouble( "Value3" ), 0.00000001 );
        CHECK_EQUAL( true, json.GetBool( "Value4" ) );

        auto func1 = [] ( const JsonObjectDemog& rJson ) { rJson.GetInt("Value2"); };
        TestHelper_BadAccessException( __LINE__, json, func1, 
                                       "'root' has an element named 'Value2' but it is not a 'Int'." );

        std::string exp_str ;
        exp_str += "{" ;
        exp_str +=    "\"String\":\"ThisString\"," ;
        exp_str +=    "\"Value1\":1," ;
        exp_str +=    "\"Value2\":2.2," ;
        exp_str +=    "\"Value3\":0.33," ;
        exp_str +=    "\"Value4\":true" ;
        exp_str += "}" ;

        std::string result_str = json.ToString();
        CHECK_EQUAL( exp_str, result_str );
    }

    TEST(TestCreatingObject)
    {
        JsonObjectDemog json_root( JsonObjectDemog::JSON_OBJECT_OBJECT );
        CHECK( !json_root.IsNull() );
        CHECK( !json_root.IsArray() );
        CHECK( json_root.IsObject() );

        json_root.Add( "String1", "ThisString_1" );
        json_root.Add( "String2", "ThisString_2" );
        json_root.Add( "String3", "ThisString_3" );

        JsonObjectDemog*  p_json_inner = new JsonObjectDemog( JsonObjectDemog::JSON_OBJECT_OBJECT );

        p_json_inner->Add( "InnerString1", "String of Inner 1" );
        p_json_inner->Add( "InnerString2", "String of Inner 2" );

        json_root.Add( "Object", *p_json_inner );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Delete the object so that we can test that json_root doesn't lose
        // !!! the values that are part of json_inner.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        delete p_json_inner ;


        JsonObjectDemog node_1( JsonObjectDemog::JSON_OBJECT_OBJECT );
        node_1.Add( "Latitude",  1.1 );
        node_1.Add( "Longitude", 1.2 );

        JsonObjectDemog node_2( JsonObjectDemog::JSON_OBJECT_OBJECT );
        node_2.Add( "Latitude",  2.1 );
        node_2.Add( "Longitude", 2.2 );

        JsonObjectDemog node_3( JsonObjectDemog::JSON_OBJECT_OBJECT );
        node_3.Add( "Latitude",  3.1 );
        node_3.Add( "Longitude", 3.2 );

        JsonObjectDemog json_array( JsonObjectDemog::JSON_OBJECT_ARRAY );
        json_array.PushBack( node_1 );
        json_array.PushBack( node_2 );
        json_array.PushBack( node_3 );

        json_root.Add( "Nodes", json_array );

        JsonObjectDemog::Iterator it = json_root.Begin();
        CHECK_EQUAL( std::string("String1"), it.GetKey() );
        CHECK_EQUAL( std::string("ThisString_1"), it.GetValue().AsString() );

        ++it ;
        CHECK_EQUAL( std::string("String2"), it.GetKey() );
        CHECK_EQUAL( std::string("ThisString_2"), it.GetValue().AsString() );

        ++it ;
        CHECK_EQUAL( std::string("String3"), it.GetKey() );
        CHECK_EQUAL( std::string("ThisString_3"), it.GetValue().AsString() );

        ++it ;
        CHECK_EQUAL( std::string("Object"), it.GetKey() );
        JsonObjectDemog obj = it.GetValue();
        CHECK( obj.IsObject() );

        JsonObjectDemog::Iterator obj_it = obj.Begin();
        CHECK_EQUAL( std::string("InnerString1"),      obj_it.GetKey() );
        CHECK_EQUAL( std::string("String of Inner 1"), obj_it.GetValue().AsString() );

        ++obj_it ;
        CHECK_EQUAL( std::string("InnerString2"),      obj_it.GetKey() );
        CHECK_EQUAL( std::string("String of Inner 2"), obj_it.GetValue().AsString() );

        ++obj_it ;
        CHECK( obj_it == obj.End() );

        ++it ;
        CHECK_EQUAL( std::string("Nodes"), it.GetKey() );
        JsonObjectDemog node_array = it.GetValue();
        CHECK( node_array.IsArray() );
        CHECK_EQUAL( 3, node_array.size() );
        CHECK_EQUAL( 1.1, node_array[IndexType(0)]["Latitude" ].AsDouble() );
        CHECK_EQUAL( 2.1, node_array[IndexType(1)]["Latitude" ].AsDouble() );
        CHECK_EQUAL( 3.1, node_array[IndexType(2)]["Latitude" ].AsDouble() );
        CHECK_EQUAL( 1.2, node_array[IndexType(0)]["Longitude"].AsDouble() );
        CHECK_EQUAL( 2.2, node_array[IndexType(1)]["Longitude"].AsDouble() );
        CHECK_EQUAL( 3.2, node_array[IndexType(2)]["Longitude"].AsDouble() );

        ++it ;
        CHECK( it == json_root.End() );

        it = json_root.Begin();
        CHECK_EQUAL( std::string("String1"), it.GetKey() );
        json_root.Remove( it );
        CHECK_EQUAL( std::string("Nodes"), it.GetKey() );
        json_root.Remove( it );
        CHECK_EQUAL( std::string("Object"), it.GetKey() );
        ++it ;
        CHECK_EQUAL( std::string("String2"), it.GetKey() );
        json_root.Remove( it );
        CHECK_EQUAL( std::string("String3"), it.GetKey() );
        json_root.Remove( it );
        CHECK( it == json_root.End() );
        
        it = json_root.Begin();
        CHECK_EQUAL( std::string("Object"), it.GetKey() );
        ++it ;
        CHECK( it == json_root.End() );
    }
}
