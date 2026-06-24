
#include "stdafx.h"

#include "NodeSTI.h"
#include "Debug.h"

#include "IndividualSTI.h"
#include "RelationshipManagerFactory.h"
#include "RelationshipGroups.h"
#include "SimulationConfig.h"

#include "SocietyFactory.h"
#include "IIdGeneratorSTI.h"
#include "NodeEventContextHost.h"
#include "ISTISimulationContext.h"
#include "ISimulationContext.h"
#include "EventTrigger.h"
#include "NodeSTIEventContext.h"

SETUP_LOGGING( "NodeSTI" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeSTI,NodeSTI)

    BEGIN_QUERY_INTERFACE_DERIVED(NodeSTI, Node)
        HANDLE_INTERFACE(INodeSTI)
    END_QUERY_INTERFACE_DERIVED(NodeSTI, Node)

    bool NodeSTI::Configure( const Configuration* config )
    {
        // A PFA burnin of at least 1 day is required to ensure transmission
        initConfigTypeMap( "PFA_Burnin_Duration_In_Days", &pfa_burnin_duration, PFA_Burnin_Duration_In_Days_DESC_TEXT, 1, FLT_MAX, 1000 * DAYSPERYEAR );

        bool ret = society->Configure( config );
        if( ret )
        {
            ret = Node::Configure( config );
        }

        if( ret &&
            !JsonConfigurable::_dryrun &&
            (ind_sampling_type != IndSamplingType::TRACK_ALL     ) &&
            (ind_sampling_type != IndSamplingType::FIXED_SAMPLING) )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "Individual_Sampling_Type", IndSamplingType::pairs::lookup_key(ind_sampling_type),
                                                    "Simulation_Type", SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type ),
                                                    "Relationship-based transmission network only works with 100% sampling."
                                                    );
        }

        return ret ;
    }

    NodeSTI::NodeSTI(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
        : Node(_parent_sim, externalNodeId, node_suid)
        , relMan(nullptr)
        , society(nullptr)
        , pRelationshipGroups( nullptr )
        , pfa_burnin_duration( 15 * DAYSPERYEAR )
        , staged_interventions()
        , staged_events()
    {
        relMan = RelationshipManagerFactory::CreateManager( this );
        society = SocietyFactory::CreateSociety( relMan );
    }

    NodeSTI::NodeSTI()
        : Node()
        , relMan(nullptr)
        , society(nullptr)
        , pRelationshipGroups( nullptr )
        , pfa_burnin_duration( 15 * DAYSPERYEAR )
        , staged_interventions()
        , staged_events()
    {
        relMan = RelationshipManagerFactory::CreateManager( this );
        society = SocietyFactory::CreateSociety( relMan );
    }

    NodeSTI::~NodeSTI(void)
    {
        delete society ;
        delete relMan ;
        // pRelationshipGroups - don't delete because it is beign deleted via transmissionGroups
    }

    NodeSTI *NodeSTI::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        NodeSTI *newnode = _new_ NodeSTI(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();

        return newnode;
    }

    void NodeSTI::Initialize()
    {
        Node::Initialize();
    }

    void NodeSTI::SetupEventContextHost()
    {
        event_context_host = _new_ NodeSTIEventContextHost(this);
    }

    void NodeSTI::SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory )
    {
        Node::SetParameters( demographics_factory, climate_factory );

        const std::string SOCIETY_KEY( "Society" );
        if( !demographics.Contains( SOCIETY_KEY ) )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Could not find the 'Society' element in the demographics data." );
        }
        std::istringstream iss( demographics[SOCIETY_KEY].ToString() );
        Configuration* p_config = Configuration::Load( iss, "demographics" );
        society->SetParameters( GetRng(), dynamic_cast<IIdGeneratorSTI*>(parent), p_config );
        delete p_config ;
    }

    IIndividualHuman* NodeSTI::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanSTI::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender);
    }

    void NodeSTI::SetupIntranodeTransmission()
    {
        //RelationshipGroups * relNodePools = dynamic_cast<RelationshipGroups*>(TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::RelationshipGroups));
        pRelationshipGroups = _new_ RelationshipGroups();
        pRelationshipGroups->SetParent( this );
        transmissionGroups = pRelationshipGroups;
        routes.push_back(string("contact"));
        transmissionGroups->Build(1.0f, 1, 1);
    }

    void NodeSTI::DepositFromIndividual( const IStrainIdentity& rStrain, const CoitalAct& rCoitalAct )
    {
        pRelationshipGroups->DepositContagion( rStrain, rCoitalAct );
    }

    RANDOMBASE* NodeSTI::GetRng()
    {
        return Node::GetRng();
    }

    void NodeSTI::Update( float dt )
    {
        // Update relationships (dissolution only, at this point)
        relMan->Update( dt );

        society->BeginUpdate();

        for (auto& person : individualHumans)
        {
            // switched from QI to static_cast for performance
            IndividualHumanSTI* sti_person = static_cast<IndividualHumanSTI*>(person);
            sti_person->UpdateEligibility();        // DJK: Could be slow to do this on every update.  Could check for relationship status changes. <ERAD-1869>
            sti_person->UpdateHistory( GetTime(), dt );
        }

        if (pfa_burnin_duration > 0)
        {
            pfa_burnin_duration -= dt;
            society->UpdatePairFormationRates( GetTime(), dt );
        }

        for (auto& person : individualHumans)
        {
            // switched from QI to static_cast for performance
            IndividualHumanSTI* sti_person = static_cast<IndividualHumanSTI*>(person);
            sti_person->ConsiderRelationships(dt);

            sti_person->StartNonPfaRelationships();
        }

        society->UpdatePairFormationAgents( GetTime(), dt );

        transmissionGroups->Build( 1.0f, 1, 1 );
        
        Node::Update( dt );

        for( auto& person : individualHumans )
        {
            // switched from QI to static_cast for performance
            IndividualHumanSTI* sti_person = static_cast<IndividualHumanSTI*>(person);
            sti_person->UpdatePausedRelationships( GetTime(), dt );
        }

        NodeSTIEventContextHost* p_NSECH = static_cast<NodeSTIEventContextHost*>(event_context_host);
        p_NSECH->ApplyStagedInterventions();
    }

    void NodeSTI::PostUpdate()
    {
        // Broadcast the events that were staged until all of the individuals have been updated.
        IIndividualEventBroadcaster* broadcaster = GetEventContext()->GetIndividualEventBroadcaster();
        for( auto event_pair : staged_events )
        {
            broadcaster->TriggerObservers( event_pair.first, event_pair.second );
        }
        staged_events.clear();

        // Distribute the interventions that were staged until all of the individuals have been updated.
        ICampaignCostObserver* pICCO;
        if( GetEventContext()->QueryInterface( GET_IID( ICampaignCostObserver ), (void**)&pICCO ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "GetEventContext()",
                                           "ICampaignCostObserver",
                                           "INodeEventContext" );
        }
        for( auto intervention_pair : staged_interventions )
        {
            IIndividualHumanEventContext* p_human = intervention_pair.first;
            IDistributableIntervention* p_intervention = intervention_pair.second;

            p_intervention->AddRef();
            p_intervention->Distribute( p_human->GetInterventionsContext(), pICCO );
            p_intervention->Release();
        }
        staged_interventions.clear();
    }

    /*const?*/ IRelationshipManager*
    NodeSTI::GetRelationshipManager() /*const?*/
    {
        return relMan;
    }

    ISociety*
    NodeSTI::GetSociety()
    {
        return society;
    }

    IActionStager* NodeSTI::GetActionStager()
    {
        return this;
    }

    void NodeSTI::StageIntervention( IIndividualHumanEventContext* pHuman, IDistributableIntervention* pIntervention )
    {
        staged_interventions.push_back( std::make_pair( pHuman, pIntervention ) );
    }

    void NodeSTI::StageEvent( IIndividualHumanEventContext* pHuman, const EventTrigger& rTrigger )
    {
        staged_events.push_back( std::make_pair( pHuman, rTrigger ) );
    }

    void
    NodeSTI::processEmigratingIndividual(
        IIndividualHuman* individual
    )
    {
        IIndividualHumanSTI* sti_individual=nullptr;
        if (individual->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualSTI", "IndividualHuman" );
        }

        sti_individual->onEmigrating();

        Node::processEmigratingIndividual( individual );
    }

    IIndividualHuman*
    NodeSTI::processImmigratingIndividual(
        IIndividualHuman* movedind
    )
    {
        // -------------------------------------------------------------------------------
        // --- SetContextTo() is called in Node::processImmigratingIndividual() but
        // --- we need need to set context before onImmigrating().  onImmigrating() needs
        // --- the RelationshipManager which is part of the node.
        // -------------------------------------------------------------------------------
        movedind->SetContextTo(getContextPointer());

        IIndividualHumanSTI* sti_individual = nullptr;
        if (movedind->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "retVal", "IIndividualSTI", "IndividualHuman" );
        }
        sti_individual->onImmigrating();

        auto retVal = Node::processImmigratingIndividual( movedind );

        event_context_host->TriggerObservers( retVal->GetEventContext(), EventTrigger::STIPostImmigrating );

        return retVal;
    }

    REGISTER_SERIALIZABLE(NodeSTI);

    void NodeSTI::serialize(IArchive& ar, NodeSTI* obj)
    {
        Node::serialize(ar, obj);
        // NodeSTI& node = *obj;
        // clorton TODO
    }
}
