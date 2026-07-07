
#include "stdafx.h"

#include "SimulationMalaria.h"
#include "IndividualMalaria.h"
#include "NodeMalaria.h"
#include "NodeMalariaCoTransmission.h"
#include "NodeMalariaGenetics.h"
#include "ReportMalariaGenetics.h"
#include "ReportMalariaCoTran.h"
#include "ReportEventRecorderMalariaCoTran.h"
#include "ReportEventRecorderMalaria.h"
#include "ReportMalaria.h"
#include "BinnedReportMalaria.h"
#include "SpatialReportMalaria.h"
#include "FactorySupport.h"
#include "ProgVersion.h"
#include "IReportMalariaDiagnostics.h"
#include "ParasiteGenetics.h"
#include "IndividualMalariaGenetics.h"
#include "MalariaDrugTypeParameters.h"
#include "SusceptibilityMalaria.h"

#pragma warning(disable : 4996)

SETUP_LOGGING( "SimulationMalaria" )

#ifdef _MALARIA_DLL

// Note: _diseaseType has to match the Simulation_Type name in config.json
static std::string _diseaseType = "MALARIA_SIM";

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

//
// This is the interface function from the DTK.
//
namespace Kernel
{
    DTK_DLLEXPORT ISimulation* __cdecl
    CreateSimulation(
        const Environment * pEnv 
    )
    {
        Environment::setInstance(const_cast<Environment*>(pEnv));
        LOG_INFO("CreateSimulation called\n");
        return Kernel::SimulationMalaria::CreateSimulation( EnvPtr->Config );
    }
}

DTK_DLLEXPORT
const char * __cdecl
GetDiseaseType()
{
    LOG_INFO_F("GetDiseaseType called for %s\n", _module);
    return _diseaseType.c_str();
}

DTK_DLLEXPORT
char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    Environment::setInstance(const_cast<Environment*>(pEnv));
    ProgDllVersion pv;
    LOG_INFO_F("GetEModuleVersion called with ver=%s\n", pv.getVersion());
    if (sVer) strcpy(sVer, pv.getVersion());
    return sVer;
}

DTK_DLLEXPORT
const char* __cdecl
GetSchema()
{
    LOG_DEBUG_F("GetSchema called for %s: map has size %d\n", _module, Kernel::JsonConfigurable::get_registration_map().size() );
    
    json::Object configSchemaAll;
    std::ostringstream schemaJsonString;
    for (auto& entry : Kernel::JsonConfigurable::get_registration_map())
    {
        const std::string classname = entry.first;
        LOG_DEBUG_F( "classname = %s\n", classname.c_str() );
        json::QuickBuilder config_schema = ((*(entry.second))());
        configSchemaAll[classname] = config_schema;
    }

    json::Writer::Write( configSchemaAll, schemaJsonString );

    putenv( ( std::string( "GET_SCHEMA_RESULT=" ) + schemaJsonString.str().c_str() ).c_str() );
    return schemaJsonString.str().c_str();
}

#ifdef __cplusplus
}
#endif

#endif

namespace Kernel
{
    ///// Simulation /////

    GET_SCHEMA_STATIC_WRAPPER_IMPL( Simulation.Malaria, SimulationMalaria )

    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(SimulationMalaria, SimulationVector)
        HANDLE_INTERFACE( IMalariaSimulationContext )
    END_QUERY_INTERFACE_DERIVED(SimulationMalaria, SimulationVector)

    SimulationMalaria::SimulationMalaria()
        : SimulationVector()
        , m_DetectionThresholds()
        , m_MalariaModel( MalariaModel::MALARIA_MECHANISTIC_MODEL )
        , m_ParasiteCohortSuidGenerator( EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks )
        , m_VectorBiteSuidGenerator( EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks )
    {
        reportClassCreator = ReportMalaria::CreateReport;
        eventReportClassCreator = ReportEventRecorderMalaria::CreateReport;
        binnedReportClassCreator = BinnedReportMalaria::CreateReport;
        spatialReportClassCreator = SpatialReportMalaria::CreateReport;

        m_DetectionThresholds.resize( MalariaDiagnosticType::pairs::count(), 0.0f );

        m_DetectionThresholds[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES   ] = 0.0f;
        m_DetectionThresholds[ MalariaDiagnosticType::BLOOD_SMEAR_GAMETOCYTES ] = 0.0f;
        m_DetectionThresholds[ MalariaDiagnosticType::PCR_PARASITES           ] = 0.05f;
        m_DetectionThresholds[ MalariaDiagnosticType::PCR_GAMETOCYTES         ] = 0.05f;
        m_DetectionThresholds[ MalariaDiagnosticType::PF_HRP2                 ] = 5.0f;
        m_DetectionThresholds[ MalariaDiagnosticType::TRUE_PARASITE_DENSITY   ] = 0.0f;
        m_DetectionThresholds[ MalariaDiagnosticType::FEVER                   ] = 1.0f;
    }

    void SimulationMalaria::Initialize(const ::Configuration *config)
    {
        SimulationVector::Initialize(config);
        if( m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS )
        {
            IndividualHumanMalariaGenetics::InitializeStaticsMalariaGenetics( config );
        }
        else
        {
            IndividualHumanMalaria::InitializeStaticsMalaria( config );
        }

        for( IReport* p_report : reports )
        {
            // broadcast the event
            IReportMalariaDiagnostics* p_irmd = nullptr;
            if( s_OK == p_report->QueryInterface( GET_IID( IReportMalariaDiagnostics ), (void**)&p_irmd ) )
            {
                p_irmd->SetDetectionThresholds( m_DetectionThresholds );
            }
        }
    }

    SimulationMalaria *SimulationMalaria::CreateSimulation()
    {
        return _new_ SimulationMalaria();
    }

    SimulationMalaria *SimulationMalaria::CreateSimulation(const ::Configuration *config)
    {
        SimulationMalaria *newsimulation = _new_ SimulationMalaria();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Malaria simulations do not currently support heterogeneous intra-node transmission.");
            }
        }

        return newsimulation;
    }

    bool SimulationMalaria::ValidateConfiguration(const ::Configuration *config)
    {
        return Kernel::SimulationVector::ValidateConfiguration(config);
    }

    SimulationMalaria::~SimulationMalaria()
    {
        // Node deletion handled by ~Kernel::Simulation()
        // no need to delete flags
        //delete m_strain_identity_flags;
    }

    void SimulationMalaria::InitializeFlags(const ::Configuration *config)
    {
    }

    bool SimulationMalaria::Configure( const Configuration * inputJson )
    {
        ParasiteGenetics* p_parasite_genetics = ParasiteGenetics::CreateInstance();

        initConfig( "Malaria_Model", m_MalariaModel, inputJson, MetadataDescriptor::Enum( "m_MalariaModel", Malaria_Model_DESC_TEXT, MDD_ENUM_ARGS( MalariaModel ) ) );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! I'm keeping the ownership of these variables in SusceptibilityMalariaConfig
        // !!! so that ParasiteGenetics and InfectionMalaria do not need to include
        // !!! SimlationMalaria.  They really belong in SusceptibilityMalariaConfig but
        // !!! they need to be configured before ParasiteGenetics which needs to be 
        // !!! configured before MalariaDrugParams. We don't have to do the same thing as
        // !!! with Malaria_Drug_Params because integers are configured before
        // !!! JsonConfigurable objects are.
        // !!! The -1 check is to see if these parameters have already been initialized via
        // !!! serialization.  If so, don't let the user change them.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if( JsonConfigurable::_dryrun || (SusceptibilityMalariaConfig::falciparumMSPVars == -1) )
        {
            initConfigTypeMap( "Falciparum_MSP_Variants",      &SusceptibilityMalariaConfig::falciparumMSPVars,      Falciparum_MSP_Variants_DESC_TEXT,      0, 1e3, DEFAULT_MSP_VARIANTS      );
            initConfigTypeMap( "Falciparum_Nonspecific_Types", &SusceptibilityMalariaConfig::falciparumNonSpecTypes, Falciparum_Nonspecific_Types_DESC_TEXT, 0, 1e3, DEFAULT_NONSPECIFIC_TYPES );
            initConfigTypeMap( "Falciparum_PfEMP1_Variants",   &SusceptibilityMalariaConfig::falciparumPfEMP1Vars,   Falciparum_PfEMP1_Variants_DESC_TEXT,   0, 1e5, DEFAULT_PFEMP1_VARIANTS   );
        }

        // see Malaria_Drug_Params below
        initConfigTypeMap( "Parasite_Genetics", p_parasite_genetics, Parasite_Genetics_DESC_TEXT, "Malaria_Model", "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS");
        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret )
        {
            initConfigTypeMap( "Report_Detection_Threshold_Blood_Smear_Parasites", &m_DetectionThresholds[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ], Report_Detection_Threshold_Blood_Smear_Parasites_DESC_TEXT, 0.0f, 50000.0f, 0.0f );
            initConfigTypeMap( "Report_Detection_Threshold_Blood_Smear_Gametocytes", &m_DetectionThresholds[ MalariaDiagnosticType::BLOOD_SMEAR_GAMETOCYTES ], Report_Detection_Threshold_Blood_Smear_Gametocytes_DESC_TEXT, 0.0f, 50000.0f, 0.0f );
            initConfigTypeMap( "Report_Detection_Threshold_PCR_Parasites", &m_DetectionThresholds[ MalariaDiagnosticType::PCR_PARASITES ], Report_Detection_Threshold_PCR_Parasites_DESC_TEXT, 0.0f, 50000.0f, 0.05f );
            initConfigTypeMap( "Report_Detection_Threshold_PCR_Gametocytes", &m_DetectionThresholds[ MalariaDiagnosticType::PCR_GAMETOCYTES ], Report_Detection_Threshold_PCR_Gametocytes_DESC_TEXT, 0.0f, 50000.0f, 0.05f );
            initConfigTypeMap( "Report_Detection_Threshold_PfHRP2", &m_DetectionThresholds[ MalariaDiagnosticType::PF_HRP2 ], Report_Detection_Threshold_PfHRP2_DESC_TEXT, 0.0f, 50000.0f, 5.0f );
            initConfigTypeMap( "Report_Detection_Threshold_True_Parasite_Density", &m_DetectionThresholds[ MalariaDiagnosticType::TRUE_PARASITE_DENSITY ], Report_Detection_Threshold_True_Parasite_Density_DESC_TEXT, 0.0f, 50000.0f, 0.0f );
            initConfigTypeMap( "Report_Detection_Threshold_Fever", &m_DetectionThresholds[ MalariaDiagnosticType::FEVER ], Report_Detection_Threshold_Fever_DESC_TEXT, 0.5f, 5.0f, 1.0f );

            ret = SimulationVector::Configure( inputJson );
            if( ret && !JsonConfigurable::_dryrun )
            {
                if( m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_CO_TRANSMISSION )
                {
                    reportClassCreator = ReportMalariaCoTran::CreateReport;
                    eventReportClassCreator = ReportEventRecorderMalariaCoTran::CreateReport;
                }
                else if( m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS )
                {
                    reportClassCreator = ReportMalariaGenetics::CreateReport;
                }
            }
        }

        MalariaDrugTypeCollection* p_malaria_drugs = MalariaDrugTypeCollection::GetInstanceNonConst();
        if( ret )
        {
            // ----------------------------------------------------------------------
            // --- This crazy thing is so that Parasite_Genetics is configured first.
            // --- Malaria_Drug_Params needs Parasite_Genetics for drug resistance.
            // ----------------------------------------------------------------------
            initConfigComplexCollectionType( "Malaria_Drug_Params", p_malaria_drugs, Malaria_Drug_Params_DESC_TEXT );

            ret = JsonConfigurable::Configure( inputJson );
            if( ret && !JsonConfigurable::_dryrun )
            {
                p_malaria_drugs->CheckConfiguration();
            }
        }

        return ret;
    }

    void SimulationMalaria::addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                        suids::suid node_suid,
                                                        NodeDemographicsFactory *nodedemographics_factory,
                                                        ClimateFactory *climate_factory )
    {
        INodeContext* node = nullptr;
        if( m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_CO_TRANSMISSION )
        {
            node = NodeMalariaCoTransmission::CreateNode( GetContextPointer(), externalNodeId, node_suid );
        }
        else if( m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS )
        {
            node = NodeMalariaGenetics::CreateNode( GetContextPointer(), externalNodeId, node_suid );
        }
        else
        {
            node = NodeMalaria::CreateNode( GetContextPointer(), externalNodeId, node_suid );
        }
        node->InitSuidGenerator(node_suid.data, nodedemographics_factory->GetNodeIDs().size());
        addNode_internal( node, nodedemographics_factory, climate_factory );
    }

    void SimulationMalaria::Update( float dt )
    {
        if( m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS )
        {
            ParasiteGenetics::CreateInstance()->ReduceGenomeMap();
        }
        SimulationVector::Update( dt );
    }


    suids::suid SimulationMalaria::GetNextParasiteSuid()
    {
        return m_ParasiteCohortSuidGenerator();
    }

    suids::suid SimulationMalaria::GetNextBiteSuid()
    {
        return m_VectorBiteSuidGenerator();
    }

    ISimulationContext* SimulationMalaria::GetContextPointer()
    {
        return (ISimulationContext*)this;
    }

    REGISTER_SERIALIZABLE(SimulationMalaria);

    void SimulationMalaria::serialize(IArchive& ar, SimulationMalaria* obj)
    {
        SimulationVector::serialize( ar, obj );
        SimulationMalaria& sim = *obj;
        ar.labelElement( "m_MalariaModel" ) & (uint32_t&)sim.m_MalariaModel;
        ar.labelElement( "m_ParasiteCohortSuidGenerator" ) & sim.m_ParasiteCohortSuidGenerator;
        ar.labelElement( "m_VectorBiteSuidGenerator" ) & sim.m_VectorBiteSuidGenerator;

        if( sim.m_MalariaModel == MalariaModel::MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS )
        {
            ar.labelElement("ParasiteGenetics"); ParasiteGenetics::serialize( ar );
        }

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We are serializing these because they have to be the same values when reading
        // !!! from a serialized population.  See SimulationMalaira::Configure() above for why here.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        ar.labelElement( "falciparumMSPVars"      ) & SusceptibilityMalariaConfig::falciparumMSPVars;
        ar.labelElement( "falciparumNonSpecTypes" ) & SusceptibilityMalariaConfig::falciparumNonSpecTypes;
        ar.labelElement( "falciparumPfEMP1Vars"   ) & SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
    }
} // end namespace Kernel
