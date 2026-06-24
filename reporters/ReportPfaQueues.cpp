
#include "stdafx.h"

#include "ReportPfaQueues.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "Individual.h"
#include "ReportUtilities.h"
#include "INodeContext.h"
#include "INodeSTI.h"
#include "ISociety.h"
#include "IPairFormationAgent.h"
#include "IdmDateTime.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationFlowController.h"
#include "SocietyImpl.h"
#include "FlowControllerImpl.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportPfaQueues" ) // <<< Name of this file

namespace Kernel
{
    // ----------------------------------------
    // --- ReportPfaQueues Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportPfaQueues, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportPfaQueues, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportPfaQueues )

    ReportPfaQueues::ReportPfaQueues()
        : ReportPfaQueues( "ReportPfaQueues.csv" )
    {
    }

    ReportPfaQueues::ReportPfaQueues( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_AddedHeader( false )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );

        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportPfaQueues::~ReportPfaQueues()
    {
    }

    bool ReportPfaQueues::Configure( const Configuration * inputJson )
    {
        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
        }
        return ret;
    }

    std::string ReportPfaQueues::GetHeader() const
    {
        return "";
    }


    std::string ReportPfaQueues::GetHeader( ISociety* pSociety ) const
    {
        std::stringstream header ;

        header         << "Time"
               << "," << "NodeID";

        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            const char* p_rel_type = RelationshipType::pairs::get_keys()[ i ];
            IPairFormationAgent* p_pfa = pSociety->GetPFA( RelationshipType::Enum( i ) );

            const std::map<int, std::vector<float>>& r_age_bins_by_gender = p_pfa->GetAgeBins();

            for( int g = 0; g < Gender::COUNT; ++g )
            {
                const char* p_gender = (g == Gender::MALE) ? "M" : "F";
                const std::vector<float>& r_age_bins = r_age_bins_by_gender.at( g );
                for( float age : r_age_bins )
                {
                    header << "," << p_rel_type << "_" << p_gender << "_Before_" << age ;
                }
                for( float age : r_age_bins )
                {
                    header << "," << p_rel_type << "_" << p_gender << "_After_" << age ;
                }
            }
        }

        for( int rel_t = 0; rel_t < RelationshipType::COUNT; ++rel_t )
        {
            for( int risk_gr = 0; risk_gr < RiskGroup::COUNT; ++risk_gr )
            {
                const char* p_rel_type = RelationshipType::pairs::get_keys()[rel_t];
                const char* p_risk_group_type = RiskGroup::pairs::get_keys()[risk_gr];
                IPairFormationAgent* p_pfa = pSociety->GetPFA( RelationshipType::Enum( rel_t ) );
                const std::map<int, std::vector<float>>& r_age_bins_by_gender = p_pfa->GetAgeBins();

                for( int g = 0; g < Gender::COUNT; ++g )
                {
                    const char* p_gender = ( g == Gender::MALE ) ? "M" : "F";
                    const std::vector<float>& r_age_bins = r_age_bins_by_gender.at( g );
                    
                    for( float age : r_age_bins )
                    {
                        header << "," << p_rel_type << "_" <<p_risk_group_type <<"_" << p_gender << "_" << age;
                    }                
                }
            }
        }

        for( int rel_t = 0; rel_t < RelationshipType::COUNT; ++rel_t )
        {
            const char* p_rel_type = RelationshipType::pairs::get_keys()[rel_t];
            IPairFormationAgent* p_pfa = pSociety->GetPFA( RelationshipType::Enum( rel_t ) );
            const std::map<int, std::vector<float>>& r_age_bins_by_gender = p_pfa->GetAgeBins();

            for( int g = 0; g < Gender::COUNT; ++g )
            {
                const char* p_gender = ( g == Gender::MALE ) ? "M" : "F";
                const std::vector<float>& r_age_bins = r_age_bins_by_gender.at( g );

                for( float age : r_age_bins )
                {
                    header << "," << "desired_flow" << "_"<< p_rel_type << "_" << p_gender << "_" << age;
                }
            }
        }


        return header.str();
    }

    void ReportPfaQueues::LogNodeData( Kernel::INodeContext* pNC )
    {
        auto time      = pNC->GetTime().time ;
        auto nodeId    = pNC->GetExternalID();
        
        INodeSTI * p_node_sti = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeSTI), (void**)&p_node_sti) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeSTI", "INodeContext");
        }
        ISociety* p_society = p_node_sti->GetSociety();

        if( !m_AddedHeader )
        {
            GetOutputStream() << GetHeader( p_society ) << "\n";
            m_AddedHeader = true;
        }

        GetOutputStream()
            << time
            << "," << nodeId;

        // pfa queues
        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            IPairFormationAgent* p_pfa = p_society->GetPFA( RelationshipType::Enum( i ) );
            const std::map<int, std::vector<int>>& r_before = p_pfa->GetQueueLengthsBefore();
            const std::map<int, std::vector<int>>& r_after  = p_pfa->GetQueueLengthsAfter();

            for( int i = 0; i < Gender::COUNT; ++i )
            {
                for( int num : r_before.at( i ) )
                {
                    GetOutputStream() << "," << num;
                }
                for( int num : r_after.at( i ) )
                {
                    GetOutputStream() << "," << num;
                }
            }
        }

        // rates for moving individuals into queue
        for( int rel_t = 0; rel_t < RelationshipType::COUNT; ++rel_t )
        {
            for( int risk_gr = 0; risk_gr < RiskGroup::COUNT; ++risk_gr )
            {
                IPairFormationAgent* p_pfa = p_society->GetPFA( RelationshipType::Enum( rel_t ) );
                const std::map<int, std::vector<float>>& r_age_bins_by_gender = p_pfa->GetAgeBins();
                const IPairFormationRateTable* rate_table = p_society->GetRates( RelationshipType::Enum( rel_t ) );

                for( int g = 0; g < Gender::COUNT; ++g )
                {
                    const char* p_gender = ( g == Gender::MALE ) ? "M" : "F";
                    const std::vector<float>& r_age_bins = r_age_bins_by_gender.at( g );

                    for( float age : r_age_bins )
                    {
                        float formation_rate = rate_table->GetRateForAgeAndSexAndRiskGroup( age, g, Kernel::RiskGroup::Enum( risk_gr ) );
                        GetOutputStream() << "," << formation_rate;
                    }
                }
            }
        }

        // desired flow
        for( int rel_t = 0; rel_t < RelationshipType::COUNT; ++rel_t )
        {
            IPairFormationAgent* p_pfa = p_society->GetPFA( RelationshipType::Enum( rel_t ) );
            const std::map<int, std::vector<float>>& r_age_bins_by_gender = p_pfa->GetAgeBins();
            IPairFormationFlowController* r_desired_flow = p_society->GetController( RelationshipType::Enum( rel_t ) );

            for( int g = 0; g < Gender::COUNT; ++g )
            {
                const char* p_gender = ( g == Gender::MALE ) ? "M" : "F";
                const std::vector<float>& desired = r_desired_flow->GetDesiredFlow().at( g );

                for( int bin_index = 0; bin_index < desired.size(); bin_index++ )
                {
                    float d = desired[bin_index];
                    GetOutputStream() << "," << d;
                }
            }
        }

        GetOutputStream() << "\n";
    }
}