
#pragma once


#include "AbstractSocietyOverrideIntervention.h"

namespace Kernel
{
    struct INodeSTIInterventionEffectsApply;

    class RelationshipFormationRateChanger : public AbstractSocietyOverrideIntervention
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, RelationshipFormationRateChanger, INodeDistributableIntervention) 

    public:        
        RelationshipFormationRateChanger();
        RelationshipFormationRateChanger( const RelationshipFormationRateChanger& );
        virtual ~RelationshipFormationRateChanger();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        // AbstractSocietyOverrideIntervention
        virtual void ApplyOverride() override;

        float m_FormationRate;
    };

}

