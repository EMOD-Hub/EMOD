
#include "StandardEventCoordinator.h"
#include "InterpolatedValueMap.h"
#include "Types.h"
#include "IAdditionalRestrictions.h"

namespace Kernel
{
    class ReferenceTrackingEventCoordinatorTrackingConfig : public StandardInterventionDistributionEventCoordinator
                                                          , public IEventCoordinatorEventContext
    {
        DECLARE_FACTORY_REGISTERED(EventCoordinatorFactory, ReferenceTrackingEventCoordinatorTrackingConfig, IEventCoordinator)    

    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        ReferenceTrackingEventCoordinatorTrackingConfig();
        virtual ~ReferenceTrackingEventCoordinatorTrackingConfig() { } 

        virtual bool Configure(const Configuration* config) override;
        virtual void SetContextTo(ISimulationEventContext *isec) override;
        virtual void Update(float dt) override;
        virtual void CheckStartDay( float campaignStartDay ) const override;

        virtual IEventCoordinatorEventContext* GetEventContext() override { return this; }

        // IEventCoordinatorEventContext methods
        virtual const std::string& GetName() const { return m_CoordinatorName;  }
        virtual const IdmDateTime& GetTime() const { return parent->GetSimulationTime(); }
        virtual IEventCoordinator* GetEventCoordinator() { return this;  }

    protected:
        virtual void InitializeRepetitions( const Configuration* inputJson ) override;
        virtual void preDistribute() override;
        virtual void DistributeInterventionsToIndividuals( INodeEventContext* event_context ) override;

        std::string              m_CoordinatorName;
        InterpolatedValueMap     m_Year2ValueMap;
        IAdditionalRestrictions* m_pTrackingRestrictions;
        float m_EndYear;
        float m_NumQualifiedWithout;
        float m_NumQualifiedNeeding;
        std::map<INodeEventContext*,std::vector<IIndividualHumanEventContext*>> m_QualifiedPeopleWithoutMap;
    };
}
