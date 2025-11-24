
#pragma once

#include <string>
#include <map>

#include "IWaningEffect.h"
#include "ISerializable.h"
#include "Configuration.h"
#include "Configure.h"
#include "FactorySupport.h"

namespace Kernel
{
    class WaningEffectCollection : public JsonConfigurable, public IComplexJsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        WaningEffectCollection();
        virtual ~WaningEffectCollection();

        // IComplexJsonConfigurable methods
        virtual bool  HasValidDefault() const override { return true; }
        virtual json::QuickBuilder GetSchema() override;
        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;

        virtual void CheckConfiguration();
        virtual void Add( IWaningEffect* pwe );
        int Size() const;
        IWaningEffect* operator[]( int index ) const;

        static void serialize( IArchive& ar, WaningEffectCollection& map );

    protected:
        std::vector<IWaningEffect*> m_Collection;
    };

    class WaningEffectCombo : public JsonConfigurable, public IWaningEffect, public IWaningEffectCount
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( WaningEffectFactory, WaningEffectCombo, IWaningEffect )

    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        WaningEffectCombo();
        virtual ~WaningEffectCombo();

        virtual bool Configure( const Configuration *config ) override;

        // IWaningEffect methods
        virtual IWaningEffect* Clone() override;
        virtual void  Update( float dt ) override;
        virtual float Current() const override;
        virtual bool  Expired() const override;
        virtual void  SetContextTo( IIndividualHumanContext *context ) override;
        virtual void  SetInitial( float newVal ) override;
        virtual void  SetCurrentTime( float dt ) override;

        // IWaningEffectCount methods
        virtual void SetCount( uint32_t numCounts ) override;
        virtual bool IsValidConfiguration( uint32_t maxCount ) const override;

    protected:
        WaningEffectCombo( WaningEffectCombo& rOrig );

        bool m_IsAdditive;
        bool m_IsExpiringWhenAllExpire;
        WaningEffectCollection m_EffectCollection;

        DECLARE_SERIALIZABLE( WaningEffectCombo );
    };
}
