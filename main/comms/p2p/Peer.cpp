/**
 * This file implements a connection between peers.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: PeerConnectionImpl.cpp
 * @author: Steven R. Emmerson
 */

#include "Channel.h"
#include "Chunk.h"
#include "ChunkInfo.h"
#include "error.h"
#include "Peer.h"
#include "PeerMsgRcvr.h"
#include "ProdIndex.h"
#include "ProdInfo.h"
#include "SctpSock.h"
#include "VersionMsg.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <thread>
#include <utility>

namespace hycast {

class Peer::Impl final {
    typedef enum {
        VERSION_STREAM_ID = 0,
        PROD_NOTICE_STREAM_ID,
        CHUNK_NOTICE_STREAM_ID,
        PROD_REQ_STREAM_ID,
        CHUNK_REQ_STREAM_ID,
        CHUNK_STREAM_ID,
        NUM_STREAM_IDS
    }      SctpStreamId;
    unsigned                          version;
    Channel<VersionMsg>               versionChan;
    Channel<ProdInfo>                 prodNoticeChan;
    Channel<ChunkInfo>                chunkNoticeChan;
    Channel<ProdIndex>                prodReqChan;
    Channel<ChunkInfo>                chunkReqChan;
    Channel<ActualChunk,LatentChunk>  chunkChan;
    PeerMsgRcvr&                      msgRcvr;
    SctpSock                          sock;

    class : public PeerMsgRcvr
    {
        void recvNotice(const ProdInfo& info) {}
        void recvNotice(const ProdInfo& info, const Peer& peer) {}
        void recvNotice(const ChunkInfo& info, const Peer& peer) {}
        void recvRequest(const ProdIndex& index, const Peer& peer) {}
        void recvRequest(const ChunkInfo& info, const Peer& peer) {}
        void recvData(LatentChunk chunk) {}
        void recvData(LatentChunk chunk, const Peer& peer) {}
    }                      defaultMsgRcvr;

    /**
     * Constructs. Blocks connecting to remote server and exchanging protocol
     * version with remote peer.
     * @param[in,out] msgRcvr  Object to receive messages from the remote peer.
     * @param[in,out] sock     Socket
     * @throw LogicError       Unknown protocol version from remote peer
     * @see Impl(Peer*, PeerMsgRcvr&, const InetSockAddr&)
     */
    Impl(   PeerMsgRcvr&  msgRcvr,
            SctpSock&&    sock)
        : Impl(msgRcvr, std::ref<SctpSock>(sock))
    {}

    /**
     * Returns the protocol version of the remote peer.
     * @pre `sock.getStreamId() == VERSION_STREAM_ID`
     * @return Protocol version of the remote peer
     * @throws std::logic_error if precondition not met
     * @threadsafety Safe
     */
    unsigned getVersion()
    {
        if (sock.getStreamId() != VERSION_STREAM_ID)
            throw std::logic_error("Current message isn't a version message");
        return versionChan.recv().getVersion();
    }
     /**
      * Every peer implementation is unique.
      */
     Impl(const Impl& impl) =delete;
     Impl(const Impl&& impl) =delete;
     Impl& operator=(const Impl& impl) =delete;
     Impl& operator=(const Impl&& impl) =delete;

public:
    /**
     * Default constructs. Any attempt to use use the resulting instance will
     * throw an exception.
     */
    Impl()
        : version(0),
          versionChan(),
          prodNoticeChan(),
          chunkNoticeChan(),
          prodReqChan(),
          chunkReqChan(),
          chunkChan(),
          msgRcvr(defaultMsgRcvr),
          sock()
    {}

    /**
     * Constructs from the containing peer object, an object to receive messages
     * from the remote peer, and a socket. Blocks connecting to remote server
     * and exchanging protocol version with remote peer.
     * @param[in,out] msgRcvr  Object to receive messages from the remote peer.
     * @param[in,out] sock     Socket
     * @throw LogicError       Unknown protocol version from remote peer
     */
    Impl(   PeerMsgRcvr&   msgRcvr,
            SctpSock&      sock)
        : version(0),
          versionChan(sock, VERSION_STREAM_ID, version),
          prodNoticeChan(sock, PROD_NOTICE_STREAM_ID, version),
          chunkNoticeChan(sock, CHUNK_NOTICE_STREAM_ID, version),
          prodReqChan(sock, PROD_REQ_STREAM_ID, version),
          chunkReqChan(sock, CHUNK_REQ_STREAM_ID, version),
          chunkChan(sock, CHUNK_STREAM_ID, version),
          msgRcvr(msgRcvr),
          sock(sock)
    {
        VersionMsg msg(version);
        versionChan.send(msg);
        const unsigned vers = getVersion();
        if (vers != version)
            throw LogicError(__FILE__, __LINE__,
            		"Remote peer uses unsupported protocol version: " +
                    std::to_string(vers));
    }

    /**
     * Constructs. Blocks connecting to remote server and exchanging protocol
     * version with remote peer.
     * @param[in,out] msgRcvr   Object to receive messages from remote peer
     * @param[in]     peerAddr  Socket address of remote peer
     * @throw LogicError        Unknown protocol version from remote peer
     * @throw RuntimeError      Other error
     */
    Impl(   PeerMsgRcvr&        msgRcvr,
            const InetSockAddr& peerAddr)
        : Impl{msgRcvr, SctpSock{peerAddr, getNumStreams()}}
    {}

    /**
     * Returns the number of streams.
     */
    static int getNumStreams()
    {
        return NUM_STREAM_IDS;
    }

    /**
     * Returns the Internet socket address of the remote peer.
     * @return Internet socket address of remote peer
     */
    const InetSockAddr& getRemoteAddr()
    {
        return sock.getRemoteAddr();
    }

    /**
     * Runs the receiver. Objects are received from the socket and passed to the
     * appropriate peer manager methods. Doesn't return until either the socket
     * is closed by the remote peer or an exception is thrown. Intended to run
     * on its own thread.
     * @throws LogicError  Not all of a chunk's data was read
     * @exceptionsafety    Basic guarantee
     * @threadsafefy       Thread-compatible but not thread-safe
     */
    void runReceiver(const Peer& peer)
    {
        int entryCancelState;
        (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &entryCancelState);
        for (;;) {
            (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
            uint32_t size = sock.getSize(); // Blocks waiting for input
            (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
            if (size == 0)
                break;
            switch (sock.getStreamId()) {
                case PROD_NOTICE_STREAM_ID:
                    msgRcvr.recvNotice(prodNoticeChan.recv(), peer);
                    break;
                case CHUNK_NOTICE_STREAM_ID:
                    msgRcvr.recvNotice(chunkNoticeChan.recv(), peer);
                    break;
                case PROD_REQ_STREAM_ID:
                    msgRcvr.recvRequest(prodReqChan.recv(), peer);
                    break;
                case CHUNK_REQ_STREAM_ID:
                    msgRcvr.recvRequest(chunkReqChan.recv(), peer);
                    break;
                case CHUNK_STREAM_ID: {
                    /*
                     * For an unknown reason, the compiler complains if the
                     * `peer->recvData` parameter is a `LatentChunk&` and not a
                     * `LatentChunk`.  This is acceptable, however, because
                     * `LatentChunk` can be trivially copied. See
                     * `PeerMgr::recvData()`.
                     */
                    LatentChunk chunk = chunkChan.recv();
                    msgRcvr.recvData(chunk, peer);
                    if (chunk.hasData())
                        throw LogicError(__FILE__, __LINE__,
                                "Latent chunk-of-data still has data");
                    break;
                }
                default:
                    sock.discard();
            }
        }
        (void)pthread_setcancelstate(entryCancelState, nullptr);
    }

    /**
     * Sends information about a product to the remote peer.
     * @param[in] prodInfo  Product information
     * @throws std::system_error if an I/O error occurs
     * @exceptionsafety Basic
     * @threadsafety    Compatible but not safe
     */
    void sendProdInfo(const ProdInfo& prodInfo) const
    {
        prodNoticeChan.send(prodInfo);
    }

    /**
     * Sends information about a chunk-of-data to the remote peer.
     * @param[in] chunkInfo  Chunk information
     * @throws std::system_error if an I/O error occurred
     * @exceptionsafety  Basic
     * @threadsafety     Compatible but not safe
     */
    void sendChunkInfo(const ChunkInfo& chunkInfo)
    {
        chunkNoticeChan.send(chunkInfo);
    }

    /**
     * Sends a request for product information to the remote peer.
     * @param[in] prodIndex  Product-index
     * @throws std::system_error if an I/O error occurred
     * @exceptionsafety  Basic
     * @threadsafety     Compatible but not safe
     */
    void sendProdRequest(const ProdIndex& prodIndex)
    {
        prodReqChan.send(prodIndex);
    }

    /**
     * Sends a request for a chunk-of-data to the remote peer.
     * @param[in] info  Chunk specification
     * @throws std::system_error if an I/O error occurred
     * @exceptionsafety  Basic
     * @threadsafety     Compatible but not safe
     */
    void sendRequest(const ChunkInfo& info)
    {
        chunkReqChan.send(info);
    }

    /**
     * Sends a chunk-of-data to the remote peer.
     * @param[in] chunk  Chunk-of-data
     * @throws std::system_error if an I/O error occurred
     * @exceptionsafety  Basic
     * @threadsafety     Compatible but not safe
     */
    void sendData(ActualChunk& chunk)
    {
        chunkChan.send(chunk);
    }

    /**
     * Indicates if this instance equals another.
     * @param[in] that  Other instance
     * @return `true` iff this instance equals the other
     * @execptionsafety Nothrow
     * @threadsafety    Thread-safe
     */
    bool operator==(const Impl& that) const noexcept {
        return this == &that; // Every instance is unique
    }

    /**
     * Indicates if this instance doesn't equal another.
     * @param[in] that  Other instance
     * @return `true` iff this instance doesn't equal the other
     * @execptionsafety Nothrow
     * @threadsafety    Thread-safe
     */
    bool operator!=(const Impl& that) const noexcept {
        return this != &that; // Every instance is unique
    }

    /**
     * Indicates if this instance is less than another.
     * @param that  Other instance
     * @return `true` iff this instance is less that the other
     * @exceptionsafety Nothrow
     * @threadsafety    Safe
     */
    bool operator<(const Impl& that) const noexcept {
        return this < &that; // Every instance is unique
    }

    /**
     * Returns the hash code of this instance.
     * @return the hash code of this instance
     * @execptionsafety Nothrow
     * @threadsafety    Thread-safe
     */
    size_t hash() const noexcept {
        return std::hash<const Impl*>()(this); // Every instance is unique
    }

    /**
     * Returns the string representation of this instance.
     * @return the string representation of this instance
     * @exceptionsafety Strong
     * @threadsafety    Safe
     */
    std::string to_string() const
    {
        return std::string("Peer::Impl{sock=") + sock.to_string() + ", version=" +
                std::to_string(version) + "}";
    }
};

Peer::Peer()
    : pImpl(new Impl())
{}

Peer::Peer(
        PeerMsgRcvr& msgRcvr,
        SctpSock&    sock)
    : pImpl(new Impl(msgRcvr, sock))
{}

Peer::Peer(
        PeerMsgRcvr&        msgRcvr,
        const InetSockAddr& peerAddr)
    : pImpl(new Impl(msgRcvr, peerAddr))
{}

void Peer::runReceiver() const
{
    pImpl->runReceiver(*this);
}

void Peer::sendNotice(const ProdInfo& prodInfo) const
{
    pImpl->sendProdInfo(prodInfo);
}

void Peer::sendNotice(const ChunkInfo& chunkInfo) const
{
    pImpl->sendChunkInfo(chunkInfo);
}

void Peer::sendRequest(const ProdIndex& prodIndex) const
{
    pImpl->sendProdRequest(prodIndex);
}

void Peer::sendRequest(const ChunkInfo& info) const
{
    pImpl->sendRequest(info);
}

void Peer::sendData(ActualChunk& chunk) const
{
    pImpl->sendData(chunk);
}

bool Peer::operator ==(const Peer& that) const noexcept
{
    return *pImpl.get() == *that.pImpl.get();
}

bool Peer::operator !=(const Peer& that) const noexcept
{
    return *pImpl.get() != *that.pImpl.get();
}

bool Peer::operator <(const Peer& that) const noexcept
{
    return *pImpl.get() < *that.pImpl.get();
}

uint16_t Peer::getNumStreams()
{
    return Impl::getNumStreams();
}

const InetSockAddr& Peer::getRemoteAddr() const
{
    return pImpl->getRemoteAddr();
}

size_t Peer::hash() const noexcept
{
    return pImpl->hash();
}

std::string Peer::to_string() const
{
    return pImpl->to_string();
}

} // namespace
