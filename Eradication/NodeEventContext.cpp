
#include "stdafx.h"
#include <typeinfo>

#include "ISimulationContext.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "Debug.h"
#include "NodeDemographics.h"
#include "Node.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "EventTrigger.h"
#include "StrainIdentity.h"
#include "IdmDateTime.h"
#include "Log.h"
#include "Exceptions.h"

SETUP_LOGGING( "NodeEventContext" )

namespace Kernel
{
    NodeEventContextHost::NodeEventContextHost()
    : node(nullptr)
    , arrival_distribution_sources()
    , departure_distribution_sources()
    , interventions()
    , node_interventions()
    , broadcaster_impl()
    {
        arrival_distribution_sources.clear();
    }

    NodeEventContextHost::NodeEventContextHost(Node* _node)
    : node(_node)
    , arrival_distribution_sources()
    , departure_distribution_sources()
    , interventions()
    , node_interventions()
    , broadcaster_impl()
    {
        arrival_distribution_sources.clear();
    }

    // This was done with macros, but prefer actual code.
    Kernel::QueryResult NodeEventContextHost::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(INodeEventContext)) 
            foundInterface = static_cast<INodeEventContext*>(this);
        else if (iid == GET_IID(INodeInterventionConsumer))
            foundInterface = static_cast<INodeInterventionConsumer*>(this);
        else if (iid == GET_IID(IOutbreakConsumer))
            foundInterface = static_cast<IOutbreakConsumer*>(this);
        else if (iid == GET_IID(ICampaignCostObserver))
            foundInterface = static_cast<ICampaignCostObserver*>(this);
        else if (iid == GET_IID( IIndividualEventBroadcaster ))
            foundInterface = static_cast<IIndividualEventBroadcaster*>(this);
        // -->> add support for other I*Consumer interfaces here <<--      
        else if (iid == GET_IID(INodeContext))
            foundInterface = (INodeContext*)node;
        else
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
            //status = InterventionsContainer::QueryInterface(iid, (void**)&foundInterface);
        }

        *ppinstance = foundInterface;
        return status;
    }

    void
    NodeEventContextHost::SetContextTo(
        INodeContext* context
    )
    {
        
        PropagateContextToDependents();
    }

    // method 1 for VisitIndividuals uses a functor/lambda function
    void NodeEventContextHost::VisitIndividuals( individual_visit_function_t func )
    {
        for( auto individual : node->individualHumans)
        {
            // TODO: I would like this function to return bool so we can keep a total of 'successful' calls, but
            // compiler threw up on me when I tried that! :(
            func(individual->GetEventContext());
        }
    }

    // method 2 for VisitIndividuals uses an interface
    int
    NodeEventContextHost::VisitIndividuals(
        IVisitIndividual * pEventCoordinator
    )
    {
        int retTotal = 0;
        float nodeCostThisInterventionThisTimestep = 0.0f;
        for( auto individual : node->individualHumans)
        {
            float costForThisIntervention = 0.0f;
            if( pEventCoordinator->visitIndividualCallback(individual->GetEventContext(), costForThisIntervention, this))
            {
                retTotal++;
                //nodeCostThisInterventionThisTimestep += (float)( costForThisIntervention * ih->GetMonteCarloWeight() );
            }
        }

        IncrementCampaignCost( nodeCostThisInterventionThisTimestep );
        LOG_DEBUG_F("Node CampaignCost = %f\n", nodeCostThisInterventionThisTimestep);
        return retTotal;
    }

    void NodeEventContextHost::notifyCampaignExpenseIncurred(
        float expenseIncurred,
        const IIndividualHumanEventContext * pIndiv
    )
    {
        release_assert( node );
        if( expenseIncurred > 0 )
        {
            LOG_DEBUG("Campaign expense was incurred\n");
        }
        float cost = expenseIncurred;
        if( pIndiv != nullptr )
        {
            cost *= float( pIndiv->GetMonteCarloWeight() );
        }
        IncrementCampaignCost( cost );
    }

    // First cut of this function writes the intervention report to stdout. It is an abbreviated,
    // 1-line-per-intervention report.
    void NodeEventContextHost::notifyCampaignEventOccurred(
        /*const*/ ISupports * pDistributedIntervention,
        /*const*/ ISupports * pDistributor,// for tb, cool to know parent intervention that 'gave', including tendency if it's an HSB.
        /*const*/ IIndividualHumanContext * pDistributeeIndividual
    )
    {
        if( Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "EEL") )
        {
            // intervention recipient
            std::stringstream msg;
            float recipientAge = float(pDistributeeIndividual->GetEventContext()->GetAge());
            msg << "hum_id="
                << pDistributeeIndividual->GetSuid().data
                << ",ra="
                << recipientAge
                << std::endl;
            Environment::getInstance()->Log->Log(Logger::DEBUG, "EEL","%s\n", msg.str().c_str() );
        }
    }

    const NodeDemographics& NodeEventContextHost::GetDemographics()
    {
        return node->demographics;
    }

    const suids::suid &
    NodeEventContextHost::GetId()
    const
    {
        return node->suid;
    }

    void NodeEventContextHost::ProcessArrivingIndividual( IIndividualHuman* ih )
    {
        for (auto& entry : arrival_distribution_sources)
        {
            for (int k = 0; k < entry.second; k++)
                entry.first->ProcessArriving(ih->GetEventContext());
        }
    }

    void NodeEventContextHost::ProcessDepartingIndividual( IIndividualHuman* ih )
    {
        //LOG_DEBUG( "ProcessDepartingIndividual\n" );
        for (auto& entry : departure_distribution_sources)
        {
            LOG_DEBUG( "ProcessDepartingIndividual for event coordinator\n" );
            for (int k = 0; k < entry.second; k++)
            {
                LOG_DEBUG( "ProcessDepartingIndividual: Calling ProcessDeparting for individual\n" );
                entry.first->ProcessDeparting(ih->GetEventContext());
            }
        }
    }

    void NodeEventContextHost::RegisterTravelDistributionSource( ITravelLinkedDistributionSource *tlds, TravelEventType type )
    {
        travel_distribution_source_map_t *sources = sourcesMapForType(type);        
        if (sources->find(tlds) != sources->end())
            (*sources)[tlds]++;
        else
            (*sources)[tlds] = 1;

        tlds->AddRef();
    }

    void NodeEventContextHost::UnregisterTravelDistributionSource( ITravelLinkedDistributionSource *tlds, TravelEventType type )
    {
        travel_distribution_source_map_t *sources = sourcesMapForType(type);        
        if (sources->find(tlds) != sources->end())
        {
            (*sources)[tlds]--;
            tlds->Release();

            if ((*sources)[tlds] == 0)
            {
                sources->erase(tlds);
            }
        } else
        {
            // unregistering something that wasnt registered...probably an error
        }
    }

    void NodeEventContextHost::RegisterObserver(
        IIndividualEventObserver * pObserver,
        const EventTrigger& trigger
    )
    {
        broadcaster_impl.RegisterObserver( pObserver, trigger );
    }

    void NodeEventContextHost::UnregisterObserver(
        IIndividualEventObserver * pObserver,
        const EventTrigger& trigger
    )
    {
        broadcaster_impl.UnregisterObserver( pObserver, trigger );
    }

    void NodeEventContextHost::TriggerObservers(
        IIndividualHumanEventContext *pIndiv,
        const EventTrigger& trigger
    )
    {
        broadcaster_impl.TriggerObservers( pIndiv, trigger );
    }

    uint64_t NodeEventContextHost::GetNumTriggeredEvents()
    {
        return broadcaster_impl.GetNumTriggeredEvents();
    }

    uint64_t NodeEventContextHost::GetNumObservedEvents()
    {
        return broadcaster_impl.GetNumObservedEvents();
    }

    void NodeEventContextHost::PropagateContextToDependents()
    {
        for (auto intervention : node_interventions)
        {
            intervention->SetContextTo( this );
        }
    }

    void NodeEventContextHost::IncrementCampaignCost(float cost)
    {
        node->Campaign_Cost += cost;
    }

    void NodeEventContextHost::UpdateInterventions(float dt)
    {
        std::vector<INodeDistributableIntervention*> expired_list;
        for( auto intervention : node_interventions )
        {
            intervention->Update( dt );
            if( intervention->Expired() )
            {
                expired_list.push_back( intervention );
            }
        }

        for( auto intven : expired_list )
        {
            node_interventions.remove( intven );
            delete intven;
        }

        broadcaster_impl.DisposeOfUnregisteredObservers();
    }

    bool NodeEventContextHost::GiveIntervention( INodeDistributableIntervention* iv )
    {
        node_interventions.push_back( iv );
        iv->AddRef();

        IBaseIntervention * pBaseIV = nullptr;
        if( iv->QueryInterface( GET_IID(IBaseIntervention), (void**)&pBaseIV ) == s_OK ) 
        {
            IncrementCampaignCost( pBaseIV->GetCostPerUnit() );
            pBaseIV->Release();
        }
        else
        {
            LOG_WARN_F("Unsuccessful in querying for IBaseIntervention from INodeDistributableIntervention (%s) for passing Campaign_Cost back to Node.\n", typeid(*iv).name());
        }
        iv->SetContextTo( this );
        return true;
    }

    void NodeEventContextHost::PurgeExistingByType( const std::string& type_name )
    {
        // returns the first element that satisfies the condition or .end() if no such element is found.
        auto it = std::find_if( node_interventions.begin(), node_interventions.end(),
                                [&]( INodeDistributableIntervention* intervention ) {
                                    return typeid( *intervention ).name() == type_name;
                                } );

        if(it != node_interventions.end()) {
            LOG_INFO_F( "Found an existing intervention '%s' in node %d. Purging.\n",
                        type_name.c_str(), GetId() );

            delete* it;    // destroy the intervention
            node_interventions.erase( it ); // remove the pointer from the list
        }
    }


    void NodeEventContextHost::PurgeExistingByName( const std::string& type_name, const InterventionName& iv_name)
    {
        // returns the first element that satisfies the condition or .end() if no such element is found.
        auto it = std::find_if( node_interventions.begin(), node_interventions.end(),
                                [&]( INodeDistributableIntervention* intervention ) {
                                    return ( type_name == typeid( *intervention ).name() && intervention->GetName() == iv_name );
                                } );

        if(it != node_interventions.end()) {
            LOG_INFO_F( "Found an existing intervention with name '%s' in node %d. Purging.\n",
                        iv_name.c_str(), GetId() );

            delete* it;    // destroy the intervention
            node_interventions.erase( it ); // remove the pointer from the list
        }
    }


    const std::list<INodeDistributableIntervention*>& NodeEventContextHost::GetNodeInterventions() const
    {
        return node_interventions;
    }


    NodeEventContextHost::~NodeEventContextHost()
    {
        cleanupDistributionSourceMap(arrival_distribution_sources);
        cleanupDistributionSourceMap(departure_distribution_sources);

        for (auto intervention : node_interventions)
        {
            delete intervention;
        }
    }

    NodeEventContextHost::travel_distribution_source_map_t* NodeEventContextHost::sourcesMapForType( INodeEventContext::TravelEventType type )
    {
        travel_distribution_source_map_t *sources = nullptr;
        switch (type)
        {
        case Arrival: sources = &arrival_distribution_sources; break;
        case Departure: sources = &departure_distribution_sources; break;
        default: break;
        }
        return sources;
    }

    void NodeEventContextHost::cleanupDistributionSourceMap( travel_distribution_source_map_t &sources )
    {
        for (auto& entry : sources)
        {
            for (int k = 0; k < entry.second; k++)
                entry.first->Release();
        }

        sources.clear();
    }

    void NodeEventContextHost::AddImportCases(
        IStrainIdentity* outbreak_strainID,
        float import_age,
        NaturalNumber num_cases_per_node,
        ProbabilityNumber prob_infection
    )
    {
        for (int i = 0; i < num_cases_per_node; i++)
        {
            IIndividualHuman* new_individual = node->configureAndAddNewIndividual(1.0, import_age, 0.0, 0.5); // using age specified by Oubreak, but otherwise community demographics for import case (e.g. immune history)

            // 0 = incubation_period_override, outbreaks are instantaneously mature
            if( GetRng()->SmartDraw( prob_infection ) )
            {
                new_individual->AcquireNewInfection( outbreak_strainID, 0 ); 
            }
        }
    }

    bool NodeEventContextHost::ContainsExistingByName( const InterventionName& iv_name )
    {
        for( auto intervention : node_interventions )
        {
            if( intervention->GetName() == iv_name )
            {
                return true;
            }
        }
        return false;
    }

    // These are all pass-throughs in order to get rid of GetNode() as an interface method.
    bool NodeEventContextHost::IsInPolygon(float* vertex_coords, int numcoords)
    {
        return node->IsInPolygon( vertex_coords, numcoords );
    }

    bool NodeEventContextHost::IsInPolygon( const json::Array &poly )
    {
        return node->IsInPolygon( poly );
    }

    bool NodeEventContextHost::IsInExternalIdSet( const std::list<ExternalNodeId_t>& nodelist )
    {
        return node->IsInExternalIdSet( nodelist );
    }

    const IdmDateTime& NodeEventContextHost::GetTime() const
    {
        return node->GetTime();
    }

    RANDOMBASE*  NodeEventContextHost::GetRng()
    {
        return node->GetRng();
    } 

    INodeContext* NodeEventContextHost::GetNodeContext()
    {
        return static_cast<INodeContext*>(node);
    }

    int NodeEventContextHost::GetIndividualHumanCount() const
    {
        size_t nodePop = node->individualHumans.size();
        return int(nodePop);
    }

    ExternalNodeId_t NodeEventContextHost::GetExternalId() const
    {
        ExternalNodeId_t nodeId = node->GetExternalID();
        return nodeId;
    }

    IIndividualEventBroadcaster* NodeEventContextHost::GetIndividualEventBroadcaster()
    {
        return this;
    }
}
