
#include "stdafx.h"
#include "IndividualRepellent.h"

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "VectorInterventionsContainerContexts.h"  // for IIndividualRepellentConsumer methods

SETUP_LOGGING( "SimpleIndividualRepellent" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleIndividualRepellent)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleIndividualRepellent)

    IMPLEMENT_FACTORY_REGISTERED(SimpleIndividualRepellent)

    SimpleIndividualRepellent::SimpleIndividualRepellent()
    : BaseIntervention()
    , m_pInsecticideWaningEffect(nullptr)
    , m_pIRC(nullptr)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
    }

    SimpleIndividualRepellent::SimpleIndividualRepellent( const SimpleIndividualRepellent& master )
    : BaseIntervention( master )
    , m_pInsecticideWaningEffect( nullptr )
    , m_pIRC( nullptr )
    {
        if( master.m_pInsecticideWaningEffect != nullptr )
        {
            m_pInsecticideWaningEffect = master.m_pInsecticideWaningEffect->Clone();
        }
    }

    SimpleIndividualRepellent::~SimpleIndividualRepellent()
    {
        delete m_pInsecticideWaningEffect;
    }

    bool
    SimpleIndividualRepellent::Configure(
        const Configuration * inputJson
    )
    {
        InsecticideName name;
        WaningConfig   repelling_config;

        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
        initConfigComplexType("Repelling_Config", &repelling_config, Repelling_Config_DESC_TEXT );
        initConfigTypeMap( "Insecticide_Name", &name, INT_Insecticide_Name_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );

        if( !JsonConfigurable::_dryrun  && configured )
        {
            WaningConfig empty_config;
            m_pInsecticideWaningEffect = new InsecticideWaningEffect( empty_config, repelling_config, empty_config, empty_config );

            name.CheckConfiguration( GetName().ToString(), "Insecticide_Name");
            m_pInsecticideWaningEffect->SetName( name );
        }
        return configured;
    }

    bool
    SimpleIndividualRepellent::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IIndividualRepellentConsumer), (void**)&m_pIRC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "IIndividualRepellentConsumer", "IIndividualHumanInterventionsContext" );
        }
        return BaseIntervention::Distribute( context, pCCO );
    }

    void SimpleIndividualRepellent::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );
        m_pInsecticideWaningEffect->SetContextTo( context );

        LOG_DEBUG("SimpleIndividualRepellent::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IIndividualRepellentConsumer), (void**)&m_pIRC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "IIndividualRepellentConsumer", "IIndividualHumanContext" );
        }
    }

    void SimpleIndividualRepellent::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        m_pInsecticideWaningEffect->Update(dt);
        GeneticProbability current = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::REPELLING );

        release_assert( m_pIRC != nullptr );
        m_pIRC->UpdateProbabilityOfIndRep( current );
    }

    ReportInterventionData SimpleIndividualRepellent::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

        data.efficacy_repelling = m_pInsecticideWaningEffect->GetCurrent( ResistanceType::REPELLING ).GetSum();

        return data;
    }

    REGISTER_SERIALIZABLE(SimpleIndividualRepellent);

    void SimpleIndividualRepellent::serialize(IArchive& ar, SimpleIndividualRepellent* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleIndividualRepellent& repellent = *obj;
        ar.labelElement("m_pInsecticideWaningEffect") & repellent.m_pInsecticideWaningEffect;
    }
}
