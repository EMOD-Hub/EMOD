
#pragma once

#include <string>
#include <map>

#include "WaningEffectMap.h"

namespace Kernel
{
    class IDMAPI WaningEffectMapLinearSeasonal : public WaningEffectMapLinear
    {
        DECLARE_FACTORY_REGISTERED( WaningEffectFactory, WaningEffectMapLinearSeasonal, IWaningEffect )
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapLinearSeasonal();
        WaningEffectMapLinearSeasonal( const WaningEffectMapLinearSeasonal& rOrig );
        virtual ~WaningEffectMapLinearSeasonal();

        virtual void  Update( float dt ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;

    protected:
        virtual bool ConfigureExpiration( const Configuration* config ) override;
        virtual bool ConfigureReferenceTimer( const Configuration* config ) override;

        IIndividualHumanContext* m_Parent;

        DECLARE_SERIALIZABLE( WaningEffectMapLinearSeasonal );
    };
}