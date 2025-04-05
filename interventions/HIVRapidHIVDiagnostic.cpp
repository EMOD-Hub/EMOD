
#include "stdafx.h"
#include "HIVRapidHIVDiagnostic.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "IIndividualHuman.h"
#include "IInfection.h"
#include "RANDOM.h"

SETUP_LOGGING( "HIVRapidHIVDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVRapidHIVDiagnostic, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVRapidHIVDiagnostic, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVRapidHIVDiagnostic)

    HIVRapidHIVDiagnostic::HIVRapidHIVDiagnostic()
        : HIVSimpleDiagnostic()
        , m_ProbReceivedResults(1.0)
        , m_SensitivityType( SensitivityType::SINGLE_VALUE )
        , m_SensitivityVersusTime( 0.0, FLT_MAX, 0.0, 1.0 )
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVRapidHIVDiagnostic::HIVRapidHIVDiagnostic( const HIVRapidHIVDiagnostic& master )
        : HIVSimpleDiagnostic( master )
        , m_ProbReceivedResults( master.m_ProbReceivedResults )
        , m_SensitivityType( master.m_SensitivityType )
        , m_SensitivityVersusTime( master.m_SensitivityVersusTime )
    {
    }

    bool HIVRapidHIVDiagnostic::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Probability_Received_Result", &m_ProbReceivedResults, HIV_RHD_Probability_Received_Result_DESC_TEXT, 0, 1.0, 1.0 );

        initConfig( "Sensitivity_Type", 
                    m_SensitivityType,
                    inputJson,
                    MetadataDescriptor::Enum( "Sensitivity_Type",
                                              HIV_RHD_Sensitivity_Type_DESC_TEXT, MDD_ENUM_ARGS( SensitivityType ) ) );

        if( m_SensitivityType ==SensitivityType::VERSUS_TIME || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Sensitivity_Versus_Time",
                               &m_SensitivityVersusTime,
                               HIV_RHD_Sensitivity_Versus_Time_DESC_TEXT,
                               "Sensitivity_Type",
                               "VERSUS_TIME" );
        }
        return HIVSimpleDiagnostic::Configure( inputJson );
    }

    bool HIVRapidHIVDiagnostic::positiveTestResult()
    {
        if( m_SensitivityType == SensitivityType::VERSUS_TIME )
        {
            const IIndividualHuman* p_individual = parent->GetEventContext()->GetIndividualHumanConst();

            base_sensitivity = 1.0;
            if( p_individual->IsInfected() )
            {
                const infection_list_t& r_infection_list = p_individual->GetInfections();
                release_assert( r_infection_list.size() == 1 );
                float infection_duration_days = r_infection_list.front()->GetDuration();
                base_sensitivity = m_SensitivityVersusTime.getValueLinearInterpolation( infection_duration_days, 1.0 );
            }
        }

        return HIVSimpleDiagnostic::positiveTestResult();
    }

    // runs on a positive test when in positive treatment fraction
    void HIVRapidHIVDiagnostic::positiveTestDistribute()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
        }

        // update the patient's medical chart with testing history
        hiv_parent->OnTestForHIV(true);

        onReceivedResult( hiv_parent, true );

        // distribute the intervention
        HIVSimpleDiagnostic::positiveTestDistribute();
    }

    // runs on a negative test when in negative treatment fraction
    void HIVRapidHIVDiagnostic::onNegativeTestResult()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
        }

        // update the patent's medical chart with testing history
        hiv_parent->OnTestForHIV(false);

        onReceivedResult( hiv_parent, false );

        // distribute the intervention
        HIVSimpleDiagnostic::onNegativeTestResult();
    }

    void HIVRapidHIVDiagnostic::onReceivedResult( IHIVMedicalHistory* pMedHistory, bool resultIsHivPositive )
    {
        if( parent->GetRng()->SmartDraw( m_ProbReceivedResults ) )
        {
            pMedHistory->OnReceivedTestResultForHIV( resultIsHivPositive );
        }

        IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();

        if( resultIsHivPositive )
        {
            broadcaster->TriggerObservers( parent->GetEventContext(), EventTrigger::HIVTestedPositive );
        }
        else
        {
            broadcaster->TriggerObservers( parent->GetEventContext(), EventTrigger::HIVTestedNegative );
        }
    }

    REGISTER_SERIALIZABLE(HIVRapidHIVDiagnostic);

    void HIVRapidHIVDiagnostic::serialize(IArchive& ar, HIVRapidHIVDiagnostic* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVRapidHIVDiagnostic& diag = *obj;

        ar.labelElement( "m_ProbReceivedResults"   ) & diag.m_ProbReceivedResults;
        ar.labelElement( "m_SensitivityType"       ) & (uint32_t&)diag.m_SensitivityType;
        ar.labelElement( "m_SensitivityVersusTime" ) & diag.m_SensitivityVersusTime;
    }
}

