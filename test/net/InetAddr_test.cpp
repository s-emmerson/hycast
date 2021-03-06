/**
 * Copyright 2017 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: InetAddr_test.cpp
 * @author: Steven R. Emmerson
 *
 * This file tests class `InetAddr`.
 */

#include "InetAddr.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <string.h>
#include <sys/socket.h>

namespace {

// The fixture for testing class InetAddr.
class InetAddrTest : public ::testing::Test {
protected:
    static const char* IPV4_ADDR1;
    static const char* IPV4_ADDR2;

    static const char* IPV6_ADDR1;
    static const char* IPV6_ADDR2;

    static const char* HOSTNAME1;
    static const char* HOSTNAME2;

    hycast::InetAddr ipv4Addr1;
    hycast::InetAddr ipv4Addr2;
    hycast::InetAddr ipv6Addr1;
    hycast::InetAddr ipv6Addr2;
    hycast::InetAddr nameAddr1;
    hycast::InetAddr nameAddr2;

    InetAddrTest()
        : ipv4Addr1{IPV4_ADDR1}
        , ipv4Addr2{IPV4_ADDR2}
        , ipv6Addr1{IPV6_ADDR1}
        , ipv6Addr2{IPV6_ADDR2}
        , nameAddr1{HOSTNAME1}
        , nameAddr2{HOSTNAME2}
    {}
};

const char* InetAddrTest::IPV4_ADDR1 = "128.117.140.56";
const char* InetAddrTest::IPV4_ADDR2 = "128.117.140.57";

const char* InetAddrTest::IPV6_ADDR1 = "2001:db8::ff00:42:8329";
const char* InetAddrTest::IPV6_ADDR2 = "2001:db8::ff00:42:8330";

const char* InetAddrTest::HOSTNAME1 = "idd.unidata.ucar.edu";
const char* InetAddrTest::HOSTNAME2 = "www.unidata.ucar.edu";

// Tests default construction
TEST_F(InetAddrTest, DefaultConstruction) {
    hycast::InetAddr addr1;
    hycast::InetAddr addr2;
}

// Tests construction from an IPv4 address string
TEST_F(InetAddrTest, ConstructionFromIPv4String) {
    hycast::InetAddr addr1{IPV4_ADDR1};
    EXPECT_STREQ(IPV4_ADDR1, addr1.to_string().data());
}

// Tests construction from an IPv6 address string
TEST_F(InetAddrTest, ConstructionFromIPv6String) {
    hycast::InetAddr addr2{IPV6_ADDR1};
    EXPECT_STREQ(IPV6_ADDR1, addr2.to_string().data());
}

// Tests construction from "lo"
TEST_F(InetAddrTest, ConstructionFromLo) {
    const std::string lo{"lo"};
    hycast::InetAddr addr1{lo};
    auto name = addr1.to_string();
    EXPECT_STREQ(lo.data(), name.data());
}

// Tests construction from "localhost"
TEST_F(InetAddrTest, ConstructionFromLocalhost) {
    const std::string localhost{"localhost"};
    hycast::InetAddr addr1{localhost};
    auto name = addr1.to_string();
    EXPECT_STREQ(localhost.data(), name.data());
}

// Tests construction from a hostname
TEST_F(InetAddrTest, ConstructionFromHostname) {
    hycast::InetAddr addr1{HOSTNAME1};
    EXPECT_STREQ(HOSTNAME1, addr1.to_string().data());
}

// Tests copy construction
TEST_F(InetAddrTest, CopyConstruction) {
    hycast::InetAddr addr1{IPV4_ADDR1};
    hycast::InetAddr addr2{addr1};
    EXPECT_STREQ(IPV4_ADDR1, addr2.to_string().data());

    hycast::InetAddr addr3{IPV6_ADDR1};
    hycast::InetAddr addr4{addr3};
    EXPECT_STREQ(IPV6_ADDR1, addr4.to_string().data());
}

// Tests copy assignment
TEST_F(InetAddrTest, CopyAssignment) {
    hycast::InetAddr addr1{IPV4_ADDR1};
    hycast::InetAddr addr2{IPV4_ADDR2};
    addr2 = addr1;
    EXPECT_STREQ(IPV4_ADDR1, addr2.to_string().data());

    hycast::InetAddr addr3{IPV6_ADDR1};
    hycast::InetAddr addr4{IPV6_ADDR2};
    addr4 = addr3;
    EXPECT_STREQ(IPV6_ADDR1, addr4.to_string().data());
}

// Tests hostname to socket address
TEST_F(InetAddrTest, HostnameToSocketAddress) {
    hycast::InetAddr nameAddr{"localhost"};
    struct sockaddr_storage storage;
    unsigned short port{38800};
    nameAddr.setSockAddrStorage(storage, port, SOCK_DGRAM);
    struct sockaddr_in* sockAddrIn =
            reinterpret_cast<struct sockaddr_in*>(&storage);
    EXPECT_EQ(AF_INET, sockAddrIn->sin_family);
    in_port_t p = htons(port);
    EXPECT_EQ(p, sockAddrIn->sin_port);
    EXPECT_EQ(inet_addr("127.0.0.1"), sockAddrIn->sin_addr.s_addr);
}

// Tests comparisons
TEST_F(InetAddrTest, Comparisons) {
    EXPECT_EQ(ipv4Addr1, ipv4Addr1);
    EXPECT_EQ(ipv6Addr1, ipv6Addr1);
    EXPECT_EQ(nameAddr1, nameAddr1);

    EXPECT_NE(ipv4Addr1, ipv4Addr2);
    EXPECT_NE(ipv6Addr1, ipv6Addr2);
    EXPECT_NE(nameAddr1, nameAddr2);

    EXPECT_NE(ipv4Addr1, ipv6Addr1);
    EXPECT_NE(ipv4Addr1, nameAddr1);
    EXPECT_NE(ipv6Addr1, ipv4Addr1);
    EXPECT_NE(ipv6Addr1, nameAddr1);
    EXPECT_NE(nameAddr1, ipv4Addr1);
    EXPECT_NE(nameAddr1, ipv6Addr1);

    EXPECT_LT(ipv4Addr1, ipv4Addr2);
    EXPECT_LT(ipv6Addr1, ipv6Addr2);
    EXPECT_LT(nameAddr1, nameAddr2);

    EXPECT_GT(ipv4Addr2, ipv4Addr1);
    EXPECT_GT(ipv6Addr2, ipv6Addr1);
    EXPECT_GT(nameAddr2, nameAddr1);

    EXPECT_LT(ipv4Addr1, ipv6Addr1);
    EXPECT_LT(ipv4Addr1, nameAddr1);
    EXPECT_LT(ipv6Addr1, nameAddr1);

    EXPECT_GT(ipv6Addr1, ipv4Addr1);
    EXPECT_GT(nameAddr1, ipv4Addr1);
    EXPECT_GT(nameAddr1, ipv6Addr1);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
