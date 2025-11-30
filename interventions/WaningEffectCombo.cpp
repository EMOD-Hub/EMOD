
#include "stdafx.h"

#include "WaningEffectCombo.h"

SETUP_LOGGING( "WaningEffectCombo" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectCollection
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( WaningEffectCollection )
    END_QUERY_INTERFACE_BODY( WaningEffectCollection )

    WaningEffectCollection::WaningEffectCollection()
        : JsonConfigurable()
    {
    }

    WaningEffectCollection::~WaningEffectCollection()
    {
        for( auto p_effect : m_Collection )
        {
            delete p_effect;
        }
        m_Collection.clear();
    }

    json::QuickBuilder WaningEffectCollection::GetSchema()
    {
        WaningConfig waning_config;

        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:WaningEffectCollection" );

        schema[ ts ] = json::Object();
        schema[ ts ][ "default" ] = json::Array();
        schema[ ts ][ "description" ] = json::String(WEC_Schema_Type_DESC_TEXT);
        schema[ ts ][ "type" ] = json::String("Vector idmAbstractType:WaningEffect");

        return schema;
    }

    void WaningEffectCollection::ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key )
    {
        // Temporary object created so we can 'operate' on json with the desired tools
        auto p_config = Configuration::CopyFromElement( (*inputJson)[ key ], inputJson->GetDataLocation() );

        int i = 0;
        const auto& json_array = json_cast<const json::Array&>((*p_config));
        for( auto data = json_array.Begin(); data != json_array.End(); ++data )
        {
            std::stringstream param_name;
            param_name << "WaningEffectCollection[" << i << "]";
            IWaningEffect* p_iwe = WaningEffectFactory::getInstance()->CreateInstance( *data,
                                                                                       inputJson->GetDataLocation(),
                                                                                       param_name.str().c_str() );
            Add( p_iwe );
            ++i;
        }
        delete p_config;
    }

    // Other methods
    void WaningEffectCollection::CheckConfiguration()
    {
    }

    void WaningEffectCollection::Add( IWaningEffect* pwe )
    {
        m_Collection.push_back( pwe );
    }

    int WaningEffectCollection::Size() const
    {
        return m_Collection.size();
    }

    IWaningEffect* WaningEffectCollection::operator[]( int index ) const
    {
        return m_Collection[ index ];
    }

    void WaningEffectCollection::serialize( IArchive& ar, WaningEffectCollection& obj )
    {
        ar & obj.m_Collection;
    }

    // ------------------------------------------------------------------------
    // --- WaningEffectCollection
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED( WaningEffectCombo )

    BEGIN_QUERY_INTERFACE_BODY( WaningEffectCombo )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IWaningEffect )
        HANDLE_INTERFACE( IWaningEffectCount )
        HANDLE_ISUPPORTS_VIA( IWaningEffect )
    END_QUERY_INTERFACE_BODY( WaningEffectCombo )


    WaningEffectCombo::WaningEffectCombo()
        : IWaningEffect()
        , JsonConfigurable()
        , m_IsAdditive( false )
        , m_IsExpiringWhenAllExpire( false )
        , m_EffectCollection()
    {
    }

    WaningEffectCombo::WaningEffectCombo( WaningEffectCombo& rOrig )
        : JsonConfigurable( rOrig )
        , m_IsAdditive( rOrig.m_IsAdditive )
        , m_IsExpiringWhenAllExpire( rOrig.m_IsExpiringWhenAllExpire )
        , m_EffectCollection()
    {
        for( int i = 0 ; i < rOrig.m_EffectCollection.Size() ; ++i )
        {
            m_EffectCollection.Add( rOrig.m_EffectCollection[ i ]->Clone() );
        }
    }

    WaningEffectCombo::~WaningEffectCombo()
    {
    }

    IWaningEffect* WaningEffectCombo::Clone()
    {
        return new WaningEffectCombo( *this );
    }

    bool WaningEffectCombo::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap( "Add_Effects", &m_IsAdditive, WEC_Add_Effects_DESC_TEXT, false );
        initConfigTypeMap( "Expires_When_All_Expire", &m_IsExpiringWhenAllExpire, WEC_Expires_When_All_Expire_DESC_TEXT, false );
        initConfigComplexType( "Effect_List", &m_EffectCollection, WEC_Effect_List_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( pInputJson );
        if( ret )
        {
            m_EffectCollection.CheckConfiguration();
        }

        return ret;
    }

    void  WaningEffectCombo::Update( float dt )
    {
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            IWaningEffectCount* p_count_effect = nullptr;
            if( s_OK != m_EffectCollection[ i ]->QueryInterface( GET_IID( IWaningEffectCount ), (void**)&p_count_effect ) )
            {
                m_EffectCollection[ i ]->Update( dt );
            }
        }
    }

    void  WaningEffectCombo::SetCount( uint32_t numCounts )
    {
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            IWaningEffectCount* p_count_effect = nullptr;
            if( s_OK == m_EffectCollection[ i ]->QueryInterface( GET_IID( IWaningEffectCount ), (void**)&p_count_effect ) )
            {
                p_count_effect->SetCount( numCounts );
            }
        }
    }

    bool WaningEffectCombo::IsValidConfiguration( uint32_t maxCount ) const
    {
        bool valid = true;
        for( int i = 0; valid && (i < m_EffectCollection.Size()); ++i )
        {
            IWaningEffectCount* p_count_effect = nullptr;
            if( s_OK == m_EffectCollection[ i ]->QueryInterface( GET_IID( IWaningEffectCount ), (void**)&p_count_effect ) )
            {
                valid = p_count_effect->IsValidConfiguration( maxCount );
            }
        }
        return valid;
    }

    float WaningEffectCombo::Current() const
    {
        float current = 1.0;
        if( m_IsAdditive )
        {
            current = 0.0;
        }
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            float i_current = m_EffectCollection[ i ]->Current();
            if( m_IsAdditive )
            {
                current += i_current;
            }
            else
            {
                current *= i_current;
            }
        }
        if( current > 1.0 )
        {
            current = 1.0;
        }

        return current;
    }

    bool WaningEffectCombo::Expired() const
    {
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            bool i_expired = m_EffectCollection[ i ]->Expired();
            if( m_IsExpiringWhenAllExpire && !i_expired )
            {
                // One has not expired, so don't expire
                return false;
            }
            else if( !m_IsExpiringWhenAllExpire && i_expired )
            {
                // Expire on the first one that has expired
                return true;
            }
        }

        // if this is true and all effects are true, then return true-expire
        // if this is false and all effects are false, then return false-don't expire
        return m_IsExpiringWhenAllExpire;
    }

    void WaningEffectCombo::SetContextTo( IIndividualHumanContext *context )
    {
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            m_EffectCollection[ i ]->SetContextTo( context );
        }
    }

    void WaningEffectCombo::SetInitial( float newVal )
    {
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            m_EffectCollection[ i ]->SetInitial( newVal );
        }
    }

    void WaningEffectCombo::SetCurrentTime( float dt )
    {
        for( int i = 0; i < m_EffectCollection.Size(); ++i )
        {
            m_EffectCollection[ i ]->SetCurrentTime( dt );
        }
    }

    REGISTER_SERIALIZABLE( WaningEffectCombo );

    void WaningEffectCombo::serialize( IArchive& ar, WaningEffectCombo* obj )
    {
        WaningEffectCombo& effect = *obj;
        ar.labelElement( "m_IsAdditive"              ) & effect.m_IsAdditive;
        ar.labelElement( "m_IsExpiringWhenAllExpire" ) & effect.m_IsExpiringWhenAllExpire;
        ar.labelElement( "m_EffectCollection"        ) & effect.m_EffectCollection;
    }
}
