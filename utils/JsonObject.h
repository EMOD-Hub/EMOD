
//////////////////////////////////////////////////////////////////////////////
//
// The wrapper class for underlying json libraries
//
#pragma once

#include <cstdint>
using namespace std;

namespace Kernel {

    enum JsonLibType
    {
        JS_RAPIDJSON  = 0,
        JS_JSONSPIRIT = 1,
        JS_CAJUN      = 2
    };

    // Json (root) object abstraction (or adapter interface) for different JSon libraries
    // It is {'key1':'value1',...}, where "value" can be a string, number, bool, or json (root) object again, or a Json array
    // which is an ordered collection of json values
    struct IJsonAdapter
    {
        virtual void CreateNewWriter(bool bCacheWriter = false) = 0;
        virtual void FinishWriter() = 0;
        virtual void BeginObject() = 0;
        virtual void EndObject() = 0;
        virtual void BeginArray() = 0;
        virtual void EndArray() = 0;
        virtual const char* ToString() const = 0;
        virtual char* ToPrettyString() const = 0;
        virtual void Parse(const char* jsBuffer) = 0;

        virtual void DeleteUnderlyingJsonObjs() = 0;

        virtual ~IJsonAdapter() { }
    };

    // JSON
    // object- { } | { members }
    //      members- pair | pair, members
    //      pair- string : value
    // array- [ ] | [ elements ]
    //      elements- value | value, elements
    // value-
    //      string- "" | "chars"
    //          chars- char | char chars
    //          char- [^"\] | \" | \\ | \/ | \b | \f | \n | \r | \t | \uXXXX
    //      number- int | int frac | int exp | int frac exp
    //          int- digit | digit1-9 digits | - digit | - digit1-9 digits
    //          frac- . digits
    //          exp- e digits
    //          digits- digit | digit digits
    //          e- e | e+ | e- | E | E+ | E-
    //      object
    //      array
    //      true | false | null

typedef uint32_t IndexType;

    struct IJsonObjectAdapter : public IJsonAdapter
    {
        virtual void Insert( const char* key)                      = 0;
        virtual void Insert( const char* key, const char* val )    = 0;
        virtual void Insert( const char* key, const int32_t val )  = 0;
        virtual void Insert( const char* key, const uint32_t val ) = 0;
        virtual void Insert( const char* key, const int64_t val )  = 0;
        virtual void Insert( const char* key, const uint64_t val ) = 0;
        virtual void Insert( const char* key, const float val )    = 0;
        virtual void Insert( const char* key, const double val )   = 0;
        virtual void Insert( const char* key, const bool val )     = 0;
        virtual void Insert( const char* key, const IJsonObjectAdapter* val ) = 0;

        virtual void Add( const char* val )    = 0;
        virtual void Add( const int32_t val )  = 0;
        virtual void Add( const uint32_t val ) = 0;
        virtual void Add( const int64_t val )  = 0;
        virtual void Add( const uint64_t val ) = 0;
        virtual void Add( const float val )    = 0;
        virtual void Add( const double val )   = 0;
        virtual void Add( const bool val )     = 0;
        virtual void Add( const IJsonObjectAdapter* val ) = 0;

        virtual bool Contains(const char* key) const = 0;

        virtual IJsonObjectAdapter* operator[](const char* key) const = 0;
        virtual IJsonObjectAdapter* GetJsonObject(const char* key)  const = 0;
        virtual IJsonObjectAdapter* GetJsonArray(const char* key)   const = 0;
        virtual const char* GetString(const char* key)          const = 0;
        virtual int32_t GetInt(const char* key)                 const = 0;
        virtual uint32_t GetUint(const char* key)               const = 0;
        virtual int64_t GetInt64(const char* key)               const = 0;
        virtual uint64_t GetUInt64(const char* key)             const = 0;
        virtual float GetFloat(const char* key)                 const = 0;
        virtual double GetDouble(const char* key)               const = 0;
        virtual bool GetBool(const char* key)                   const = 0;

        virtual IJsonObjectAdapter* operator[](IndexType index) const = 0;

        virtual const char* AsString() const = 0;
        virtual int32_t AsInt()        const = 0;
        virtual uint32_t AsUint()      const = 0;
        virtual int64_t AsInt64()      const = 0;
        virtual uint64_t AsUint64()    const = 0;
        virtual float AsFloat()        const = 0;
        virtual double AsDouble()      const = 0;
        virtual bool AsBool()          const = 0;

        virtual operator const char*() const = 0;
        virtual operator int32_t()     const = 0;
        virtual operator float()       const = 0;
        virtual operator bool()        const = 0;

        virtual unsigned int GetSize() const = 0;

        virtual ~IJsonObjectAdapter() { }
    };

    IJsonObjectAdapter* CreateJsonObjAdapter(JsonLibType jsLib = JS_RAPIDJSON);
}