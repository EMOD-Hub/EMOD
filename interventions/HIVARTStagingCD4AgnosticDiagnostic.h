
#pragma once

#include "InterpolatedValueMap.h"
#include "HIVARTStagingAbstract.h"

namespace Kernel
{
    class IDMAPI HIVARTStagingCD4AgnosticDiagnostic : public HIVARTStagingAbstract
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVARTStagingCD4AgnosticDiagnostic, IDistributableIntervention)

    public: 
        HIVARTStagingCD4AgnosticDiagnostic();
        HIVARTStagingCD4AgnosticDiagnostic( const HIVARTStagingCD4AgnosticDiagnostic& );

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        virtual bool MakeDecision( IIndividualHumanHIV *pHIV, 
                                   float year, 
                                   float CD4count, 
                                   bool hasActiveTB, 
                                   bool isPregnant ) override;

        virtual bool TestAdult( float WHO_Stage, float year, bool hasActiveTB, bool isPregnant );
        virtual bool TestChild( float WHO_Stage, float year, bool hasActiveTB, float ageDays   );

        float adultAge;

        InterpolatedValueMap adultByWHOStage,  childByWHOStage;
        InterpolatedValueMap adultByTB,        childByTB;
        InterpolatedValueMap adultByPregnant;
        InterpolatedValueMap childTreatUnderAgeThreshold;

        DECLARE_SERIALIZABLE(HIVARTStagingCD4AgnosticDiagnostic);
    };
}
