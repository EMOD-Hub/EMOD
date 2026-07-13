
#pragma once
#include "Susceptibility.h"

namespace Kernel
{
    class SusceptibilitySTIConfig : public SusceptibilityConfig
    {
    protected:
        friend class SusceptibilitySTI;
    };
    
    class SusceptibilitySTI : public Susceptibility
    {
    public:
        virtual ~SusceptibilitySTI(void);
        static SusceptibilitySTI *CreateSusceptibility(IIndividualHumanContext *context, float immmod, float riskmod);

    protected:

        SusceptibilitySTI();
        SusceptibilitySTI(IIndividualHumanContext *context);
        virtual void Initialize(float immmod, float riskmod) override;

        DECLARE_SERIALIZABLE(SusceptibilitySTI);
    };
}
