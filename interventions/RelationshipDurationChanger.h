
#pragma once

#include "AbstractSocietyOverrideIntervention.h"

namespace Kernel
{
    struct INodeSTIInterventionEffectsApply;

    class RelationshipDurationChanger : public AbstractSocietyOverrideIntervention
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, RelationshipDurationChanger, INodeDistributableIntervention) 

    public:        
        RelationshipDurationChanger();
        RelationshipDurationChanger( const RelationshipDurationChanger& );
        virtual ~RelationshipDurationChanger();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        // AbstractSocietyOverrideIntervention
        virtual void ApplyOverride() override;

        float m_OverrideDurationHeterogeniety;
        float m_OverrideDurationScale;
    };

}

