
#include "stdafx.h"
#include "InterventionForCurrentPartners.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "IIndividualHumanSTI.h"
#include "INodeContext.h"
#include "INodeSTI.h"
#include "Common.h"
#include "RANDOM.h"

SETUP_LOGGING( "InterventionForCurrentPartners" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( InterventionForCurrentPartners )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( InterventionForCurrentPartners )

    IMPLEMENT_FACTORY_REGISTERED( InterventionForCurrentPartners )

    InterventionForCurrentPartners::InterventionForCurrentPartners()
        : BaseIntervention()
        , m_RelationshipTypes()
        , m_PrioritizePartnersBy( PartnerPrioritizationType::NO_PRIORITIZATION )
        , m_MinimumDurationYears( 0.0f )
        , m_MinimumDurationDays( 0.0f )
        , m_MaximumPartners( 1000.0f )
        , m_UseEventOrConfig( EventOrConfig::Event )
        , m_EventToBroadcast()
        , m_di( nullptr )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    InterventionForCurrentPartners::InterventionForCurrentPartners( const InterventionForCurrentPartners& rMaster )
        : BaseIntervention( rMaster )
        , m_RelationshipTypes(    rMaster.m_RelationshipTypes )
        , m_PrioritizePartnersBy( rMaster.m_PrioritizePartnersBy )
        , m_MinimumDurationYears( rMaster.m_MinimumDurationYears )
        , m_MinimumDurationDays(  rMaster.m_MinimumDurationDays )
        , m_MaximumPartners(      rMaster.m_MaximumPartners )
        , m_UseEventOrConfig(     rMaster.m_UseEventOrConfig )
        , m_EventToBroadcast(     rMaster.m_EventToBroadcast )
        , m_di( nullptr )
    {
        if( rMaster.m_di != nullptr )
        {
            m_di = rMaster.m_di->Clone();
        }
    }

    InterventionForCurrentPartners::~InterventionForCurrentPartners()
    {
        delete m_di;
    }

    bool InterventionForCurrentPartners::Configure( const Configuration * inputJson )
    {
        std::set<std::string> allowable_relationship_types = GetAllowableRelationshipTypes();
        std::vector<std::string> rel_type_strings;
        initConfigTypeMap( "Relationship_Types", &rel_type_strings, IFCP_Relationship_Types_DESC_TEXT, nullptr, allowable_relationship_types );

        initConfigTypeMap( "Minimum_Duration_Years", &m_MinimumDurationYears, IFCP_Minimum_Duration_Years_DESC_TEXT, 0.0, 200, 0.0 );
        initConfigTypeMap( "Maximum_Partners", &m_MaximumPartners, IFCP_Maximum_Partners_DESC_TEXT, 0.0, 100.0, 100.0 );

        initConfig( "Prioritize_Partners_By", m_PrioritizePartnersBy, inputJson, MetadataDescriptor::Enum( "Prioritize_Partners_By", IFCP_Prioritize_Partners_By_DESC_TEXT, MDD_ENUM_ARGS( PartnerPrioritizationType ) ) );

        initConfig( "Event_Or_Config", m_UseEventOrConfig, inputJson, MetadataDescriptor::Enum( "EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        if( m_UseEventOrConfig == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Broadcast_Event", &m_EventToBroadcast, IFCP_Broadcast_Event_DESC_TEXT );
        }

        IndividualInterventionConfig intervention_config;
        if( m_UseEventOrConfig == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Intervention_Config", &intervention_config, IFCP_Intervention_Config_DESC_TEXT );
        }

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( (m_UseEventOrConfig == EventOrConfig::Event) && m_EventToBroadcast.IsUninitialized() )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "If you set 'Event_Or_Config' = 'Event', then you must define 'Broadcast_Event'" );
            }
            if( m_UseEventOrConfig == EventOrConfig::Config )
            {
                if( intervention_config._json.Type() == ElementType::NULL_ELEMENT )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            "If you set 'Event_Or_Config' = 'Config', then you must define 'Intervention_Config'" );
                }
                else
                {
                    m_di = InterventionFactory::CreateIntervention( intervention_config._json, inputJson->GetDataLocation(), "Intervention_Config" );
                }
            }
            m_RelationshipTypes = ConvertStringsToRelationshipTypes( "Relationship_Types", rel_type_strings );
            m_MinimumDurationDays = m_MinimumDurationYears * DAYSPERYEAR;
        }
        return ret;
    }

    void InterventionForCurrentPartners::Update( float dt )
    {
        expired = true;

        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        IIndividualHumanSTI* p_human_sti = nullptr;
        if( s_OK != parent->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHumanContext" );
        }

        const std::vector<IRelationship*>& relationships = p_human_sti->GetRelationships();

        std::vector<IRelationship*> reduced_relationships = SelectRelationships( relationships );

        std::vector<IIndividualHumanEventContext*> partners = SelectPartners( p_human_sti, reduced_relationships );

        ReducePartners( partners );

        if( m_UseEventOrConfig == EventOrConfig::Event )
        {
            DistributeToPartnersEvent( partners );
        }
        else
        {
            DistributeToPartnersIntervention( partners );
        }
    }

    std::vector<IRelationship*> 
        InterventionForCurrentPartners::SelectRelationships( const std::vector<IRelationship*>& rRelationships )
    {
        // --------------------------------------------------------------------------
        // --- Only include relationships whose duration is greater than the minimum
        // --------------------------------------------------------------------------
        std::vector<IRelationship*> reduced_relationships;
        for( auto p_rel : rRelationships )
        {
            if( (p_rel->GetState() == RelationshipState::NORMAL) &&
                (m_MinimumDurationDays <= p_rel->GetDuration()) )
            {
                reduced_relationships.push_back( p_rel );
            }
        }
        return reduced_relationships;
    }

    std::vector<IIndividualHumanEventContext*> 
        InterventionForCurrentPartners::SelectPartners( IIndividualHumanSTI* pHumanStiSelf,
                                                       std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners;

        switch( m_PrioritizePartnersBy )
        {
            case PartnerPrioritizationType::NO_PRIORITIZATION:
                partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::CHOSEN_AT_RANDOM:
                partners = SelectPartnersChosenAtRandom( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::LONGER_TIME_IN_RELATIONSHIP:
                partners = SelectPartnersLongerTime( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::SHORTER_TIME_IN_RELATIONSHIP:
                partners = SelectPartnersShorterTime( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::OLDER_AGE:
                partners = SelectPartnersOlderAge( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::YOUNGER_AGE:
                partners = SelectPartnersYoungerAge( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::RELATIONSHIP_TYPE:
                partners = SelectPartnersRelationshipType( pHumanStiSelf, reducedRelationships );
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "m_PrioritizePartnersBy",
                                                         m_PrioritizePartnersBy,
                                                         PartnerPrioritizationType::pairs::lookup_key( m_PrioritizePartnersBy ) );
        }
        return partners;
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersNoPrioritization( IIndividualHumanSTI* pHumanStiSelf,
                                                                        std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners;
        for( auto p_rel : reducedRelationships )
        {
            IIndividualHumanSTI* p_human_sti_partner = p_rel->GetPartner( pHumanStiSelf );
            IIndividualHumanEventContext* p_human_event = p_human_sti_partner->GetIndividualHuman()->GetEventContext();
            partners.push_back( p_human_event );
        }
        return partners;
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersChosenAtRandom( IIndividualHumanSTI* pHumanStiSelf,
                                                                      std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );

        RANDOMBASE* p_rng = parent->GetRng();
        auto myran = [p_rng] ( int i ) { return p_rng->uniformZeroToN32( i ); };
        std::random_shuffle( partners.begin(), partners.end(), myran );

        return partners;
    }

    bool LongerTime( IRelationship* pRelA, IRelationship* pRelB )
    {
        return pRelA->GetDuration() > pRelB->GetDuration();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersLongerTime( IIndividualHumanSTI* pHumanStiSelf,
                                                                  std::vector<IRelationship*>& reducedRelationships )
    {
        std::sort( reducedRelationships.begin(), reducedRelationships.end(), LongerTime );
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        return partners;
    }

    bool ShorterTime( IRelationship* pRelA, IRelationship* pRelB )
    {
        return pRelA->GetDuration() < pRelB->GetDuration();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersShorterTime( IIndividualHumanSTI* pHumanStiSelf,
                                                                   std::vector<IRelationship*>& reducedRelationships )
    {
        std::sort( reducedRelationships.begin(), reducedRelationships.end(), ShorterTime );
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        return partners;
    }

    bool OlderAge( IIndividualHumanEventContext* pHumanA, IIndividualHumanEventContext* pHumanB )
    {
        // --------------------------------------------------------------------------------------------
        // --- We had to using SimDayBorn (i.e. birthday) because it is set when the person is created.
        // --- Age on the other hand depends on when it is updated.  This fixes a bug were the age of
        // --- some of the partners were updated and some were not.
        // --------------------------------------------------------------------------------------------

        IIndividualHumanSTI* p_human_sti_A = nullptr;
        if( s_OK != pHumanA->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti_A ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pHumanA", "IIndividualHumanSTI", "IIndividualHumanEventContext" );
        }

        IIndividualHumanSTI* p_human_sti_B = nullptr;
        if( s_OK != pHumanB->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti_B ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pHumanB", "IIndividualHumanSTI", "IIndividualHumanEventContext" );
        }

        // you are older if your birthday was longer ago
        return p_human_sti_A->GetSimDayBorn() < p_human_sti_B->GetSimDayBorn();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersOlderAge( IIndividualHumanSTI* pHumanStiSelf,
                                                                std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        std::sort( partners.begin(), partners.end(), OlderAge );
        return partners;
    }

    bool YoungerAge( IIndividualHumanEventContext* pHumanA, IIndividualHumanEventContext* pHumanB )
    {
        // SEE COMMENT IN OlderAge()

        IIndividualHumanSTI* p_human_sti_A = nullptr;
        if( s_OK != pHumanA->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti_A ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pHumanA", "IIndividualHumanSTI", "IIndividualHumanEventContext" );
        }

        IIndividualHumanSTI* p_human_sti_B = nullptr;
        if( s_OK != pHumanB->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti_B ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pHumanB", "IIndividualHumanSTI", "IIndividualHumanEventContext" );
        }

        // you are younger if your birthday was more recent
        return p_human_sti_A->GetSimDayBorn() > p_human_sti_B->GetSimDayBorn();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersYoungerAge( IIndividualHumanSTI* pHumanStiSelf,
                                                                  std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        std::sort( partners.begin(), partners.end(), YoungerAge );
        return partners;
    }

    std::vector<IIndividualHumanEventContext*> 
        InterventionForCurrentPartners::SelectPartnersRelationshipType( IIndividualHumanSTI* pHumanStiSelf,
                                                                        std::vector<IRelationship*>& reducedRelationships )
    {
        RANDOMBASE* p_rng = parent->GetRng();
        auto myran = [p_rng] ( int i ) { return p_rng->uniformZeroToN32( i ); };

        std::vector<IRelationship*> sorted_rels;
        for( auto rel_type : m_RelationshipTypes )
        {
            // ------------------------------------------
            // --- find the relationships of a given type
            // ------------------------------------------
            std::vector<IRelationship*> sorted_rels_by_type;
            for( int i = 0 ; i < reducedRelationships.size() ; )
            {
                if( rel_type == reducedRelationships[ i ]->GetType() )
                {
                    sorted_rels_by_type.push_back( reducedRelationships[ i ] );
                    reducedRelationships[ i ] = reducedRelationships.back();
                    reducedRelationships.pop_back();
                }
                else
                {
                    ++i;
                }
            }

            // ----------------------------------------------------------------------
            // --- multiple relationships of the same type will be randomly selected
            // ----------------------------------------------------------------------
            std::random_shuffle( sorted_rels_by_type.begin(), sorted_rels_by_type.end(), myran );

            // ------------------------------------------------------------------------------
            // --- Append the relationships for the current type to the end of the total list
            // ------------------------------------------------------------------------------
            sorted_rels.insert( sorted_rels.end(), sorted_rels_by_type.begin(), sorted_rels_by_type.end() );
        }

        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, sorted_rels );
        return partners;
    }

    void InterventionForCurrentPartners::ReducePartners( std::vector<IIndividualHumanEventContext*>& partners )
    {
        // -----------------------------------------------------------------------
        // --- randomly round to the nearest integer such that if m_MaximumPartners is 2.3, 
        // --- it returns two 70% of the time and three 30%.
        // -----------------------------------------------------------------------
        int max = parent->GetRng()->randomRound( m_MaximumPartners );
        while( partners.size() > max )
        {
            partners.pop_back();
        }
    }

    void InterventionForCurrentPartners::DistributeToPartnersEvent( const std::vector<IIndividualHumanEventContext*>& partners )
    {
        INodeSTI* p_node_sti = nullptr;
        if( parent->GetEventContext()->GetNodeEventContext()->GetNodeContext()->QueryInterface( GET_IID( INodeSTI ), (void**)&p_node_sti ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetEventContext()->GetNodeEventContext()",
                                           "INodeSTI",
                                           "INodeEventContext" );
        }

        // Stage the events to avoid issues where the order of the
        // individuals being updated changes the results.
        IActionStager* p_stager = p_node_sti->GetActionStager();
        for( auto p_human_event : partners )
        {
            p_stager->StageEvent( p_human_event, m_EventToBroadcast );
        }
    }

    void InterventionForCurrentPartners::DistributeToPartnersIntervention( const std::vector<IIndividualHumanEventContext*>& partners )
    {
        INodeSTI* p_node_sti = nullptr;
        if( parent->GetEventContext()->GetNodeEventContext()->GetNodeContext()->QueryInterface( GET_IID( INodeSTI ), (void**)&p_node_sti ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetEventContext()->GetNodeEventContext()",
                                           "INodeSTI",
                                           "INodeEventContext" );
        }

        // Stage the distribution of the interventions to avoid issues where
        // the order of the individuals being updated changes the results.
        IActionStager* p_stager = p_node_sti->GetActionStager();
        for( auto p_human_event : partners )
        {
            IDistributableIntervention *clone_di = m_di->Clone();
            p_stager->StageIntervention( p_human_event, clone_di );
        }
    }

    REGISTER_SERIALIZABLE( InterventionForCurrentPartners );

    void InterventionForCurrentPartners::serialize( IArchive& ar, InterventionForCurrentPartners* obj )
    {
        BaseIntervention::serialize( ar, obj );
        InterventionForCurrentPartners& ifcp = *obj;

        std::vector<uint32_t> rel_types;
        for( auto rel : ifcp.m_RelationshipTypes )
        {
            rel_types.push_back( uint32_t(rel) );
        }

        ar.labelElement( "m_RelationshipTypes"    ) & rel_types;
        ar.labelElement( "m_PrioritizePartnersBy" ) & (uint32_t&)ifcp.m_PrioritizePartnersBy;
        ar.labelElement( "m_MinimumDurationYears" ) & ifcp.m_MinimumDurationYears;
        ar.labelElement( "m_MinimumDurationDays"  ) & ifcp.m_MinimumDurationDays;
        ar.labelElement( "m_MaximumPartners"      ) & ifcp.m_MaximumPartners;
        ar.labelElement( "m_UseEventOrConfig"     ) & (uint32_t&)ifcp.m_UseEventOrConfig;
        ar.labelElement( "m_EventToBroadcast"     ) & ifcp.m_EventToBroadcast;
        ar.labelElement( "m_di"                   ) & ifcp.m_di;

        ifcp.m_RelationshipTypes.clear();
        for( auto irel : rel_types )
        {
            ifcp.m_RelationshipTypes.push_back( RelationshipType::Enum(irel) );
        }
    }
}
