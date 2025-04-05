
#pragma once

#include <functional>
#include "FactorySupport.h"
#include "Interventions.h"
#include "ObjectFactory.h"

namespace Kernel
{
    class InterventionFactory
    {
    public:
        // returns NULL if could not create a distributable intervention with the specified definition
        static IDistributableIntervention* CreateIntervention( const json::Element& rJsonElement,
                                                               const std::string& rDataLocation,
                                                               const char* parameterName,
                                                               bool throwIfNull=false );
        static void CreateInterventionList( const json::Element& rJsonElement,
                                            const std::string& rDataLocation,
                                            const char* parameterName,
                                            std::vector<IDistributableIntervention*>& interventionsList );

        // returns NULL if could not create a node distributable intervention with the specified definition
        static INodeDistributableIntervention* CreateNDIIntervention( const json::Element& rJsonElement,
                                                                      const std::string& rDataLocation,
                                                                      const char* parameterName,
                                                                      bool throwIfNull=false );
        static void CreateNDIInterventionList( const json::Element& rJsonElement,
                                               const std::string& rDataLocation,
                                               const char* parameterName,
                                               std::vector<INodeDistributableIntervention*>& interventionsList );

        static void SetUseDefaults( bool useDefaults );
        static bool IsUsingDefaults();

    protected:
        static bool m_UseDefaults;
    };

    class IndividualIVFactory : public ObjectFactory<IDistributableIntervention, IndividualIVFactory>
    {
    public:
        void ValidateSimType( IDistributableIntervention* );

    protected:
        template<class IObject, class Factory> friend class Kernel::ObjectFactory;
        IndividualIVFactory();
    };

    class NodeIVFactory : public ObjectFactory<INodeDistributableIntervention, NodeIVFactory>
    {
    public:
        void ValidateSimType( INodeDistributableIntervention* );

    protected:
        template<class IObject, class Factory> friend class Kernel::ObjectFactory;
        NodeIVFactory();
    };
}
