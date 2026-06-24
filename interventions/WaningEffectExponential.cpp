
#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "IArchive.h"

SETUP_LOGGING( "WaningEffectExponential" )

namespace Kernel
{
    // --------------------------- WaningEffectExponential ---------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectExponential)
    BEGIN_QUERY_INTERFACE_BODY(WaningEffectExponential)
    END_QUERY_INTERFACE_BODY(WaningEffectExponential)

    WaningEffectExponential::WaningEffectExponential()
    : WaningEffectConstant()
    , decayTimeConstant( 0.0f )
    {
    }

    WaningEffectExponential::WaningEffectExponential( const WaningEffectExponential& rOrig )
    : WaningEffectConstant( rOrig )
    , decayTimeConstant( rOrig.decayTimeConstant )
    {
    }

    IWaningEffect* WaningEffectExponential::Clone()
    {
        return new WaningEffectExponential( *this );
    }

    bool WaningEffectExponential::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Decay_Time_Constant", &decayTimeConstant, WEE_Decay_Time_Constant_DESC_TEXT, 0, 100000, 100);
        return WaningEffectConstant::Configure( pInputJson );
    }

    void  WaningEffectExponential::Update(float dt)
    {
        if ( decayTimeConstant > dt )
        {
            currentEffect *= (1 - dt/decayTimeConstant);
        }
        else
        {
            currentEffect = 0;
        }
    }

    REGISTER_SERIALIZABLE(WaningEffectExponential);

    void WaningEffectExponential::serialize(IArchive& ar, WaningEffectExponential* obj)
    {
        WaningEffectConstant::serialize( ar, obj );
        WaningEffectExponential& effect = *obj;
        ar.labelElement("decayTimeConstant") & effect.decayTimeConstant;
    }
}
