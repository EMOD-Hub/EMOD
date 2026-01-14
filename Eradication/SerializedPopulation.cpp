
#include "stdafx.h"
#include "SerializedPopulation.h"

#include <ctime>
#include "Debug.h"  // release_assert
#include "Exceptions.h"
#include "FileSystem.h"
#include <iomanip>
#include "JsonObject.h"
#include "JsonFullReader.h"
#include "JsonFullWriter.h"
#include "Log.h"
#include <lz4.h>
#include "NoCrtWarnings.h"
#include <numeric>
#include "Node.h"
#include <snappy.h>
#include "Sugar.h"  // EnvPtr
#include "NodeEventContext.h"
#include "Memory.h"
#include "SerializationParameters.h"

// Version info for created file
#include "ProgVersion.h"

/********************************************************************************************************
Header versions

    Version 1 
    schema:
        { "metadata": {
            "version": 1,
            "date": "Day Mon day HH:MM:SS year",
            "compressed": true|false,
            "bytecount": #
        }

    Version 2|3 
    schema:
        { "metadata": {
            "version": 2|3,
            "date": "Day Mon day HH:MM:SS year",
            "compressed": true|false,
            "engine": NONE|LZ4|SNAPPY,
            "bytecount": #,
            "chunkcount": #,
            "chunksizes": [ #, #, #, ..., # ]
        }

    Version 4 
    schema:
        {
            "version": 4,
            "date": "Day Mon day HH:MM:SS year",
            "compression": NONE|LZ4|SNAPPY,
            "bytecount": #,
            "chunkcount": #,
            "chunksizes": [ #, #, #, ..., # ]
        }

    Version 5 
    Added emod_info to header. emod_info contains version information.
    schema:
        {
            "version": 5,
            "date": "Day Mon day HH:MM:SS year",
            "compression": NONE|LZ4|SNAPPY,
            "emod_info": {},
            "bytecount": #,
            "chunkcount": #,
            "chunksizes": [ #, #, #, ..., # ]
        }
    
    Version 6
    Changed layout so nodes and humans are separate.
    - Human lists have a maximum of 2000 (Serialization_Max_Humans_Per_Collection).
    - Each compression chunk has its own compression type.
    - Header has a fixed size based on the number of nodes and human groups.
    - This fixed size means that the size values have to be placed in strings.
    - Using HEX so that the number of characters in the header is constant
    so that we can overwrite it once we get all of the info.
    - Compression string is always three characters.
    schema:
        {
            "version": 6,
            "author": "IDM",
            "tool": "DTK",
            "date": "Day Mon day HH:MM:SS year",
            "emod_info": {},
            "sim_compression": "LZ4",
            "sim_chunk_size": "FFFFFFFF",
            "node_compressions": [ "NON", "LZ4", "SNA", ..., "SNA" ]
            "node_suids": [ "00000001", "00000002" ],
            "node_chunk_sizes": [ "FFFFFFFF", "FFFFFFFF", "FFFFFFFF", ..., "FFFFFFFF" ],
            "human_compressions": [      "NON",      "LZ4",      "SNA", ...,      "SNA" ]
            "human_node_suids":   [ "00000001", "00000002", "00000002", ..., "00000002" ],
            "human_num_humans":   [ "000000FF", "00000FFF", "00000FFF", ..., "0000000A" ],
            "human_chunk_sizes":  [ "FFFFFFFF", "FFFFFFFF", "FFFFFFFF", ..., "FFFFFFFF" ]
        }
        NOTE: I took an FPG simulation of JoshS where it created one node with 10K people.
        In version 5 (all humans in node chunk    ), Peak Working Set reached 17,331-MB.
        In version 6 (humans in max groups of 1000), Peak Working Set reached  5,417-MB.
*/

// Each type must be the same number of characters so that we can replace them
// but use the exact same number of chacters in the JSON.
static std::string COMPRESSION_STR_NONE   = "NON";
static std::string COMPRESSION_STR_LZ4    = "LZ4";
static std::string COMPRESSION_STR_SNAPPY = "SNA";

SETUP_LOGGING( "SerializedPopulation" )

#define FILE_HEADER_VERSION (6)

namespace SerializedState
{
    // ------------------------------------------------------------------------
    // --- Helper methods for Header
    // ------------------------------------------------------------------------

    std::string Uint64ToHexString( uint64_t value )
    {
        std::stringstream ss;
        ss << std::hex << std::setfill( '0' ) << std::setw( 16 ) << value;
        return ss.str();
    }

    uint64_t HexStringToUint64( const std::string& rHexString )
    {
        uint64_t num = 0;
        sscanf_s( rHexString.c_str(), "%llx", &num );
        return num;
    }

    void ValidateCompressionString( const char* pFor, size_t index, const std::string& compression_str )
    {
        if( (compression_str != COMPRESSION_STR_NONE) &&
            (compression_str != COMPRESSION_STR_LZ4) &&
            (compression_str != COMPRESSION_STR_SNAPPY) )
        {
            std::stringstream ss;
            ss << "INVALID COMPRESSION TYPE: The header's '" << pFor << "_compressions[" << index << "]' = '" << compression_str << "'\n";
            ss << "The value must be one of: " << COMPRESSION_STR_NONE << ", " << COMPRESSION_STR_LZ4 << ", or " << COMPRESSION_STR_SNAPPY;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );

        }
    }

    json::Array ConvertStringArrayToJson( const std::vector<std::string>& rStringArray )
    {
        json::Array json_array;
        for( size_t i = 0; i < rStringArray.size(); ++i )
        {
            json_array.Insert( json::String( rStringArray[ i ] ) );
        }
        return json_array;
    }

    template<typename T>
    json::Array ConvertIntgerArrayToJson( const std::vector<T>& rIntegerArray )
    {
        json::Array json_array;
        for( size_t i = 0; i < rIntegerArray.size(); ++i )
        {
            std::string str = Uint64ToHexString( uint64_t( rIntegerArray[ i ] ) );
            json_array.Insert( json::String( str ) );
        }

        return json_array;
    }

    std::vector<std::string> ConvertCompressionStringsJsonArray( IJsonObjectAdapter& rJsonObject,
                                                                 const char* pObjTypeStr,
                                                                 const char* pjsonKeyStr )
    {
        std::vector<std::string> compression_str_array;

        IJsonObjectAdapter* p_adapter = rJsonObject.GetJsonArray( pjsonKeyStr );
        IJsonObjectAdapter& r_adapter = *p_adapter;

        for( size_t i = 0; i < r_adapter.GetSize(); ++i )
        {
            auto& item = *(r_adapter[ Kernel::IndexType( i ) ] );

            std::string compression_str = std::string( item.AsString() );

            ValidateCompressionString( pObjTypeStr, i, compression_str );

            compression_str_array.push_back( compression_str );
        }
        delete p_adapter;

        return compression_str_array;
    }

    template<typename T>
    std::vector<T> ConvertIntegerJsonArray( IJsonObjectAdapter& rJsonObject,
                                            const char* pObjTypeStr,
                                            const char* pjsonKeyStr )
    {
        std::vector<T> integer_array;

        IJsonObjectAdapter* p_adapter = rJsonObject.GetJsonArray( pjsonKeyStr );
        IJsonObjectAdapter& r_adapter = *p_adapter;

        for( size_t i = 0; i < r_adapter.GetSize(); ++i )
        {
            auto& item = *(r_adapter[ Kernel::IndexType( i ) ]);

            std::string integer_str = std::string( item.AsString() );
            T integer_val = T( HexStringToUint64( integer_str ) );

            integer_array.push_back( integer_val );
        }
        delete p_adapter;

        return integer_array;
    }

    template<typename T>
    void CheckArrayLengths( const std::vector<std::string>& rCompressionArray,
                            const std::vector<T>& rIntegerArray,
                            const char* pCompressionArrayName,
                            const char* pIntegerArrayName )
    {
        if( rCompressionArray.size() != rIntegerArray.size() )
        {
            std::stringstream ss;
            ss << "INVALID HEADER: ";
            ss << pCompressionArrayName << ".size()[=" << rCompressionArray.size() << "]";
            ss << " != ";
            ss << pIntegerArrayName     << ".size()[=" << rIntegerArray.size()     << "]\n";
            ss << "They must have the same number of items.";
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    // ------------------------------------------------------------------------
    // --- Header
    // ------------------------------------------------------------------------

    Header::Header() :
        version( FILE_HEADER_VERSION ),
        date(),
        sim_compression(),
        sim_chunk_size( 0 ),
        node_compressions(),
        node_suids(),
        node_chunk_sizes(),
        human_compressions(),
        human_node_suids(),
        human_num_humans(),
        human_chunk_sizes(),
        emod_info()
    {
        std::time_t now = std::time( nullptr );
        struct tm gmt;
        gmtime_s( &gmt, &now );
        char asc_time[ 256 ];
        strftime( asc_time, sizeof( asc_time ), "%a %b %d %H:%M:%S %Y", &gmt );
        date = asc_time;
    }

    bool Header::operator==( const Header& rThat ) const
    {
        if( this->version != rThat.version ) return false;
        if( this->date    != rThat.date    ) return false;

        if( this->emod_info.toString() != rThat.emod_info.toString() ) return false;

        if( this->sim_compression != rThat.sim_compression ) return false;
        if( this->sim_chunk_size  != rThat.sim_chunk_size  ) return false;

        if( this->node_compressions.size() != rThat.node_compressions.size() ) return false;
        for( int i = 0; i < this->node_compressions.size(); ++i )
        {
            if( this->node_compressions[ i ] != rThat.node_compressions[ i ] ) return false;
        }

        if( this->node_suids.size() != rThat.node_suids.size() ) return false;
        for( int i = 0; i < this->node_suids.size(); ++i )
        {
            if( this->node_suids[ i ] != rThat.node_suids[ i ] ) return false;
        }

        if( this->node_chunk_sizes.size() != rThat.node_chunk_sizes.size() ) return false;
        for( int i = 0; i < this->node_chunk_sizes.size(); ++i )
        {
            if( this->node_chunk_sizes[ i ] != rThat.node_chunk_sizes[ i ] ) return false;
        }

        if( this->human_compressions.size() != rThat.human_compressions.size() ) return false;
        for( int i = 0; i < this->human_compressions.size(); ++i )
        {
            if( this->human_compressions[ i ] != rThat.human_compressions[ i ] ) return false;
        }

        if( this->human_node_suids.size() != rThat.human_node_suids.size() ) return false;
        for( int i = 0; i < this->human_node_suids.size(); ++i )
        {
            if( this->human_node_suids[ i ] != rThat.human_node_suids[ i ] ) return false;
        }

        if( this->human_num_humans.size() != rThat.human_num_humans.size() ) return false;
        for( int i = 0; i < this->human_num_humans.size(); ++i )
        {
            if( this->human_num_humans[ i ] != rThat.human_num_humans[ i ] ) return false;
        }

        if( this->human_chunk_sizes.size() != rThat.human_chunk_sizes.size() ) return false;
        for( int i = 0; i < this->human_chunk_sizes.size(); ++i )
        {
            if( this->human_chunk_sizes[ i ] != rThat.human_chunk_sizes[ i ] ) return false;
        }

        return true;
    }

    bool Header::operator!=( const Header& rThat ) const
    {
        return !operator==( rThat );
    }

    std::string Header::ToString() const
    {
        std::stringstream os;
        json::Writer::Write( ToJson(), os );
        return os.str();
    }

    json::Object Header::ToJson() const
    {
        release_assert( node_compressions.size()  == node_chunk_sizes.size()  );
        release_assert( human_compressions.size() == human_node_suids.size()  );
        release_assert( human_chunk_sizes.size()  == human_chunk_sizes.size() );

        json::Object header;
        header[ "version"   ] = json::Uint64( version );
        header[ "author"    ] = json::String( "IDM" );
        header[ "tool"      ] = json::String( "DTK" );
        header[ "date"      ] = json::String( date );
        header[ "emod_info" ] = json::Object( emod_info.toJson() );

        // ---------------------------
        // --- Add info about Sim Data
        // ---------------------------
        header[ "sim_compression" ] = json::String( sim_compression );
        header[ "sim_chunk_size"  ] = json::String( Uint64ToHexString( sim_chunk_size ) );

        // ----------------------------
        // --- Add info about Node Data
        // ----------------------------
        header[ "node_compressions" ] = ConvertStringArrayToJson(          node_compressions );
        header[ "node_suids"        ] = ConvertIntgerArrayToJson<int32_t>( node_suids        );
        header[ "node_chunk_sizes"  ] = ConvertIntgerArrayToJson<uint64_t>( node_chunk_sizes );

        // -----------------------------
        // --- Add info about Human Data
        // -----------------------------
        header[ "human_compressions" ] = ConvertStringArrayToJson(           human_compressions );
        header[ "human_node_suids"   ] = ConvertIntgerArrayToJson<int32_t >( human_node_suids   );
        header[ "human_num_humans"   ] = ConvertIntgerArrayToJson<int32_t >( human_num_humans   );
        header[ "human_chunk_sizes"  ] = ConvertIntgerArrayToJson<uint64_t>( human_chunk_sizes  );

        return header;
    }

    void Header::FromJson( const std::string& rJsonStr )
    {
        Kernel::IJsonObjectAdapter* p_file_adapter = Kernel::CreateJsonObjAdapter();
        p_file_adapter->Parse( rJsonStr.c_str() );
        IJsonObjectAdapter& file_info = *p_file_adapter;

        version = 0;
        version = file_info.GetUint( "version" );

        ValidateVersion( version );

        date = std::string( file_info.GetString( "date" ) );

        IJsonObjectAdapter* p_emod_info_adapter = file_info.GetJsonObject( "emod_info" );
        emod_info = ProgDllVersion( p_emod_info_adapter );

        // -----------------
        // --- read sim data
        // -----------------
        sim_compression = file_info.GetString( "sim_compression" );

        std::string sim_chunk_str = file_info.GetString( "sim_chunk_size" );
        sim_chunk_size = HexStringToUint64( sim_chunk_str );

        // ------------------
        // --- read node data
        // ------------------
        node_compressions = ConvertCompressionStringsJsonArray( file_info, "node", "node_compressions" );
        node_suids        = ConvertIntegerJsonArray<int32_t>(   file_info, "node", "node_suids"       );
        node_chunk_sizes  = ConvertIntegerJsonArray<size_t>(    file_info, "node", "node_chunk_sizes" );

        CheckArrayLengths<int32_t>( node_compressions, node_suids,       "node_compressions", "node_suids"       );
        CheckArrayLengths<size_t >( node_compressions, node_chunk_sizes, "node_compressions", "node_chunk_sizes" );

        // -------------------
        // --- Read human data
        // -------------------
        human_compressions = ConvertCompressionStringsJsonArray( file_info, "human", "human_compressions" );
        human_node_suids   = ConvertIntegerJsonArray<int32_t>(   file_info, "human", "human_node_suids"   );
        human_num_humans   = ConvertIntegerJsonArray<int32_t>(   file_info, "human", "human_num_humans"   );
        human_chunk_sizes  = ConvertIntegerJsonArray<size_t>(    file_info, "human", "human_chunk_sizes"  );

        CheckArrayLengths<int32_t>( human_compressions, human_node_suids,  "human_compressions", "human_node_suids"  );
        CheckArrayLengths<int32_t>( human_compressions, human_num_humans,  "human_compressions", "human_num_humans"  );
        CheckArrayLengths<size_t >( human_compressions, human_chunk_sizes, "human_compressions", "human_chunk_sizes" );

        delete p_emod_info_adapter;
        delete p_file_adapter;
    }

    void Header::ValidateVersion( int versionInFile ) const
    {
        if( versionInFile != FILE_HEADER_VERSION )
        {
            std::stringstream msg;
            msg << "INVALID VERSION: The header version in the file is " << versionInFile << "\n";
            msg << "This version of EMOD only supports header version " << FILE_HEADER_VERSION << ".";
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    // ----------------------
    // --- internal functions
    // ----------------------

    const std::string& GetSchemeStr( Scheme scheme )
    {
        switch( scheme )
        {
            case Scheme::NONE:
                return COMPRESSION_STR_NONE;

            case Scheme::LZ4:
                return COMPRESSION_STR_LZ4;

            case Scheme::SNAPPY:
                return COMPRESSION_STR_SNAPPY;

            default:
                std::stringstream ss;
                ss << "UNKNOWN COMPRESSION SCHEME: scheme = " << scheme;
                throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    Scheme GetScheme( const std::string& rCompressionSchemeStr )
    {
        if( rCompressionSchemeStr == COMPRESSION_STR_NONE )
        {
            return Scheme::NONE;
        }
        else if( rCompressionSchemeStr == COMPRESSION_STR_LZ4 )
        {
            return Scheme::LZ4;
        }
        else if( rCompressionSchemeStr == COMPRESSION_STR_SNAPPY )
        {
            return Scheme::SNAPPY;
        }
        else
        {
            std::stringstream ss;
            ss << "UNKNOWN COMRPESSION SCHEME STRING: value = " << rCompressionSchemeStr;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    Scheme GetCompressionScheme( size_t jsonSize )
    {
        // #define LZ4_MAX_INPUT_SIZE      (0x7E000000) // defined in lz4.h
#define SNAPPY_MAX_INPUT_SIZE   (0xFFFFFFFF)

        // default to LZ4
        Scheme scheme = Scheme::LZ4;

        if( jsonSize > SNAPPY_MAX_INPUT_SIZE )
        {
            // If bigger than snappy can handle, then no compression
            scheme = Scheme::NONE;
        }
        else if( jsonSize > LZ4_MAX_INPUT_SIZE )
        {
            // If the payload is too big for LZ4, then use snappy.
            scheme = Scheme::SNAPPY;
        }

        return scheme;
    }

    // ------------------------------------------------------------------------
    // --- Write Functions
    // ------------------------------------------------------------------------

    void SaveSerializedSimulation( Kernel::Simulation* sim, uint32_t time_step, bool compress, bool use_full_precision )
    {
        sim->CheckMemoryFailure( false );

        string filename;
        GenerateFilename( time_step, filename );
        LOG_INFO_F( "Writing state to '%s'\n", filename.c_str() );
        FILE* f = OpenFileForWriting( filename );

        try
        {
            // Create a Header object without the real values, but
            // enough so that we can get the size of the JSON.
            Header size_header;

            std::vector<int32_t> node_suids;
            std::vector<int32_t> human_node_suids;
            std::vector<std::vector<IIndividualHuman*>> human_collections;

            PrepareSimulationData( sim, node_suids, human_node_suids, human_collections );

            PopulateSizeHeader( node_suids, human_node_suids, human_collections, size_header );

            sim->CheckMemoryFailure( false );

            string size_string;
            ConstructHeaderSize( size_header, size_string );
            WriteMagicNumber( f );
            WriteHeaderSize( size_string, f );
            WriteHeader( size_header, f );

            Header real_header;
            WriteSimData( use_full_precision, sim, f, real_header );

            sim->CheckMemoryFailure( false );

            Kernel::NodeMap_t& node_map = GetNodes( sim );

            for( auto entry : node_map )
            {
                WriteNodeData( use_full_precision, sim, entry.second, f, real_header );
                sim->CheckMemoryFailure( false );
            }

            for( int i = 0; i < human_collections.size(); ++i )
            {
                WriteHumanData( use_full_precision, sim, human_node_suids[ i ], human_collections[ i ], f, real_header );
                sim->CheckMemoryFailure( false );
            }

            WriteRealHeader( size_string, real_header, f );

            fflush( f );
            fclose( f );
        }
        catch( ... )
        {
            fclose( f );
            ostringstream msg;
            msg << "Exception writing serialized population file '" << filename.c_str() << "'." << std::endl;
            LOG_ERR( msg.str().c_str() );

            // We do not know what went wrong or exactly what the exception is. We will just pass it along.
            throw;  // Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        sim->CheckMemoryFailure( false );
    }

    void WriteData( Simulation* pSim,
                    size_t bufferSize,
                    const char* pBuffer,
                    Scheme& rScheme,
                    size_t& rCompressedDataSize, FILE* f )
    {
        rScheme = GetCompressionScheme( bufferSize );

        std::string compressed_data;
        CompressData( rScheme, bufferSize, pBuffer, rCompressedDataSize, compressed_data );

        pSim->CheckMemoryFailure( false );

        WriteChunk( rCompressedDataSize, compressed_data, f );

        pSim->CheckMemoryFailure( false );
    }

    void WriteSimData( bool use_full_precision, Simulation* pSim, FILE* f, Header& rRealHeader )
    {
        Kernel::IArchive* writer = static_cast<Kernel::IArchive*>(new Kernel::JsonFullWriter( false, use_full_precision ));
        (*writer)& pSim;

        Scheme scheme = Scheme::NONE;
        size_t compressed_data_size = 0;
        WriteData( pSim, writer->GetBufferSize(), writer->GetBuffer(), scheme, compressed_data_size, f );

        rRealHeader.sim_compression = GetSchemeStr( scheme );
        rRealHeader.sim_chunk_size = compressed_data_size;

        delete writer;
    }

    void WriteNodeData( bool use_full_precision,
                        Simulation* pSim,
                        INodeContext* pNode,
                        FILE* f,
                        Header& rRealHeader )
    {
        Kernel::IArchive* writer = static_cast<Kernel::IArchive*>(new Kernel::JsonFullWriter( false, use_full_precision ));
        (*writer)& pNode;

        Scheme scheme = Scheme::NONE;
        size_t compressed_data_size = 0;
        WriteData( pSim, writer->GetBufferSize(), writer->GetBuffer(), scheme, compressed_data_size, f );

        rRealHeader.node_suids.push_back( pNode->GetSuid().data );
        rRealHeader.node_compressions.push_back( GetSchemeStr( scheme ) );
        rRealHeader.node_chunk_sizes.push_back( compressed_data_size );

        delete writer;
    }

    void WriteHumanData( bool use_full_precision,
                         Simulation* pSim,
                         int32_t humanNodeSuid,
                         std::vector<IIndividualHuman*>& rHumanCollection,
                         FILE* f,
                         Header& rRealHeader )
    {
        // -------------------------------------------------------------------
        // --- I thought we could just write the array, but it didn't work.
        // --- Things where happier if the main chunk was an object/dictionary.
        // -------------------------------------------------------------------
        Kernel::IArchive* writer = static_cast<Kernel::IArchive*>(new Kernel::JsonFullWriter( false, use_full_precision ));
        (*writer).startObject();
        (*writer).labelElement("human_collection") & rHumanCollection;
        (*writer).endObject();

        Scheme scheme = Scheme::NONE;
        size_t compressed_data_size = 0;
        WriteData( pSim, writer->GetBufferSize(), writer->GetBuffer(), scheme, compressed_data_size, f );

        rRealHeader.human_compressions.push_back( GetSchemeStr( scheme ) );
        rRealHeader.human_chunk_sizes.push_back( compressed_data_size );
        rRealHeader.human_node_suids.push_back( humanNodeSuid );
        rRealHeader.human_num_humans.push_back( rHumanCollection.size() );

        delete writer;
    }

    void WriteChunk( size_t compressDataSize,
                     const std::string& rCompressedData,
                     FILE* f )
    {
        fwrite( rCompressedData.c_str(), sizeof( char ), compressDataSize, f );
    }

    void CompressData( Scheme scheme,
                       size_t bufferSize,
                       const char* pBuffer,
                       size_t& rCompressedDataSize,
                       std::string& rCompressedData )
    {
        switch( scheme )
        {
            case NONE:
            {
                // No compression, just use the raw JSON text.
                rCompressedDataSize = bufferSize;
                rCompressedData.resize( rCompressedDataSize );
                char* buffer = &*(rCompressedData).begin();
                memcpy( buffer, pBuffer, bufferSize );
            }
            break;

            case LZ4:
            {
                size_t source_size = bufferSize;
                size_t required = LZ4_COMPRESSBOUND( source_size );
                rCompressedData.resize( required );

                char* buffer = &*(rCompressedData).begin();
                *reinterpret_cast<uint32_t*>(buffer) = uint32_t( source_size );
                char* payload = buffer + sizeof( uint32_t );
                size_t compressed_size = LZ4_compress_default( pBuffer, payload, int32_t( source_size ), int32_t( required ) );
                // Python binding for LZ4 writes the source_size first, account for that space
                size_t payload_size = compressed_size + sizeof( uint32_t );
                rCompressedData.resize( payload_size );
                rCompressedDataSize = payload_size;
            }
            break;

            case SNAPPY:
            {
                rCompressedDataSize = snappy::Compress( pBuffer, bufferSize, &rCompressedData );
            }
            break;

            default:
            {
                ostringstream msg;
                msg << "Unknown compression scheme chosen: " << (uint32_t)scheme << std::endl;
                throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    void GenerateFilename( uint32_t time_step, string& filename_with_path )
    {
        // Make sure length accounts for "state-#####-###.dtk" (19 chars)
        size_t length = strlen("state-#####-###.dtk") + 1;  // Don't forget the null terminator (for sprintf_s)!
        string filename;
        filename.resize(length);
        char* buffer = &*filename.begin();

        if( EnvPtr->MPI.NumTasks == 1 )
        {
            // Don't bother with rank suffix if only one task.
            sprintf_s(buffer, length, "state-%05d.dtk", time_step);
        }
        else
        {   // Add rank suffix when running multi-core.
            sprintf_s(buffer, length, "state-%05d-%03d.dtk", time_step, EnvPtr->MPI.Rank);
        }

        filename_with_path = FileSystem::Concat<string>(EnvPtr->OutputPath, filename);
    }

    FILE* OpenFileForWriting(const string& filename)
    {
        FILE* f = nullptr;
        errno = 0;
        bool opened = (fopen_s(&f, filename.c_str(), "wb") == 0);

        if( !opened )
        {
            std::stringstream ss;
            ss << "Received error '" << FileSystem::GetSystemErrorMessage() << "' while opening file for writing.";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
        }

        return f;
    }

    Kernel::NodeMap_t& GetNodes( Kernel::Simulation* sim )
    {
        return sim->nodes;
    }

    void PrepareSimulationData( Kernel::Simulation* sim,
                                std::vector<int32_t>& node_suids,
                                std::vector<int32_t>& human_collecttion_node_suids,
                                std::vector<std::vector<IIndividualHuman*>>& human_collections )
    {
        Kernel::NodeMap_t& node_map = GetNodes( sim );

        for( auto entry : node_map )
        {
            int32_t node_suid = entry.first.data;
            node_suids.push_back( node_suid );
            human_collecttion_node_suids.push_back( node_suid );
            human_collections.push_back( std::vector<IIndividualHuman*>() );

            INodeEventContext::individual_visit_function_t fn =
                [node_suid,
                &human_collecttion_node_suids,
                &human_collections](IIndividualHumanEventContext* ihec)
                {
                    std::vector<IIndividualHuman*>* p_humans = &(human_collections.back());
                    if( p_humans->size() >= SerializationParameters::GetInstance()->GetMaxHumansPerCollection() )
                    {
                        human_collecttion_node_suids.push_back( node_suid );
                        human_collections.push_back( std::vector<IIndividualHuman*>() );
                        p_humans = &(human_collections.back());
                    }
                    IIndividualHuman* p_human = const_cast<IIndividualHuman*>(ihec->GetIndividualHumanConst());
                    p_humans->push_back( p_human );
                };
            entry.second->GetEventContext()->VisitIndividuals( fn );
        }
    }

    void PopulateSizeHeader( std::vector<int32_t>& node_suids,
                             const std::vector<int32_t>& rHumanNodeSuids,
                             const std::vector<std::vector<IIndividualHuman*>>& rHumanCollections,
                             Header& rSizeHeader )
    {
        rSizeHeader.sim_compression = COMPRESSION_STR_NONE;
        rSizeHeader.sim_chunk_size = 0;

        for( int i = 0; i < node_suids.size(); ++i )
        {
            rSizeHeader.node_compressions.push_back( COMPRESSION_STR_NONE );
            rSizeHeader.node_suids.push_back( node_suids[ i ] );
            rSizeHeader.node_chunk_sizes.push_back( 0 );
        }

        release_assert( rHumanNodeSuids.size() == rHumanCollections.size() );
        for( int i = 0; i < rHumanCollections.size(); ++i )
        {
            rSizeHeader.human_compressions.push_back( COMPRESSION_STR_NONE );
            rSizeHeader.human_node_suids.push_back( rHumanNodeSuids[ i ] );
            rSizeHeader.human_num_humans.push_back( rHumanCollections[ i ].size() );
            rSizeHeader.human_chunk_sizes.push_back( 0 );
        }
    }

    void ConstructHeaderSize( const Header& header, string& size_string )
    {
        std::ostringstream temp;
        uint32_t header_size = header.ToString().size();
        temp << setw(12) << right << header_size;
        size_string = temp.str();
    }

    void WriteMagicNumber( FILE* f )
    {
        fwrite("IDTK", 1, 4, f);
    }

    void WriteHeaderSize( const string& size_string, FILE* f )
    {
        fwrite(size_string.c_str(), 1, size_string.length(), f);
    }

    void WriteHeader( const Header& header, FILE* f )
    {
        string header_string = header.ToString();
        fwrite(header_string.c_str(), 1, header_string.length(), f);
    }

    void WriteRealHeader( const std::string& rOrigSizeString,
                          const Header& rRealHeader,
                          FILE* f )
    {
        std::string size_string;
        ConstructHeaderSize( rRealHeader, size_string );
        release_assert( rOrigSizeString == size_string );

        int header_start_position = 4; // Magic Number
        header_start_position += size_string.length();

        fseek( f, header_start_position, SEEK_SET );
        string header_string = rRealHeader.ToString();

        fwrite( header_string.c_str(), 1, header_string.length(), f );
    }

    // ------------------------------------------------------------------------
    // --- Read Functions
    // ------------------------------------------------------------------------

    Kernel::ISimulation* LoadSerializedSimulation( const char* filename )
    {
        MemoryGauge mem;
        // read the config.json file so that you can set the warning and halt memory limits
        mem.Configure( EnvPtr->Config );
        mem.CheckMemoryFailure( false );

        Kernel::ISimulation* newsim = nullptr;

        FILE* f = OpenFileForReading(filename);

        try
        {
            CheckMagicNumber(f, filename);
            Header header;
            ReadHeader(f, filename, header);

            mem.CheckMemoryFailure( false );
            header.emod_info.checkSerializationVersion( filename );
            newsim = ReadDtkVersion6( mem, f, filename, header );
            mem.CheckMemoryFailure( false );

            fclose( f );
        }
        catch(SerializationException se)
        {
            fclose( f );
            ostringstream msg;
            msg << "An error while reading serialized population file, '" << filename << "' occured." << std::endl << std::endl;
            msg << se.GetMsg() << std::endl;
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        catch( ... )
        {
            fclose( f );
            ostringstream msg;
            msg << "Unexpected error reading serialized population file, '" << filename << "'." << std::endl;
            LOG_ERR( msg.str().c_str() );

            // We do not know what went wrong or exactly what the exception is. We will just pass it along.
            throw;  // Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
        }

        return newsim;
    }

    FILE* OpenFileForReading( const char* filename )
    {
        FILE* f = nullptr;
        errno = 0;
        bool opened = (fopen_s(&f, filename, "rb") == 0);

        if( !opened )
        {
            std::stringstream ss;
            ss << "Received error '" << FileSystem::GetSystemErrorMessage() << "' while opening file for reading.";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, ss.str().c_str() );
        }

        return f;
    }

    uint32_t ReadMagicNumber( FILE* f, const char* filename )
    {
        uint32_t magic = 0x0BADBEEF;
        size_t count = 1;
        size_t size = sizeof( magic );
        size_t bytes_read = fread(&magic, count, size, f);
        if (bytes_read != (count * size))
        {
            ostringstream msg;
            msg << " read " << bytes_read << " of " << (count * size) << " bytes for magic number";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }

        return magic;
    }

    void CheckMagicNumber( FILE* f, const char* filename )
    {
        uint32_t magic = ReadMagicNumber(f, filename);
        bool valid = (magic == *(uint32_t*)"IDTK");
        if( !valid )
        {
            ostringstream msg;
            msg << "Serialized population file has wrong magic number, expected 0x" << std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)"IDTK";
            msg << " 'IDTK', got 0x" << std::hex << std::setw(8) << std::setfill('0') << magic << '.' << std::endl;
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    uint32_t ReadHeaderSize( FILE* f, const char* filename )
    {
        char size_string[ 13 ];   // allocate space for null terminator
        size_t byte_count = sizeof( size_string ) - 1;
        size_t bytes_read = fread( size_string, 1, byte_count, f ); // don't read null terminator
        if( bytes_read != byte_count )
        {
            ostringstream msg;
            msg << " read " << bytes_read << " of " << byte_count << " bytes for header";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }
        size_string[byte_count] = '\0';
        errno = 0;
        uint32_t header_size = strtoul(size_string, nullptr, 10);
        if( errno == ERANGE )
        {
            ostringstream msg;
            msg << "Could not convert header size from text ('" << size_string << "') to unsigned integer." << std::endl;
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return header_size;
    }

    void CheckHeaderSize( uint32_t header_size )
    {
        if( header_size == 0 )
        {
            string msg( "Serialized population header size is 0, which is not valid.\n" );
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
        }
    }

    void ReadHeader( FILE* f, const char* filename, Header& header )
    {
        uint32_t count = ReadHeaderSize(f, filename);
        CheckHeaderSize(count);
        string header_text;
        header_text.resize(count);
        char* buffer = &*header_text.begin();
        size_t bytes_read = fread(buffer, 1, count, f);
        if( bytes_read != count )
        {
            ostringstream msg;
            msg << "read " << bytes_read << " of " << count << " bytes for header";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }
        header.FromJson( header_text );
    }

    void ReadChunk( FILE* f, size_t byte_count, const char* filename, vector<char>& chunk )
    {
        chunk.resize(byte_count);
        size_t bytes_read = fread(chunk.data(), 1, byte_count, f);
        if( bytes_read != byte_count )
        {
            ostringstream msg;
            msg << "read " << bytes_read << " of " << byte_count << " bytes for chunk";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }
    }

    namespace lz4
    {
        // source is a block of LZ4 compressed data with the size of the uncompressed data prepended
        size_t Uncompress( const char* block, size_t block_size, string& result )
        {
            size_t result_size = *reinterpret_cast<uint32_t*>(const_cast<char*>(block));
            const char* lz4_data = block + sizeof(uint32_t);
            result.resize(result_size);
            char* buffer = &*result.begin();
            int32_t data_size = int32_t(block_size - sizeof(uint32_t));
            int32_t count = LZ4_decompress_safe(lz4_data, buffer, data_size, result_size);

            return size_t(count);
        }
    }

    void JsonFromChunk( const vector<char>& chunk, const std::string& rCompressionSchemeStr, std::string& json )
    {
        if( rCompressionSchemeStr == COMPRESSION_STR_NONE )
        {
            json.resize( chunk.size() );
            char* dest = &*json.begin();
            memcpy( dest, chunk.data(), chunk.size() );
        }
        else if( rCompressionSchemeStr == COMPRESSION_STR_LZ4 )
        {
            lz4::Uncompress( chunk.data(), chunk.size(), json );
        }
        else if( rCompressionSchemeStr == COMPRESSION_STR_SNAPPY )
        {
            snappy::Uncompress( chunk.data(), chunk.size(), &json );
        }
        else
        {
            std::stringstream ss;
            ss << "UNKNOWN COMRPESSION SCHEME STRING: value = " << rCompressionSchemeStr;
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    Simulation* ReadSimData( MemoryGauge& mem, FILE* f, const char* filename, const Header& rHeader )
    {
        std::vector<char> chunk;
        ReadChunk( f, rHeader.sim_chunk_size, filename, chunk );

        mem.CheckMemoryFailure( false );

        std::string json;
        JsonFromChunk( chunk, rHeader.sim_compression, json );

        mem.CheckMemoryFailure( false );

        // Deserialize simulation
        ISimulation* p_new_sim = nullptr;

        Kernel::IArchive* reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
        (*reader) & p_new_sim;

        mem.CheckMemoryFailure( false );

        delete reader;

        mem.CheckMemoryFailure( false );

        Kernel::Simulation* simulation = static_cast<Kernel::Simulation*>(p_new_sim);
        return simulation;
    }

    Node* ReadNodeData( MemoryGauge& mem,
                        FILE* f,
                        const char* filename,
                        const std::string rCompressionSchemeStr,
                        size_t nodeChunkSize )
    {
        // Read node JSON
        std::vector<char> chunk;
        ReadChunk( f, nodeChunkSize, filename, chunk);

        mem.CheckMemoryFailure( false );

        std::string json;
        JsonFromChunk(chunk, rCompressionSchemeStr, json );

        mem.CheckMemoryFailure( false );

        // Deserialize node
        Kernel::IArchive* reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
        Kernel::ISerializable* obj;
        (*reader) & obj;

        mem.CheckMemoryFailure( false );

        delete reader;

        mem.CheckMemoryFailure( false );

        Kernel::Node* p_node = static_cast<Kernel::Node*>(obj);
        return p_node;
    }

    void ReadHumanData( MemoryGauge& mem, 
                        FILE* f,
                        const char* filename,
                        const std::string rCompressionSchemeStr,
                        size_t humanChunkSize,
                        std::vector<IIndividualHuman*>& rHumanCollection )
    {
        // Read human JSON
        std::vector<char> chunk;
        ReadChunk( f, humanChunkSize, filename, chunk);

        mem.CheckMemoryFailure( false );

        std::string json;
        JsonFromChunk(chunk, rCompressionSchemeStr, json );

        mem.CheckMemoryFailure( false );

        // Deserialize humans
        Kernel::IArchive* reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
        (*reader).startObject();
        (*reader).labelElement("human_collection") & rHumanCollection;
        (*reader).endObject();

        mem.CheckMemoryFailure( false );

        delete reader;

        mem.CheckMemoryFailure( false );
    }

    void AddHumans( Kernel::Node* pNode,
                    const std::vector<Kernel::IIndividualHuman*>& rHumanCollection )
    {
        pNode->individualHumans.insert( pNode->individualHumans.end(),
                                        rHumanCollection.begin(),
                                        rHumanCollection.end());
    }

    Kernel::ISimulation* ReadDtkVersion6( MemoryGauge& mem, FILE* f, const char* filename, Header& header)
    {
        Kernel::Simulation* p_newsim = ReadSimData( mem, f, filename, header );

        // For each node
        for( size_t i = 0; i < header.node_chunk_sizes.size(); ++i)
        {
            Kernel::Node* p_node = ReadNodeData( mem, f, filename, header.node_compressions[ i ], header.node_chunk_sizes[ i ] );

            GetNodes(p_newsim)[p_node->GetSuid()] = p_node;
        }

        std::vector<IIndividualHuman*> human_collection;
        human_collection.reserve( SerializationParameters::GetInstance()->GetMaxHumansPerCollection() );
        for( size_t i = 0; i < header.human_chunk_sizes.size(); ++i )
        {
            human_collection.clear();
            ReadHumanData( mem, f, filename, header.human_compressions[ i ], header.human_chunk_sizes[ i ], human_collection );

            Kernel::suids::suid node_suid;
            node_suid.data = header.human_node_suids[ i ];

            INodeContext* p_nc = GetNodes( p_newsim )[ node_suid ];
            Kernel::Node* p_node = static_cast<Kernel::Node*>(p_nc);

            AddHumans( p_node, human_collection );
        }

        return p_newsim;
    }

}   // namespace SerializedState
