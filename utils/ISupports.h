
#pragma once

#include "IdmApi.h"
#include "Debug.h"
#include <string>
#include <list>
#include <vector>
#include <stdint.h>

#include "BoostLibWrapper.h"

namespace Kernel
{
    struct IConfigurable;

    /* This file contains the basic definitions needed for a simplified COM-like ISupports/IUnknown model for implementing dynamic interface querying.
        Reference counting is not supported yet and wont be unless our ownership model becomes more complex. 
    */

    // TODO (future): see how boost can serialize objects in dynamic libraries...obviously it wont be able to register the type statically! <ERAD-283>

    enum QueryResult { s_OK = 0, e_NOINTERFACE = 1, e_NULL_POINTER = 2 };

    typedef boost::uuids::uuid iid_t;

    template<class T>
    class TypeInfo
    {
    public:
        static iid_t GetIID(const char *stringname);
    };

    class IDMAPI TypeInfoHelper
    {
    public:
        static boost::uuids::name_generator dtk_name_gen; // uuid generator for dtk namespace
    };
   
    template<class T>
    iid_t TypeInfo<T>::GetIID(const char *stringname)
    {
        static iid_t iid = TypeInfoHelper::dtk_name_gen("kernel.emod.intven.com"+std::string(stringname));
        return iid;
    }

    // vvv Use this macro to retrieve IIDs conveniently and auto-generate them if they havent been already 
#define GET_IID(interfacename) \
    Kernel::TypeInfo<interfacename>::GetIID(#interfacename)
    
    struct IDMAPI ISupports
    {
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) = 0;

        // TODO: not implementing these because we have a simple hierarchical ownership structure...for the time being, but should be done soon <ERAD-285>
        virtual int32_t AddRef() = 0; // these return signed values because subtle concurrency issues could result in negative refcounts being returned in theory
        virtual int32_t Release() = 0;

        virtual IConfigurable* GetConfigurable() { return nullptr; }

        virtual ~ISupports() {}
    };
    
    class ISupportException : public std::exception
    {
    public:
        ISupportException()
        {
        }
        virtual std::string what()
        {
            return std::string( "Fatal exception: Release() called on object with negative reference count." );
        }
    };

#define DECLARE_QUERY_INTERFACE() \
    virtual QueryResult QueryInterface(iid_t iid, void** pinstance);

// TODO: need some method to ensure that refcounts will be serialized...maybe a macro that must exist in the serialize method or else a compile error is thrown
// small helper class to keep ref counts auto-initialized
class IDMAPI RefCount
{
public:
    RefCount() : m_refcount(0) { }
    RefCount(int32_t _rc) : m_refcount(_rc) { }
    operator int32_t&() { return m_refcount; }
    template<class Archive> void serialize(Archive &ar, const unsigned int v) { ar & m_refcount; }
protected:
    int32_t m_refcount;
};

#ifdef WIN32
#include "windows.h"    // bring in InterlockedIncrement and InterlockedDecrement
#define IMPLEMENT_DEFAULT_REFERENCE_COUNTING() \
    protected: \
        RefCount m_refcount; \
    private:\
        /* force classes wishing to be serialized to always serialize their refcount. slightly ugly but vital user safeguard. */\
        /* user must rename their serialize method serialize_impl */\
        template<class Archive>\
        void serialize(Archive &ar, const unsigned int v) { ar & m_refcount; }\
    public:\
        virtual int32_t AddRef() { return InterlockedIncrement((LONG volatile *)&m_refcount); }\
        virtual int32_t Release()\
        {\
            int32_t count = InterlockedDecrement((LONG volatile *)&m_refcount);\
            if(count == 0){ delete this; }\
            if (count < 0) { throw ISupportException(); }\
            return count;\
        }
#define IMPLEMENT_ONLY_REFERENCE_COUNTING() \
    protected: \
        RefCount m_refcount; \
    public:\
        virtual int32_t AddRef() { return InterlockedIncrement((LONG volatile *)&m_refcount); }\
        virtual int32_t Release()\
        {\
            int32_t count = InterlockedDecrement((LONG volatile *)&m_refcount);\
            if(count == 0){ delete this; }\
            if (count < 0) { throw ISupportException(); }\
            return count;\
        }
#else
#define IMPLEMENT_DEFAULT_REFERENCE_COUNTING() \
    protected: \
        RefCount m_refcount; \
    private:\
        /* force classes wishing to be serialized to always serialize their refcount. slightly ugly but vital user safeguard. */\
        /* user must rename their serialize method serialize_impl */\
        template<class Archive>\
        void serialize(Archive &ar, const unsigned int v) { ar & m_refcount; }\
    public:\
        virtual int32_t AddRef() { return ++m_refcount; }\
        virtual int32_t Release()\
        {\
            int32_t count = --m_refcount;\
            if(count == 0){ delete this; }\
            if (count < 0) { throw ISupportException(); }\
            return count;\
        }
#define IMPLEMENT_ONLY_REFERENCE_COUNTING() \
    protected: \
        RefCount m_refcount; \
    public:\
        virtual int32_t AddRef() { return ((int32_t volatile*)&++m_refcount); }\
        virtual int32_t Release()\
        {\
            int32_t count = ((int32_t volatile*)&--m_refcount);\
            if(count == 0){ delete this; }\
            if (count < 0) { throw ISupportException(); }\
            return count;\
        }
#endif

// for classes that need to implement these methods but don't actually have reference counted behavior
#define IMPLEMENT_NO_REFERENCE_COUNTING() \
    public:\
        virtual int32_t AddRef() { return 1; }\
        virtual int32_t Release() { return 1; }

    /*
    // reference implementation, adapted from https://developer.mozilla.org/en/Implementing_QueryInterface#The_NS_GET_IID_macro

    QueryResult
        MyImplementation::QueryInterface( iid_t iid, void** ppinstance )
    {
        assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return QueryResult.E_NULL_POINTER;

        IUnknown* foundInterface;

        if ( iid == TypeInfo<IX>::GetIID()) 
            foundInterface = static_cast<IX*>(this);
        else if ( iid == TypeInfo<IY>::GetIID()) )
            foundInterface = static_cast<IY*>(this);
         else if ( iid == TypeInfo<IUnknown>::GetIID()) )
            foundInterface = static_cast<IUnknown*>(static_cast<IX*>(this));
        else
            foundInterface = 0;

        QueryResult status;
        if ( !foundInterface )
            status = QueryResult.E_NOINTERFACE;
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = QueryResult.S_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }
    */    


///////////////////////////////////////////////////////////////
// testing out some nice ISupports helper macros 


#define BEGIN_QUERY_INTERFACE_BODY(classname)\
    QueryResult classname::QueryInterface( iid_t iid, void** ppinstance )\
{\
    assert(ppinstance); /* TODO: add a real message: "QueryInterface requires a non-NULL destination!");*/\
    \
    if ( !ppinstance )\
    return e_NULL_POINTER;\
    \
    ISupports* foundInterface;\
    if (false)\
    ;\

#define BEGIN_QUERY_INTERFACE_DERIVED(classname, basename)\
    BEGIN_QUERY_INTERFACE_BODY(classname) // for symmetry with the different END_QUERY part

#define HANDLE_INTERFACE(interface_class)\
    else if ( iid == GET_IID(interface_class) ) \
    foundInterface = static_cast<interface_class*>(this);

#define HANDLE_ISUPPORTS_VIA(via_interface)\
    else if ( iid == GET_IID(ISupports) )\
    foundInterface = static_cast<ISupports*>(static_cast<via_interface*>(this));

#define END_QUERY_INTERFACE_BODY(classname)\
    else\
    foundInterface = 0;\
    \
    QueryResult status;\
    if ( !foundInterface )\
    status = e_NOINTERFACE;\
    else\
{\
    foundInterface->AddRef();\
    status = s_OK;\
}\
    \
    *ppinstance = foundInterface;\
    return status;\
}

#define END_QUERY_INTERFACE_DERIVED(classname, basename)\
    else\
    foundInterface = 0;\
    \
    QueryResult status;\
    if ( !foundInterface )\
    status = basename::QueryInterface(iid, (void**)&foundInterface);\
    else\
{\
    foundInterface->AddRef();\
    status = s_OK;\
}\
    \
    *ppinstance = foundInterface;\
    return status;\
}\

#define IMPL_QUERY_INTERFACE0(classname)\
    BEGIN_QUERY_INTERFACE_BODY(classname)\
    HANDLE_INTERFACE(ISupports)\
    END_QUERY_INTERFACE_BODY(classname)

#define IMPL_QUERY_INTERFACE1(classname, if1)\
    BEGIN_QUERY_INTERFACE_BODY(classname)\
    HANDLE_INTERFACE(if1)\
    HANDLE_ISUPPORTS_VIA(if1)\
    END_QUERY_INTERFACE_BODY(classname)

#define IMPL_QUERY_INTERFACE2(classname, if1, if2)\
    BEGIN_QUERY_INTERFACE_BODY(classname)\
    HANDLE_INTERFACE(if1)\
    HANDLE_INTERFACE(if2)\
    HANDLE_ISUPPORTS_VIA(if1)\
    END_QUERY_INTERFACE_BODY(classname)
}
