
#include "stdafx.h"

#include "ReportNodeDemographics.h"

#include "report_params.rc"
#include "FactorySupport.h"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "ReportUtilities.h"
#include "Properties.h"
#include "NodeProperties.h"
#include "IdmDateTime.h"
#include "INodeContext.h"

SETUP_LOGGING( "ReportNodeDemographics" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( ReportNodeDemographics, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportNodeDemographics, BaseTextReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportNodeDemographics )
#endif

    ReportNodeDemographics::ReportNodeDemographics()
        : ReportNodeDemographics( "ReportNodeDemographics.csv" )
    {
    }

    ReportNodeDemographics::ReportNodeDemographics( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_StratifyByGender(true)
        , m_StratifyByAge(true)
        , m_AgeYears()
        , m_IPKeyToCollect()
        , m_IPValuesList()
        , m_Data()
    {
        initSimTypes( 1, "*" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportNodeDemographics::~ReportNodeDemographics()
    {
        for( int i = 0; i < m_Data.size(); ++i )
        {
            for( int j = 0; j < m_Data[ i ].size(); ++j )
            {
                for( int k = 0; k < m_Data[ i ][ j ].size(); ++k )
                {
                    for( int m = 0; m < m_Data[ i ][ j ].size(); ++m )
                    {
                        delete m_Data[ i ][ j ][ k ][ m ];
                        m_Data[ i ][ j ][ k ][ m ] = nullptr;
                    }
                }
            }
        }
    }

    bool ReportNodeDemographics::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "IP_Key_To_Collect", &m_IPKeyToCollect, RND_IP_Key_To_Collect_DESC_TEXT, "" );
        if( inputJson->Exist("Age_Bins") || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap("Age_Bins", &m_AgeYears, Report_Age_Bins_DESC_TEXT );
        }
        else
        {
            m_AgeYears.push_back( 40.0 );
            m_AgeYears.push_back( 80.0 );
            m_AgeYears.push_back( 125.0 );
        }
        initConfigTypeMap( "Stratify_By_Gender", &m_StratifyByGender, RND_Stratify_By_Gender_DESC_TEXT, true );
        bool ret = JsonConfigurable::Configure( inputJson );

        return ret;
    }

    NodeData* ReportNodeDemographics::CreateNodeData()
    {
        return new NodeData();
    }

    void ReportNodeDemographics::Initialize( unsigned int nrmSize )
    {
        if( !m_IPKeyToCollect.empty() )
        {
            std::list<std::string> ip_value_list = IPFactory::GetInstance()->GetIP( m_IPKeyToCollect )->GetValues<IPKeyValueContainer>().GetValuesToList();
            for( auto val : ip_value_list )
            {
                m_IPValuesList.push_back( val );
            }
        }
        else
        {
            m_IPValuesList.push_back( "<no ip>" ); // need at least one in the list
        }

        if( m_AgeYears.size() == 0 ) // implies don't stratify by years
        {
            m_AgeYears.push_back( 125.0 );
            m_StratifyByAge = false;
        }

        int num_genders = m_StratifyByGender ? 2 : 1; // 1 implies not stratifying by gender

        // initialize the counters so that they can be indexed by gender, age, IP, and other (has symptoms)
        int num_other = GetNumInOtherStratification();
        for( int g = 0 ; g < num_genders ; g++ )
        {
            m_Data.push_back( std::vector<std::vector<std::vector<NodeData*>>>() );
            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                m_Data[ g ].push_back( std::vector<std::vector<NodeData*>>() );
                for( int i = 0 ; i < m_IPValuesList.size() ; ++i )
                {
                    m_Data[ g ][ a ].push_back( std::vector<NodeData*>() );
                    for( int s = 0; s < num_other; ++s )
                    {
                        NodeData* pnd = CreateNodeData();
                        m_Data[ g ][ a ][ i ].push_back( pnd );
                    }
                }
            }
        }
        BaseTextReport::Initialize( nrmSize );
    }

    bool ReportNodeDemographics::HasOtherStratificationColumn() const
    {
        return false;
    }

    std::string ReportNodeDemographics::GetOtherStratificationColumnName() const
    {
        return "";
    }

    int ReportNodeDemographics::GetNumInOtherStratification() const
    {
        return 1;
    }

    std::string ReportNodeDemographics::GetOtherStratificationValue( int otherIndex ) const
    {
        return "";
    }

    int ReportNodeDemographics::GetIndexOfOtherStratification( IIndividualHumanEventContext* individual ) const
    {
        return 0;
    }

    std::string ReportNodeDemographics::GetHeader() const
    {
        std::stringstream header ;
        header <<         "Time" 
               << "," << "NodeID" ;
        if( m_StratifyByGender )
        {
            header << "," << "Gender" ;
        }
        if( m_StratifyByAge )
        {
            header << "," << "AgeYears" ;
        }
        if( !m_IPKeyToCollect.empty() )
        {
            header << ",IndividualProp=" << m_IPKeyToCollect;
        }
        if( HasOtherStratificationColumn() )
        {
            header << "," << GetOtherStratificationColumnName();
        }

        header << "," << "NumIndividuals" 
               << "," << "NumInfected"
               ;

        for( auto pnp : NPFactory::GetInstance()->GetNPList() )
        {
            header << ",NodeProp=" << pnp->GetKey<NPKey>().ToString();
        }

        return header.str();
    }

    bool ReportNodeDemographics::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return true ;
    }

    void ReportNodeDemographics::LogIndividualData( IIndividualHuman* individual ) 
    {
        int gender_index  = m_StratifyByGender ? (int)(individual->GetGender()) : 0;
        int age_bin_index = ReportUtilities::GetAgeBin( individual->GetAge(), m_AgeYears );
        int ip_index      = GetIPIndex( individual->GetProperties() );
        int other_index   = GetIndexOfOtherStratification( individual->GetEventContext() );

        NodeData* p_nd = m_Data[ gender_index ][ age_bin_index ][ ip_index ][ other_index ];
        p_nd->num_people += 1;

        if( individual->GetInfections().size() > 0 )
        {
            p_nd->num_infected += 1;
        }
        LogIndividualData( individual, p_nd );
    }

    void ReportNodeDemographics::LogNodeData( INodeContext* pNC )
    {
        float time = pNC->GetTime().time;
        auto node_id = pNC->GetExternalID();
        NPKeyValueContainer& np_values = pNC->GetNodeProperties();

        int num_genders = m_StratifyByGender ? 2 : 1; // 1 implies not stratifying by gender
        for( int g = 0 ; g < num_genders ; g++ )
        {
            char* gender = (g == 0) ? "M" : "F"  ;

            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                for( int i = 0 ; i < m_IPValuesList.size() ; ++i )
                {
                    for( int s = 0; s < GetNumInOtherStratification(); ++s )
                    {
                        GetOutputStream() << time
                            << "," << node_id;
                        if( m_StratifyByGender )
                        {
                            GetOutputStream() << "," << gender;
                        }
                        if( m_StratifyByAge )
                        {
                            GetOutputStream() << "," << m_AgeYears[ a ];
                        }
                        if( !m_IPKeyToCollect.empty() )
                        {
                            GetOutputStream() << ", " << m_IPValuesList[ i ];
                        }
                        if( HasOtherStratificationColumn() )
                        {
                            GetOutputStream() << ", " << GetOtherStratificationValue( s );
                        }

                        WriteNodeData( m_Data[ g ][ a ][ i ][ s ] );

                        for( auto pnp : NPFactory::GetInstance()->GetNPList() )
                        {
                            NPKeyValue kv = np_values.Get( pnp->GetKey<NPKey>() );
                            GetOutputStream() << "," << kv.GetValueAsString();
                        }
                        GetOutputStream() << std::endl;
                    }
                }
            }
        }

        // Reset the counters for the next node
        for( int g = 0 ; g < num_genders ; g++ )
        {
            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                for( int i = 0 ; i < m_IPValuesList.size() ; ++i )
                {
                    for( int s = 0; s < GetNumInOtherStratification(); ++s )
                    {
                        m_Data[ g ][ a ][ i ][ s ]->Reset();
                    }
                }
            }
        }
    }

    void ReportNodeDemographics::WriteNodeData( const NodeData* pData )
    {
        GetOutputStream() << "," << pData->num_people
                          << "," << pData->num_infected;
    }

    int ReportNodeDemographics::GetIPIndex( IPKeyValueContainer* pProps ) const
    {
        int index = 0;
        if( !m_IPKeyToCollect.empty() )
        {
            std::string value = pProps->Get( IPKey(m_IPKeyToCollect) ).GetValueAsString();

            index = std::find( m_IPValuesList.cbegin(), m_IPValuesList.cend(), value ) - m_IPValuesList.cbegin();
        }
        return index;
    }
}
