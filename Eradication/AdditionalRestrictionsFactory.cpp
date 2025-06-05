
#include "stdafx.h"

#include "AdditionalRestrictionsFactory.h"
#include "ObjectFactoryTemplates.h"

SETUP_LOGGING("AdditionalRestrictionsFactory")

namespace Kernel
{
    AdditionalRestrictionsFactory * AdditionalRestrictionsFactory::_instance = nullptr;

    template AdditionalRestrictionsFactory* ObjectFactory<IAdditionalRestrictions, AdditionalRestrictionsFactory>::getInstance();

    IAdditionalRestrictions* AdditionalRestrictionsFactory::CreateInstance( const json::Element& rJsonElement,
                                                                            const std::string& rDataLocation,
                                                                            const char* parameterName )
    {
        IAdditionalRestrictions* p_ar = 
            ObjectFactory<IAdditionalRestrictions, AdditionalRestrictionsFactory>::CreateInstance( rJsonElement,
                                                                                                   rDataLocation,
                                                                                                   parameterName );

        if( p_ar != nullptr )
        {
            // ------------------------------------------------------------------------
            // --- Verify that the object is supported for the current simulation type
            // ------------------------------------------------------------------------
            CheckSimType( p_ar );
        }
        return p_ar;
    }
}