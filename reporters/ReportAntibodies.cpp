#include "stdafx.h"
#include "ReportAntibodies.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "MalariaContexts.h"
#include "SusceptibilityMalaria.h"
#include "SimulationEventContext.h"


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
        : ReportAntibodies( "ReportAntibodiesConcentration.csv" )
    {
    }

    ReportAntibodies::ReportAntibodies( const std::string& rReportName )
        : BaseTextReport( rReportName, true )
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_ReportingInterval( 1.0f )
        , m_InfectedOnly( false )
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
        initConfigTypeMap( "Contain_Capacity_Data", &m_IsCapacityData,    RA_Contain_Capacity_Data_DESC_TEXT, false );
        initConfigTypeMap( "Reporting_Interval",    &m_ReportingInterval, RA_Reporting_Interval_DESC_TEXT, 1.0, 1000000.0, 1.0 );
        initConfigTypeMap( "Infected_Only",         &m_InfectedOnly,      RA_Infected_Only_DESC_TEXT, false );

        m_ReportFilter.ConfigureParameters( *this, inputJson );

        bool ret = JsonConfigurable::Configure( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );
            m_NextDayToCollectData = m_ReportFilter.GetStartDay();
            if (m_IsCapacityData)
            {
                report_name = "ReportAntibodiesCapacity.csv";
            }
            report_name = m_ReportFilter.GetNewReportName( report_name );
        }
        return ret;
    }

    void ReportAntibodies::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
        BaseTextReport::Initialize( nrmSize );
    }

    void ReportAntibodies::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( report_name, demographicNodeIds );
    }

    std::string ReportAntibodies::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"
               << ",NodeID"
               << ",IndividualID"
               << ",Gender"
               << ",AgeYears"
               << ",IsInfected"
               << ",PyrogenicThreshold"
               << ",FeverKillingRate";

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
        bool isValidTime = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
        BaseTextReport::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
        m_IsCollectingData = (currentTime >= m_NextDayToCollectData) && isValidTime;
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
        if(!m_IsCollectingData) return;
        if(!m_ReportFilter.IsValidHuman( individual )) return;
        INodeEventContext* p_nec = individual->GetEventContext()->GetNodeEventContext();
        if(!m_ReportFilter.IsValidNode( p_nec )) return;

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != individual->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "individual", "IMalariaHumanContext", "IIndividualHuman");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();
        bool is_infected = individual->IsInfected();

        // check that individual has any antibodies at all
        if(!is_infected)
        {
            // skip this person if we only want infected or if no antibodies present
            // MSP1 antibodies are always generated first, so checking them is sufficient
            if(m_InfectedOnly || susceptibility_malaria->get_fraction_of_variants_with_antibodies( MalariaAntibodyType::MSP1 ) == 0.0f) return;
        }

        float current_time = p_nec->GetTime().time;
        float dt = 1.0; // malaria simulation dt=1
        uint32_t node_id = p_nec->GetExternalId();
        uint32_t ind_id = individual->GetSuid().data;
        const char* gender = ( individual->GetGender() == Gender::FEMALE ) ? "F" : "M";
        float age_years = individual->GetAge() / DAYSPERYEAR;
        float pyrogenic_threshold = susceptibility_malaria->get_pyrogenic_threshold();
        float fever_killing_rate  = susceptibility_malaria->get_fever_killing_rate();

        GetOutputStream()
            << current_time
            << "," << node_id
            << "," << ind_id
            << "," << gender
            << "," << age_years
            << "," << is_infected
            << "," << pyrogenic_threshold
            << "," << fever_killing_rate;

        std::vector<MalariaAntibody> r_antibodies; // comes back sorted by variant value
        // log MSP1 antibodies
        susceptibility_malaria->GetAntibodiesForReporting( r_antibodies, current_time, dt, MalariaAntibodyType::MSP1 );
        LogAntibodyData( r_antibodies, SusceptibilityMalariaConfig::falciparumMSPVars );

        // log PfEMP1_major antibodies
        susceptibility_malaria->GetAntibodiesForReporting( r_antibodies, current_time, dt, MalariaAntibodyType::PfEMP1_major );
        LogAntibodyData( r_antibodies, SusceptibilityMalariaConfig::falciparumPfEMP1Vars );

        GetOutputStream() << endl;
    }

    void ReportAntibodies::LogAntibodyData( const std::vector<MalariaAntibody>& r_antibodies, int num_variants )
    {
        int antibody_index = 0; // works because r_antibodies is sorted by variant value
        for(int i = 0; i < num_variants; ++i) 
        {
            GetOutputStream() << ",";
            if(antibody_index < r_antibodies.size() && r_antibodies[antibody_index].GetAntibodyVariant() == i)
            {
                if(m_IsCapacityData)
                    GetOutputStream() << r_antibodies[antibody_index].GetAntibodyCapacity();
                else
                    GetOutputStream() << r_antibodies[antibody_index].GetAntibodyConcentration();
                antibody_index++;
            }
        }
    }
}
