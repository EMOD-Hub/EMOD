
#pragma once

#include "InterpolatedValueMap.h"
#include "AbstractDecision.h"

namespace Kernel
{
    class HIVPiecewiseByYearAndSexDiagnostic : public AbstractDecision
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVPiecewiseByYearAndSexDiagnostic, IDistributableIntervention)

    public: 
        HIVPiecewiseByYearAndSexDiagnostic();
        HIVPiecewiseByYearAndSexDiagnostic( const HIVPiecewiseByYearAndSexDiagnostic& );

        virtual bool Configure( const Configuration* pConfig ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        virtual bool MakeDecision( float dt ) override;

    protected:
        int interpolation_order;
        float female_multiplier;
        float default_value;
        InterpolatedValueMap year2ValueMap;

        DECLARE_SERIALIZABLE(HIVPiecewiseByYearAndSexDiagnostic);
    };
}
