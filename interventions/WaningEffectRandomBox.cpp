
#include "stdafx.h"

#include "WaningEffectRandomBox.h"
#include "CajunIncludes.h"
#include "IArchive.h"
#include "IIndividualHumanContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "WaningEffectRandomBox" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectRandomBox
    // ------------------------------------------------------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectRandomBox)
    BEGIN_QUERY_INTERFACE_BODY(WaningEffectRandomBox)
    END_QUERY_INTERFACE_BODY(WaningEffectRandomBox)

    WaningEffectRandomBox::WaningEffectRandomBox()
    : WaningEffectConstant()
    , m_ExpectedDiscardTime(0.0)
    , m_DiscardCounter(FLT_MAX)
    {
    }

    WaningEffectRandomBox::WaningEffectRandomBox( const WaningEffectRandomBox& master )
    : WaningEffectConstant( master )
    , m_ExpectedDiscardTime( master.m_ExpectedDiscardTime )
    , m_DiscardCounter( master.m_DiscardCounter )
    {
    }

    WaningEffectRandomBox::~WaningEffectRandomBox()
    {
    }

    IWaningEffect* WaningEffectRandomBox::Clone()
    {
        return new WaningEffectRandomBox( *this );
    }

    bool WaningEffectRandomBox::Configure(const Configuration * pInputJson)
    {
        initConfigTypeMap("Expected_Discard_Time", &m_ExpectedDiscardTime, WERB_Expected_Discard_Time_DESC_TEXT, 0, 100000, 100);
        
        bool ret = WaningEffectConstant::Configure(pInputJson);

        return ret;
    }

    void WaningEffectRandomBox::SetContextTo( IIndividualHumanContext *context )
    {
        if( m_ExpectedDiscardTime == 0 )
        {
            m_DiscardCounter = 0;
        }
        else if( m_DiscardCounter == FLT_MAX )
        {
            m_DiscardCounter = context->GetRng()->expdist( 1.0 / m_ExpectedDiscardTime );
        }
        LOG_DEBUG_F( "Discard after %0.1f days based on expected duration of %0.1f\n", m_DiscardCounter, m_ExpectedDiscardTime );
    }

    void WaningEffectRandomBox::Update(float dt)
    {
        m_DiscardCounter -= dt;
        LOG_DEBUG_F("Discard counter = %0.1f\n", m_DiscardCounter);

        if( Expired() ) 
        {
            LOG_DEBUG("Waning effect has expired.\n");
            currentEffect = 0.0f;
        }
    }

    bool WaningEffectRandomBox::Expired() const
    {
        return m_DiscardCounter <= 0.0f;
    }

    REGISTER_SERIALIZABLE(WaningEffectRandomBox);

    void WaningEffectRandomBox::serialize(IArchive& ar, WaningEffectRandomBox* obj)
    {
        WaningEffectConstant::serialize( ar, obj );
        WaningEffectRandomBox& effect = *obj; 
        ar.labelElement( "m_ExpectedDiscardTime" ) & effect.m_ExpectedDiscardTime;
        ar.labelElement( "m_DiscardCounter"      ) & effect.m_DiscardCounter;
    }
}