
#include "stdafx.h"
#include <math.h>

#include "Common.h"
#include "SusceptibilityVector.h"

SETUP_LOGGING( "SusceptibilityVector" )

namespace Kernel
{
    AgeDependentBitingRisk::Enum SusceptibilityVectorConfig::age_dependent_biting_risk_type = AgeDependentBitingRisk::OFF;
    float  SusceptibilityVectorConfig::m_newborn_biting_risk = 0.2f;
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Vector.Susceptibility,SusceptibilityVectorConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityVectorConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityVectorConfig)

    bool SusceptibilityVectorConfig::Configure( const Configuration* config )
    {
        initConfig( "Age_Dependent_Biting_Risk_Type", age_dependent_biting_risk_type, config, MetadataDescriptor::Enum("age_dependent_biting_risk_type", Age_Dependent_Biting_Risk_Type_DESC_TEXT, MDD_ENUM_ARGS(AgeDependentBitingRisk)) );
        initConfigTypeMap( "Newborn_Biting_Risk_Multiplier", &m_newborn_biting_risk, Newborn_Biting_Risk_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.2f, "Age_Dependent_Biting_Risk_Type", "LINEAR" );
        return JsonConfigurable::Configure( config );
    }

    SusceptibilityVector::SusceptibilityVector() : Kernel::Susceptibility()
    {
    }

    SusceptibilityVector::SusceptibilityVector(IIndividualHumanContext *context) : Kernel::Susceptibility(context)
    {
    }

    BEGIN_QUERY_INTERFACE_DERIVED(SusceptibilityVector, Susceptibility)
        HANDLE_INTERFACE(IVectorSusceptibilityContext)
    END_QUERY_INTERFACE_DERIVED(SusceptibilityVector, Susceptibility)

    void SusceptibilityVector::Initialize(float immmod, float riskmod)
    {
        Susceptibility::Initialize(immmod, riskmod);
        m_relative_biting_rate = riskmod;
        m_age_dependent_biting_risk = BitingRiskAgeFactor();
    }

    SusceptibilityVector *SusceptibilityVector::CreateSusceptibility(IIndividualHumanContext *context, float immmod, float riskmod)
    {
        SusceptibilityVector *newsusceptibility = _new_ SusceptibilityVector(context);
        newsusceptibility->Initialize(immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilityVector::~SusceptibilityVector()
    {
    }

    void SusceptibilityVector::Update(float dt)
    {
        // base-class behavior
        Susceptibility::Update(dt);

        // update age-based biting risk
        m_age_dependent_biting_risk = BitingRiskAgeFactor();
    }

    void SusceptibilityVector::SetRelativeBitingRate( float rate )
    {
        m_relative_biting_rate = rate;
    }

    float SusceptibilityVector::GetRelativeBitingRate(void) const
    {
        return m_relative_biting_rate * m_age_dependent_biting_risk;
    }

    float SusceptibilityVector::BitingRiskAgeFactor()
    {
        float risk = 1.0f;
        switch(SusceptibilityVectorConfig::age_dependent_biting_risk_type)
        {
            case AgeDependentBitingRisk::OFF:
                // risk is independent by age
                break;

            case AgeDependentBitingRisk::LINEAR:
                risk = LinearBitingFunction(GetParent()->GetAge());
                break;

            case AgeDependentBitingRisk::SURFACE_AREA_DEPENDENT:
                risk = SurfaceAreaBitingFunction(GetParent()->GetAge());
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, 
                    "age_dependent_biting_risk_type", SusceptibilityVectorConfig::age_dependent_biting_risk_type, 
                    AgeDependentBitingRisk::pairs::lookup_key(SusceptibilityVectorConfig::age_dependent_biting_risk_type) );
        }

        LOG_DEBUG_F("Age-dependent biting-risk = %f for %0.2f-year-old individual.\n", risk, GetParent()->GetAge()/DAYSPERYEAR);
        return risk;
    }

    float SusceptibilityVector::LinearBitingFunction(float input_age)
    {
        if ( input_age < 20 * DAYSPERYEAR )
        {
            // linear from birth to age 20 years
            float newborn_risk = SusceptibilityVectorConfig::m_newborn_biting_risk;
            return newborn_risk + input_age * (1 - newborn_risk) / (20 * DAYSPERYEAR);
        }

        return 1.0f;
    }

    REGISTER_SERIALIZABLE(SusceptibilityVector);

    void SusceptibilityVector::serialize(IArchive& ar, SusceptibilityVector* obj)
    {
        SusceptibilityVector& susceptibility = *obj;
        Susceptibility::serialize(ar, obj);
        ar.labelElement("m_relative_biting_rate") & susceptibility.m_relative_biting_rate;
        ar.labelElement("m_age_dependent_biting_risk") & susceptibility.m_age_dependent_biting_risk;
    }
}
