/**
 * This file implements a receiver of multicast messages.
 *
 * Copyright 2017 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: McastReceiver.cpp
 * @author: Steven R. Emmerson
 */

#include <mcast/McastReceiver.h>
#include <mcast/McastSender.h>
#include "config.h"

#include "Codec.h"
#include "error.h"
#include "UdpSock.h"

namespace hycast {

class McastReceiver::Impl final
{
    class Dec : public Decoder
    {
        McastUdpSock  sock;
    protected:
        size_t read(
                const struct iovec* iov,
                const int           iovcnt,
                const bool          peek)
        {
            return sock.recv(iov, iovcnt, peek);
        }
        void discard()
        {
            sock.discard();
        }
    public:
        explicit Dec(const InetSockAddr& mcastAddr)
            : Decoder{UdpSock::maxPayload}
            , sock{McastUdpSock(mcastAddr)}
        {}
        Dec(    const InetSockAddr& mcastAddr,
                const InetAddr&     srcAddr)
            : Decoder{UdpSock::maxPayload}
            , sock{McastUdpSock(mcastAddr, srcAddr)}
        {}
        bool hasRecord()
        {
            return sock.hasRecord();
        }
        size_t getSize()
        {
            return sock.getSize();
        }
    };

    Dec            decoder;
    McastContentRcvr*  msgRcvr;
    const unsigned version;

public:
    Impl(   const InetSockAddr& mcastAddr,
            McastContentRcvr&       msgRcvr,
            const unsigned      version)
        : decoder(mcastAddr)
        , msgRcvr(&msgRcvr)
        , version{version}
    {}

    Impl(   const InetSockAddr& mcastAddr,
            const InetAddr&     srcAddr,
            McastContentRcvr&       msgRcvr,
            const unsigned      version)
        : decoder{mcastAddr, srcAddr}
        , msgRcvr{&msgRcvr}
        , version{version}
    {}

    void operator ()()
    {
        for (;;) {
            // Keep consistent with McastSender::send(Product)
            McastSender::MsgIdType msgId;
            decoder.fill(sizeof(msgId));
            decoder.decode(msgId);
            switch (msgId) {
                /*
                 * NB: In all the following, the input message *must* be
                 * completely consumed; otherwise, its tail might be read in the
                 * next iteration.
                 */
                case McastSender::prodInfoMsgId: {
                    decoder.fill(0);
                    auto prodInfo = ProdInfo::deserialize(decoder, version);
                    msgRcvr->receive(prodInfo);
                    break;
                }
                case McastSender::chunkMsgId: {
                    decoder.fill(LatentChunk::getMetadataSize(version));
                    auto chunk = LatentChunk::deserialize(decoder, version);
                    msgRcvr->receive(chunk);
                    if (chunk.hasData())
                        throw LOGIC_ERROR("Latent chunk-of-data not drained");
                    break;
                }
                default:
                    throw RUNTIME_ERROR("Invalid message type: " +
                            std::to_string(msgId));
            }
            decoder.clear();
        }
    }
};

McastReceiver::McastReceiver(
        const InetSockAddr& mcastAddr,
        McastContentRcvr&       msgRcvr,
        const unsigned      version)
    : pImpl{new Impl(mcastAddr, msgRcvr, version)}
{}

McastReceiver::McastReceiver(
        const InetSockAddr& mcastAddr,
        const InetAddr&     srcAddr,
        McastContentRcvr&       msgRcvr,
        const unsigned      version)
    : pImpl{new Impl(mcastAddr, srcAddr, msgRcvr, version)}
{}

void McastReceiver::operator ()()
{
    pImpl->operator()();
}

} // namespace
