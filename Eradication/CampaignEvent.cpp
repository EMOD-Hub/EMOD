
#include "stdafx.h"
#include <string>
#include <functional>

#include "ISupports.h"
#include "Debug.h"
#include "EnumSupport.h"
#include "Environment.h"
#include "Sugar.h"
#include "Log.h"
#include "Configuration.h"
#include "Configure.h"
#include "ConfigurationImpl.h"
#include "CampaignEvent.h"
#include "EventCoordinator.h"
#include "FactorySupport.h"
#include "SimulationEventContext.h"
#include "ObjectFactoryTemplates.h"

SETUP_LOGGING( "CampaignEvent" )

using namespace std;

namespace Kernel
{
    CampaignEventFactory* CampaignEventFactory::_instance = nullptr;

    template CampaignEventFactory* ObjectFactory<CampaignEvent, CampaignEventFactory>::getInstance();

    CampaignEventFactory::CampaignEventFactory()
        : ObjectFactory<CampaignEvent, CampaignEventFactory>( false ) // do not queryForReturnInterface
    {
    }

    CampaignEvent* CampaignEventFactory::CreateInstance( const json::Element& rJsonElement,
                                                         const std::string& rDataLocation,
                                                         const char* parameterName )
    {
        CampaignEvent *ce = ObjectFactory<CampaignEvent, CampaignEventFactory>::CreateInstance( rJsonElement,
                                                                                                rDataLocation,
                                                                                                parameterName );
        release_assert(ce);

        if (ce && !JsonConfigurable::_dryrun)
        {           
            // now try to instantiate and configure the NodeSet and the EventCoordinator

            ce->nodeset = NodeSetFactory::getInstance()->CreateInstance( ce->nodeset_config._json,
                                                                         rDataLocation,
                                                                         "Nodeset_Config" );

            if (!ce->nodeset)
            {
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__,
                                                      std::string( json::QuickInterpreter(ce->nodeset_config._json)["class"].As<String>()).c_str() );
            }

            ce->event_coordinator = EventCoordinatorFactory::getInstance()->CreateInstance( ce->event_coordinator_config._json,
                                                                                            rDataLocation,
                                                                                            "Event_Coordinator_Config" );

            if (!ce->event_coordinator)
            {
                throw FactoryCreateFromJsonException(__FILE__, __LINE__, __FUNCTION__,
                                                      string( json::QuickInterpreter(ce->event_coordinator_config._json)["class"].As<String>()).c_str());
            }

            // make sure the start day for the coordinator makes sense
            ce->event_coordinator->CheckStartDay( ce->GetStartDay() );
        }
        return ce;
    }

    // CampaignEvent

    IMPLEMENT_FACTORY_REGISTERED(CampaignEvent)

    IMPL_QUERY_INTERFACE1(CampaignEvent, IConfigurable)

    CampaignEvent::CampaignEvent()
        : start_day(0.0f)
        , event_index(0)
        , nodeset(nullptr)
        , event_coordinator(nullptr)
        , nodeset_config()
        , event_coordinator_config()
    {
    }

    bool CampaignEvent::Configure(const Configuration * inputJson)
    {
        initConfigTypeMap( "Start_Day", &start_day, Start_Day_DESC_TEXT, 0 );
        initConfigComplexType( "Nodeset_Config", &nodeset_config, Nodeset_Config_DESC_TEXT );
        initConfigComplexType( "Event_Coordinator_Config", &event_coordinator_config, Event_Coordinator_Config_DESC_TEXT );
        return JsonConfigurable::Configure( inputJson );
    }

    void CampaignEvent::Dispatch( ISimulationEventContext *isec )
    {
        event_coordinator->SetContextTo(isec);

        // this activates the event_coordinator for the nodeset
        ISimulationEventContext::node_visit_function_t visit_func = 
            [this](suids::suid node_id, INodeEventContext *nec) -> bool
            {
                if (nodeset->Contains(nec))
                {
                    event_coordinator->AddNode(node_id);
                    return true;
                }
                else
                {
                    return false;
                }
            };
       
        isec->VisitNodes(visit_func); // add all the nodes
        isec->RegisterEventCoordinator(event_coordinator);

    }

    CampaignEvent::~CampaignEvent()
    {
        if (event_coordinator) { event_coordinator->Release(); event_coordinator = nullptr;} 
        if (nodeset) { nodeset->Release(); nodeset = nullptr; }
    }

    float CampaignEvent::GetStartDay() const { return start_day; }

    int CampaignEvent::GetEventIndex() const { return event_index; }

    void CampaignEvent::SetEventIndex(int index) { event_index = index; }

    void CampaignEvent::CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        std::vector<ExternalNodeId_t> nodes_missing_in_demographics = nodeset->IsSubset(demographic_node_ids);
        if (!nodes_missing_in_demographics.empty())
        {
            std::stringstream nodes_missing_in_demographics_str;
            std::copy(nodes_missing_in_demographics.begin(), nodes_missing_in_demographics.end(), ostream_iterator<int>(nodes_missing_in_demographics_str, " "));  // list of missing nodes

            std::stringstream error_msg;
            error_msg << std::string("Found NodeIDs in the campaign that are missing in demographics: ") << nodes_missing_in_demographics_str.str();
            error_msg << std::string(". Only nodes configured in demographics can be used in a campaign.");

            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, error_msg.str().c_str());
        }
    }

}
