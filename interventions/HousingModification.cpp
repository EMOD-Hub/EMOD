
#include "stdafx.h"
#include "HousingModification.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "VectorInterventionsContainerContexts.h"  // for IHousingModificationConsumer methods
#include "Log.h"

SETUP_LOGGING( "SimpleHousingModification" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleHousingModification)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleHousingModification)

    IMPLEMENT_FACTORY_REGISTERED(SimpleHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(IRSHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(ScreeningHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(SpatialRepellentHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(MultiInsecticideIRSHousingModification)

    REGISTER_SERIALIZABLE(IRSHousingModification);
    REGISTER_SERIALIZABLE(ScreeningHousingModification);
    REGISTER_SERIALIZABLE(SpatialRepellentHousingModification);
    REGISTER_SERIALIZABLE(MultiInsecticideIRSHousingModification);

    // ------------------------------------------------------------------------
    // --- SimpleHousingModification
    // ------------------------------------------------------------------------

    SimpleHousingModification::SimpleHousingModification()
    : BaseIntervention()
    , m_pInsecticideWaningEffect(nullptr)
    , m_pIHMC(nullptr)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    SimpleHousingModification::SimpleHousingModification( const SimpleHousingModification& master )
    : BaseIntervention( master )
    , m_pInsecticideWaningEffect( nullptr )
    , m_pIHMC( nullptr )
    {
        if( master.m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect = master.m_pInsecticideWaningEffect->Clone();
        }
    }

    SimpleHousingModification::~SimpleHousingModification()
    {
        delete m_pInsecticideWaningEffect;
    }

    bool SimpleHousingModification::Configure( const Configuration * inputJson )
    {
        WaningConfig repelling_config;
        WaningConfig killing_config;
        InsecticideName name;

        initConfigInsecticideName( &name );
        initConfigRepelling( &repelling_config );
        initConfigKilling( &killing_config );

        bool configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && configured )
        {
            WaningConfig empty_config;
            m_pInsecticideWaningEffect = new InsecticideWaningEffect( empty_config,
                                                                      repelling_config,
                                                                      empty_config,
                                                                      killing_config );

            SetInsecticideName( name );
        }
        return configured;
    }

    void SimpleHousingModification::initConfigInsecticideName( InsecticideName* pName )
    {
        initConfigTypeMap( "Insecticide_Name", pName, INT_Insecticide_Name_DESC_TEXT );
    }

    void SimpleHousingModification::SetInsecticideName( InsecticideName& rName )
    {
        rName.CheckConfiguration( GetName().ToString(), "Insecticide_Name");
        m_pInsecticideWaningEffect->SetName( rName );
    }

    void SimpleHousingModification::initConfigRepelling( WaningConfig* pRepellingConfig )
    {
        initConfigComplexType( "Repelling_Config", pRepellingConfig, Repelling_Config_DESC_TEXT );
    }

    void SimpleHousingModification::initConfigKilling( WaningConfig* pKillingConfig )
    {
        initConfigComplexType( "Killing_Config", pKillingConfig, HM_Killing_Config_DESC_TEXT );
    }

    bool
    SimpleHousingModification::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        if (s_OK != context->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&m_pIHMC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanInterventionsContext" );
        }

        context->PurgeExisting( typeid(*this).name() );

        return BaseIntervention::Distribute( context, pCCO );
    }

    void SimpleHousingModification::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );
        if( m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect->SetContextTo( context );
        }

        LOG_DEBUG("SimpleHousingModification::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&m_pIHMC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanContext" );
        }
    }

    void SimpleHousingModification::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        m_pInsecticideWaningEffect->Update( dt );

        ApplyEffectsRepelling( dt );
        ApplyEffectsKilling( dt );
    }

    ReportInterventionData SimpleHousingModification::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

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

    void SimpleHousingModification::ApplyEffectsRepelling( float dt )
    {
        GeneticProbability current_repellingrate = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::REPELLING );

        release_assert( m_pIHMC != nullptr );

        m_pIHMC->UpdateProbabilityOfHouseRepelling( current_repellingrate );
    }

    void SimpleHousingModification::ApplyEffectsKilling( float dt )
    {
        GeneticProbability current_killingrate = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING );

        release_assert( m_pIHMC != nullptr );

        m_pIHMC->UpdateProbabilityOfHouseKilling( current_killingrate );
    }

    REGISTER_SERIALIZABLE(SimpleHousingModification);

    void SimpleHousingModification::serialize(IArchive& ar, SimpleHousingModification* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleHousingModification& mod = *obj;
        ar.labelElement("m_pInsecticideWaningEffect") & mod.m_pInsecticideWaningEffect;
    }

    // ------------------------------------------------------------------------
    // --- IRSHousingModification
    // ------------------------------------------------------------------------

    void IRSHousingModification::serialize(IArchive& ar, IRSHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    // ------------------------------------------------------------------------
    // --- MultiInsecticideIRSHousingModification
    // ------------------------------------------------------------------------

    bool MultiInsecticideIRSHousingModification::Configure( const Configuration * inputJson )
    {
        InsecticideWaningEffectCollection* p_iwec = new InsecticideWaningEffectCollection(false,true,false,true);

        initConfigComplexCollectionType( "Insecticides", p_iwec, HM_MIRS_Insecticides_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && configured )
        {
            p_iwec->CheckConfiguration();
            m_pInsecticideWaningEffect = p_iwec;
        }
        return configured;
    }

    void MultiInsecticideIRSHousingModification::serialize(IArchive& ar, MultiInsecticideIRSHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    // ------------------------------------------------------------------------
    // --- ScreeningHousingModification
    // ------------------------------------------------------------------------

    void ScreeningHousingModification::serialize(IArchive& ar, ScreeningHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    // ------------------------------------------------------------------------
    // --- SpatialRepellentHousingModification
    // ------------------------------------------------------------------------

    void SpatialRepellentHousingModification::initConfigKilling( WaningConfig* pKillingConfig )
    {
        // do not include killing
    }

    void SpatialRepellentHousingModification::ApplyEffectsKilling( float dt )
    {
        // no killing
    }

    void SpatialRepellentHousingModification::serialize(IArchive& ar, SpatialRepellentHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }
}

