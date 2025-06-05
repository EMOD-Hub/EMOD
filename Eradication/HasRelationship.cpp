
#include "stdafx.h"

#include "HasRelationship.h"
#include "AdditionalRestrictionsFactory.h"
#include "IRelationship.h"
#include "IIndividualHumanSTI.h"
#include "SimulationConfig.h"
#include "Common.h"
#include "IndividualEventContext.h"
#include "IIndividualHuman.h"
#include "NodeEventContext.h"
#include "IdmDateTime.h"

static constexpr float DAYSPERMONTH = (DAYSPERYEAR / MONTHSPERYEAR);

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HasRelationship)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasRelationship)

    IMPLEMENT_FACTORY_REGISTERED(HasRelationship)
    REGISTER_SERIALIZABLE(HasRelationship)

    HasRelationship::HasRelationship()
        : AdditionalRestrictionsSTIAbstract()
        , m_OfRelationshipType( TargetRelationshipType::NA )
        , m_ThatRecently( RecentlyType::NA )
        , m_ThatRecentlyEndedDueTo( RelationshipTerminationReason::NA )
        , m_pWithPartnerWho( nullptr )
        , m_TimeStepDuration(0.0f)
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    HasRelationship::~HasRelationship()
    {
        delete m_pWithPartnerWho;
    }

#define OF_RELATIONSHIP_TYPE "Of_Relationship_Type"
#define THAT_RECENTLY "That_Recently"
#define THAT_RECENTLY_ENDED_DUE_TO "That_Recently_Ended_Due_To"
#define WITH_PARTNER_WHO "With_Partner_Who"

    bool HasRelationship::Configure(const Configuration* config)
    {
        initConfig( OF_RELATIONSHIP_TYPE, m_OfRelationshipType, config,
                    MetadataDescriptor::Enum(OF_RELATIONSHIP_TYPE,
                                              AR_Of_Relationship_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(TargetRelationshipType)) );

        initConfig( THAT_RECENTLY, m_ThatRecently, config,
                    MetadataDescriptor::Enum(THAT_RECENTLY,
                                              AR_That_Recently_DESC_TEXT,
                                              MDD_ENUM_ARGS(RecentlyType)) );

        initConfig( THAT_RECENTLY_ENDED_DUE_TO, m_ThatRecentlyEndedDueTo, config,
                    MetadataDescriptor::Enum(THAT_RECENTLY_ENDED_DUE_TO,
                                              AR_That_Recently_Ended_Due_To_DESC_TEXT,
                                              MDD_ENUM_ARGS(RelationshipTerminationReason)),
                    THAT_RECENTLY, "ENDED");

        AdditionalTargetingConfig partner_config;
        initConfigComplexType( WITH_PARTNER_WHO, &partner_config, AR_With_Partner_Who_DESC_TEXT );

        bool configured = AdditionalRestrictionsSTIAbstract::Configure( config );
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( (partner_config._json.Type() == json::OBJECT_ELEMENT) &&
                (json::QuickInterpreter(partner_config._json).As<json::Object>().Size() > 0) )
            {
                m_pWithPartnerWho = AdditionalRestrictionsFactory::getInstance()->CreateInstance( partner_config._json,
                                                                                                  config->GetDataLocation(),
                                                                                                  WITH_PARTNER_WHO );
            }

            if( (m_OfRelationshipType == TargetRelationshipType::NA) &&
                (m_ThatRecently == RecentlyType::NA) &&
                (m_pWithPartnerWho == nullptr) )
            {
                std::stringstream ss;
                ss << "No parameters were configured so no relationships would be targeted.\n";
                ss << "'HasRelationship' requires that you define at least one parameter.\n";
                ss << "The parameters are:\n";
                ss << "'" << OF_RELATIONSHIP_TYPE << "'\n";
                ss << "'" << THAT_RECENTLY        << "'\n";
                ss << "'" << WITH_PARTNER_WHO     << "'\n";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            m_TimeStepDuration = GET_CONFIGURABLE( SimulationConfig )->Sim_Tstep;
        }
        return configured;
    }

    bool HasRelationship::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        IIndividualHumanSTI* p_human_sti = GetSTIIndividual( pContext );

        bool result = false;
        if( m_ThatRecently == RecentlyType::ENDED )
        {
            IRelationship* p_rel = p_human_sti->GetExitingRelationship();
            if( p_rel == nullptr )
            {
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // !!! This is really lame but I don't know what else to do right now.
                // !!! We should throw an exception earlier but there is no way to know
                // !!! if the user used this with NLHTIV listening for the ExitedRelationship event.
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__,
                                                 "You can only use 'That_Recently' = 'ENDED' with 'ExitedRelationship'." );
            }
            result = true;
            if( m_ThatRecentlyEndedDueTo != RelationshipTerminationReason::NA )
            {
                result = (p_rel->GetTerminationReason() == m_ThatRecentlyEndedDueTo);
            }
            if( result )
            {
                result = IsRelationshipQualified( p_human_sti, p_rel );
            }
        }
        else
        {
            const std::vector<IRelationship*>& r_relationships = p_human_sti->GetRelationships();
            for( auto p_rel : r_relationships )
            {
                if( IsRelationshipQualified( p_human_sti, p_rel ) )
                {
                    result = true;
                    break;
                }
            }
        }
        return (result == m_CompareTo);
    }

    bool HasRelationship::IsRelationshipQualified( IIndividualHumanSTI* pHumanSti, 
                                               IRelationship* pRel ) const
    {
        bool valid_rel = true;
        if( m_OfRelationshipType != TargetRelationshipType::NA )
        {
            valid_rel &= (pRel->GetType() == m_OfRelationshipType);
        }

        if( m_ThatRecently == RecentlyType::STARTED )
        {
            IIndividualHumanEventContext * p_human = pHumanSti->GetIndividualHuman()->GetEventContext();

            // ------------------------------------------------------------------------------------
            // --- We use the start minus current here instead of just relationship.duration
            // --- because relationship.duration may not have been udpated yet.  If this is called
            // --- from Standard Event Coordinator (SEC), then the relationships have not been udpated
            // --- and neither has the relationship.duration.  If this is called from NLHTIV,
            // --- then the relationships have been updated.  This tries to get the relationships
            // --- that started during the previous timestep as well as this one.  However,
            // --- you will only see the relationships that started this timestep if you are using
            // --- NLHTIV because of when the relationships are updated.  That is, SEC gets only
            // --- previous timestep while NLHTIV will see both previous and current timestep.
            // ------------------------------------------------------------------------------------
            float start = pRel->GetStartTime();
            float current = p_human->GetNodeEventContext()->GetTime().time;
            float duration = current - start;
            valid_rel &= (duration <= m_TimeStepDuration);
        }

        if( m_pWithPartnerWho != nullptr )
        {
            IIndividualHumanSTI* p_partner_sti = pRel->GetPartner( pHumanSti );

            IIndividualHumanEventContext * p_partner = p_partner_sti->GetIndividualHuman()->GetEventContext();

            valid_rel &= m_pWithPartnerWho->IsQualified( p_partner );
        }
        return valid_rel;
    }

    void HasRelationship::serialize(IArchive& ar, HasRelationship* obj)
    {
        // TODO: implement me
    }
}