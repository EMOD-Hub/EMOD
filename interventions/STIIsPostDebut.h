
#pragma once

#include "StandardDiagnostic.h"

namespace Kernel
{
    class IDMAPI STIIsPostDebut : public StandardDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, STIIsPostDebut, IDistributableIntervention)

    public:
        STIIsPostDebut();
        STIIsPostDebut( const STIIsPostDebut& ); // copy ctor

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // SimpleDiagnostic
        virtual void ConfigureSensitivitySpecificity( const Configuration* inputJson ) override;
        virtual void ConfigureOther( const Configuration* inputJson ) override;
        virtual bool positiveTestResult() override;    // Test if recipient "tests positive"

    protected:
        DECLARE_SERIALIZABLE(STIIsPostDebut);
    };
}
