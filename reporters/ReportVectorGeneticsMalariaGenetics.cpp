
#include "stdafx.h"

#include "ReportVectorGeneticsMalariaGenetics.h"

#include "report_params.rc"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "ReportUtilities.h"
#include "IdmDateTime.h"
#include "IVectorCohort.h"
#include "ParasiteGenetics.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorGeneticsMalariaGenetics" ) // <<< Name of this file

namespace Kernel
{
    // -----------------------------------------------
    // --- ReportVectorGeneticsMalariaGenetics Methods
    // -----------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportVectorGeneticsMalariaGenetics, ReportVectorGenetics )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportVectorGeneticsMalariaGenetics, ReportVectorGenetics )

    IMPLEMENT_FACTORY_REGISTERED( ReportVectorGeneticsMalariaGenetics )

    ReportVectorGeneticsMalariaGenetics::ReportVectorGeneticsMalariaGenetics()
        : ReportVectorGeneticsMalariaGenetics( "ReportVectorGeneticsMalariaGenetics.csv" )
    {
    }

    ReportVectorGeneticsMalariaGenetics::ReportVectorGeneticsMalariaGenetics( const std::string& rReportName )
        : ReportVectorGenetics( rReportName )
        , m_HaveRegisteredVectorCounter()
        , m_ParasiteBarcodes()
        , m_OocystAndSporozoitDataNameMap()
        , m_OocystAndSporozoitDataBitsMap()
        , m_SporozoiteBarcodeHascodeCountMap()
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();

        initSimTypes(  1, "MALARIA_SIM" );
    }

    ReportVectorGeneticsMalariaGenetics::~ReportVectorGeneticsMalariaGenetics()
    {
        for( auto& r_entry : m_OocystAndSporozoitDataNameMap )
        {
            delete r_entry.second;
        }
        m_OocystAndSporozoitDataNameMap.clear();
        m_OocystAndSporozoitDataBitsMap.clear();
        m_SporozoiteBarcodeHascodeCountMap.clear();
    }

    bool ReportVectorGeneticsMalariaGenetics::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Parasite_Barcodes", &m_ParasiteBarcodes, RVGMG_Parasite_Barcodes_DESC_TEXT );

        bool ret = ReportVectorGenetics::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING(EnvPtr->Config, "Malaria_Model") != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__,
                                                     "'ReportVectorGeneticsMalariaGenetics' can only be used with 'MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
            }
            if( (m_StratifyBy == StratifyBy::ALLELE) || (m_StratifyBy == StratifyBy::ALLELE_FREQ) )
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__,
                                                  "'ReportVectorGeneticsMalariaGenetics' does not support stratifying by allele at this time." );
            }
        }
        return ret;
    }

    std::string ReportVectorGeneticsMalariaGenetics::GetHeader() const
    {
        std::stringstream header ;
        header << ReportVectorGenetics::GetHeader();

        header << ",AvgOocystDurationDays";
        header << ",AvgNumSporozoitesPerVector";

        for( auto barcode : m_ParasiteBarcodes )
        {
            header << ",NumVectorsWithSporozoites_" << barcode;
        }
        if( m_ParasiteBarcodes.size() > 0 )
        {
            header << ",NumVectorsWithSporozoites_Other";
        }

        return header.str();
    }

    void ReportVectorGeneticsMalariaGenetics::InitializeStratificationData( IVectorPopulationReporting* pIVPR )
    {
        ReportVectorGenetics::InitializeStratificationData( pIVPR );
        if( (m_StratifyBy == StratifyBy::GENOME) || (m_StratifyBy == StratifyBy::SPECIFIC_GENOME) )
        {
            for( auto& r_entry : m_PossibleGenomes )
            {
                OccystAndSporozoiteData* p_data = new OccystAndSporozoiteData();
                for( auto bar : m_ParasiteBarcodes )
                {
                    const std::vector<int64_t>& r_hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "Parasite_Barcodes", bar );
                    ReportUtilitiesMalaria::BarcodeColumn* p_bc = new ReportUtilitiesMalaria::BarcodeColumn( bar );
                    p_data->barcode_columns.push_back( p_bc );

                    for( int64_t hash : r_hashcodes )
                    {
                        p_data->barcode_column_map[ hash ] = p_bc;
                    }
                }

                m_OocystAndSporozoitDataNameMap[ r_entry.name ] = p_data;
                m_OocystAndSporozoitDataBitsMap[ r_entry.genome.GetBits() ] = p_data;
                for( auto genome : r_entry.similar_genomes )
                {
                    m_OocystAndSporozoitDataBitsMap[ genome.GetBits() ] = p_data;
                }
            }
        }
    }

    void ReportVectorGeneticsMalariaGenetics::ResetOtherCounters()
    {
        for( auto& r_entry : m_OocystAndSporozoitDataNameMap )
        {
            r_entry.second->Reset();
        }
    }

    void ReportVectorGeneticsMalariaGenetics::CollectOtherDataByGenome( NameToColumnData& rColumnData )
    {
        OccystAndSporozoiteData* p_data = m_OocystAndSporozoitDataNameMap[ rColumnData.name ];
        float avg_duration_per_oocyst = 0.0;
        if( p_data->num_maturing_oocyst > 0 )
        {
            avg_duration_per_oocyst = p_data->sum_oocyst_duration / float( p_data->num_maturing_oocyst );
        }
        float avg_num_sporozoites_per_vector = 0.0;
        if( p_data->num_vectors > 0.0 )
        {
            avg_num_sporozoites_per_vector = p_data->sum_sporozoites / float( p_data->num_vectors );
        }
        rColumnData.other.push_back( avg_duration_per_oocyst );
        rColumnData.other.push_back( avg_num_sporozoites_per_vector );
        for( auto p_column : p_data->barcode_columns )
        {
            rColumnData.other.push_back( p_column->GetCount() );
        }
        if( p_data->barcode_columns.size() > 0 )
        {
            rColumnData.other.push_back( p_data->barcode_column_other.GetCount() );
        }
    }

    void ReportVectorGeneticsMalariaGenetics::CountVector( IVectorCohort* pCohort )
    {
        if( pCohort->GetState() == VectorStateEnum::STATE_INFECTIOUS  )
        {
            IVectorCohortIndividualMalariaGenetics *p_ivcmg = nullptr;
            if( pCohort->QueryInterface( GET_IID( IVectorCohortIndividualMalariaGenetics ), (void**)&p_ivcmg ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "p_cohort", "IVectorCohortIndividualMalariaGenetics", "IVectorCohort" );
            }

            OccystAndSporozoiteData* p_data = m_OocystAndSporozoitDataBitsMap[ pCohort->GetGenome().GetBits() ];

            p_data->sum_oocyst_duration += p_ivcmg->GetSumOfDurationsOfMaturingOocysts();
            p_data->num_maturing_oocyst += p_ivcmg->GetNumMaturingOocysts();
            p_data->sum_sporozoites     += p_ivcmg->GetNumSporozoites();
            p_data->num_vectors         += 1;

            m_SporozoiteBarcodeHascodeCountMap.clear();

            p_ivcmg->CountSporozoiteBarcodeHashcodes( m_SporozoiteBarcodeHascodeCountMap );

            // counting vectors with the barcode
            bool found_other = false;
            for( auto spz_barcode_count : m_SporozoiteBarcodeHascodeCountMap )
            {
                if( spz_barcode_count.second > 0 )
                {
                    if( p_data->barcode_column_map.find( spz_barcode_count.first ) != p_data->barcode_column_map.end() )
                    {
                        p_data->barcode_column_map[ spz_barcode_count.first ]->AddCount( 1 ); // 1=the vector
                    }
                    else
                    {
                        found_other = true;
                    }
                }
            }
            // only count the vector once if it has at least one sporozoite cohort that is "other"
            if( found_other )
            {
                p_data->barcode_column_other.AddCount( 1 );
            }
        }
    }

    void ReportVectorGeneticsMalariaGenetics::LogNodeData( INodeContext * pNC )
    {
        ReportVectorGenetics::LogNodeData( pNC );

        if( !m_HaveRegisteredVectorCounter[ pNC ] )
        {
            m_HaveRegisteredVectorCounter[ pNC ] = true;

            INodeVector * pNodeVector = NULL;
            if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
            {
                throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
            }

            const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
            for( auto p_ivpr : vector_pop_list )
            {
                IVectorPopulationReportingMalariaGenetics *p_vprmg = nullptr;
                if( p_ivpr->QueryInterface( GET_IID( IVectorPopulationReportingMalariaGenetics ), (void**)&p_vprmg ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                                   "p_ivpr", "IVectorPopulationReportingMalariaGenetics", "IVectorPopulationReporting" );
                }
                p_vprmg->RegisterCounter( this );
            }
        }
    }

}
