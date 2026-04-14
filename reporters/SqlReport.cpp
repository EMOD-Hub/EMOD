
#include "stdafx.h"

#include "SqlReport.h"

#include "report_params.rc"
#include <sqlite3.h>

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "ReportUtilities.h"
#include "JsonFullWriter.h"
#include "JsonFullReader.h"
#include "Properties.h"

using namespace Kernel ;

SETUP_LOGGING( "SqlReport" )

// Output file name
static const std::string _report_name = "SqlReport.db";

namespace Kernel
{
    // ---------------------------
    // --- PatientInfection Methods
    // ---------------------------

    BEGIN_QUERY_INTERFACE_BODY( PatientInfection )
    END_QUERY_INTERFACE_BODY( PatientInfection )

    PatientInfection::PatientInfection()
        : infection_id( 0 )
        , sim_time_created( 0.0 )
    {
    }

    PatientInfection::~PatientInfection()
    {
    }

    REGISTER_SERIALIZABLE(PatientInfection);

    void PatientInfection::serialize( IArchive& ar, PatientInfection* obj )
    {
        PatientInfection& pi = *obj;
        ar.labelElement( "infection_id"     ) & pi.infection_id;
        ar.labelElement( "sim_time_created" ) & pi.sim_time_created;
    }

    // ---------------------------
    // --- SqlPatient Methods
    // ---------------------------

    BEGIN_QUERY_INTERFACE_BODY( SqlPatient )
    END_QUERY_INTERFACE_BODY( SqlPatient )

    SqlPatient::SqlPatient()
        : human_id(0)
        , node_id(0)
        , gender(0)
        , age_days(0.0f)
        , infectiousness(0.0f)
        , infections()
    {
    }

    SqlPatient::~SqlPatient()
    {
        for( auto p_inf : infections )
        {
            delete p_inf;
        }
        infections.clear();
    }

    REGISTER_SERIALIZABLE(SqlPatient);

    void SqlPatient::serialize( IArchive& ar, SqlPatient* obj )
    {
        SqlPatient& sp = *obj;
        ar.labelElement( "human_id"       ) & sp.human_id;
        ar.labelElement( "node_id"        ) & sp.node_id;
        ar.labelElement( "gender"         ) & (int&)sp.gender;
        ar.labelElement( "age_days"       ) & sp.age_days;
        ar.labelElement( "infectiousness" ) & sp.infectiousness;
        ar.labelElement( "infections"     ) & sp.infections;
    }

    // ----------------------------------------
    // --- Table Methods
    // ----------------------------------------
    Table::Table( const char* pTableName )
        : m_TableName( std::string( pTableName ) )
        , m_Columns()
        , m_PrimaryKeyStatement()
        , m_ForeignKeyStatements()
    {
    }

    Table::~Table()
    {
    }

    void Table::AddColumn( const char* pColumnName, const char* pDataTypeName )
    {
        release_assert( pColumnName != nullptr );
        release_assert( pDataTypeName != nullptr );
        m_Columns.push_back( Column( pColumnName, pDataTypeName ) );
    }

    void Table::AddPrimaryKey( const char* pKey )
    {
        release_assert( pKey != nullptr );
        std::stringstream ss;
        ss << "PRIMARY KEY " << pKey;
        m_PrimaryKeyStatement = ss.str();
    }

    void Table::AddForeignKey( const char* pKey, const char* pReference )
    {
        release_assert( pKey != nullptr );
        release_assert( pReference != nullptr );

        std::stringstream ss;
        ss << "FOREIGN KEY " << pKey << " REFERENCES " << pReference << " ON DELETE CASCADE ON UPDATE NO ACTION ";
        m_ForeignKeyStatements.push_back( ss.str() );
    }

    std::string Table::GetCreateTableStatement() const
    {
        std::stringstream ss;
        ss << "CREATE TABLE " << m_TableName << " (";
        for( auto& r_col : m_Columns )
        {
            ss << r_col.column_name << " " << r_col.data_type_name << " NOT NULL,";
        }
        if( !m_PrimaryKeyStatement.empty() )
        {
            ss << " " << m_PrimaryKeyStatement;
        }
        for( auto& r_fk : m_ForeignKeyStatements )
        {
            ss << " " << r_fk;
        }
        ss << " );";
        return ss.str();
    }

    std::string Table::GetInsertStatement() const
    {
        std::stringstream ss;
        ss << "INSERT INTO " << m_TableName << " (";
        for( int i = 0; i < m_Columns.size(); ++i )
        {
            ss << " " << m_Columns[i].column_name;
            if( (i + 1) < m_Columns.size() )
            {
                ss << ",";
            }
        }
        ss << " ) VALUES (";
        for( int i = 0; i < m_Columns.size(); ++i )
        {
            ss << " ?";
            if( (i + 1) < m_Columns.size() )
            {
                ss << ",";
            }
        }
        ss << " )";
        return ss.str();
    }

    // ----------------------------------------
    // --- SqlReport Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_BODY( SqlReport )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IReport )
        HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_BODY( SqlReport )

    IMPLEMENT_FACTORY_REGISTERED( SqlReport )

    SqlReport::SqlReport()
        : SqlReport( _report_name )
    {
    }

    SqlReport::SqlReport( const std::string& rReportName )
        : BaseReport()
        , m_TableHuman( "Humans" )
        , m_TableHealth( "Health" )
        , m_TableInfections( "Infections" )
        , m_TableInfectionData( "InfectionData" )
        , m_TableIndividualProperties( "IndividualProperties" )
        , m_pDB( nullptr )
        , m_pStatementInsertHuman( nullptr )
        , m_pStatementInsertHealth( nullptr )
        , m_pStatementInsertInfection( nullptr )
        , m_pStatementInsertInfectionData( nullptr )
        , m_IncludeTableHealth( true )
        , m_IncludeTableInfectionData( true )
        , m_IncludeIP( false )
        , m_StartDay( 0.0f )
        , m_EndDay( FLT_MAX )
        , m_RunNumber( 0 )
        , m_ReportName( rReportName )
        , m_PreviousPatientIds()
        , m_PreviousInfectionIds()
        , m_CurrentPatients()
    {
        initSimTypes( 1, "*" );

        m_ReportName = FileSystem::Concat( EnvPtr->OutputPath, m_ReportName );

        m_TableHuman.AddColumn( "RunNumber",      "INT"  );
        m_TableHuman.AddColumn( "HumanID",        "INT"  );
        m_TableHuman.AddColumn( "Gender",         "TEXT" );
        m_TableHuman.AddColumn( "HomeNodeID",     "INT"  );
        m_TableHuman.AddColumn( "InitialAgeDays", "REAL" );
        m_TableHuman.AddColumn( "SimTimeAdded",   "REAL" );
        m_TableHuman.AddPrimaryKey( "(RunNumber,HumanID)" );

        m_TableHealth.AddColumn( "RunNumber",      "INT"  );
        m_TableHealth.AddColumn( "HumanID",        "INT"  );
        m_TableHealth.AddColumn( "NodeID",         "INT"  );
        m_TableHealth.AddColumn( "SimTime",        "REAL" );
        m_TableHealth.AddColumn( "Infectiousness", "REAL" );
        m_TableHealth.AddForeignKey( "(RunNumber,HumanID)", "Humans (RunNumber,HumanID)" );

        m_TableInfections.AddColumn( "RunNumber",      "INT"  );
        m_TableInfections.AddColumn( "InfectionID",    "INT"  );
        m_TableInfections.AddColumn( "HumanID",        "INT"  );
        m_TableInfections.AddColumn( "SimTimeCreated", "REAL" );
        m_TableInfections.AddPrimaryKey( "(RunNumber,InfectionID)" );
        m_TableInfections.AddForeignKey( "(RunNumber,HumanID)", "Humans (RunNumber,HumanID)" );

        m_TableInfectionData.AddColumn( "RunNumber",   "INT"  );
        m_TableInfectionData.AddColumn( "InfectionID", "INT"  );
        m_TableInfectionData.AddColumn( "SimTime",     "REAL" );
        m_TableInfectionData.AddForeignKey( "(RunNumber,InfectionID)", "Infections (RunNumber,InfectionID)" );

        m_TableIndividualProperties.AddColumn( "RunNumber",  "INT"  );
        m_TableIndividualProperties.AddColumn( "KeyValueID", "INT"  );
        m_TableIndividualProperties.AddColumn( "Key",        "TEXT" );
        m_TableIndividualProperties.AddColumn( "Value",      "TEXT" );
        m_TableIndividualProperties.AddPrimaryKey( "(RunNumber,KeyValueID)" );
    }

    SqlReport::~SqlReport()
    {
        for( auto p_patient : m_CurrentPatients )
        {
            delete p_patient;
        }
        m_CurrentPatients.clear();
    }

    bool SqlReport::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Include_Health_Table",          &m_IncludeTableHealth,        SQL_Include_Health_Table_DESC_TEXT,          true );
        initConfigTypeMap( "Include_Infection_Data_Table",  &m_IncludeTableInfectionData, SQL_Include_Infection_Data_Table_DESC_TEXT,  true );
        initConfigTypeMap( "Include_Individual_Properties", &m_IncludeIP,                 SQL_Include_Individual_Properties_DESC_TEXT, false );

        initConfigTypeMap( "Start_Day", &m_StartDay, Report_Start_Day_DESC_TEXT, 0.0, FLT_MAX, 0.0     );
        initConfigTypeMap( "End_Day",   &m_EndDay,   Report_End_Day_DESC_TEXT,   0.0, FLT_MAX, FLT_MAX );

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

    void SqlReport::Initialize( unsigned int nrmSize )
    {
        if( EnvPtr->MPI.Rank != 0)
        {
            return;
        }

        m_RunNumber = GET_CONFIG_INTEGER( EnvPtr->Config, "Run_Number" );


        FileSystem::RemoveFile( m_ReportName );

        OpenDatabase( true );

        // -------------------------------------------------------------------------------
        // --- This turns off the generate of the temporary file used for rolling back
        // --- commits.  I'm turning this off in hopes that the generation of this file
        // --- is what is causing problems with Belegost.
        // --- options are: DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF
        // -------------------------------------------------------------------------------
        ExecuteStatement( "PRAGMA journal_mode = OFF" );

        ExecuteStatement( m_TableHuman.GetCreateTableStatement().c_str() );
        if( m_IncludeIP )
        {
            ExecuteStatement( m_TableIndividualProperties.GetCreateTableStatement().c_str() );
            for( auto p_ip : IPFactory::GetInstance()->GetIPList() )
            {
                std::string column_name = "IPKey_" + p_ip->GetKeyAsString();
                m_TableHealth.AddColumn( column_name.c_str(), "INT");

                std::string foreign_key = "(RunNumber," + column_name + ")";
                m_TableHealth.AddForeignKey( foreign_key.c_str(), "IndividualProperties (RunNumber,KeyValueID)");
            }
            FillIndividualPropertiesTable();
        }
        if( m_IncludeTableHealth )
        {
            ExecuteStatement( m_TableHealth.GetCreateTableStatement().c_str() );
        }
        ExecuteStatement( m_TableInfections.GetCreateTableStatement().c_str() );
        if( m_IncludeTableInfectionData )
        {
            ExecuteStatement( m_TableInfectionData.GetCreateTableStatement().c_str() );
        }

        if( m_pStatementInsertHuman == nullptr )
        {
            CreatePreparedStatement( m_TableHuman.GetInsertStatement().c_str(),     &m_pStatementInsertHuman     );
            CreatePreparedStatement( m_TableInfections.GetInsertStatement().c_str(), &m_pStatementInsertInfection );

            if( m_IncludeTableHealth )
            {
                CreatePreparedStatement( m_TableHealth.GetInsertStatement().c_str(), &m_pStatementInsertHealth );
            }
            if( m_IncludeTableInfectionData )
            {
                CreatePreparedStatement( m_TableInfectionData.GetInsertStatement().c_str(), &m_pStatementInsertInfectionData );
            }
        }
    }

    void SqlReport::FillIndividualPropertiesTable()
    {
        for( auto p_ip : IPFactory::GetInstance()->GetIPList() )
        {
            IPKeyValueContainer container = p_ip->GetValues<IPKeyValueContainer>();
            for( auto key_value : container )
            {
                std::stringstream ss;
                ss << "INSERT INTO IndividualProperties (RunNumber,KeyValueID,Key,Value) VALUES ( "
                   << m_RunNumber << ","
                   << key_value.GetUniqueID()             << " ,"
                   << "'" << key_value.GetKeyAsString()   << "',"
                   << "'" << key_value.GetValueAsString() << "' "
                   << ")";
                ExecuteStatement( ss.str().c_str() );
            }
        }
    }

    void SqlReport::OpenDatabase( bool isCreating )
    {
        /* Open database */
        int flags = SQLITE_OPEN_READWRITE;
        if( isCreating )
        {
            flags |= SQLITE_OPEN_CREATE;
        }
        int rc = sqlite3_open_v2( m_ReportName.c_str(), &m_pDB, flags, nullptr );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to open database (" << m_ReportName << ")\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // --------------------------------------------------------------------------------------------------
        // --- From https://www.whoishostingthis.com/compare/sqlite/optimize/
        // --- "SQLite, by default, waits for the OS to write to disk after issuing each of these inserts.
        // --- You can turn this pause off with a simple command. ... Yu should also know that this could
        // --- cause a database corruption in the event of a crash or power outage. So, you will want to weigh
        // --- the increased speed here against any potential risks."
        // --- NOTE:  This got one sim from 60 to 15 seconds.  Adding prepared statements got the sime
        // ---        from 90 to 60 seconds.
        // --------------------------------------------------------------------------------------------------
        ExecuteStatement( "PRAGMA synchronous = OFF" );
    }

    void SqlReport::CloseDatabase()
    {
        sqlite3_close( m_pDB );
        m_pDB = nullptr;
    }

    void SqlReport::ExecuteStatement( const char* pSqlStatement )
    {
        // if not nullptr, it was allocated using sqlite_malloc() and should be freed using sqlite_free()
        char * p_error_message = nullptr;

        int rc = sqlite3_exec( m_pDB, pSqlStatement, nullptr, nullptr, &p_error_message );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to execute statement (" << pSqlStatement << ")\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << p_error_message;

            sqlite3_free( p_error_message );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::CreatePreparedStatement( const char* pSqlStatement, sqlite3_stmt** ppSqliteStatement )
    {
        int rc = sqlite3_prepare_v2( m_pDB, pSqlStatement, -1, ppSqliteStatement, nullptr );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to execute statement (" << pSqlStatement << ")\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::Bind( sqlite3_stmt* pSqliteStatement, int index, int val )
    {
        int rc = sqlite3_bind_int( pSqliteStatement, index, val );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to bind parameter on index=" << index << "\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::Bind( sqlite3_stmt* pSqliteStatement, int index, int64_t val )
    {
        int rc = sqlite3_bind_int64( pSqliteStatement, index, val );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to bind parameter on index=" << index << "\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::Bind( sqlite3_stmt* pSqliteStatement, int index, char val )
    {
        char buff[ 2 ];
        buff[ 0 ] = val;
        buff[ 1 ] = 0;

        int rc = sqlite3_bind_text( pSqliteStatement, index, buff, 1, SQLITE_TRANSIENT );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to bind parameter on index=" << index << "\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::Bind( sqlite3_stmt* pSqliteStatement, int index, const std::string& val )
    {
        int rc = sqlite3_bind_text( pSqliteStatement, index, val.c_str(), val.length(), SQLITE_TRANSIENT );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to bind parameter on index=" << index << "\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::Bind( sqlite3_stmt* pSqliteStatement, int index, float val )
    {
        int rc = sqlite3_bind_double( pSqliteStatement, index, val );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to bind parameter on index=" << index << "\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::Step( sqlite3_stmt* pSqliteStatement )
    {
        int rc = sqlite3_step( pSqliteStatement );
        if( rc != SQLITE_DONE )
        {
            std::stringstream ss;
            ss << "Failed to step\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        rc = sqlite3_reset( pSqliteStatement );
        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to reset\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        rc = sqlite3_clear_bindings( pSqliteStatement );
        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to clear bindings\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SqlReport::DeleteStatement( sqlite3_stmt** ppSqliteStatement )
    {
        int rc = sqlite3_finalize( *ppSqliteStatement );

        if( rc != SQLITE_OK )
        {
            std::stringstream ss;
            ss << "Failed to finalize\n";
            ss << "Error code: " << rc << "\n";
            ss << "Error message: " << sqlite3_errmsg( m_pDB );

            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        *ppSqliteStatement = nullptr;
    }

    void SqlReport::BeginTimestep()
    {
    }

    void SqlReport::LogNodeData( INodeContext * pNC )
    {
    }

    bool SqlReport::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return (m_StartDay <= currentTime) && (currentTime <= m_EndDay) ;
    }

    void SqlReport::LogIndividualData( IIndividualHuman* pInd )
    {
        SqlPatient* p_patient = CreateSqlPatient();

        ExtractPatientInfo( pInd, p_patient );

        for( auto* p_infection : pInd->GetInfections() )
        {
            PatientInfection* p_pi = CreatePatientInfection();

            ExtractInfectionInfo( p_infection, p_pi );

            p_patient->infections.push_back( p_pi );
        }
        m_CurrentPatients.push_back( p_patient );
    }

    void SqlReport::ExtractPatientInfo( IIndividualHuman* pInd, SqlPatient* pPatient )
    {
        // individual identifying info
        pPatient->human_id       = pInd->GetSuid().data;
        pPatient->node_id        = pInd->GetParent()->GetExternalID();
        pPatient->gender         = (pInd->GetGender() == Gender::FEMALE) ? 'F' : 'M';
        pPatient->age_days       = pInd->GetAge();
        pPatient->infectiousness = pInd->GetInfectiousness();
        pPatient->ip_key_values  = pInd->GetProperties();
    }

    void SqlReport::ExtractInfectionInfo( IInfection* pInfection, PatientInfection* pPatientInfection )
    {
        pPatientInfection->infection_id     = pInfection->GetSuid().data;
        pPatientInfection->sim_time_created = pInfection->GetSimTimeCreated();
    }

    void SqlReport::EndTimestep( float currentTime, float dt )
    {
        if( !IsCollectingIndividualData( currentTime, dt ) ) return;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Subtracting dt because the data is really at currentTime-dt.
        // !!! EndTimestep() is called right after time is incremented.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        currentTime = currentTime - dt;

        if( EnvPtr->MPI.NumTasks > 1 )
        {
            CollectDataFromOtherCores( currentTime );
        }

        if( EnvPtr->MPI.Rank != 0 ) return;

        ExecuteStatement( "BEGIN TRANSACTION;" );

        for( auto p_patient : m_CurrentPatients )
        {
            if( std::find( m_PreviousPatientIds.begin(),
                           m_PreviousPatientIds.end(),
                           p_patient->human_id ) == m_PreviousPatientIds.end() )
            {
                AddNewHuman( currentTime, p_patient );
                m_PreviousPatientIds.push_back( p_patient->human_id );
            }
            AddHealth( currentTime, p_patient );

            AddInterventions( currentTime, p_patient );

            for( auto p_infection : p_patient->infections )
            {
                if( std::find( m_PreviousInfectionIds.begin(),
                               m_PreviousInfectionIds.end(),
                               p_infection->infection_id ) == m_PreviousInfectionIds.end() )
                {
                    AddNewInfection( currentTime, p_patient, p_infection );
                    m_PreviousInfectionIds.push_back( p_infection->infection_id );
                }
                AddInfection( currentTime, p_infection );
            }
        }

        ExecuteStatement( "END TRANSACTION;" );

        m_CurrentPatients.clear();
    }

    void SqlReport::AddNewHuman( float currentTime, const SqlPatient* pPatient )
    {
        int index = 1;

        index = BindToHuman( currentTime, index, pPatient );

        Step( m_pStatementInsertHuman );
    }

    void SqlReport::AddHealth( float currentTime, const SqlPatient* pPatient )
    {
        if( m_IncludeTableHealth )
        {
            int index = 1;

            index = BindToHealth( currentTime, index, pPatient );

            Step( m_pStatementInsertHealth );
        }
    }

    void SqlReport::AddInterventions( float currentTime, const SqlPatient* pPatient )
    {
        // Do nothing
    }

    void SqlReport::AddNewInfection( float currentTime,
                                     const SqlPatient* pPatient,
                                     const PatientInfection* pInfection )
    {
        int index = 1;

        index = BindToInfection( currentTime, index, pPatient, pInfection );

        Step( m_pStatementInsertInfection );
    }

    void SqlReport::AddInfection( float currentTime, const PatientInfection* pInfection )
    {
        if( m_IncludeTableInfectionData )
        {
            int index = 1;

            index = BindToInfectionData( currentTime, index, pInfection );

            Step( m_pStatementInsertInfectionData );
        }
    }

    int SqlReport::BindToHuman( float currentTime, int nextIndex, const SqlPatient* pPatient )
    {
        float time_added = currentTime - pPatient->age_days;
        if( time_added < 0.0 )
        {
            time_added = 0.0;
        }
        Bind( m_pStatementInsertHuman, nextIndex++, (int)m_RunNumber        );
        Bind( m_pStatementInsertHuman, nextIndex++, (int)pPatient->human_id );
        Bind( m_pStatementInsertHuman, nextIndex++,      pPatient->gender   );
        Bind( m_pStatementInsertHuman, nextIndex++, (int)pPatient->node_id  );
        Bind( m_pStatementInsertHuman, nextIndex++,      pPatient->age_days );
        Bind( m_pStatementInsertHuman, nextIndex++,      time_added         );

        return nextIndex;
    }

    int SqlReport::BindToHealth( float currentTime, int nextIndex, const SqlPatient* pPatient )
    {
        Bind( m_pStatementInsertHealth, nextIndex++, (int)m_RunNumber              );
        Bind( m_pStatementInsertHealth, nextIndex++, (int)pPatient->human_id       );
        Bind( m_pStatementInsertHealth, nextIndex++, (int)pPatient->node_id        );
        Bind( m_pStatementInsertHealth, nextIndex++,      currentTime              );
        Bind( m_pStatementInsertHealth, nextIndex++,      pPatient->infectiousness );

        if( m_IncludeIP )
        {
            for( auto p_ip : IPFactory::GetInstance()->GetIPList() )
            {
                int id =  pPatient->ip_key_values->Get( p_ip->GetKey<IPKey>() ).GetUniqueID();
                Bind( m_pStatementInsertHealth, nextIndex++, id );
            }
        }

        return nextIndex;
    }

    int SqlReport::BindToInfection( float currentTime, int nextIndex, const SqlPatient* pPatient, const PatientInfection* pInfection )
    {
        Bind( m_pStatementInsertInfection, nextIndex++, (int)m_RunNumber                  );
        Bind( m_pStatementInsertInfection, nextIndex++, (int)pInfection->infection_id     );
        Bind( m_pStatementInsertInfection, nextIndex++, (int)pPatient->human_id           );
        Bind( m_pStatementInsertInfection, nextIndex++,      pInfection->sim_time_created );

        return nextIndex;
    }

    int SqlReport::BindToInfectionData( float currentTime, int nextIndex, const PatientInfection* pInfection )
    {
        Bind( m_pStatementInsertInfectionData, nextIndex++, (int)m_RunNumber              );
        Bind( m_pStatementInsertInfectionData, nextIndex++, (int)pInfection->infection_id );
        Bind( m_pStatementInsertInfectionData, nextIndex++,      currentTime              );

        return nextIndex;
    }

    std::string SqlReport::GetReportName() const
    {
        return m_ReportName;
    }

    void SqlReport::Reduce()
    {
        // Do nothing
    }

    void SqlReport::Finalize()
    {
        if (EnvPtr->MPI.Rank == 0)
        {
            DeleteStatement( &m_pStatementInsertHuman        );
            DeleteStatement( &m_pStatementInsertInfection    );

            if( m_IncludeTableHealth )
            {
                DeleteStatement( &m_pStatementInsertHealth );
            }
            if( m_IncludeTableInfectionData )
            {
                DeleteStatement( &m_pStatementInsertInfectionData );
            }
            CloseDatabase();
        }
    }

    void SqlReport::CollectDataFromOtherCores( float currentTime )
    {
        if( EnvPtr->MPI.Rank != 0 )
        {
            bool has_data = (m_CurrentPatients.size() > 0);
            ReportUtilities::SendHasData( has_data );

            if( has_data )
            {
                JsonFullWriter writer;
                IArchive* par = static_cast<Kernel::IArchive*>(&writer);
                IArchive& ar = *par;

                ar.labelElement("m_CurrentPatients") & m_CurrentPatients;

                std::string json_data( ar.GetBuffer() );

                ReportUtilities::SendData( json_data );

                m_CurrentPatients.clear();
            }
        }
        else
        {
            for( int fromRank = 1 ; fromRank < EnvPtr->MPI.NumTasks ; ++fromRank )
            {
                bool has_data = ReportUtilities::GetHasData( fromRank );

                if( has_data )
                {
                    std::vector<char> received;
                    ReportUtilities::GetData( fromRank, received );

                    JsonFullReader reader( received.data() );
                    IArchive* reader_par = static_cast<Kernel::IArchive*>(&reader);
                    IArchive& reader_ar = *reader_par;

                    std::vector<SqlPatient*> patients_on_other_core;
                    reader_ar.labelElement("m_CurrentPatients") & patients_on_other_core;

                    for( auto p_patient : patients_on_other_core )
                    {
                        m_CurrentPatients.push_back( p_patient );
                    }
                }
            }
        }
    }

    PatientInfection* SqlReport::CreatePatientInfection()
    {
        return new PatientInfection();
    }

    SqlPatient* SqlReport::CreateSqlPatient()
    {
        return new SqlPatient();
    }
}
