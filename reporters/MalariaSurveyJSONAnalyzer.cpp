
#include "stdafx.h"

#include "MalariaSurveyJSONAnalyzer.h"
#include "report_params.rc"
#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "SusceptibilityMalaria.h"
#include "NodeEventContext.h"
#include "Serializer.h"
#include "ReportUtilities.h"
#include "ReportUtilitiesMalaria.h"
#include "math.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#else
#include "RandomNumberGeneratorFactory.h"
#include "RANDOM.h"
#endif

SETUP_LOGGING( "MalariaSurveyJSONAnalyzer" )

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

Kernel::instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new MalariaSurveyJSONAnalyzer()); // <<< Report to create
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
// --- Patient Methods
// ---------------------------

    Patient::Patient(int id_, float age_, float local_birthday_)
        : id(id_)
        , node_id(-1)
        , initial_age(age_)
        , local_birthday(local_birthday_)
    { }

    Patient::~Patient()
    { }

    void Patient::Serialize( IJsonObjectAdapter& root, JSerializer& helper )
    {
        LOG_DEBUG("Serializing Patient\n");
        root.BeginObject();

        LOG_DEBUG("Inserting simple variables\n");
        root.Insert("id",             id);
        root.Insert("node_id",        node_id);
        root.Insert("initial_age",    initial_age);
        root.Insert("local_birthday", local_birthday);

        root.Insert("strain_ids");
        root.BeginArray();
        for (auto& inner : strain_ids)
        {
            // TODO: add helper function for serializing pair<int,int> ??
            root.BeginArray();
            root.Add(inner.first);
            root.Add(inner.second);
            root.EndArray();
        }
        root.EndArray();

        LOG_DEBUG("Inserting array variables\n");
        ReportUtilities::SerializeVector( root, helper, "ip_data",                        ip_data                          );
        ReportUtilities::SerializeVector( root, helper, "true_asexual_parasites",         true_asexual_density             );
        ReportUtilities::SerializeVector( root, helper, "true_gametocytes",               true_gametocyte_density          );
        ReportUtilities::SerializeVector( root, helper, "smeared_true_asexual_parasites", smeared_true_asexual_density     );
        ReportUtilities::SerializeVector( root, helper, "smeared_true_gametocytes",       smeared_true_gametocyte_density  );
        ReportUtilities::SerializeVector( root, helper, "asexual_parasites",              asexual_parasite_density         );
        ReportUtilities::SerializeVector( root, helper, "gametocytes",                    gametocyte_density               );
        ReportUtilities::SerializeVector( root, helper, "pcr_parasites",                  pcr_parasite_density             );
        ReportUtilities::SerializeVector( root, helper, "pcr_gametocytes",                pcr_gametocyte_density           );
        ReportUtilities::SerializeVector( root, helper, "pfhrp2",                         pfhrp2                           );
        ReportUtilities::SerializeVector( root, helper, "smeared_asexual_parasites",      smeared_asexual_parasite_density );
        ReportUtilities::SerializeVector( root, helper, "smeared_gametocytes",            smeared_gametocyte_density       );
        ReportUtilities::SerializeVector( root, helper, "infectiousness",                 infectiousness                   );
        ReportUtilities::SerializeVector( root, helper, "infectiousness_smeared",         infectiousness_smeared           );
        ReportUtilities::SerializeVector( root, helper, "infectiousness_age_scaled",      infectiousness_age_scaled        );
        ReportUtilities::SerializeVector( root, helper, "pos_asexual_fields",             pos_asexual_fields               );
        ReportUtilities::SerializeVector( root, helper, "pos_gametocyte_fields",          pos_gametocyte_fields            );
        ReportUtilities::SerializeVector( root, helper, "temps",                          fever                            );

        root.EndObject();
    }

    std::vector< std::pair<int,int> > DeserializeStrainIds( IJsonObjectAdapter& root )
    {
        std::vector< std::pair<int,int> > value_list;

        IJsonObjectAdapter* p_json_array = root.GetJsonArray( "strain_ids" );
        for( unsigned int i = 0 ; i < p_json_array->GetSize(); ++i )
        {
            IJsonObjectAdapter* p_inner_array = (*p_json_array)[IndexType(i)];
            int first  = (*p_inner_array)[IndexType(0)]->AsInt();
            int second = (*p_inner_array)[IndexType(1)]->AsInt();
            value_list.push_back( std::make_pair( first, second ) );
        }
        return value_list;
    }

    void Patient::Deserialize( IJsonObjectAdapter& root )
    {
        id             = root.GetInt(   "id"             );
        node_id        = root.GetInt(   "node_id"        );
        initial_age    = root.GetFloat( "initial_age"    );
        local_birthday = root.GetFloat( "local_birthday" );

        strain_ids = DeserializeStrainIds( root );

        ReportUtilities::DeserializeVector( root, false, "ip_data",                        ip_data                          );
        ReportUtilities::DeserializeVector( root, false, "true_asexual_parasites",         true_asexual_density             );
        ReportUtilities::DeserializeVector( root, false, "true_gametocytes",               true_gametocyte_density          );
        ReportUtilities::DeserializeVector( root, false, "smeared_true_asexual_parasites", smeared_true_asexual_density     );
        ReportUtilities::DeserializeVector( root, false, "smeared_true_gametocytes",       smeared_true_gametocyte_density  );
        ReportUtilities::DeserializeVector( root, false, "asexual_parasites",              asexual_parasite_density         );
        ReportUtilities::DeserializeVector( root, false, "gametocytes",                    gametocyte_density               );
        ReportUtilities::DeserializeVector( root, false, "pcr_parasites",                  pcr_parasite_density             );
        ReportUtilities::DeserializeVector( root, false, "pcr_gametocytes",                pcr_gametocyte_density           );
        ReportUtilities::DeserializeVector( root, false, "pfhrp2",                         pfhrp2                           );
        ReportUtilities::DeserializeVector( root, false, "smeared_asexual_parasites",      smeared_asexual_parasite_density );
        ReportUtilities::DeserializeVector( root, false, "smeared_gametocytes",            smeared_gametocyte_density       );
        ReportUtilities::DeserializeVector( root, false, "infectiousness",                 infectiousness                   );
        ReportUtilities::DeserializeVector( root, false, "infectiousness_smeared",         infectiousness_smeared           );
        ReportUtilities::DeserializeVector( root, false, "infectiousness_age_scaled",      infectiousness_age_scaled        );
        ReportUtilities::DeserializeVector( root, false, "pos_asexual_fields",             pos_asexual_fields               );
        ReportUtilities::DeserializeVector( root, false, "pos_gametocyte_fields",          pos_gametocyte_fields            );
        ReportUtilities::DeserializeVector( root, false, "temps",                          fever                            );
    }

// ----------------------------------------
// --- PatientMap Methods
// ----------------------------------------

    PatientMap::PatientMap()
    { }

    PatientMap::~PatientMap()
    { }

    void PatientMap::Clear()
    {
        for( auto& entry: m_Map )
        {
            delete entry.second;
        }
        m_Map.clear();
    }

    template <typename T>
    void UpdateVector( std::vector<T>& existing_vector, std::vector<T>& new_vector )
    {
        for( int i = existing_vector.size(); i < new_vector.size() ; ++i )
        {
            existing_vector.push_back( new_vector[i] );
        }
    }

    void PatientMap::Update( const IIntervalData& rOther )
    {
        const PatientMap& rOtherMap = static_cast< const PatientMap& >(rOther);

        for( auto& other_entry : rOtherMap.m_Map )
        {
            Patient* new_patient = other_entry.second;
            Patient* existing_patient = FindPatient( new_patient->id );

            if( existing_patient == nullptr )
            {
                existing_patient = new Patient(new_patient->id,-1,-1);
                Add( existing_patient );
            }

            existing_patient->node_id        = new_patient->node_id;
            existing_patient->initial_age    = new_patient->initial_age;
            existing_patient->local_birthday = new_patient->local_birthday;
            existing_patient->strain_ids     = new_patient->strain_ids;

            UpdateVector( existing_patient->ip_data,                          new_patient->ip_data                          );
            UpdateVector( existing_patient->true_asexual_density,             new_patient->true_asexual_density             );
            UpdateVector( existing_patient->true_gametocyte_density,          new_patient->true_gametocyte_density          );
            UpdateVector( existing_patient->smeared_true_asexual_density,     new_patient->smeared_true_asexual_density     );
            UpdateVector( existing_patient->smeared_true_gametocyte_density,  new_patient->smeared_true_gametocyte_density  );
            UpdateVector( existing_patient->asexual_parasite_density ,        new_patient->asexual_parasite_density         );
            UpdateVector( existing_patient->gametocyte_density,               new_patient->gametocyte_density               );
            UpdateVector( existing_patient->pcr_parasite_density,             new_patient->pcr_parasite_density             );
            UpdateVector( existing_patient->pcr_gametocyte_density,           new_patient->pcr_gametocyte_density           );
            UpdateVector( existing_patient->pfhrp2                ,           new_patient->pfhrp2                           );
            UpdateVector( existing_patient->smeared_asexual_parasite_density, new_patient->smeared_asexual_parasite_density );
            UpdateVector( existing_patient->smeared_gametocyte_density,       new_patient->smeared_gametocyte_density       );
            UpdateVector( existing_patient->infectiousness,                   new_patient->infectiousness                   );
            UpdateVector( existing_patient->infectiousness_smeared,           new_patient->infectiousness_smeared           );
            UpdateVector( existing_patient->infectiousness_age_scaled,        new_patient->infectiousness_age_scaled        );
            UpdateVector( existing_patient->pos_asexual_fields,               new_patient->pos_asexual_fields               );
            UpdateVector( existing_patient->pos_gametocyte_fields,            new_patient->pos_gametocyte_fields            );
            UpdateVector( existing_patient->fever,                            new_patient->fever                            );
        }
    }

    void PatientMap::Serialize( IJsonObjectAdapter& rjoa, JSerializer& js )
    {
        rjoa.Insert("patient_array");
        rjoa.BeginArray();
        for( auto &id_patient_pair: m_Map )
        {
            Patient* patient = id_patient_pair.second;
            release_assert( patient );
            patient->Serialize( rjoa, js );
        }
        rjoa.EndArray();
    }

    void PatientMap::Deserialize( IJsonObjectAdapter& rjoa )
    {
        IJsonObjectAdapter* p_json_array = rjoa.GetJsonArray("patient_array");

        for( unsigned int i = 0 ; i < p_json_array->GetSize() ; ++i )
        {
            Patient* new_patient = new Patient(-1,-1,-1);
            new_patient->Deserialize( *(*p_json_array)[i] );

            Add( new_patient );
        }
        delete p_json_array;
    }

    Patient* PatientMap::FindPatient( uint32_t id )
    {
        Patient* p_patient = nullptr;
        std::map<uint32_t,Patient*>::iterator it = m_Map.find( id );
        if( it != m_Map.end() )
        {
            p_patient = it->second;
        }
        return p_patient;
    }

    void PatientMap::Add( Patient* pPatient )
    {
        m_Map.insert( std::make_pair( pPatient->id, pPatient ) );
    }

// ----------------------------------------
// --- MalariaSurveyJSONAnalyzer Methods
// ----------------------------------------

    BEGIN_QUERY_INTERFACE_BODY( MalariaSurveyJSONAnalyzer )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IReport )
        HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_BODY( MalariaSurveyJSONAnalyzer )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( MalariaSurveyJSONAnalyzer )
#endif

    MalariaSurveyJSONAnalyzer::MalariaSurveyJSONAnalyzer()
        : BaseEventReportIntervalOutput( _module,
                                         true,// true => one file per report
                                         new PatientMap(),
                                         new PatientMap(),
                                         true, // true = use Min/Max Age filter
                                         true ) // true = use other human filters (ip/intervention)
        , m_IPKeyToCollect()
        , m_pPatientMap(nullptr)
        , m_DetectionThresholds()
        , m_pRNG( nullptr )
    {
#ifdef _REPORT_DLL
        m_pRNG = DLL_HELPER.CreateRandomNumberGenerator( EnvPtr );
#else
        m_pRNG = RandomNumberGeneratorFactory::CreateRandomNumberGeneratorForReport();
#endif

        initSimTypes( 1, "MALARIA_SIM" );
    }

    MalariaSurveyJSONAnalyzer::~MalariaSurveyJSONAnalyzer()
    {
        delete m_pRNG;
    }

    bool MalariaSurveyJSONAnalyzer::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "IP_Key_To_Collect", &m_IPKeyToCollect, Survey_IP_Key_To_Collect_DESC_TEXT, "" );

        bool configured = BaseEventReportIntervalOutput::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            m_pPatientMap = static_cast<PatientMap*>(m_pIntervalData);
        }
        return configured;
    }

    void MalariaSurveyJSONAnalyzer::Initialize( unsigned int nrmSize )
    {
        if( !m_IPKeyToCollect.empty() )
        {
            // If user provides a key, ensure it is valid.
            IPKey key( m_IPKeyToCollect );
            release_assert( key.IsValid() ); // an exception should be thrown in the line above
        }

        BaseEventReportIntervalOutput::Initialize( nrmSize );
    }

    bool MalariaSurveyJSONAnalyzer::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        if( HaveUnregisteredAllEvents() || !report_filter.IsValidHuman( context->GetIndividualHumanConst() ) )
        {
            return false ;
        }
        m_has_data = true;

        LOG_DEBUG_F( "MalariaSurveyAnalyzer notified of event by %d-year old individual.\n", (int) (context->GetAge() / DAYSPERYEAR) );

        // individual context for suid
        IIndividualHumanContext * iindividual = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&iindividual) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanContext", "IIndividualHumanEventContext");
        }

        int id           = iindividual->GetSuid().data;
        double mc_weight = context->GetMonteCarloWeight();
        double age       = context->GetAge();

        std::string ip_info;
        IPKeyValueContainer* p_props = iindividual->GetEventContext()->GetProperties();
        if( m_IPKeyToCollect.empty() )
        {
            ip_info = p_props->ToString();
        }
        else
        {
            ip_info = p_props->Get( IPKey( m_IPKeyToCollect ) ).ToString();
        }

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        // get the correct existing patient or insert a new one
        Patient* patient = m_pPatientMap->FindPatient(id);

        if( patient == nullptr )
        {
            patient = new Patient(id, age, m_interval_timer-age); // birthday relative to current interval (i.e. output file)
            m_pPatientMap->Add( patient );

            patient->strain_ids = individual_malaria->GetInfectingStrainIds();
            INodeEventContext* node_context = context->GetNodeEventContext();
            patient->node_id = node_context->GetExternalId();
        }

        patient->ip_data.push_back( ip_info );

        // Push back today's disease variables
        float max_fever = susceptibility_malaria->GetMaxFever();
        if( max_fever < m_DetectionThresholds[ MalariaDiagnosticType::FEVER ] )
        {
            max_fever = 0.0f;
        }
        patient->fever.push_back( max_fever > 0 ? max_fever + 37.0f : -1.0f );

        // GetInfectiousness is an inline function of IndividualHuman (not belonging to any queriable interface presently)
        float infectiousness = static_cast<IndividualHuman*>(context)->GetInfectiousness();
        float infectiousness_smeared = ReportUtilitiesMalaria::BinomialInfectiousness( m_pRNG, infectiousness );
        float infectiousness_age_scaled = infectiousness * SusceptibilityVector::SurfaceAreaBitingFunction( age );
        patient->infectiousness.push_back( infectiousness );
        patient->infectiousness_smeared.push_back( infectiousness_smeared );
        patient->infectiousness_age_scaled.push_back( infectiousness_age_scaled );

        // True values in model
        patient->true_asexual_density.push_back( susceptibility_malaria->get_parasite_density() );
        patient->true_gametocyte_density.push_back( individual_malaria->GetGametocyteDensity() );

        // Smeared true values in model
        patient->smeared_true_asexual_density.push_back( ReportUtilitiesMalaria::NASBADensityWithUncertainty( m_pRNG, susceptibility_malaria->get_parasite_density()));
        patient->smeared_true_gametocyte_density.push_back( ReportUtilitiesMalaria::NASBADensityWithUncertainty( m_pRNG, individual_malaria->GetGametocyteDensity()));

        // Values incorporating variability and sensitivity of blood test
        patient->asexual_parasite_density.push_back( individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::BLOOD_SMEAR_PARASITES   ) );
        patient->gametocyte_density.push_back(       individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::BLOOD_SMEAR_GAMETOCYTES ) );
        patient->pcr_parasite_density.push_back(     individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::PCR_PARASITES           ) );
        patient->pcr_gametocyte_density.push_back(   individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::PCR_GAMETOCYTES         ) );
        patient->pfhrp2.push_back(                   individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::PF_HRP2                 ) );

        // Positive fields of view (out of 200 views in Garki-like setup)
        int positive_asexual_fields = 0;
        float parasite_density = susceptibility_malaria->get_parasite_density();
        ReportUtilitiesMalaria::CountPositiveSlideFields( m_pRNG, parasite_density, 200, 1.0f / 400, positive_asexual_fields );
        patient->pos_asexual_fields.push_back( (float)positive_asexual_fields );

        int positive_gametocyte_fields = 0;
        float gametocyte_density = individual_malaria->GetGametocyteDensity();
        ReportUtilitiesMalaria::CountPositiveSlideFields( m_pRNG, gametocyte_density, 200, 1.0f / 400, positive_gametocyte_fields );
        patient->pos_gametocyte_fields.push_back( (float)positive_gametocyte_fields );

        LOG_DEBUG_F("(a,g) = (%d,%d)\n", (int)positive_asexual_fields, (int)positive_gametocyte_fields);

        // Values incorporating variability and sensitivity of blood test with uncertainty
        patient->smeared_asexual_parasite_density.push_back(ReportUtilitiesMalaria::FieldsOfViewToDensity((float)positive_asexual_fields));
        patient->smeared_gametocyte_density.push_back(ReportUtilitiesMalaria::FieldsOfViewToDensity((float)positive_gametocyte_fields));

        return true;
    }

    void MalariaSurveyJSONAnalyzer::SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js )
    {
        output.Insert("ntsteps", m_reporting_interval);

        release_assert( m_pPatientMap );
        m_pPatientMap->Serialize( output, js );
    }

    void MalariaSurveyJSONAnalyzer::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
        m_DetectionThresholds = rDetectionThresholds;
    }
}