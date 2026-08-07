
#pragma once

#include <string>
#include <map>
#include <vector>

#include "Configure.h"
#include "Properties.h"
#include "NodeProperties.h"
#include "JsonConfigurableCollection.h"

namespace Kernel
{
    struct IIndividualHumanEventContext;

    template<class Key, class KeyValue, class Container>
    class IDMAPI PropertyRestrictions : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
    public:
        PropertyRestrictions();
        virtual void ConfigureFromJsonAndKey( const Configuration *, const std::string &key ) override;

        virtual json::QuickBuilder GetSchema() override;

        int Size() const;
        void Add( std::map< std::string, std::string >& rMap );
        void Add(KeyValue& kv);
        bool Qualifies( const Container& rPropertiesContainer ) const;
        std::string GetAsString() const;

    private:
        std::list< Container > _restrictions;
    };
}
