
#include "stdafx.h"
#include "MalariaDiagnostic.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "MalariaContexts.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "MalariaDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(MalariaDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(MalariaDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(MalariaDiagnostic)

    bool MalariaDiagnostic::Configure(const Configuration * inputJson)
    {
        initConfig( "Diagnostic_Type",
                    malaria_diagnostic_type,
                    inputJson,
                    MetadataDescriptor::Enum("Diagnostic_Type", MD_Diagnostic_Type_DESC_TEXT, MDD_ENUM_ARGS( MalariaDiagnosticType ) ) );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE:  I want to say that Measurement_Sensitivity is dependent on Diagnostic_Type=BLOOD_SMEAR_PARASITES or BLOOD_SMEAR_GAMETOCYTES
        // !!! but I can't.  If you don't specify Diagnostic_Type, the default is BLOOD_SMEAR_PARASITES but Measurement_Senstivity
        // !!! won't see Diagnostic_Type so will assume that Measurement_Sensitivity should be default.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        initConfigTypeMap("Measurement_Sensitivity", &measurement_sensitivity, MD_Measurement_Sensitvity_DESC_TEXT, 0.0f, 1e6, 0.1f );
        initConfigTypeMap("Detection_Threshold",     &detection_threshold,     MD_Detection_Threshold_DESC_TEXT,    0.0f, 1e6, 0.0f );

        bool configured = StandardDiagnostic::Configure(inputJson);
        return configured;
    }

    void MalariaDiagnostic::ConfigureSensitivitySpecificity( const Configuration* inputJson )
    {
        // disable Base_Specificity & Base_Sensitivity
    }

    MalariaDiagnostic::MalariaDiagnostic()
    : StandardDiagnostic()
    , malaria_diagnostic_type(MalariaDiagnosticType::BLOOD_SMEAR_PARASITES)
    , measurement_sensitivity(0.1f)
    , detection_threshold(0.0f)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    MalariaDiagnostic::MalariaDiagnostic( const MalariaDiagnostic& master )
        : StandardDiagnostic( master )
        , malaria_diagnostic_type(  master.malaria_diagnostic_type  )
        , measurement_sensitivity(  master.measurement_sensitivity  )
        , detection_threshold(      master.detection_threshold      )
    {
    }

    MalariaDiagnostic::~MalariaDiagnostic()
    { 
        LOG_DEBUG("Destructing Malaria Diagnostic \n");
    }

    bool MalariaDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        IMalariaHumanContext* individual_malaria = NULL;
        if(parent->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&individual_malaria ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent", "IMalariaHumanContext", "IIndividualHuman" );
        }

        float measurement = individual_malaria->MakeDiagnosticMeasurement( malaria_diagnostic_type,
                                                                           measurement_sensitivity );
        bool tested_positive = (measurement > detection_threshold);
        return tested_positive;
    }

    REGISTER_SERIALIZABLE(MalariaDiagnostic);

    void MalariaDiagnostic::serialize(IArchive& ar, MalariaDiagnostic* obj)
    {
        StandardDiagnostic::serialize( ar, obj );
        MalariaDiagnostic& md = *obj;
        ar.labelElement("malaria_diagnostic_type"  ) & (uint32_t&)md.malaria_diagnostic_type;
        ar.labelElement("measurement_sensitivity"  ) & md.measurement_sensitivity;
        ar.labelElement("detection_threshold"      ) & md.detection_threshold;
    }
}
