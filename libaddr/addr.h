// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief The various libaddr classes.
 *
 * This header includes the base addr class used to handle one binary
 * address.
 */

// C++
//
#include    <cstring>
#include    <memory>
#include    <string>
#include    <vector>


// C
//
#include    <arpa/inet.h>



namespace addr
{




/** \brief Initialize an IPv6 address as such.
 *
 * This function initializes a sockaddr_in6 with all zeroes except
 * for the sin6_family which is set to AF_INET6.
 *
 * return The initialized IPv6 address.
 */
constexpr sockaddr_in6 init_in6()
{
    sockaddr_in6 in6 = sockaddr_in6();
    in6.sin6_family = AF_INET6;
    return in6;
}


/** \brief Result of a compare between IP addresses.
 *
 * This enumeration includes compare results between IP addresses.
 *
 * The results can also be used by the addr_range class which explains
 * the range like results.
 */
enum class compare_t
{
    COMPARE_EQUAL,                          // lhs == rhs
    COMPARE_SMALLER,                        // lhs < rhs
    COMPARE_LARGER,                         // lhs > rhs
    COMPARE_OVERLAP_SMALL_VS_LARGE,         // lhs is before rhs with an overlap
    COMPARE_OVERLAP_LARGE_VS_SMALL,         // rhs is before lhs with an overlap
    COMPARE_INCLUDED,                       // rhs is included in lhs
    COMPARE_INCLUDES,                       // lhs is included in rhs
    COMPARE_IPV4_VS_IPV6,                   // lhs is an IPv4, rhs an IPv6
    COMPARE_IPV6_VS_IPV4,                   // lhs is an IPv6, rhs an IPv4
    COMPARE_SMALL_VS_LARGE,                 // lhs is smaller than rhs
    COMPARE_LARGE_VS_SMALL,                 // lhs is larger than rhs
    COMPARE_FOLLOWS,                        // rhs is just after lhs
    COMPARE_FOLLOWING,                      // lhs is just after rhs
    COMPARE_FIRST,                          // lhs is defined, rhs is empty
    COMPARE_LAST,                           // lhs is empty, rhs is defined
    COMPARE_UNORDERED,                      // lhs and rhs are both empty
};


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
                                    addr(sockaddr_in const & in);
                                    addr(sockaddr_in6 const & in6);

    void                            set_interface(std::string const & interface);
    void                            set_hostname(std::string const & hostname);
    void                            set_from_socket(int s, bool peer);
    void                            set_ipv4(sockaddr_in const & in);
    void                            set_ipv6(sockaddr_in6 const & in6);
    void                            set_port(int port);
    void                            set_protocol(char const * protocol);
    void                            set_protocol(int protocol);
    void                            set_mask(uint8_t const * mask);
    void                            apply_mask();

    std::string                     get_interface() const;
    std::string                     get_hostname() const;
    bool                            is_hostname_an_ip() const;
    int                             get_family() const;
    bool                            is_default() const;
    bool                            is_ipv4() const;
    void                            get_ipv4(sockaddr_in & in) const;
    void                            get_ipv6(sockaddr_in6 & in6) const;
    std::string                     to_ipv4_string(string_ip_t mode) const;
    std::string                     to_ipv6_string(string_ip_t mode) const;
    std::string                     to_ipv4or6_string(string_ip_t mode) const;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    unsigned __int128               ip_to_uint128() const;
#pragma GCC diagnostic pop

    network_type_t                  get_network_type() const;
    std::string                     get_network_type_string() const;

    int                             create_socket(socket_flag_t flags) const;
    int                             connect(int s) const;
    int                             bind(int s) const;
    std::string                     get_name() const;
    std::string                     get_service() const;
    int                             get_port() const;
    int                             get_protocol() const;
    void                            get_mask(uint8_t * mask) const;
    int                             get_mask_size() const;

    bool                            match(addr const & ip, bool any = false) const;
    bool                            is_next(addr const & a) const;
    bool                            is_previous(addr const & a) const;
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
    sockaddr_in6                    f_address = init_in6();
    uint8_t                         f_mask[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
    int                             f_protocol = IPPROTO_TCP;
    mutable network_type_t          f_private_network = network_type_t::NETWORK_TYPE_UNDEFINED;
    std::string                     f_interface = std::string();
    std::string                     f_hostname = std::string();
};





}
// namespace addr


inline bool operator == (sockaddr_in6 const & a, sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_in6)) == 0;
}


inline bool operator != (sockaddr_in6 const & a, sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_in6)) != 0;
}


inline bool operator < (sockaddr_in6 const & a, sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_in6)) < 0;
}


inline bool operator <= (sockaddr_in6 const & a, sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_in6)) <= 0;
}


inline bool operator > (sockaddr_in6 const & a, sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_in6)) > 0;
}


inline bool operator >= (sockaddr_in6 const & a, sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_in6)) >= 0;
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
