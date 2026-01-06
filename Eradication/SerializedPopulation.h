
#pragma once

#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>
#include "Simulation.h"
#include "ProgVersion.h"


namespace SerializedState {

    #define SERIAL_POP_VERSION  (6)

    enum Scheme
    {
        NONE = 0,
        SNAPPY = 1,
        LZ4 = 2,
    };
    
    struct Header
    {
        uint32_t version;
        std::string date;
        std::string sim_compression;
        size_t      sim_chunk_size;
        std::vector<std::string> node_compressions;
        std::vector<int32_t    > node_suids;
        std::vector<size_t     > node_chunk_sizes;
        std::vector<std::string> human_compressions;
        std::vector<int32_t    > human_node_suids;
        std::vector<int32_t    > human_num_humans;
        std::vector<size_t     > human_chunk_sizes;
        ProgDllVersion emod_info;

        Header();

        bool operator==( const Header& rThat ) const;
        bool operator!=( const Header& rThat ) const;

        std::string ToString() const;
        json::Object ToJson() const;
        void FromJson( const std::string& rJsonStr );
        void ValidateVersion( int versionInFile ) const;
    };


    Kernel::ISimulation* LoadSerializedSimulation(const char* filename);
    void SaveSerializedSimulation(Kernel::Simulation* sim, uint32_t time_step, bool compress, bool use_full_precision);


    void WriteData( Simulation* pSim, size_t bufferSize, const char* pBuffer, Scheme& rScheme, size_t& rCompressedDataSize, FILE* f );
    void WriteSimData( bool use_full_precision, Simulation* pSim, FILE* f, Header& rRealHeader );
    void WriteNodeData( bool use_full_precision, Simulation* pSim, INodeContext* pNode, FILE* f, Header& rRealHeader );
    void WriteHumanData( bool use_full_precision,
                         Simulation* pSim,
                         int32_t humanNodeSuid,
                         std::vector<IIndividualHuman*>& rHumanCollection,
                         FILE* f,
                         Header& rRealHeader );
    void WriteChunk( size_t compressDataSize,
                     const std::string& rCompressedData,
                     FILE* f );
    void CompressData( Scheme scheme,
                       size_t bufferSize,
                       const char* pBuffer,
                       size_t& rCompressedDataSize,
                       std::string& rCompressedData );
    void GenerateFilename( uint32_t time_step, string& filename_with_path );
    FILE* OpenFileForWriting( const string& filename );
    Kernel::NodeMap_t& GetNodes( Kernel::Simulation* sim );
    void PrepareSimulationData( Kernel::Simulation* sim,
                                std::vector<int32_t>& node_suids,
                                std::vector<int32_t>& human_collecttion_node_suids,
                                std::vector<std::vector<IIndividualHuman*>>& human_collections );
    void PopulateSizeHeader( std::vector<int32_t>& node_suids,
                             const std::vector<int32_t>& rHumanNodeSuids,
                             const std::vector<std::vector<IIndividualHuman*>>& rHumanCollections,
                             Header& rSizeHeader );
    void ConstructHeaderSize( const Header& header, string& size_string );
    void WriteMagicNumber( FILE* f );
    void WriteHeaderSize( const string& size_string, FILE* f );
    void WriteHeader( const Header& header, FILE* f );
    void WriteRealHeader( const std::string& rOrigSizeString,
                          const Header& rRealHeader,
                          FILE* f );

    FILE* OpenFileForReading( const char* filename );
    uint32_t ReadMagicNumber( FILE* f, const char* filename );
    void CheckMagicNumber( FILE* f, const char* filename );
    uint32_t ReadHeaderSize( FILE* f, const char* filename );
    void CheckHeaderSize( uint32_t header_size );
    void ReadHeader( FILE* f, const char* filename, Header& header );
    void ReadChunk( FILE* f, size_t byte_count, const char* filename, vector<char>& chunk );
    void JsonFromChunk( const vector<char>& chunk, const std::string& rCompressionSchemeStr, std::string& json );
    Simulation* ReadSimData( MemoryGauge& mem, FILE* f, const char* filename, const Header& rHeader );
    Node* ReadNodeData( MemoryGauge& mem,
                        FILE* f,
                        const char* filename,
                        const std::string rCompressionSchemeStr,
                        size_t nodeChunkSize );
    void ReadHumanData( MemoryGauge& mem, 
                        FILE* f,
                        const char* filename,
                        const std::string rCompressionSchemeStr,
                        size_t humanChunkSize,
                        std::vector<IIndividualHuman*>& rHumanCollection );
    Kernel::ISimulation* ReadDtkVersion6( MemoryGauge& mem, FILE* f, const char* filename, Header& header );


    // Declare this function so the friend declarations in Simulation.h will work:
    Kernel::NodeMap_t& GetNodes( Kernel::Simulation* sim );

    // Declare this function so the friend declarations in Node.h will work:
    void AddHumans( Kernel::Node* pNode,
                    const std::vector<Kernel::IIndividualHuman*>& rHumanCollection );
}
