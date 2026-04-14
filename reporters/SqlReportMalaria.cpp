
#include "stdafx.h"

#include "SqlReportMalaria.h"

#include "report_params.rc"
#include <sqlite3.h>

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "MalariaContexts.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "ReportUtilities.h"
#include "ReportUtilitiesMalaria.h"
#include "IDrug.h"
#include "Interventions.h"


using namespace Kernel ;

SETUP_LOGGING( "SqlReportMalaria" )

// Output file name
static const std::string _report_name = "SqlReportMalaria.db";

namespace Kernel
{
    // -----------------------------
    // --- PatientDrugStatus Methods
    // -----------------------------
    PatientDrugStatus::PatientDrugStatus()
        : drug_name()
        , current_efficacy( 0.0 )
        , num_remaining_doses()
    {
    }

    PatientDrugStatus::~PatientDrugStatus()
    {
    }

    void PatientDrugStatus::serialize( IArchive& ar, PatientDrugStatus& pds )
    {
        ar.startObject();
        ar.labelElement( "drug_name"           ) & pds.drug_name;
        ar.labelElement( "current_efficacy"    ) & pds.current_efficacy;
        ar.labelElement( "num_remaining_doses" ) & pds.num_remaining_doses;
        ar.endObject();
    }

    // ------------------------------------
    // --- PatientInfectionMalaria Methods
    // ------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( PatientInfectionMalaria, PatientInfection )
    END_QUERY_INTERFACE_DERIVED( PatientInfectionMalaria, PatientInfection )

    PatientInfectionMalaria::PatientInfectionMalaria()
        : PatientInfection()
        , infected_red_blood_cells( 0.0f )
        , num_mature_gametocytes_female( 0 )
        , num_mature_gametocytes_male( 0 )
    {
    }

    PatientInfectionMalaria::~PatientInfectionMalaria()
    {
    }

    REGISTER_SERIALIZABLE(PatientInfectionMalaria);

    void PatientInfectionMalaria::serialize( IArchive& ar, PatientInfectionMalaria* obj )
    {
        PatientInfection::serialize( ar, obj );
        PatientInfectionMalaria& pi = *obj;
        ar.labelElement( "infected_red_blood_cells"      ) & pi.infected_red_blood_cells;
        ar.labelElement( "num_mature_gametocytes_female" ) & pi.num_mature_gametocytes_female;
        ar.labelElement( "num_mature_gametocytes_male"   ) & pi.num_mature_gametocytes_male;
    }

    // ------------------------------
    // --- SqlPatientMalaria Methods
    // ------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( SqlPatientMalaria, SqlPatient )
    END_QUERY_INTERFACE_DERIVED( SqlPatientMalaria, SqlPatient )

    SqlPatientMalaria::SqlPatientMalaria()
        : SqlPatient()
        , relative_biting_rate(0.0f)
        , is_clinical_case(false)
        , is_severe_case(false)
        , severe_case_type(SevereCaseTypesEnum::NONE)
        , inv_microliters_blood(0.0f)
        , red_blood_cell_count(0.0f)
        , cytokines(0.0f)
        , hrp2(0.0f)
        , pfemp1_major_variant_fraction(0.0f)
        , drug_status()
    {
    }

    SqlPatientMalaria::~SqlPatientMalaria()
    {
    }

    REGISTER_SERIALIZABLE(SqlPatientMalaria);

    void SqlPatientMalaria::serialize( IArchive& ar, SqlPatientMalaria* obj )
    {
        SqlPatient::serialize( ar, obj );
        SqlPatientMalaria& sp = *obj;
        ar.labelElement( "relative_biting_rate"          ) & sp.relative_biting_rate;
        ar.labelElement( "is_clinical_case"              ) & sp.is_clinical_case;
        ar.labelElement( "is_severe_case"                ) & sp.is_severe_case;
        ar.labelElement( "severe_case_type"              ) & (uint32_t&)sp.severe_case_type;
        ar.labelElement( "inv_microliters_blood"         ) & sp.inv_microliters_blood;
        ar.labelElement( "red_blood_cell_count"          ) & sp.red_blood_cell_count;
        ar.labelElement( "cytokines"                     ) & sp.cytokines;
        ar.labelElement( "hrp2"                          ) & sp.hrp2;
        ar.labelElement( "pfemp1_major_variant_fraction" ) & sp.pfemp1_major_variant_fraction;
        ar.labelElement( "drug_status"                   ) & sp.drug_status;
    }

    // ----------------------------------------
    // --- SqlReportMalaria Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( SqlReportMalaria, SqlReport )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
    END_QUERY_INTERFACE_DERIVED( SqlReportMalaria, SqlReport )

    IMPLEMENT_FACTORY_REGISTERED( SqlReportMalaria )

    SqlReportMalaria::SqlReportMalaria()
        : SqlReportMalaria( _report_name )
    {
    }

    SqlReportMalaria::SqlReportMalaria( const std::string& rReportName )
        : SqlReport( rReportName )
        , m_TableDrugStatus( "DrugStatus" )
        , m_TableSeverCaseType( "SevereCaseType" )
        , m_pStatementInsertDrugStatus( nullptr )
        , m_IncludeTableDrugStatus( false )
    {
        initSimTypes( 1, "MALARIA_SIM" );

        m_TableSeverCaseType.AddColumn( "SevereCaseTypeID", "INT"  );
        m_TableSeverCaseType.AddColumn( "Name",             "TEXT" );
        m_TableSeverCaseType.AddPrimaryKey( "(SevereCaseTypeID)"   );

        m_TableHealth.AddColumn( "RelativeBitingRate",    "REAL" );
        m_TableHealth.AddColumn( "IsClinicalCase",        "INT"  );
        m_TableHealth.AddColumn( "IsSevereCase",          "INT"  );
        m_TableHealth.AddColumn( "SevereCaseTypeID",      "INT"  );
        m_TableHealth.AddColumn( "InvMicrolitersBlood",   "REAL" );
        m_TableHealth.AddColumn( "RedBloodCellCount",     "INT"  );
        m_TableHealth.AddColumn( "Cytokines",             "REAL" );
        m_TableHealth.AddColumn( "HRP2",                  "REAL" );
        m_TableHealth.AddColumn( "PfEMP1VariantFraction", "REAL" );
        m_TableHealth.AddForeignKey( "(SevereCaseTypeID)", "SevereCaseType (SevereCaseTypeID)" );

        m_TableInfectionData.AddColumn( "InfectedRedBloodCells",      "INT" );
        m_TableInfectionData.AddColumn( "NumMatureGametocytesFemale", "INT" );
        m_TableInfectionData.AddColumn( "NumMatureGametocytesMale",   "INT" );

        m_TableDrugStatus.AddColumn( "RunNumber",         "INT"  );
        m_TableDrugStatus.AddColumn( "HumanID",           "INT"  );
        m_TableDrugStatus.AddColumn( "SimTime",           "REAL" );
        m_TableDrugStatus.AddColumn( "DrugName",          "TEXT" );
        m_TableDrugStatus.AddColumn( "CurrentEfficacy",   "REAL" );
        m_TableDrugStatus.AddColumn( "NumRemainingDoses", "INT"  );
        m_TableDrugStatus.AddForeignKey( "(RunNumber,HumanID)", "Humans (RunNumber,HumanID)" );
    }

    SqlReportMalaria::~SqlReportMalaria()
    {
    }

    bool SqlReportMalaria::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Include_Drug_Status_Table", &m_IncludeTableDrugStatus, SQLM_Include_Drug_Status_Table_DESC_TEXT,    false );

        bool ret = SqlReport::Configure( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
        }
        return ret;
    }

    void SqlReportMalaria::Initialize( unsigned int nrmSize )
    {
        if( EnvPtr->MPI.Rank != 0)
        {
            return;
        }

        SqlReport::Initialize( nrmSize );

        if( m_IncludeTableHealth )
        {
            ExecuteStatement( m_TableSeverCaseType.GetCreateTableStatement().c_str() );
            FillSevereCaseTypeTable();
        }
        if( m_IncludeTableDrugStatus )
        {
            ExecuteStatement( m_TableDrugStatus.GetCreateTableStatement().c_str() );
            CreatePreparedStatement( m_TableDrugStatus.GetInsertStatement().c_str(), &m_pStatementInsertDrugStatus);
        }
    }

    void SqlReportMalaria::FillSevereCaseTypeTable()
    {
        for( int i = 0; i < SevereCaseTypesEnum::pairs::count(); ++i )
        {
            int id = SevereCaseTypesEnum::pairs::get_values()[ i ];
            const char* name = SevereCaseTypesEnum::pairs::get_keys()[ i ];

            std::stringstream ss;
            ss << "INSERT INTO SevereCaseType (SevereCaseTypeID,Name) VALUES (" << id << ",'" << name << "')";
            ExecuteStatement( ss.str().c_str() );
        }
    }

    void SqlReportMalaria::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
    }

    void SqlReportMalaria::ExtractPatientInfo( IIndividualHuman* pInd, SqlPatient* pPatient )
    {
        SqlReport::ExtractPatientInfo( pInd, pPatient );

        IIndividualHumanVectorContext* p_ind_vector = nullptr;
        if (s_OK != pInd->QueryInterface(GET_IID(IIndividualHumanVectorContext), (void**)&p_ind_vector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pInd", "IIndividualHumanVectorContext", "IIndividualHuman");
        }

        IMalariaHumanContext* p_ind_malaria = nullptr;
        if (s_OK != pInd->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&p_ind_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pInd", "IMalariaHumanContext", "IIndividualHuman");
        }
        SqlPatientMalaria* p_patient_malaria = static_cast<SqlPatientMalaria*>(pPatient);

        IMalariaSusceptibility* p_sus_malaria = p_ind_malaria->GetMalariaSusceptibilityContext();

        p_patient_malaria->relative_biting_rate          = p_ind_vector->GetRelativeBitingRate();
        p_patient_malaria->is_clinical_case              = p_ind_malaria->HasClinicalSymptomNew( ClinicalSymptomsEnum::CLINICAL_DISEASE );
        p_patient_malaria->is_severe_case                = p_ind_malaria->HasClinicalSymptomNew( ClinicalSymptomsEnum::SEVERE_DISEASE );
        p_patient_malaria->severe_case_type              = p_sus_malaria->CheckSevereCaseType();
        p_patient_malaria->inv_microliters_blood         = p_sus_malaria->get_inv_microliters_blood();
        p_patient_malaria->red_blood_cell_count          = p_sus_malaria->get_RBC_count();
        p_patient_malaria->cytokines                     = p_sus_malaria->get_cytokines();
        p_patient_malaria->hrp2                          = p_sus_malaria->GetPfHRP2();
        p_patient_malaria->pfemp1_major_variant_fraction = p_sus_malaria->get_fraction_of_variants_with_antibodies( MalariaAntibodyType::PfEMP1_major );

        std::list<void*> drug_list = pInd->GetInterventionsContext()->GetInterventionsByInterface( GET_IID( IDrug ) );
        for( void* p_void_drug : drug_list )
        {
            IDrug* p_drug = static_cast<IDrug*>(p_void_drug);

            PatientDrugStatus pds;
            pds.drug_name           = p_drug->GetDrugName();
            pds.current_efficacy    = p_drug->GetDrugCurrentEfficacy();
            pds.num_remaining_doses = p_drug->GetNumRemainingDoses();

            p_patient_malaria->drug_status.push_back( pds );
        }
    }

    void SqlReportMalaria::ExtractInfectionInfo( IInfection* pInfection, PatientInfection* pPatientInfection )
    {
        SqlReport::ExtractInfectionInfo( pInfection, pPatientInfection );

        IInfectionMalaria* p_inf_malaria = nullptr;
        if (s_OK != pInfection->QueryInterface(GET_IID(IInfectionMalaria), (void**)&p_inf_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pInfection", "IInfectionMalaria", "IInfection");
        }

        PatientInfectionMalaria* p_pi_malaria = static_cast<PatientInfectionMalaria*>(pPatientInfection);

        p_pi_malaria->infected_red_blood_cells      = p_inf_malaria->get_irbc();
        p_pi_malaria->num_mature_gametocytes_female = p_inf_malaria->get_FemaleGametocytes( GametocyteStages::Mature );
        p_pi_malaria->num_mature_gametocytes_male   = p_inf_malaria->get_MaleGametocytes( GametocyteStages::Mature );
    }

    void SqlReportMalaria::AddInterventions( float currentTime, const SqlPatient* pPatient )
    {
        SqlReport::AddInterventions( currentTime, pPatient );

        if( m_IncludeTableDrugStatus )
        {
            const SqlPatientMalaria* p_patient_malaria = static_cast<const SqlPatientMalaria*>(pPatient);

            for( auto& rDrugStatus : p_patient_malaria->drug_status )
            {
                int index = 1;
                Bind( m_pStatementInsertDrugStatus, index++, (int)m_RunNumber );
                Bind( m_pStatementInsertDrugStatus, index++, (int)p_patient_malaria->human_id );
                Bind( m_pStatementInsertDrugStatus, index++,      currentTime );
                Bind( m_pStatementInsertDrugStatus, index++,      rDrugStatus.drug_name );
                Bind( m_pStatementInsertDrugStatus, index++,      rDrugStatus.current_efficacy );
                Bind( m_pStatementInsertDrugStatus, index++, (int)rDrugStatus.num_remaining_doses );

                Step( m_pStatementInsertDrugStatus );
            }
        }
    }

    int SqlReportMalaria::BindToHealth( float currentTime, int nextIndex, const SqlPatient* pPatient )
    {
        nextIndex = SqlReport::BindToHealth( currentTime, nextIndex, pPatient ); // must bind first to base class

        const SqlPatientMalaria* p_patient_malaria = static_cast<const SqlPatientMalaria*>(pPatient);

        Bind( m_pStatementInsertHealth, nextIndex++,      p_patient_malaria->relative_biting_rate          );
        Bind( m_pStatementInsertHealth, nextIndex++, (int)p_patient_malaria->is_clinical_case              );
        Bind( m_pStatementInsertHealth, nextIndex++, (int)p_patient_malaria->is_severe_case                );
        Bind( m_pStatementInsertHealth, nextIndex++, (int)p_patient_malaria->severe_case_type              );
        Bind( m_pStatementInsertHealth, nextIndex++,      p_patient_malaria->inv_microliters_blood         );
        Bind( m_pStatementInsertHealth, nextIndex++,      p_patient_malaria->red_blood_cell_count          );
        Bind( m_pStatementInsertHealth, nextIndex++,      p_patient_malaria->cytokines                     );
        Bind( m_pStatementInsertHealth, nextIndex++,      p_patient_malaria->hrp2                          );
        Bind( m_pStatementInsertHealth, nextIndex++,      p_patient_malaria->pfemp1_major_variant_fraction );

        return nextIndex;
    }

    int SqlReportMalaria::BindToInfectionData( float currentTime, int nextIndex, const PatientInfection* pInfection )
    {
        nextIndex = SqlReport::BindToInfectionData( currentTime, nextIndex, pInfection ); // must bind first to base class

        const PatientInfectionMalaria* p_pi_malaria = static_cast<const PatientInfectionMalaria*>(pInfection);

        Bind( m_pStatementInsertInfectionData, nextIndex++, p_pi_malaria->infected_red_blood_cells      );
        Bind( m_pStatementInsertInfectionData, nextIndex++, p_pi_malaria->num_mature_gametocytes_female );
        Bind( m_pStatementInsertInfectionData, nextIndex++, p_pi_malaria->num_mature_gametocytes_male   );

        return nextIndex;
    }

    void SqlReportMalaria::Finalize()
    {
        if (EnvPtr->MPI.Rank == 0)
        {
            if( m_IncludeTableDrugStatus )
            {
                DeleteStatement( &m_pStatementInsertDrugStatus );
            }
            SqlReport::Finalize();
        }
    }

    PatientInfection* SqlReportMalaria::CreatePatientInfection()
    {
        return new PatientInfectionMalaria();
    }

    SqlPatient* SqlReportMalaria::CreateSqlPatient()
    {
        return new SqlPatientMalaria();
    }
}
