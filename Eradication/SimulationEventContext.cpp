
#pragma once

#include "stdafx.h"

#include "SimulationEventContext.h"
#include "Simulation.h"
#include "Debug.h"
#include "CampaignEvent.h"
#include "EventCoordinator.h"
#include "Exceptions.h"
#include "Log.h"
#include "Configure.h"
#include "InterventionFactory.h"
#include "EventTriggerCoordinator.h"

/*
    Notes on extending ECH functionality
    Each of the ECH levels implements ISupports so that extended versions can be queried
    To expose more methods specific to your simulation/node/individual, first override the setupEventContext methods in your derived sim/node/ind type to instantiate a derived ECH
    Have your ECH implement an extended EC interface, and support querying it
    Then your event coordinators that need disease-specific info can query the appropriate sim/node/ind-ECH for the disease-specific EC

*/

SETUP_LOGGING( "SimulationEventContext" )

namespace Kernel
{
    IMPL_QUERY_INTERFACE1(SimulationEventContextHost, ISimulationEventContext)

    SimulationEventContextHost::SimulationEventContextHost(Simulation* _sim)
        : sim(_sim)
        , event_coordinators()
        , event_queue()
        , coordinator_broadcaster_impl()
        , node_broadcaster_impl()
    {
    }

    SimulationEventContextHost::SimulationEventContextHost()
        : sim(nullptr)
        , event_coordinators()
        , event_queue()
        , coordinator_broadcaster_impl()
        , node_broadcaster_impl()
    {
    }

    const IdmDateTime&
    SimulationEventContextHost::GetSimulationTime()
    const
    {
        return sim->GetSimulationTime();
    }

    int
    SimulationEventContextHost::GetSimulationTimestep()
    const
    {
        return sim->GetSimulationTimestep();
    }

    SimulationEventContextHost::~SimulationEventContextHost()
    {
        while (!event_queue.empty())
        {
            CampaignEvent *ce = event_queue.top();
            ce->Release();
            event_queue.pop();
        }

        for (auto event_coordinator : event_coordinators)
        {
            event_coordinator->Release();
        }
        event_coordinators.clear();
    }


    void SimulationEventContextHost::VisitNodes(node_visit_function_t func)
    {
        int visited_nodes = 0;

        for (auto& entry : sim->nodes)
        {
            visited_nodes += func(entry.first, entry.second->GetEventContext());
        }

        if (visited_nodes == 0 )
        {
            LOG_WARN("No nodes were visited.  No nodes were added to the event coordinator.\n");
        }
        else
        {
            LOG_INFO_F("%d node(s) visited.\n", visited_nodes);
        }
    }

    INodeEventContext* SimulationEventContextHost::GetNodeEventContext( suids::suid node_id )
    {
        return sim->nodes.at(node_id)->GetEventContext();
    }

    void SimulationEventContextHost::Update( float dt )
    {
        // check the queue, execute pending events for this timestep
        LOG_DEBUG_F("sim->GetSimulationTime() = %f, event_queue.top()->GetStartDay() = %f\n", float(sim->GetSimulationTime().time), event_queue.empty() ? -1 : event_queue.top()->GetStartDay());
        while (!(event_queue.empty()) && (float(sim->GetSimulationTime().time) >= event_queue.top()->GetStartDay()))
        {
            CampaignEvent *next_event = event_queue.top();
            LOG_INFO("Time for campaign event. Calling Dispatch...\n");
            next_event->Dispatch(this);
            event_queue.pop();
           
            next_event->Release();
                
        }

        // update registered event coordinators
        for (auto event_coordinator : event_coordinators)
        {
            event_coordinator->Update(dt);
        }

        for (auto event_coordinator : event_coordinators)
        {
            event_coordinator->UpdateNodes(dt);
        }


        // remove terminate ECs
        for (auto iterator = event_coordinators.begin(); iterator != event_coordinators.end(); )
        {
            IEventCoordinator *iec = *iterator;
            if (iec->IsFinished())
            {
                iec->Release();
                iterator = event_coordinators.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }

        coordinator_broadcaster_impl.DisposeOfUnregisteredObservers();
        node_broadcaster_impl.DisposeOfUnregisteredObservers();
    }

    void SimulationEventContextHost::LoadCampaignFromFile(const std::string& campaignfile, const std::vector<ExternalNodeId_t>& nodeIds_demographics)
    {
        using namespace json;
        Configuration *campaign = Configuration::Load(campaignfile);
        if( !campaign )
        {
            // Configuration::Load returned a null pointer. Json may be malformed. We will not continue without the campaign. Don't actually
            // know campaign filename used, just have ifstream. Guessing name based on 1E7(?) samples. :)
            throw FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, "campaign.json" );
        }
   
        try 
        {
            // instantiate a campaign event for each entry in the file
            if( campaign->Exist( "Use_Defaults" ) )
            {
                // store value of Use_Defaults from campaign.json in InterventionFactory.
                InterventionFactory::SetUseDefaults( ((*campaign)["Use_Defaults"].As< json::Number >() != 0) );
                // We're about to parse some campaign event stuff from campaign.json. Set JC::_useDefaults.
                JsonConfigurable::_useDefaults = InterventionFactory::IsUsingDefaults();
                LOG_DEBUG_F( "UseDefault values for campaign.json (when key not found) specified in campaign.json as: %d.\n", JsonConfigurable::_useDefaults );
            }
            else
            {
                LOG_DEBUG( "Do NOT Use default values for campaign.json when key not found since not specified in campaign.json and 'default' is false.\n" );
            }

            Array events = (*campaign)["Events"].As<Array>();

            for (int k= 0; k < events.Size(); k++ )
            {
                std::stringstream param_name;
                param_name << "Events[" << k << "]";

                CampaignEvent *ce = CampaignEventFactory::getInstance()->CreateInstance( events[k], campaign->GetDataLocation(), param_name.str().c_str() );
                if (ce)
                {
                    ce->CheckForValidNodeIDs(nodeIds_demographics);
                    if( ce->GetStartDay() < sim->GetSimulationTime().time )
                    {
                        LOG_WARN_F("Discarding old event for t=%0.1f.\n", ce->GetStartDay());
                        delete ce;
                        ce = nullptr;
                        continue;
                    }
                    ce->SetEventIndex(k);
                    event_queue.push(ce);
                } 
                else
                {
                    std::stringstream s ;
                    s << "Failure loading campaign events: could not instantiate object for Event " << k << std::endl ;
                    throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() ); // JB hint-y
                }
            }
        }
        catch( FactoryCreateFromJsonException& )
        {
            // rethrow
            throw ;
        }
        catch (ParseException &pe)
        {
            std::string errMsg;
            JsonUtility::logJsonException( pe, errMsg );
            throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errMsg.c_str() );
        }
        catch (ScanException &se)
        {
            std::string errMsg;
            JsonUtility::logJsonException( se, errMsg );
            throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errMsg.c_str() );
        }
        catch (json::Exception &e)
        {
            std::string raw_msg = std::string( e.what() );
            if( raw_msg.find( "Object name not found" ) != std::string::npos )
            {
                unsigned pos = raw_msg.find( ":" );
                std::string param_name = raw_msg.substr( pos+2 );
                throw Kernel::MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, campaignfile.c_str(), param_name.c_str() );
            }
            else
            {
                throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
            }
        }
        JsonConfigurable::_useDefaults = false;
    }

    void SimulationEventContextHost::RegisterEventCoordinator( IEventCoordinator* iec )
    {
        event_coordinators.push_back(iec); iec->AddRef();
    }

    ICoordinatorEventBroadcaster* SimulationEventContextHost::GetCoordinatorEventBroadcaster()
    {
        return this;
    }

    INodeEventBroadcaster* SimulationEventContextHost::GetNodeEventBroadcaster()
    {
        return this;
    }

    void SimulationEventContextHost::RegisterObserver( ICoordinatorEventObserver* pObserver,
                                                       const EventTriggerCoordinator& trigger )
    {
        coordinator_broadcaster_impl.RegisterObserver( pObserver, trigger );
    }

    void SimulationEventContextHost::UnregisterObserver( ICoordinatorEventObserver* pObserver,
                                                         const EventTriggerCoordinator& trigger )
    {
        coordinator_broadcaster_impl.UnregisterObserver( pObserver, trigger );
    }

    void SimulationEventContextHost::TriggerObservers( IEventCoordinatorEventContext* pCoordinator,
                                                       const EventTriggerCoordinator& trigger )
    {
        coordinator_broadcaster_impl.TriggerObservers( pCoordinator, trigger );
    }

    uint64_t SimulationEventContextHost::GetNumTriggeredEvents()
    {
        // This is a weird interface thing where the same method is actually for different data.
        // The problem could be that this SimulationEventContext should not be implementing both
        // interfaces.  I don't want this data anyway so I'm going to leave this hear for now.
        release_assert( false );
        return 0;
    }

    uint64_t SimulationEventContextHost::GetNumObservedEvents()
    {
        // This is a weird interface thing where the same method is actually for different data.
        // The problem could be that this SimulationEventContext should not be implementing both
        // interfaces.  I don't want this data anyway so I'm going to leave this here for now.
        release_assert( false );
        return 0;
    }

    void SimulationEventContextHost::RegisterObserver( INodeEventObserver* pObserver,
                                                       const EventTriggerNode& trigger )
    {
        node_broadcaster_impl.RegisterObserver( pObserver, trigger );
    }

    void SimulationEventContextHost::UnregisterObserver( INodeEventObserver* pObserver,
                                                         const EventTriggerNode& trigger )
    {
        node_broadcaster_impl.UnregisterObserver( pObserver, trigger );
    }

    void SimulationEventContextHost::TriggerObservers( INodeEventContext* pNodeEventContext,
                                                       const EventTriggerNode& trigger )
    {
        node_broadcaster_impl.TriggerObservers( pNodeEventContext, trigger );
    }

    void SimulationEventContextHost::propagateContextToDependents()
    {
        for (auto event_coordinator : event_coordinators)
        {
            event_coordinator->SetContextTo(this);
        }
    }

    bool SimulationEventContextHost::campaign_event_comparison::operator()(const CampaignEvent* _Left, const CampaignEvent* _Right) const
    {    // Apply operator< to operands:
        // - order the priority_queue by start day
        // - order events on same start day by their order in the campaign JSON file
        return ( (_Left->GetStartDay() > _Right->GetStartDay()) || 
                 (_Left->GetStartDay() == _Right->GetStartDay() && _Left->GetEventIndex() > _Right->GetEventIndex()) );
    }
}

namespace Kernel {
    struct campaign_event_comparison //: public binary_function<CampaignEvent *, CampaignEvent*, bool>
    {
        bool operator()(const CampaignEvent* _Left, const CampaignEvent* _Right) const;
    };

    bool campaign_event_comparison::operator()(const CampaignEvent* _Left, const CampaignEvent* _Right) const
    {    // Apply operator< to operands:
        // - order the priority_queue by start day
        // - order events on same start day by their order in the campaign JSON file
        return ( (_Left->GetStartDay() > _Right->GetStartDay()) || 
                 (_Left->GetStartDay() == _Right->GetStartDay() && _Left->GetEventIndex() > _Right->GetEventIndex()) );
    }

#if 0
    template<class Archive>
    void serialize(Archive &ar, SimulationEventContextHost& sech, const unsigned int v)
    {
        ar & sech.sim; // hope this works!
        ar & sech.event_coordinators;
        ar & sech.campaign_filename;
        if (typename Archive::is_loading())
        {
            sech.LoadCampaignFromFile(sech.campaign_filename);
        }
    }
#endif
}
