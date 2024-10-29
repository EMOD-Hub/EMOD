
#include "stdafx.h"

#include <numeric>

#include "ChannelDataMap.h"
#include "Log.h"
#include "RapidJsonImpl.h"
#include "Serializer.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "ProgVersion.h"
#include "Configuration.h"
#include "IdmMpi.h"
#include "Debug.h"

#include "JsonFullWriter.h"
#include "JsonFullReader.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "ChannelData" )

// ----------------------------------------------------------------------------
// --- ChannelID
// ----------------------------------------------------------------------------

ChannelID::ChannelID()
    : m_Index( -1 )
    , m_ChannelName()
{
}

ChannelID::ChannelID( int index, const std::string& channelName )
    : m_Index( index )
    , m_ChannelName( channelName )
{
}

const std::string& ChannelID::GetName() const
{
    release_assert( !m_ChannelName.empty() );
    return m_ChannelName;
}



// ----------------------------------------------------------------------------
// --- ChannelDataMap
// ----------------------------------------------------------------------------

ChannelDataMap::ChannelDataMap()
    : channel_data_map()
    , channel_index_to_map()
    , timesteps_reduced(0)
    , p_output_augmentor(nullptr)
    , start_year( -FLT_MAX )
    , stop_year( FLT_MAX )
{
}

ChannelDataMap::~ChannelDataMap()
{
}

void ChannelDataMap::ClearData()
{
    for (auto& entry : channel_data_map )
    {
        entry.second.clear();
    }

    timesteps_reduced = 0;
}

void ChannelDataMap::IncreaseChannelLength( int numElements )
{
    // elongate all storage vectors by one to make room for new timestep data
    for (auto& entry : channel_data_map )
    {
        entry.second.insert(entry.second.end(), numElements, 0.0f );
    }
}

void ChannelDataMap::IncreaseChannelLength( const std::string& channel_name, int numElements )
{
    for( int i = 0 ; i < numElements ; i++ )
    {
        channel_data_map[ channel_name ].push_back( 0.0f );
    }
}

bool ChannelDataMap::IsEmpty() const
{
    return channel_data_map.empty();
}

int ChannelDataMap::GetNumChannels() const
{
    return channel_data_map.size();
}

int ChannelDataMap::GetChannelLength() const
{
    if( channel_data_map.empty() )
        return 0 ;
    else
        return channel_data_map.begin()->second.size();
}

bool ChannelDataMap::HasChannel(std::string channel_name) const
{
    return channel_data_map.find(channel_name) != channel_data_map.end();
}

std::vector<std::string> ChannelDataMap::GetChannelNames() const
{
    std::vector<std::string> names ;

    for( auto channel : channel_data_map )
    {
        names.push_back( channel.first );
    }

    return names ;
}

const ChannelDataMap::channel_data_t& ChannelDataMap::GetChannel( const std::string& channel_name )
{
    return channel_data_map[ channel_name ] ;
}

ChannelID ChannelDataMap::AddChannel( const std::string& channel_name )
{
    channel_data_t data ;
    channel_data_map[ channel_name ] = data ;
    channel_data_t* p_data = &channel_data_map[ channel_name ];
    channel_index_to_map.push_back( p_data );

    return ChannelID( (channel_index_to_map.size()-1), channel_name );
}

void ChannelDataMap::RemoveChannel( const std::string& channel_name )
{
    channel_data_map.erase( channel_name );
}

void ChannelDataMap::SetChannelData( const std::string& channel_name, const ChannelDataMap::channel_data_t& rChannelData )
{
    channel_data_map[ channel_name ] = rChannelData ;
}

void ChannelDataMap::SetLastValue( const std::string& channel_name, ChannelDataMap::channel_data_element_t value )
{
    channel_data_map[ channel_name ].back() = value ;
}

void ChannelDataMap::Accumulate( const ChannelID& rID, channel_data_element_t value )
{
    release_assert( (0 <= rID.m_Index) && rID.m_Index < channel_index_to_map.size() );

    channel_data_t *data = channel_index_to_map[ rID.m_Index ];
    if (data->size() == 0) // initialize the vectors if this is the first time. assume we are on timestep 0
        data->push_back(value);
    else
    {
        data->back() += value;
    }
}

void ChannelDataMap::Accumulate( const std::string& channel_name, ChannelDataMap::channel_data_element_t value )
{
    channel_data_t *data = &channel_data_map[channel_name];
    if (data->size() == 0) // initialize the vectors if this is the first time. assume we are on timestep 0
        data->push_back(value);
    else
    {
        data->back() += value;
    }
}

void ChannelDataMap::Accumulate( const std::string& channel_name, int index, ChannelDataMap::channel_data_element_t value )
{
    channel_data_map[ channel_name ][ index ] += value ;
    //*(data.end() - num_total_bins + bin_index) += value;
}

void ChannelDataMap::ExponentialValues( const std::string& channel_name )
{
    channel_data_t& r_channel_data = channel_data_map[ channel_name ];
    for( float& r_data : r_channel_data )
    {
        // --------------------------------------------------------------------
        // --- We cast to a double and back to a float to ensure that we get 
        // --- consistent results.  If we just have "r_data = exp( r_data )",
        // --- the result is not as consistent as you'd like, especially when
        // --- comparing a full sim and one from a serialized population.
        // --------------------------------------------------------------------
        double val = r_data;
        val = exp( val );
        r_data = float( val );
    }
}

void ChannelDataMap::Reduce()
{
    LOG_DEBUG( "Reduce\n" );
    // reduce each element of each channel
    // will be very slow
    // keeps track of how many timesteps have been reduced and only reduces ones that havent been reduced yet

    // --------------------------------------------------
    // --- Synchronize the number of channels in the map
    // --------------------------------------------------
    std::map<std::string,int> send_name_size_map;
    for( auto& entry : channel_data_map )
    {
        send_name_size_map[ entry.first ] = entry.second.size();
    }

    Kernel::JsonFullWriter writer;
    Kernel::IArchive* par = static_cast<Kernel::IArchive*>(&writer);
    Kernel::IArchive& ar = *par;

    ar.labelElement("Channels") & send_name_size_map;

    const char* buffer = ar.GetBuffer();
    uint32_t buffer_size = ar.GetBufferSize();

    // send my channel names and number of channels to all other cores
    IdmMpi::RequestList outbound_requests;
    for( int to_rank = 0 ; to_rank < EnvPtr->MPI.NumTasks ; ++to_rank )
    {
        if( to_rank == EnvPtr->MPI.Rank ) continue;

        IdmMpi::Request size_request;
        EnvPtr->MPI.p_idm_mpi->SendIntegers( &buffer_size, 1, to_rank, &size_request );
        outbound_requests.Add( size_request );

        if( buffer_size > 0 )
        {
            IdmMpi::Request buffer_request;
            EnvPtr->MPI.p_idm_mpi->SendChars( const_cast<char*>(buffer), buffer_size, to_rank, &buffer_request );
            outbound_requests.Add( buffer_request );
        }
    }

    // Get from the other cores their channel names and sizes and update my map
    for( int from_rank = 0 ; from_rank < EnvPtr->MPI.NumTasks ; ++from_rank )
    {
        if( from_rank == EnvPtr->MPI.Rank ) continue;

        std::vector<char> receive_json;

        int receive_size = 0;
        EnvPtr->MPI.p_idm_mpi->ReceiveIntegers( &receive_size, 1, from_rank );

        if (receive_size > 0)
        {
            receive_json.resize( receive_size + 1 );
            EnvPtr->MPI.p_idm_mpi->ReceiveChars( receive_json.data(), receive_size, from_rank );
            receive_json.push_back( '\0' );

            Kernel::JsonFullReader reader( receive_json.data() );
            Kernel::IArchive* reader_par = static_cast<Kernel::IArchive*>(&reader);
            Kernel::IArchive& reader_ar = *reader_par;

            std::map<std::string,int> get_name_size_map;
            reader_ar.labelElement("Channels") & get_name_size_map;

            for( auto& entry : get_name_size_map )
            {
                std::string name = entry.first;
                int         size = entry.second;

                if( channel_data_map.count( name ) == 0 )
                {
                    channel_data_t data(size);
                    channel_data_map[ name ] = data;
                }
                else if( channel_data_map[ name ].size() != size )
                {
                    std::stringstream ss;
                    ss << "Channel=" << name << "  from rank=" << from_rank << " had size=" << size << " while this rank=" << EnvPtr->MPI.Rank << " had size=" << channel_data_map[ name ].size() << "\n";
                    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }
    }

    // synchronize with other cores
    EnvPtr->MPI.p_idm_mpi->WaitAll( outbound_requests );

    // -----------------------------------------------
    // --- Reduce the data in each channel of the map
    // -----------------------------------------------
    channel_data_t receive_buffer;

    if (channel_data_map.size() == 0) return; // nothing to do if nothing has been recorded yet

    int total_timesteps_recorded = (int)((*(channel_data_map.begin())).second).size(); // number of timesteps recorded so far
    int timesteps_to_reduce = total_timesteps_recorded - timesteps_reduced;

    if (timesteps_reduced == total_timesteps_recorded)
    {
        return; // nothing to do if we've already reduced everything
    }

    for (auto& pair : channel_data_map)
    {
        channel_data_t *channel_data = &(pair.second);

        receive_buffer.resize(timesteps_to_reduce);

        // zero it
        memset(receive_buffer.data(), 0, sizeof(float) * receive_buffer.size());

        LOG_DEBUG_F("Reducing %d timesteps of channel %s\n", timesteps_to_reduce, pair.first.c_str());

        if ( EnvPtr->Log->CheckLogLevel(Logger::VALIDATION, _module) )
        {
            LOG_DEBUG_F( "timesteps_reduced   = %d\n", timesteps_reduced );
            LOG_DEBUG_F( "timesteps_to_reduce = %d\n", timesteps_to_reduce );
            std::ostringstream data;
            data << '[';
            for ( int i = 0; i < timesteps_to_reduce; ++i)
            {
                data << ' ' << (*channel_data)[timesteps_reduced + i];
            }
            data << " ]" << std::endl;
            LOG_DEBUG( data.str().c_str() );
        }

        EnvPtr->MPI.p_idm_mpi->Reduce( &((*channel_data)[timesteps_reduced]), &((receive_buffer)[0]), (int)timesteps_to_reduce );

        for (int k = timesteps_reduced; k < total_timesteps_recorded; k++)
        {
            (*channel_data)[k] = receive_buffer[ k - timesteps_reduced ];
        }

        // hack because channel_data is already a copy 
        channel_data_map[ pair.first ] = *channel_data; // pray that magic STL copy constructors make this work out
    }

    timesteps_reduced = total_timesteps_recorded;
}

void ChannelDataMap::SetStartStopYears( float start, float stop )
{
    release_assert( start < stop );
    start_year = start;
    stop_year = stop;
}

void ChannelDataMap::WriteOutput( 
    const std::string& filename, 
    std::map<std::string, std::string>& units_map )
{
    // Add some header stuff to InsetChart.json.
    // { "Header":
    //   { "DateTime" },
    //   { "DTK_Version" },
    //   { "Report_Version" },
    //   { "Timesteps" },      # of timestamps in data
    //   { "Channels" }        # of channels in data
    // }
    time_t now = time(0);
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

    ProgDllVersion pv;
    ostringstream dtk_ver;
    dtk_ver << pv.getVersion() << " " << pv.getSccsBranch() << " " << pv.getBuildDate();

    Kernel::JSerializer js;
    Kernel::IJsonObjectAdapter* pIJsonObj = Kernel::CreateJsonObjAdapter();
    pIJsonObj->CreateNewWriter();
    pIJsonObj->BeginObject();

    pIJsonObj->Insert("Header");
    pIJsonObj->BeginObject();
    pIJsonObj->Insert("DateTime",            now3.substr(0,now3.length()-1).c_str()); // have to remove trailing '\n'
    pIJsonObj->Insert("DTK_Version",         dtk_ver.str().c_str());
    pIJsonObj->Insert("Report_Type",         "InsetChart");
    pIJsonObj->Insert("Report_Version",      "3.2");
    pIJsonObj->Insert("Start_Time",          (*EnvPtr->Config)["Start_Time"         ].As<Number>() );
    if( start_year > -FLT_MAX )
    {
        pIJsonObj->Insert("Report_Start_Year", start_year );
        pIJsonObj->Insert("Report_Stop_Year",  stop_year  );
    }
    pIJsonObj->Insert("Simulation_Timestep", (*EnvPtr->Config)["Simulation_Timestep"].As<Number>() );
    unsigned int timesteps = 0;
    if( !channel_data_map.empty() )
    {
        timesteps = (double)((channel_data_map.begin()->second).size());
    }
    pIJsonObj->Insert("Timesteps", (int)timesteps);
    pIJsonObj->Insert("Channels", (int)channel_data_map.size()); // this is "Header":"Channels" metadata

    if( p_output_augmentor != nullptr )
    {
        p_output_augmentor->AddDataToHeader( pIJsonObj );
    }

    pIJsonObj->EndObject(); // end of "Header"

    LOG_DEBUG("Iterating over channel_data_map\n");
    pIJsonObj->Insert("Channels"); // this is the top-level "Channels" for arrays of time-series data
    pIJsonObj->BeginObject();
    for( auto& entry : channel_data_map )
    {
        std::string name = entry.first;
        pIJsonObj->Insert(name.c_str());
        pIJsonObj->BeginObject();
        pIJsonObj->Insert("Units", units_map[name].c_str());
        pIJsonObj->Insert("Data");
        pIJsonObj->BeginArray();
        for (auto val : entry.second)
        {
            if (std::isnan(val)) val = 0;   // Since NaN isn't part of the json standard, force all NaN values to zero
            if (std::isinf(val)) val = 0;   // Since INF isn't part of the json standard, force all INF values to zero
            pIJsonObj->Add(val);
        }
        pIJsonObj->EndArray();
        pIJsonObj->EndObject(); // end of channel by name
    }
    pIJsonObj->EndObject(); // end of "Channels"
    pIJsonObj->EndObject(); // end of entire report

    // Write output to file
    // GetFormattedOutput() could be used for a smaller but less human readable file
    LOG_DEBUG("Writing JSON output file\n");
    const char* buffer;
    js.GetFormattedOutput(pIJsonObj, buffer);

    ofstream inset_chart_json;
    FileSystem::OpenFileForWriting( inset_chart_json, FileSystem::Concat( EnvPtr->OutputPath, filename ).c_str() );

    inset_chart_json << buffer << endl;
    inset_chart_json.flush();
    inset_chart_json.close();

    pIJsonObj->FinishWriter();
    delete pIJsonObj ;
}


void ChannelDataMap::normalizeChannel(
    const std::string &channel_name,
    float normalization_value )
{
    auto iterator = channel_data_map.find(channel_name);

    if (iterator != channel_data_map.end())
    {
        channel_data_t& channel_data = (*iterator).second;

        if (normalization_value != 0)
        {
            for (auto& value : channel_data)
            {
                value /= normalization_value;
            }
        }
        else
        {
            LOG_DEBUG_F("Normalizing report-channel %s to 0!\n", channel_name.c_str());
            memset(channel_data.data(), 0, channel_data.size() * sizeof(float));
        }
    }
    else
    {
        LOG_DEBUG_F("Skipping normalization because channel not enabled - %s\n", channel_name.c_str());
    }
}

void ChannelDataMap::normalizeChannel(
    const std::string &channel_name,
    const std::string &normalization_channel_name )
{
    // N.B. Using std::map.find() instead of operator[] on channelDataMap or else this channel will be 
    //      inserted only on rank-0 of multi-core simulations resulting in an infinite wait on next Reduce()
    auto ci  = channel_data_map.find(channel_name);
    auto ci2 = channel_data_map.find(normalization_channel_name);

    if ((ci != channel_data_map.end()) && (ci2 != channel_data_map.end()))
    {
        channel_data_t& channel_data          = (*ci).second;
        channel_data_t& normalization_channel = (*ci2).second;

        if( normalization_channel.size() == channel_data.size() )
        {
            int timestep = 0;
            for (auto& value : channel_data)
            {
                float normalization = normalization_channel[timestep];
                if (normalization != 0.0f)
                {
                    value /= normalization;
                }
                else
                {
                    value = 0.0f;
                }
                timestep++;
            }
        }
        else
        {
            std::ostringstream ss ;
            ss << "The channel to be normalized (" << channel_name <<") and the normalizing channel (" << normalization_channel_name << ") must have the same length.  "
               << channel_name << "=" << channel_data.size() << ", " << normalization_channel_name << "=" << normalization_channel.size() ;
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }
    else
    {
        std::ostringstream ss ;
        ss << "There must be data for both channels if they are going to be normalized: " 
           << channel_name << "_Has_Data=" << (ci != channel_data_map.end()) << " and " 
           << normalization_channel_name << "_Has_Data=" << (ci2 != channel_data_map.end());
        throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }
}

float op_log(float value) { return (value > 0.0f) ? log10(value) : -10.0f; }

void ChannelDataMap::addDerivedLogScaleSummaryChannel( 
    const std::string& source_channel_name,
    const std::string& log_channel_name )
{
    LOG_DEBUG( "addDerivedLogScaleSummaryChannel\n" );
    channel_data_t& source_channel = channel_data_map[source_channel_name];
    if (source_channel.size() > 0)
    {
        channel_data_t log_channel(source_channel.size());
        std::transform(source_channel.begin(), source_channel.end(), log_channel.begin(), op_log);
        channel_data_map[log_channel_name] = log_channel;
    }
    else
    {
        LOG_WARN_F("Failed to add derived channel %s as %s is not enabled\n", log_channel_name.c_str(), source_channel_name.c_str());
    }
}

void ChannelDataMap::addDerivedCumulativeSummaryChannel(
    const std::string& source_channel_name,
    const std::string& cumulative_channel_name )
{
    LOG_DEBUG( "addDerivedCumulativeSummaryChannel\n" );
    channel_data_t& source_channel = channel_data_map[source_channel_name];
    if (source_channel.size() > 0)
    {
        channel_data_t cumulative_channel(source_channel.size());
        std::partial_sum(source_channel.begin(), source_channel.end(), cumulative_channel.begin());
        channel_data_map[cumulative_channel_name] = cumulative_channel;
    }
    else
    {
        LOG_WARN_F("Failed to add derived channel %s as %s is not enabled\n", cumulative_channel_name.c_str(), source_channel_name.c_str());
    }
}
