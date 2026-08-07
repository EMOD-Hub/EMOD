
#pragma once

#include <string>
#include <functional>

#include "Configure.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    struct INodeContext;
    struct INodeEventContext;
    struct IIndividualHuman;
    struct ISimulationEventContext;

    class IReport : public JsonConfigurable
    {
    public:
        virtual ~IReport() {} ;

        virtual bool Configure( const Configuration* inputJson ) = 0;

        virtual void Initialize(unsigned int nrmSize) = 0;
        virtual void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographicNodeIds) = 0;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) = 0;
        virtual void BeginTimestep() = 0;
        virtual void LogIndividualData( IIndividualHuman* individual ) = 0;
        virtual void LogNodeData( INodeContext* pNC ) = 0;
        virtual void EndTimestep( float currentTime, float dt) = 0;

        // Return true if LogIndividualData() should be called.
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const = 0 ;

        virtual void Reduce() = 0;
        virtual void Finalize() = 0;

        virtual std::string GetReportName() const = 0;
    };

    // This abstract class provides a default implementation to some of pure virtual methods
    // of IReport.  This allows other classes to be IReport's
    // without needing the equivalent of the code below.
    class BaseReport : public IReport
    {
    public:
        virtual ~BaseReport(){};

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual bool Configure( const Configuration* inputJson ) override { return true ; };
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) { return Kernel::e_NOINTERFACE; }
        virtual int32_t AddRef()  { return -1 ; }
        virtual int32_t Release() { return -1 ; }

        // -------------------
        // --- IReport Methods
        // -------------------
        virtual void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographicNodeIds) {};
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) {} ;
    };
}
