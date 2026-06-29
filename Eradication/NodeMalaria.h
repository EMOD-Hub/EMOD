
#pragma once

#include "Common.h"
#include "MalariaContexts.h"
#include "NodeVector.h"

namespace Kernel
{
    class NodeMalaria : public NodeVector, INodeMalaria
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static NodeMalaria *CreateNode(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid);
        virtual ~NodeMalaria();

        virtual float GetNewClinicalCases()         const override { return m_New_Clinical_Cases; } 
        virtual float GetNewSevereCases()           const override { return m_New_Severe_Cases; }
        virtual float GetMaternalAntibodyFraction() const override { return m_Maternal_Antibody_Fraction; }

        virtual void SetupEventContextHost() override;
        virtual bool Configure( const Configuration* config ) override;

        virtual IIndividualHuman* addNewIndividual( float = 1.0f, float = 0.0f, int = 0, int = 0, float = 1.0f, float = 1.0f, float = 1.0f) override;

    protected:
        // These vectors should be dimensioned by MalariaDiagnosticType

        float m_New_Clinical_Cases;
        float m_New_Severe_Cases;
        float m_Maternal_Antibody_Fraction;

        virtual IIndividualHuman* createHuman( suids::suid id, float MCweight, float init_age, int gender) override;

        virtual void updatePopulationStatistics(float = 1.0f) override;
        virtual void accumulateIndividualPopulationStatistics(float dt, IIndividualHuman* individual);
        virtual void updateNodeStateCounters(IIndividualHuman *ih);
        virtual void resetNodeStateCounters(void) override;

        /* clorton virtual */ const SimulationConfig *params() /* clorton override */;

        NodeMalaria();
        NodeMalaria(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid);
        virtual void Initialize() override;

        virtual INodeContext *getContextPointer() override { return static_cast<INodeContext*>(this); }

        DECLARE_SERIALIZABLE(NodeMalaria);
    };
}
