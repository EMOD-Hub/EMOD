
#pragma once

#include "HIVSimpleDiagnostic.h"
#include "IHIVInterventionsContainer.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
    struct IHIVMedicalHistory;

    ENUM_DEFINE( SensitivityType,
                 ENUM_VALUE_SPEC( SINGLE_VALUE, 1 )
                 ENUM_VALUE_SPEC( VERSUS_TIME,  2 ) )

    class HIVRapidHIVDiagnostic : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVRapidHIVDiagnostic, IDistributableIntervention)

    public: 
        HIVRapidHIVDiagnostic();
        HIVRapidHIVDiagnostic( const HIVRapidHIVDiagnostic& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Configure( const Configuration* inputJson ) override;

    protected:
        virtual bool positiveTestResult() override;
        virtual void onNegativeTestResult() override;
        virtual void positiveTestDistribute() override;
        virtual void onReceivedResult( IHIVMedicalHistory* pMedHistory, bool resultIsHivPositive );

        float m_ProbReceivedResults;
        SensitivityType::Enum m_SensitivityType;
        InterpolatedValueMap m_SensitivityVersusTime;

        DECLARE_SERIALIZABLE(HIVRapidHIVDiagnostic);
    };
}
