
#pragma once

#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"
#include "VectorEnums.h"
#include "VectorDefs.h"
#include "Common.h"

namespace Kernel
{
    ENUM_DEFINE( EIRType,
                 ENUM_VALUE_SPEC( MONTHLY, 0 )
                 ENUM_VALUE_SPEC( DAILY, 1 ) )

        class InputEIR : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(Outbreak)
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, InputEIR, INodeDistributableIntervention)

    public:
        InputEIR();
        InputEIR( const InputEIR& master );
        virtual ~InputEIR() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Distribute( INodeEventContext *pNodeContext, IEventCoordinator2 *pEC ) override;
        virtual void Update(float dt) override;

        // IBaseIntervention
        virtual float GetCostPerUnit() const override;
    protected:
        AgeDependentBitingRisk::Enum age_dependence;
        EIRType::Enum eir_type;
        std::vector<float> monthly_EIR; // 12 values of EIR by month
        std::vector<float> daily_EIR;
        float scaling_factor;
        float today;
        tAgeBitingFunction risk_function;
    };
}
