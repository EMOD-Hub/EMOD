/***************************************************************************************************

Copyright (c) 2021 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"
#include "ReportAntibodies.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "MalariaContexts.h"
#include "SusceptibilityMalaria.h"

SETUP_LOGGING( "ReportAntibodies" )

namespace Kernel
{
    // ----------------------------------------
    // --- ReportAntibodies Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportAntibodies, BaseTextReport )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportAntibodies, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportAntibodies )

    ReportAntibodies::ReportAntibodies()
        : ReportAntibodies( "ReportAntibodies.csv" )
    {
    }

    ReportAntibodies::ReportAntibodies( const std::string& rReportName )
        : BaseTextReport( rReportName, true )
        , m_StartDay( 0.0f )
        , m_EndDay( FLT_MAX )
        , m_ReportingInterval( 1.0f )
        , m_IsCapacityData( false )
        , m_NextDayToCollectData( 0.0f )
        , m_IsCollectingData( false )
    {
        initSimTypes( 1, "MALARIA_SIM" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportAntibodies::~ReportAntibodies()
    {
    }

    bool ReportAntibodies::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Day",             &m_StartDay,       Report_Start_Day_DESC_TEXT, 0.0, FLT_MAX, 0.0     );
        initConfigTypeMap( "End_Day",               &m_EndDay,         Report_End_Day_DESC_TEXT,   0.0, FLT_MAX, FLT_MAX );
        initConfigTypeMap( "Contain_Capacity_Data", &m_IsCapacityData, Contain_Capacity_Data_DESC_TEXT, false );

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

    void ReportAntibodies::Initialize( unsigned int nrmSize )
    {
        BaseTextReport::Initialize( nrmSize );
    }

    std::string ReportAntibodies::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"
               << ",NodeID"
               << ",IndividualID"
               << ",Gender"
               << ",AgeYears"
               << ",IsInfected";

        for( int i = 0; i < SusceptibilityMalariaConfig::falciparumMSPVars; ++i )
        {
            header << ",MSP_" << i;
        }
        for( int i = 0; i < SusceptibilityMalariaConfig::falciparumPfEMP1Vars; ++i )
        {
            header << ",PfEMP1_" << i;
        }

        return header.str();
    }

    void ReportAntibodies::UpdateEventRegistration( float currentTime,
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

    bool ReportAntibodies::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsCollectingData;
    }

    void ReportAntibodies::LogIndividualData( IIndividualHuman* individual ) 
    {
        INodeEventContext* p_nec = individual->GetEventContext()->GetNodeEventContext();
        float current_time = p_nec->GetTime().time;
        float dt = 1.0;
        uint32_t node_id = p_nec->GetExternalId();
        uint32_t ind_id = individual->GetSuid().data;
        const char* gender = (individual->GetGender() == Gender::FEMALE) ? "F" : "M";
        float age_years = individual->GetAge() / DAYSPERYEAR;
        bool is_infected = individual->IsInfected();

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != individual->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "individual", "IMalariaHumanContext", "IIndividualHuman");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        GetOutputStream()
            << current_time
            << "," << node_id
            << "," << ind_id
            << "," << gender
            << "," << age_years
            << "," << is_infected;

        const std::vector<MalariaAntibody>& r_msp_antibodies = susceptibility_malaria->GetMSPAntibodies( current_time, dt );
        for( auto& r_msp_antibody : r_msp_antibodies )
        {
            if( m_IsCapacityData )
                GetOutputStream() << "," << r_msp_antibody.GetAntibodyCapacity();
            else
                GetOutputStream() << "," << r_msp_antibody.GetAntibodyConcentration();
        }

        const std::vector<MalariaAntibody>& r_pfemp1_antibodies = susceptibility_malaria->GetPfEMP1MajorAntibodies( current_time, dt );
        for( auto& r_pfemp1_antibody : r_pfemp1_antibodies )
        {
            if( m_IsCapacityData )
                GetOutputStream() << "," << r_pfemp1_antibody.GetAntibodyCapacity();
            else
                GetOutputStream() << "," << r_pfemp1_antibody.GetAntibodyConcentration();
        }
        GetOutputStream() << endl;
    }
}