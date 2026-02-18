
#pragma once 

#include <cstdint>
#include <string>
#include <vector>
#include "IdmApi.h"

namespace IdmMpi
{
    class Request
    {
    public:
        friend class MessageInterfaceBase;
        friend class RequestList;

        Request();
        ~Request();
    private:
        int m_Data;
    };

    class RequestList
    {
    public:
        friend class MessageInterfaceBase;

        RequestList();
        ~RequestList();

        void Add( const Request& request );
    private:
        std::vector<int> m_DataList;
    };

    class IDMAPI MessageInterface
    {
    public:
        static MessageInterface* Create( int argc, char* argv[] );
        static MessageInterface* CreateNull();

        virtual int GetRank() const = 0;
        virtual int GetNumTasks() const = 0;
        virtual void Abort( int errorcode ) = 0;
        virtual void Barrier() = 0;
        virtual void ReceiveIntegers( int* pBuf, int count, int fromRank ) = 0;
        virtual void ReceiveChars( char* pBuf, int count, int fromRank ) = 0;
        virtual void SendIntegers( const uint32_t* pBuf, int count, int toRank, Request* pRequest ) = 0;
        virtual void SendChars( const char* pBuf, int count, int toRank, Request* pRequest ) = 0;
        virtual void Reduce( float* pSendBuff, float* pReceiveBuff, int size ) = 0;
        virtual void GatherToRoot( const std::string& rSend, std::string& rReceive ) = 0;
        virtual void Sync( const std::vector<int>& rSendInts, std::vector<int>& rReceiveInts ) = 0;
        virtual void BroadcastInteger( int* buffer, int size, int rank ) = 0;
        virtual void BroadcastChar( char* buffer, int size, int rank ) = 0;
        virtual void PostChars( char* pBuf, int size, int myRank ) = 0;
        virtual void GetChars( std::vector<char>& rReceive, int fromRank ) = 0;
        virtual void WaitAll( RequestList& rRequestList ) = 0;
        virtual void Finalize() = 0;

        virtual ~MessageInterface()
        {
        }

    protected:
        MessageInterface()
        {
        }
    };
}
