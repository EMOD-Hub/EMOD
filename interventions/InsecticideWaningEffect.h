
#pragma once

#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "Insecticides.h"


namespace Kernel
{
    struct IWaningEffect;
    struct IIndividualHumanContext;

    struct IInsecticideWaningEffect : ISerializable
    {
        virtual IInsecticideWaningEffect* Clone() = 0;
        virtual void SetName( const InsecticideName& rName ) = 0;
        virtual void SetContextTo( IIndividualHumanContext *context ) = 0;
        virtual void Update( float dt ) = 0;
        virtual GeneticProbability GetCurrent( ResistanceType::Enum ) const = 0;
        virtual bool Has( ResistanceType::Enum type ) const = 0;
    };

    class InsecticideWaningEffect : public IInsecticideWaningEffect,
                                    public JsonConfigurable
                                    
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        InsecticideWaningEffect();
        InsecticideWaningEffect( bool hasLarvalKilling,
                                 bool hasRepelling,
                                 bool hasBlocking,
                                 bool hasKilling );
        InsecticideWaningEffect( const WaningConfig& rLarvalKillingConfig,
                                 const WaningConfig& rRepellingConfig,
                                 const WaningConfig& rBlockingConfig,
                                 const WaningConfig& rKillingConfig );
        InsecticideWaningEffect( const InsecticideWaningEffect& rMaster );
        virtual ~InsecticideWaningEffect();

        virtual bool Configure( const Configuration * config ) override;

        virtual std::string GetTypeName() const override;

        // IInsecticideWaningEffect
        virtual IInsecticideWaningEffect* Clone() override;
        virtual void SetName( const InsecticideName& rName ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual void Update( float dt ) override;
        virtual GeneticProbability GetCurrent( ResistanceType::Enum rt ) const override;
        virtual bool Has( ResistanceType::Enum type ) const override;

    private:
        InsecticideName m_Name;
        std::vector<bool> m_Has;
        std::vector<IWaningEffect*> m_Effects;

        DECLARE_SERIALIZABLE(InsecticideWaningEffect);
    };

    class InsecticideWaningEffectCollection : public IInsecticideWaningEffect,
                                              public JsonConfigurableCollection<InsecticideWaningEffect>
                                              
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        InsecticideWaningEffectCollection();
        InsecticideWaningEffectCollection( bool hasLarvalKilling,
                                           bool hasRepelling,
                                           bool hasBlocking,
                                           bool hasKilling );
        InsecticideWaningEffectCollection( const InsecticideWaningEffectCollection& rMaster );
        virtual ~InsecticideWaningEffectCollection();

        virtual void CheckConfiguration() override;

        // IInsecticideWaningEffect
        virtual IInsecticideWaningEffect* Clone() override;
        virtual void SetName( const InsecticideName& rName ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual void Update( float dt ) override;
        virtual GeneticProbability GetCurrent( ResistanceType::Enum rt ) const override;
        virtual bool Has( ResistanceType::Enum type ) const override;

    protected:
        virtual InsecticideWaningEffect* CreateObject() override;

        std::vector<bool> m_Has;
        std::vector<GeneticProbability> m_Current;

        DECLARE_SERIALIZABLE(InsecticideWaningEffectCollection);
    };

}
