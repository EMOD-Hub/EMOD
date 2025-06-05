
#pragma once

#include <functional>
#include "FactorySupport.h"

namespace Kernel
{
    // ObjectFactory is a template for creating a factory that can create similar objects based
    // on a JSON object/dictionary.  This is a fantastic/sneaky way to be able to add new objects
    // without the need to modify any existing code.
    // - IObject - This is typically an interface that all objects of this time must implement.
    //             This object must be JsonConfigurable.
    // - Factory - This is the name of the new factory.
    // A new class of objects must implement the IObject interface and use the DECLARE_FACTORY_REGISTERED()
    // macro in its header and the IMPLEMENT_FACTORY_REGISTERED() in its CPP file.  These macros
    // need to reference this new factory.  During the creation of the static variables
    // (i.e. before calling main()), the new class will register itself with the factory.
    // Later one can use the CreateInstance() and a JSON Object/dictionary to create one of these
    // objects.  However, one of the elements of this JSON dictionary should have a key named 'class'
    // and the value of 'class' should be the name of the class objects.  The factory will use this
    // class name to find the registered no argument constructor, create the new object, and configuring
    // from the input JSON.
    template<class IObject, class Factory>
    class ObjectFactory
    {
    public:
        static Factory* getInstance();

        virtual ~ObjectFactory();

        // Used in DECLARE_FACTORY_REGISTERED() to register classes with this factory.
        virtual void Register( const char *classname, instantiator_function_t _if );

        // Return the schema of all of the classes that have registered with this factory.
        virtual json::QuickBuilder GetSchema();

        // Return a new configured instance of the object defined by 'pConfig'
        virtual IObject* CreateInstance( const json::Element& rJsonElement,
                                         const std::string& rDataLocation,
                                         const char* parameterName );

        // check that the input JSON ('pConfig') is valid
        // skip_exceptions ignores no-class-def and wrong-class-def; does not ignore bad formatting
        bool ElementIsValid( const json::Element& rJsonElement,
                             const std::string& rDataLocation,
                             const char* parameterName,
                             bool skip_exceptions=true );

    protected:
        ObjectFactory( bool queryForReturnInterface=true );

        // Provides a hook for the factory to add other stuff to the schema for an object
        virtual void ModifySchema( json::QuickBuilder& rSchema, ISupports*pObject ){};

        // Returns the name fo the factory
        std::string GetFactoryName();

        void CheckSimType( ISupports* pObject );

        support_spec_map_t m_RegisteredClasses;
        json::Object       m_FactorySchema;
        bool               m_QueryForReturnInterface;

        static Factory* _instance;
    };
}
