
#pragma once

#include "BaseTextReportEvents.h"
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class ReportHumanMigrationTracking : public BaseTextReportEvents
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportHumanMigrationTracking, IReport )
#endif

    public:
        ReportHumanMigrationTracking();
        virtual ~ReportHumanMigrationTracking();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReportEvents::AddRef(); }
        virtual int32_t Release() override { return BaseTextReportEvents::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetHeader() const override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Reduce() override;

    private:
        struct MigrationData
        {
            MigrationData()
                : age_years(-1.0)
                , gender(-1)
                , home_node_id(-1)
                , node_id(-1)
                , migration_type_str()
            { }

            float         age_years;
            int           gender;
            bool          is_adult;
            uint32_t      home_node_id;
            uint32_t      node_id;
            std::string   migration_type_str;
        };

        float m_EndTime ;
        std::map<long,MigrationData> m_MigrationDataMap ;
    };
}
