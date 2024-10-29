
#include "stdafx.h"
#include "BinnedReport.h"

#include <map>
#include <string>

#include "Debug.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "IIndividualHuman.h"
#include "ProgVersion.h"
#include "Serializer.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "BinnedReport" )

static const std::string _report_name = "BinnedReport.json";

static const char* _axis_labels[] = { "Age" };

namespace Kernel {

const char * BinnedReport::_pop_label = "Population";
const char * BinnedReport::_infected_label = "Infected";
const char * BinnedReport::_new_infections_label = "New Infections";

Kernel::IReport*
BinnedReport::CreateReport()
{
    return new BinnedReport();
}

BinnedReport::BinnedReport()
: BinnedReport( _report_name )
{
}

// Derived constructor calls base constructor to initialized reduced timesteps etc. 
BinnedReport::BinnedReport( const std::string& rReportName )
    : BaseChannelReport( rReportName )
    , num_timesteps(0)
    , num_axes(0)
    , axis_labels()
    , num_bins_per_axis()
    , num_total_bins(0)
    , values_per_axis()
//    , friendly_names_per_axis()
    , population_bins(nullptr)
    , infected_bins(nullptr)
    , new_infections_bins(nullptr)
    , p_output_augmentor(nullptr)
    , _age_bin_upper_values(nullptr)
{
    LOG_DEBUG( "BinnedReport ctor\n" );

    // These __ variables exist for super-easy intialization/specification by humans and don't persist past the ctor.
    // We don't want these as static consts outside the class, but ultimately as members so that each sim type can define their own age boundaries.
    
    float __age_bin_upper_values[] = { 1825.0,  3650.0,  5475.0,  7300.0,  9125.0, 10950.0, 12775.0, 14600.0, 16425.0, 18250.0, 20075.0, 21900.0, 23725.0, 25550.0, 27375.0, 29200.0, 31025.0, 32850.0, 34675.0, 36500.0, 999999.0 };
    char * __age_bin_friendly_names[] = { "<5",   "5-9", "10-14", "15-19", "20-24", "25-29", "30-34", "35-39", "40-44", "45-49", "50-54", "55-59", "60-64", "65-69", "70-74", "75-79", "80-84", "85-89", "90-94", "95-99", ">100" };
    _num_age_bins = sizeof( __age_bin_upper_values )/sizeof(float); 

    // Now let's actually initialize the single underscore vector variables we're going to use (the "tedious" way)
    // NOTE: 100 picked as "hopefully we won't need any bigger than this"

    _age_bin_friendly_names.resize( _num_age_bins );
    _age_bin_upper_values = new float[100];
    memset( _age_bin_upper_values, 0, sizeof( float ) * 100 );
    // It can be fun to use 1-line STL initializers, but sometimes readability is more important
    for( int idx = 0; idx < _num_age_bins ; idx++ )
    {
        _age_bin_upper_values[idx] = __age_bin_upper_values[idx];
        _age_bin_friendly_names[idx] = __age_bin_friendly_names[idx];
    }
    
}

BinnedReport::~BinnedReport()
{
    delete[] population_bins;
    delete[] infected_bins;
    delete[] new_infections_bins;
    //p_output_augmentor - do not delete since this class does not own it
}

#ifndef WIN32
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#endif
void BinnedReport::Initialize( unsigned int nrmSize )
{
    for( unsigned int idx=0; idx<sizeof(_num_bins_per_axis)/sizeof(int); idx++ )
    {
        _num_bins_per_axis[idx] = _num_age_bins;
    }
    
    num_timesteps = 0;
    _nrmSize = nrmSize;
    release_assert( _nrmSize );

    // wish we could just use C++11 initializer lists here, but alas... not yet implemented :(
    axis_labels = std::vector<std::string>(_axis_labels, _axis_labels + (sizeof(_axis_labels) / sizeof(char*)));
    num_bins_per_axis = std::vector<int>(_num_bins_per_axis, _num_bins_per_axis + (sizeof(_num_bins_per_axis) / sizeof(int)));

    num_axes = _countof(_axis_labels);

    num_total_bins = 1;
    for (int i : num_bins_per_axis)
        num_total_bins *= i;

    values_per_axis.resize( num_axes );
    for( int axis_idx=0; axis_idx < num_axes; axis_idx++ )
    {
        for( int idx=0; idx< num_bins_per_axis[axis_idx]; idx++ )
        {
            values_per_axis[axis_idx].push_back( _age_bin_upper_values[idx] );
        }
    }


    initChannelBins();
}

void BinnedReport::initChannelBins()
{
    LOG_DEBUG_F( "num_total_bins = %d\n", num_total_bins );
    population_bins     = new float[num_total_bins];
    infected_bins       = new float[num_total_bins];
    new_infections_bins = new float[num_total_bins];
    
    clearChannelsBins();
}

void BinnedReport::clearChannelsBins()
{
    memset(population_bins    , 0, num_total_bins * sizeof(float));
    memset(infected_bins      , 0, num_total_bins * sizeof(float));
    memset(new_infections_bins, 0, num_total_bins * sizeof(float));
}

void BinnedReport::BeginTimestep()
{
    channelDataMap.IncreaseChannelLength( num_total_bins );

    num_timesteps++;
}

void BinnedReport::EndTimestep( float currentTime, float dt )
{
    Accumulate(_pop_label, population_bins);
    Accumulate(_infected_label, infected_bins);
    Accumulate(_new_infections_label, new_infections_bins);
    
    clearChannelsBins();
}

void BinnedReport::Accumulate(std::string channel_name, float bin_data[])
{
    for(int i = 0; i < num_total_bins; i++)
    {
        // NOTE: We have to call Accumulate() here even if there's nothing to accumulate, because 
        // we need to make sure the channels are added.  Could potentially check outside the for-loop
        // on timestep 0, and add a channel that isn't there...?
        // if(bin_data[i] > 0)
            Accumulate(channel_name, bin_data[i], i);
    }
}

void BinnedReport::Accumulate(std::string channel_name, float value, int bin_index)
{
    if ( bin_index < 0 || bin_index >= num_total_bins)
    {
        throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "bin_index", bin_index, num_total_bins );
    }

    // initialize the vectors if this is the first time. assume we are at t=0 and the map has no keys.
    if( !channelDataMap.HasChannel( channel_name ) )
    {
        channelDataMap.IncreaseChannelLength( channel_name, num_total_bins );
    }
    int length = channelDataMap.GetChannelLength() ;
    if( length == 0) 
    {
        channelDataMap.IncreaseChannelLength( num_total_bins );
        length = channelDataMap.GetChannelLength() ;
    }

    // accumulate value at last timestep for the appropriate bin_index
    int index = length - num_total_bins + bin_index ;
    channelDataMap.Accumulate( channel_name, index, value );
}

void BinnedReport::LogNodeData( Kernel::INodeContext * pNC )
{
    // Nothing in this function.  All the updates will be done at the individual level in LogIndividualData(IndividualHuman*)
    LOG_DEBUG( "LogNodeData.\n" );
}

int BinnedReport::calcBinIndex( Kernel::IIndividualHuman* individual)
{
    float age = float(individual->GetAge());
    //bool isFemale      = (individual->GetGender() == FEMALE);

    // Calculate bin
    int agebin = lower_bound( values_per_axis[0].begin(), values_per_axis[0].end(), age ) - values_per_axis[0].begin();
    //int bin_index = ( age_bin_upper_edges.size() * isFemale ) + agebin;
    int bin_index = agebin;

    release_assert( bin_index < num_total_bins );
    return bin_index;
}

void  BinnedReport::LogIndividualData( Kernel::IIndividualHuman* individual )
{
    LOG_DEBUG( "LogIndividualData\n" );

    float mc_weight = float(individual->GetMonteCarloWeight());

    int bin_index = calcBinIndex(individual);

    population_bins[bin_index] += mc_weight;

    if (individual->IsInfected())
    {
        infected_bins[bin_index] += mc_weight;
    }

    NewInfectionState::_enum nis = individual->GetNewInfectionState();

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewInfection)
    {
        new_infections_bins[bin_index] += mc_weight;
    }
}

void BinnedReport::Finalize()
{
    LOG_DEBUG( "Finalize\n" );

    postProcessAccumulatedData();

    std::map<std::string, std::string> units_map;
    populateSummaryDataUnitsMap(units_map);

    // Add some header stuff to InsetChart.json.
    // { "Header":
    //   { "DateTime" },
    //   { "DTK_Version" },
    //   { "Report_Version" },
    //   { "Timesteps" },      # of timestamps in data
    //   { "Channels" }        # of channels in data
    // }
    time_t now = time(nullptr);
#ifdef WIN32
    tm now2;
    localtime_s(&now2,&now);
    char timebuf[26];
    asctime_s(timebuf,26,&now2);
    std::string now3 = std::string(timebuf);
#else
    tm* now2 = localtime(&now);
    std::string now3 = std::string(asctime(now2));
#endif

    Kernel::JSerializer js;
    Kernel::IJsonObjectAdapter* pIJsonObj = Kernel::CreateJsonObjAdapter();
    pIJsonObj->CreateNewWriter();
    pIJsonObj->BeginObject();

    pIJsonObj->Insert("Header");
    pIJsonObj->BeginObject();
    pIJsonObj->Insert("DateTime", now3.substr(0,now3.length()-1).c_str()); // have to remove trailing '\n'
    ProgDllVersion pv;
    ostringstream dtk_ver;
    dtk_ver << pv.getVersion() << " " << pv.getSccsBranch() << " " << pv.getBuildDate();
    pIJsonObj->Insert("DTK_Version", dtk_ver.str().c_str());
    pIJsonObj->Insert("Report_Version", "2.1");
    int timesteps = 0;
    if( !channelDataMap.IsEmpty() && num_total_bins > 0 )
    {
        timesteps = int(double(channelDataMap.GetChannelLength()) / double(num_total_bins));
    }
    pIJsonObj->Insert("Timesteps", timesteps);

    if( p_output_augmentor != nullptr )
    {
        p_output_augmentor->AddDataToHeader( pIJsonObj );
    }

    pIJsonObj->Insert("Subchannel_Metadata");
    pIJsonObj->BeginObject();

    pIJsonObj->Insert("AxisLabels");
    pIJsonObj->BeginArray();
    js.JSerialize(axis_labels, pIJsonObj);
    pIJsonObj->EndArray();

    pIJsonObj->Insert("NumBinsPerAxis");
    pIJsonObj->BeginArray();
    js.JSerialize(num_bins_per_axis, pIJsonObj);
    pIJsonObj->EndArray();

    pIJsonObj->Insert("ValuesPerAxis");
    pIJsonObj->BeginArray();
    for (auto& values : values_per_axis)
    {
        pIJsonObj->BeginArray();
        js.JSerialize(values, pIJsonObj);
        pIJsonObj->EndArray();
    }
    pIJsonObj->EndArray();

    pIJsonObj->Insert("MeaningPerAxis");
    pIJsonObj->BeginArray();
    //for (auto& names : friendly_names_per_axis)
    {
        pIJsonObj->BeginArray();
        //js.JSerialize(names, pIJsonObj);
        js.JSerialize(_age_bin_friendly_names, pIJsonObj);
        pIJsonObj->EndArray();
    }
    pIJsonObj->EndArray();
    pIJsonObj->EndObject(); // end of "Subchannel_Metadata"

    pIJsonObj->Insert("Channels", int(channelDataMap.GetNumChannels())); // this is "Header":"Channels" metadata
    pIJsonObj->EndObject(); // end of "Header"

    LOG_DEBUG("Iterating over channelDataMap\n");
    std::vector<std::string> channel_names = channelDataMap.GetChannelNames();
    pIJsonObj->Insert("Channels"); // this is the top-level "Channels" for arrays of time-series data
    pIJsonObj->BeginObject();
    for( auto name : channel_names )
    {
        const ChannelDataMap::channel_data_t& channel_data = channelDataMap.GetChannel( name );

        pIJsonObj->Insert(name.c_str());
        pIJsonObj->BeginObject();
        pIJsonObj->Insert("Units", units_map[name].c_str());
        pIJsonObj->Insert("Data");
        formatChannelDataBins(pIJsonObj, channel_data.data(), num_bins_per_axis, 0, num_total_bins);
        pIJsonObj->EndObject(); // end of channel by name
    }
    pIJsonObj->EndObject(); // end of "Channels"
    pIJsonObj->EndObject(); // end of "BinnedReport"

    // Write output to file
    // GetPrettyFormattedOutput() can be used for nicer indentation but bigger filesize
    LOG_DEBUG("Writing JSON output file\n");
    char* buffer;
    js.GetPrettyFormattedOutput(pIJsonObj, buffer);

    ofstream binned_report_json;
    FileSystem::OpenFileForWriting( binned_report_json, FileSystem::Concat( EnvPtr->OutputPath, report_name ).c_str() );

    binned_report_json << buffer << endl;
    binned_report_json.close();

    pIJsonObj->FinishWriter();
    delete pIJsonObj ;
}

json::Element BinnedReport::formatChannelDataBins(const float data[], std::vector<int>& dims, int start_axis, int num_remaining_bins)
{
    json::Array arr;
    LOG_DEBUG("formatChannelDataBins\n");

    if(start_axis < dims.size())
    {
        int num_bins = num_remaining_bins / dims[start_axis];
        arr.Resize(dims[start_axis]);
        for(int i = 0; i < dims[start_axis]; i++)
        {
            arr[i] = formatChannelDataBins(data + (i * num_bins), dims, start_axis + 1, num_bins);
        }
    }
    else
    {
        arr.Resize(num_timesteps);
        for(int i = 0; i < num_timesteps; i++)
        {
            ChannelDataMap::channel_data_t::value_type val = data[i * num_total_bins];
            if (std::isnan(val)) val = 0;   // Since NaN isn't part of the json standard, force all NaN values to zero
            arr[i] = Number(val);
        }
    }

    return arr;
}

void BinnedReport::formatChannelDataBins(Kernel::IJsonObjectAdapter* pIJsonObj, const float data[], std::vector<int>& dims, int start_axis, int num_remaining_bins)
{
    LOG_DEBUG("formatChannelDataBins\n");

    if(start_axis < dims.size())
    {
        int num_bins = num_remaining_bins / dims[start_axis];
        pIJsonObj->BeginArray();
        for(int i = 0; i < dims[start_axis]; i++)
        {
            formatChannelDataBins(pIJsonObj, data + (i * num_bins), dims, start_axis + 1, num_bins);
        }
        pIJsonObj->EndArray();
    }
    else
    {
        pIJsonObj->BeginArray();
        for(int i = 0; i < num_timesteps; i++)
        {
            ChannelDataMap::channel_data_t::value_type val = data[i * num_total_bins];
            if (std::isnan(val)) val = 0;   // Since NaN isn't part of the json standard, force all NaN values to zero
            pIJsonObj->Add(val);
        }
        pIJsonObj->EndArray();
    }
}


void BinnedReport::populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) { }
void BinnedReport::postProcessAccumulatedData() { }
}
