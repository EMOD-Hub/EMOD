
#pragma once

#include "BinnedReport.h"
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class VectorHabitatReport : public BinnedReport
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, VectorHabitatReport, IReport )
#endif

    public:
        VectorHabitatReport();
        virtual ~VectorHabitatReport();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BinnedReport::AddRef(); }
        virtual int32_t Release() override { return BinnedReport::Release(); }

        virtual void Initialize( unsigned int nrmSize ) override;

        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override { return false ; };
        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

    protected:
        virtual void initChannelBins() override;

        void clearChannelsBins();
        void Accumulate( const std::string& channel_name, const ChannelDataMap::channel_data_t& binned_data);
        int calcBinIndex(const Kernel::IIndividualHuman* individual);

        typedef std::map< std::string, int > habitat_idx_map_t;
        habitat_idx_map_t species_habitat_idx_map;  // e.g. ("arabiensis:CONSTANT", 1)

        ChannelDataMap::channel_data_t current_habitat_capacity;
        ChannelDataMap::channel_data_t total_larva;
        ChannelDataMap::channel_data_t egg_crowding_factor;
        ChannelDataMap::channel_data_t local_larval_mortality;
        ChannelDataMap::channel_data_t artificial_larval_mortality;
        ChannelDataMap::channel_data_t rainfall_larval_mortality;
        ChannelDataMap::channel_data_t local_larval_growth_mod;
    };
}
