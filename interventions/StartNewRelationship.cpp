
#include "stdafx.h"
#include "StartNewRelationship.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "INodeContext.h"
#include "INodeSTI.h"
#include "IPairFormationAgent.h"

SETUP_LOGGING( "StartNewRelationship" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(StartNewRelationship)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(INonPfaRelationshipStarter)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(StartNewRelationship)

    IMPLEMENT_FACTORY_REGISTERED(StartNewRelationship)
    
    StartNewRelationship::StartNewRelationship()
        : BaseIntervention()
        , m_RelationshipType( RelationshipType::TRANSITORY )
        , m_PartnerHasIP()
        , m_RelationshipCreatedEvent()
        , m_CondomUsageParametersType( CondomUsageParametersType::USE_DEFAULT )
        , m_CondumUsageSigmoid( 1.0, 1.0, 2000.0, 1.0 )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    StartNewRelationship::StartNewRelationship( const StartNewRelationship& rMaster )
        : BaseIntervention( rMaster )
        , m_RelationshipType( rMaster.m_RelationshipType )
        , m_PartnerHasIP( rMaster.m_PartnerHasIP )
        , m_RelationshipCreatedEvent( rMaster.m_RelationshipCreatedEvent )
        , m_CondomUsageParametersType( rMaster.m_CondomUsageParametersType )
        , m_CondumUsageSigmoid( rMaster.m_CondumUsageSigmoid )
    {
    }

    StartNewRelationship::~StartNewRelationship()
    {
    }

    bool StartNewRelationship::Configure( const Configuration* inputJson )
    {
        IPKeyValueParameter has_ip_parameter;

        initConfig( "Relationship_Type",
                    m_RelationshipType,
                    inputJson,
                    MetadataDescriptor::Enum( "Relationship_Type", 
                                              SNR_Relationship_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(RelationshipType) ) );

        initConfigTypeMap( "Partner_Has_IP", &has_ip_parameter, SNR_Partner_Has_IP_DESC_TEXT );
        initConfigTypeMap( "Relationship_Created_Event", &m_RelationshipCreatedEvent, SNR_Relationship_Created_Event_DESC_TEXT );

        initConfig( "Condom_Usage_Parameters_Type",
                    m_CondomUsageParametersType,
                    inputJson,
                    MetadataDescriptor::Enum( "CondomUsageParametersType",
                                               SNR_Condom_Usage_Parameters_Type_DESC_TEXT,
                                               MDD_ENUM_ARGS(CondomUsageParametersType) ) );

        initConfigTypeMap( "Condom_Usage_Sigmoid", &m_CondumUsageSigmoid, SNR_Condom_Usage_Sigmoid_DESC_TEXT, "Condom_Usage_Parameters_Type", "SPECIFY_USAGE");

        bool is_configured = BaseIntervention::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            m_PartnerHasIP = has_ip_parameter;
        }
        return is_configured;
    }

    bool StartNewRelationship::Distribute( IIndividualHumanInterventionsContext *context,
                                           ICampaignCostObserver * pCCO )
    {
        bool distributed =  BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            ISTIInterventionsContainer* p_sti_container = nullptr;
            if (s_OK != context->QueryInterface(GET_IID(ISTIInterventionsContainer), (void**)&p_sti_container) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "context", "ISTIInterventionsContainer", "IIndividualHumanInterventionsContext" );
            }

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! The intervention is added normally but this method is for performance.
            // !!! By having our own list of these interventions, we don't have to search
            // !!! the interventions list for every person every timestep.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            p_sti_container->AddNonPfaRelationshipStarter( this );
        }
        return distributed;
    }

    void StartNewRelationship::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;
    }

    void StartNewRelationship::StartNonPfaRelationship()
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We want this method called at approximately the same place in the code
        // !!! as when the normal PFA relationships are created.  This allows these
        // !!! relationships to have similar timing of when their methods are called
        // !!! and events are broadcasted at similar locations.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        // -----------------------------------------
        // --- Get interfaces to appropriate objects
        // -----------------------------------------
        IIndividualHumanSTI* p_individual_sti = nullptr;
        if( parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_individual_sti) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent", "IIndividualHumanSTI", "IIndividualHumanContext" );
        }

        INodeContext* p_node_context = parent->GetEventContext()->GetNodeEventContext()->GetNodeContext();

        INodeSTI* p_node_sti = nullptr;
        if( p_node_context->QueryInterface(GET_IID(INodeSTI), (void**)&p_node_sti) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "p_node_context", "INodeSTI", "INodeContext" );
        }
        ISociety* p_society = p_node_sti->GetSociety();
        IPairFormationAgent* p_pfa = p_society->GetPFA( m_RelationshipType );

        // --------------------------------------------
        // --- Put this person into a new relationship
        // --------------------------------------------
        Sigmoid *p_condom_usage = nullptr;
        if( m_CondomUsageParametersType == CondomUsageParametersType::SPECIFY_USAGE )
        {
            p_condom_usage = &m_CondumUsageSigmoid;
        }

        bool was_relationship_created = p_pfa->StartNonPfaRelationship( p_individual_sti, m_PartnerHasIP, p_condom_usage );
        if( was_relationship_created )
        {
            // --------------------------------------------------------------------------------
            // --- Broadcast an event based on whether or not the new relationship was created
            // --------------------------------------------------------------------------------
            IIndividualEventBroadcaster* p_broadcaster = p_node_context->GetEventContext()->GetIndividualEventBroadcaster();
            p_broadcaster->TriggerObservers( parent->GetEventContext(), m_RelationshipCreatedEvent );
        }

        expired = true;
    }

    REGISTER_SERIALIZABLE(StartNewRelationship);

    void StartNewRelationship::serialize(IArchive& ar, StartNewRelationship* obj)
    {
        BaseIntervention::serialize( ar, obj );
        StartNewRelationship& snr = *obj;
        ar.labelElement("m_RelationshipType"          ) & (uint32_t&)snr.m_RelationshipType;
        ar.labelElement("m_PartnerHasIP"              ) & snr.m_PartnerHasIP;
        ar.labelElement("m_RelationshipCreatedEvent"  ) & snr.m_RelationshipCreatedEvent;
        ar.labelElement("m_CondomUsageParametersType" ) & (uint32_t&)snr.m_CondomUsageParametersType;
        ar.labelElement("m_CondumUsageSigmoid"        ) & snr.m_CondumUsageSigmoid;
    }
}
