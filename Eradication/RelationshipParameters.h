
#pragma once

#include "IRelationshipParameters.h"
#include "Configure.h"

namespace Kernel 
{
    class RelationshipParameters : public IRelationshipParameters, public JsonConfigurable
    {
    public:
        RelationshipParameters( RelationshipType::Enum type );
        ~RelationshipParameters();

        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        virtual bool Configure( const Configuration* config ) override;

        RelationshipType::Enum GetType() const;

        virtual float GetCoitalActRate()                const;
        virtual float GetDurationWeibullHeterogeneity() const;
        virtual float GetDurationWeibullScale()         const;
        virtual const Sigmoid& GetCondomUsage()         const;

        virtual const std::vector<RelationshipMigrationAction::Enum>& GetMigrationActions() const;
        virtual const std::vector<float>& GetMigrationActionsCDF() const;

        virtual void SetOverrideCoitalActRate( float overrideRate ) override;
        virtual void SetOverrideCondomUsageProbability( const Sigmoid* pOverride ) override;
        virtual void SetOverrideRelationshipDuration( float heterogeniety, float scale ) override;

    private:
        RelationshipType::Enum m_Type;
        float m_CoitalActRate;
        float m_DurationWeibullHeterogeneity;
        float m_DurationWeibullScale;
        Sigmoid m_CondomUsage;
        std::vector<RelationshipMigrationAction::Enum> m_MigrationActions;
        std::vector<float> m_MigrationActionsCDF;

        float m_OverrideCoitalActRate;
        const Sigmoid* m_pOverrideCondomUsage;
        float m_OverrideDurationWeibullHeterogeneity;
        float m_OverrideDurationWeibullScale;
    };
}
