
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IRelationship.h"
#include "EventTrigger.h"
#include "Sigmoid.h"
#include "ISTIInterventionsContainer.h"

namespace Kernel
{
    ENUM_DEFINE( CondomUsageParametersType,
                 ENUM_VALUE_SPEC( USE_DEFAULT,   0 )
                 ENUM_VALUE_SPEC( SPECIFY_USAGE, 1 ) )

    class StartNewRelationship : public BaseIntervention
                               , public INonPfaRelationshipStarter
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, StartNewRelationship, IDistributableIntervention)
    public:
        StartNewRelationship();
        StartNewRelationship( const StartNewRelationship& rMaster );
        virtual ~StartNewRelationship();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update(float dt) override;

        // IStartNewRelationship
        virtual void StartNonPfaRelationship() override;

    protected:
        RelationshipType::Enum          m_RelationshipType;
        IPKeyValue                      m_PartnerHasIP;
        EventTrigger                    m_RelationshipCreatedEvent;
        CondomUsageParametersType::Enum m_CondomUsageParametersType;
        Sigmoid                         m_CondumUsageSigmoid;

        DECLARE_SERIALIZABLE(StartNewRelationship);
    };
}
