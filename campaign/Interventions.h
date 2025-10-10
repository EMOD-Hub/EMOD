
#pragma once

#include <string>
#include <list>

#include "ISupports.h"
#include "Configure.h"                 // for JsonConfigurable
#include "InterventionEnums.h"
#include "Properties.h"
#include "NodeProperties.h"
#include "BroadcasterObserver.h"
#include "InterventionName.h"

#include "ISerializable.h"
#include "IArchive.h"
#include "IReportInterventionDataAccess.h"

namespace Kernel
{
    struct IIndividualHumanContext;
    struct IIndividualHumanEventContext;
    class EventTrigger;

    struct ICampaignCostObserver : ISupports
    {
        virtual void notifyCampaignExpenseIncurred( float expenseIncurred, const IIndividualHumanEventContext * pIndiv ) = 0;
        virtual void notifyCampaignEventOccurred( /*const*/ ISupports * pDistributedIntervention, /*const*/ ISupports * pDistributor, /*const*/ IIndividualHumanContext * pDistributeeIndividual ) = 0;
        virtual ~ICampaignCostObserver() {}
    };

    struct IIndividualHumanInterventionsContext;

    struct IDistributableIntervention : ISerializable
    {
        // Distribute transfers ownership of this object to the context if it succeeds, the context becomes responsible for freeing it
        // returns false if cannot distribute to the individual represented by this context, for whatever reason
        virtual const InterventionName& GetName() const = 0;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) = 0;
        virtual void SetContextTo(IIndividualHumanContext *context) = 0;
        virtual void Update(float dt) = 0;
        virtual bool Expired() = 0;
        virtual void SetExpired( bool isExpired ) = 0;
        virtual IDistributableIntervention * Clone()  = 0;
        virtual bool NeedsInfectiousLoopUpdate() const = 0;

        virtual ~IDistributableIntervention() { }
    };

    struct IIndividualHumanInterventionsContext : ISerializable // ISupports
    {
        // specific helpers for interoperating with the interventions container might go here
        virtual void SetContextTo(IIndividualHumanContext *context) = 0;
        virtual IIndividualHumanContext* GetParent() = 0;
        virtual std::list<void*>                       GetInterventionsByInterface( iid_t iid ) = 0;
        virtual void PurgeExistingByType( const std::string &type_name ) = 0;
        virtual void PurgeExistingByName( const std::string& type_name, const InterventionName& name ) = 0;
        virtual bool ContainsExistingByName( const InterventionName& name ) = 0;
        virtual void ChangeProperty( const char *property, const char* new_value ) = 0;
        virtual const std::vector<IDistributableIntervention*>& GetInterventions() const = 0;
        virtual uint32_t GetNumInterventions() const = 0;
        virtual uint32_t GetNumInterventionsAdded() = 0;
        virtual const IPKeyValue& GetLastIPChange() const = 0;

        virtual ~IIndividualHumanInterventionsContext() {}
    };

    struct INodeEventContext;
    struct IEventCoordinator2;

    struct INodeDistributableIntervention : ISupports
    {
        // Distribute transfers ownership of this object to the context if it succeeds, the context becomes responsible for freeing it
        // returns false if cannot distribute to the individual represented by this context, for whatever reason
        virtual const InterventionName& GetName() const = 0;
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = nullptr ) = 0;
        virtual void SetContextTo(INodeEventContext *context) = 0;
        virtual void Update(float dt) = 0;
        virtual bool Expired() = 0;
        virtual void SetExpired( bool isExpired ) = 0;
        virtual INodeDistributableIntervention * Clone()  = 0;

        virtual ~INodeDistributableIntervention() { }
    };

    struct IInterventionConsumer : ISupports
    {
        virtual bool GiveIntervention( IDistributableIntervention * pIV ) = 0;
    };

    struct INodeInterventionConsumer : ISupports
    {
        virtual bool GiveIntervention( INodeDistributableIntervention * pIV ) = 0;
    };

    struct IBaseIntervention : ISupports
    {
        virtual float GetCostPerUnit() const = 0;
    };

    struct IIndividualHumanEventContext;

    // TODO - BaseInterventions looks concrete, but can't be instantiated. :(
    struct BaseIntervention : IDistributableIntervention, IBaseIntervention, IReportInterventionDataAccess, JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        virtual const InterventionName& GetName() const override { return name; };
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual float GetCostPerUnit() const override { return cost_per_unit; }
        virtual bool Expired() override ;
        virtual void SetExpired( bool isExpired ) override;
        virtual bool NeedsInfectiousLoopUpdate() const { return false; }

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        BaseIntervention();
        BaseIntervention( const BaseIntervention& );
        virtual ~BaseIntervention();
        virtual bool Configure(const Configuration* inputJson) override;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        virtual bool AbortDueToDisqualifyingInterventionStatus( IIndividualHumanContext* pHuman );
        virtual bool UpdateIndividualsInterventionStatus( bool checkDisqualifyingProperties=true );

        static void serialize( IArchive& ar, BaseIntervention* obj );

        IIndividualHumanContext *parent;
        InterventionName name;
        float cost_per_unit;
        bool expired;
        bool dont_allow_duplicates;
        bool first_time;
        IPKeyValueContainer disqualifying_properties;
        IPKeyValue status_property;
    };

    struct BaseNodeIntervention : INodeDistributableIntervention, IBaseIntervention, IReportInterventionDataAccess, JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual const InterventionName& GetName() const override { return name; };
        virtual float GetCostPerUnit() const override { return cost_per_unit; }
        virtual bool Expired() override;
        virtual void SetExpired( bool isExpired ) override;
        virtual void SetContextTo( INodeEventContext *context ) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        BaseNodeIntervention();
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = nullptr ) override;

        virtual bool AbortDueToDisqualifyingInterventionStatus( INodeEventContext* context );
        virtual bool UpdateNodesInterventionStatus();

        INodeEventContext *parent;
        InterventionName name;
        float cost_per_unit;
        bool expired;
        bool dont_allow_duplicates;
        bool first_time;
        NPKeyValueContainer disqualifying_properties;
        NPKeyValue status_property;
    };
}
