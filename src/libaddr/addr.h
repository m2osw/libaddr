// Network Address -- classes functions to ease handling IP addresses
// Copyright (c) 2012-2018  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/libaddr
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
#pragma once

/** \file
 * \brief The various libaddr classes.
 *
 * This header includes the base addr class used to handle one binary
 * address.
 */

// C++ library
//
#include <memory>
#include <cstring>
#include <vector>

// C library
//
#include <arpa/inet.h>



namespace addr
{




/** \brief Initialize an IPv6 address as such.
 *
 * This function initializes a sockaddr_in6 with all zeroes except
 * for the sin6_family which is set to AF_INET6.
 *
 * return The initialized IPv6 address.
 */
constexpr struct sockaddr_in6 init_in6()
{
    struct sockaddr_in6 in6 = sockaddr_in6();
    in6.sin6_family = AF_INET6;
    return in6;
}


class addr
{
public:
    enum class network_type_t
    {
        NETWORK_TYPE_UNDEFINED,
        NETWORK_TYPE_PRIVATE,
        NETWORK_TYPE_CARRIER,
        NETWORK_TYPE_LINK_LOCAL,
        NETWORK_TYPE_MULTICAST,
        NETWORK_TYPE_LOOPBACK,
        NETWORK_TYPE_ANY,
        NETWORK_TYPE_UNKNOWN,
        NETWORK_TYPE_PUBLIC = NETWORK_TYPE_UNKNOWN  // we currently do not distinguish public and unknown
    };

    enum class computer_interface_address_t
    {
        COMPUTER_INTERFACE_ADDRESS_ERROR = -1,
        COMPUTER_INTERFACE_ADDRESS_FALSE,
        COMPUTER_INTERFACE_ADDRESS_TRUE
    };

    enum class string_ip_t
    {
        STRING_IP_ONLY,
        STRING_IP_BRACKETS,         // IPv6 only
        STRING_IP_PORT,             // in IPv6, includes brackets
        STRING_IP_MASK,
        STRING_IP_BRACKETS_MASK,    // IPv6 only
        STRING_IP_ALL
    };

    typedef std::shared_ptr<addr>   pointer_t;
    typedef std::vector<addr>       vector_t;
    typedef int                     socket_flag_t;

    static socket_flag_t const      SOCKET_FLAG_CLOEXEC  = 0x01;
    static socket_flag_t const      SOCKET_FLAG_NONBLOCK = 0x02;
    static socket_flag_t const      SOCKET_FLAG_REUSE    = 0x04;

                                    addr();
                                    addr(struct sockaddr_in const & in);
                                    addr(struct sockaddr_in6 const & in6);

    static vector_t                 get_local_addresses();

    void                            set_from_socket(int s, bool peer);
    void                            set_ipv4(struct sockaddr_in const & in);
    void                            set_ipv6(struct sockaddr_in6 const & in6);
    void                            set_port(int port);
    void                            set_protocol(char const * protocol);
    void                            set_protocol(int protocol);
    void                            set_mask(uint8_t const * mask);
    void                            apply_mask();

    bool                            is_ipv4() const;
    void                            get_ipv4(struct sockaddr_in & in) const;
    void                            get_ipv6(struct sockaddr_in6 & in6) const;
    std::string                     to_ipv4_string(string_ip_t mode) const;
    std::string                     to_ipv6_string(string_ip_t mode) const;
    std::string                     to_ipv4or6_string(string_ip_t mode) const;

    network_type_t                  get_network_type() const;
    std::string                     get_network_type_string() const;
    computer_interface_address_t    is_computer_interface_address() const;

    std::string                     get_iface_name() const;
    int                             create_socket(socket_flag_t flags) const;
    int                             connect(int s) const;
    int                             bind(int s) const;
    std::string                     get_name() const;
    std::string                     get_service() const;
    int                             get_port() const;
    int                             get_protocol() const;
    void                            get_mask(uint8_t * mask);

    bool                            match(addr const & ip) const;
    bool                            operator == (addr const & rhs) const;
    bool                            operator != (addr const & rhs) const;
    bool                            operator <  (addr const & rhs) const;
    bool                            operator <= (addr const & rhs) const;
    bool                            operator >  (addr const & rhs) const;
    bool                            operator >= (addr const & rhs) const;

private:
    void                            address_changed();

    // always keep address in an IPv6 structure
    //
    struct sockaddr_in6             f_address = init_in6();
    uint8_t                         f_mask[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
    std::string                     f_iface_name;
    int                             f_protocol = IPPROTO_TCP;
    mutable network_type_t          f_private_network_defined = network_type_t::NETWORK_TYPE_UNDEFINED;
};





}
// addr namespace


inline bool operator == (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) == 0;
}


inline bool operator != (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) != 0;
}


inline bool operator < (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) < 0;
}


inline bool operator <= (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) <= 0;
}


inline bool operator > (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) > 0;
}


inline bool operator >= (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) >= 0;
}


inline bool operator == (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) == 0;
}


inline bool operator != (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) != 0;
}


inline bool operator < (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) < 0;
}


inline bool operator <= (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) <= 0;
}


inline bool operator > (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) > 0;
}


inline bool operator >= (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) >= 0;
}


// vim: ts=4 sw=4 et
