
#include "stdafx.h"

#include "ReportSimulationStats.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "ISimulationContext.h"
#include "IIndividualHuman.h"
#include "Interventions.h"
#include "Memory.h"
#include "IdmString.h"

SETUP_LOGGING( "ReportSimulationStats" )

using namespace std;

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( ReportSimulationStats, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportSimulationStats, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportSimulationStats )

    ReportSimulationStats::ReportSimulationStats()
        : BaseTextReport("ReportSimulationStats.csv")
        , m_pSim(nullptr)
        , m_InitialTimeSim(0)
        , m_InitialTimeStep(0)
        , m_NumNodes(0)
        , m_NumPeople(0)
        , m_NumInterventionsPersisted(0)
        , m_NumInterventionsAdded(0)
        , m_NumInfections(0)
        , m_NumEventsTriggered(0)
        , m_NumEventsObserved(0)
    {
        initSimTypes( 1, "*" );
        m_InitialTimeSim = time( nullptr );
        m_InitialTimeStep = m_InitialTimeSim;
    }

    ReportSimulationStats::~ReportSimulationStats()
    {
    }

    std::string ReportSimulationStats::GetHeader() const
    {
        std::stringstream header ;
        header
                   << "TotalDuration(secs)"
            << "," << "SimulationTime(Days)"
            << "," << "Rank"
            << "," << "StepDuration(secs)"
            << "," << "WorkingMemory(MB)"
            << "," << "PeakWorkingMemory(MB)"
            << "," << "VirtualMemory(MB)"
            << "," << "PeakVirtualMemory(MB)"
            << "," << "FreeRAM(MB)"
            << "," << "TotalRAM(MB)"
            << "," << "FreeSwap(MB)"
            << "," << "TotalSwap(MB)"
            << "," << "NumNodes"
            << "," << "NumIndividuals"
            << "," << "NumInterventionsPersisted"
            << "," << "NumInterventionsAdded"
            << "," << "NumInfections"
            << "," << "NumIndividualEventsTriggered"
            << "," << "NumIndividualEventsObserved"
            ;
        return header.str();
    }

    void ReportSimulationStats::LogIndividualData( IIndividualHuman* individual )
    {
        ++m_NumPeople;
        m_NumInfections += individual->GetInfections().size();
        m_NumInterventionsPersisted += individual->GetInterventionsContext()->GetNumInterventions();
        m_NumInterventionsAdded += individual->GetInterventionsContext()->GetNumInterventionsAdded();
    }

    void ReportSimulationStats::LogNodeData( INodeContext* pNC )
    {
        if( m_pSim == nullptr )
        {
            m_pSim = pNC->GetParent();
        }
        ++m_NumNodes;
        m_NumEventsTriggered += pNC->GetEventContext()->GetIndividualEventBroadcaster()->GetNumTriggeredEvents();
        m_NumEventsObserved += pNC->GetEventContext()->GetIndividualEventBroadcaster()->GetNumObservedEvents();
    }

    void ReportSimulationStats::EndTimestep( float currentTime, float dt )
    {
        time_t now = time( nullptr );
        time_t sim_duration = now - m_InitialTimeSim;
        time_t step_duration = now - m_InitialTimeStep;
        m_InitialTimeStep = now;

        const ProcessMemoryInfo& r_proc_mem = m_pSim->GetProcessMemory();
        const SystemMemoryInfo&  r_sys_mem  = m_pSim->GetSystemMemory();

        GetOutputStream() 
                   << currentTime
            << "," << EnvPtr->MPI.Rank
            << "," << step_duration
            << "," << r_proc_mem.currentMB
            << "," << r_proc_mem.peakCurrentMB
            << "," << r_proc_mem.virtualMB
            << "," << r_proc_mem.peakVirtualMB
            << "," << r_sys_mem.ramFreeMB
            << "," << r_sys_mem.ramTotalMB
            << "," << r_sys_mem.virtualFreeMB
            << "," << r_sys_mem.virtualTotalMB
            << "," << m_NumNodes
            << "," << m_NumPeople
            << "," << m_NumInterventionsPersisted
            << "," << m_NumInterventionsAdded
            << "," << m_NumInfections
            << "," << m_NumEventsTriggered
            << "," << m_NumEventsObserved
            << GetOtherData()
            << endl;

        GetDataFromOtherCores();
        if( EnvPtr->MPI.Rank == 0 )
        {
            // add the total simulation duration onto the beginning of each line
            IdmString data = reduced_stream.str();
            std::vector<IdmString> line_list = data.split( '\n' );
            std::stringstream ss;
            for( auto line : line_list )
            {
                if( !line.empty() )
                {
                    ss << sim_duration << "," << line << endl;
                }
            }
            WriteData( ss.str() );
            reduced_stream.str( std::string() ); // clear stream
        }

        ClearData();
    }

    void ReportSimulationStats::ClearData()
    {
        m_NumNodes = 0;
        m_NumPeople = 0;
        m_NumInterventionsPersisted = 0;
        m_NumInterventionsAdded = 0;
        m_NumInfections = 0;
        m_NumEventsTriggered = 0;
        m_NumEventsObserved = 0;
    }

    std::string ReportSimulationStats::GetOtherData()
    {
        return "";
    }
}
