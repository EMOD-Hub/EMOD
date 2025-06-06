
#pragma once

#include "AgeDiagnostic.h"

namespace Kernel
{
    class CD4Diagnostic : public AgeDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, CD4Diagnostic, IDistributableIntervention)

    public: 
        CD4Diagnostic();
        CD4Diagnostic( const CD4Diagnostic& );
        virtual ~CD4Diagnostic();

    protected:
        virtual void ConfigureRangeThresholds( const Configuration* inputJson ) override;
        virtual float GetValue() const override;

        DECLARE_SERIALIZABLE(CD4Diagnostic);
    };
}
