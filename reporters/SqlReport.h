
#pragma once

#include <vector>
#include <map>

#include "IReport.h"
#include "Configuration.h"
#include "ReportFactory.h"
#include "ISerializable.h"

struct sqlite3;
struct sqlite3_stmt;

namespace Kernel
{
    struct IInfection;
    struct IArchive;
    class IPKeyValueContainer;

    // PatientInfeciton is a base-class object for containing data about a person's infection.
    // The data in the object is for the current timestep.
    // The SqlReport will collect information about each person, then write everything
    // to the database for one time step.  
    struct PatientInfection : ISerializable
    {
        PatientInfection();
        ~PatientInfection();

        uint32_t infection_id;
        float    sim_time_created;

        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE(PatientInfection);
    };

    // SqlPatient is the base-class object for collecting data for a person.
    // The data in the object is for the current timestep.
    // The SqlReport will collect information about each person, then write everything
    // to the database for one time step.  
    struct SqlPatient : ISerializable
    {
        SqlPatient();
        ~SqlPatient();

        uint32_t human_id;
        uint32_t node_id;
        char  gender;
        float age_days;
        float infectiousness;
        IPKeyValueContainer* ip_key_values;

        std::vector<PatientInfection*> infections;

        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE(SqlPatient);
    };

    // Column objects are intended to pair the name of the column and its datatype.
    // This information will be used when creating the CREATE TABLE and INSERT
    // SQL commands.
    struct Column
    {
        Column( const char* pColumnName, const char* pDataTypeName )
            : column_name( std::string(pColumnName) )
            , data_type_name( std::string(pDataTypeName) )
        {
        }
        Column() : Column("","") { }
        ~Column() {}

        std::string column_name;
        std::string data_type_name;
    };

    // Table objects are intended to collect the data about a database table
    // so that the CREATE TABLE and INSERT commands can be created for the table.
    // It is NOT meant to be an object where one uses it to interact with the database.
    // i.e. One must use other objects to get data in and out of the database.
    class Table
    {
    public:
        Table( const char* pTableName );
        ~Table();

        void AddColumn( const char* pColumnName, const char* pDataTypeName );
        void AddPrimaryKey( const char* pKey );
        void AddForeignKey( const char* pKey, const char* pReference );

        std::string GetCreateTableStatement() const;
        std::string GetInsertStatement() const;

    private:
        std::string m_TableName;
        std::vector<Column> m_Columns;
        std::string m_PrimaryKeyStatement;
        std::vector<std::string> m_ForeignKeyStatements;
    };

    // SqlReport is the base-class (GENERIC_SIM) report that allows one to collect data on the
    // population during each time step.  This is a relational database where the data for one
    // person is contained in multiple tables that are related to each other.  The base-class
    // database has the following tables:
    //    Humans               - List of all the humans in the simulation during the time the report was collecting data
    //    Health               - List of each human at each time step with data about the person.  When people
    //                           stop showing up in the list, they probably died.  
    //    Infections           - List of each infection in the population during the time the report was collecting data
    //                           This table is like the Humans table but for infections.  It contains information
    //                           about the infection that doesn't change over time.
    //    InfectionData        - List of each infection at each time step during the time the report was collecting data
    //                           This table will allow the user to see how an infections changes over time
    //    IndividualProperties - An optional table that can contain all of the strings for all of the
    //                           individual properties in the simulation (i.e. key-value paris).
    //                           The Health table can have integer IDs for each person at each time step
    //                           instead of having actual strings.
    class SqlReport : public BaseReport
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED( ReportFactory, SqlReport, IReport )
    public:
        static IReport* CreateReport();
        SqlReport();
        SqlReport( const std::string& rReportName );
        virtual ~SqlReport();

        // IReport methods
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void BeginTimestep() override;
        virtual void LogNodeData( INodeContext * pNC ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual std::string GetReportName() const override;
        virtual void Reduce() override;
        virtual void Finalize() override;

    protected:
        // -----------------------------------------------------------------------------------------
        // --- SQLite Wrappers
        // --- The following methods are intended to be a wrapper around the sqlite library methods.
        // --- They should do all of interaction with the API including checking for errors.
        // -----------------------------------------------------------------------------------------

        // open the database where you also create it or open it assuming that it already exists
        void OpenDatabase( bool isCreating=false );

        // close the database
        void CloseDatabase();

        // Have the database execute an SQL statement.  The input text is assumed to be valid SQL.
        // This method is the main work horse of this interface except for the use of prepared statements.
        void ExecuteStatement( const char* pSqlStatement );

        // This method is used to create a statement that is going to be frequently used and needs to be
        // highly efficent.  In this database, it is mostly used to insert data into the database.
        // The method should return a new object in the variable "ppSqlliteStatement" that will be used
        // later in the Bind() and Step() methods.  For the insert case, it is assumed that the
        // developer will have an insert statement like the following:
        //    INSERT INTO MyTable ( ColA, ColB, ColC ) VALUES ( ?, ?, ? )
        void CreatePreparedStatement( const char* pSqlStatement, sqlite3_stmt** ppSqliteStatement );

        // The following Bind() methods allow the developer to associated a value for a particular spot
        // in the insert statement.  The insert statement should be similar to the following:
        //    INSERT INTO MyTable ( ColA, ColB, ColC ) VALUES ( ?, ?, ? )
        // The indexes refer to the index of the column in the statement.  In the above example,
        //    ColA has index = 1
        //    ColB has index = 2
        //    ColC has index = 3
        // Hence, when setting the value for the row to be inserted for ColA, you want to use index=1
        // in the Bind() statement.

        void Bind( sqlite3_stmt* pSqliteStatement, int index, int val );
        void Bind( sqlite3_stmt* pSqliteStatement, int index, int64_t val );
        void Bind( sqlite3_stmt* pSqliteStatement, int index, char val );
        void Bind( sqlite3_stmt* pSqliteStatement, int index, const std::string& val );
        void Bind( sqlite3_stmt* pSqliteStatement, int index, float val );

        // The Step() method executes the prepared statement.  It should be used after all of the
        // Bind() calls have been made to assign data to each column.
        void Step( sqlite3_stmt* pSqliteStatement );

        // When cleaning up the report, we need the DeleteStatement() method to delete the different
        // prepared statement objects that were created.
        void DeleteStatement( sqlite3_stmt** ppSqliteStatement );

        // --------------------------------------------------------------------------------------------
        // --- Data Extraction methods
        // --- The following methods are used to extract data out of the EMOD objects into temporary
        // --- objects that will be used later to put the data into the database.
        // --- The CreateXXX() methods can be overriden by subclasses so that they can create objects
        // --- that extend the base class container.
        // --------------------------------------------------------------------------------------------

        // Create a new object for infection information.
        virtual PatientInfection* CreatePatientInfection();

        // Create a new object with data about a person.  Should contain a list of the infections.
        virtual SqlPatient* CreateSqlPatient();

        // Extract data out of the EMOD object and put into these temporary report objects
        virtual void ExtractPatientInfo( IIndividualHuman* pInd, SqlPatient* pPatient );
        virtual void ExtractInfectionInfo( IInfection* pInfection, PatientInfection* pPatientInfection );

        // ---------------------------------------------------------------------------------
        // --- Add To Database
        // --- The following methods are from the algorithm in EndTimestep() where the data
        // --- is added to the database as a single transaction each time step.
        // --- The AddXXX() methods should be overriden when adding new tables.
        // ---------------------------------------------------------------------------------

        // This method is used to add a human to the database that is new to the database.
        // The "newness" could be because the person was just born, the sim just started,
        // or the report just started collecting data.  The data that is added is data that
        // does not change for the human like their original node or the day they were added to the sim.
        virtual void AddNewHuman( float currentTime, const SqlPatient* pPatient );

        // For each human the database knows about, this method is called each time step to
        // add current data about the person, typically health related data.
        virtual void AddHealth( float currentTime, const SqlPatient* pPatient );

        // This method is intended to add data about the interventions a person has each time step
        virtual void AddInterventions( float currentTime, const SqlPatient* pPatient );

        // Like AddNewHuman(), this method is called to add data about an infection that does not change
        // over time like when it was created or whom it belongs to.
        virtual void AddNewInfection( float currentTime, const SqlPatient* pPatient, const PatientInfection* pInfection );

        // Like AddHelath(), this method adds data about an infection each time step.  When an infection
        // stops appearing in this table, it is usually because it has been cleared.
        virtual void AddInfection( float currentTime, const PatientInfection* pInfection );

        // ------------------------------------------------------------------------------------------
        // --- BindToXXX
        // --- These methods are called from the AddXXX() methods.  The developer can override these
        // --- methods to add data to a table.
        // !!! NOTE: The Bind()calls used in the BindToXXX() methods must be in the same order as
        // !!!       the AddColumn() methods.
        // ------------------------------------------------------------------------------------------

        virtual int BindToHuman(         float currentTime, int nextIndex, const SqlPatient* pPatient );
        virtual int BindToHealth(        float currentTime, int nextIndex, const SqlPatient* pPatient );
        virtual int BindToInfection(     float currentTime, int nextIndex, const SqlPatient* pPatient, const PatientInfection* pInfection );
        virtual int BindToInfectionData( float currentTime, int nextIndex, const PatientInfection* pInfection );

        // Populate a table with the IP values used in the sim
        void FillIndividualPropertiesTable();

        // Send SqlPatient data to the Rank=0 core so that they can be added to the database
        virtual void CollectDataFromOtherCores( float currentTime );

        Table m_TableHuman;
        Table m_TableHealth;
        Table m_TableInfections;
        Table m_TableInfectionData;
        Table m_TableIndividualProperties;

        sqlite3* m_pDB; 
        sqlite3_stmt* m_pStatementInsertHuman;
        sqlite3_stmt* m_pStatementInsertHealth;
        sqlite3_stmt* m_pStatementInsertInfection;
        sqlite3_stmt* m_pStatementInsertInfectionData;

        bool m_IncludeTableHealth;
        bool m_IncludeTableInfectionData;
        bool m_IncludeIP;
        float m_StartDay;
        float m_EndDay;
        uint16_t m_RunNumber;
        std::string m_ReportName;
        std::vector<uint32_t>   m_PreviousPatientIds;
        std::vector<uint32_t>   m_PreviousInfectionIds;
        std::vector<SqlPatient*> m_CurrentPatients;
    };
}
