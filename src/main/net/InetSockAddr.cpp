/**
 * This file defines an Internet socket address.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: InetSockAddr.cpp
 * @author: Steven R. Emmerson
 */

#include "InetAddr.h"
#include "InetSockAddr.h"
#include "PortNumber.h"

#include <arpa/inet.h>
#include <cstring>
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <system_error>

namespace hycast {

class InetSockAddrImpl final
{
    InetAddr  inetAddr;
    in_port_t port;  // In host byte-order
public:
    /**
     * Constructs from nothing. The resulting instance will have the default
     * Internet address and the port number will be 0.
     * @throws std::bad_alloc if required memory can't be allocated
     */
    InetSockAddrImpl()
        : inetAddr(),
          port(0)
    {}

    /**
     * Constructs from a string representation of an Internet address and a port
     * number.
     * @param[in] ipAddr  String representation of Internet address
     * @param[in] port    Port number in host byte-order
     * @throws std::bad_alloc if required memory can't be allocated
     * @throws std::invalid_argument if the string representation is invalid
     */
    InetSockAddrImpl(
            const std::string ipAddr,
            const in_port_t   port)
        : inetAddr(ipAddr),
          port(port)
    {}

    /**
     * Constructs from an IPV4 address and a port number.
     * @param[in] addr  IPv4 address
     * @param[in] port  Port number
     * @throws std::bad_alloc if required memory can't be allocated
     */
    InetSockAddrImpl(
            const in_addr_t  addr,
            const PortNumber port)
        : inetAddr{addr},
          port{port.get_host()}
    {}

    /**
     * Constructs from an IPV6 address and a port number.
     * @param[in] addr  IPv6 address
     * @param[in] port  Port number in host byte-order
     * @throws std::bad_alloc if required memory can't be allocated
     */
    InetSockAddrImpl(
            const struct in6_addr& addr,
            const in_port_t        port)
        : inetAddr(addr),
          port(port)
    {}

    /**
     * Constructs from an IPv4 socket address.
     * @param[in] addr  IPv4 socket address
     * @throws std::bad_alloc if required memory can't be allocated
     */
    InetSockAddrImpl(const struct sockaddr_in& addr)
        : inetAddr(addr.sin_addr.s_addr),
          port(ntohs(addr.sin_port))
    {}

    /**
     * Constructs from an IPv6 socket address.
     * @param[in] addr  IPv6 socket address
     * @throws std::bad_alloc if required memory can't be allocated
     */
    InetSockAddrImpl(const struct sockaddr_in6& sockaddr)
        : inetAddr(sockaddr.sin6_addr),
          port(ntohs(sockaddr.sin6_port))
    {}

    /**
     * Returns the string representation of the Internet socket address.
     * @return String representation of the Internet socket address
     * @throws std::bad_alloc if required memory can't be allocated
     * @exceptionsafety Strong
     */
    std::string to_string() const
    {
        std::string addr = inetAddr.to_string();
        return (addr.find(':') == std::string::npos)
                ? addr + ":" + std::to_string(port)
                : std::string("[") + addr + "]:" + std::to_string(port);
    }

    /**
     * Connects a socket to this instance's endpoint.
     * @param[in] sd  Socket descriptor
     * @throws std::system_error
     * @exceptionsafety Strong
     * @threadsafety    Safe
     */
    void connect(const int sd) const
    {
        auto set = inetAddr.getSockAddr(port).get();
        for (auto sockAddr : *set)
            if (::connect(sd, &sockAddr, sizeof(sockAddr)) == 0)
                return;
        throw std::system_error(errno, std::system_category(),
                "connect() failure: socket=" + std::to_string(sd) +
                ", addr=" + to_string());
    }

    /**
     * Binds this instance's endpoint to a socket.
     * @param[in] sd  Socket descriptor
     * @throws std::system_error
     * @exceptionsafety Strong
     * @threadsafety    Safe
     */
    void bind(int sd) const
    {
        auto set = inetAddr.getSockAddr(port).get();
        for (auto sockAddr : *set)
            if (::bind(sd, &sockAddr, sizeof(sockAddr)) == 0)
                return;
        throw std::system_error(errno, std::system_category(),
                "bind() failure: socket=" + std::to_string(sd) +
                ", addr=" + to_string());
    }

};

InetSockAddr::InetSockAddr()
    : pImpl{new InetSockAddrImpl()}
{}

InetSockAddr::InetSockAddr(
        const std::string ip_addr,
        const in_port_t   port)
    : pImpl(new InetSockAddrImpl(ip_addr, port))
{}

InetSockAddr::InetSockAddr(
        const in_addr_t  addr,
        const PortNumber port)
    : pImpl{new InetSockAddrImpl(addr, port)}
{}

InetSockAddr::InetSockAddr(
        const struct in6_addr& addr,
        const in_port_t        port)
    : pImpl{new InetSockAddrImpl(addr, port)}
{}

InetSockAddr::InetSockAddr(const struct sockaddr_in& addr)
    : pImpl{new InetSockAddrImpl(addr)}
{}

InetSockAddr::InetSockAddr(const struct sockaddr_in6& sockaddr)
    : pImpl{new InetSockAddrImpl(sockaddr)}
{}

InetSockAddr::InetSockAddr(const InetSockAddr& that) noexcept
    : pImpl(that.pImpl)
{}

InetSockAddr& InetSockAddr::operator =(const InetSockAddr& rhs) noexcept
{
    pImpl = rhs.pImpl; // InetSockAddrImpl is an immutable class
    return *this;
}

std::string InetSockAddr::to_string() const
{
    return pImpl->to_string();
}

void InetSockAddr::connect(int sd) const
{
    pImpl->connect(sd);
}

void InetSockAddr::bind(int sd) const
{
    pImpl->bind(sd);
}

} // namespace
