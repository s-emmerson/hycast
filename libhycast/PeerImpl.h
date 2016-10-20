/**
 * This file declares a connection between peers.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: PeerConnection.h
 * @author: Steven R. Emmerson
 */

#ifndef PEERIMPL_H_
#define PEERIMPL_H_

#include "ChunkChannel.h"
#include "ChunkInfo.h"
#include "ProdIndex.h"
#include "ProdInfo.h"
#include "RegChannel.h"
#include "Socket.h"

#include <thread>
#include "PeerMgr.h"

namespace hycast {

class PeerImpl final {
    typedef enum {
        PROD_NOTICE_STREAM_ID = 0,
        CHUNK_NOTICE_STREAM_ID,
        PROD_REQ_STREAM_ID,
        CHUNK_REQ_STREAM_ID,
        CHUNK_STREAM_ID,
        NUM_STREAM_IDS
    }      SctpStreamId;
    RegChannel<ProdInfo>  prodNoticeChan;
    RegChannel<ChunkInfo> chunkNoticeChan;
    RegChannel<ProdIndex> prodReqChan;
    RegChannel<ChunkInfo> chunkReqChan;
    ChunkChannel          chunkChan;
    PeerMgr*                 peerMgr;
    Socket                sock;
    unsigned              version;
    std::thread           recvThread;

    /**
     * Receives objects and calls the appropriate methods of the associated
     * peer. Doesn't return until the destructor is called or an exception is
     * thrown.
     * @throws std::runtime_error if an invalid SCTP stream ID is encountered
     * @exceptionsafety Basic
     * @threadsafety    Compatible but not safe
     * @see ~PeerConnectionImpl()
     */
    void runReceiver();

public:
    /**
     * Constructs from a peer, a socket, and a protocol version. Immediately
     * starts receiving objects from the socket and passing them to the
     * appropriate peer methods.
     * @param[in,out] peer     Peer. Must exist for the duration of the
     *                         constructed instance.
     * @param[in,out] sock     Socket
     * @param[in]     version  Protocol version
     */
    PeerImpl(
            PeerMgr&    peer,
            Socket&  sock,
            unsigned version);
    /**
     * Destroys this instance. Cancels the receiving thread and joins it.
     */
    ~PeerImpl();
    /**
     * Sends information about a product to the remote peer.
     * @param[in] prodInfo  Product information
     */
    void sendProdInfo(const ProdInfo& prodInfo);
    /**
     * Sends information about a chunk-of-data to the remote peer.
     * @param[in] chunkInfo  Chunk information
     */
    void sendChunkInfo(const ChunkInfo& chunkInfo);
    /**
     * Sends a request for product information to the remote peer.
     * @param[in] prodIndex  Product-index
     */
    void sendProdRequest(const ProdIndex& prodIndex);
    /**
     * Sends a request for a chunk-of-data to the remote peer.
     * @param[in] info  Chunk specification
     */
    void sendRequest(const ChunkInfo& info);
    /**
     * Sends a chunk-of-data to the remote peer.
     * @param[in] chunk  Chunk-of-data
     */
    void sendData(const ActualChunk& chunk);
};

} // namespace

#endif /* PEERIMPL_H_ */
