
#pragma once

#include <list>
#include <map>
#include <string>
#include <fstream>
#include <set>

#include "BaseChannelReport.h"
#include "Log.h"
#include "Configure.h"

//////////////////////////////////////////////////////////////////////////////
/*
SpatialReport is similar to the regular Report but is designed for reporting
detailed spatial values.  Because spatial data can grow quite large, we don't
necessarily want to keep it all in memory, instead writing it incrementally to
disk after every n timesteps.  To optimize disk considerations, channel data
is all sent to rank 0 to be written.

Note: we assume that nodes are always processed in the same order for the
lifetime of the simulation.  It's possible that could potentially change in
the future, at which time, more care would need to be taken to ensure nodes
get written in the same order every time.
*/
//////////////////////////////////////////////////////////////////////////////

namespace Kernel {

template <class T>
inline std::set< std::string > getKeys(
    const std::map< std::string, T > myMap
    )
{
    std::set< std::string > returnSet;

    for( auto entry: myMap )
    {
        returnSet.insert( entry.first );
    }
    return returnSet;
}

class SpatialReport : public BaseChannelReport
{
public:
    static IReport* CreateReport();
    virtual ~SpatialReport() { }
    GET_SCHEMA_STATIC_WRAPPER(SpatialReport)
    IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    DECLARE_QUERY_INTERFACE()

    virtual void Initialize( unsigned int nrmSize );

    virtual void BeginTimestep();
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return true ; } ;
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual );
    virtual void LogNodeData( Kernel::INodeContext * pNC );
    virtual void EndTimestep( float currentTime, float dt );

    virtual void Finalize();

protected:
    SpatialReport();
    virtual bool Configure( const Configuration* config ) override;

    virtual const char* GetChannelsDependsOn() const;

    virtual void Accumulate( std::string channel_name, int nodeid, float value );

    virtual void addDerivedCumulativeSummaryChannel(std::string channel_name, std::string channel_cumulative_name);

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();

    virtual void WriteHeader( std::ofstream* file );
    virtual void WriteHeaderParameters( std::ofstream* file );
    virtual void InitializeFiles();
    virtual void WriteData( ChannelDataMap& rChannelDataMap );
    virtual void ClearData();


//#define IS_SOC_ENABLED(x) ( spatial_output_channels.find( x.name ) != spatial_output_channels.end() ? true : false )
    struct ChannelInfo
    {
        std::string name;
        std::string units;
        bool enabled;

        ChannelInfo() { }
        ChannelInfo(std::string _name, std::string _units) : name(_name), units(_units), enabled(false) { }
        //ChannelInfo(std::string _name, std::string _units) : name(_name), units(_units) { }
    };

    typedef map< string, ChannelInfo * > tChanInfoMap;

    virtual void populateChannelInfos(tChanInfoMap &channel_infos);
    virtual void shuffleNodeData();

    ChannelInfo air_temperature_info;
    ChannelInfo births_info;
    ChannelInfo campaign_cost_info;
    ChannelInfo disease_deaths_info;
    ChannelInfo human_infectious_reservoir_info;
    ChannelInfo infected_info;
    ChannelInfo land_temperature_info;
    ChannelInfo new_infections_info;
    ChannelInfo new_reported_infections_info;
    ChannelInfo population_info;
    ChannelInfo prevalence_info;
    ChannelInfo rainfall_info;
    ChannelInfo relative_humidity_info;

    // counters for LogIndividualData stuff
    float new_infections;
    float new_reported_infections;
    float disease_deaths;

    template<class T>
    struct vec_append
    {
        T operator() (const T &lhs, const T &rhs)
        {
            T ret (lhs);
            ret.insert(ret.end(), rhs.begin(), rhs.end());
            return ret;
        }
    };

    std::map<int, int> nodeid_index_map;
    bool has_shuffled_nodes;
    int total_timesteps;

    typedef std::map<std::string, std::ofstream *> channel_file_map_t;
    channel_file_map_t channel_file_map;
    jsonConfigurable::tFixedStringSet spatial_output_channels;
};
}
