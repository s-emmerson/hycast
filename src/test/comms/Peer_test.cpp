/**
 * This file tests the class `Peer`.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Peer_test.cpp
 * @author: Steven R. Emmerson
 */

#include <comms/MsgRcvr.h>
#include <comms/Peer.h>
#include "ChunkInfo.h"
#include "ClntSctpSock.h"
#include "HycastTypes.h"
#include "InetSockAddr.h"
#include "ProdInfo.h"
#include "SrvrSctpSock.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <mutex>
#include <ratio>
#include <thread>

namespace {

static hycast::InetSockAddr serverSockAddr;

// The fixture for testing class Peer.
class PeerTest : public ::testing::Test {
friend class TestPeerMgr;
protected:
    // You can remove any or all of the following functions if its body
    // is empty.

    PeerTest() {
    }

    virtual ~PeerTest() {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    virtual void SetUp() {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    virtual void TearDown() {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    class TestMsgRcvr final : public hycast::MsgRcvr {
        PeerTest* peerTest;
    public:
        TestMsgRcvr(PeerTest& peerTest)
            : peerTest{&peerTest} {}
        void recvNotice(const hycast::ProdInfo& info, hycast::Peer& peer) {
            EXPECT_TRUE(peerTest->prodInfo == info);
        }
        void recvNotice(const hycast::ChunkInfo& info, hycast::Peer& peer) {
            EXPECT_TRUE(peerTest->chunkInfo == info);
        }
        void recvRequest(const hycast::ProdIndex& index, hycast::Peer& peer) {
            EXPECT_TRUE(peerTest->prodIndex == index);
        }
        void recvRequest(const hycast::ChunkInfo& info, hycast::Peer& peer) {
            EXPECT_TRUE(peerTest->chunkInfo == info);
        }
        void recvData(hycast::LatentChunk chunk, hycast::Peer& peer) {
            ASSERT_EQ(sizeof(peerTest->data), chunk.getSize());
            char data2[sizeof(peerTest->data)];
            chunk.drainData(data2);
            EXPECT_EQ(0, memcmp(peerTest->data, data2, sizeof(peerTest->data)));
        }
    };

    void runTestReceiver(const hycast::SrvrSctpSock& serverSock)
    {
        hycast::SctpSock sock{serverSock.accept()};
        TestMsgRcvr msgRcvr{*this};
        hycast::Peer peer{msgRcvr, sock};
        peer.runReceiver();
    }

    void runTestSender()
    {
        hycast::ClntSctpSock sock(serverSockAddr, hycast::Peer::getNumStreams());
        TestMsgRcvr msgRcvr{*this};
        hycast::Peer peer(msgRcvr, sock);
        peer.sendNotice(prodInfo);
        peer.sendNotice(chunkInfo);
        peer.sendRequest(prodIndex);
        peer.sendRequest(chunkInfo);
        hycast::ActualChunk actualChunk(chunkInfo, data, sizeof(data));
        peer.sendData(actualChunk);
    }

    void startTestReceiver()
    {
        // Server socket must exist before client connects
        hycast::SrvrSctpSock sock(serverSockAddr, hycast::Peer::getNumStreams());
        receiverThread = std::thread([=]{ this->runTestReceiver(sock); });
    }

    void startTestSender()
    {
        senderThread = std::thread(&PeerTest::runTestSender, this);
    }

    void runPerfReceiver(hycast::SrvrSctpSock serverSock)
    {
        hycast::SctpSock sock{serverSock.accept()};
        class PerfMsgRcvr final : public hycast::MsgRcvr {
        public:
            void recvNotice(const hycast::ProdInfo& info, hycast::Peer& peer) {}
            void recvNotice(const hycast::ChunkInfo& info, hycast::Peer& peer) {}
            void recvRequest(const hycast::ProdIndex& index, hycast::Peer& peer) {}
            void recvRequest(const hycast::ChunkInfo& info, hycast::Peer& peer) {}
            void recvData(hycast::LatentChunk chunk, hycast::Peer& peer) {
                chunk.discard();
            }
        };
        PerfMsgRcvr msgRcvr{};
        hycast::Peer peer{msgRcvr, sock};
        peer.runReceiver();
    }

    void runPerfSender()
    {
        hycast::ClntSctpSock sock(serverSockAddr, hycast::Peer::getNumStreams());
        TestMsgRcvr msgRcvr{*this};
        hycast::Peer peer(msgRcvr, sock);
        const size_t dataSize = 1000000;
        hycast::ChunkInfo chunkInfo(2, 3);
        for (hycast::ChunkSize chunkSize = hycast::chunkSizeMax - 8;
                chunkSize > 4000; chunkSize /= 2) {
            char data[chunkSize];
            std::chrono::high_resolution_clock::time_point start =
                    std::chrono::high_resolution_clock::now();
            size_t remaining = dataSize;
            while (remaining > 0) {
                size_t nbytes = chunkSize < remaining ? chunkSize : remaining;
                hycast::ActualChunk chunk(chunkInfo, data, nbytes);
                peer.sendData(chunk);
                remaining -= nbytes;
            }
            std::chrono::high_resolution_clock::time_point stop =
                    std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span =
                    std::chrono::duration_cast<std::chrono::duration<double>>
                    (stop - start);
            std::cerr << "Chunk size=" + std::to_string(chunkSize) +
                    " bytes, duration=" + std::to_string(time_span.count()) +
                    " s, byte rate=" + std::to_string(dataSize/time_span.count()) +
                    " Hz" << std::endl;
        }
    }

    void startPerfReceiver()
    {
        // Server socket must exist before client connects
        hycast::SrvrSctpSock sock(serverSockAddr, hycast::Peer::getNumStreams());
        receiverThread = std::thread(&PeerTest::runPerfReceiver, this, sock);
    }

    void startPerfSender()
    {
        senderThread = std::thread(&PeerTest::runPerfSender, this);
    }

    void waitReceiver()
    {
        receiverThread.join();
    }

    void waitSender()
    {
        senderThread.join();
    }

    // Objects declared here can be used by all tests in the test case for Peer.
    std::thread       senderThread;
    std::thread       receiverThread;
    hycast::ProdInfo  prodInfo{"product", 1, 100000, 1400};
    hycast::ChunkInfo chunkInfo{2, 3};
    hycast::ProdIndex prodIndex{2};
    char              data[2000] = {};
};

// Tests default construction
TEST_F(PeerTest, DefaultConstruction) {
    hycast::Peer peer{};
}

// Tests to_string
TEST_F(PeerTest, ToString) {
    EXPECT_STREQ("PeerImpl{sock=SocketImpl{sock=-1}, version=0}",
            hycast::Peer().to_string().data());
}

// Tests transmission
TEST_F(PeerTest, Transmission) {
    startTestReceiver();
    startTestSender();
    waitSender();
    waitReceiver();
}

#if 0
// Tests performance
TEST_F(PeerTest, Performance) {
    startPerfReceiver();
    startPerfSender();
    waitSender();
    waitReceiver();
}
#endif

}  // namespace

int main(int argc, char **argv) {
    const char* serverIpAddrStr = "127.0.0.1";
    if (argc > 1)
        serverIpAddrStr = argv[1];
    serverSockAddr = hycast::InetSockAddr{serverIpAddrStr, 38800};
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}