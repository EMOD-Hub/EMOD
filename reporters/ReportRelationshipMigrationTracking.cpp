
#include "stdafx.h"

#include "ReportRelationshipMigrationTracking.h"

#include "ISimulationContext.h"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IMigrationInfo.h"
#include "IRelationship.h"
#include "IIndividualHumanSTI.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "IMigrate.h"
#include "SimulationEventContext.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportRelationshipMigrationTracking" ) // <<< Name of this file

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "STI_SIM", "HIV_SIM", nullptr };// <<< Types of simulation the report is to be used with

instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportRelationshipMigrationTracking()); // <<< Report to create
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
#endif// _REPORT_DLL

// ----------------------------------------
// --- ReportRelationshipMigrationTracking Methods
// ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportRelationshipMigrationTracking, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportRelationshipMigrationTracking, BaseTextReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportRelationshipMigrationTracking )
#endif

    ReportRelationshipMigrationTracking::ReportRelationshipMigrationTracking()
        : BaseTextReportEvents( "ReportRelationshipMigrationTracking.csv" )
        , m_EndTime(0.0)
        , m_MigrationDataMap()
        , m_IsCollectingData( false )
        , m_ReportFilter( "", "", true, true, true )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportRelationshipMigrationTracking::~ReportRelationshipMigrationTracking()
    {
    }

    bool ReportRelationshipMigrationTracking::Configure( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        // BaseTextReportEvents::Configure() doesn't do anything
        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );

            // Manually push required events into the eventTriggerList
            //eventTriggerList.push_back( EventTrigger::Emigrating         );
            //eventTriggerList.push_back( EventTrigger::Immigrating        );
            eventTriggerList.push_back( EventTrigger::STIPreEmigrating );
            eventTriggerList.push_back( EventTrigger::STIPostImmigrating );
            eventTriggerList.push_back( EventTrigger::NonDiseaseDeaths );
            eventTriggerList.push_back( EventTrigger::DiseaseDeaths );
        }
        return ret;
    }

    void ReportRelationshipMigrationTracking::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
        BaseTextReportEvents::Initialize( nrmSize );
    }

    void ReportRelationshipMigrationTracking::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), demographicNodeIds );
    }

    void ReportRelationshipMigrationTracking::UpdateEventRegistration( float currentTime, 
                                                                       float dt, 
                                                                       std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                       ISimulationEventContext* pSimEventContext )
    {
        bool is_valid_time = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
        if( !m_IsCollectingData && is_valid_time )
        {
            m_IsCollectingData = true;
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
        }
        else if( m_IsCollectingData && !is_valid_time )
        {
            UnregisterAllBroadcasters();
            m_IsCollectingData = false;
        }
    }

    std::string ReportRelationshipMigrationTracking::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( report_name );
    }

    std::string ReportRelationshipMigrationTracking::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"             << ", "
               << "Year"             << ", "
               << "IndividualID"     << ", "
               << "AgeYears"         << ", "
               << "Gender"           << ", "
               << "From_NodeID"      << ", "
               << "To_NodeID"        << ", "
               << "MigrationType"    << ", "
               << "Event"            << ","
               << "IsInfected"       << ", "
               << "Rel_ID"           << ", "
               << "NumCoitalActs"    << ", "
               << "IsDiscordant"     << ", "
               << "HasMigrated"      << ", "
               << "RelationshipType" << ", "
               << "RelationshipState"<< ", "
               << "PartnerID"        << ", "
               << "Male_NodeID"      << ", "
               << "Female_NodeID"
               ;

        return header.str();
    }

    bool ReportRelationshipMigrationTracking::notifyOnEvent( IIndividualHumanEventContext *context, 
                                                             const EventTrigger& trigger )
    {
        if( !m_IsCollectingData || !m_ReportFilter.IsValidHuman( context->GetIndividualHumanConst() ) )
        {
            return true;
        }

        const IIndividualHuman* p_ih = context->GetIndividualHumanConst();
        IMigrate * im = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IMigrate), (void**)&im) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMigrate", "IIndividualHumanEventContext");
        }

        IIndividualHumanSTI* p_hsti = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_hsti) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanSTI", "IndividualHuman");
        }

        ISimulationContext* p_sim = context->GetNodeEventContext()->GetNodeContext()->GetParent();

        bool is_emigrating  = (trigger == EventTrigger::STIPreEmigrating);

        float time = context->GetNodeEventContext()->GetTime().time ;
        float year = context->GetNodeEventContext()->GetTime().Year();
        long individual_id = context->GetSuid().data ;
        float age_years = p_ih->GetAge() / DAYSPERYEAR ;
        char gender = (p_ih->GetGender() == 0) ? 'M' : 'F' ;
        uint32_t from_node_id = p_sim->GetNodeExternalID( context->GetNodeEventContext()->GetId() ) ;
        uint32_t to_node_id = 0;
        std::string mig_type_str = "N/A" ;
        if( is_emigrating )
        {
            to_node_id = p_sim->GetNodeExternalID( im->GetMigrationDestination() ) ;
            MigrationType::Enum mig_type = im->GetMigrationType();
            if( mig_type == MigrationType::LOCAL_MIGRATION )
                mig_type_str = "local" ;
            else if( mig_type == MigrationType::AIR_MIGRATION )
                mig_type_str = "air" ;
            else if( mig_type == MigrationType::REGIONAL_MIGRATION )
                mig_type_str = "regional" ;
            else if( mig_type == MigrationType::SEA_MIGRATION )
                mig_type_str = "sea" ;
            else if( mig_type == MigrationType::INTERVENTION_MIGRATION )
                mig_type_str = "intervention" ;
            else
            {
                printf("mig_type=%d\n",mig_type);
                release_assert( false );
            }
        }

        bool is_infected = context->IsInfected();
        for( auto prel : p_hsti->GetRelationships() )
        {
            int rel_id = prel->GetSuid().data;
            int female_id = prel->GetFemalePartnerId().data;
            int male_id   = prel->GetMalePartnerId().data;
            int partner_id = 0;
            if( female_id == context->GetSuid().data )
            {
                partner_id = male_id;
            }
            else
            {
                partner_id = female_id;
            }
            unsigned int num_acts = prel->GetTotalCoitalActs();
            bool is_discordant = prel->IsDiscordant();
            bool has_migrated = prel->HasMigrated();
            const char* rel_type_str  = RelationshipType::pairs::lookup_key( prel->GetType() );
            const char* rel_state_str = RelationshipState::pairs::lookup_key( prel->GetState() );
            unsigned int male_node_id = 0;
            if( prel->MalePartner() != nullptr )
            {
                male_node_id = prel->MalePartner()->GetNodeSuid().data;
            }
            unsigned int female_node_id = 0;
            if( prel->FemalePartner() != nullptr )
            {
                female_node_id = prel->FemalePartner()->GetNodeSuid().data;
            }

            GetOutputStream() << time
                       << "," << year 
                       << "," << individual_id 
                       << "," << age_years 
                       << "," << gender 
                       << "," << from_node_id 
                       << "," << to_node_id 
                       << "," << mig_type_str 
                       << "," << trigger.ToString() 
                       << "," << is_infected 
                       << "," << rel_id 
                       << "," << num_acts 
                       << "," << is_discordant 
                       << "," << has_migrated 
                       << "," << rel_type_str 
                       << "," << rel_state_str 
                       << "," << partner_id 
                       << "," << male_node_id 
                       << "," << female_node_id 
                       << endl;
        }

        return true;
    }

    void ReportRelationshipMigrationTracking::LogIndividualData( IIndividualHuman* individual ) 
    {
    }

    bool ReportRelationshipMigrationTracking::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return false ;
    }

    void ReportRelationshipMigrationTracking::EndTimestep( float currentTime, float dt )
    {
        m_EndTime = currentTime ;
        if( m_IsCollectingData )
        {
            BaseTextReportEvents::EndTimestep( currentTime, dt );
        }
    }

    void ReportRelationshipMigrationTracking::Reduce()
    {
        BaseTextReportEvents::EndTimestep( m_EndTime, 1.0 );
    }

    bool ReportRelationshipMigrationTracking::IsValidNode( INodeEventContext* pNEC ) const
    {
        return m_ReportFilter.IsValidNode( pNEC );
    }
}
