
#pragma once


#include "AbstractSocietyOverrideIntervention.h"
#include "Sigmoid.h"

namespace Kernel
{
    struct INodeSTIInterventionEffectsApply;

    class CondomUsageProbabilityChanger : public AbstractSocietyOverrideIntervention
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, CondomUsageProbabilityChanger, INodeDistributableIntervention) 

    public:        
        CondomUsageProbabilityChanger();
        CondomUsageProbabilityChanger( const CondomUsageProbabilityChanger& );
        virtual ~CondomUsageProbabilityChanger();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        // AbstractSocietyOverrideIntervention
        virtual void ApplyOverride() override;

        Sigmoid m_OverridingCondomUsage;
    };

}

