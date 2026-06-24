
#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"

SETUP_LOGGING( "WaningEffectBox" )

namespace Kernel
{
    // --------------------------- WaningEffectBox ---------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectBox)
    BEGIN_QUERY_INTERFACE_BODY(WaningEffectBox)
    END_QUERY_INTERFACE_BODY(WaningEffectBox)

    WaningEffectBox::WaningEffectBox()
    : WaningEffectConstant()
    , boxDuration( 0.0f )
    {
    }

    WaningEffectBox::WaningEffectBox( const WaningEffectBox& rOrig )
    : WaningEffectConstant( rOrig )
    , boxDuration( rOrig.boxDuration )
    {
    }

    IWaningEffect* WaningEffectBox::Clone()
    {
        return new WaningEffectBox( *this );
    }

    bool WaningEffectBox::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Box_Duration",   &boxDuration,   WEB_Box_Duration_DESC_TEXT,   0, 100000, 100);

        bool ret = WaningEffectConstant::Configure(pInputJson);
        LOG_VALID_F( "WaningEffectBox configured and initialized with effect = %f and boxDuration = %f.\n", currentEffect, boxDuration );
        return ret;
    }

    void  WaningEffectBox::Update(float dt)
    {
        boxDuration -= dt;
        if ( boxDuration < 0 )
        {
            currentEffect = 0;
        }
        LOG_VALID_F( "boxDuration = %f, currentEffect = %f.\n", boxDuration, currentEffect );
    }

    REGISTER_SERIALIZABLE(WaningEffectBox);

    void WaningEffectBox::serialize(IArchive& ar, WaningEffectBox* obj)
    {
        WaningEffectConstant::serialize( ar, obj );
        WaningEffectBox& effect = *obj;
        ar.labelElement("boxDuration") & effect.boxDuration;
    }
}
