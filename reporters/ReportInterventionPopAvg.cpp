

#include "stdafx.h"

#include "ReportInterventionPopAvg.h"
#include "NodeEventContext.h"
#include "INodeContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "ISimulationContext.h"
#include "IReportInterventionDataAccess.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "ReportInterventionPopAvg" )

namespace Kernel
{
    // ----------------------------------------
    // --- ReportInterventionPopAvg Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportInterventionPopAvg, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportInterventionPopAvg, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportInterventionPopAvg )

    ReportInterventionPopAvg::ReportInterventionPopAvg()
        : ReportInterventionPopAvg( "ReportInterventionPopAvg.csv" )
    {
    }

    ReportInterventionPopAvg::ReportInterventionPopAvg( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_IsValidTime(false)
        , m_Stats()
        , m_NotSupported()
    {
        initSimTypes( 1, "MALARIA_SIM" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportInterventionPopAvg::~ReportInterventionPopAvg()
    {
    }

    bool ReportInterventionPopAvg::Configure( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        bool ret = JsonConfigurable::Configure( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );
        }
        return ret;
    }

    void ReportInterventionPopAvg::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
        BaseTextReport::Initialize( nrmSize );
    }

    void ReportInterventionPopAvg::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), demographicNodeIds );
    }

    std::string ReportInterventionPopAvg::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"
               << ",NodeID"
               << ",NodePopulation"
               << ",InterventionName"
               << ",FractionHas"
               << ",AvgNumberOfInterventions"
               << ",AvgEfficacy-Attracting"
               << ",AvgEfficacy-Repelling"
               << ",AvgEfficacy-Blocking"
               << ",AvgEfficacy-Killing"
               << ",AvgEfficacy-Usage"
               << ",AvgEfficacy-AcquisitionBlocking"
               << ",AvgEfficacy-TransmissionBlocking"
               << ",AvgEfficacy-MortalityBlocking"
               << ",DrugConcentration"
            ;

        return header.str();
    }

    void ReportInterventionPopAvg::UpdateEventRegistration( float currentTime,
                                                       float dt,
                                                       std::vector<INodeEventContext*>& rNodeEventContextList,
                                                       ISimulationEventContext* pSimEventContext )
    {
        m_IsValidTime = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
    }

    bool ReportInterventionPopAvg::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsValidTime;
    }

    void ReportInterventionPopAvg::LogIndividualData( IIndividualHuman* individual ) 
    {
        INodeEventContext* p_nec = individual->GetEventContext()->GetNodeEventContext();

        if( !m_ReportFilter.IsValidNode( p_nec ) ) return;
        if( !m_ReportFilter.IsValidHuman( individual ) ) return;

        float time = p_nec->GetTime().time;
        uint32_t node_id = p_nec->GetExternalId();

        std::set<std::string> intervention_name_set;
        auto interventions = individual->GetInterventionsContext()->GetInterventions();
        for( auto p_interven : interventions )
        {
            const std::string& r_name = p_interven->GetName().ToString();
            UpdateStats( r_name, p_interven, intervention_name_set );
        }
    }

    void ReportInterventionPopAvg::LogNodeData( INodeContext * pNC )
    {
        if( !m_IsValidTime ) return;
        if( !m_ReportFilter.IsValidNode( pNC->GetEventContext() ) ) return;

        float time = pNC->GetTime().time;
        uint32_t node_id = pNC->GetExternalID();
        float node_pop = float( pNC->GetEventContext()->GetIndividualHumanCount() );

        std::set<std::string> intervention_name_set;
        auto p_interventions = pNC->GetEventContext()->GetNodeInterventions();
        for( auto p_node_interven : p_interventions )
        {
            const std::string& r_name = p_node_interven->GetName().ToString();
            UpdateStats( r_name, p_node_interven, intervention_name_set );
        }

        for( auto& r_entry : m_Stats )
        {
            release_assert( r_entry.second.num_has > 0 );

            float frac_has = 0.0;
            if( r_entry.second.is_individual )
            {
                frac_has = r_entry.second.num_has / node_pop;
            }
            else
            {
                // we are reporting the intervention for each node so we cannot average over the nodes
                frac_has = 1.0f;
            }

            float avg_num_interventions = r_entry.second.num_instances / r_entry.second.num_has;

            float attracting = r_entry.second.efficacy_attracting / r_entry.second.num_instances;
            float repelling  = r_entry.second.efficacy_repelling  / r_entry.second.num_instances;
            float blocking   = r_entry.second.efficacy_blocking   / r_entry.second.num_instances;
            float killing    = r_entry.second.efficacy_killing    / r_entry.second.num_instances;
            float usage      = r_entry.second.efficacy_usage      / r_entry.second.num_instances;
            float acq        = r_entry.second.efficacy_acq        / r_entry.second.num_instances;
            float tran       = r_entry.second.efficacy_tran       / r_entry.second.num_instances;
            float mort       = r_entry.second.efficacy_mort       / r_entry.second.num_instances;
            float drug       = r_entry.second.concentration_drug  / r_entry.second.num_instances;

            GetOutputStream()
                << time
                << "," << node_id
                << "," << node_pop
                << "," << r_entry.second.intervention_name
                << "," << frac_has
                << "," << avg_num_interventions
                << "," << attracting
                << "," << repelling
                << "," << blocking
                << "," << killing
                << "," << usage
                << "," << acq
                << "," << tran
                << "," << mort
                << "," << drug
                << "\n";
        }
        m_Stats.clear();
    }

    std::string ReportInterventionPopAvg::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( BaseTextReport::GetReportName() );
    }

    void ReportInterventionPopAvg::UpdateStats( const std::string& rInterventionName,
                                                ISupports* pIntervention,
                                                std::set<std::string>& rInterventionNameSet )
    {
        bool already_has = (rInterventionNameSet.find( rInterventionName ) != rInterventionNameSet.end());
        if( !already_has )
        {
            rInterventionNameSet.insert( rInterventionName );
        }

        IReportInterventionDataAccess * p_report_access = nullptr;
        if( s_OK == pIntervention->QueryInterface( GET_IID( IReportInterventionDataAccess ), (void**)&p_report_access ) )
        {
            ReportInterventionData rid = p_report_access->GetReportInterventionData();
            if( m_Stats.find( rInterventionName ) == m_Stats.end() )
            {
                InterventionStats stats;
                stats.intervention_name = rInterventionName;
                stats.is_individual = rid.is_individual;
                m_Stats.insert( std::make_pair( rInterventionName, stats ) );
            }
            InterventionStats& r_stats = m_Stats[ rInterventionName ];
            r_stats.AddData( rid, already_has );
        }
        else if( m_NotSupported.find( rInterventionName ) == m_NotSupported.end() )
        {
            LOG_WARN_F( "The intervention, %s, does not support IReportInterventionDataAccess so it cannot be included in ReportInterventionPopAvg.\n", rInterventionName.c_str() );
            m_NotSupported.insert( rInterventionName );
        }
    }
}