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
#include    <iostream>
#include    <memory>
#include    <set>
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
 * the possible range like results.
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
    COMPARE_PRECEDES,                       // lhs is just before rhs
    COMPARE_FOLLOWS,                        // lhs is just after rhs
    COMPARE_FIRST,                          // lhs is defined, rhs is empty
    COMPARE_LAST,                           // lhs is empty, rhs is defined
    COMPARE_IPV4_VS_IPV6,                   // lhs is an IPv4, rhs an IPv6
    COMPARE_IPV6_VS_IPV4,                   // lhs is an IPv6, rhs an IPv4
    COMPARE_UNORDERED,                      // lhs and rhs are both empty or are not ranges
};


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


class addr
{
public:
    typedef std::shared_ptr<addr>   pointer_t;
    typedef std::set<addr>          set_t;
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
    void                            set_port_defined(bool defined = true);
    void                            set_port(int port);
    void                            set_protocol_defined(bool defined = true);
    void                            set_protocol(char const * protocol);
    void                            set_protocol(int protocol);
    void                            set_mask_defined(bool defined = true);
    void                            set_mask(uint8_t const * mask);
    void                            set_mask_count(int mask_size);
    void                            apply_mask(bool inversed = false);

    std::string                     get_interface() const;
    std::string                     get_hostname() const;
    bool                            is_hostname_an_ip() const;
    int                             get_family() const;
    bool                            is_default() const;
    bool                            is_lan(bool include_all = false) const;
    bool                            is_wan(bool include_default = true) const;
    bool                            is_ipv4() const;
    void                            get_ipv4(sockaddr_in & in) const;
    void                            get_ipv6(sockaddr_in6 & in6) const;
    std::string                     to_ipv4_string(string_ip_t mode) const;
    std::string                     to_ipv6_string(string_ip_t mode) const;
    std::string                     to_ipv4or6_string(string_ip_t mode = string_ip_t::STRING_IP_ALL) const;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    unsigned __int128               ip_to_uint128() const;
    void                            ip_from_uint128(unsigned __int128 u);
#pragma GCC diagnostic pop

    network_type_t                  get_network_type() const;
    std::string                     get_network_type_string() const;

    int                             create_socket(socket_flag_t flags) const;
    int                             connect(int s) const;
    int                             bind(int s);
    int                             bind(int s) const;
    std::string                     get_name() const;
    std::string                     get_service() const;
    bool                            get_port_defined() const;
    int                             get_port() const;
    std::string                     get_str_port() const;
    bool                            is_protocol_defined() const;
    int                             get_protocol() const;
    void                            get_mask(uint8_t * mask) const;
    int                             get_mask_size() const;
    bool                            is_mask_ipv4_compatible() const;
    bool                            is_mask_defined() const;

    bool                            match(addr const & ip, bool any = false) const;
    bool                            is_next(addr const & a) const;
    bool                            is_previous(addr const & a) const;
    bool                            operator == (addr const & rhs) const;
    bool                            operator != (addr const & rhs) const;
    bool                            operator <  (addr const & rhs) const;
    bool                            operator <= (addr const & rhs) const;
    bool                            operator >  (addr const & rhs) const;
    bool                            operator >= (addr const & rhs) const;
    addr &                          operator ++ ();
    addr                            operator ++ (int);
    addr &                          operator -- ();
    addr                            operator -- (int);
    addr                            operator + (int offset) const;
    addr                            operator - (int offset) const;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    __int128                        operator - (addr const & rhs) const;
#pragma GCC diagnostic push
    addr &                          operator += (int offset);
    addr &                          operator -= (int offset);

private:
    void                            address_changed();

    // always keep address in an IPv6 structure
    //
    sockaddr_in6                    f_address = init_in6();
    uint8_t                         f_mask[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
    bool                            f_port_defined = false;
    bool                            f_protocol_defined = false;
    bool                            f_mask_defined = false;
    int                             f_protocol = IPPROTO_TCP;
    mutable network_type_t          f_private_network = network_type_t::NETWORK_TYPE_UNDEFINED;
    std::string                     f_interface = std::string();
    std::string                     f_hostname = std::string();
};



/** \brief Set of flags attached to an ostream for an addr object.
 *
 * This structure holds data used to output an addr object in an ostream.
 *
 * The mode defines how each address is output (see addr::to_ipv4or6_string()).
 *
 * The separator is used whenever a vector or set of addresses is printed
 * out.
 */
struct _ostream_info
{
    string_ip_t         f_mode = string_ip_t::STRING_IP_ALL;
    std::string         f_sep = std::string(",");
};

/** \brief Retrieve the addr ostream index.
 *
 * Each class (or at least each project) can allocate a unique index that
 * it later references when sending data to an ostream (or istream).
 *
 * At the moment, the index is allocated for the addr class specifically.
 * If we extend the addr_range::vector_t to printing in an ostream as well,
 * we can add support for the separator between each range.
 */
int get_ostream_index();


/** \brief An intermediate structure to pass a new mode to ostream.
 *
 * This structure is used by the setaddrmode() function to update the
 * address mode in this specific ostream.
 *
 * \sa setaddrmode()
 */
struct _setaddrmode
{
    string_ip_t   f_mode;
};


/** \brief Change the address ostream mode to \p mode.
 *
 * The conversion of an address to a string uses the to_ipv4or6_string()
 * function. That function accepts a mode parameter. By default, it is
 * set to "all" (addr::string_ip_t::STRING_IP_ALL). You can change
 * the mode using this setaddrmode() function in your ostream:
 *
 * \code
 *     std::cout << setaddrmode(addr::string_ip_t::STRING_IP_PORT)
 *               << my_address
 *               << std::endl;
 * \endcode
 *
 * The most common is to use the addr::string_ip_t::STRING_IP_PORT
 * although any other value will work as expected (i.e. if you do not
 * want to the port number and do not care about brackets around an IPv6,
 * then the most basic addr::string_ip_t::STRING_IP_ONLY can be
 * used).
 */
inline _setaddrmode setaddrmode(string_ip_t mode)
{
    return { mode };
}


/** \brief An intermediate structure to pass a new separator to ostream.
 *
 * This structure is used by the setaddrsep() function to update the
 * address separator in this specific ostream.
 *
 * \sa setaddrsep()
 */
struct _setaddrsep
{
    std::string     f_sep;
};


/** \brief Change the address separator to \p sep.
 *
 * This function initializes a _setsep structure which can then be passed
 * to an ostream in order to change the separator used to print between
 * each address when writing a container of addresses to an ostream.
 *
 * The default separator is the comma (",").
 *
 * \code
 *     std::cout << setaddrsep("\n") << addr_set << std::endl;
 * \endcode
 *
 * \param[in] sep  The new separator to use to print a set of addresses.
 */
inline _setaddrsep setaddrsep(std::string const & sep)
{
    return { sep };
}


inline void basic_stream_event_callback(std::ios_base::event e, std::ios_base & out, int index)
{
    switch(e)
    {
    case std::ios_base::erase_event:
        delete static_cast<_ostream_info *>(out.pword(index));
        out.pword(index) = nullptr;
        break;

    case std::ios_base::copyfmt_event:
        {
            _ostream_info * info(static_cast<_ostream_info *>(out.pword(index)));
            if(info != nullptr)
            {
                _ostream_info * new_info(new _ostream_info(*info));
                out.pword(index) = new_info;
            }
        }
        break;

    default:
        // ignore imbue; we have nothing to do with the locale
        break;

    }
}


/** \brief Change the current address mode.
 *
 * This ostream extension function allows you to change the address mode
 * using the setaddrmode() function.
 *
 * \sa setaddrmode()
 */
template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits> &
operator << (std::basic_ostream<_CharT, _Traits> & out, _setaddrmode mode)
{
    int const index(get_ostream_index());
    _ostream_info * info(static_cast<_ostream_info *>(out.pword(index)));
    if(info == nullptr)
    {
        info = new _ostream_info;
        out.pword(index) = info;
        out.register_callback(basic_stream_event_callback, index);
    }
    info->f_mode = mode.f_mode;
    return out;
}


/** \brief Change the current address separator.
 *
 * The address containers (vector/set) can be printed with an ostream. This
 * parameter defines which separator to use between each address.
 *
 * The addresses are separated by commas by default.
 *
 * \sa setaddrsep()
 */
template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits> &
operator << (std::basic_ostream<_CharT, _Traits> & out, _setaddrsep sep)
{
    int const index(get_ostream_index());
    _ostream_info * info(static_cast<_ostream_info *>(out.pword(index)));
    if(info == nullptr)
    {
        info = new _ostream_info;
        out.pword(index) = info;
        out.register_callback(basic_stream_event_callback, index);
    }
    info->f_sep = sep.f_sep;
    return out;
}


/** \brief Output an address in your stream.
 *
 * This function outputs the specified address in your output stream.
 *
 * \param[in,out] out  The output stream where the address is written.
 * \param[in] address  The address to output in the stream.
 *
 * \return The reference to out for chaining.
 */
template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits> &
operator << (std::basic_ostream<_CharT, _Traits> & out, addr const & address)
{
    _ostream_info * info(static_cast<_ostream_info *>(out.pword(get_ostream_index())));
    if(info == nullptr)
    {
        out << address.to_ipv4or6_string(string_ip_t::STRING_IP_ALL);
    }
    else
    {
        out << address.to_ipv4or6_string(info->f_mode);
    }
    return out;
}


template<typename _CharT, typename _Traits, typename _ContainerT>
inline typename std::enable_if<
          std::is_same<_ContainerT, addr::vector_t>::value
                || std::is_same<_ContainerT, addr::set_t>::value
        , std::basic_ostream<_CharT, _Traits>>::type &
operator << (std::basic_ostream<_CharT, _Traits> & out, _ContainerT const & addresses)
{
    std::string sep(",");
    _ostream_info * info(static_cast<_ostream_info *>(out.pword(get_ostream_index())));
    if(info != nullptr)
    {
        sep = info->f_sep;
    }

    bool first(true);
    for(auto const & a : addresses)
    {
        if(first)
        {
            first = false;
        }
        else
        {
            out << sep;
        }
        out << a;
    }
    return out;
}



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
