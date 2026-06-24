

#include "stdafx.h"
#include "ReportInfectionStatsMalaria.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "MalariaContexts.h"

SETUP_LOGGING( "ReportInfectionStatsMalaria" )

namespace Kernel
{
    // ----------------------------------------
    // --- ReportInfectionStatsMalaria Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportInfectionStatsMalaria, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportInfectionStatsMalaria, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportInfectionStatsMalaria )

    ReportInfectionStatsMalaria::ReportInfectionStatsMalaria()
        : ReportInfectionStatsMalaria( "ReportInfectionStatsMalaria.csv" )
    {
    }

    ReportInfectionStatsMalaria::ReportInfectionStatsMalaria( const std::string& rReportName )
        : BaseTextReport( rReportName, true )
        , m_StartDay( 0.0f )
        , m_EndDay( FLT_MAX )
        , m_ReportingInterval( 1.0f )
        , m_NextDayToCollectData( 0.0f )
        , m_IsCollectingData( false )
        , m_IncludeColumnHepatocytes( true )
        , m_IncludeColumnIRBC( true )
        , m_IncludeColumnGametocytes( true )
        , m_ThresholdHepatocytes( 0.0 )
        , m_ThresholdIRBC( 0.0 )
        , m_ThresholdGametocytes( 0.0 )
    {
        initSimTypes( 1, "MALARIA_SIM" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportInfectionStatsMalaria::~ReportInfectionStatsMalaria()
    {
    }

    bool ReportInfectionStatsMalaria::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Day",  &m_StartDay, Report_Start_Day_DESC_TEXT, 0.0, FLT_MAX, 0.0     );
        initConfigTypeMap( "End_Day",    &m_EndDay,   Report_End_Day_DESC_TEXT,   0.0, FLT_MAX, FLT_MAX );

        initConfigTypeMap( "Include_Column_Hepatocyte",  &m_IncludeColumnHepatocytes, RISM_Include_Column_Hepatocyte_DESC_TEXT, true );
        initConfigTypeMap( "Include_Column_IRBC",        &m_IncludeColumnIRBC,        RISM_Include_Column_IRBC_DESC_TEXT,       true );
        initConfigTypeMap( "Include_Column_Gametocyte",  &m_IncludeColumnGametocytes, RISM_Include_Column_Gametocyte_DESC_TEXT, true );

        initConfigTypeMap( "Include_Data_Threshold_Hepatocytes",  &m_ThresholdHepatocytes, RISM_Include_Data_Threshold_Hepatocytes_DESC_TEXT, 0.0, FLT_MAX, 0.0     );
        initConfigTypeMap( "Include_Data_Threshold_IRBC",         &m_ThresholdIRBC,        RISM_Include_Data_Threshold_IRBC_DESC_TEXT,        0.0, FLT_MAX, 0.0     );
        initConfigTypeMap( "Include_Data_Threshold_Gametocytes",  &m_ThresholdGametocytes, RISM_Include_Data_Threshold_Gametocytes_DESC_TEXT, 0.0, FLT_MAX, 0.0     );

        initConfigTypeMap( "Reporting_Interval", &m_ReportingInterval, Report_Reporting_Interval_DESC_TEXT, 1.0, 1000000.0, 1.0 );

        bool ret = JsonConfigurable::Configure( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_StartDay >= m_EndDay )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Start_Day", m_StartDay, "End_Day", m_EndDay );
            }
            m_NextDayToCollectData = m_StartDay;
        }
        return ret;
    }

    void ReportInfectionStatsMalaria::Initialize( unsigned int nrmSize )
    {
        BaseTextReport::Initialize( nrmSize );
    }

    std::string ReportInfectionStatsMalaria::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"
               << ",NodeID"
               << ",IndividualID"
               << ",Gender"
               << ",AgeYears"
               << ",InfectionID"
               << ",Infectiousness"
               << ",Duration";

        if( m_IncludeColumnHepatocytes )
        {
            header << ",Hepatocytes";
        }
        if( m_IncludeColumnIRBC )
        {
            header << ",IRBCs";
        }
        if( m_IncludeColumnGametocytes )
        {
            header << ",Gametocytes";
        }

        return header.str();
    }

    void ReportInfectionStatsMalaria::UpdateEventRegistration( float currentTime,
                                                               float dt,
                                                               std::vector<INodeEventContext*>& rNodeEventContextList,
                                                               ISimulationEventContext* pSimEventContext )
    {
        BaseTextReport::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );

        m_IsCollectingData = (currentTime >= m_NextDayToCollectData)
                           && (m_StartDay <= currentTime) && (currentTime <= m_EndDay);
        if( m_IsCollectingData )
        {
            m_NextDayToCollectData += m_ReportingInterval;
        }
    }

    bool ReportInfectionStatsMalaria::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsCollectingData;
    }

    void ReportInfectionStatsMalaria::LogIndividualData( IIndividualHuman* individual ) 
    {
        INodeEventContext* p_nec = individual->GetEventContext()->GetNodeEventContext();
        float time = p_nec->GetTime().time;
        uint32_t node_id = p_nec->GetExternalId();
        uint32_t ind_id = individual->GetSuid().data;
        const char* gender = (individual->GetGender() == Gender::FEMALE) ? "F" : "M";
        float age_years = individual->GetAge() / DAYSPERYEAR;
        bool is_infected = individual->IsInfected();
        float infectiousness = individual->GetInfectiousness();

        for( auto p_inf : individual->GetInfections() )
        {
            const IInfectionMalaria* p_inf_malaria = nullptr;
            if( p_inf->QueryInterface( GET_IID( IInfectionMalaria ), (void**)&p_inf_malaria ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "p_inf", "IInfectionMalaria", "IInfection" );
            }

            int64_t num_hepatocytes = p_inf_malaria->get_hepatocytes();
            int64_t num_irbc        = p_inf_malaria->get_irbc();
            int64_t num_gametocytes = p_inf_malaria->get_FemaleGametocytes( GametocyteStages::Mature )
                                    + p_inf_malaria->get_MaleGametocytes( GametocyteStages::Mature );

            bool write_line = true;
            if( m_IncludeColumnHepatocytes && (num_hepatocytes < m_ThresholdHepatocytes) )
            {
                write_line = false;
            }
            if( m_IncludeColumnIRBC && (num_irbc < m_ThresholdIRBC) )
            {
                write_line = false;
            }
            if( m_IncludeColumnGametocytes && (num_gametocytes < m_ThresholdGametocytes) )
            {
                write_line = false;
            }
            if( !write_line ) continue;

            GetOutputStream()
                <<        time
                << "," << node_id
                << "," << ind_id
                << "," << gender
                << "," << age_years
                << "," << p_inf->GetSuid().data
                << "," << infectiousness
                << "," << p_inf->GetDuration();

            if( m_IncludeColumnHepatocytes )
            {
                GetOutputStream() << "," << num_hepatocytes;
            }
            if( m_IncludeColumnIRBC )
            {
                GetOutputStream() << "," << num_irbc;
            }
            if( m_IncludeColumnGametocytes )
            {
                GetOutputStream() << "," << num_gametocytes;
            }
            GetOutputStream() << endl;
        }
    }
}