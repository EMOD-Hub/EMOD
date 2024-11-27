
#include "stdafx.h"
#include "VectorControlNodeTargeted.h"

#include "Exceptions.h"
#include "InterventionFactory.h"
#include "NodeVectorEventContext.h" // for INodeVectorInterventionEffectsApply methods
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "JsonConfigurableCollection.h"
#include "DistributionFactory.h"
#include "Insecticides.h"

SETUP_LOGGING( "VectorControlNodeTargeted" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleVectorControlNode)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleVectorControlNode)

    //IMPLEMENT_FACTORY_REGISTERED(SimpleVectorControlNode) // don't register unusable base class
    IMPLEMENT_FACTORY_REGISTERED(Larvicides)
    IMPLEMENT_FACTORY_REGISTERED(SpaceSpraying)
    IMPLEMENT_FACTORY_REGISTERED(MultiInsecticideSpaceSpraying)
    IMPLEMENT_FACTORY_REGISTERED(IndoorSpaceSpraying)
    IMPLEMENT_FACTORY_REGISTERED(MultiInsecticideIndoorSpaceSpraying)
    IMPLEMENT_FACTORY_REGISTERED(SpatialRepellent)
    IMPLEMENT_FACTORY_REGISTERED(ArtificialDiet)
    IMPLEMENT_FACTORY_REGISTERED(SugarTrap)
    IMPLEMENT_FACTORY_REGISTERED(OvipositionTrap)
    IMPLEMENT_FACTORY_REGISTERED(OutdoorRestKill)
    IMPLEMENT_FACTORY_REGISTERED(AnimalFeedKill)


    SimpleVectorControlNode::SimpleVectorControlNode()
        : BaseNodeIntervention()
        , m_HabitatTarget(VectorHabitatType::ALL_HABITATS)
        , m_pInsecticideWaningEffect( nullptr )
        , m_LarvalKillingConfig()
        , m_RepellingConfig()
        , m_KillingConfig()
        , m_pINVIC(nullptr)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
    }

    SimpleVectorControlNode::SimpleVectorControlNode( const SimpleVectorControlNode& master )
        : BaseNodeIntervention( master )
        , m_HabitatTarget( master.m_HabitatTarget )
        , m_pInsecticideWaningEffect( nullptr )
        , m_LarvalKillingConfig() //shouldn't need to copy
        , m_RepellingConfig()     //shouldn't need to copy
        , m_KillingConfig()       //shouldn't need to copy
        , m_pINVIC( nullptr )
    {
        if( master.m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect = master.m_pInsecticideWaningEffect->Clone();
        }
    }

    SimpleVectorControlNode::~SimpleVectorControlNode()
    {
        delete m_pInsecticideWaningEffect;
    }

    bool SimpleVectorControlNode::Configure( const Configuration * inputJson )
    {
        // TODO: consider to what extent we want to pull the decay constants out of here as well
        //       in particular, for spatial repellents where there is reduction but not killing,
        //       the primary constant is un-used in BOX and DECAY (but not BOXDECAY)
        //       whereas, oviposition traps only have a killing effect.  (ERAD-599)
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10.0);

        bool configured = ConfigureKilling( inputJson );

        return configured;
    }

    void SimpleVectorControlNode::initConfigKilling()
    {
        initConfigComplexType( "Killing_Config", &m_KillingConfig,  VCN_Killing_Config_DESC_TEXT   );
    }

    void SimpleVectorControlNode::initConfigRepelling()
    {
    }

    bool SimpleVectorControlNode::ConfigureKilling( const Configuration* config )
    {
        InsecticideName name;

        initConfigRepelling();
        initConfigKilling();
        initConfigTypeMap( "Insecticide_Name", &name, INT_Insecticide_Name_DESC_TEXT );

        bool configured = BaseNodeIntervention::Configure( config );

        if( configured && !JsonConfigurable::_dryrun )
        {
            WaningConfig empty_config;
            m_pInsecticideWaningEffect = new InsecticideWaningEffect( m_LarvalKillingConfig, m_RepellingConfig, empty_config, m_KillingConfig );

            // try to get rid of memory no longer needed
            m_LarvalKillingConfig = empty_config;
            m_RepellingConfig     = empty_config;
            m_KillingConfig       = empty_config;

            name.CheckConfiguration( GetName().ToString(), "Insecticide_Name");
            m_pInsecticideWaningEffect->SetName( name );
        }
        return configured;
    }

    void SimpleVectorControlNode::SetContextTo( INodeEventContext *context )
    {
        BaseNodeIntervention::SetContextTo( context );

        // NOTE: Can't use WaningEffects that need SetConextTo() - i.e. aging.  Should be able to use calendar ones.
        //m_pInsecticideWaningEffect->SetContextTo( context );

        if (s_OK != context->QueryInterface(GET_IID(INodeVectorInterventionEffectsApply), (void**)&m_pINVIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "INodeVectorInterventionEffectsApply", "INodeEventContext" );
        }
    }

    bool SimpleVectorControlNode::Distribute( INodeEventContext *pNodeContext, IEventCoordinator2 *pEC )
    {
        // Just one of each of these allowed
        pNodeContext->PurgeExisting( typeid(*this).name() ); // hmm?  let's come back to this and query the right interfaces everywhere.
        return BaseNodeIntervention::Distribute( pNodeContext, pEC );
    }
    
    GeneticProbability SimpleVectorControlNode::GetKilling( ResistanceType::Enum rt ) const
    {
        return m_pInsecticideWaningEffect->GetCurrent( rt );
    }

    VectorHabitatType::Enum SimpleVectorControlNode::GetHabitatTarget() const
    {
        return m_HabitatTarget;
    }

    void SimpleVectorControlNode::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        if( m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect->Update(dt);
        }
        
        ApplyEffects( dt );
    }

    void SimpleVectorControlNode::CheckHabitatTarget( VectorHabitatType::Enum habitatType,
                                                      const char* pParameterName )
    {
        if( habitatType == VectorHabitatType::NONE )
        {
            std::stringstream ss;
            ss << "Invalid parameter value: '" << pParameterName << "' = 'NONE'\n";
            ss << "Please select one of the following types:\n";
            // start at 1 to skip NONE
            for( int i = 1; i < VectorHabitatType::pairs::count(); ++i )
            {
                ss << VectorHabitatType::pairs::get_keys()[ i ] << "\n";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        else if( habitatType != VectorHabitatType::ALL_HABITATS )
        {
            VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;

            std::stringstream ss_config_habitats;
            bool found = false;
            for( int i = 0; !found && (i < p_vp->vector_species.Size()); ++i )
            {
                VectorSpeciesParameters* p_vsp = p_vp->vector_species[ i ];
                const std::vector<IVectorHabitat*>& r_habitats = p_vsp->habitat_params.GetHabitats();
                for( int j = 0; !found && (j < r_habitats.size()); ++j )
                {
                    const char* p_habitat_name = VectorHabitatType::pairs::lookup_key( r_habitats[ j ]->GetVectorHabitatType() );
                    ss_config_habitats << p_vsp->name << " : " << p_habitat_name << "\n";
                    found |= (r_habitats[ j ]->GetVectorHabitatType() == habitatType);
                }
            }
            if( !found )
            {
                const char* p_habitat_name = VectorHabitatType::pairs::lookup_key( habitatType );

                std::stringstream ss;
                ss << "Invalid parameter value: '" << pParameterName << "' = '" << p_habitat_name << "'\n";
                ss << "This habitat type is not configured as a type in 'Vector_Species_Params.Habitats'.\n";
                ss << "Please select from one of the configured types:\n";
                ss << ss_config_habitats.str();
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    ReportInterventionData SimpleVectorControlNode::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseNodeIntervention::GetReportInterventionData();

        if( m_pInsecticideWaningEffect->Has( ResistanceType::REPELLING ) )
        {
            data.efficacy_repelling = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::REPELLING ).GetSum();
        }
        if( m_pInsecticideWaningEffect->Has( ResistanceType::KILLING ) )
        {
            data.efficacy_killing = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING ).GetSum();
        }

        return data;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- Larvicides ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    Larvicides::Larvicides()
        : SimpleVectorControlNode()
        , m_Coverage(1.0f)
    {
    }

    Larvicides::Larvicides( const Larvicides& rMaster )
        : SimpleVectorControlNode( rMaster )
        , m_Coverage( rMaster.m_Coverage )
    {
    }

    Larvicides::~Larvicides()
    {
    }

    bool Larvicides::Configure( const Configuration * inputJson )
    {
        initConfig( "Habitat_Target", m_HabitatTarget, inputJson, MetadataDescriptor::Enum("Habitat_Target", LV_Habitat_Target_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)) );
        initConfigComplexType("Larval_Killing_Config", &m_LarvalKillingConfig, LV_Larval_Killing_Config_DESC_TEXT );
        initConfigTypeMap("Spray_Coverage", &m_Coverage, Spray_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f);

        bool configured = SimpleVectorControlNode::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            CheckHabitatTarget( m_HabitatTarget, "Habitat_Target" );
        }
        return configured;
    }

    void Larvicides::initConfigKilling()
    {
    }

    void Larvicides::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );

        GeneticProbability larval_killing = GetKilling( ResistanceType::LARVAL_KILLING ) * m_Coverage;
        m_pINVIC->UpdateLarvalKilling( GetHabitatTarget(), larval_killing );
    }

    ReportInterventionData Larvicides::GetReportInterventionData() const
    {
        // only has larval killing so don't call SimpleVectorControlNode::GetReportInterventionData()
        ReportInterventionData data = BaseNodeIntervention::GetReportInterventionData();

        data.efficacy_killing = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::LARVAL_KILLING ).GetSum() * m_Coverage;

        return data;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- SpaceSpraying ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    SpaceSpraying::SpaceSpraying()
        : SimpleVectorControlNode()
        , m_Coverage(1.0f)
    {
    }

    SpaceSpraying::SpaceSpraying( const SpaceSpraying& rMaster )
        : SimpleVectorControlNode( rMaster )
        , m_Coverage( rMaster.m_Coverage )
    {
    }

    SpaceSpraying::~SpaceSpraying()
    {
    }

    bool SpaceSpraying::Configure( const Configuration * inputJson )
    {
        if( inputJson->Exist( "Spray_Kill_Target" ) )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                             "'Spray_Kill_Target' is no longer supported.  Use 'Insecticide.Resistances'." );
        }
        initConfigTypeMap("Spray_Coverage", &m_Coverage, Spray_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f);

        bool configured = SimpleVectorControlNode::Configure( inputJson );

        return configured;
    }

    void SpaceSpraying::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );

        GeneticProbability killing = GetKilling( ResistanceType::KILLING ) * m_Coverage;

        m_pINVIC->UpdateOutdoorKilling( killing );
    }

    ReportInterventionData SpaceSpraying::GetReportInterventionData() const
    {
        // only has killing so don't call SimpleVectorControlNode::GetReportInterventionData()
        ReportInterventionData data = BaseNodeIntervention::GetReportInterventionData();

        data.efficacy_killing = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING ).GetSum() * m_Coverage;

        return data;
    }

    // ---------------------------------------------------------------------------------------------------------
    //------------------------------ MultiInsecticideSpaceSpraying ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    MultiInsecticideSpaceSpraying::MultiInsecticideSpaceSpraying()
        : SpaceSpraying()
    {
    }

    MultiInsecticideSpaceSpraying::MultiInsecticideSpaceSpraying( const MultiInsecticideSpaceSpraying& rMaster )
        : SpaceSpraying( rMaster )
    {
    }

    MultiInsecticideSpaceSpraying::~MultiInsecticideSpaceSpraying()
    {
    }

    bool MultiInsecticideSpaceSpraying::ConfigureKilling( const Configuration* config )
    {
        InsecticideWaningEffectCollection* p_iwec = new InsecticideWaningEffectCollection(false,false,false,true);

        initConfigComplexCollectionType( "Insecticides", p_iwec, MISS_Insecticides_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( config );
        if( !JsonConfigurable::_dryrun && configured )
        {
            p_iwec->CheckConfiguration();
            m_pInsecticideWaningEffect = p_iwec;
        }
        return configured;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- IndoorSpaceSpraying ----------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    IndoorSpaceSpraying::IndoorSpaceSpraying()
        : SimpleVectorControlNode()
        , m_Coverage(1.0f)
    {
    }

    IndoorSpaceSpraying::IndoorSpaceSpraying( const IndoorSpaceSpraying& rMaster )
        : SimpleVectorControlNode( rMaster )
        , m_Coverage( rMaster.m_Coverage )
    {
    }

    IndoorSpaceSpraying::~IndoorSpaceSpraying()
    {
    }

    bool IndoorSpaceSpraying::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Spray_Coverage", &m_Coverage, Spray_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f);

        bool configured = SimpleVectorControlNode::Configure( inputJson );
        return configured;
    }

    void IndoorSpaceSpraying::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );

        GeneticProbability killing = GetKilling() * m_Coverage;
        m_pINVIC->UpdateIndoorKilling( killing );
    }

    ReportInterventionData IndoorSpaceSpraying::GetReportInterventionData() const
    {
        // only has killing so don't call SimpleVectorControlNode::GetReportInterventionData()
        ReportInterventionData data = BaseNodeIntervention::GetReportInterventionData();

        data.efficacy_killing = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING ).GetSum() * m_Coverage;

        return data;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- MultiInsecticideIndoorSpaceSpraying ------------------------
    // ---------------------------------------------------------------------------------------------------------

    MultiInsecticideIndoorSpaceSpraying::MultiInsecticideIndoorSpaceSpraying()
        : IndoorSpaceSpraying()
    {
    }

    MultiInsecticideIndoorSpaceSpraying::MultiInsecticideIndoorSpaceSpraying( const MultiInsecticideIndoorSpaceSpraying& rMaster )
        : IndoorSpaceSpraying( rMaster )
    {
    }

    MultiInsecticideIndoorSpaceSpraying::~MultiInsecticideIndoorSpaceSpraying()
    {
    }

    bool MultiInsecticideIndoorSpaceSpraying::ConfigureKilling( const Configuration* config )
    {
        InsecticideWaningEffectCollection* p_iwec = new InsecticideWaningEffectCollection(false,false,false,true);

        initConfigComplexCollectionType( "Insecticides", p_iwec, MIISS_Insecticides_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( config );
        if( !JsonConfigurable::_dryrun && configured )
        {
            p_iwec->CheckConfiguration();
            m_pInsecticideWaningEffect = p_iwec;
        }
        return configured;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- SpatialRepellent ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    SpatialRepellent::SpatialRepellent()
        : SimpleVectorControlNode()
        , m_Coverage(1.0)
    {
    }

    SpatialRepellent::SpatialRepellent( const SpatialRepellent& rMaster )
        : SimpleVectorControlNode( rMaster )
        , m_Coverage( rMaster.m_Coverage )
    {
    }

    SpatialRepellent::~SpatialRepellent()
    {
    }

    void SpatialRepellent::initConfigRepelling()
    {
        initConfigTypeMap("Spray_Coverage", &m_Coverage, Spray_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f);
        initConfigComplexType("Repelling_Config", &m_RepellingConfig, Repelling_Config_DESC_TEXT );
    }

    void SpatialRepellent::initConfigKilling()
    {
        // override so Killing_Config is not included
    }

    void SpatialRepellent::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );

        GeneticProbability repelling = GetKilling( ResistanceType::REPELLING ) * m_Coverage;
        m_pINVIC->UpdateVillageSpatialRepellent( repelling );
    }

    ReportInterventionData SpatialRepellent::GetReportInterventionData() const
    {
        // only has repelling so don't call SimpleVectorControlNode::GetReportInterventionData()
        ReportInterventionData data = BaseNodeIntervention::GetReportInterventionData();

        data.efficacy_repelling = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::REPELLING ).GetSum() * m_Coverage;

        return data;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- ArtificialDiet ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    ArtificialDiet::ArtificialDiet()
        : SimpleVectorControlNode()
        , m_AttractionTarget( ArtificialDietTarget::AD_OutsideVillage )
        , m_pAttractionEffect( nullptr )
    {
    }

    ArtificialDiet::ArtificialDiet( const ArtificialDiet& rMaster )
        : SimpleVectorControlNode( rMaster )
        , m_AttractionTarget( rMaster.m_AttractionTarget )
        , m_pAttractionEffect( nullptr )
    {
        if( rMaster.m_pAttractionEffect != nullptr )
        {
            this->m_pAttractionEffect = rMaster.m_pAttractionEffect->Clone();
        }
    }

    ArtificialDiet::~ArtificialDiet()
    {
        delete m_pAttractionEffect;
    }

    bool ArtificialDiet::Configure( const Configuration * inputJson )
    {
        WaningConfig attraction_config;

        initConfig( "Artificial_Diet_Target", m_AttractionTarget, inputJson, MetadataDescriptor::Enum("Artificial_Diet_Target", AD_Target_DESC_TEXT, MDD_ENUM_ARGS(ArtificialDietTarget)) );
        initConfigComplexType("Attraction_Config", &attraction_config, AD_Attraction_Config_DESC_TEXT );

        bool configured = SimpleVectorControlNode::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            m_pAttractionEffect = WaningEffectFactory::getInstance()->CreateInstance( attraction_config._json,
                                                                                      inputJson->GetDataLocation(),
                                                                                      "Attraction_Config" );
        }
        return configured;
    }

    bool ArtificialDiet::ConfigureKilling( const Configuration* config )
    {
        // skip other stuff in base class
        return BaseNodeIntervention::Configure( config );
    }

    void ArtificialDiet::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );

        m_pAttractionEffect->Update(dt);

        switch( m_AttractionTarget )
        {
            case ArtificialDietTarget::AD_WithinVillage:
                m_pINVIC->UpdateADIVAttraction( m_pAttractionEffect->Current() );
                break;

            case ArtificialDietTarget::AD_OutsideVillage:
                m_pINVIC->UpdateADOVAttraction( m_pAttractionEffect->Current() );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                            "GetAttractionTarget()",
                                                            m_AttractionTarget,
                                                            ArtificialDietTarget::pairs::lookup_key(m_AttractionTarget) );
        }
    }

    ReportInterventionData ArtificialDiet::GetReportInterventionData() const
    {
        // only has attracting so don't call SimpleVectorControlNode::GetReportInterventionData()
        ReportInterventionData data = BaseNodeIntervention::GetReportInterventionData();

        data.efficacy_attracting = m_pAttractionEffect->Current();

        return data;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- SugarTrap ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    SugarTrap::SugarTrap()
        : SimpleVectorControlNode()
        , m_pExpirationDistribution( nullptr )
        , m_ExpirationTimer( 0.0 )
        , m_TimerHasExpired( false )
    {
        m_ExpirationTimer.handle = std::bind( &SugarTrap::Callback, this, std::placeholders::_1 );
    }

    SugarTrap::SugarTrap( const SugarTrap& master )
        : SimpleVectorControlNode( master )
        , m_pExpirationDistribution( nullptr )
        , m_ExpirationTimer( master.m_ExpirationTimer )
        , m_TimerHasExpired( master.m_TimerHasExpired )
    {
        if( master.m_pExpirationDistribution != nullptr )
        {
            this->m_pExpirationDistribution = master.m_pExpirationDistribution->Clone();
        }
        m_ExpirationTimer.handle = std::bind( &SugarTrap::Callback, this, std::placeholders::_1 );
    }

    SugarTrap::~SugarTrap()
    {
        delete m_pExpirationDistribution;
    }

    bool SugarTrap::Configure( const Configuration * inputJson )
    {
        DistributionFunction::Enum expiration_distribution_type( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Expiration_Period_Distribution",
                    expiration_distribution_type,
                    inputJson,
                    MetadataDescriptor::Enum( "Expiration_Period_Distribution",
                                              Expiration_Period_Distribution_DESC_TEXT,
                                              MDD_ENUM_ARGS( DistributionFunction ) ) );
        m_pExpirationDistribution = DistributionFactory::CreateDistribution( this, expiration_distribution_type, "Expiration_Period", inputJson );

        bool configured = SimpleVectorControlNode::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            bool found = false;
            VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;
            for( int i = 0; !found && (i < p_vp->vector_species.Size()); ++i )
            {
                found |= (p_vp->vector_species[ i ]->vector_sugar_feeding != VectorSugarFeeding::VECTOR_SUGAR_FEEDING_NONE);
            }
            if( !found )
            {
                std::stringstream ss;
                ss << "Using 'SugarTrap' intervention but 'Vector_Sugar_Feeding_Frequency' set to 'VECTOR_SUGAR_FEEDING_NONE'\n";
                ss << "for all the species.  'Vector_Sugar_Feeding_Frequency' must be set to something besides\n";
                ss << "'VECTOR_SUGAR_FEEDING_NONE' for at least one specie when using 'SugarTrap'.\n";
                ss << "Options are:\n";
                for( int i = 0; i < VectorSugarFeeding::pairs::count(); ++i )
                {
                    ss << VectorSugarFeeding::pairs::get_keys()[ i ] << "\n";
                }
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return configured;
    }

    bool SugarTrap::Distribute( INodeEventContext *pNodeContext, IEventCoordinator2 *pEC )
    {
        bool distributed =  SimpleVectorControlNode::Distribute( pNodeContext, pEC );
        if( distributed )
        {
            release_assert( m_pExpirationDistribution != nullptr );
            m_ExpirationTimer = m_pExpirationDistribution->Calculate( parent->GetRng() );

            // ----------------------------------------------------------------------------
            // --- Assuming dt=1.0 and decrementing timer so that a timer of zero expires
            // --- when it is distributed but is not used.  A timer of one should be used
            // --- the day it is distributed but expire:
            // ---    distributed->used->expired on all same day
            // ----------------------------------------------------------------------------
            m_ExpirationTimer.Decrement( 1.0 );
        }
        return distributed;
    }

    void SugarTrap::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        if( m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect->Update(dt);
        }

        // Check expiratin in case it expired when it was distributed
        if( !m_TimerHasExpired )
        {
            ApplyEffects( dt );
        }

        m_ExpirationTimer.Decrement( dt );
        if( m_TimerHasExpired )
        {
            SetExpired( true );
        }
    }

    void SugarTrap::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );
        m_pINVIC->UpdateSugarFeedKilling( GetKilling() );
    }

    void SugarTrap::Callback( float dt )
    {
        m_TimerHasExpired = true;
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- OvipositionTrap ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    bool OvipositionTrap::Configure( const Configuration * inputJson )
    {
        if( !JsonConfigurable::_dryrun )
        {
            VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_sampling_type;
            if (vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_NUMBER || vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_PERCENT)
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Explicit oviposition only implemented in individual-mosquito model, not in cohort model." );
            }
        }

        WaningConfig killing_config;

        initConfig( "Habitat_Target", m_HabitatTarget, inputJson, MetadataDescriptor::Enum("Habitat_Target", OT_Habitat_Target_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)) );
        initConfigComplexType("Killing_Config",  &killing_config, OT_Killing_Config_DESC_TEXT  );

        bool configured = SimpleVectorControlNode::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            CheckHabitatTarget( m_HabitatTarget, "Habitat_Target" );

            WaningConfig empty_config;
            m_pInsecticideWaningEffect = new InsecticideWaningEffect( empty_config, empty_config, empty_config, killing_config );
        }
        return configured;
    }

    bool OvipositionTrap::ConfigureKilling( const Configuration* config )
    {
        // skip other stuff in base class
        return BaseNodeIntervention::Configure( config );
    }

    void OvipositionTrap::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );
        m_pINVIC->UpdateOviTrapKilling( GetHabitatTarget(), GetKilling().GetDefaultValue() );
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- OutdoorRestKill ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    void OutdoorRestKill::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );
        m_pINVIC->UpdateOutdoorRestKilling( GetKilling() );
    }

    // ---------------------------------------------------------------------------------------------------------
    //--------------------------------------------- AnimalFeedKill ---------------------------------------------
    // ---------------------------------------------------------------------------------------------------------

    void AnimalFeedKill::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );
        m_pINVIC->UpdateAnimalFeedKilling( GetKilling() );
    }
}
