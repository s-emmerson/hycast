/**
 * This file declares the implementation of an I/O channel for serializable
 * objects.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: RegChannelImpl.h
 * @author: Steven R. Emmerson
 */

#ifndef REGCHANNELIMPL_H_
#define REGCHANNELIMPL_H_

#include "ChannelImpl.h"
#include "Serializable.h"
#include "Socket.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace hycast {

template <class T>
class RegChannelImpl final : public ChannelImpl {
public:
    RegChannelImpl(
            Socket&            sock,
            const unsigned     streamId,
            const unsigned     version);
    /**
     * Sends a serializable object.
     * @param[in] obj  Serializable object.
     */
    void send(const Serializable& obj);
    /**
     * Returns the object in the current message.
     * @return the object in the current message
     */
    typename std::result_of<decltype(&T::deserialize)
            (const char*, size_t, unsigned)>::type recv();
};

} // namespace

#endif /* REGCHANNELIMPL_H_ */