
#include "stdafx.h"

#include "ReportSimpleMalariaTransmission.h"

#include "report_params.rc"
#include "FileSystem.h"
#include "Exceptions.h"
#include "NodeEventContext.h"
#include "IdmDateTime.h"
#include "StrainIdentityMalariaCoTran.h"
#include "MalariaCoTransmissionContexts.h"
#include "ReportUtilities.h"
#include "IIndividualHuman.h"
#include "IInfection.h"
#include "MalariaContexts.h"
#include "SimulationEventContext.h"

SETUP_LOGGING("ReportSimpleMalariaTransmission")

// These need to be after SETUP_LOGGING so that the LOG messages in the
// templates don't make GCC complain.
// I'm also seeing a "undefined reference" from the Linux linker
#include "BaseTextReportEventsTemplate.h"

namespace Kernel
{
    // ----------------------------------------
    // --- ReportSimpleMalariaTransmission Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportSimpleMalariaTransmission, BaseTextReportEvents )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportSimpleMalariaTransmission, BaseTextReportEvents )

    IMPLEMENT_FACTORY_REGISTERED( ReportSimpleMalariaTransmission )

    ReportSimpleMalariaTransmission::ReportSimpleMalariaTransmission() 
        : BaseTextReportEvents( "ReportSimpleMalariaTransmission.csv" )
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_OutputWritten( false )
        , m_DetectionThresholds()
    {
        initSimTypes( 1, "MALARIA_SIM" );

        eventTriggerList.push_back( EventTrigger::VectorToHumanTransmission );
    }

    ReportSimpleMalariaTransmission::~ReportSimpleMalariaTransmission()
    {
    }

    bool ReportSimpleMalariaTransmission::Configure( const Configuration* inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        bool include_human_to_vector_transmission = false;
        initConfigTypeMap( "Include_Human_To_Vector_Transmission", &include_human_to_vector_transmission, CoTran_Include_Human_To_Vector_Transmission_DESC_TEXT, false );

        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_CO_TRANSMISSION" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'ReportSimpleMalariaTransmission' can only be used with 'MALARIA_MECHANISTIC_MODEL_WITH_CO_TRANSMISSION'.");
            }

            m_ReportFilter.CheckParameters( inputJson );

            if( include_human_to_vector_transmission )
            {
                eventTriggerList.push_back( EventTrigger::HumanToVectorTransmission );
            }
        }
        return configured;
    }

    void ReportSimpleMalariaTransmission::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
        BaseTextReportEvents::Initialize( nrmSize );
    }

    void ReportSimpleMalariaTransmission::CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& nodeIds_demographics)
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), nodeIds_demographics );
        BaseTextReportEvents::CheckForValidNodeIDs( nodeIds_demographics );
    }

    void ReportSimpleMalariaTransmission::UpdateEventRegistration( float currentTime,
                                                                   float dt,
                                                                   std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                   ISimulationEventContext* pSimEventContext )
    {
        bool is_valid_time = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );

        if( !is_registered && is_valid_time )
        {
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
        }
        else if( is_registered && !is_valid_time )
        {
            // unregestering is not in base class
            for( auto pNEC : rNodeEventContextList )
            {
                if( IsValidNode( pNEC ) )
                {
                    IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                    UpdateRegistration( broadcaster, false );
                }
            }
            is_registered = false;
        }
    }

    bool ReportSimpleMalariaTransmission::IsValidNode( INodeEventContext* pNEC ) const
    {
        return m_ReportFilter.IsValidNode( pNEC );
    }

    void ReportSimpleMalariaTransmission::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
        m_DetectionThresholds = rDetectionThresholds;
        release_assert( m_DetectionThresholds.size() == MalariaDiagnosticType::pairs::count() );
    }

    std::string ReportSimpleMalariaTransmission::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( report_name );
    }

    std::string ReportSimpleMalariaTransmission::GetHeader() const
    {
        std::stringstream header ;

        header
            << "node_id"
            << ",acquireTime"
            << ",acquireIndividualId"
            << ",acquireIndividualAgeDays"
            << ",acquireIndividualHasFever"
            << ",acquireInfectionIds"
            << ",concurrentInfectionIds"
            << ",vectorId"
            << ",transmitTime"
            << ",transmitIndividualId"
            << ",transmitInfectionIds"
            << ",transmitGametocyteDensities";

        return header.str();
    }

    template <typename T>
    std::string ConvertVector( const std::vector<T>& rVector )
    {
        std::stringstream text_stream;
        text_stream << "\"";

        for( int i = 0; i < rVector.size(); ++i )
        {
            text_stream << rVector[ i ];
            if( (i + 1) < rVector.size() )
            {
                text_stream << ",";
            }
        }

        text_stream << "\"";
        return text_stream.str();
    }

    bool ReportSimpleMalariaTransmission::notifyOnEvent( IIndividualHumanEventContext *context,
                                                             const EventTrigger& trigger )
    {
        // --------------------------------------------------------------------------------
        // --- NOTE: We check that GetIndividualHumanConst() is not nullptr because if this
        // --- "context" does return nullptr, then the context is really a vector getting
        // --- infected by a human.
        // --------------------------------------------------------------------------------
        if( !is_registered ||
            ( (context->GetIndividualHumanConst() != nullptr) &&
              !m_ReportFilter.IsValidHuman( context->GetIndividualHumanConst() ) ) )
        {
            return false;
        }

        IMalariaHumanReport* p_human_report = nullptr;
        if( context->QueryInterface(GET_IID(IMalariaHumanReport), (void **)&p_human_report) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "IMalariaHumanReport", "IIndividualHumanEventContext" );
        }
        const StrainIdentityMalariaCoTran& r_si_malaria = p_human_report->GetRecentTransmissionInfo();

        ExternalNodeId_t node_id = context->GetNodeEventContext()->GetExternalId();
        float time = context->GetNodeEventContext()->GetTime().time;

        uint32_t tran_id = r_si_malaria.GetTransmittedPersonID();
        uint32_t acq_id  = r_si_malaria.GetAcquiredPersonID();

        std::vector<uint32_t> tran_inf_ids;
        std::vector<float> tran_density;
        for( auto info : r_si_malaria.GetTransmittedInfections() )
        {
            tran_inf_ids.push_back( info.infection_id );
            tran_density.push_back( info.gametocyte_density );
        }

        std::vector<uint32_t> acq_inf_ids;
        for( auto info : r_si_malaria.GetAcquiredInfections() )
        {
            acq_inf_ids.push_back( info.infection_id );
        }

        float acq_age_days = 0.0;
        bool acq_has_fever = false;
        uint32_t vec_id = 0;
        float time_of_vector_infection = time;
        std::vector<uint32_t> con_inf_ids;
        if( trigger == EventTrigger::VectorToHumanTransmission )
        {
            const IIndividualHuman* p_human = context->GetIndividualHumanConst();

            for( auto info : p_human->GetInfections() )
            {
                if( std::find( acq_inf_ids.begin(), acq_inf_ids.end(), info->GetSuid().data ) == acq_inf_ids.end() )
                {
                    con_inf_ids.push_back( info->GetSuid().data );
                }
            }

            vec_id = r_si_malaria.GetVectorID();
            time_of_vector_infection = r_si_malaria.GetTimeOfVectorInfection();

            IMalariaHumanContext* p_human_malaria = nullptr;
            if( context->QueryInterface( GET_IID( IMalariaHumanContext ), (void **)&p_human_malaria ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "context", "IMalariaHumanContext", "IIndividualHumanEventContext" );
            }
            IMalariaSusceptibility* p_suscept_malaria = p_human_malaria->GetMalariaSusceptibilityContext();
            release_assert( acq_id == p_human->GetSuid().data );
            acq_age_days = p_human->GetAge();
            acq_has_fever = (p_suscept_malaria->get_fever() > m_DetectionThresholds[ MalariaDiagnosticType::FEVER ]);
        }
        else
        {
            vec_id = context->GetSuid().data;
        }


        GetOutputStream() 
                   << node_id
            << "," << time
            << "," << acq_id
            << "," << acq_age_days
            << "," << acq_has_fever
            << "," << ConvertVector( acq_inf_ids )
            << "," << ConvertVector( con_inf_ids )
            << "," << vec_id
            << "," << time_of_vector_infection
            << "," << tran_id
            << "," << ConvertVector( tran_inf_ids )
            << "," << ConvertVector( tran_density)
            << "\n";

        return true;
    }
}