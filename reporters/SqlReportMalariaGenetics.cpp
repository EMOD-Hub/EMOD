
#include "stdafx.h"

#include "SqlReportMalariaGenetics.h"

#include "report_params.rc"
#include <sqlite3.h>

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "MalariaContexts.h"
#include "MalariaEnums.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "ParasiteGenetics.h"
#include "StrainIdentityMalariaGenetics.h"

#define INSERT_GENOME    "INSERT INTO ParasiteGenomes (RunNumber,GenomeID,BarcodeID,Barcode) VALUES (?,?,?,?)"
#define INSERT_SEQUENCE  "INSERT INTO GenomeSequenceData (RunNumber,GenomeID,NucleotideSequence,AlleleRoots,GenomeLocation) VALUES (?,?,?,?,?)"


using namespace Kernel ;

SETUP_LOGGING( "SqlReportMalariaGenetics" )

// Output file name
static const std::string _report_name = "SqlReportMalariaGenetics.db";

namespace Kernel
{
    // -------------------------------------------
    // --- PatientInfectionMalariaGenetics Methods
    // -------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( PatientInfectionMalariaGenetics, PatientInfectionMalaria )
    END_QUERY_INTERFACE_DERIVED( PatientInfectionMalariaGenetics, PatientInfectionMalaria )

    PatientInfectionMalariaGenetics::PatientInfectionMalariaGenetics()
        : PatientInfectionMalaria()
        , genome()
    {
    }

    PatientInfectionMalariaGenetics::~PatientInfectionMalariaGenetics()
    {
    }

    REGISTER_SERIALIZABLE(PatientInfectionMalariaGenetics);

    void PatientInfectionMalariaGenetics::serialize( IArchive& ar, PatientInfectionMalariaGenetics* obj )
    {
        PatientInfectionMalaria::serialize( ar, obj );
        PatientInfectionMalariaGenetics& pi = *obj;
        ar.labelElement( "genome" ) & pi.genome;
    }

    // ----------------------------------------
    // --- SqlReportMalariaGenetics Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( SqlReportMalariaGenetics, SqlReportMalaria )
    END_QUERY_INTERFACE_DERIVED( SqlReportMalariaGenetics, SqlReportMalaria )

    IMPLEMENT_FACTORY_REGISTERED( SqlReportMalariaGenetics )

    SqlReportMalariaGenetics::SqlReportMalariaGenetics()
        : SqlReportMalaria(_report_name)
        , m_TableGenomeLocations(     "GenomeLocations"     )
        , m_TableGenomeLocationTypes( "GenomeLocationTypes" )
        , m_TableParasiteGenomes(     "ParasiteGenomes"     )
        , m_TableGenomeSequenceData(  "GenomeSequenceData"  )
        , m_pStatementInsertGenome( nullptr )
        , m_pStatementInsertSequenceData( nullptr )
        , m_PreviousGenomeIds()
        , m_GenomeLocations()
    {
        initSimTypes( 1, "MALARIA_SIM" );

        m_TableInfections.AddColumn( "GenomeID", "INT" );

        m_TableGenomeLocationTypes.AddColumn( "LocationTypeID", "INT"  );
        m_TableGenomeLocationTypes.AddColumn( "Name",           "TEXT" );
        m_TableGenomeLocationTypes.AddPrimaryKey( "(LocationTypeID)"   );

        m_TableGenomeLocations.AddColumn( "RunNumber", "INT"  );
        m_TableGenomeLocations.AddColumn( "GenomeLocation", "INT"  );
        m_TableGenomeLocations.AddColumn( "LocationTypeID", "INT"  );
        m_TableGenomeLocations.AddPrimaryKey( "(RunNumber,GenomeLocation)"   );
        m_TableGenomeLocations.AddForeignKey( "(LocationTypeID)", "GenomeLocationType (LocationTypeID)" );

        m_TableParasiteGenomes.AddColumn( "RunNumber", "INT"  );
        m_TableParasiteGenomes.AddColumn( "GenomeID",  "INT"  );
        m_TableParasiteGenomes.AddColumn( "BarcodeID", "INT"  );
        m_TableParasiteGenomes.AddColumn( "Barcode",   "TEXT" );
        m_TableParasiteGenomes.AddPrimaryKey( "(RunNumber,GenomeID)"   );

        m_TableGenomeSequenceData.AddColumn( "RunNumber",          "INT" );
        m_TableGenomeSequenceData.AddColumn( "GenomeID",           "INT" );
        m_TableGenomeSequenceData.AddColumn( "NucleotideSequence", "INT" );
        m_TableGenomeSequenceData.AddColumn( "AlleleRoots",        "INT" );
        m_TableGenomeSequenceData.AddColumn( "GenomeLocation",     "INT" );
        m_TableGenomeSequenceData.AddForeignKey( "(RunNumber,GenomeID)",       "ParasiteGenomes (RunNumber,GenomeID)"       );
        m_TableGenomeSequenceData.AddForeignKey( "(RunNumber,GenomeLocation)", "GenomeLocations (RunNumber,GenomeLocation)" );
    }

    SqlReportMalariaGenetics::~SqlReportMalariaGenetics()
    {
    }

    bool SqlReportMalariaGenetics::Configure( const Configuration * inputJson )
    {
        bool ret = SqlReportMalaria::Configure( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'SqlReportMalariaGenetics' can only be used with 'Malaria_Model'='MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
            }
        }
        return ret;
    }

    void SqlReportMalariaGenetics::Initialize( unsigned int nrmSize )
    {
        if( EnvPtr->MPI.Rank != 0)
        {
            return;
        }

        SqlReportMalaria::Initialize( nrmSize );

        m_GenomeLocations = ParasiteGenetics::GetInstance()->GetLocations();

        ExecuteStatement( m_TableGenomeLocationTypes.GetCreateTableStatement().c_str() );
        ExecuteStatement( m_TableGenomeLocations.GetCreateTableStatement().c_str() );
        ExecuteStatement( m_TableParasiteGenomes.GetCreateTableStatement().c_str() );
        ExecuteStatement( m_TableGenomeSequenceData.GetCreateTableStatement().c_str() );

        FillLocationTypeTable();
        FillLocationsTable();

        CreatePreparedStatement( m_TableParasiteGenomes.GetInsertStatement().c_str(),    &m_pStatementInsertGenome       );
        CreatePreparedStatement( m_TableGenomeSequenceData.GetInsertStatement().c_str(), &m_pStatementInsertSequenceData );
    }

    void SqlReportMalariaGenetics::FillLocationTypeTable()
    {
        for( int i = 0; i < GenomeLocationType::pairs::count(); ++i )
        {
            int id = GenomeLocationType::pairs::get_values()[ i ];
            const char* name = GenomeLocationType::pairs::get_keys()[ i ];

            std::stringstream ss;
            ss << "INSERT INTO GenomeLocationTypes (LocationTypeID,Name) VALUES (" << id << ",'" << name << "')";
            ExecuteStatement( ss.str().c_str() );
        }
    }

    void SqlReportMalariaGenetics::FillLocationsTable()
    {
        for( auto loc : m_GenomeLocations )
        {
            std::stringstream ss;
            ss << "INSERT INTO GenomeLocations (RunNumber,GenomeLocation,LocationTypeID) VALUES ("
               << m_RunNumber << ","
               << loc.second << "," 
               << loc.first << ")";
            ExecuteStatement( ss.str().c_str() );
        }
    }

    void SqlReportMalariaGenetics::ExtractInfectionInfo( IInfection* pInfection, PatientInfection* pPatientInfection )
    {
        SqlReportMalaria::ExtractInfectionInfo( pInfection, pPatientInfection );

        PatientInfectionMalariaGenetics* p_pi_genetics = static_cast<PatientInfectionMalariaGenetics*>(pPatientInfection);

        const StrainIdentityMalariaGenetics& r_si_genetics = static_cast<const StrainIdentityMalariaGenetics&>(pInfection->GetInfectiousStrainID());

        p_pi_genetics->genome = r_si_genetics.GetGenome();
    }

    int SqlReportMalariaGenetics::BindToInfection( float currentTime,
                                                   int nextIndex,
                                                   const SqlPatient* pPatient,
                                                   const PatientInfection* pInfection )
    {
        const PatientInfectionMalariaGenetics* p_pi_genetics = static_cast<const PatientInfectionMalariaGenetics*>(pInfection);

        nextIndex = SqlReportMalaria::BindToInfection( currentTime, nextIndex, pPatient, pInfection ); // must bind first to base class

        Bind( m_pStatementInsertInfection, nextIndex++, (int)p_pi_genetics->genome.GetID() );

        return nextIndex;
    }

    void SqlReportMalariaGenetics::AddNewInfection( float currentTime,
                                                    const SqlPatient* pPatient,
                                                    const PatientInfection* pInfection )
    {
        SqlReportMalaria::AddNewInfection( currentTime, pPatient, pInfection );

        const PatientInfectionMalariaGenetics* p_pi_genetics = static_cast<const PatientInfectionMalariaGenetics*>(pInfection);

        if( std::find( m_PreviousGenomeIds.begin(),
                        m_PreviousGenomeIds.end(),
                        p_pi_genetics->genome.GetID() ) == m_PreviousGenomeIds.end() )
        {
            AddNewGenome( currentTime, p_pi_genetics->genome );
            m_PreviousGenomeIds.push_back( p_pi_genetics->genome.GetID() );
        }
    }

    void SqlReportMalariaGenetics::AddNewGenome( float currentTime, const ParasiteGenome& rGenome )
    {
        Bind( m_pStatementInsertGenome, 1, (int)m_RunNumber                  );
        Bind( m_pStatementInsertGenome, 2, (int)rGenome.GetID()              );
        Bind( m_pStatementInsertGenome, 3,      rGenome.GetBarcodeHashcode() );
        Bind( m_pStatementInsertGenome, 4,      rGenome.GetBarcode()         );

        Step( m_pStatementInsertGenome );

        const std::vector<int32_t>& r_ns  = rGenome.GetNucleotideSequence();
        const std::vector<int32_t>& r_ar  = rGenome.GetAlleleRoots();

        for( int i = 0; i < r_ns.size(); ++i )
        {
            Bind( m_pStatementInsertSequenceData, 1, (int)m_RunNumber                   );
            Bind( m_pStatementInsertSequenceData, 2, (int)rGenome.GetID()               );
            Bind( m_pStatementInsertSequenceData, 3, (int)r_ns[ i ]                     );
            Bind( m_pStatementInsertSequenceData, 4, (int)r_ar[ i ]                     );
            Bind( m_pStatementInsertSequenceData, 5, (int)m_GenomeLocations[ i ].second );

            Step( m_pStatementInsertSequenceData );
        }
    }

    void SqlReportMalariaGenetics::Finalize()
    {
        if (EnvPtr->MPI.Rank == 0)
        {
            DeleteStatement( &m_pStatementInsertGenome );
            DeleteStatement( &m_pStatementInsertSequenceData );

            SqlReport::Finalize();
        }
    }

    PatientInfection* SqlReportMalariaGenetics::CreatePatientInfection()
    {
        return new PatientInfectionMalariaGenetics();
    }
}
