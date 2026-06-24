
#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"

SETUP_LOGGING( "WaningEffectConstant" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectConstant)
    BEGIN_QUERY_INTERFACE_BODY(WaningEffectConstant)
    END_QUERY_INTERFACE_BODY(WaningEffectConstant)

    WaningEffectConstant::WaningEffectConstant()
    : IWaningEffect()
    , JsonConfigurable()
    , currentEffect(0.0f)
    , usingDefault(false)
    {
    }

    WaningEffectConstant::WaningEffectConstant( const WaningEffectConstant& rOrig )
    : JsonConfigurable( rOrig )
    , currentEffect( rOrig.currentEffect )
    , usingDefault(rOrig.usingDefault)
    {
    }

    IWaningEffect* WaningEffectConstant::Clone()
    {
        return new WaningEffectConstant( *this );
    }

    bool WaningEffectConstant::Configure( const Configuration * pInputJson )
    {
         initConfigTypeMap("Initial_Effect", &currentEffect, WE_Initial_Effect_DESC_TEXT, 0, 1, notSetByUser);
        
         bool ret= JsonConfigurable::Configure(pInputJson);
        
        if (currentEffect == notSetByUser)
        {
            usingDefault = true;
            currentEffect = 1.0;
        }

        return ret;
    }

    void  WaningEffectConstant::Update(float dt)
    {
        // constant effect = nothing to update
        return;
    }

    float WaningEffectConstant::Current() const
    {
        return currentEffect;
    }

    void WaningEffectConstant::SetInitial(float newVal)
    {
        if (currentEffect != newVal && !usingDefault)
        {
            LOG_DEBUG_F("Overriding Initial_Effect with: %f \n", newVal);
        }
        currentEffect = newVal;
    }

    REGISTER_SERIALIZABLE(WaningEffectConstant);

    void WaningEffectConstant::serialize(IArchive& ar, WaningEffectConstant* obj)
    {
        WaningEffectConstant& effect = *obj;
        ar.labelElement("currentEffect") & effect.currentEffect;
    }
}
