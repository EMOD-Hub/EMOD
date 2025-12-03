#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"
#include "ReportFilter.h"

namespace Kernel
{
    class ReportAntibodies: public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportAntibodies, IReport )
    public:
        ReportAntibodies();
        virtual ~ReportAntibodies();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;


    protected:
        ReportAntibodies( const std::string& rReportName );
        
        ReportFilter m_ReportFilter;
        float m_ReportingInterval;
        bool  m_InfectedOnly;
        bool  m_IsCapacityData;
        float m_NextDayToCollectData;
        bool  m_IsCollectingData;
    };
}
