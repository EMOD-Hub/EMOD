
#include "stdafx.h"

#include "ReportMalariaFiltered.h"
#include "report_params.rc"
#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "IMigrate.h"
#include "INodeContext.h"
#include "SimulationEventContext.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#endif

SETUP_LOGGING( "ReportMalariaFiltered" )

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportMalariaFiltered()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

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

// ----------------------------------------
// --- ReportMalariaFiltered Methods
// ----------------------------------------

#define DEFAULT_NAME ("ReportMalariaFiltered.json")

    BEGIN_QUERY_INTERFACE_DERIVED( ReportMalariaFiltered, ReportMalaria )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportMalariaFiltered, ReportMalaria )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportMalariaFiltered )
#endif

    ReportMalariaFiltered::ReportMalariaFiltered()
        : ReportMalaria()
        , stat_pop_age_range_id()
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_IsValidDay(false)
        , m_HasValidNode(false)
        , m_HasCollectedData(false)
    {
        initSimTypes( 1, "MALARIA_SIM" );
        report_name = DEFAULT_NAME;

        stat_pop_age_range_id = AddChannel( "Statistical Population-AgeRange" );
    }

    ReportMalariaFiltered::~ReportMalariaFiltered()
    { }

    bool ReportMalariaFiltered::Configure( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        bool ret = ReportMalaria::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );
        }
        return ret;
    }

    const char* ReportMalariaFiltered::GetParameterNameForHasInterventions() const
    {
        return "Has_Interventions";
    }

    const char* ReportMalariaFiltered::GetDescTextForHasInterventions() const
    {
        return RMF_Has_Interventions_DESC_TEXT;
    }

    const char* ReportMalariaFiltered::GetDependsOnForHasInterventions() const
    {
        return nullptr;
    }

    const char* ReportMalariaFiltered::GetParameterNameForHasIP() const
    {
        return "Has_IP";
    }

    const char* ReportMalariaFiltered::GetDescTextForHasIP() const
    {
        return RMF_Has_IP_DESC_TEXT;
    }

    const char* ReportMalariaFiltered::GetDependsOnForHasIP() const
    {
        return nullptr;
    }

    const char* ReportMalariaFiltered::GetParameterNameForIncludePregancies() const
    {
        return "Include_Pregnancies";
    }

    const char* ReportMalariaFiltered::GetDescTextForIncludePregancies() const
    {
        return RMF_Include_Pregnancies_DESC_TEXT;
    }

    const char* ReportMalariaFiltered::GetDependsOnForIncludePregancies() const
    {
        return nullptr;
    }

    const char* ReportMalariaFiltered::GetParameterNameFor30DayAvg() const
    {
        return "Include_30Day_Avg_Infection_Duration";
    }

    const char* ReportMalariaFiltered::GetDescTextFor30DayAvg() const
    {
        return RMF_Reporting_Include_30Day_Avg_Infection_Duration_DESC_TEXT;
    }

    const char* ReportMalariaFiltered::GetDependsOnFor30DayAvg() const
    {
        return nullptr;
    }

    void ReportMalariaFiltered::populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map )
    {
        ReportVector::populateSummaryDataUnitsMap( units_map );

        units_map[ stat_pop_age_range_id.GetName() ] = "";
    }

    void ReportMalariaFiltered::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
        ReportMalaria::Initialize( nrmSize );
    }

    void ReportMalariaFiltered::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), nodeIds_demographics );

        // ----------------------------------------------------------------------------------
        // --- We only want to normalize on the number of nodes included in the report.
        // --- If GetNumNodesIncluded() == 0, then we are doing all nodes in the simulation.
        // --- Have to put this here because CheckForValidNodeIDs() gets called after Initialize().
        // ----------------------------------------------------------------------------------
        uint32_t tmp_num_nodes = m_ReportFilter.GetNumNodesIncluded();
        if( tmp_num_nodes > 0 )
        {
            _nrmSize = tmp_num_nodes;
        }
        release_assert( _nrmSize );
    }

    void ReportMalariaFiltered::UpdateEventRegistration( float currentTime,
                                                         float dt, 
                                                         std::vector<INodeEventContext*>& rNodeEventContextList,
                                                         ISimulationEventContext* pSimEventContext )
    {
        m_IsValidDay =  m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
        m_HasCollectedData |= m_IsValidDay;

        if( m_IsValidDay )
        {
            ReportMalaria::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
        }
    }

    void ReportMalariaFiltered::BeginTimestep()
    {
        m_HasValidNode = false;
        if( m_IsValidDay )
        {
            ReportMalaria::BeginTimestep();
            BeginTimestepOther();
        }
    }

    void ReportMalariaFiltered::EndTimestep( float currentTime, float dt )
    {
        if( m_IsValidDay && m_HasValidNode )
        {
            // don't accumulate "Disease Deaths" if no valid nodes
            ReportMalaria::EndTimestep( currentTime, dt );
            EndTimestepOther( currentTime, dt );
        }
    }

    void ReportMalariaFiltered::LogNodeData( INodeContext* pNC )
    {
        if( m_IsValidDay && m_ReportFilter.IsValidNode( pNC->GetEventContext() ) )
        {
            m_HasValidNode = true;
            ReportMalaria::LogNodeData( pNC );
            LogNodeDataOther( pNC );
        }
    }

    void ReportMalariaFiltered::LogIndividualData( IIndividualHuman* individual ) 
    {
        if( !m_IsValidDay ) return;
        if( !m_ReportFilter.IsValidNode( individual->GetEventContext()->GetNodeEventContext() ) ) return;
        if( !m_ReportFilter.IsValidHuman( individual ) ) return;

        Accumulate( stat_pop_age_range_id, 1 );
        ReportMalaria::LogIndividualData( individual );
        LogIndividualDataOther( individual );
    }

    bool ReportMalariaFiltered::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsValidDay;
    }

    bool ReportMalariaFiltered::notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger )
    {
        if( !m_IsValidDay ) return false;
        if( !m_ReportFilter.IsValidNode( pEntity->GetNodeEventContext() ) ) return false;
        if( !m_ReportFilter.IsValidHuman( pEntity->GetIndividualHumanConst() ) ) return false;

        ReportMalaria::notifyOnEvent( pEntity, trigger );
        notifyOnEventOther( pEntity, trigger );

        return true;
    }

    void ReportMalariaFiltered::postProcessAccumulatedData()
    {
        if( m_HasCollectedData )
        {
            // --------------------------------------------------------------------
            // --- EIR and HBR will get "normalized" in ReportVector.
            // --- This is to adjust things so we get rates for this subpopulation
            // --- that are similar to the total population
            // --------------------------------------------------------------------
            const ChannelDataMap::channel_data_t& r_pop_all  = channelDataMap.GetChannel( Report::stat_pop_id.GetName()           );
            const ChannelDataMap::channel_data_t& r_pop_age  = channelDataMap.GetChannel( stat_pop_age_range_id.GetName()         );
            const ChannelDataMap::channel_data_t& r_eir_data = channelDataMap.GetChannel( ReportVector::daily_EIR_id.GetName()    );
            const ChannelDataMap::channel_data_t& r_hbr_data = channelDataMap.GetChannel( ReportVector::daily_HBR_id.GetName()    );
            const ChannelDataMap::channel_data_t& r_res_data = channelDataMap.GetChannel( Report::hum_infectious_res_id.GetName() );

            release_assert( r_pop_all.size() == r_pop_age.size() );
            release_assert( r_pop_all.size() == r_eir_data.size() );
            release_assert( r_pop_all.size() == r_hbr_data.size() );
            release_assert( r_pop_all.size() == r_res_data.size() );

            ChannelDataMap::channel_data_t new_eir_data;
            ChannelDataMap::channel_data_t new_hbr_data;
            ChannelDataMap::channel_data_t new_res_data;
            for( int i = 0; i < r_eir_data.size(); ++i )
            {
                float new_eir = 0.0;
                float new_hbr = 0.0;
                float new_res = 0.0;
                if( r_pop_all[ i ]  != 0.0 )
                {
                    new_eir = r_eir_data[ i ] * r_pop_age[ i ] / r_pop_all[ i ];
                    new_hbr = r_hbr_data[ i ] * r_pop_age[ i ] / r_pop_all[ i ];
                    new_res = r_res_data[ i ] * r_pop_age[ i ] / r_pop_all[ i ];
                }
                new_eir_data.push_back( new_eir );
                new_hbr_data.push_back( new_hbr );
                new_res_data.push_back( new_res );
            }
            channelDataMap.RemoveChannel(  ReportVector::daily_EIR_id.GetName() );
            channelDataMap.SetChannelData( ReportVector::daily_EIR_id.GetName(), new_eir_data );

            channelDataMap.RemoveChannel(  ReportVector::daily_HBR_id.GetName() );
            channelDataMap.SetChannelData( ReportVector::daily_HBR_id.GetName(), new_hbr_data );

            channelDataMap.RemoveChannel(  Report::hum_infectious_res_id.GetName() );
            channelDataMap.SetChannelData( Report::hum_infectious_res_id.GetName(), new_res_data );

            // -----------------------------------------------------
            // --- Replace that stat pop with the people we counted
            // -----------------------------------------------------
            channelDataMap.RemoveChannel( Report::stat_pop_id.GetName() );
            channelDataMap.SetChannelData( Report::stat_pop_id.GetName(), r_pop_age );
            channelDataMap.RemoveChannel( stat_pop_age_range_id.GetName() );

            channelDataMap.RemoveChannel( Report::infected_id.GetName() );
            channelDataMap.SetChannelData( Report::infected_id.GetName(), channelDataMap.GetChannel( ReportMalaria::num_people_infected_id.GetName() ) );
            // will be normalized in Report::postProcessAccumulatedData()

            postProcessAccumulatedDataOther();

            ReportMalaria::postProcessAccumulatedData();

            RemoveUnwantedChannels();
        }
    }

    std::string ReportMalariaFiltered::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( ReportMalaria::GetReportName() );
    }
}
