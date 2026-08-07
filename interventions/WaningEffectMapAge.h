#pragma once

#include <string>
#include <map>

#include "WaningEffectMap.h"

namespace Kernel
{
    class IDMAPI WaningEffectMapLinearAge : public WaningEffectMapLinear
    {
        DECLARE_FACTORY_REGISTERED( WaningEffectFactory, WaningEffectMapLinearAge, IWaningEffect )
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapLinearAge();
        WaningEffectMapLinearAge( const WaningEffectMapLinearAge& rOrig );
        virtual ~WaningEffectMapLinearAge();

        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual float GetMultiplier( float timeSinceStart ) const override;

    protected:
        virtual bool ConfigureExpiration( const Configuration* config ) override;
        virtual bool ConfigureReferenceTimer( const Configuration* config ) override;

        IIndividualHumanContext* m_Parent;

        DECLARE_SERIALIZABLE( WaningEffectMapLinearAge );
    };
}