
#pragma once

#include "AbstractDecision.h"

namespace Kernel
{
    class HIVSigmoidByYearAndSexDiagnostic : public AbstractDecision
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVSigmoidByYearAndSexDiagnostic, IDistributableIntervention)

    public: 
        HIVSigmoidByYearAndSexDiagnostic();
        HIVSigmoidByYearAndSexDiagnostic( const HIVSigmoidByYearAndSexDiagnostic& );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual bool MakeDecision( float dt ) override;

    protected:

        float rampMin;
        float rampMax;
        float rampMidYear;
        float rampRate;
        float femaleMultiplier;

        DECLARE_SERIALIZABLE(HIVSigmoidByYearAndSexDiagnostic);
    };
}
