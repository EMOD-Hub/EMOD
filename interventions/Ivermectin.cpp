
#include "stdafx.h"
#include "Ivermectin.h"

#include <typeinfo>

#include "InterventionEnums.h"
#include "VectorInterventionsContainerContexts.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "Ivermectin" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(Ivermectin)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(Ivermectin)

    IMPLEMENT_FACTORY_REGISTERED(Ivermectin)

    Ivermectin::Ivermectin()
    : BaseIntervention()
    , m_pInsecticideWaningEffect(nullptr)
    , m_pIVIES(nullptr)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    Ivermectin::Ivermectin( const Ivermectin& master )
    : BaseIntervention( master )
    , m_pInsecticideWaningEffect(nullptr)
    , m_pIVIES(nullptr)
    {
        if( master.m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect = master.m_pInsecticideWaningEffect->Clone();
        }
    }

    Ivermectin::~Ivermectin()
    {
        delete m_pInsecticideWaningEffect;
    }

    bool Ivermectin::Configure( const Configuration * inputJson )
    {
        WaningConfig killing_config;
        InsecticideName name;

        initConfigTypeMap(     "Insecticide_Name", &name,           INT_Insecticide_Name_DESC_TEXT );
        initConfigComplexType( "Killing_Config",   &killing_config, IVM_Killing_Config_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && configured )
        {
            WaningConfig empty_config;
            m_pInsecticideWaningEffect = new InsecticideWaningEffect( empty_config, empty_config, empty_config, killing_config );

            name.CheckConfiguration( GetName().ToString(), "Insecticide_Name");
            m_pInsecticideWaningEffect->SetName( name );
        }
        return configured;
    }

    bool Ivermectin::Distribute( IIndividualHumanInterventionsContext *context,
                                 ICampaignCostObserver * const pCCO )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            if (s_OK != context->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&m_pIVIES) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "context",
                                               "IVectorInterventionEffectsSetter",
                                               "IIndividualHumanInterventionsContext" );
            }
        }
        return distributed;
    }

    void Ivermectin::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );
        m_pInsecticideWaningEffect->SetContextTo( context );

        LOG_DEBUG("Ivermectin::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&m_pIVIES) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context",
                                           "IVectorInterventionEffectsSetter",
                                           "IIndividualHumanContext" );
        }
    }

    void Ivermectin::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        m_pInsecticideWaningEffect->Update(dt);

        GeneticProbability killing = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING );

        m_pIVIES->UpdateInsecticidalDrugKillingProbability( killing );

        // Discard if efficacy is sufficiently low
        if (killing.GetDefaultValue() < 1e-5)
        {
            expired = true;
        }
    }

    ReportInterventionData Ivermectin::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

        data.efficacy_killing = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::KILLING ).GetSum();

        return data;
    }

    REGISTER_SERIALIZABLE(Ivermectin);

    void Ivermectin::serialize(IArchive& ar, Ivermectin* obj)
    {
        BaseIntervention::serialize( ar, obj );
        Ivermectin& ivermectin = *obj;
        ar.labelElement("m_pInsecticideWaningEffect") & ivermectin.m_pInsecticideWaningEffect;
    }
}
