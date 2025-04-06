
#pragma once

#include <vector>

#include "BaseTextReportEvents.h"
#include "ReportFactory.h"
#include "ReportFilter.h"

namespace Kernel
{
    class ReportFpgNewInfections : public BaseTextReportEvents
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportFpgNewInfections, IReport )
    public:
        ReportFpgNewInfections();
        virtual ~ReportFpgNewInfections();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReportEvents::AddRef(); }
        virtual int32_t Release() override { return BaseTextReportEvents::Release(); }

        // BaseTextReportEvents
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsValidNode( INodeEventContext* pNEC ) const override;
        virtual std::string GetHeader() const override;
        virtual std::string GetReportName() const override;

        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger) override;

    protected:
        ReportFilter m_ReportFilter;
        bool m_OutputWritten;
    };
}