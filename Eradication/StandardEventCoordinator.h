
#pragma once

#include <string>
#include <list>
#include <vector>

#include "EventCoordinator.h"
#include "Configure.h"
#include "NodeEventContext.h"
#include "DemographicRestrictions.h"

namespace Kernel
{
    ENUM_DEFINE( IndividualSelectionType,
                 ENUM_VALUE_SPEC( DEMOGRAPHIC_COVERAGE, 0 )
                 ENUM_VALUE_SPEC( TARGET_NUM_INDIVIDUALS, 1 ))

    struct ICampaignCostObserver; // TODO - Nasty fwd declaration because I'm scared to include NodeEventContext.h! :)

    // Standard distribution ec that just gives out the intervention once to the fraction of people specified by the coverage parameter
    class StandardInterventionDistributionEventCoordinator : public IEventCoordinator, public ITravelLinkedDistributionSource, public IVisitIndividual, public IEventCoordinator2, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, StandardInterventionDistributionEventCoordinator, IEventCoordinator)

    public:
        DECLARE_CONFIGURED(StandardInterventionDistributionEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        StandardInterventionDistributionEventCoordinator( bool useDemographicCoverage = true );
        virtual ~StandardInterventionDistributionEventCoordinator();

        virtual IConfigurable* GetConfigurable() override { return JsonConfigurable::GetConfigurable(); }

        // IEventCoordinator
        virtual void SetContextTo(ISimulationEventContext *isec);
        virtual void CheckStartDay( float campaignStartDay ) const override {};

        virtual void AddNode( const suids::suid& node_suid);

        virtual void Update(float dt);
        virtual void UpdateNodes(float dt);
        virtual bool visitIndividualCallback(IIndividualHumanEventContext *ihec, float &incrementalCostOut, ICampaignCostObserver * pICCO );

        virtual bool IsFinished(); // returns true when the EC requires no further updates and can be disposed of

        virtual IEventCoordinatorEventContext* GetEventContext() override { return nullptr; }

        // ITravelLinkedDistributionSource
        virtual void ProcessDeparting(IIndividualHumanEventContext *dc);
        virtual void ProcessArriving(IIndividualHumanEventContext *dc);

        virtual float GetDemographicCoverage() const;
        virtual TargetDemographicType::Enum GetTargetDemographic() const;
        virtual float GetMinimumAge() const;
        virtual float GetMaximumAge() const;

        virtual bool qualifiesDemographically( const IIndividualHumanEventContext * pIndividual );

    protected:
        virtual float getDemographicCoverageForIndividual( const IIndividualHumanEventContext *pInd ) const;
        virtual void preDistribute(); 
        virtual void InitializeRepetitions( const Configuration* inputJson );
        virtual void InitializeIndividualSelectionType( const Configuration* inputJson );
        virtual void CheckRepetitionConfiguration();
        virtual void UpdateRepetitions();
        virtual bool IsTimeToUpdate( float dt );
        virtual void DistributeInterventionsToNodes( INodeEventContext* event_context );
        virtual void DistributeInterventionsToIndividuals( INodeEventContext* event_context );
        virtual bool DistributeInterventionsToIndividual( IIndividualHumanEventContext *ihec,
                                                          float & incrementalCostOut,
                                                          ICampaignCostObserver * pICCO );
        void LogNumInterventionsDistributed( int totalIndivGivenIntervention, INodeEventContext* event_context );
        void FindQualifyingIndividuals( INodeEventContext* pNEC,
                                        std::vector<IIndividualHumanEventContext*>& r_qualified_individuals );
        std::vector<IIndividualHumanEventContext*> SelectIndividuals( const std::vector<IIndividualHumanEventContext*>& r_qualified_individuals );
        // helpers
        void regenerateCachedNodeContextPointers();
        virtual bool TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec);

        ISimulationEventContext  *parent;
        bool distribution_complete;
        int num_repetitions;
        int tsteps_between_reps;
        int tsteps_since_last;
        //bool include_emigrants;
        //bool include_immigrants;
        int intervention_activated;
        std::vector<INodeEventContext*> cached_nodes;
        std::vector<suids::suid> node_suids; // to help with serialization
        DemographicRestrictions demographic_restrictions;
        float demographic_coverage;
        PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer> node_property_restrictions;
        bool use_demographic_coverage;
        IndividualSelectionType::Enum individual_selection_type;
        int target_num_individuals;
        std::string log_intervention_name;
        bool avoid_duplicates;
        IDistributableIntervention* m_pInterventionIndividual;
        INodeDistributableIntervention* m_pInterventionNode;
    };
}
