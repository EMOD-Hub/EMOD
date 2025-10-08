
#pragma once

#include <string>
#include <list>

#include "IdmApi.h"

#include "ISupports.h"
#include "Types.h"
#include "Configuration.h"
#include "suids.hpp"
#include "IndividualEventContext.h"
#include "Interventions.h" // for IIndividualHumanEventObserver
#include "ExternalNodeId.h"

namespace Kernel
{
    struct INodeContext;
    struct IIndividualEventBroadcaster;
    struct NodeDemographics;
    class Node;
    struct IStrainIdentity;
    
    class IndividualHuman;
    class RANDOMBASE;
    struct IdmDateTime;

    /* possible TODO: could differentiate these in the future to provide extra information related to the travel
    struct IIndividualHumanTravelLinkedDistributionContext : public IIndividualHumanEventContext
    {

    };*/

    struct IDMAPI IOutbreakConsumer : public ISupports
    {
        virtual void AddImportCases( IStrainIdentity* outbreak_strainID, float import_age, NaturalNumber num_cases_per_node, ProbabilityNumber prob_infect ) = 0;
    };

    struct ITravelLinkedDistributionSource : ISupports
    {
        virtual void ProcessDeparting(IIndividualHumanEventContext *dc) = 0;
        virtual void ProcessArriving(IIndividualHumanEventContext *dc) = 0;
    };

    struct IVisitIndividual
    {
        virtual bool visitIndividualCallback( IIndividualHumanEventContext *ihec, float & incrementalCostOut, ICampaignCostObserver * pICCO ) = 0;
        virtual ~IVisitIndividual() {}
    };

    struct IDMAPI INodeEventContext : public ISupports 
    {
        // If you prefer lambda functions/functors, you can use this.
        typedef std::function<void (/*suids::suid, */IIndividualHumanEventContext*)> individual_visit_function_t;
        virtual void VisitIndividuals(individual_visit_function_t func) = 0;
        virtual int VisitIndividuals(IVisitIndividual* pIndividualVisitImpl) = 0;

        virtual const NodeDemographics& GetDemographics() = 0;
        virtual const IdmDateTime& GetTime() const = 0;
        //virtual float GetYear() const = 0;

        // to update any node-owned interventions
        virtual void UpdateInterventions(float = 0.0f) = 0;
        
        // methods to install hooks for arrival/departure/birth/etc
        enum TravelEventType { Arrival = 0, Departure = 1 };
        virtual void RegisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type) = 0;
        virtual void UnregisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type) = 0;

        virtual const suids::suid & GetId() const = 0;
        virtual void SetContextTo(INodeContext* context) = 0;
        virtual bool ContainsExistingByName( const InterventionName& iv_name ) = 0;
        virtual void PurgeExistingByType( const std::string& type_name) = 0;
        virtual void PurgeExistingByName( const std::string& type_name, const InterventionName& iv_name ) = 0;
        virtual const std::list<INodeDistributableIntervention*>& GetNodeInterventions() const = 0;

        virtual bool IsInPolygon(float* vertex_coords, int numcoords) = 0;
        virtual bool IsInPolygon( const json::Array &poly ) = 0;
        virtual bool IsInExternalIdSet( const std::list<ExternalNodeId_t>& nodelist ) = 0;
        virtual RANDOMBASE* GetRng() = 0;
        virtual INodeContext* GetNodeContext() = 0;

        virtual int GetIndividualHumanCount() const = 0;
        virtual ExternalNodeId_t GetExternalId() const = 0;

        virtual IIndividualEventBroadcaster* GetIndividualEventBroadcaster() = 0;
    };

    class Simulation;
    struct IBTNTI;      // BasicTestNodeTargetedIntervention

    struct IBTNTIConsumer : public ISupports
    {
        virtual void GiveBTNTI(IBTNTI* BTNTI) = 0;
        virtual const IBTNTI* GetBTNTI() = 0; // look at the BTNTI they have...why not, some campaigns may want to introspect on this sort of thing. returns NULL if there is no BTNTI currently
    };
}
