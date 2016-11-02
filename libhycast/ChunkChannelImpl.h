/**
 * This file declares an implementation of an I/O channel for exchanging
 * chunks-of-data.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: ChunkChannelImpl.h
 * @author: Steven R. Emmerson
 */

#ifndef CHUNK_CHANNEL_IMPL_H_
#define CHUNK_CHANNEL_IMPL_H_

#include "ChannelImpl.h"
#include "Chunk.h"
#include "Socket.h"

namespace hycast {

class ChunkChannelImpl final : public ChannelImpl {
public:
    /**
     * Constructs from nothing. Any attempt to use the resulting instance will
     * throw an exception.
     */
    ChunkChannelImpl() =default;
    /**
     * Constructs from an SCTP socket, SCTP stream identifier, and protocol
     * version.
     * @param[in] sock      SCTP socket
     * @param[in] streamId  SCTP stream ID
     * @param[in] version   Protocol version
     */
    ChunkChannelImpl(
            Socket&        sock,
            const unsigned streamId,
            const unsigned version);
    /**
     * Sends a chunk-of-data.
     * @param[in] chunk  Chunk of data
     */
    void send(const ActualChunk& chunk);
    /**
     * Returns the chunk-of-data in the current message.
     * @return the chunk-of-data in the current message
     */
    LatentChunk recv();
};

} // namespace

#endif
