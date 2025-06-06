
#pragma once

#include "Diagnostics.h"

namespace Kernel
{
    class StiCoInfectionDiagnostic : public SimpleDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, StiCoInfectionDiagnostic, IDistributableIntervention)

    public: 
        StiCoInfectionDiagnostic();
        StiCoInfectionDiagnostic( const StiCoInfectionDiagnostic& );
        virtual ~StiCoInfectionDiagnostic();

    protected:
        virtual bool positiveTestResult() override;

        DECLARE_SERIALIZABLE(StiCoInfectionDiagnostic);
    };
}
