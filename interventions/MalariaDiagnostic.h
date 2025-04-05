
#pragma once

#include <string>
#include <list>
#include <vector>

#include "StandardDiagnostic.h"
#include "MalariaEnums.h"

namespace Kernel
{
    class MalariaDiagnostic : public StandardDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MalariaDiagnostic, IDistributableIntervention)

    public: 
        MalariaDiagnostic();
        MalariaDiagnostic( const MalariaDiagnostic& );

        virtual bool Configure( const Configuration* pConfig ) override;
        virtual ~MalariaDiagnostic();

    protected:
        virtual void ConfigureSensitivitySpecificity( const Configuration* inputJson ) override;
        virtual bool positiveTestResult() override;

        MalariaDiagnosticType::Enum malaria_diagnostic_type;
        float measurement_sensitivity;
        float detection_threshold;

        DECLARE_SERIALIZABLE(MalariaDiagnostic);
    };
}


