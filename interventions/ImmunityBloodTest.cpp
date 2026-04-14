
#include "stdafx.h"
#include "ImmunityBloodTest.h"
#include "IIndividualHumanContext.h"
#include "IIndividualHuman.h"
#include "IndividualEventContext.h"

SETUP_LOGGING("ImmunityBloodTest")

const float ImmuneThreshold = 1.0f; //threshold to be considered immune

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(ImmunityBloodTest, StandardDiagnostic)
    END_QUERY_INTERFACE_DERIVED(ImmunityBloodTest, StandardDiagnostic)
    IMPLEMENT_FACTORY_REGISTERED(ImmunityBloodTest)

    ImmunityBloodTest::ImmunityBloodTest() 
        : StandardDiagnostic( false )
        , threshold_acquisitionImmunity(ImmuneThreshold)
    {
        initSimTypes(1, "*");
    }

    ImmunityBloodTest::ImmunityBloodTest(const ImmunityBloodTest& master) 
        : StandardDiagnostic(master)
        , threshold_acquisitionImmunity(master.threshold_acquisitionImmunity)
    {
    }

    ImmunityBloodTest::~ImmunityBloodTest()
    {
        LOG_DEBUG("Destructing ImmunityBloodTest \n");
    }

    bool ImmunityBloodTest::Configure(const Configuration * inputJson)
    {
        initConfigTypeMap("Positive_Threshold_Acquisition_Immunity", &threshold_acquisitionImmunity, IBT_Positive_Threshold_Acquisition_Immunity_DESC_TEXT, 0.0, 1.0, ImmuneThreshold);

        return StandardDiagnostic::Configure(inputJson);
    }

    EventOrConfig::Enum ImmunityBloodTest::getEventOrConfig( const Configuration * inputJson )
    {
        return EventOrConfig::Event;
    }

    void ImmunityBloodTest::ConfigurePositiveConfig( const Configuration * inputJson )
    {
        // do nothing because we don't want the config part
    }

    void ImmunityBloodTest::ConfigureNegativeConfig( const Configuration * inputJson )
    {
        // do nothing because we don't want the config part
    }

    void ImmunityBloodTest::CheckEventsConfigs( const Configuration * inputJson )
    {
        StandardDiagnostic::CheckEventsConfigs( inputJson );

        if( !JsonConfigurable::_dryrun )
        {
            if( negative_diagnosis_event.IsUninitialized() && positive_diagnosis_event.IsUninitialized() )
            {
                std::stringstream ss;
                ss << "Neither Positive_Diagnosis_Event nor Negative_Diagnosis_Event is defined." << std::endl;
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            //else if( positive_diagnosis_event.IsUninitialized() )
            //{
            //    LOG_INFO( "Positive_Diagnosis_Event is not defined. No notification will be given for this event.\n" );
            //}
            else if( negative_diagnosis_event.IsUninitialized() )
            {
                LOG_INFO( "Negative_Diagnosis_Event is not defined. No notification will be given for this event.\n" );
            }
        }
    }

    bool ImmunityBloodTest::positiveTestResult()
    {
        const IIndividualHuman* p_iih = parent->GetEventContext()->GetIndividualHumanConst();

        // return true if individual has natural immunity or immunity acquired by intervention
        float acquisitionSusceptibility = p_iih->GetAcquisitionImmunity();   // actually susceptibility 
        bool has_attribute = (acquisitionSusceptibility <= (1.0 - threshold_acquisitionImmunity));
        LOG_DEBUG_F("acquisitionModifier = %f,  has_attribute = %d  threshold__AcquisitionImmunity = %f\n", acquisitionSusceptibility, has_attribute, threshold_acquisitionImmunity);

        return applySensitivityAndSpecificity(has_attribute);
    }

    REGISTER_SERIALIZABLE(ImmunityBloodTest);

    void ImmunityBloodTest::serialize(IArchive& ar, ImmunityBloodTest* obj)
    {
        StandardDiagnostic::serialize(ar, obj);
        ImmunityBloodTest& ibt = *obj;
        ar.labelElement("threshold_acquisitionImmunity") & ibt.threshold_acquisitionImmunity;
    }
}
