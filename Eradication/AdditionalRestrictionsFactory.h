
#pragma once

#include <string>
#include <map>

#include "Configure.h"
#include "IAdditionalRestrictions.h"
#include "ObjectFactory.h"

namespace Kernel
{
    class AdditionalRestrictionsFactory : public ObjectFactory<IAdditionalRestrictions, AdditionalRestrictionsFactory>
    {
    public:
        virtual IAdditionalRestrictions* CreateInstance( const json::Element& rJsonElement,
                                                         const std::string& rDataLocation,
                                                         const char* parameterName) override;
    };
}
