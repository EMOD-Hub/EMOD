

#include "stdafx.h"
#include "ReportInfectionDuration.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "ReportInfectionDuration" )

namespace Kernel
{
    // ----------------------------------------
    // --- ReportInfectionDuration Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportInfectionDuration, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportInfectionDuration, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportInfectionDuration )

    ReportInfectionDuration::ReportInfectionDuration()
        : BaseTextReport( "ReportInfectionDuration.csv" )
        , m_IsRegistered( false )
        , m_StartDay( 0.0f )
        , m_EndDay( FLT_MAX )
    {
        initSimTypes( 1, "*" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportInfectionDuration::~ReportInfectionDuration()
    {
    }

    bool ReportInfectionDuration::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Day",  &m_StartDay, Report_Start_Day_DESC_TEXT, 0.0, FLT_MAX, 0.0     );
        initConfigTypeMap( "End_Day",    &m_EndDay,   Report_End_Day_DESC_TEXT,   0.0, FLT_MAX, FLT_MAX );

        bool ret = JsonConfigurable::Configure( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_StartDay >= m_EndDay )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Start_Day", m_StartDay, "End_Day", m_EndDay );
            }
        }
        return ret;
    }

    void ReportInfectionDuration::Initialize( unsigned int nrmSize )
    {
        BaseTextReport::Initialize( nrmSize );
    }

    std::string ReportInfectionDuration::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"
               << ",NodeID"
               << ",IndividualID"
               << ",Gender"
               << ",AgeYears"
               << ",InfectionDuration";

        return header.str();
    }

    bool ReportInfectionDuration::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return false ;
    }

    void ReportInfectionDuration::UpdateEventRegistration( float currentTime,
                                                           float dt,
                                                           std::vector<INodeEventContext*>& rNodeEventContextList,
                                                           ISimulationEventContext* pSimEventContext )
    {
        if( (m_StartDay <= currentTime) && (currentTime <= m_EndDay) && !m_IsRegistered )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                release_assert( pNEC );
                IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                broadcaster->RegisterObserver( this, EventTrigger::InfectionCleared );
            }
            m_IsRegistered = true;
        }
    }

    bool ReportInfectionDuration::notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger )
    {
        const IIndividualHuman* individual = pEntity->GetIndividualHumanConst();

        INodeEventContext* p_nec = pEntity->GetNodeEventContext();
        float time = p_nec->GetTime().time;

        if( (m_StartDay <= time) && (time <= m_EndDay) && (trigger == EventTrigger::InfectionCleared) )
        {
            uint32_t node_id = p_nec->GetExternalId();
            uint32_t ind_id = individual->GetSuid().data;
            const char* gender = (individual->GetGender() == Gender::FEMALE) ? "F" : "M";
            float age_years = individual->GetAge() / DAYSPERYEAR;

            bool found = false;
            for( auto p_inf : individual->GetInfections() )
            {
                if( p_inf->GetStateChange() == InfectionStateChange::Cleared )
                {
                    found = true;

                    GetOutputStream()
                        << time
                        << "," << node_id
                        << "," << ind_id
                        << "," << gender
                        << "," << age_years
                        << "," << p_inf->GetDuration()
                        << endl;
                    break;
                }
            }
            release_assert( found );
        }
        return true;
    }
}