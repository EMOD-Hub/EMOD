
#pragma once

#include "ReportNodeDemographics.h"
#include "ReportUtilitiesMalaria.h"
#include "EventTrigger.h"
#include "BroadcasterObserver.h"
#include "IReportMalariaDiagnostics.h"
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class NodeDataMalaria : public NodeData
    {
    public:
        NodeDataMalaria()
            : NodeData()
            , sum_infectiousness(0.0)
            , sum_parasite_density(0.0)
            , sum_gametocyte_density(0.0)
            , sum_pfemp1(0.0)
            , sum_inf_cleard_dur(0.0)
            , num_inf_cleared(0)
            , num_infections( 0 )
            , num_has_symptoms( 0 )
            , num_has_fever( 0 )
        {
        };

        virtual void Reset()
        {
            NodeData::Reset();
            num_infections = 0;
            sum_infectiousness = 0.0;
            sum_parasite_density = 0.0;
            sum_gametocyte_density = 0.0;
            sum_pfemp1 = 0.0;
            sum_inf_cleard_dur = 0.0;
            num_inf_cleared = 0;
            num_has_symptoms = 0;
            num_has_fever = 0;
        }

        float sum_infectiousness;
        float sum_parasite_density;
        float sum_gametocyte_density;
        float sum_pfemp1;
        float sum_inf_cleard_dur;
        uint32_t num_inf_cleared;
        uint32_t num_infections;
        uint32_t num_has_symptoms;
        uint32_t num_has_fever;
    };

    class ReportNodeDemographicsMalaria : public ReportNodeDemographics
                                        , public IIndividualEventObserver
                                        , public IReportMalariaDiagnostics
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportNodeDemographicsMalaria, IReport )
#endif
    public:
        ReportNodeDemographicsMalaria();
        virtual ~ReportNodeDemographicsMalaria();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return ReportNodeDemographics::AddRef(); }
        virtual int32_t Release() override { return ReportNodeDemographics::Release(); }

        // ReportNodeDemographics
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        // IObserver
        bool notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger ) override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

    protected:
        ReportNodeDemographicsMalaria( const std::string& rReportName );
        virtual NodeData* CreateNodeData() override;
        virtual void WriteNodeData( const NodeData* pData ) override;
        virtual void LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData ) override;

        virtual bool HasOtherStratificationColumn() const override;
        virtual std::string GetOtherStratificationColumnName() const override;
        virtual int GetNumInOtherStratification() const override;
        virtual std::string GetOtherStratificationValue( int otherIndex ) const override;
        virtual int GetIndexOfOtherStratification( IIndividualHumanEventContext* individual ) const override;

        bool m_IsRegistered;
        bool m_StratifyBySymptoms;
        std::vector<float> m_DetectionThresholds;
        std::vector<EventTrigger> m_EventsOfInterest;
    };
}
