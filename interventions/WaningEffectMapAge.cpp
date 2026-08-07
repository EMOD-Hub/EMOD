
#include "stdafx.h"

#include "WaningEffectMapAge.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "WaningEffectMapAge" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectMapAge
    // ------------------------------------------------------------------------
    IMPLEMENT_FACTORY_REGISTERED( WaningEffectMapLinearAge )
    BEGIN_QUERY_INTERFACE_DERIVED( WaningEffectMapLinearAge, WaningEffectMapLinear )
    END_QUERY_INTERFACE_DERIVED( WaningEffectMapLinearAge, WaningEffectMapLinear )

    WaningEffectMapLinearAge::WaningEffectMapLinearAge()
        : WaningEffectMapLinear( MAX_HUMAN_AGE )
        , m_Parent( nullptr )
    {
    }

    WaningEffectMapLinearAge::WaningEffectMapLinearAge( const WaningEffectMapLinearAge& rOrig )
        : WaningEffectMapLinear( rOrig )
        , m_Parent( nullptr )
    {
    }

    WaningEffectMapLinearAge::~WaningEffectMapLinearAge()
    {
    }

    bool WaningEffectMapLinearAge::ConfigureExpiration( const Configuration* config )
    {
        // disable m_ExpireAtDurationMapEnd
        return true;
    }

    bool WaningEffectMapLinearAge::ConfigureReferenceTimer( const Configuration* config )
    {
        // disable Reference_Timer
        return true;
    }

    float WaningEffectMapLinearAge::GetMultiplier( float timeSinceStart ) const
    {
        if( m_Parent == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "SetContextTo() has not been called properly." );
        }
        float age_years = m_Parent->GetEventContext()->GetAge() / DAYSPERYEAR;
        return WaningEffectMapLinear::GetMultiplier( age_years );
    }

    void WaningEffectMapLinearAge::SetContextTo( IIndividualHumanContext *context )
    {
        m_Parent = context;
    }


    REGISTER_SERIALIZABLE( WaningEffectMapLinearAge );

    void WaningEffectMapLinearAge::serialize( IArchive& ar, WaningEffectMapLinearAge* obj )
    {
        WaningEffectMapLinear::serialize( ar, obj );
        WaningEffectMapLinearAge& effect = *obj;
    }
}
