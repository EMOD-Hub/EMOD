
#include "stdafx.h"
#include "BitingRisk.h"
#include "VectorInterventionsContainerContexts.h"
#include "IIndividualHumanContext.h"
#include "Distributions.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "BitingRisk" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED( BitingRisk )

    BEGIN_QUERY_INTERFACE_BODY( BitingRisk )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( BitingRisk )

        BitingRisk::BitingRisk()
        : BaseIntervention()
        , m_IBitingRisk( nullptr )
        , m_Distribution( nullptr )
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
    }

    BitingRisk::BitingRisk( const BitingRisk& master )
        : BaseIntervention( master )
        , m_IBitingRisk( nullptr )
        , m_Distribution( master.m_Distribution->Clone() )
    {
    }

    BitingRisk::~BitingRisk()
    {
        delete m_Distribution;
    }

    bool BitingRisk::Configure( const Configuration * inputJson )
    {
        float param1_risk = 0.0, param2_risk = 0.0;
        DistributionFunction::Enum risk_distribution_function(DistributionFunction::CONSTANT_DISTRIBUTION);
        initConfig("Risk_Distribution", risk_distribution_function, inputJson, MetadataDescriptor::Enum("Risk_Distribution", BR_Risk_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)));
        m_Distribution = DistributionFactory::CreateDistribution( this, risk_distribution_function, "Risk", inputJson );
       
        return BaseIntervention::Configure( inputJson );
    }

    bool BitingRisk::Distribute( IIndividualHumanInterventionsContext *context,
                                 ICampaignCostObserver * const pCCO )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            if( s_OK != context->QueryInterface( GET_IID( IBitingRisk ), (void**)&m_IBitingRisk ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBitingRisk", "IIndividualHumanInterventionsContext" );
            }
        }
        return distributed;
    }

    void BitingRisk::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );

        if( s_OK != context->GetInterventionsContext()->QueryInterface( GET_IID( IBitingRisk ), (void**)&m_IBitingRisk ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IBitingRisk", "IIndividualHumanInterventionsContext" );
        }
    }

    void BitingRisk::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        float rate = m_Distribution->Calculate( parent->GetRng() );
        m_IBitingRisk->UpdateRelativeBitingRate( rate );
        expired = true;
    }

    REGISTER_SERIALIZABLE( BitingRisk );

    void BitingRisk::serialize( IArchive& ar, BitingRisk* obj )
    {
        BaseIntervention::serialize( ar, obj );
        BitingRisk& biting_risk = *obj;
        ar.labelElement( "m_Distribution" ) & biting_risk.m_Distribution;
    }
}
