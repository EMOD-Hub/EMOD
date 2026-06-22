
#pragma once

#include "BaseTextReport.h"
#include "IVectorMigrationReporting.h"
#include "VectorEnums.h"
#include "VectorGenome.h"
#include "VectorGene.h"
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class ReportVectorMigration : public BaseTextReport, public IVectorMigrationReporting
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportVectorMigration, IReport )
#endif

    public:
        ReportVectorMigration();
        virtual ~ReportVectorMigration();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetHeader() const override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void LogVectorMigration( ISimulationContext* pSim,
                                         float currentTime, 
                                         const suids::suid& nodeSuid, 
                                         IVectorCohort* pivc ) override;
    protected:
        float m_StartDay;
        float m_EndDay;
        bool m_IsValidDay;
        std::vector<VectorStateEnum::Enum> m_MustBeInState;
        std::vector<ExternalNodeId_t> m_MustBeFromNode;
        std::vector<ExternalNodeId_t> m_MustBeToNode;
        bool m_IncludeGenomeData;
        std::vector<std::string> m_SpeciesList;
        std::string m_FilenameSuffix;
    };
}
