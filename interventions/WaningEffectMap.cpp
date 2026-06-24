
#include "stdafx.h"

#include "WaningEffectMap.h"
#include "CajunIncludes.h"
#include "IArchive.h"

SETUP_LOGGING( "WaningEffectMapAbstract" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectMapLinear
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(WaningEffectMapAbstract)
    END_QUERY_INTERFACE_BODY(WaningEffectMapAbstract)

    WaningEffectMapAbstract::WaningEffectMapAbstract( float maxTime )
    : WaningEffectConstant()
    , m_Expired(false)
    , m_EffectOriginal(0.0)
    , m_ExpireAtDurationMapEnd(false)
    , m_TimeSinceStart(0.0)
    , m_DurationMap( 0.0, maxTime )
    , m_RefTime(0)
    {
    }

    WaningEffectMapAbstract::WaningEffectMapAbstract( float minTime, float maxTime, float minValue, float maxValue )
    : WaningEffectConstant()
    , m_Expired( false )
    , m_EffectOriginal( 0.0 )
    , m_ExpireAtDurationMapEnd( false )
    , m_TimeSinceStart( 0.0 )
    , m_DurationMap( minTime, maxTime, minValue, maxValue )
    , m_RefTime( 0 )
    {
    }


    WaningEffectMapAbstract::WaningEffectMapAbstract( const WaningEffectMapAbstract& rOrig )
    : WaningEffectConstant( rOrig )
    , m_Expired( rOrig.m_Expired )
    , m_EffectOriginal( rOrig.m_EffectOriginal )
    , m_ExpireAtDurationMapEnd( rOrig.m_ExpireAtDurationMapEnd )
    , m_TimeSinceStart( rOrig.m_TimeSinceStart )
    , m_DurationMap( rOrig.m_DurationMap )
    , m_RefTime( rOrig.m_RefTime )
    {
    }

    WaningEffectMapAbstract::~WaningEffectMapAbstract()
    {
    }

    bool WaningEffectMapAbstract::Configure( const Configuration * pInputJson )
    {
        bool configured = ConfigureExpiration( pInputJson );

        if( configured || JsonConfigurable::_dryrun )
        {
            configured = ConfigureReferenceTimer( pInputJson );
        }

        if( configured || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Durability_Map", &m_DurationMap, WEM_Durability_Map_DESC_TEXT );

            configured = WaningEffectConstant::Configure( pInputJson );
            if( configured && !JsonConfigurable::_dryrun )
            {
                m_EffectOriginal = currentEffect;

                if( m_DurationMap.size() == 0 )
                {
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Durability_Map has no values.  Please add values to the map." );
                }
            }
        }
        return configured;
    }

    bool WaningEffectMapAbstract::ConfigureExpiration( const Configuration * pInputJson )
    {
        initConfigTypeMap( "Expire_At_Durability_Map_End", &m_ExpireAtDurationMapEnd, WEM_Expire_At_Durability_Map_End_DESC_TEXT, false );
        return true;
    }

    bool WaningEffectMapAbstract::ConfigureReferenceTimer( const Configuration * pInputJson )
    {
        initConfigTypeMap( "Reference_Timer", &m_RefTime, WEM_Reference_Timer_DESC_TEXT, 0 );
        return true;
    }

    void WaningEffectMapAbstract::SetCurrentTime(float current_time)
    {
        // We want to "fast-forward" the time-since-start for birth-triggered individuals so the map catches up
        // to what it would have been if they'd been alive. In limit, if born on same day as distribution (ref-time)
        // current_time == ref_time and time-since-start = 0; if born at t=inf, time-since-start = inf.
        if( m_RefTime > 0 )
        {
            m_TimeSinceStart = current_time - m_RefTime + 1; // GH issue jgauld/DtkTrunk/issues/187, off-by-one error
            LOG_DEBUG_F( "m_TimeSinceStart set to %f after offset of current_time %f and m_RefTime of %d.\n", float(m_TimeSinceStart), current_time, float(m_RefTime) );
            release_assert( m_TimeSinceStart  > 0.0f );
        }
    }

    void WaningEffectMapAbstract::UpdateEffect()
    {
        float multiplier = GetMultiplier( m_TimeSinceStart );

        if( m_Expired && m_ExpireAtDurationMapEnd )
        {
            currentEffect = 0.0;
        }
        else
        {
            currentEffect = multiplier * m_EffectOriginal;
        }
        LOG_DEBUG_F( "currentEffect = %f.\n", currentEffect );

        if( m_ExpireAtDurationMapEnd )
        {
            m_Expired = m_DurationMap.isAtEnd( m_TimeSinceStart );
        }
    }

    void  WaningEffectMapAbstract::Update(float dt)
    {
        UpdateEffect();
        m_TimeSinceStart += dt;
    }

    bool WaningEffectMapAbstract::Expired() const
    {
        return m_Expired;
    }

    void WaningEffectMapAbstract::SetInitial(float newVal)
    {
        m_EffectOriginal = newVal;
    }

    void WaningEffectMapAbstract::serialize( IArchive& ar, WaningEffectMapAbstract* obj )
    {
        WaningEffectConstant::serialize( ar, obj );
        WaningEffectMapAbstract& effect = *obj;
        ar.labelElement( "m_Expired"                ) & effect.m_Expired;
        ar.labelElement( "m_EffectOriginal"         ) & effect.m_EffectOriginal;
        ar.labelElement( "m_ExpireAtDurationMapEnd" ) & effect.m_ExpireAtDurationMapEnd;
        ar.labelElement( "m_TimeSinceStart"         ) & effect.m_TimeSinceStart;
        ar.labelElement( "m_DurationMap"            ) & effect.m_DurationMap;
    }

    // ------------------------------------------------------------------------
    // --- WaningEffectMapLinear
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(WaningEffectMapLinear)
    BEGIN_QUERY_INTERFACE_DERIVED(WaningEffectMapLinear, WaningEffectMapAbstract)
    END_QUERY_INTERFACE_DERIVED(WaningEffectMapLinear, WaningEffectMapAbstract)

    REGISTER_SERIALIZABLE(WaningEffectMapLinear);


    WaningEffectMapLinear::WaningEffectMapLinear( float maxTime )
    : WaningEffectMapAbstract( maxTime )
    {
    }

    WaningEffectMapLinear::~WaningEffectMapLinear()
    {
    }

    WaningEffectMapLinear::WaningEffectMapLinear( const WaningEffectMapLinear& rOrig )
    : WaningEffectMapAbstract( rOrig )
    {
    }

    IWaningEffect* WaningEffectMapLinear::Clone()
    {
        return new WaningEffectMapLinear( *this );
    }

    float WaningEffectMapLinear::GetMultiplier( float timeSinceStart ) const
    {
        return m_DurationMap.getValueLinearInterpolation( timeSinceStart );
    }

    void WaningEffectMapLinear::serialize( IArchive& ar, WaningEffectMapLinear* obj )
    {
        WaningEffectMapAbstract::serialize( ar, obj );
    }

    // ------------------------------------------------------------------------
    // --- WaningEffectMapPiecewise
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(WaningEffectMapPiecewise)
    BEGIN_QUERY_INTERFACE_DERIVED(WaningEffectMapPiecewise, WaningEffectMapAbstract)
    END_QUERY_INTERFACE_DERIVED(WaningEffectMapPiecewise, WaningEffectMapAbstract)

    REGISTER_SERIALIZABLE(WaningEffectMapPiecewise);


    WaningEffectMapPiecewise::WaningEffectMapPiecewise()
    : WaningEffectMapAbstract()
    {
    }

    WaningEffectMapPiecewise::WaningEffectMapPiecewise( float minTime, float maxTime, float minValue, float maxValue )
        : WaningEffectMapAbstract( minTime, maxTime, minValue, maxValue )
    {
    }

    WaningEffectMapPiecewise::~WaningEffectMapPiecewise()
    {
    }

    WaningEffectMapPiecewise::WaningEffectMapPiecewise( const WaningEffectMapPiecewise& rOrig )
    : WaningEffectMapAbstract( rOrig )
    {
    }

    IWaningEffect* WaningEffectMapPiecewise::Clone()
    {
        return new WaningEffectMapPiecewise( *this );
    }

    float WaningEffectMapPiecewise::GetMultiplier( float timeSinceVaccination ) const
    {
        release_assert( m_DurationMap.size() > 0 );

        float first_time = m_DurationMap.begin()->first;
        float first_mult = m_DurationMap.begin()->second;
        float last_time  = m_DurationMap.rbegin()->first;
        float last_mult  = m_DurationMap.rbegin()->second;

        // ------------------------------------------------------------------------------------
        // --- Times that are outside the range are capped at either the first or last value 
        // ------------------------------------------------------------------------------------
        float rate = 0.0;
        if( timeSinceVaccination <= first_time )
        {
            rate = first_mult;
        }
        else if( timeSinceVaccination >= last_time )
        {
            rate = last_mult;
        }
        else
        {
            rate = m_DurationMap.getValuePiecewiseConstant( timeSinceVaccination, first_mult );
        }
        return rate;
    }

    void WaningEffectMapPiecewise::serialize( IArchive& ar, WaningEffectMapPiecewise* obj )
    {
        WaningEffectMapAbstract::serialize( ar, obj );
    }
}
