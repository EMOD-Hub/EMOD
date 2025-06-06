
#pragma once

#include "InterpolatedValueMap.h"
#include "HIVARTStagingAbstract.h"

namespace Kernel
{
    class IDMAPI HIVARTStagingByCD4Diagnostic : public HIVARTStagingAbstract
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVARTStagingByCD4Diagnostic, IDistributableIntervention)

    public: 
        HIVARTStagingByCD4Diagnostic();
        HIVARTStagingByCD4Diagnostic( const HIVARTStagingByCD4Diagnostic& );

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        virtual bool MakeDecision( IIndividualHumanHIV *pHIV, 
                                   float year, 
                                   float CD4count, 
                                   bool hasActiveTB, 
                                   bool isPregnant ) override;

        InterpolatedValueMap threshold;
        InterpolatedValueMap ifActiveTB;
        InterpolatedValueMap ifPregnant;

        DECLARE_SERIALIZABLE(HIVARTStagingByCD4Diagnostic);
    };
}
