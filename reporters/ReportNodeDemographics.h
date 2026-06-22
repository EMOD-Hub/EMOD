
#pragma once

#include "BaseTextReport.h"
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class IPKeyValueContainer;
    struct IIndividualHumanEventContext;

    class NodeData
    {
    public:
        NodeData()
        : num_people(0)
        , num_infected(0)
        {
        };

        virtual void Reset()
        {
            num_people = 0;
            num_infected = 0;
        }

        uint32_t num_people;
        uint32_t num_infected;
    };

    class ReportNodeDemographics: public BaseTextReport
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportNodeDemographics, IReport )
#endif
    public:
        ReportNodeDemographics();
        virtual ~ReportNodeDemographics();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;

    protected:
        ReportNodeDemographics( const std::string& rReportName );
        virtual NodeData* CreateNodeData();
        virtual void WriteNodeData( const NodeData* pData );
        virtual void LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData ) {};

        virtual bool HasOtherStratificationColumn() const;
        virtual std::string GetOtherStratificationColumnName() const;
        virtual int GetNumInOtherStratification() const;
        virtual std::string GetOtherStratificationValue( int otherIndex ) const;
        virtual int GetIndexOfOtherStratification( IIndividualHumanEventContext* individual ) const;

        int GetIPIndex( IPKeyValueContainer* pProps ) const;

        bool m_StratifyByGender;
        bool m_StratifyByAge;
        std::vector<float> m_AgeYears;
        std::string m_IPKeyToCollect;
        std::vector<std::string> m_IPValuesList;
        std::vector<std::vector<std::vector<std::vector<NodeData*>>>> m_Data; // dimensions - gender, age, IP, symptoms (malaria)
    };
}
