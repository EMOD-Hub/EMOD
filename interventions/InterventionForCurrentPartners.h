
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IRelationship.h"
#include "EventTrigger.h"

namespace Kernel
{
    ENUM_DEFINE( PartnerPrioritizationType,
                 ENUM_VALUE_SPEC( NO_PRIORITIZATION,            0 )   // All partners are contacted
                 ENUM_VALUE_SPEC( CHOSEN_AT_RANDOM,             1 )   // Partners are randomly selected until Maximum_Partners have received the intervention
                 ENUM_VALUE_SPEC( LONGER_TIME_IN_RELATIONSHIP,  2 )   // Partners are sorted in descending order of the duration of the relationship. 
                                                                      // Partners are contacted from the beginning of this list until Maximum_Partners have received the intervention.
                 ENUM_VALUE_SPEC( SHORTER_TIME_IN_RELATIONSHIP, 3 )   // Same thing but ascending order.
                 ENUM_VALUE_SPEC( OLDER_AGE,                    4 )   // Same thing but descending order based on age.
                 ENUM_VALUE_SPEC( YOUNGER_AGE,                  5 )   // Same thing but ascending order based on age.
                 ENUM_VALUE_SPEC( RELATIONSHIP_TYPE,            6 ) ) // In this case, the partners are sorted based on the order of the relationship types defined in the Relationship_Types parameter. 
                                                                      // For example, "Relationship_Types":["MARITAL", "INFORMAL","TRANSITORY","COMMERCIAL"], will prioritize marital > informal > transitory > commercial, selecting at random between multiple partners of the same type.

    class InterventionForCurrentPartners : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, InterventionForCurrentPartners, IDistributableIntervention )

    public:
        InterventionForCurrentPartners();
        InterventionForCurrentPartners( const InterventionForCurrentPartners& rMaster );
        virtual ~InterventionForCurrentPartners();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void Update( float dt ) override;

    protected:
        std::vector<IRelationship*> SelectRelationships( const std::vector<IRelationship*>& rRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartners( IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        void ReducePartners( std::vector<IIndividualHumanEventContext*>& partners );
        void DistributeToPartnersEvent( const std::vector<IIndividualHumanEventContext*>& partners );
        void DistributeToPartnersIntervention( const std::vector<IIndividualHumanEventContext*>& partners );

        std::vector<IIndividualHumanEventContext*> SelectPartnersNoPrioritization( IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartnersChosenAtRandom(   IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartnersLongerTime(       IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartnersShorterTime(      IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartnersOlderAge(         IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartnersYoungerAge(       IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );
        std::vector<IIndividualHumanEventContext*> SelectPartnersRelationshipType( IIndividualHumanSTI* pHumanStiSelf, std::vector<IRelationship*>& reducedRelationships );

        std::vector<RelationshipType::Enum> m_RelationshipTypes;
        PartnerPrioritizationType::Enum m_PrioritizePartnersBy;
        float m_MinimumDurationYears;
        float m_MinimumDurationDays;
        float m_MaximumPartners;
        EventOrConfig::Enum m_UseEventOrConfig;
        EventTrigger m_EventToBroadcast;
        IDistributableIntervention* m_di;

        DECLARE_SERIALIZABLE( InterventionForCurrentPartners );
    };
}
