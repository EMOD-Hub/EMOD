
#pragma once


#include "AbstractSocietyOverrideIntervention.h"

namespace Kernel
{
    struct INodeSTIInterventionEffectsApply;

    class CoitalActRateChanger : public AbstractSocietyOverrideIntervention
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, CoitalActRateChanger, INodeDistributableIntervention) 

    public:        
        CoitalActRateChanger();
        CoitalActRateChanger( const CoitalActRateChanger& );
        virtual ~CoitalActRateChanger();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        // AbstractSocietyOverrideIntervention
        virtual void ApplyOverride() override;

        float m_OverrideCoitalActRate;
    };

}

