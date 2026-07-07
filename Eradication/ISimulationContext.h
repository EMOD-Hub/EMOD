
// put all contexts in one place to reduce clutter in includes
#pragma once
#include <vector>
#include "suids.hpp"
#include "ISupports.h"
#include "IdmApi.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    class IReport;
    class SimulationConfig;
    struct DemographicsContext;
    struct INodeQualifier;
    struct IIndividualHuman;
    struct IdmDateTime;
    class EventTrigger;
    struct ISimulationEventContext;
    struct ProcessMemoryInfo;
    struct SystemMemoryInfo;

    struct IDMAPI ISimulationContext : public ISupports
    {
        // demographics
        virtual const DemographicsContext* GetDemographicsContext() const = 0;

        // time services
        virtual const IdmDateTime& GetSimulationTime() const = 0;

        // memory services
        virtual void CheckMemoryFailure( bool onlyCheckForFailure ) = 0;
        virtual const ProcessMemoryInfo& GetProcessMemory() = 0;
        virtual const SystemMemoryInfo& GetSystemMemory() = 0;

        // id services
        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual suids::suid GetNodeSuid( ExternalNodeId_t external_node_id ) = 0;
        virtual ExternalNodeId_t GetNodeExternalID( const suids::suid& rNodeSuid ) = 0;
        virtual uint32_t    GetNodeRank( const suids::suid& rNodeSuid ) = 0;

        // migration
        virtual void PostMigratingIndividualHuman( IIndividualHuman *i ) = 0;
        virtual bool CanSupportFamilyTrips() const = 0;

        // events
        virtual void DistributeEventToOtherNodes( const EventTrigger& rEventTrigger, INodeQualifier* pQualifier ) = 0;
        virtual void UpdateNodeEvents() = 0;
        virtual ISimulationEventContext* GetSimulationEventContext() = 0;

        // reporting
        virtual std::vector<IReport*>& GetReports() = 0;
        virtual std::vector<IReport*>& GetReportsNeedingIndividualData() = 0;
    };
}
