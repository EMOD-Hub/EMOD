
#include <memory>
#include "stdafx.h"
#include "MpiDataExchanger.h"
#include "BinaryArchiveWriter.h"
#include "BinaryArchiveReader.h"
#include "Environment.h"
#include "IdmDateTime.h"
#include "Sugar.h"
#include "Log.h"
#include "IdmMpi.h"
#include "NoCrtWarnings.h"

SETUP_LOGGING( "MpiDataExchanger" )

namespace Kernel
{
    MpiDataExchanger::MpiDataExchanger( const char* pName,
                                        WithSelfFunc withSelfFunc,
                                        SendToOthersFunc toOthersFunc,
                                        ReceiveFromOthersFunc fromOthersFunc,
                                        ClearDataFunc clearFunc )
        : m_pName( pName )
        , m_WithSelfFunc( withSelfFunc )
        , m_ToOthersFunc( toOthersFunc )
        , m_FromOthersFunc( fromOthersFunc )
        , m_ClearDataFunc( clearFunc )
    {
    }

    MpiDataExchanger::~MpiDataExchanger()
    {
    }

    void MpiDataExchanger::ExchangeData( IdmDateTime& currentTime )
    {
        std::vector< uint32_t > message_size_by_rank( EnvPtr->MPI.NumTasks );   // "buffers" for size of buffer messages
        IdmMpi::RequestList outbound_requests;     // requests for each outbound message
        std::vector< BinaryArchiveWriter* > outbound_messages;  // buffers for outbound messages

        for (int destination_rank = 0; destination_rank < EnvPtr->MPI.NumTasks; ++destination_rank)
        {
            if (destination_rank == EnvPtr->MPI.Rank)
            {
                m_WithSelfFunc( destination_rank );
            }
            else
            {
                auto binary_writer = new BinaryArchiveWriter();
                IArchive* writer = static_cast<IArchive*>(binary_writer);

                m_ToOthersFunc( writer, destination_rank );

                if( EnvPtr->Log->CheckLogLevel(Logger::VALIDATION, _module) )
                {
                    SaveData( int(currentTime.time), EnvPtr->MPI.Rank, destination_rank, "send", writer->GetBuffer(), writer->GetBufferSize() );
                }

                size_t buffer_size = message_size_by_rank[destination_rank] = writer->GetBufferSize();
                IdmMpi::Request size_request;
                EnvPtr->MPI.p_idm_mpi->SendIntegers( &message_size_by_rank[destination_rank], 1, destination_rank, &size_request );
                outbound_requests.Add( size_request );

                if (buffer_size > 0)
                {
                    const char* buffer = writer->GetBuffer();

                    IdmMpi::Request buffer_request;
                    EnvPtr->MPI.p_idm_mpi->SendChars( const_cast<char*>(buffer), buffer_size, destination_rank, &buffer_request );
                    outbound_requests.Add( buffer_request );

                    outbound_messages.push_back( binary_writer );
                }
            }

            m_ClearDataFunc( destination_rank );
        }

        for (int source_rank = 0; source_rank < EnvPtr->MPI.NumTasks; ++source_rank)
        {
            if (source_rank == EnvPtr->MPI.Rank) continue;  // We don't use MPI to send data to ourselves.

            int size = 0;
            EnvPtr->MPI.p_idm_mpi->ReceiveIntegers( &size, 1, source_rank );

            if (size > 0)
            {
                std::unique_ptr<char[]> buffer(new char[size]);
                EnvPtr->MPI.p_idm_mpi->ReceiveChars( buffer.get(), size, source_rank );

                if( EnvPtr->Log->CheckLogLevel(Logger::VALIDATION, _module) ) 
                {
                    SaveData( int(currentTime.time), source_rank, EnvPtr->MPI.Rank, "recv", buffer.get(), size );
                }

                auto binary_reader = std::make_shared<BinaryArchiveReader>(buffer.get(), size);
                IArchive* reader = static_cast<IArchive*>(binary_reader.get());

                if( reader->HasError() )
                {
                    SaveData( int(currentTime.time), source_rank, EnvPtr->MPI.Rank, "recv", buffer.get(), size );
                }

                m_FromOthersFunc( reader, source_rank );

                m_ClearDataFunc( source_rank );
            }
        }

        // Clean up from Sends
        EnvPtr->MPI.p_idm_mpi->WaitAll( outbound_requests );

        for (auto writer : outbound_messages)
        {
            delete writer;
        }
    }

    void MpiDataExchanger::SaveData( uint32_t time_step,
                                     uint32_t source, 
                                     uint32_t dest, 
                                     char* suffix, 
                                     const char* buffer, 
                                     size_t size )
    {
        char filename[256];
        sprintf_s(filename, 256, "%s\\%03d-%02d-%02d-%s-%s.json", EnvPtr->OutputPath.c_str(), time_step, source, dest, m_pName, suffix);
        FILE* f = nullptr;
        errno = 0;
        if ( fopen_s( &f, filename, "w" ) != 0 )
        {
            std::stringstream ss;
            ss << "Could not open for writing '"<< filename << "'";
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
        }
        fwrite(buffer, 1, size, f);
        fflush(f);
        fclose(f);
    }
}
