
#pragma once

#include "Configure.h"
#include "IAdditionalRestrictions.h"

namespace Kernel
{
    ENUM_DEFINE( MoreOrLessType,
        ENUM_VALUE_SPEC( LESS , 0 )
        ENUM_VALUE_SPEC( MORE , 1 ) )


    class AdditionalRestrictionsAbstract : public IAdditionalRestrictions, public JsonConfigurable
    {
    public:
        AdditionalRestrictionsAbstract();
        virtual bool Configure(const Configuration* config) override;

        virtual IConfigurable*  GetConfigurable()  override  { return JsonConfigurable::GetConfigurable(); }

    protected:
        bool m_CompareTo;
    };
}
