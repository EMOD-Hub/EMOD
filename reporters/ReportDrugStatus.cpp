

#include "stdafx.h"

#include "ReportDrugStatus.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "IDrug.h"

SETUP_LOGGING( "ReportDrugStatus" )

namespace Kernel
{
    // ----------------------------------------
    // --- ReportDrugStatus Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportDrugStatus, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportDrugStatus, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportDrugStatus )

    ReportDrugStatus::ReportDrugStatus()
        : ReportDrugStatus( "ReportDrugStatus.csv" )
    {
    }

    ReportDrugStatus::ReportDrugStatus( const std::string& rReportName )
        : BaseTextReport( rReportName )
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

    ReportDrugStatus::~ReportDrugStatus()
    {
    }

    bool ReportDrugStatus::Configure( const Configuration * inputJson )
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

    void ReportDrugStatus::Initialize( unsigned int nrmSize )
    {
        BaseTextReport::Initialize( nrmSize );
    }

    std::string ReportDrugStatus::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"
               << ",NodeID"
               << ",IndividualID"
               << ",Gender"
               << ",AgeYears"
               << ",Infected"
               << ",Infectiousness"
               << ",DrugName"
               << ",CurrentEfficacy"
               //<< ",CurrentConcentration" users might want as individal drugs, not combo
               << ",NumRemainingDoses";

        return header.str();
    }

    bool ReportDrugStatus::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return (m_StartDay <= currentTime) && (currentTime <= m_EndDay) ;
    }

    void ReportDrugStatus::LogIndividualData( IIndividualHuman* individual ) 
    {
        INodeEventContext* p_nec = individual->GetEventContext()->GetNodeEventContext();
        float time = p_nec->GetTime().time;
        uint32_t node_id = p_nec->GetExternalId();
        uint32_t ind_id = individual->GetSuid().data;
        const char* gender = (individual->GetGender() == Gender::FEMALE) ? "F" : "M";
        float age_years = individual->GetAge() / DAYSPERYEAR;
        bool is_infected = individual->IsInfected();
        float infectiousness = individual->GetInfectiousness();

        std::list<void*> drug_list = individual->GetInterventionsContext()->GetInterventionsByInterface( GET_IID(IDrug) );
        for( void* p_void_drug : drug_list )
        {
            IDrug* p_drug = static_cast<IDrug*>(p_void_drug);

            GetOutputStream()
                << time
                << "," << node_id
                << "," << ind_id
                << "," << gender
                << "," << age_years
                << "," << is_infected
                << "," << infectiousness
                << "," << p_drug->GetDrugName()
                << "," << p_drug->GetDrugCurrentEfficacy()
                //<< "," << p_drug->GetDrugCurrentConcentration() users might want this per drug and not as part of combo drug
                << "," << p_drug->GetNumRemainingDoses()
                << endl;
        }
    }
}