
#include "stdafx.h"

#include "InsecticideWaningEffect.h"
#include "IWaningEffect.h"

SETUP_LOGGING( "InsecticideWaningEffect" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- InsecticideWaningEffect
    // ------------------------------------------------------------------------

    IMPL_QUERY_INTERFACE2(InsecticideWaningEffect, IInsecticideWaningEffect, IConfigurable)

    InsecticideWaningEffect::InsecticideWaningEffect()
        : InsecticideWaningEffect( false, false, false, false )
    {
    }

    InsecticideWaningEffect::InsecticideWaningEffect( bool hasLarvalKilling,
                                                      bool hasRepelling,
                                                      bool hasBlocking,
                                                      bool hasKilling )
        : JsonConfigurable()
        , m_Name()
        , m_Has()
        , m_Effects()
    {
        for( int i = 0; i < ResistanceType::pairs::count(); ++i )
        {
            m_Has.push_back( false );
            m_Effects.push_back( nullptr );
        }
        m_Has[ ResistanceType::LARVAL_KILLING ] = hasLarvalKilling;
        m_Has[ ResistanceType::REPELLING      ] = hasRepelling;
        m_Has[ ResistanceType::BLOCKING       ] = hasBlocking;
        m_Has[ ResistanceType::KILLING        ] = hasKilling;

        release_assert( m_Has.size() == ResistanceType::pairs::count() );
        release_assert( m_Has.size() == m_Effects.size() );
    }

    InsecticideWaningEffect::InsecticideWaningEffect( const WaningConfig& rLarvalKillingConfig,
                                                      const WaningConfig& rRepellingConfig,
                                                      const WaningConfig& rBlockingConfig,
                                                      const WaningConfig& rKillingConfig )
        : InsecticideWaningEffect(false,false,false,false)
    {
        if( rLarvalKillingConfig._json.Type() != json::NULL_ELEMENT )
        {
            m_Has[ ResistanceType::LARVAL_KILLING ] = true;
            m_Effects[ ResistanceType::LARVAL_KILLING ] = WaningEffectFactory::getInstance()->CreateInstance( rLarvalKillingConfig._json,
                                                                                                              "campaign",
                                                                                                              "Larval_Killing_Config" );
        }
        if( rRepellingConfig._json.Type() != json::NULL_ELEMENT )
        {
            m_Has[ ResistanceType::REPELLING ] = true;
            m_Effects[ ResistanceType::REPELLING ] = WaningEffectFactory::getInstance()->CreateInstance( rRepellingConfig._json,
                                                                                                         "campaign",
                                                                                                         "Repelling_Config" );
        }
        if( rBlockingConfig._json.Type() != json::NULL_ELEMENT )
        {
            m_Has[ ResistanceType::BLOCKING ] = true;
            m_Effects[ ResistanceType::BLOCKING ] = WaningEffectFactory::getInstance()->CreateInstance( rBlockingConfig._json,
                                                                                                        "campaign",
                                                                                                        "Blocking_Config" );
        }
        if( rKillingConfig._json.Type() != json::NULL_ELEMENT )
        {
            m_Has[ ResistanceType::KILLING ] = true;
            m_Effects[ ResistanceType::KILLING ]  = WaningEffectFactory::getInstance()->CreateInstance( rKillingConfig._json,
                                                                                                        "campaign",
                                                                                                        "Killing_Config"  );
        }
    }

    InsecticideWaningEffect::InsecticideWaningEffect( const InsecticideWaningEffect& rMaster )
        : JsonConfigurable( rMaster )
        , m_Name( rMaster.m_Name )
        , m_Has( rMaster.m_Has )
        , m_Effects()
    {
        for( auto p_effect : rMaster.m_Effects )
        {
            IWaningEffect* p_clone = nullptr;
            if( p_effect != nullptr )
            {
                p_clone = p_effect->Clone();
            }
            m_Effects.push_back( p_clone );
        }
        release_assert( m_Has.size() == ResistanceType::pairs::count() );
        release_assert( m_Has.size() == m_Effects.size() );
    }

    InsecticideWaningEffect::~InsecticideWaningEffect()
    {
        for( auto p_effect : m_Effects )
        {
            delete p_effect;
        }
    }

    std::string InsecticideWaningEffect::GetTypeName() const
    {
        std::stringstream ss;
        ss << "InsecticideWaningEffect_";
        ss << (m_Has[ ResistanceType::LARVAL_KILLING ] ? "L" : "");
        ss << (m_Has[ ResistanceType::REPELLING ] ? "R" : "");
        ss << (m_Has[ ResistanceType::BLOCKING ] ? "B" : "");
        ss << (m_Has[ ResistanceType::KILLING ] ? "K" : "");
        return ss.str();
    }

    bool InsecticideWaningEffect::Configure( const Configuration * inputJson )
    {
        // make sure that it setup to have at least one WaningConfig
        bool at_least_one_has = false;
        for( auto has : m_Has )
        {
            at_least_one_has |= has;
        }
        release_assert( at_least_one_has );

        WaningConfig larval_config;
        WaningConfig repelling_config;
        WaningConfig blocking_config;
        WaningConfig killing_config;

        initConfigTypeMap( "Insecticide_Name", &m_Name, INT_Insecticide_Name_DESC_TEXT );
        if( m_Has[ ResistanceType::LARVAL_KILLING ] )
        {
            initConfigComplexType("Larval_Killing_Config", &larval_config, IWE_Larval_Killing_Config_DESC_TEXT );
        }
        if( m_Has[ ResistanceType::REPELLING ] )
        {
            initConfigComplexType("Repelling_Config", &repelling_config, IWE_Repelling_Config_DESC_TEXT );
        }
        if( m_Has[ ResistanceType::BLOCKING ] )
        {
            initConfigComplexType("Blocking_Config", &blocking_config, IWE_Blocking_Config_DESC_TEXT );
        }
        if( m_Has[ ResistanceType::KILLING ] )
        {
            initConfigComplexType("Killing_Config", &killing_config,  IWE_Killing_Config_DESC_TEXT );
        }

        bool configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && configured )
        {
            if( m_Name.empty() || (m_Name == JsonConfigurable::default_string) )
            {
                std::stringstream ss;
                ss << "'Insecticide_Name' must be defined and cannot be empty string in 'Insecticides'";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( m_Has[ ResistanceType::LARVAL_KILLING ] )
            {
                m_Effects[ ResistanceType::LARVAL_KILLING ] = WaningEffectFactory::getInstance()->CreateInstance( larval_config._json,
                                                                                                                  inputJson->GetDataLocation(),
                                                                                                                  "Larval_Killing_Config" );
            }
            if( m_Has[ ResistanceType::REPELLING ] )
            {
                m_Effects[ ResistanceType::REPELLING ] = WaningEffectFactory::getInstance()->CreateInstance( repelling_config._json,
                                                                                                             inputJson->GetDataLocation(),
                                                                                                             "Repelling_Config" );
            }
            if( m_Has[ ResistanceType::BLOCKING ] )
            {
                m_Effects[ ResistanceType::BLOCKING ] = WaningEffectFactory::getInstance()->CreateInstance( blocking_config._json,
                                                                                                            inputJson->GetDataLocation(),
                                                                                                            "Blocking_Config" );
            }
            if( m_Has[ ResistanceType::KILLING ] )
            {
                m_Effects[ ResistanceType::KILLING ]  = WaningEffectFactory::getInstance()->CreateInstance( killing_config._json,
                                                                                                            inputJson->GetDataLocation(),
                                                                                                            "Killing_Config"  );
            }
        }

        return configured;
    }

    IInsecticideWaningEffect* InsecticideWaningEffect::Clone()
    {
        return new InsecticideWaningEffect( *this );
    }

    void InsecticideWaningEffect::SetName( const InsecticideName& rName )
    {
        m_Name = rName;
    }

    void InsecticideWaningEffect::SetContextTo( IIndividualHumanContext *context )
    {
        for( auto p_effect : m_Effects )
        {
            if( p_effect != nullptr )
            {
                p_effect->SetContextTo( context );
            }
        }
    }

    void InsecticideWaningEffect::Update( float dt )
    {
        for( auto p_effect : m_Effects )
        {
            if( p_effect != nullptr )
            {
                p_effect->Update( dt );
            }
        }
    }

    GeneticProbability InsecticideWaningEffect::GetCurrent( ResistanceType::Enum rt ) const
    {
        IWaningEffect* p_effect = m_Effects[ rt ];
        release_assert( m_Has[ rt ] );
        release_assert( p_effect != nullptr );

        GeneticProbability current = p_effect->Current();

        const Insecticide* p_insecticide = m_Name.GetInsecticide();
        if( p_insecticide != nullptr )
        {
            current *= p_insecticide->GetResistance( rt );
        }
        return current;
    }

    bool InsecticideWaningEffect::Has( ResistanceType::Enum rt ) const
    {
        return m_Has[ rt ];
    }

    REGISTER_SERIALIZABLE(InsecticideWaningEffect);

    void InsecticideWaningEffect::serialize(IArchive& ar, InsecticideWaningEffect* obj)
    {
        InsecticideWaningEffect& iwe = *obj;

        std::string tmp_name = iwe.m_Name;

        ar.labelElement("m_Name") & tmp_name;
        ar.labelElement("m_Has") & iwe.m_Has;
        ar.labelElement("m_Effects") & iwe.m_Effects;

        (std::string&)(iwe.m_Name) = tmp_name;
    }

    // ------------------------------------------------------------------------
    // --- InsecticideWaningEffectCollection
    // ------------------------------------------------------------------------

    IMPL_QUERY_INTERFACE2( InsecticideWaningEffectCollection, IInsecticideWaningEffect, IConfigurable )

        InsecticideWaningEffectCollection::InsecticideWaningEffectCollection()
        : InsecticideWaningEffectCollection( false, false, false, false )
    {
    }

    InsecticideWaningEffectCollection::InsecticideWaningEffectCollection( bool hasLarvalKilling,
                                                                          bool hasRepelling,
                                                                          bool hasBlocking,
                                                                          bool hasKilling )
        : JsonConfigurableCollection( "Insecticides" )
        , m_Has()
        , m_Current()
    {
        for( int i = 0; i < ResistanceType::pairs::count(); ++i )
        {
            m_Current.push_back( GeneticProbability( 1.0 ) );
            m_Has.push_back( false );
        }
        m_Has[ ResistanceType::LARVAL_KILLING ] = hasLarvalKilling;
        m_Has[ ResistanceType::REPELLING      ] = hasRepelling;
        m_Has[ ResistanceType::BLOCKING       ] = hasBlocking;
        m_Has[ ResistanceType::KILLING        ] = hasKilling;
    }

    InsecticideWaningEffectCollection::InsecticideWaningEffectCollection( const InsecticideWaningEffectCollection& rMaster )
        : JsonConfigurableCollection( rMaster )
        , m_Has( rMaster.m_Has )
        , m_Current( rMaster.m_Current )
    {
        for( auto p_i : rMaster.m_Collection )
        {
            InsecticideWaningEffect* p_new_i = new InsecticideWaningEffect( *p_i );
            this->m_Collection.push_back( p_new_i );
        }
    }

    InsecticideWaningEffectCollection::~InsecticideWaningEffectCollection()
    {
    }

    void InsecticideWaningEffectCollection::CheckConfiguration()
    {
        if( m_Collection.size() == 0 )
        {
            std::stringstream ss;
            ss << "Invalid empty 'Insecticides' array.\n";
            ss << "You must specify at least one insecticide and its corresponding effects.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    InsecticideWaningEffect* InsecticideWaningEffectCollection::CreateObject()
    {
        return new InsecticideWaningEffect( m_Has[ ResistanceType::LARVAL_KILLING ],
                                            m_Has[ ResistanceType::REPELLING ],
                                            m_Has[ ResistanceType::BLOCKING ],
                                            m_Has[ ResistanceType::KILLING ] );
    }

    IInsecticideWaningEffect* InsecticideWaningEffectCollection::Clone()
    {
        return new InsecticideWaningEffectCollection( *this );
    }

    void InsecticideWaningEffectCollection::SetName( const InsecticideName& rName )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "The name should not be being set on a collection." );
    }

    void InsecticideWaningEffectCollection::SetContextTo( IIndividualHumanContext *context )
    {
        for( auto p_iwe : m_Collection )
        {
            p_iwe->SetContextTo( context );
        }
    }

    void InsecticideWaningEffectCollection::Update( float dt )
    {
        release_assert( m_Has.size() == ResistanceType::pairs::count() );
        release_assert( m_Has.size() == m_Current.size() );

        // ----------------------------------------------------------------------------
        // --- Each type of effect can have multiple effects for each insecticide.
        // --- Hence, we need to properly combine the different effects for the type
        // --- correctly.  That is,
        // ---    total_current = 1 - (1 - current[i])*(1 - current[j])...
        // ----------------------------------------------------------------------------

        std::fill( m_Current.begin(), m_Current.end(), GeneticProbability( 1.0 ) );

        for( auto p_iwe : m_Collection )
        {
            p_iwe->Update( dt );

            for( int i = 0 ; i < m_Current.size(); ++i )
            {
                if( m_Has[ i ] )
                {
                    m_Current[ i ] *= (1.0 - p_iwe->GetCurrent( ResistanceType::Enum( i ) ));
                }
            }
        }

        for( int i = 0 ; i < m_Current.size(); ++i )
        {
            m_Current[ i ] = 1.0 - m_Current[ i ];
        }
    }

    GeneticProbability InsecticideWaningEffectCollection::GetCurrent( ResistanceType::Enum rt ) const
    {
        release_assert( m_Has[ rt ] );
        return m_Current[ rt ];
    }

    bool InsecticideWaningEffectCollection::Has( ResistanceType::Enum rt ) const
    {
        return m_Has[ rt ];
    }

    REGISTER_SERIALIZABLE(InsecticideWaningEffectCollection);

    void InsecticideWaningEffectCollection::serialize(IArchive& ar, InsecticideWaningEffectCollection* obj)
    {
        InsecticideWaningEffectCollection& iwe = *obj;
        ar.labelElement("m_Collection") & iwe.m_Collection;
        ar.labelElement("m_Has") & iwe.m_Has;
        ar.labelElement("m_Current") & iwe.m_Current;
    }
}
