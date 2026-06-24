
#include "stdafx.h"

#include <functional>
#include <map>

#include "SpatialReportMalaria.h"
#include "MalariaContexts.h"
#include "Sugar.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "ProgVersion.h"
#include "ReportUtilitiesMalaria.h"
#include "INodeContext.h"

using namespace std;

SETUP_LOGGING( "SpatialReportMalaria" )

namespace Kernel {

GET_SCHEMA_STATIC_WRAPPER_IMPL(SpatialReportMalaria,SpatialReportMalaria)

BEGIN_QUERY_INTERFACE_BODY( SpatialReportMalaria )
    HANDLE_INTERFACE( IReportMalariaDiagnostics )
    HANDLE_INTERFACE( IReport )
    HANDLE_ISUPPORTS_VIA( IReport )
END_QUERY_INTERFACE_BODY( SpatialReportMalaria )

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
SpatialReportMalaria::CreateReport()
{
    return new SpatialReportMalaria();
}

SpatialReportMalaria::SpatialReportMalaria()
: SpatialReportVector()
, prevalence_by_diagnostic_info()
, mean_parasitemia_info(   "Mean_Parasitemia",   "geo. mean parasites/microliter")
, new_clinical_cases_info( "New_Clinical_Cases", "")
, new_severe_cases_info(   "New_Severe_Cases",   "")
, m_DetectionThresholds()
, m_Detected()
, m_MeanParasitemia(0.0)
{
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "Blood_Smear_Parasite_Prevalence",   "% Detected Infected" ) );
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "Blood_Smear_Gametocyte_Prevalence", "% Detected Infected" ) );
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "PCR_Parasite_Prevalence",           "% Detected Infected" ) );
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "PCR_Gametocyte_Prevalence",         "% Detected Infected" ) );
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "PfHRP2_Prevalence",                 "% Detected Infected" ) );
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "True_Prevalence",                   "% Detected Infected" ) );
    prevalence_by_diagnostic_info.push_back( ChannelInfo( "Fever_Prevalence",                  "% Detected Infected" ) );

    release_assert( MalariaDiagnosticType::pairs::count() == prevalence_by_diagnostic_info.size() );

    m_Detected.resize( MalariaDiagnosticType::pairs::count(), 0.0 );
}

void SpatialReportMalaria::Initialize( unsigned int nrmSize )
{
    SpatialReportVector::Initialize( nrmSize );

    if( mean_parasitemia_info.enabled && !prevalence_by_diagnostic_info[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].enabled )
    {
        LOG_WARN("Mean_Parasitemia requires that Parasite_Prevalence be enabled.  Enabling Parasite_Prevalence.");
        prevalence_by_diagnostic_info[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].enabled = true ;
        channelDataMap.IncreaseChannelLength( prevalence_by_diagnostic_info[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].name, _nrmSize );
    }
}

void SpatialReportMalaria::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
{
    m_DetectionThresholds = rDetectionThresholds;
}


void SpatialReportMalaria::populateChannelInfos(tChanInfoMap &channel_infos)
{
    SpatialReportVector::populateChannelInfos(channel_infos);

    for( auto& r_pbdi : prevalence_by_diagnostic_info )
    {
        channel_infos[ r_pbdi.name ] = &r_pbdi;
    }
    channel_infos[ mean_parasitemia_info.name   ] = &mean_parasitemia_info;
    channel_infos[ new_clinical_cases_info.name ] = &new_clinical_cases_info;
    channel_infos[ new_severe_cases_info.name   ] = &new_severe_cases_info;
}

void SpatialReportMalaria::LogIndividualData( Kernel::IIndividualHuman* individual )
{
    SpatialReportVector::LogIndividualData( individual );

    release_assert( m_DetectionThresholds.size() == prevalence_by_diagnostic_info.size() );

    ReportUtilitiesMalaria::LogIndividualMalariaInfectionAssessment( individual,
                                                                     m_DetectionThresholds,
                                                                     m_Detected,
                                                                     m_MeanParasitemia );
}


void
SpatialReportMalaria::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    SpatialReportVector::LogNodeData(pNC);

    auto nodeid = pNC->GetExternalID();

    const INodeMalaria* pMalariaNode = nullptr;
    if( pNC->QueryInterface( GET_IID(INodeMalaria), (void**)&pMalariaNode ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeMalaria", "INodeContext" );
    }

    for( int i = 0 ; i < prevalence_by_diagnostic_info.size() ; ++i )
    {
        ChannelInfo& r_pbdi = prevalence_by_diagnostic_info[ i ];
        if( r_pbdi.enabled )
        {
            Accumulate( r_pbdi.name, nodeid, m_Detected[ i ] );
        }
    }
    std::fill( m_Detected.begin(), m_Detected.end(), 0.0f );

    if(mean_parasitemia_info.enabled)
        Accumulate( mean_parasitemia_info.name, nodeid, m_MeanParasitemia );

    if(new_clinical_cases_info.enabled)
        Accumulate( new_clinical_cases_info.name, nodeid, pMalariaNode->GetNewClinicalCases() );

    if(new_severe_cases_info.enabled)
        Accumulate( new_severe_cases_info.name, nodeid, pMalariaNode->GetNewSevereCases() );
}

void
SpatialReportMalaria::postProcessAccumulatedData()
{
    SpatialReportVector::postProcessAccumulatedData();

    // make sure to normalize Mean Parasitemia BEFORE Parasite Prevalence, then it is exponentiated
    if( mean_parasitemia_info.enabled && prevalence_by_diagnostic_info[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].enabled )
    {
        normalizeChannel(mean_parasitemia_info.name, prevalence_by_diagnostic_info[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].name);

        // Only need to transform mean log-parasitemia to geometric-mean parasitemia if that channel is enabled.  
        if( channelDataMap.HasChannel( mean_parasitemia_info.name ) )
        {
            channelDataMap.ExponentialValues( mean_parasitemia_info.name );
        }
    }
    else if( mean_parasitemia_info.enabled && !prevalence_by_diagnostic_info[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].enabled )
    {
        throw GeneralConfigurationException(  __FILE__, __LINE__, __FUNCTION__, "If 'Mean_Parasitemia' is enabled, then 'Parasite_Prevalence' must be enabled.");
    }

    // now normalize rest of channels
    for( auto& r_pbdi : prevalence_by_diagnostic_info )
    {
        if( r_pbdi.enabled )
        {
            normalizeChannel( r_pbdi.name, population_info.name );
        }
    }
}

}
