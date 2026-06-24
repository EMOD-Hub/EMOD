
#include "stdafx.h"

#include "ReportVectorStatsMalariaGenetics.h"

#include "report_params.rc"
#include "VectorContexts.h"
#include "MalariaGeneticsContexts.h"
#include "ParasiteGenetics.h"
#include "IVectorCohort.h"
#include "INodeContext.h"


// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorStatsMalariaGenetics" ) // <<< Name of this file

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( ReportVectorStatsMalariaGenetics, ReportVectorStats )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportVectorStatsMalariaGenetics, ReportVectorStats )

    IMPLEMENT_FACTORY_REGISTERED( ReportVectorStatsMalariaGenetics )

    ReportVectorStatsMalariaGenetics::ReportVectorStatsMalariaGenetics()
        : ReportVectorStats( "ReportVectorStatsMalariaGenetics.csv" )
        , m_ParasiteStats()
        , m_BarcodeColumns()
        , m_BarcodeColumnMap()
        , m_BarcodeColumnOther()
        , m_SporozoiteBarcodeHascodeCountMap()
        , m_HaveRegisteredVectorCounter()
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    ReportVectorStatsMalariaGenetics::~ReportVectorStatsMalariaGenetics()
    {
        for( auto p_bc : m_BarcodeColumns )
        {
            delete p_bc;
        }
        m_BarcodeColumns.clear();
        m_BarcodeColumnMap.clear();
    }

    bool ReportVectorStatsMalariaGenetics::Configure( const Configuration * inputJson )
    {
        std::vector<std::string> barcodes;

        initConfigTypeMap( "Barcodes", &barcodes, RVSMG_Barcodes_DESC_TEXT );

        // don't want genome marker columns
        bool is_configured = ReportVectorStats::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if (GET_CONFIG_STRING(EnvPtr->Config, "Malaria_Model") != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS")
            {
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__,
                    "'ReportVectorStatsMalariaGenetics' can only be used with 'MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
            }

            for( auto bar : barcodes )
            {
                const std::vector<int64_t>& r_hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "Barcodes", bar );
                ReportUtilitiesMalaria::BarcodeColumn* p_bc = new ReportUtilitiesMalaria::BarcodeColumn( bar );
                m_BarcodeColumns.push_back( p_bc );

                for( int64_t hash : r_hashcodes )
                {
                    m_BarcodeColumnMap[ hash ] = p_bc;
                }
            }
        }
        return is_configured;
    }

    std::string ReportVectorStatsMalariaGenetics::GetHeader() const
    {
        std::stringstream header ;

        // don't want genome marker columns
        header << ReportVectorStats::GetHeader();

        header << ",NumVectorsNone"
               << ",NumVectorsOnlyOocysts"
               << ",NumVectorsOnlySporozoites"
               << ",NumVectorsBothOocystsSporozoites"
               << ",NumBitesAdults"
               << ",NumBitesInfected"
               << ",NumBitesInfectious"
               << ",NumParasiteCohortsOocysts"
               << ",NumParasiteCohortsSporozoites"
               << ",NumOocysts"
               << ",NumSporozoites"
               << ",NumInfectiousToAdult"
               << ",NumInfectiousToInfected";

        for( auto p_column : m_BarcodeColumns )
        {
            header << "," << p_column->GetColumnName();
        }
        if( m_BarcodeColumns.size() > 0 )
        {
            header << ",OtherBarcodes";
        }

        return header.str();
    }

    void ReportVectorStatsMalariaGenetics::LogNodeData( INodeContext * pNC )
    {
        ReportVectorStats::LogNodeData( pNC );

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

    void ReportVectorStatsMalariaGenetics::CountVector( IVectorCohort* pCohort )
    {
        if( (pCohort->GetState() == VectorStateEnum::STATE_ADULT     ) ||
            (pCohort->GetState() == VectorStateEnum::STATE_INFECTED  ) ||
            (pCohort->GetState() == VectorStateEnum::STATE_INFECTIOUS)  )
        {
            IVectorCohortIndividualMalariaGenetics *p_ivcmg = nullptr;
            if( pCohort->QueryInterface( GET_IID( IVectorCohortIndividualMalariaGenetics ), (void**)&p_ivcmg ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "p_cohort", "IVectorCohortIndividualMalariaGenetics", "IVectorCohort" );
            }

            uint32_t num_cohorts_oocysts     = p_ivcmg->GetNumParasiteCohortsOocysts();
            uint32_t num_cohorts_sporozoites = p_ivcmg->GetNumParasiteCohortsSporozoites();

            m_ParasiteStats.num_cohorts_oocysts     += num_cohorts_oocysts;
            m_ParasiteStats.num_cohorts_sporozoites += num_cohorts_sporozoites;
            m_ParasiteStats.num_oocysts             += p_ivcmg->GetNumOocysts();
            m_ParasiteStats.num_sporozoites         += p_ivcmg->GetNumSporozoites();

            if( (num_cohorts_oocysts > 0) && (num_cohorts_sporozoites == 0) )
                m_ParasiteStats.num_vectors_only_oocysts += 1;
            else if( (num_cohorts_oocysts == 0) && (num_cohorts_sporozoites > 0) )
                m_ParasiteStats.num_vectors_only_sporozoites += 1;
            else if( (num_cohorts_oocysts > 0) && (num_cohorts_sporozoites > 0) )
                m_ParasiteStats.num_vectors_oocysts_sporozoites += 1;
            else
                m_ParasiteStats.num_vectors_none += 1;

            if( p_ivcmg->ChangedFromInfectiousToAdult() )
                m_ParasiteStats.num_infectious_to_adult += 1;
            else if( p_ivcmg->ChangedFromInfectiousToInfected() )
                m_ParasiteStats.num_infectious_to_infected += 1;

            if( pCohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
            {
                m_SporozoiteBarcodeHascodeCountMap.clear();

                p_ivcmg->CountSporozoiteBarcodeHashcodes( m_SporozoiteBarcodeHascodeCountMap );

                // counting vectors with the barcode
                for( auto spz_barcode_count : m_SporozoiteBarcodeHascodeCountMap )
                {
                    if( spz_barcode_count.second > 0 )
                    {
                        if( m_BarcodeColumnMap.find( spz_barcode_count.first ) != m_BarcodeColumnMap.end() )
                        {
                            m_BarcodeColumnMap[ spz_barcode_count.first ]->AddCount( 1 ); // 1=the vector
                        }
                        else
                        {
                            m_BarcodeColumnOther.AddCount( 1 );
                        }
                    }
                }
            }
        }
    }

    void ReportVectorStatsMalariaGenetics::ResetOtherCounters()
    {
        // don't want genome marker columns
        ReportVectorStats::ResetOtherCounters();
        m_OtherStats.Reset();
    }

    void ReportVectorStatsMalariaGenetics::CollectOtherData( IVectorPopulationReporting* pIVPR )
    {
        // don't want genome marker columns
        ReportVectorStats::CollectOtherData( pIVPR );
        
        IVectorPopulationReportingMalariaGenetics *p_vprmg = nullptr;
        if( pIVPR->QueryInterface( GET_IID( IVectorPopulationReportingMalariaGenetics ), (void**)&p_vprmg ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "p_cohort", "IVectorPopulationReportingMalariaGenetics", "IVectorPopulationReporting" );
        }

        OtherVectorStats stats;
        p_vprmg->ExtractOtherVectorStats( stats );

        m_OtherStats += stats; 
    }

    void ReportVectorStatsMalariaGenetics::WriteOtherData()
    {
        // don't want genome marker columns
        ReportVectorStats::WriteOtherData();
        GetOutputStream() << "," << m_ParasiteStats.num_vectors_none
                          << "," << m_ParasiteStats.num_vectors_only_oocysts
                          << "," << m_ParasiteStats.num_vectors_only_sporozoites
                          << "," << m_ParasiteStats.num_vectors_oocysts_sporozoites
                          << "," << m_OtherStats.num_bites_adults
                          << "," << m_OtherStats.num_bites_infected
                          << "," << m_OtherStats.num_bites_infectious
                          << "," << m_ParasiteStats.num_cohorts_oocysts
                          << "," << m_ParasiteStats.num_cohorts_sporozoites
                          << "," << m_ParasiteStats.num_oocysts
                          << "," << m_ParasiteStats.num_sporozoites
                          << "," << m_ParasiteStats.num_infectious_to_adult
                          << "," << m_ParasiteStats.num_infectious_to_infected;

        for( auto p_column : m_BarcodeColumns )
        {
            GetOutputStream() << "," << p_column->GetCount();
        }
        if( m_BarcodeColumns.size() > 0 )
        {
            GetOutputStream() << "," << m_BarcodeColumnOther.GetCount();
        }

        // this needs to be here instead of other counters because this strucutre is updated
        // while the vectors are being updated.
        m_ParasiteStats.Reset();
        m_BarcodeColumnOther.ResetCount();
        for( auto p_column : m_BarcodeColumns )
        {
            p_column->ResetCount();
        }
    }
}
