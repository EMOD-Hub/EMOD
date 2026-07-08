
#pragma once

#include "BaseTextReport.h"
#include "ReportUtilitiesMalaria.h"
#include "suids.hpp"
#include "ReportFactory.h"

namespace Kernel
{
    struct IVectorPopulationReporting;

    class ReportMicrosporidia : public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportMicrosporidia, IReport )
    public:
        ReportMicrosporidia();
        virtual ~ReportMicrosporidia();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetHeader() const override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

    protected:
        ReportMicrosporidia( const std::string& rReportName );

        std::vector<std::string> m_SpeciesList ;
        std::vector<std::vector<std::string>> m_MsStrainNamesBySpecies;
        bool m_IncludeMicrosporidiaModifier;
    };
}
