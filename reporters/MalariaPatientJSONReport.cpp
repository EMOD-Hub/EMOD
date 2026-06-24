
#include "stdafx.h"
#include "MalariaPatientJSONReport.h"
#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "MalariaEnums.h"
#include "IDrug.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "Serializer.h"
#include "ReportUtilitiesMalaria.h"
#include "SimulationEventContext.h"
#include "NodeEventContext.h"
#include "ReportUtilities.h"
#include "Serializer.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#else
#include "RandomNumberGeneratorFactory.h"
#include "RANDOM.h"
#endif

// Output file name
static const std::string _report_name = "MalariaPatientReport.json"; // <<< Filename to put data into

SETUP_LOGGING( "MalariaPatientJSONReport" ) // <<< Name of this file

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

instantiator_function_t rif = []()
{
    return (IReport*)(new MalariaPatientJSONReport()); // <<< Report to create
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
#endif //_REPORT_DLL

    // ---------------------------
    // --- MalariaPatent Methods
    // ---------------------------

    MalariaPatient::MalariaPatient(int id_, float age_, float birthday_)
        : id(id_)
        , initial_age(age_)
        , birthday(birthday_)
        , n_drug_treatments(0)
    { }

    MalariaPatient::~MalariaPatient()
    { }

    void MalariaPatient::JSerialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        LOG_DEBUG("Serializing MalariaPatient\n");
        root->BeginObject();

        LOG_DEBUG("Inserting simple variables\n");
        root->Insert("id", id);
        root->Insert("initial_age", initial_age);
        root->Insert("birthday", birthday);

        LOG_DEBUG("Inserting array variables\n");
        SerializeChannel("true_asexual_parasites", true_asexual_density, root, helper);
        SerializeChannel("true_gametocytes", true_gametocyte_density, root, helper);
        SerializeChannel("asexual_parasites", asexual_parasite_density, root, helper);
        SerializeChannel("gametocytes", gametocyte_density, root, helper);
        SerializeChannel("infected_mosquito_fraction", infectiousness, root, helper);
        SerializeChannel("hemoglobin", hemoglobin, root, helper);
        SerializeChannel("temps", fever, root, helper);
        SerializeChannel("treatment", drug_treatments, root, helper);
        SerializeChannel("asexual_positive_fields", pos_fields_of_view, root, helper);
        SerializeChannel("gametocyte_positive_fields", gametocyte_pos_fields_of_view, root, helper);

        root->EndObject();
    }

    void MalariaPatient::SerializeChannel( std::string channel_name, std::vector<float> &channel_data,
                                           IJsonObjectAdapter* root, JSerializer* helper )
    {
        root->Insert(channel_name.c_str());
        helper->JSerialize(channel_data, root);
    }

    void MalariaPatient::SerializeChannel( std::string channel_name, std::vector<std::string> &channel_data,
                                           IJsonObjectAdapter* root, JSerializer* helper )
    {
        root->Insert(channel_name.c_str());
        helper->JSerialize(channel_data, root);
    }

    void MalariaPatient::Deserialize( IJsonObjectAdapter& root )
    {
        id = root.GetInt( "id" );
        initial_age = root.GetFloat( "initial_age" );
        birthday = root.GetFloat( "birthday" );

        ReportUtilities::DeserializeVector( root, false, "true_asexual_parasites",     true_asexual_density          );
        ReportUtilities::DeserializeVector( root, false, "true_gametocytes",           true_gametocyte_density       );
        ReportUtilities::DeserializeVector( root, false, "asexual_parasites",          asexual_parasite_density      );
        ReportUtilities::DeserializeVector( root, false, "gametocytes",                gametocyte_density            );
        ReportUtilities::DeserializeVector( root, false, "infected_mosquito_fraction", infectiousness                );
        ReportUtilities::DeserializeVector( root, false, "hemoglobin",                 hemoglobin                    );
        ReportUtilities::DeserializeVector( root, false, "temps",                      fever                         );
        ReportUtilities::DeserializeVector( root, false, "treatment",                  drug_treatments               );
        ReportUtilities::DeserializeVector( root, false, "asexual_positive_fields",    pos_fields_of_view            );
        ReportUtilities::DeserializeVector( root, false, "gametocyte_positive_fields", gametocyte_pos_fields_of_view );
    }

    void MalariaPatient::Update( const MalariaPatient& rPatientFromOtherCore )
    {
        release_assert( this->id == rPatientFromOtherCore.id );

        UpdateVector( this->true_asexual_density         , rPatientFromOtherCore.true_asexual_density          );
        UpdateVector( this->true_gametocyte_density      , rPatientFromOtherCore.true_gametocyte_density       );
        UpdateVector( this->asexual_parasite_density     , rPatientFromOtherCore.asexual_parasite_density      );
        UpdateVector( this->gametocyte_density           , rPatientFromOtherCore.gametocyte_density            );
        UpdateVector( this->infectiousness               , rPatientFromOtherCore.infectiousness                );
        UpdateVector( this->hemoglobin                   , rPatientFromOtherCore.hemoglobin                    );
        UpdateVector( this->fever                        , rPatientFromOtherCore.fever                         );
        //UpdateVector( this->drug_treatments              , rPatientFromOtherCore.drug_treatments               );
        UpdateVector( this->pos_fields_of_view           , rPatientFromOtherCore.pos_fields_of_view            );
        UpdateVector( this->gametocyte_pos_fields_of_view, rPatientFromOtherCore.gametocyte_pos_fields_of_view );

        this->drug_treatments.push_back( rPatientFromOtherCore.drug_treatments.back() );

        this->n_drug_treatments = this->drug_treatments.size();
    }

    void MalariaPatient::UpdateVector( std::vector<float>& rThis, const std::vector<float>& rOther )
    {
        rThis.push_back( rOther.back() );
    }

// ----------------------------------------
// --- MalariaPatientJSONReport Methods
// ----------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( MalariaPatientJSONReport )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
    HANDLE_INTERFACE( IReport )
    HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_BODY( MalariaPatientJSONReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( MalariaPatientJSONReport )
#endif

    MalariaPatientJSONReport::MalariaPatientJSONReport()
        : BaseReport()
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_IsValidTime( false )
        , m_NumTimeSteps(0)
        , m_PatientMapTimeStep()
        , m_PatientMapMaster()
        , m_DetectionThresholds()
        , m_pRNG( nullptr )
    {
        initSimTypes( 1, "MALARIA_SIM" );

#ifdef _REPORT_DLL
    m_pRNG = DLL_HELPER.CreateRandomNumberGenerator( EnvPtr );
#else
    m_pRNG = RandomNumberGeneratorFactory::CreateRandomNumberGeneratorForReport();
#endif
    }

    MalariaPatientJSONReport::~MalariaPatientJSONReport()
    {
        m_PatientMapTimeStep.clear();
        m_PatientMapMaster.clear();
        delete m_pRNG;
    }

    bool MalariaPatientJSONReport::Configure( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );
        }
        return ret;
    }

    void MalariaPatientJSONReport::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
    }

    void MalariaPatientJSONReport::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), demographicNodeIds );
    }

    void MalariaPatientJSONReport::UpdateEventRegistration( float currentTime,
                                                            float dt,
                                                            std::vector<INodeEventContext*>& rNodeEventContextList,
                                                            ISimulationEventContext* pSimEventContext )
    {
        m_IsValidTime = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
    }

    void MalariaPatientJSONReport::BeginTimestep()
    {
        if( m_IsValidTime )
        {
            m_NumTimeSteps += 1;
        }
    }

    void MalariaPatientJSONReport::LogNodeData( INodeContext * pNC )
    {
        LOG_DEBUG( "LogNodeData\n" );
    }

    bool MalariaPatientJSONReport::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsValidTime;
    }

    void MalariaPatientJSONReport::LogIndividualData( IIndividualHuman* individual )
    {
        if( !m_IsValidTime ) return;
        if( !m_ReportFilter.IsValidNode( individual->GetEventContext()->GetNodeEventContext() ) ) return;
        if( !m_ReportFilter.IsValidHuman( individual ) ) return;

        LOG_DEBUG( "LogIndividualData\n" );

        // individual identifying info
        int id           = individual->GetSuid().data;
        double mc_weight = individual->GetMonteCarloWeight();
        double age       = individual->GetAge();

        // get malaria contexts
        const IndividualHumanMalaria* individual_malaria = static_cast<const IndividualHumanMalaria*>(individual);
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        // get the correct existing patient or insert a new one
        patient_map_t::const_iterator it = m_PatientMapTimeStep.find(id);
        if ( it == m_PatientMapTimeStep.end() )
        {
            float sim_time = individual->GetEventContext()->GetNodeEventContext()->GetTime().time;
            MalariaPatient patient(id, age, sim_time -age);
            m_PatientMapTimeStep.insert( std::make_pair(id, patient) );
        }
        MalariaPatient& r_patient = m_PatientMapTimeStep[ id ];

        // Push back today's disease variables for infected individuals
        float max_fever = susceptibility_malaria->GetMaxFever();
        if( max_fever < m_DetectionThresholds[ MalariaDiagnosticType::FEVER ] )
        {
            max_fever = 0.0f;
        }
        r_patient.fever.push_back( max_fever > 0 ? max_fever + 37.0f : -1.0f );
        r_patient.hemoglobin.push_back( susceptibility_malaria->GetHemoglobin() );
        r_patient.infectiousness.push_back( individual->GetInfectiousness() * 100.0f ); // (100.0f = turn fraction into percentage)

        // True values in model
        r_patient.true_asexual_density.push_back( susceptibility_malaria->get_parasite_density() ); // Getting this directly will make the value one day earlier than the other three densities
        r_patient.true_gametocyte_density.push_back( individual_malaria->GetGametocyteDensity() );

        // Values incorporating variability and sensitivity of blood test
        r_patient.asexual_parasite_density.push_back( individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ) );
        r_patient.gametocyte_density.push_back( individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::BLOOD_SMEAR_GAMETOCYTES ) );

        // Positive fields of view (out of 200 views in Garki-like setup)
        int pos_fields = 0;
        float parasite_density = susceptibility_malaria->get_parasite_density();
        ReportUtilitiesMalaria::CountPositiveSlideFields( m_pRNG, parasite_density, 200, 1.0f / 400, pos_fields );
        r_patient.pos_fields_of_view.push_back( float( pos_fields ) );

        int gam_pos_fields = 0;
        float gametocyte_density = individual_malaria->GetGametocyteDensity();
        ReportUtilitiesMalaria::CountPositiveSlideFields( m_pRNG, gametocyte_density, 200, 1.0f / 400, gam_pos_fields );
        r_patient.gametocyte_pos_fields_of_view.push_back(float(gam_pos_fields));

        // New drugs
        std::list<void*> drug_list = individual->GetInterventionsContext()->GetInterventionsByInterface( GET_IID(IDrug) );
        LOG_DEBUG_F( "Drug doses distributed = %d\n", drug_list.size() );

        int new_drugs = drug_list.size() - r_patient.n_drug_treatments;
        r_patient.n_drug_treatments += new_drugs;
        std::string new_drug_names = "";

        while(new_drugs > 0)
        {
            IDrug* p_drug = static_cast<IDrug*>(drug_list.back());
            new_drug_names += p_drug->GetDrugName();
            drug_list.pop_back();
            new_drugs--;
            if (new_drugs == 0) break;
            new_drug_names += " + ";
        }

        r_patient.drug_treatments.push_back(new_drug_names);
    }

    void MalariaPatientJSONReport::EndTimestep( float currentTime, float dt )
    {
        LOG_DEBUG( "EndTimestep\n" );
        // ----------------------------------------------------------------------------------
        // --- NOTE: We need to do synchronize even if the core is not collecting data.
        // --- We want all cores/ranks communicating if they have no real data.  Part of the
        // --- reason for this is so we don't force the Rank=0 core to know which cores are
        // --- collecting data and which are not.
        // ----------------------------------------------------------------------------------
        if( EnvPtr->MPI.Rank != 0 )
        {
            bool has_data = m_PatientMapTimeStep.size() > 0;

            ReportUtilities::SendHasData( has_data );

            if( has_data )
            {
                IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
                pIJsonObj->CreateNewWriter();
                pIJsonObj->BeginObject();
                pIJsonObj->Insert( "PatientArray" );
                pIJsonObj->BeginArray();

                JSerializer js;
                for( auto entry : m_PatientMapTimeStep )
                {
                    entry.second.JSerialize( pIJsonObj, &js );
                }
                m_PatientMapTimeStep.clear();

                pIJsonObj->EndArray();
                pIJsonObj->EndObject();

                std::string json_data = pIJsonObj->ToString();

                delete pIJsonObj;

                ReportUtilities::SendData( json_data );
            }
        }
        else
        {
            for( auto entry : m_PatientMapTimeStep )
            {
                UpdatePatientInMaster( entry.second );
            }
            m_PatientMapTimeStep.clear();

            for( int fromRank = 1; fromRank < EnvPtr->MPI.NumTasks; ++fromRank )
            {
                bool has_data = ReportUtilities::GetHasData( fromRank );

                if( has_data )
                {
                    std::vector<char> received;
                    ReportUtilities::GetData( fromRank, received );

                    IJsonObjectAdapter* p_json_obj = CreateJsonObjAdapter();
                    p_json_obj->Parse( received.data() );
                    IJsonObjectAdapter* p_json_array = p_json_obj->GetJsonArray( "PatientArray" );

                    UpdatePatientMapFromOtherCore( *p_json_array );

                    delete p_json_array;
                    delete p_json_obj;
                }
            }
        }
    }

    void MalariaPatientJSONReport::Reduce()
    { }

    void MalariaPatientJSONReport::UpdatePatientMapFromOtherCore( IJsonObjectAdapter& pJsonArray )
    {
        for( uint32_t i = 0; i < pJsonArray.GetSize(); ++i )
        {
            MalariaPatient patient;

            patient.Deserialize( *(pJsonArray[ IndexType( i ) ]) );

            UpdatePatientInMaster( patient );
        }
    }

    void MalariaPatientJSONReport::UpdatePatientInMaster( const MalariaPatient& rPatient )
    {
        if( m_PatientMapMaster.count( rPatient.id ) == 0 )
        {
            m_PatientMapMaster.insert( std::make_pair( rPatient.id, rPatient ) );
        }
        else
        {
            MalariaPatient& r_existing = m_PatientMapMaster[ rPatient.id ];

            r_existing.Update( rPatient );
        }
    }

    std::string MalariaPatientJSONReport::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( _report_name );
    }

    void MalariaPatientJSONReport::Finalize()
    {
        std::string output_file_name = GetReportName();

        // Open output file
        LOG_INFO_F( "Writing file: %s\n", output_file_name.c_str() );
        ofstream ofs;
        FileSystem::OpenFileForWriting( ofs, FileSystem::Concat( EnvPtr->OutputPath, output_file_name ).c_str() );

        // Accumulate array of patients as JSON
        int counter = 0;
        JSerializer js;
        LOG_DEBUG("Creating JSON object adaptor.\n");
        IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
        LOG_DEBUG("Creating new JSON writer.\n");
        pIJsonObj->CreateNewWriter();
        LOG_DEBUG("Beginning malaria patient array.\n");
        pIJsonObj->BeginObject();
        pIJsonObj->Insert("patient_array");
        pIJsonObj->BeginArray();
        for(auto &id_patient_pair: m_PatientMapMaster)
        {
            LOG_DEBUG_F("Serializing patient %d\n", counter);
            id_patient_pair.second.JSerialize(pIJsonObj, &js);
            LOG_DEBUG_F("Finished serializing patient %d\n", counter);
        }
        pIJsonObj->EndArray();
        pIJsonObj->Insert("ntsteps", m_NumTimeSteps);
        pIJsonObj->EndObject();

        // Write output to file
        // GetPrettyFormattedOutput() can be used for nicer indentation but bigger filesize
        // NOTE: This report can be quite large.  The pretty format can take this report
        //       from 60 MB to 300 MB.
        const char* sHumans;
        js.GetFormattedOutput(pIJsonObj, sHumans);
        if (sHumans)
        {
            ofs << sHumans << endl;
            LOG_DEBUG("Done inserting\n");
        }
        else
        {
            std::stringstream ss;
            ss << "Failed to create JSON formatted data for file '" << output_file_name << "'";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        if (ofs.is_open())
        {
            ofs.close();
            LOG_DEBUG("Done writing\n");
        }
        pIJsonObj->FinishWriter();
        delete pIJsonObj ;
    }

    void MalariaPatientJSONReport::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
        m_DetectionThresholds = rDetectionThresholds;
    }
}
