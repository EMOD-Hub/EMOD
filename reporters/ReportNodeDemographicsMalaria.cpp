
#include "stdafx.h"

#include "ReportNodeDemographicsMalaria.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "ReportUtilities.h"
#include "Properties.h"
#include "NodeProperties.h"
#include "StrainIdentity.h"
#include "MalariaContexts.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#endif

SETUP_LOGGING( "ReportNodeDemographicsMalaria" )

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportNodeDemographicsMalaria()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char*
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char *
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void
GetReportInstantiator( Kernel::instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif
#endif //_REPORT_DLL

    BEGIN_QUERY_INTERFACE_DERIVED( ReportNodeDemographicsMalaria, ReportNodeDemographics )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportNodeDemographicsMalaria, ReportNodeDemographics )
#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportNodeDemographicsMalaria )
#endif

    ReportNodeDemographicsMalaria::ReportNodeDemographicsMalaria()
        : ReportNodeDemographics( "ReportNodeDemographicsMalaria.csv" )
        , m_IsRegistered(false)
        , m_StratifyBySymptoms(false)
        , m_DetectionThresholds()
        , m_EventsOfInterest()
    {
        initSimTypes( 1, "MALARIA_SIM" );

        m_EventsOfInterest.push_back( EventTrigger::InfectionCleared );
    }

    ReportNodeDemographicsMalaria::ReportNodeDemographicsMalaria( const std::string& rReportName )
        : ReportNodeDemographics( rReportName )
        , m_IsRegistered(false)
        , m_StratifyBySymptoms( false )
        , m_DetectionThresholds()
        , m_EventsOfInterest()
    {
        initSimTypes( 1, "MALARIA_SIM" );

        m_EventsOfInterest.push_back( EventTrigger::InfectionCleared );
    }

    ReportNodeDemographicsMalaria::~ReportNodeDemographicsMalaria()
    {
    }

    bool ReportNodeDemographicsMalaria::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Stratify_By_Has_Clinical_Symptoms", &m_StratifyBySymptoms, RNDM_Stratify_By_Has_Clinical_Symptoms_DESC_TEXT, false );
        bool ret = ReportNodeDemographics::Configure( inputJson );


        return ret;
    }

    void ReportNodeDemographicsMalaria::Initialize( unsigned int nrmSize )
    {
        ReportNodeDemographics::Initialize( nrmSize );
    }

    void ReportNodeDemographicsMalaria::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
        m_DetectionThresholds = rDetectionThresholds;
    }

    std::string ReportNodeDemographicsMalaria::GetHeader() const
    {
        std::stringstream header ;

        header << ReportNodeDemographics::GetHeader();

        header << "," << "AvgInfectiousness";
        header << "," << "AvgParasiteDensity";
        header << "," << "AvgGametocyteDensity";
        header << "," << "AvgVariantFractionPfEMP1Major";
        header << "," << "AvgNumInfections";
        header << "," << "AvgInfectionClearedDuration";
        header << "," << "NumInfectionsCleared";
        header << "," << "NumHasFever";
        if( !m_StratifyBySymptoms )
        {
            header << "," << "NumHasClinicalSymptoms";
        }

        return header.str();
    }

    void ReportNodeDemographicsMalaria::LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData ) 
    {
        IMalariaHumanContext* p_human_malaria = nullptr;
        if( s_OK != individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_human_malaria ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IMalariaHumanContext", "IIndividualHuman" );
        }
        float fever = p_human_malaria->GetMalariaSusceptibilityContext()->get_fever();

        NodeDataMalaria* p_ndm = (NodeDataMalaria*)pNodeData;
        p_ndm->sum_pfemp1 += p_human_malaria->GetMalariaSusceptibilityContext()->get_fraction_of_variants_with_antibodies( MalariaAntibodyType::PfEMP1_major );

        if( individual->IsInfected() )
        {
            p_ndm->num_infections += individual->GetInfections().size();

            std::vector<std::pair<int,int>> all_strains = p_human_malaria->GetInfectingStrainIds();

            p_ndm->sum_infectiousness     += individual->GetInfectiousness();
            p_ndm->sum_parasite_density   += p_human_malaria->GetMalariaSusceptibilityContext()->get_parasite_density();
            p_ndm->sum_gametocyte_density += p_human_malaria->GetGametocyteDensity();
        }
        p_ndm->num_has_symptoms += p_human_malaria->HasClinicalSymptomContinuing( ClinicalSymptomsEnum::CLINICAL_DISEASE ) ? 1 : 0;
        p_ndm->num_has_fever += (fever > m_DetectionThresholds[ MalariaDiagnosticType::FEVER ]) ? 1 : 0;
    }

    NodeData* ReportNodeDemographicsMalaria::CreateNodeData()
    {
        return new NodeDataMalaria();
    }

    void ReportNodeDemographicsMalaria::WriteNodeData( const NodeData* pData )
    {
        ReportNodeDemographics::WriteNodeData( pData );

        NodeDataMalaria* p_ndm = (NodeDataMalaria*)pData;

        float avg_infectiousness = 0.0;
        float avg_parasite = 0.0;
        float avg_gametocyte = 0.0;
        float avg_pfemp1 = 0.0;
        if( p_ndm->num_people > 0 )
        {
            avg_infectiousness = p_ndm->sum_infectiousness     / float( p_ndm->num_people );
            avg_parasite       = p_ndm->sum_parasite_density   / float( p_ndm->num_people );
            avg_gametocyte     = p_ndm->sum_gametocyte_density / float( p_ndm->num_people );
            avg_pfemp1         = p_ndm->sum_pfemp1             / float( p_ndm->num_people );
        }

        float avg_num_infs = 0.0;
        if( p_ndm->num_infected > 0 )
        {
            avg_num_infs = float( p_ndm->num_infections ) / float( p_ndm->num_infected );
        }

        float avg_inf_dur = 0.0;
        if( p_ndm->num_inf_cleared > 0 )
        {
            avg_inf_dur = p_ndm->sum_inf_cleard_dur / float( p_ndm->num_inf_cleared );
        }

        GetOutputStream() << "," << avg_infectiousness;
        GetOutputStream() << "," << avg_parasite;
        GetOutputStream() << "," << avg_gametocyte;
        GetOutputStream() << "," << avg_pfemp1;
        GetOutputStream() << "," << avg_num_infs;
        GetOutputStream() << "," << avg_inf_dur;
        GetOutputStream() << "," << p_ndm->num_inf_cleared;
        GetOutputStream() << "," << p_ndm->num_has_fever;
        if( !m_StratifyBySymptoms )
        {
            GetOutputStream() << "," << p_ndm->num_has_symptoms;
        }
    }

    void ReportNodeDemographicsMalaria::UpdateEventRegistration( float currentTime,
                                                                 float dt,
                                                                 std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                 ISimulationEventContext* pSimEventContext )
    {
        if( !m_IsRegistered )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                release_assert( pNEC );
                IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                for( auto trigger :m_EventsOfInterest )
                {
                    broadcaster->RegisterObserver( this, trigger );
                }
            }
            m_IsRegistered = true;
        }
    }

    bool ReportNodeDemographicsMalaria::notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger )
    {
        if( trigger == EventTrigger::InfectionCleared )
        {
            int gender_index  = m_StratifyByGender ? (int)(pEntity->GetGender()) : 0;
            int age_bin_index = ReportUtilities::GetAgeBin( pEntity->GetAge(), m_AgeYears );
            int ip_index      = GetIPIndex( pEntity->GetProperties() );
            int other_index   = GetIndexOfOtherStratification( pEntity );

            NodeData* p_nd = m_Data[ gender_index ][ age_bin_index ][ ip_index ][ other_index ];
            NodeDataMalaria* p_ndm = (NodeDataMalaria*)p_nd;

            const IIndividualHuman* individual = pEntity->GetIndividualHumanConst();
            bool found = false;
            for( auto p_inf : individual->GetInfections() )
            {
                if( p_inf->GetStateChange() == InfectionStateChange::Cleared )
                {
                    found = true;
                    p_ndm->num_inf_cleared += 1;
                    p_ndm->sum_inf_cleard_dur += p_inf->GetDuration();
                    break;
                }
            }
            release_assert( found );
        }
        return true;
    }

    bool ReportNodeDemographicsMalaria::HasOtherStratificationColumn() const
    {
        return m_StratifyBySymptoms;
    }

    std::string ReportNodeDemographicsMalaria::GetOtherStratificationColumnName() const
    {
        return "HasClinicalSymptoms";
    }

    int ReportNodeDemographicsMalaria::GetNumInOtherStratification() const
    {
        if( m_StratifyBySymptoms )
            return 2;
        else
            return 1;
    }

    std::string ReportNodeDemographicsMalaria::GetOtherStratificationValue( int otherIndex ) const
    {
        if( otherIndex == 0 )
            return "F";
        else
            return "T";
    }

    int ReportNodeDemographicsMalaria::GetIndexOfOtherStratification( IIndividualHumanEventContext* individual ) const
    {
        IMalariaHumanContext* p_human_malaria = nullptr;
        if( m_StratifyBySymptoms )
        {
            if( s_OK != individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_human_malaria ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IMalariaHumanContext", "IIndividualHuman" );
            }
        }

        if( m_StratifyBySymptoms && p_human_malaria->HasClinicalSymptomContinuing( ClinicalSymptomsEnum::CLINICAL_DISEASE ) )
            return 1;
        else
            return 0;
    }

}
