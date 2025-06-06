
#pragma once

#include "StandardDiagnostic.h"

namespace Kernel
{
    class ImmunityBloodTest : public StandardDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, ImmunityBloodTest, IDistributableIntervention)

    public:
        ImmunityBloodTest();
        ImmunityBloodTest(const ImmunityBloodTest&);
        virtual ~ImmunityBloodTest();

        virtual bool Configure(const Configuration* pConfig) override;
        virtual bool positiveTestResult() override;

    protected:
        virtual EventOrConfig::Enum getEventOrConfig( const Configuration * inputJson ) override;
        virtual void ConfigurePositiveConfig( const Configuration * inputJson ) override;
        virtual void ConfigureNegativeConfig( const Configuration * inputJson ) override;
        virtual void CheckEventsConfigs( const Configuration * inputJson ) override;

        float threshold_acquisitionImmunity;

        DECLARE_SERIALIZABLE(ImmunityBloodTest);
    };
}
