
#pragma once

#include "IdmApi.h"
#include <string>
#include <functional>
#include <map>
#include "ISupports.h"  
#include "Configuration.h"
#include "Sugar.h"
#include "Environment.h"
#include <typeinfo>
#include <stdio.h>
#include "Exceptions.h"

#if defined(WIN32)
#define DTK_DLLEXPORT   __declspec(dllexport)
#else // Other non-windows platform
#define DTK_DLLEXPORT   
#endif

namespace Kernel
{
    using namespace std;

    //////////////////////////////////////////////////////////////////////////
    // CreateInstance/ClassFactory helpers
   
    typedef std::function<ISupports* (void)> instantiator_function_t;
    typedef map<string, instantiator_function_t> support_spec_map_t;

    template <class ReturnTypeT>
    ReturnTypeT* CreateInstanceFromSpecs(const Configuration* config, support_spec_map_t &specs)
    {
        string classname = "PREPARSED_CLASSNAME";
        try {
            classname = GET_CONFIG_STRING(config, "class");
        }
        catch( JsonTypeConfigurationException& except )
        {
            std::ostringstream errMsg;
            string templateClassName = typeid(ReturnTypeT).name();
            templateClassName = templateClassName.substr( templateClassName.find_last_of("::")+1 );
            errMsg << "'" 
                   << templateClassName
                   << "' could not instantiate object from json because class was not specified as required.\nDetails from caught exception: "
                   << std::endl
                   << except.GetMsg()
                   << std::endl;
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        }

        ISupports* obj = nullptr;

        map<string, instantiator_function_t>::iterator it = specs.find(classname);
        if (specs.end() == it)
        {
            std::ostringstream errMsg;
            errMsg << "Could not instantiate unknown class '" << classname << "'." << std::endl;
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        } 
        else
        {
            obj = it->second(); // create object
            obj->AddRef(); // increment reference counting for 'obj'

            IConfigurable* conf_obj = obj->GetConfigurable();
            release_assert(conf_obj);

            if (!conf_obj->Configure(config))
            {
                // release references to the objects
                obj->Release();
                return nullptr;
            }
        }

        // returning a plain object pointer type, force casted
        return static_cast<ReturnTypeT*>(obj); // obj should have a reference count of 1
    }

#define DECLARE_FACTORY_REGISTERED(factoryname, classname, via_interface) \
    private: \
    class IDMAPI RegistrationHookCaller\
    {\
    public:\
        RegistrationHookCaller()\
        {\
            classname::in_class_registration_hook();\
        }\
    };\
    static void in_class_registration_hook()\
    {\
        factoryname::getInstance()->Register(#classname, \
            []() { return (ISupports*)(via_interface*)(_new_ classname()); });\
    }\
    static RegistrationHookCaller registration_hook_caller; \
    virtual classname * Clone() { return new classname( *this ); }

#define IMPLEMENT_FACTORY_REGISTERED(classname) \
    classname::RegistrationHookCaller classname::registration_hook_caller;


// This definition will be gone eventually once we do campaign/event coordinator dll
#define DECLARE_FACTORY_REGISTERED_EXPORT(factoryname, classname, via_interface) \
    private: \
    class DTK_DLLEXPORT RegistrationHookCaller\
    {\
    public:\
        RegistrationHookCaller()\
        {\
            classname::in_class_registration_hook();\
        }\
    };\
    static void in_class_registration_hook()\
    {\
        factoryname::getInstance()->Register(#classname, \
            []() { return (ISupports*)(via_interface*)(_new_ classname()); });\
    }\
    static RegistrationHookCaller registration_hook_caller;
}
