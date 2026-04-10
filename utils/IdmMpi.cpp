
#include <stdafx.h>
#include <memory.h> //needed for memcpy on linux
#include "IdmMpi.h"

namespace IdmMpi
{
    Request::Request()
        : m_Data(MPI_REQUEST_NULL)
    {
    }
    
    Request::~Request()
    {
    }

    RequestList::RequestList()
        : m_DataList()
    {
    }

    RequestList::~RequestList()
    {
    }

    void RequestList::Add( const Request& request )
    {
        m_DataList.push_back( request.m_Data );
    }

    class MessageInterfaceBase : public MessageInterface
    {
    private:
        int m_NumTasks;
        int m_Rank;
    public:
        MessageInterfaceBase( int argc, char* argv[] )
            : MessageInterface()
            , m_NumTasks(1)
            , m_Rank(0)
        {
            // this could take &argc, argv
            MPI_Init( nullptr, nullptr );
            MPI_Comm_rank( MPI_COMM_WORLD, &m_Rank );
            MPI_Comm_size( MPI_COMM_WORLD, &m_NumTasks );
        }

        virtual ~MessageInterfaceBase()
        {
        }

        virtual int GetRank() const
        {
            return m_Rank;
        }

        virtual int GetNumTasks() const
        {
            return m_NumTasks;
        }

        virtual void Abort( int errorcode )
        {
            MPI_Abort( MPI_COMM_WORLD, errorcode );
        }

        virtual void Barrier()
        {
            MPI_Barrier( MPI_COMM_WORLD );
        }

        virtual void ReceiveIntegers( int* pBuf, int count, int fromRank )
        {
            MPI_Status status;
            MPI_Recv( pBuf, count, MPI_UNSIGNED, fromRank, 0, MPI_COMM_WORLD, &status);
        }

        virtual void ReceiveChars( char* pBuf, int count, int fromRank )
        {
            MPI_Status status;
            MPI_Recv( pBuf, count, MPI_BYTE, fromRank, 0, MPI_COMM_WORLD, &status);
        }

        virtual void SendIntegers( const uint32_t* pBuf, int count, int toRank, Request* pRequest )
        {
            MPI_Request* p_mpi_request = &(pRequest->m_Data);
            MPI_Isend( (void*)pBuf, count, MPI_UNSIGNED, toRank, 0, MPI_COMM_WORLD, p_mpi_request ); 
        }

        virtual void SendChars( const char* pBuf, int count, int toRank, Request* pRequest )
        {
            MPI_Request* p_mpi_request = &(pRequest->m_Data);
            MPI_Isend( (void*)pBuf, count, MPI_BYTE, toRank, 0, MPI_COMM_WORLD, p_mpi_request ); 
        }

        virtual void Reduce( float* pSendBuff, float* pReceiveBuff, int size )
        {
            MPI_Reduce((void*)pSendBuff, (void*)pReceiveBuff, size, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
        }

        virtual void GatherToRoot( const std::string& rToSend, std::string& rReceived )
        {
            int32_t length = int32_t(rToSend.size());

            if( m_Rank > 0 )
            {
                MPI_Gather( (void*)&length, 1, MPI_INTEGER4, nullptr, m_NumTasks, MPI_INTEGER4, 0, MPI_COMM_WORLD );
                MPI_Gatherv( (void*)rToSend.data(), length, MPI_BYTE, nullptr, nullptr, nullptr, MPI_BYTE, 0, MPI_COMM_WORLD );
            }
            else
            {
                std::vector<int32_t> lengths(m_NumTasks);
                MPI_Gather((void*)&length, 1, MPI_INTEGER4, lengths.data(), 1, MPI_INTEGER4, 0, MPI_COMM_WORLD );
                int32_t total = 0;
                std::vector<int32_t> displs(m_NumTasks);
                for (size_t i = 0; i < m_NumTasks; ++i)
                {
                    displs[i] = total;
                    total += lengths[i];
                }
                std::vector<char> buffer(total);
                MPI_Gatherv((void*)rToSend.data(), length, MPI_BYTE, (void*)buffer.data(), lengths.data(), displs.data(), MPI_BYTE, 0, MPI_COMM_WORLD);
            
                // ------------------------------------------------------------------------------------
                // --- Make sure to avoid string methods that look for the string to be null terminated.
                // --- If the string has binary data in it, then you won't get all of it.
                // ------------------------------------------------------------------------------------
                rReceived.assign( buffer.begin(), buffer.end() );
            }
        }

        virtual void Sync( const std::vector<int>& rSendInts, std::vector<int>& rReceiveInts )
        {
            int32_t count = (int32_t)rSendInts.size();

            std::vector<int32_t> lengths(m_NumTasks);
            MPI_Allgather( (void*)&count, 1, MPI_INTEGER4, lengths.data(), 1, MPI_INTEGER4, MPI_COMM_WORLD );

            int32_t total = 0;
            std::vector<int32_t> displs(m_NumTasks);
            for (size_t i = 0; i < m_NumTasks; ++i)
            {
                displs[i] = total;
                total += lengths[i];
            }
            rReceiveInts.resize( total );

            MPI_Allgatherv( (void*)rSendInts.data(), count, MPI_INTEGER4, (void*)rReceiveInts.data(), lengths.data(), displs.data(), MPI_INTEGER4, MPI_COMM_WORLD );
        }

        virtual void BroadcastInteger( int* buffer, int size, int rank )
        {
            MPI_Bcast( (void*)buffer, size, MPI_INTEGER4, rank, MPI_COMM_WORLD );
        }

        virtual void BroadcastChar( char* buffer, int size, int rank )
        {
            MPI_Bcast( (void*)buffer, size, MPI_BYTE, rank, MPI_COMM_WORLD );
        }

        virtual void PostChars( char* pBuf, int size, int myRank )
        {
            MPI_Bcast( (void*)&size,    1, MPI_INTEGER4, myRank, MPI_COMM_WORLD );
            MPI_Bcast( (void*)pBuf,  size, MPI_BYTE,     myRank, MPI_COMM_WORLD );
        }

        virtual void GetChars( std::vector<char>& rReceive, int fromRank )
        {
            uint32_t size = 0;
            MPI_Bcast( (void*)&size, 1, MPI_INTEGER4, fromRank, MPI_COMM_WORLD );
            rReceive.resize( size );
            MPI_Bcast( (void*)rReceive.data(), size, MPI_BYTE, fromRank, MPI_COMM_WORLD );
        }

        virtual void WaitAll( RequestList& rRequestList )
        {
            std::vector<MPI_Status> status( rRequestList.m_DataList.size() );
            MPI_Waitall( static_cast<int>(rRequestList.m_DataList.size()), static_cast<MPI_Request*>(rRequestList.m_DataList.data()), static_cast<MPI_Status*>(status.data()) );
        }

        virtual void Finalize()
        {
            MPI_Finalize();
        }
    };

    class MessageInterfaceNull: public MessageInterface
    {
    public:
        MessageInterfaceNull()
            : MessageInterface()
        {
        }

        virtual ~MessageInterfaceNull()
        {
        }

        virtual int GetRank() const override
        {
            return 0;
        }

        virtual int GetNumTasks() const override
        {
            return 1;
        }

        virtual void Reduce( float* pSendBuff, float* pReceiveBuff, int size ) override
        {
            memcpy( pReceiveBuff, pSendBuff, size*sizeof(float) );
        }

        virtual void GatherToRoot( const std::string& rToSend, std::string& rReceived )
        {
            rReceived = rToSend;
        }

        virtual void Abort( int errorcode ) override {}
        virtual void Barrier() override {}
        virtual void ReceiveIntegers( int* pBuf, int count, int fromRank ) override {}
        virtual void ReceiveChars( char* pBuf, int count, int fromRank ) override {}
        virtual void SendIntegers( const uint32_t* pBuf, int count, int toRank, Request* pRequest ) override {}
        virtual void SendChars( const char* pBuf, int count, int toRank, Request* pRequest ) override {}
        virtual void Sync( const std::vector<int>& rSendInts, std::vector<int>& rReceiveInts ) override {}
        virtual void BroadcastInteger( int* buffer, int size, int rank ) override {}
        virtual void BroadcastChar( char* buffer, int size, int rank ) override {}
        virtual void PostChars( char* pBuf, int size, int myRank ) override {}
        virtual void GetChars( std::vector<char>& rReceive, int fromRank ) override {}
        virtual void WaitAll( RequestList& rRequestList ) override {}
        virtual void Finalize() override {}
    };

    MessageInterface* MessageInterface::Create( int argc, char* argv[] )
    {
        return new MessageInterfaceBase( argc, argv );
    }

    MessageInterface* MessageInterface::CreateNull()
    {
        return new MessageInterfaceNull();
    }
}
