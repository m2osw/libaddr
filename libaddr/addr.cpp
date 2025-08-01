// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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


/** \file
 * \brief The implementation of the addr class.
 *
 * This file includes the implementation of the addr class. The one that
 * deals with low level classes.
 */

// self
//
#include    "libaddr/addr.h"
#include    "libaddr/exception.h"


// advgetopt
//
#include    <advgetopt/validator_integer.h>


// cppthread
//
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>


// snapdev
//
#include    <snapdev/int128_literal.h>
#include    <snapdev/math.h>
#include    <snapdev/not_reached.h>
#include    <snapdev/ostream_int128.h>
#include    <snapdev/static_to_dynamic_buffer.h>


// C++
//
#include    <sstream>
#include    <iostream>


// C
//
#include    <netdb.h>


// last include
//
#include    <snapdev/poison.h>



/** \mainpage
 * \brief libaddr, a C++ library to handle network IP addresses in IPv4 and IPv6.
 *
 * ### Introduction
 *
 * This library is used to parse strings of IP addresses to lists of
 * binary IP addresses ready to be used by functions such as bind(),
 * send(), recv(), etc.
 *
 * The library supports multiple addresses separated by commas and/or
 * spaces, ports, and CIDR masks. It can check whether an address matches
 * another taking the mask in account. It can sort IPs numerically. It
 * can determine the type of an IP address (i.e. is it a local address,
 * a private address, a public address?)
 *
 * The library also has a function to read IP addresses from your
 * computer interfaces and return that list. Very practical to know
 * whether an IP address represents your computer or not.
 *
 * ### Usage
 *
 * The library is composed of three main classes:
 *
 * \li addr
 *
 * The address class holds one address, a port, a protocol and a few
 * other parts. This is what one uses to connect or listen with an
 * address.
 *
 * The address is kept by addr in an IPv6 address structure.
 *
 * By default the CIDR of the address is all 1s (i.e. no masking, all
 * bits considered important.) The mask is always 128 bits. If you are
 * dealing with IPv4, make sure that the first 12 bytes are set to 255.
 *
 * The class also offers a set of functions to transform the binary
 * address it is holding to a string.
 *
 * \li addr_range
 *
 * It is possible to define a range of addresses and ports. This class
 * holds a 'from' address and a 'to' address. By default neither is
 * defined. You have to call the set_from() and set_to() functions.
 *
 * The addr_range::vector_t is what the addr_parser returns after
 * parsing a string representing one of more addresses.
 *
 * \note
 * The range is functional, however, the parser does not yet support
 * parsing range of addresses and ports.
 *
 * \li addr_parser
 *
 * The parser is used to transform a string to an address.
 *
 * It supports many variations of its input, which are handled by
 * the 'allow' flags. The set_allow() and get_allow() functions can
 * be used to tweak the parser in supporting such and such feature.
 *
 * By default, the input is expected to be an address and a port
 * separated by a colon (i.e. `"1.2.3.4:1234"` in IPv4 and `"[::1]:1234"`
 * in IPv6.)
 *
 * ### Parser
 *
 * The parser supports the following syntax (ranges are not yet supported
 * and they do not appear in the following list):
 *
 * \code
 *    start: address_list
 *
 *    address_list: address_cidr
 *                | address_list address_list_separators address_cidr
 *
 *    address_list_separators: ' '
 *                           | ','
 *                           | address_list_separators address_list_separators
 *
 *    address_cidr: address_port
 *                | address_port '/' cidr
 *
 *    address_port: address
 *                | address ':' port
 *
 *    address: ipv4
 *           | ipv6
 *
 *    cidr: decimal_number
 *        | ipv4
 *        | ipv6
 *
 *    ipv4: decimal_number '.' decimal_number '.' decimal_number '.' decimal_number
 *
 *    ipv6: '[' hexadecimal_number_list ']'
 *
 *    port: decimal_number
 *
 *    hexadecimal_number_list: hexadecimal_number
 *                           | hexadecimal_number_list ':' hexadecimal_number
 *
 *    decimal_number: [0-9]+
 *
 *    hexadecimal_number: [0-9a-fA-F]+
 * \endcode
 *
 * When accepting multiple addresses separated by commas or spaces, the
 * number of commas and spaces separating two address is not significant.
 * The input string can also start or end with commas and spaces. The
 * following variable defines exactly two IP address:
 *
 * \code
 *       addresses=  ,1.2.3.4,   ,5.6.7.8,,
 * \endcode
 *
 * (note that the parser should not be passed the "addresses=" part.)
 */


/** \brief The libaddr classes are all defined in this namespace.
 *
 * The addr namespace includes all the addr classes.
 */
namespace addr
{

/*
 * Various sytem address structures

// Any address is 16 bytes or less
struct sockaddr {
   unsigned short    sa_family;    // address family, AF_xxx
   char              sa_data[14];  // 14 bytes of protocol address
};

struct sockaddr_storage {
    sa_family_t  ss_family;     // address family

    // all this is padding, implementation specific, ignore it:
    char      __ss_pad1[_SS_PAD1SIZE];
    int64_t   __ss_align;
    char      __ss_pad2[_SS_PAD2SIZE];
};


typedef uint32_t in_addr_t;   // or `__be32`
struct in_addr {
    in_addr_t        s_addr;
};


// IPv4
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};


struct in6_addr
{
    union
    {
         uint8_t    __u6_addr8[16];
#ifdef __USE_MISC
         uint16_t   __u6_addr16[8];
         uint32_t   __u6_addr32[4];
#endif
    } __in6_u;
#define s6_addr     __in6_u.__u6_addr8
#ifdef __USE_MISC
# define s6_addr16  __in6_u.__u6_addr16
# define s6_addr32  __in6_u.__u6_addr32
#endif
};

// IPv6
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // address family, AF_INET6
    u_int16_t       sin6_port;     // port number, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // Scope ID
};


*/


namespace
{



/** \brief Whether the ostream index was allocated.
 *
 * This flag tells us whether we already allocated the ostream index or
 * not. We want to allocate it exactly once.
 *
 * This allows our implementation works even when you use the feature
 * before calling main().
 */
bool g_ostream_index_allocated = false;


/** \brief The ostream index.
 *
 * This value represents the ostream index as returned by the
 * std::ios_base::xalloc() function.
 *
 * You retrieve this value by calling the get_ostream_index() function
 * to make sure it is safely allocated in a multithread environment before
 * or after main() was called.
 *
 * \note
 * There no default value. Although it looks like the C++ implementation
 * under Linux returns 4 the first time we call xalloc(), this is an
 * implementation detail. To know whether the index was allocated or not,
 * we instead use the g_ostream_index_allocated flag.
 */
int g_ostream_index = 0;



} // no name namespace



/** \brief Create an addr object that represents an ANY address.
 *
 * This function initializes the addr object with the ANY address.
 * The port is set to 0 and the protocol to TCP.
 *
 * It is strongly suggested that you change those parameters
 * before really using this address since a port of zero and
 * the protocol may be wrong.
 */
addr::addr()
{
    // keep default protocol (TCP)
}


/** \brief Create an addr object from a binary IPv4 address.
 *
 * This function initializes this addr object with the specified IPv4
 * address. The is_ipv4() function will return true.
 *
 * \param[in] in  The binary IPv4 address.
 */
addr::addr(sockaddr_in const & in)
{
    set_ipv4(in);
    // keep default protocol (TCP)
}


/** \brief Create an addr object from a binary IPv6 address.
 *
 * This function initializes this addr object with the specified IPv6
 * address. The is_ipv4() function will return false.
 *
 * \param[in] in6  The binary IPv6 address.
 */
addr::addr(sockaddr_in6 const & in6)
{
    set_ipv6(in6);
    // keep default protocol (TCP)
}


/** \brief Save an IPv4 in this addr object.
 *
 * This function saves the specified IPv4 in this addr object.
 *
 * Since we save the data in an IPv6 structure, it is kept in
 * the addr as an IPv4 mapped in an IPv6 address. It can still
 * be retrieved right back as an IPv4 with the get_ipv4() function.
 *
 * \param[in] in  The IPv4 address to save in this addr object.
 */
void addr::set_ipv4(sockaddr_in const & in)
{
    if(in.sin_family != AF_INET)
    {
        // although we convert the IPv4 to an IPv6 below, we really only
        // support AF_INET on entry
        //
        throw addr_invalid_argument("addr::set_ipv4(): the input address does not represent an IPv4 address (family is not AF_INET).");
    }

    // reset the address first
    memset(&f_address, 0, sizeof(f_address));

    // then transform the IPv4 to an IPv6
    //
    // Note: this is not an IPv6 per se, it is an IPv4 mapped within an
    //       IPv6 and your network stack needs to support IPv4 anyway
    //       in order to use that IP...
    //
    f_address.sin6_family = AF_INET6;
    f_address.sin6_port = in.sin_port;
    f_address.sin6_addr.s6_addr16[5] = 0xFFFF;
    f_address.sin6_addr.s6_addr32[3] = in.sin_addr.s_addr;

    address_changed();
}


/** \brief Set the address as the IPv4 loopback address.
 *
 * This function sets the address to 127.0.0.1. Remember that loopback
 * addresses can be anything in 127.0.0.0/8. The 127.0.0.1 is one out
 * of 16 million possible addresses.
 *
 * For example, some DNS have been using 127.0.0.53.
 */
void addr::set_ipv4_loopback()
{
    sockaddr_in in = {};

    in.sin_family = AF_INET;
    in.sin_port = f_address.sin6_port;
    in.sin_addr.s_addr = htonl((127 << 24) + 1);
    set_ipv4(in);
}


/** \brief Mark the port as defined.
 *
 * When parsing an address, the port in an addr object is always considered
 * valid. We have no other means to mark the port as defined or not, so we
 * use a separate flag.
 *
 * \param[in] defined  Whether the port was defined (true) or not.
 */
void addr::set_port_defined(bool defined)
{
    f_port_defined = defined;
}


/** \brief Set the port by name or number.
 *
 * This function checks whether the \p port parameter represents a port
 * number ("80") or a port name ("http"). If the name is found in the
 * /etc/services file, then the corresponding number is used. If not found
 * then the function throws.
 *
 * \param[in] port  The name of the protcol to use.
 *
 * \return true if the input was recognized as a valid number or a name
 * which matches one of the services defined in /etc/services.
 */
bool addr::set_port(char const * port)
{
    // if the port is a decimal number, use it as is
    //
    std::int64_t p(0);
    if(advgetopt::validator_integer::convert_string(port, p))
    {
        set_port(p);
        return true;
    }

    struct servent service = {};
    struct servent * list(nullptr);

    snapdev::static_to_dynamic_buffer<char, 1024> buf;
    std::string const proto(get_protocol_name());
    for(;;)
    {
        int const r(getservbyname_r(
                  port
                , proto.c_str()
                , &service
                , buf.get()
                , buf.size()
                , &list));

        // from the example, r may be 0 on a "not found" in which case
        // the list pointer is nullptr
        //
        if(r == 0 && list != nullptr)
        {
            set_port(ntohs(service.s_port));
            return true;
        }
        if(r != ERANGE)
        {
            return false;
        }
        buf.increase_size(1024);        // LCOV_EXCL_LINE
    }                                   // LCOV_EXCL_LINE
}


/** \brief Set the port of this address.
 *
 * This function changes the port of this address to \p port.
 *
 * When setting the port to 0 and then binding this address, the port will
 * then automatically be assigned by the network stack. This is useful
 * to create UDP connections that work both ways. In that case, the client
 * should set its port to 0 before calling bind().
 *
 * \exception addr_invalid_argument
 * This exception is raised whenever the \p port parameter is set to
 * an invalid number (negative or larger than 65535.)
 *
 * \param[in] port  The new port to save in this address.
 */
void addr::set_port(int port)
{
    if(port > 65535
    || port < 0)
    {
        throw addr_invalid_argument("port to set_port() cannot be out of the allowed range [0..65535].");
    }
    f_port_defined = true;
    f_address.sin6_port = htons(port);
}


/** \brief Mark the protocol as defined.
 *
 * This function marks the protocol as defined whether the set_protocol()
 * gets called or not.
 *
 * This is used by users of the object to know whether the protocol was
 * explicitly defined or just defined as a default like we often do.
 *
 * If you want to have a specific default, you can first call the
 * set_protocol() and then call this function and set the \p defined
 * parameter to false.
 *
 * \param[in] defined  Whether the protocol was defined or not.
 *
 * \sa set_protocol()
 * \sa get_protocol()
 * \sa get_protocol_name()
 */
void addr::set_protocol_defined(bool defined)
{
    f_protocol_defined = defined;
}


/** \brief Change the protocol using a string.
 *
 * This function is used to change the current protocol defined in
 * this addr object.
 *
 * \exception addr_invalid_argument
 * The supported protocol names are defined in /etc/protocols. However,
 * internally, we are likely to support many less protocols. This function
 * calls the set_protocol(int) function to define this address protocol
 * and it is likely to raise an error on unsupported protocols.
 *
 * \param[in] protocol  The name of the protocol.
 *
 * \sa get_protocol()
 * \sa get_protocol_name()
 */
void addr::set_protocol(char const * protocol)
{
    if(protocol == nullptr)
    {
        throw addr_invalid_argument("protocol pointer to set_protocol() cannot be a nullptr.");
    }

    protoent proto = {};
    protoent * ptr(nullptr);

    snapdev::static_to_dynamic_buffer<char, 1024> buf;
    for(;;)
    {
        int const r(getprotobyname_r(
                  protocol
                , &proto
                , buf.get()
                , buf.size()
                , &ptr));
        if(r == 0
        && ptr != nullptr)
        {
            break;
        }
        if(r != ERANGE)
        {
            throw addr_invalid_argument(
                              std::string("unknown protocol \"")
                            + protocol
                            + "\", expected \"tcp\" or \"udp\" (string).");
        }
        buf.increase_size(1024);    // LCOV_EXCL_LINE
    }                               // LCOV_EXCL_LINE

    set_protocol(proto.p_proto);
}


/** \brief Set the protocol numerically.
 *
 * This function sets the protocol from a number instead of a name.
 *
 * Note that we only support IPPROTO_TCP and IPPROTO_UDP for now.
 * Any other protocol will make this function raise an exception.
 *
 * \todo
 * We may want to support any protocol number at this level. If your
 * application is limited then it should verify the protocol and
 * make sure it supports it before using this address. At the same
 * time, the IP protocol is pretty much locked up with just TCP
 * and UDP these days (there is the IP protocol, but that's not
 * useful at our level.)
 *
 * \exception addr_invalid_argument
 * This exception is raised if the specified protocol is not currently
 * supported by the addr implementation.
 *
 * \param[in] protocol  The new numeric protocol.
 */
void addr::set_protocol(int protocol)
{
    switch(protocol)
    {
    case IPPROTO_IP:
    case IPPROTO_TCP:
    case IPPROTO_UDP:
        if(protocol != f_protocol)
        {
            f_protocol_defined = true;
            f_protocol = protocol;

            // TBD: I don't think that the network type is affected by the
            //      protocol value; but long term this may be a good idea?
            //
            address_changed();
        }
        break;

    default:
        throw addr_invalid_argument(
                          "unknown protocol number "
                        + std::to_string(protocol)
                        + ", expected \"tcp\" ("
                        + std::to_string(static_cast<int>(IPPROTO_TCP))
                        + ") or \"udp\" ("
                        + std::to_string(static_cast<int>(IPPROTO_UDP))
                        + ") (numeric).");

    }
}


/** \brief Change the mask defined flag.
 *
 * By default, address has no mask. This means a mask has a size of 0.
 *
 * To make sure that the mask was defined, and since the size is not
 * reliable, the flag can be used for that purpose. If the set_mask()
 * and set_mask_count() functions are called, that flag will be set
 * to true. By default the flag is false.
 *
 * This function can be used to force the flag to true or false.
 *
 * \param[in] defined  Whether the mask was defined (true) or not (false).
 */
void addr::set_mask_defined(bool defined)
{
    f_mask_defined = defined;
}


/** \brief Set the mask.
 *
 * The input mask must be at least 16 bytes. If you are dealing with an
 * IPv4, make sure the first 12 bytes are 255.
 *
 * \todo
 * Look into having a type that clearly defines the buffer size.
 *
 * \param[in] mask  The mask to save in this address.
 */
void addr::set_mask(std::uint8_t const * mask)
{
    f_mask_defined = true;
    memcpy(f_mask, mask, sizeof(f_mask));
}


/** \brief Set the mask.
 *
 * This function sets the mask to the specified number. An IPv4 supports a
 * mask of 0 to 32, but we only handle IPv6 addresses which are 128 bits.
 * So for an IPv4, call this function with a number from 96 to 128.
 *
 * \exception out_of_range
 * If the \p mask_size parameter is too small or too large, then this
 * exception is raised.
 *
 * \param[in] mask_size  The number of bits that are not masked (number of 1s
 * starting from the left side, the same as the `.../CIDR` number).
 */
void addr::set_mask_count(int mask_size)
{
    int const min(is_ipv4() ? 96 : 0);
    if(mask_size < min || mask_size > 128)
    {
        throw out_of_range("the mask size " + std::to_string(mask_size) + " is out of range.");
    }

    f_mask_defined = true;
    std::size_t count(sizeof(f_mask));
    std::uint8_t * mask(f_mask);
    while(mask_size >= 8)
    {
        *mask = 0xFF;
        ++mask;
        mask_size -= 8;
        --count;
    }
    if(mask_size != 0)
    {
        *mask = 0xFF00 >> mask_size;
        ++mask;
        --count;
    }
    for(; count > 0; --count, ++mask)
    {
        *mask = 0;
    }
}


/** \brief Apply the mask to the IP address.
 *
 * This function applies the mask to this address IP address. We offer
 * to functions:
 *
 * 1. AND (\p inversed = false)
 *
 *    This means the bits that are 0 in the mask will also be 0 in the
 *    address once the function returns.
 *
 * 2. OR (\p inversed = true)
 *
 *    This means the bits that are 0 in the mask are set to 1 in the
 *    address once the function returns.
 *
 * This should be called to canonicalize an IP/mask in cases where the
 * address represents a group of all the addresses represented by the
 * mask. It can also be useful to generate the broadcast address (when
 * inversed is set to true).
 *
 * \param[in] inversed  If true, applied the inversed mask with an OR (|).
 */
void addr::apply_mask(bool inversed)
{
    for(int idx(0); idx < 16; ++idx)
    {
        if(inversed)
        {
            f_address.sin6_addr.s6_addr[idx] |= ~f_mask[idx];
        }
        else
        {
            f_address.sin6_addr.s6_addr[idx] &= f_mask[idx];
        }
    }
}


/** \brief Get the mask.
 *
 * The output buffer for the mask must be at least 16 bytes. If you are
 * dealing with an IPv4, all the bytes are expected to be 255 except
 * the bottom 4 bytes (offset 12, 13, 14, 15).
 *
 * \todo
 * Look into having a type that clearly defines the buffer size.
 *
 * \param[out] mask  The buffer where the mask gets copied.
 */
void addr::get_mask(std::uint8_t * mask) const
{
    memcpy(mask, f_mask, sizeof(f_mask));
}


/** \brief Get the mask as a number of bits set to 1.
 *
 * This function computes the number of bits set to 1 starting from the most
 * significant bit.
 *
 * If the function detects that a simple number of bits cannot represent
 * the mask, then the function returns -1.
 *
 * A count of 0 means that the mask is all zeroes.
 *
 * A count of 128 means that the mask is all ones.
 *
 * To compute an IPv4 count, subtract 96 from the result. If the result is
 * not at least 96, then the mask is not a valid IPv4 mask.
 *
 * \note
 * The current Internet consortium says that only masks that can be
 * represented as a simple number are valid. In other words, if this
 * function returns -1, this means the mask is considered invalid.
 *
 * \return The number of bits in the mask or -1 if the mask has holes.
 */
int addr::get_mask_size() const
{
    int count(0);

    bool found(false);
    for(std::size_t i(0); i < sizeof(f_mask); ++i)
    {
        if(found)
        {
            if(f_mask[i] != 0x00)
            {
                return -1;
            }
        }
        else
        {
            switch(f_mask[i])
            {
            case 0xFF:
                count += 8;
                break;

            case 0xFE:
                count += 7;
                found = true;
                break;

            case 0xFC:
                count += 6;
                found = true;
                break;

            case 0xF8:
                count += 5;
                found = true;
                break;

            case 0xF0:
                count += 4;
                found = true;
                break;

            case 0xE0:
                count += 3;
                found = true;
                break;

            case 0xC0:
                count += 2;
                found = true;
                break;

            case 0x80:
                count += 1;
                found = true;
                break;

            case 0x00:
                found = true;
                break;

            default:
                return -1;

            }
        }
    }

    return count;
}


/** \brief Check whether the mask makes sense for an IPv4 address.
 *
 * When converting a mask to a string, it has to be compatible with
 * an IPv4 if the address is an IPv4 otherwise that mask makes no
 * sense.
 *
 * For the mask to be a valid IPv4 address, the get_mask_size()
 * function must return 96 or more. This function is a simplification
 * which verifies that (nearly) as fast as possible.
 *
 * \return true if the mask can be used by an IPv4, false otherwise.
 */
bool addr::is_mask_ipv4_compatible() const
{
    for(std::size_t i(0); i < sizeof(f_mask) - 4; ++i)
    {
        if(f_mask[i] != 255)
        {
            return false;
        }
    }

    return true;
}


/** \brief Check whether the mask was defined.
 *
 * This function returns true if the mask was defined. The mask is marked
 * as defined when one of the set_mask() function gets called.
 */
bool addr::is_mask_defined() const
{
    return f_mask_defined;
}


/** \brief Return the interface name.
 *
 * It is possible to indicate an interface name along an address to make
 * sure that this address is only used with that specific interface. If
 * the interface does not support that address, then the bind() will
 * fail.
 *
 * \return The interface name attached to this address or an empty string.
 *
 * \sa set_hostname()
 */
std::string addr::get_interface() const
{
    return f_interface;
}


/** \brief Return the original hostname.
 *
 * When parsing an address with the addr::addr_parser::parse() function,
 * the hostnames appearing in the input get transformed in IP addresses.
 * It also saves the corresponding address in the hostname parameter
 * of the address and it can be retrieved with this function.
 *
 * You can use the set_hostname() function if you would like to change
 * this value.
 *
 * \note
 * The parser saves the original address which means the hostname may
 * actually be an IP address. You can check that with the
 * is_hostname_an_ip() function.
 *
 * \return The original hostname.
 *
 * \sa set_hostname()
 */
std::string addr::get_hostname() const
{
    return f_hostname;
}


/** \brief Check whether the hostname is an IP address or not.
 *
 * Check whether the hostname represents a valid IP address (opposed to
 * an actual domain name).
 *
 * \warning
 * If you did not use the parser, the hostname is empty and therefore
 * it is neither an IP nor a hostname. However, in this special case
 * this function returns true.
 *
 * \return true if the hostname parameter represents an IP address.
 */
bool addr::is_hostname_an_ip() const
{
    if(f_hostname.empty())
    {
        // this is not a valid hostname, so return true
        //
        return true;
    }

    in6_addr ignore;
    static_assert(sizeof(ignore) >= sizeof(in_addr));
    return inet_pton(AF_INET, f_hostname.c_str(), &ignore) == 1
        || inet_pton(AF_INET6, f_hostname.c_str(), &ignore) == 1;
}


/** \brief Get the family representing this IP address.
 *
 * This is either an IPv4, so AF_INET, or an IPv6, in which case the function
 * returns AF_INET6. No other family is supported by the addr at the moment.
 *
 * \return One of AF_INET or AF_INET6.
 */
int addr::get_family() const
{
    return is_ipv4() ? AF_INET : AF_INET6;
}


/** \brief Check whether this address represents the ANY or NULL address.
 *
 * The IPv4 and IPv6 have an ANY address also called the default address
 * and the null address. This function returns true if this address
 * represents the ANY address.
 *
 * The any address is represented by `"0.0.0.0"` in IPv4 and `"::"` in
 * IPv6. (i.e. all zeroes)
 *
 * \note
 * You can also determine this by calling the get_network_type() function
 * and compare the result against `network_type_t::NETWORK_TYPE_ANY`.
 *
 * \warning
 * The IP may have a mask attached to it (see is_mask_defined()). If
 * so then is_default() may still return true but the address object
 * really represents a range (i.e. "0.0.0.0/8"). You want to use both
 * functions to really make sure it is just the default address.
 *
 * \return true if this addr represents the any address.
 *
 * \sa is_mask_defined()
 */
bool addr::is_default() const
{
    // this is for IPv4 or IPv6
    //
    return f_address.sin6_addr.s6_addr32[0] == 0
        && f_address.sin6_addr.s6_addr32[1] == 0
        && f_address.sin6_addr.s6_addr16[4] == 0
        && (f_address.sin6_addr.s6_addr16[5] == 0 || f_address.sin6_addr.s6_addr16[5] == 0xFFFF)
        && f_address.sin6_addr.s6_addr32[3] == 0;
}


/** \brief Check whether the IP address is considered valid.
 *
 * Some IPv6 IP addresses are not considered valid. They represent things
 * such as documentation or were deprecated.
 *
 * \note
 * The IPv6 addresses used to represent IPv4 are considered valid, although
 * as IPv6 addresses they are not (that is, such IPs cannot be used with
 * the IPv6 stack).
 *
 * \todo
 * Actually find a document about all the prefixes and determine the
 * ones that do not represent a valid IPv6 address. At the moment, we
 * only include documentation IPs (prefix 2001:db8::/32 and 3fff::/20).
 *
 * \return true is the address is considered valid.
 */
bool addr::is_valid() const
{
    network_type_t const type(addr::get_network_type());
    switch(type)
    {
    case network_type_t::NETWORK_TYPE_DOCUMENTATION:
    case network_type_t::NETWORK_TYPE_UNDEFINED: // this should not happen here
        return false;

    default:
        return true;

    }
}


/** \brief Check whether the address is a LAN IP.
 *
 * For some connections, you may want to prevent them on a WAN connection
 * (i.e. public network such as the Internet).
 *
 * This function checks whether the IP represents the Private network
 * (i.e. 10.x.x.x, 192.16.x.x, etc.) or the Loopback (127.x.x.x).
 *
 * Note that by default the special IPs are not viewed as LAN IPs. These
 * are used in very specific circumstances and are not likely to be used
 * in a configuration file where an administrator could enter such an IP.
 * For this reason, this function returns false by default for those IP
 * addresses. To include those addresses, set the \p include_all flag
 * to true.
 *
 * \note
 * Of course, the function works with IPv4 and IPv6 addresses. The
 * examples above only show IPv4 IPs as these are well known.
 *
 * \param[in] include_all  Also view carrier, link local, and multicast as
 * LAN connections and return true in those cases too.
 *
 * \return true if the IP represents a LAN IP.
 */
bool addr::is_lan(bool include_all) const
{
    network_type_t const type(get_network_type());

    if(type == network_type_t::NETWORK_TYPE_PRIVATE
    || type == network_type_t::NETWORK_TYPE_LOOPBACK)
    {
        return true;
    }

    if(!include_all)
    {
        return false;
    }

    switch(type)
    {
    case network_type_t::NETWORK_TYPE_CARRIER:
    case network_type_t::NETWORK_TYPE_LINK_LOCAL:
    case network_type_t::NETWORK_TYPE_MULTICAST:
        return true;

    default:
        return false;

    }

    snapdev::NOT_REACHED();
}


/** \brief Check wether the IP address represents a WAN IP.
 *
 * This function returns true if the IP address represents a WAN or public
 * IP address. Addresses that are considered PUBLIC (a.k.a. "unknown") are
 * considerd WAN IPs and thus this function returns true in that case.
 *
 * Further, when the \p include_default flag is set to true (which is the
 * default) the IP can be the ANY address (0.0.0.0 or ::). If you do not
 * want to allow the ANY address (safer, but required the client to know
 * of your address) then make sure to set that flag to false.
 *
 * \param[in] include_default  Whether the ANY address is viewed as a WAN
 * address for this test.
 *
 * \return true if the IP address represents a public IP.
 */
bool addr::is_wan(bool include_default) const
{
    network_type_t const type(get_network_type());

    if(type == network_type_t::NETWORK_TYPE_PUBLIC)
    {
        return true;
    }

    if(include_default
    && type == network_type_t::NETWORK_TYPE_ANY)
    {
        return true;
    }

    return false;
}


/** \brief Check whether this address represents an IPv4 address.
 *
 * The IPv6 format supports embedding IPv4 addresses. This function
 * returns true if the embedded address is an IPv4. When this function
 * returns true, the get_ipv4() can be called. Otherwise, the get_ipv4()
 * function throws an exception.
 *
 * \return true if this address represents an IPv4 address.
 */
bool addr::is_ipv4() const
{
    return f_address.sin6_addr.s6_addr32[0] == 0
        && f_address.sin6_addr.s6_addr32[1] == 0
        && f_address.sin6_addr.s6_addr16[4] == 0
        && f_address.sin6_addr.s6_addr16[5] == 0xFFFF;
}


/** \brief Retreive the IPv4 address.
 *
 * This function can be used to retrieve the IPv4 address of this addr
 * object. If the address is not an IPv4, then the function throws.
 *
 * \exception addr_invalid_state
 * This exception is raised if the address is not an IPv4 address.
 *
 * \param[out] in  The structure where the IPv4 Internet address gets saved.
 */
void addr::get_ipv4(sockaddr_in & in) const
{
    if(is_ipv4())
    {
        // this is an IPv4 mapped in an IPv6, "unmap" that IP
        //
        memset(&in, 0, sizeof(in));
        in.sin_family = AF_INET;
        in.sin_port = f_address.sin6_port;
        in.sin_addr.s_addr = f_address.sin6_addr.s6_addr32[3];
        return;
    }

    throw addr_invalid_state("Not an IPv4 compatible address.");
}


/** \brief Save the specified IPv6 address in this addr object.
 *
 * This function saves the specified IPv6 address in this addr object.
 * The function does not check the validity of the address. It is
 * expected to be valid.
 *
 * The address may be an embedded IPv4 address.
 *
 * \param[in] in6  The source IPv6 to save in the addr object.
 */
void addr::set_ipv6(sockaddr_in6 const & in6)
{
    if(in6.sin6_family != AF_INET6)
    {
        throw addr_invalid_argument("addr::set_ipv6(): the input address does not represent an IPv6 address (family is not AF_INET6).");
    }
    memcpy(&f_address, &in6, sizeof(in6));

    address_changed();
}


/** \brief Set the address as the IPv6 loopback address.
 *
 * This function sets the address to ::1.
 */
void addr::set_ipv6_loopback()
{
    sockaddr_in6 in6 = {};

    in6.sin6_family = AF_INET6;
    in6.sin6_port = f_address.sin6_port;
    in6.sin6_addr.s6_addr16[7] = htons(1);
    set_ipv6(in6);
}


/** \brief Retrieve a copy of this addr IP address.
 *
 * This function returns the current IP address saved in this
 * addr object. The IP may represent an IPv4 address in which
 * case the is_ipv4() returns true.
 *
 * \param[out] in6  The structure where the address gets saved.
 */
void addr::get_ipv6(sockaddr_in6 & in6) const
{
    memcpy(&in6, &f_address, sizeof(in6));
}


/** \brief Retrive the IPv4 as a string.
 *
 * This function returns a string representing the IP address
 * defined in this addr object.
 *
 * The \p mode parameter defines what gets output.
 *
 * \li ip_string_t::STRING_IP_ONLY -- only the IP address
 * \li ip_string_t::STRING_IP_PORT -- the IP and port
 * \li ip_string_t::STRING_IP_MASK -- the IP and mask
 * \li ip_string_t::STRING_IP_ALL -- the IP, port, and mask
 *
 * The ip_string_t::STRING_IP_BRACKETS is viewed as
 * ip_string_t::STRING_IP_ONLY.
 *
 * The ip_string_t::STRING_IP_BRACKETS_MASK is viewed as
 * ip_string_t::STRING_IP_MASK.
 *
 * \exception addr_invalid_state_exception
 * If the addr object does not currently represent an IPv4 then
 * this exception is raised.
 *
 * \param[in] mode  How the output string is to be built.
 */
std::string addr::to_ipv4_string(string_ip_t const mode) const
{
    std::stringstream result;

    // this is an IPv4 mapped in an IPv6, "unmap" that IP
    // so the inet_ntop() can correctly generate an output IP
    //
    if((mode & (STRING_IP_ADDRESS | STRING_IP_BRACKET_ADDRESS)) != 0)
    {
        if(!is_ipv4())
        {
            throw addr_invalid_state("Not an IPv4 compatible address.");
        }

        if((mode & STRING_IP_DEFAULT_AS_ASTERISK) != 0
        && is_default())
        {
            result << "*";
        }
        else
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
            in_addr in = {
                .s_addr = f_address.sin6_addr.s6_addr32[3],
            };
#pragma GCC diagnostic pop
            char buf[INET_ADDRSTRLEN + 1];
            if(inet_ntop(AF_INET, &in, buf, sizeof(buf)) == nullptr)
            {
                // this should just never happen
                //
                throw addr_unexpected_error("inet_ntop() somehow failed with AF_INET and IPv4 address"); // LCOV_EXCL_LINE
            }
            result << buf;
        }
    }

    if((mode & (STRING_IP_PORT | STRING_IP_PORT_NAME)) != 0)
    {
        if((mode & (STRING_IP_ADDRESS | STRING_IP_BRACKET_ADDRESS)) != 0)
        {
            result << ':';
        }
        bool name_available(false);
        if((mode & STRING_IP_PORT_NAME) != 0)
        {
            std::string const service_name(get_port_name());
            if(!service_name.empty())
            {
                name_available = true;
                result << service_name;
            }
        }
        if(!name_available)
        {
            result << get_port();
        }
    }

    if((mode & (STRING_IP_MASK | STRING_IP_BRACKET_MASK | STRING_IP_MASK_AS_ADDRESS | STRING_IP_MASK_IF_NEEDED)) != 0
    && (get_mask_size() != 128 || (mode & STRING_IP_MASK_IF_NEEDED) == 0))
    {
        if(!is_mask_ipv4_compatible())
        {
            throw addr_unexpected_mask("mask is not valid for an IPv4 address");
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        in_addr in = {
            .s_addr = htonl((f_mask[12] << 24) | (f_mask[13] << 16) | (f_mask[14] << 8) | f_mask[15]),
        };
#pragma GCC diagnostic pop
        char buf[INET_ADDRSTRLEN + 1];
        if(inet_ntop(AF_INET, &in, buf, sizeof(buf)) == nullptr)
        {
            // this should just never happen
            //
            throw addr_unexpected_error("inet_ntop() somehow failed with AF_INET and IPv4 address"); // LCOV_EXCL_LINE
        }

        if((mode & (STRING_IP_ADDRESS
                  | STRING_IP_BRACKET_ADDRESS
                  | STRING_IP_PORT
                  | STRING_IP_PORT_NAME)) != 0)
        {
            result << '/';
        }
        int bits(-1);
        if((mode & STRING_IP_MASK_AS_ADDRESS) == 0)
        {
            bits = get_mask_size();
        }
        if(bits == -1)
        {
            result << buf;
        }
        else
        {
            result << (bits - 96);
        }
    }

    return result.str();
}


/** \brief Convert the addr object to a string.
 *
 * This function converts the addr object to a canonicalized string.
 * This can be used to compare two IPv6 together as strings, although
 * it is probably better to compare them using the < and == operators.
 *
 * By default the function returns with the IPv6 address defined
 * between square bracket, so the output of this function can be
 * used as the input of the set_addr_port() function. You may
 * also request the address without the brackets.
 *
 * \exception addr_invalid_state_exception
 * If the binary IP address cannot be converted to ASCII, this exception
 * is raised.
 *
 * \param[in] mode  How the output string is to be built.
 *
 * \return The addr object converted to an IPv6 address.
 */
std::string addr::to_ipv6_string(string_ip_t const mode) const
{
    std::stringstream result;

    bool include_brackets((mode &
                (STRING_IP_BRACKET_ADDRESS
                | STRING_IP_BRACKET_MASK
                | STRING_IP_PORT
                | STRING_IP_PORT_NAME)) != 0);

    if((mode & (STRING_IP_ADDRESS | STRING_IP_BRACKET_ADDRESS)) != 0)
    {
        // insert the IP, even if ANY or "BROADCAST"
        //
        if(include_brackets)
        {
            result << '[';
        }

        if(is_default())
        {
            if((mode & STRING_IP_DEFAULT_AS_ASTERISK) != 0)
            {
                result << "*";
            }
            else if((mode & STRING_IP_DEFAULT_AS_IPV4) != 0)
            {
                include_brackets = false;
                result.str(std::string());
                result << "0.0.0.0";
            }
            else
            {
                // this is exactly what inet_ntop() outputs for the default
                // IPv6 address
                //
                result << "::";
            }
        }
        else
        {
            char buf[INET6_ADDRSTRLEN + 1];
            if(inet_ntop(AF_INET6, &f_address.sin6_addr, buf, sizeof(buf)) == nullptr)
            {
                // this should just never happen
                //
                throw addr_unexpected_error("inet_ntop() somehow failed with AF_INET6 and IPv6 address"); // LCOV_EXCL_LINE
            }

            result << buf;
        }

        if(include_brackets)
        {
            result << ']';
        }
    }

    if((mode & (STRING_IP_PORT | STRING_IP_PORT_NAME)) != 0)
    {
        if((mode & (STRING_IP_ADDRESS | STRING_IP_BRACKET_ADDRESS)) != 0)
        {
            result << ':';
        }
        bool name_available(false);
        if((mode & STRING_IP_PORT_NAME) != 0)
        {
            std::string const service_name(get_port_name());
            if(!service_name.empty())
            {
                name_available = true;
                result << service_name;
            }
        }
        if(!name_available)
        {
            result << get_port();
        }
    }

    if((mode & (STRING_IP_MASK | STRING_IP_BRACKET_MASK | STRING_IP_MASK_AS_ADDRESS | STRING_IP_MASK_IF_NEEDED)) != 0
    && (get_mask_size() != 128 || (mode & STRING_IP_MASK_IF_NEEDED) == 0))
    {
        char buf[INET6_ADDRSTRLEN + 1];
        if(inet_ntop(AF_INET6, f_mask, buf, sizeof(buf)) == nullptr)
        {
            // this should just never happen
            //
            throw addr_unexpected_error("inet_ntop() somehow failed with AF_INET and IPv4 address"); // LCOV_EXCL_LINE
        }

        if((mode & (STRING_IP_ADDRESS
                  | STRING_IP_BRACKET_ADDRESS
                  | STRING_IP_PORT
                  | STRING_IP_PORT_NAME)) != 0)
        {
            result << '/';
        }
        int bits(-1);
        if((mode & STRING_IP_MASK_AS_ADDRESS) == 0)
        {
            bits = get_mask_size();
        }
        if(bits == -1)
        {
            if(include_brackets)
            {
                result << '[';
            }
            result << buf;
            if(include_brackets)
            {
                result << ']';
            }
        }
        else
        {
            result << bits;
        }
    }

    return result.str();
}


/** \brief Return the address as IPv4 or IPv6.
 *
 * Depending on whether the address represents an IPv4 or an IPv6,
 * this function returns the corresponding address. Since the format
 * of both types of addresses can always be distinguished, it poses
 * no concerns.
 *
 * \exception 
 * If include_brackets is false and include_port is true, this
 * exception is raised because we cannot furfill the request.
 *
 * \param[in] mode  How the output string is to be built.
 *
 * \return The addr object converted to an IPv4 or an IPv6 address.
 */
std::string addr::to_ipv4or6_string(string_ip_t mode) const
{
    return is_ipv4() ? to_ipv4_string(mode)
                     : to_ipv6_string(mode);
}


/** \brief Convert the IP address to an unsigned 128 bit ingeter.
 *
 * This function converts the IP address to an integer of 128 bits which
 * supports IPv6 and IPv4.
 *
 * \return The address converted to a unsigned int128 bit integer.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
unsigned __int128 addr::ip_to_uint128() const
{
    // warning: the address is defined in big endian so we want to
    //          swap those bytes
    //
    unsigned __int128 result(0);
    for(std::size_t idx(0); idx < sizeof(f_address.sin6_addr.s6_addr); ++idx)
    {
        result <<= 8;
        result |= static_cast<unsigned __int128>(f_address.sin6_addr.s6_addr[idx]);
    }

    return result;
}
#pragma GCC diagnostic pop


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
void addr::ip_from_uint128(unsigned __int128 u)
{
    std::size_t idx(sizeof(f_address.sin6_addr.s6_addr));
    while(idx > 0)
    {
        --idx;
        f_address.sin6_addr.s6_addr[idx] = static_cast<std::remove_all_extents_t<decltype(f_address.sin6_addr.s6_addr)>>(u);
        u >>= 8;
    }
}
#pragma GCC diagnostic pop


/** \brief Determine the type of network this IP represents.
 *
 * The IP address may represent various type of networks. This
 * function returns that type.
 *
 * The function checks the address either as IPv4 when is_ipv4()
 * returns true, otherwise as IPv6.
 *
 * See:
 *
 * \li https://en.wikipedia.org/wiki/Reserved_IP_addresses
 * \li https://en.wikipedia.org/wiki/IPv6_address
 * \li https://tools.ietf.org/html/rfc3330
 * \li https://tools.ietf.org/html/rfc5735 (IPv4)
 * \li https://tools.ietf.org/html/rfc5156 (IPv6)
 *
 * \todo
 * Look at implementing all the IPv6 types.
 *
 * \return One of the possible network types as defined in the
 *         network_type_t enumeration.
 */
network_type_t addr::get_network_type() const
{
    if(f_private_network == network_type_t::NETWORK_TYPE_UNDEFINED)
    {
        f_private_network = network_type_t::NETWORK_TYPE_UNKNOWN;

        if(is_ipv4())
        {
            // get the address in host order
            //
            // we can use a simple mask + compare to know whether it is
            // this or that once in host order
            //
            uint32_t const host_ip(ntohl(f_address.sin6_addr.s6_addr32[3]));

            if((host_ip & 0xFF000000) == 0x0A000000         // 10.0.0.0/8
            || (host_ip & 0xFFF00000) == 0xAC100000         // 172.16.0.0/12
            || (host_ip & 0xFFFF0000) == 0xC0A80000)        // 192.168.0.0/16
            {
                f_private_network = network_type_t::NETWORK_TYPE_PRIVATE;
            }
            else if((host_ip & 0xFFC00000) == 0x64400000)   // 100.64.0.0/10
            {
                f_private_network = network_type_t::NETWORK_TYPE_CARRIER;
            }
            else if((host_ip & 0xFFFF0000) == 0xA9FE0000)   // 169.254.0.0/16
            {
                f_private_network = network_type_t::NETWORK_TYPE_LINK_LOCAL; // i.e. DHCP
            }
            else if((host_ip & 0xF0000000) == 0xE0000000)   // 224.0.0.0/4
            {
                // there are many sub-groups on this one which are probably
                // still in use...
                //
                f_private_network = network_type_t::NETWORK_TYPE_MULTICAST;
            }
            else if((host_ip & 0xFF000000) == 0x7F000000)   // 127.0.0.0/8
            {
                f_private_network = network_type_t::NETWORK_TYPE_LOOPBACK; // i.e. localhost
            }
            else if(host_ip == 0x00000000)
            {
                f_private_network = network_type_t::NETWORK_TYPE_ANY; // i.e. 0.0.0.0
            }
        }
        else //if(is_ipv6()) -- if not IPv4, we have an IPv6
        {
            // for IPv6 it was simplified by using a prefix for
            // all types; really way easier than IPv4
            //
            if(f_address.sin6_addr.s6_addr32[0] == 0      // ::
            && f_address.sin6_addr.s6_addr32[1] == 0
            && f_address.sin6_addr.s6_addr32[2] == 0
            && f_address.sin6_addr.s6_addr32[3] == 0)
            {
                // this is the "any" IP address
                f_private_network = network_type_t::NETWORK_TYPE_ANY;
            }
            else
            {
                std::uint16_t const prefix(ntohs(f_address.sin6_addr.s6_addr16[0]));

                if((prefix & 0xFF00) == 0xFD00)                 // fd00::/8
                {
                    f_private_network = network_type_t::NETWORK_TYPE_PRIVATE;
                }
                else if((prefix & 0xFFC0) == 0xFE80    // fe80::/10
                     || (prefix & 0xFF0F) == 0xFF02)   // ffx2::/16
                {
                    f_private_network = network_type_t::NETWORK_TYPE_LINK_LOCAL; // i.e. DHCP
                }
                else if((prefix & 0xFF0F) == 0xFF01    // ffx1::/16
                     || (f_address.sin6_addr.s6_addr32[0] == 0      // ::1
                      && f_address.sin6_addr.s6_addr32[1] == 0
                      && f_address.sin6_addr.s6_addr32[2] == 0
                      && f_address.sin6_addr.s6_addr16[6] == 0
                      && f_address.sin6_addr.s6_addr16[7] == htons(1)))
                {
                    f_private_network = network_type_t::NETWORK_TYPE_LOOPBACK;
                }
                else if((prefix & 0xFF00) == 0xFF00)   // ff00::/8
                {
                    // this one must be after the link-local and loopback networks
                    f_private_network = network_type_t::NETWORK_TYPE_MULTICAST;
                }
                else if(prefix == 0x2001)
                {
                    std::uint16_t const next_prefix(ntohs(f_address.sin6_addr.s6_addr16[1]));
                    if(next_prefix == 0x0DB8)
                    {
                        f_private_network = network_type_t::NETWORK_TYPE_DOCUMENTATION;
                    }
                }
                else if(prefix == 0x3FFF)
                {
                    std::uint16_t const next_prefix(ntohs(f_address.sin6_addr.s6_addr16[1]));
                    if((next_prefix & 0xF000) == 0x0000)
                    {
                        f_private_network = network_type_t::NETWORK_TYPE_DOCUMENTATION;
                    }
                }
            }
        }
    }

    return f_private_network;
}


/** \brief Get the network type string
 *
 * Translate the network type into a string, which can be really useful
 * to log that information.
 *
 * Note that PUBLIC is the same as UNKNOWN, this function returns
 * "Unknown" in that case, though.
 *
 * \return The string representing the type of network.
 */
char const * addr::get_network_type_string() const
{
    switch(get_network_type())
    {
    case network_type_t::NETWORK_TYPE_UNDEFINED: // LCOV_EXCL_LINE -- get_network_type() defines it...
        return "Undefined"; // LCOV_EXCL_LINE

    case network_type_t::NETWORK_TYPE_PRIVATE:
        return "Private";

    case network_type_t::NETWORK_TYPE_CARRIER:
        return "Carrier";

    case network_type_t::NETWORK_TYPE_LINK_LOCAL:
        return "Local Link";

    case network_type_t::NETWORK_TYPE_MULTICAST:
        return "Multicast";

    case network_type_t::NETWORK_TYPE_LOOPBACK:
        return "Loopback";

    case network_type_t::NETWORK_TYPE_ANY:
        return "Any";

    case network_type_t::NETWORK_TYPE_DOCUMENTATION:
        return "Documentation";

    case network_type_t::NETWORK_TYPE_UNKNOWN: // == NETWORK_TYPE_PUBLIC
        return "Unknown";

    }
    snapdev::NOT_REACHED(); // LCOV_EXCL_LINE
}


/** \brief Create a socket from the IP address held by this addr object.
 *
 * This function creates a socket that corresponds to the addr object
 * definitions, it takes the protocol and family information in account.
 *
 * The flags can be used to add one or more of the following flags:
 *
 * \li SOCKET_FLAG_NONBLOCK -- create socket as non-block
 * \li SOCKET_FLAG_CLOEXEC -- close socket on an execv()
 * \li SOCKET_FLAG_REUSE -- for TCP socket, mark the address as immediately
 * reusable, ignored for UDP; only useful for server (bind + listen after
 * this call)
 *
 * \note
 * The IP protocol is viewed as TCP in this function.
 *
 * \warning
 * This class does not hold the socket created by this function.
 *
 * \param[in] flags  A set of socket flags to use when creating the socket.
 *
 * \return The socket file descriptor or -1 on errors.
 */
int addr::create_socket(socket_flag_t flags) const
{
    int const sock_flags(
              ((flags & SOCKET_FLAG_CLOEXEC)  != 0 ? SOCK_CLOEXEC  : 0)
            | ((flags & SOCKET_FLAG_NONBLOCK) != 0 ? SOCK_NONBLOCK : 0));
    int const family(is_ipv4() ? AF_INET : AF_INET6);

    switch(f_protocol)
    {
    case IPPROTO_IP: // interpret as TCP...
    case IPPROTO_TCP:
        {
            int s(socket(family, SOCK_STREAM | sock_flags, IPPROTO_TCP));

            if(s >= 0
            && (flags & SOCKET_FLAG_REUSE) != 0)
            {
                // set the "reuse that address immediately" flag, we totally
                // ignore errors on that one
                //
                int optval(1);
                socklen_t const optlen(sizeof(optval));
                static_cast<void>(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, optlen));
            }
            return s;
        }

    case IPPROTO_UDP:
        return socket(family, SOCK_DGRAM | sock_flags, IPPROTO_UDP);

    default:            // LCOV_EXCL_LINE
        // this should never happen since we control the f_protocol field
        //
        return -1;      // LCOV_EXCL_LINE

    }
}


/** \brief Connect the specified socket to this IP address.
 *
 * When you create a TCP client, you can connect to a server. This
 * is done by using the connect() function which makes use of the
 * address to connect to the server.
 *
 * This function makes sure to select the correct connect() function
 * depending on whether this IP address is an IPv4 or an IPv6 address
 * (although we could always try with the IPv6 structure, it may or
 * may not work properly on all systems, so for now we use the
 * distinction.)
 *
 * \param[in] s  The socket to connect to the address.
 *
 * \return 0 if the bind() succeeded, -1 on errors
 */
int addr::connect(int s) const
{
    // only TCP can connect, UDP binds and sends only
    //
    switch(f_protocol)
    {
    case IPPROTO_IP: // interpret as TCP...
    case IPPROTO_TCP:
        if(is_ipv4())
        {
            // this would most certainly work using the IPv6 address
            // as in the else part, but to be sure, we use the IPv4
            // as specified in the address (there could be other reasons
            // than just your OS for this to fail if using IPv6.)
            //
            // IMPORTANT NOTE: also the family is used in the socket()
            //                 call above and must match the address here...
            //
            sockaddr_in ipv4;
            get_ipv4(ipv4);
            return ::connect(s, reinterpret_cast<sockaddr const *>(&ipv4), sizeof(ipv4));
        }
        else
        {
            return ::connect(s, reinterpret_cast<sockaddr const *>(&f_address), sizeof(sockaddr_in6));
        }
        break;

    }

    return -1;
}


/** \brief Create a server with this socket listening on this IP address.
 *
 * This function will bind the socket \p s to the address defined in
 * this addr object. This creates a server listening on that IP address.
 *
 * If the IP address is 127.0.0.1, then only local processes can connect
 * to that server. If the IP address is 0.0.0.0, then anyone can connect
 * to the server.
 *
 * This function works for TCP and UDP servers.
 *
 * If the IP address represents an IPv4 address, then the bind() is done
 * with an IPv4 address and not the IPv6 as it is stored.
 *
 * It is possible to set the port to 0 in which case the network stack
 * will automatically assign a port number to our service. This is
 * particularly useful when dealing with UDP sockets since both sides
 * act like clients and servers, only the client should not have a
 * predefined port. This is where you use a port of 0. At the time
 * the function returns, the port assigned by the system will be written
 * to the addr object by the set_from_socket() function. If you want to
 * reuse the same addr object for another bind(), make sure to reset
 * the port if required in your situation.
 *
 * \param[in] s  The socket to bind to this address.
 *
 * \return 0 if the bind() succeeded, -1 on errors
 */
int addr::bind(int s)
{
    // call the const version which just binds
    int const r(const_cast<addr const *>(this)->bind(s));

    // if the bind() worked and the port is 0, then use the
    // set_from_socket() to determine the auto-assigned port
    //
    if(r == 0
    && get_port() == 0)
    {
        set_from_socket(s, false);
    }

    return r;
}


/** \brief Bind your socket to this address.
 *
 * The constant version of the bind() function does not modify your address.
 * If you used port 0, it will remain port 0. You can see obtain the port
 * with which your address is bound by calling a function such as the
 * set_from_socket() on a different address object.
 *
 * \param[in] s  The socket to attach this address to.
 *
 * \return 0 if the bind() succeeded, -1 on errors
 */
int addr::bind(int s) const
{
    if(is_ipv4())
    {
        sockaddr_in ipv4;
        get_ipv4(ipv4);
        return ::bind(s, reinterpret_cast<sockaddr const *>(&ipv4), sizeof(ipv4));
    }
    else
    {
        return ::bind(s, reinterpret_cast<sockaddr const *>(&f_address), sizeof(sockaddr_in6));
    }
}


/** \brief Send a message over UDP.
 *
 * If you successfully called the create_socket() function on an address
 * using the UDP protocol, you can then send messages using this function.
 *
 * If the address is not defined with the UDP protocol, then the function
 * fails and errno is set to EINVAL.
 *
 * \param[in] s  The socket as opened by create_socket().
 * \param[in] buffer  The buffer with the message to send.
 * \param[in] size  The size of the buffer in bytes.
 *
 * \return the number of bytes sent on success, -1 on error and errno set
 * to the error code.
 *
 * \sa sendto(2)
 */
ssize_t addr::sendto(int s, char const * buffer, std::size_t size) const
{
    if(f_protocol != IPPROTO_UDP)
    {
        errno = EINVAL;
        return -1;
    }

    if(is_ipv4())
    {
        sockaddr_in ipv4;
        get_ipv4(ipv4);
        return ::sendto(
              s
            , buffer
            , size
            , 0
            , reinterpret_cast<sockaddr const *>(&ipv4)
            , sizeof(ipv4));
    }
    else
    {
        return ::sendto(
              s
            , buffer
            , size
            , 0
            , reinterpret_cast<sockaddr const *>(&f_address)
            , sizeof(sockaddr_in6));
    }
}


/** \brief Receive a message over UDP.
 *
 * If you successfully called the create_socket() function on an address
 * using the UDP protocol, you can then receive messages using this function.
 *
 * If the socket was not opened with the UDP protocol, then the function
 * fails and errno is set to EINVAL.
 *
 * \warning
 * This addr object gets updated, first with the socket local information
 * and assuming the recvfrom() succeeds, with the source socket information.
 * So in the end this object has the address of the sender.
 *
 * \warning
 * The function may return 0 for several different reasons: when an empty
 * message was sent, when the source close the socket (orderly shutdown),
 * if the \p size parameter was 0 on entry.
 *
 * \param[in] s  The socket as opened by create_socket().
 * \param[in] buffer  The buffer where the message is saved.
 * \param[in] size  The size of the buffer in bytes.
 *
 * \return the number of bytes received on success, -1 on error and errno set
 * to the error code.
 *
 * \sa recvfrom(2)
 */
ssize_t addr::recvfrom(int s, char * buffer, std::size_t size)
{
    // get the socket family type
    //
    set_from_socket(s, false);

    if(f_protocol != IPPROTO_UDP)
    {
        errno = EINVAL;
        return -1;
    }

    socklen_t length(0);
    int r;
    if(is_ipv4())
    {
        sockaddr_in ipv4;
        length = sizeof(ipv4);
        r = ::recvfrom(
              s
            , buffer
            , size
            , 0
            , reinterpret_cast<sockaddr *>(&ipv4)
            , &length);
        if(r >= 0)
        {
            set_ipv4(ipv4);
        }
    }
    else
    {
        sockaddr_in6 ipv6;
        length = sizeof(ipv6);
        r = ::recvfrom(
              s
            , buffer
            , size
            , 0
            , reinterpret_cast<sockaddr *>(&ipv6)
            , &length);
        if(r >= 0)
        {
            set_ipv6(ipv6);
        }
    }
    return r;
}


/** \brief Set the interface on which to listen.
 *
 * When binding an AF_INET or AF_INET6, we can forcibly bind the socket
 * to a specific interface. This means we won't be able to mistakingly
 * open a port on the wrong interface.
 *
 * \warning
 * This feature requires the service to bind its socket as root, which is
 * a rather strange behavior. Services which are not running as root will
 * ignore this parameter (try to use it and ignore the error).
 *
 * \param[in] interface  The name of the interface to bind to.
 *
 * \sa get_interface()
 */
void addr::set_interface(std::string const & interface)
{
    f_interface = interface;
}


/** \brief Set the corresponding host.
 *
 * When parsing an address, we transform the hostnames to IP addresses.
 *
 * When connecting to a web server via TLS, you need to enter the SNI,
 * which is the server name. To make it available, we use this function
 * to save the hostname as is.
 *
 * \param[in] hostname  The name of the host to connect to.
 *
 * \sa get_hostname()
 */
void addr::set_hostname(std::string const & hostname)
{
    f_hostname = hostname;
}


/** \brief Initializes this addr object from a socket information.
 *
 * When you connect to a server or a clients connect to your server, the
 * socket defines two IP addresses and ports: one on your side and one on
 * the other side.
 *
 * The other side is called the _peer name_.
 *
 * Your side is called the _socket name_ (i.e. the IP address of your
 * computer, representing the interface used to perform that connection).
 *
 * If you call this function with \p peer set to false then you get the
 * address and port from your side. If you set \p peer to true,
 * you get the other side address and port details.
 *
 * \todo
 * Move this to our eventdispatcher once we create that separate library.
 * Probably within a form of low level socket class.
 *
 * \exception addr_invalid_argument
 * The function must be called with a valid socket (positive or 0) or this
 * exception is raised.
 *
 * \exception addr_io_error
 * When the retrieval of the socket address failed, this exception is raised.
 * Assuming the socket is valid, this should never happen.
 *
 * \exception addr_invalid_state
 * The type of addresses supported are INET (IPv4) and INET6 (IPv6). If
 * another type is returned (i.e. you passed a Unix socket), then this
 * exception is raised. Similarly, this exception is raised if the connection
 * is not a TCP or UDP connection.
 *
 * \param[in] s  The socket from which you want to retrieve peer information.
 * \param[in] peer  Whether to retrieve the peer (other side
 * IP:\<ephemeral port>) or socket name (your IP:\<port used to connect>).
 */
void addr::set_from_socket(int s, bool peer)
{
    // make sure the socket is defined and well
    //
    if(s < 0)
    {
        throw addr_invalid_argument("addr::set_from_socket(): the socket cannot be a negative number.");
    }

    sockaddr_storage address = sockaddr_storage();
    socklen_t length(sizeof(address));
    int r;
    if(peer)
    {
        // this retrieves the information from the other side
        //
        r = getpeername(s, reinterpret_cast<sockaddr *>(&address), &length);
    }
    else
    {
        // retrieve the local socket information
        //
        r = getsockname(s, reinterpret_cast<sockaddr *>(&address), &length);
    }
    if(r != 0)
    {
        int const e(errno);
        throw addr_io_error(
                  std::string("addr::set_from_socket(): ")
                + (peer ? "getpeername()" : "getsockname()")
                + " failed to retrieve IP address details (errno: "
                + std::to_string(e)
                + ", "
                + strerror(e)
                + ").");
    }

    switch(address.ss_family)
    {
    case AF_INET:
        set_ipv4(reinterpret_cast<sockaddr_in &>(address));
        break;

    case AF_INET6:
        set_ipv6(reinterpret_cast<sockaddr_in6 &>(address));
        break;

    default:
        throw addr_invalid_state(
                  std::string("addr::set_from_socket(): ")
                + (peer ? "getpeername()" : "getsockname()")
                + " returned a type of address which is not understood, i.e. not AF_INET or AF_INET6.");

    }

    int type(0);
    length = sizeof(type);
    r = getsockopt(s, SOL_SOCKET, SO_TYPE, &type, &length);
    if(r != 0)
    {
        // LCOV_EXCL_START
        int const e(errno);
        throw addr_io_error(
                  std::string("addr::set_from_socket(): ")
                + (peer ? "getpeername()" : "getsockname()")
                + " failed to retrieve IP address details (errno: "
                + std::to_string(e)
                + ", "
                + strerror(e)
                + ").");
        // LCOV_EXCL_STOP
    }

    switch(type)
    {
    case SOCK_STREAM:
        f_protocol = IPPROTO_TCP;
        break;

    case SOCK_DGRAM:
        f_protocol = IPPROTO_UDP;
        break;

    // LCOV_EXCL_START
    default:
        throw addr_invalid_state(
                  "addr::set_from_socket(): getsockopt() returned a type of connection which is not understood,"
                  " i.e. not SOCK_STREAM or SICK_DGRAM.");
    // LCOV_EXCL_STOP

    }
}


/** \brief Transform the IP into a domain name.
 *
 * This function transforms the IP address in this `addr` object in a
 * name such as "snap.website".
 *
 * \note
 * The function does not cache the result because it is rarely used (at least
 * at this time). So you should cache the result and avoid calling this
 * function more than once as the process can be very slow.
 *
 * \todo
 * Speed enhancement can be achieved by using getaddrinfo_a(). That would
 * work with a vector of addr objects.
 *
 * \return The domain name. If not available, an empty string.
 */
std::string addr::get_name() const
{
    char host[NI_MAXHOST];

    int flags(NI_NAMEREQD);
    if(f_protocol == IPPROTO_UDP)
    {
        flags |= NI_DGRAM;
    }

    // TODO: test with the NI_IDN* flags and make sure we know what we get
    //       (i.e. we want UTF-8 as a result)
    //
    int const r(getnameinfo(
              reinterpret_cast<sockaddr const *>(&f_address)
            , sizeof(f_address)
            , host
            , sizeof(host)
            , nullptr
            , 0
            , flags));

    // if return value is 0, then it worked
    //
    return r == 0 ? host : std::string();
}


/** \brief Transform the port into a service name.
 *
 * This function transforms the port in this `addr` object in a
 * name such as "http".
 *
 * \note
 * The function does not cache the result because it is rarely used (at least
 * at this time). So you should cache the result and avoid calling this
 * function more than once as the process is somewhat slow.
 *
 * \warning
 * The getnameinfo() will return a string with a number if it does not
 * know the server (i.e. this is the equivalent to std::to_string() of
 * the port). For port 0, the function always returns an empty string.
 *
 * \return The service name. If not available, an empty string.
 */
std::string addr::get_service() const
{
    if(get_port() == 0)
    {
        return std::string();
    }

    char service[NI_MAXSERV];

    int flags(NI_NAMEREQD);
    if(f_protocol == IPPROTO_UDP)
    {
        flags |= NI_DGRAM;
    }
    int const r(getnameinfo(
              reinterpret_cast<sockaddr const *>(&f_address)
            , sizeof(f_address)
            , nullptr
            , 0
            , service
            , sizeof(service)
            , flags));

    // return value is 0, then it worked
    //
    return r == 0 ? service
                  : std::string();
}


/** \brief Check whether the port was explicitly defined or not.
 *
 * This function returns the flag as set by the set_port_defined(). By
 * default the flag is set to false. It is also set to true if you
 * explicitly call the set_port() function.
 *
 * \return true if the port was explicitly set, false otherwise.
 */
bool addr::is_port_defined() const
{
    return f_port_defined;
}


/** \brief Retrieve the port.
 *
 * This function retrieves the port of the IP address in host order.
 *
 * \return The port defined along this address.
 */
int addr::get_port() const
{
    return ntohs(f_address.sin6_port);
}


/** \brief Convert the port into a name.
 *
 * Many ports are used for specific services. For example, port 80 represents
 * HTTP. This function converts the ports using the /etc/services file.
 *
 * \return The name of the port or an empty string.
 */
std::string addr::get_port_name() const
{
    servent service = {};
    servent * list(nullptr);

    snapdev::static_to_dynamic_buffer<char, 1024> buf;
    std::string const proto(get_protocol_name());
    for(;;)
    {
        int const r(getservbyport_r(
              htons(get_port())
            , proto.c_str()
            , &service
            , buf.get()
            , buf.size()
            , &list));
        // from the example, r may be 0 on a "not found" in which case
        // the list pointer is nullptr
        //
        if(r == 0 && list != nullptr)
        {
            return service.s_name;
        }
        if(r != ERANGE)
        {
            return std::string();
        }
        buf.increase_size(1024);        // LCOV_EXCL_LINE
    }                                   // LCOV_EXCL_LINE
}


/** \brief Retrieve the port.
 *
 * This function retrieves the port of the IP address in host order.
 *
 * \return The port defined along this address as a string.
 */
std::string addr::get_str_port() const
{
    return std::to_string(get_port());
}


/** \brief Whether the protocol was defined.
 *
 * This flag is set to try if the set_protocol_defined() or just
 * set_protocol() functions get called. By default, the protocol is
 * set to "tcp" (IPPROTO_TCP). So you do not have a way to know whether
 * an explicit protocol was defined.
 *
 * This flag allows you to save that information in this object. In most
 * cases, you want to:
 *
 * 1. parse the protocol out of the address,
 * 2. call the address parser
 * 3. save the protocol in the resulting addr objects
 *
 * The set_protocol() of the parser should also be called, but that will
 * not automatically mark
 */
bool addr::is_protocol_defined() const
{
    return f_protocol_defined;
}


/** \brief Retrieve the protocol.
 *
 * This function retrieves the protocol as specified with the last
 * set_protocol() function call. On construction, the protocol is
 * set to IPPROTO_TCP but it is marked as undefined.
 *
 * When you change the protocol with the set_protocol() function, it
 * gets marked as defined.
 *
 * \return protocol such as IPPROTO_TCP or IPPROTO_UDP.
 */
int addr::get_protocol() const
{
    return f_protocol;
}


/** \brief Get the protocol name.
 *
 * A list of protocols is found in the /etc/protocols file. This function
 * transforms the protocol number in one of the names found in that file.
 *
 * If no such name is available, then this function returns an empty string.
 *
 * \return The name of the protocol or an empty string.
 *
 * \sa get_protocol()
 * \sa set_protocol()
 */
std::string addr::get_protocol_name() const
{
    struct protoent proto = {};
    struct protoent * list(nullptr);

    snapdev::static_to_dynamic_buffer<char, 1024> buf;
    for(;;)
    {
        int const r(getprotobynumber_r(
                  f_protocol
                , &proto
                , buf.get()
                , buf.size()
                , &list));
        if(r == 0)
        {
            return proto.p_name;
        }
        // LCOV_EXCL_START
        //
        // at the moment it is not possible to set the protocol
        // to a value that doesn't have a name in /etc/protocols
        //
        if(r != ERANGE)
        {
            return std::string();
        }
        buf.increase_size(1024);
    }
    // LCOV_EXCL_STOP
}


/** \brief Check whether an IP matches a CIDR.
 *
 * When an IP address is defined along a mask, it can match a set of
 * other IP addresses. This function can be used to see whether
 * \p ip matches \p this IP address and mask.
 *
 * So in other words, the mask of `this` addr object is used to mask
 * both, `this` and `p` before comparing the masked result.
 *
 * \warning
 * This function only checks the IP address. It totally ignores the
 * port, family, protocol and other peripheral details.
 *
 * \param[in] ip  The address to match against this IP/mask CIDR.
 * \param[in] any  If `this` addr object is an ANY address (see is_default())
 * then always return true.
 *
 * \return true if \p ip is a match.
 *
 * \sa is_default()
 */
bool addr::match(addr const & ip, bool any) const
{
    if(any
    && is_default())
    {
        return true;
    }

    for(int idx(0); idx < 16; ++idx)
    {
        if((f_address.sin6_addr.s6_addr[idx] & f_mask[idx]) != (ip.f_address.sin6_addr.s6_addr[idx] & f_mask[idx]))
        {
            return false;
        }
    }

    return true;
}


/** \brief Determine whether addresses are adjacent.
 *
 * This function returns true if the specified address (\p a) is the very
 * next address of `this` address.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's.
 *
 * \param[in] a  The address to check against.
 *
 * \return true if this address + 1 equal \p a.
 */
bool addr::is_next(addr const & a) const
{
    using namespace snapdev::literals;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    unsigned __int128 lhs(ip_to_uint128());
    unsigned __int128 rhs(a.ip_to_uint128());
#pragma GCC diagnostic pop
    return lhs != 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_uint128 && lhs + 1 == rhs;
}


/** \brief Determine whether addresses are adjacent.
 *
 * This function returns true if the specified address (\p a) is the very
 * previous address of `this` address.
 *
 * \note
 * The addresses do not wrap around. So the address with all f's is not
 * just before the default address.
 *
 * \param[in] a  The address to check against.
 *
 * \return true if this address - 1 equal \p a.
 */
bool addr::is_previous(addr const & a) const
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    unsigned __int128 lhs(ip_to_uint128());
    unsigned __int128 rhs(a.ip_to_uint128());
#pragma GCC diagnostic pop
    return lhs != 0 && lhs - 1 == rhs;
}


/** \brief Check whether two addresses are equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) for equality. If both represent the same IP
 * address, then the function returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is equal to \p rhs.
 */
bool addr::operator == (addr const & rhs) const
{
    return f_address.sin6_addr == rhs.f_address.sin6_addr;
}


/** \brief Check whether two addresses are not equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) for inequality. If both represent the same IP
 * address, then the function returns false.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is not equal to \p rhs.
 */
bool addr::operator != (addr const & rhs) const
{
    return f_address.sin6_addr != rhs.f_address.sin6_addr;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr::operator < (addr const & rhs) const
{
    return f_address.sin6_addr < rhs.f_address.sin6_addr;
}


/** \brief Compare two addresses to know which one is smaller or equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know whether the left hand side is smaller or
 * equal to thr right handside.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr::operator <= (addr const & rhs) const
{
    return f_address.sin6_addr <= rhs.f_address.sin6_addr;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr::operator > (addr const & rhs) const
{
    return f_address.sin6_addr > rhs.f_address.sin6_addr;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr::operator >= (addr const & rhs) const
{
    return f_address.sin6_addr >= rhs.f_address.sin6_addr;
}


/** \brief Compute the next IP address.
 *
 * This function adds one to this IP address. In many cases, this may have
 * no meaning. It is, however, useful to go through all the IP addresses
 * of a range.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \return A reference to this addr object.
 */
addr & addr::operator ++ ()
{
    using namespace snapdev::literals;

    ip_from_uint128(snapdev::saturated_add(ip_to_uint128(), 1_uint128));
    return *this;
}


/** \brief Compute the next IP address.
 *
 * This function adds one to this IP address and returns a copy of the result.
 * In many cases, this may have no meaning. It is, however, useful to go
 * through all the IP addresses of a range.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \return A copy of this addr object representing the next IP address.
 */
addr addr::operator ++ (int)
{
    using namespace snapdev::literals;

    addr result(*this);
    ip_from_uint128(snapdev::saturated_add(ip_to_uint128(), 1_uint128));
    return result;
} // LCOV_EXCL_LINE


/** \brief Compute the previous IP address.
 *
 * This function subtracts one to this IP address. In many cases, this may
 * have no meaning. It is, however, useful to go through all the IP addresses
 * of a range.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \return A reference to this addr object.
 */
addr & addr::operator -- ()
{
    using namespace snapdev::literals;

    ip_from_uint128(snapdev::saturated_subtract(ip_to_uint128(), 1_uint128));
    return *this;
}


/** \brief Compute the previous IP address.
 *
 * This function subtracts one from this IP address and returns a copy of
 * the result. In many cases, this may have no meaning. It is, however,
 * useful to go through all the IP addresses of a range starting from the
 * end of the range.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \return A copy of this addr object representing the next IP address.
 */
addr addr::operator -- (int)
{
    using namespace snapdev::literals;

    addr result(*this);
    ip_from_uint128(snapdev::saturated_subtract(ip_to_uint128(), 1_uint128));
    return result;
} // LCOV_EXCL_LINE


/** \brief Add an offset to this IP address.
 *
 * This function adds the specified \p offset to this IP address. In many
 * cases, this may have no meaning. It can be useful to compute a range of
 * IP addresses.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \param[in] offset  The offset to add to this IP address.
 *
 * \return A copy of this addr object plus the specified \p offset.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
addr addr::operator + (int offset) const
{
    addr result(*this);
    if(offset < 0)
    {
        result.ip_from_uint128(snapdev::saturated_subtract(ip_to_uint128(), static_cast<unsigned __int128>(static_cast<unsigned int>(-offset))));
    }
    else
    {
        result.ip_from_uint128(snapdev::saturated_add(ip_to_uint128(), static_cast<unsigned __int128>(offset)));
    }
    return result;
} // LCOV_EXCL_LINE
#pragma GCC diagnostic pop


/** \brief Subtract an offset to this IP address.
 *
 * This function subtracts the specified \p offset to this IP address. In many
 * cases, this may have no meaning. It can be useful to compute a range of
 * IP addresses.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \param[in] offset  The offset to subtract from this IP address.
 *
 * \return A copy of this addr object minus the specified \p offset.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
addr addr::operator - (int offset) const
{
    addr result(*this);
    if(offset < 0)
    {
        result.ip_from_uint128(snapdev::saturated_add(ip_to_uint128(), static_cast<unsigned __int128>(static_cast<unsigned int>(-offset))));
    }
    else
    {
        result.ip_from_uint128(snapdev::saturated_subtract(ip_to_uint128(), static_cast<unsigned __int128>(offset)));
    }
    return result;
} // LCOV_EXCL_LINE
#pragma GCC diagnostic pop


/** \brief Compute the distance between two IP addresses.
 *
 * This function computes the distance between two IP addresses. This distance
 * can be used to define the size of a range of addresses (i.e. "to - from + 1").
 *
 * In case of a from/to range distance, the difference will always be positive
 * if you do:
 *
 * \code
 *     __int128 distance = r.get_to() - r.get_from() + 1;
 * \endcode
 *
 * However, the distance can be negative when the right handside is a larger
 * address than the left handside.
 *
 * \warning
 * The function returns a signed __int128 number. This means a large `to`
 * minus a small `from` can result in an invalid distance (i.e. an overflow
 * can occur).
 *
 * \param[in] rhs  The right handside address to subtract from this address.
 *
 * \return The difference between this address and \p rhs.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
__int128 addr::operator - (addr const & rhs) const
{
    return static_cast<__int128>(ip_to_uint128() - rhs.ip_to_uint128());
}
#pragma GCC diagnostic push


/** \brief Add an offset to this IP address.
 *
 * This function adds the specified \p offset to this IP address. In many
 * cases, this may have no meaning. It can be useful to compute a range of
 * IP addresses or compute the next or previous address with a step other
 * than 1.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \param[in] offset  The offset to add to this IP address.
 *
 * \return A reference to this IP address.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
addr & addr::operator += (int offset)
{
    if(offset < 0)
    {
        ip_from_uint128(snapdev::saturated_subtract(ip_to_uint128(), static_cast<unsigned __int128>(static_cast<unsigned int>(-offset))));
    }
    else
    {
        ip_from_uint128(snapdev::saturated_add(ip_to_uint128(), static_cast<unsigned __int128>(offset)));
    }
    return *this;
}
#pragma GCC diagnostic pop


/** \brief Subtract an offset to this IP address.
 *
 * This function subtracts the specified \p offset to this IP address. In many
 * cases, this may have no meaning. It can be useful to compute a range of
 * IP addresses or compute the next or previous address with a step other
 * than -1.
 *
 * \warning
 * The function does not check wether the address is an IPv4 or IPv6. It
 * manages the address as one large 128 bit number.
 *
 * \note
 * The addresses do not wrap around. So the default address (all 0's) is not
 * just after the address with all f's. That also means there is no next
 * address after the all f's address and there is no previous address before
 * all 0's.
 *
 * \param[in] offset  The offset to add to this IP address.
 *
 * \return A reference to this IP address.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
addr & addr::operator -= (int offset)
{
    if(offset < 0)
    {
        ip_from_uint128(snapdev::saturated_add(ip_to_uint128(), static_cast<unsigned __int128>(static_cast<unsigned int>(-offset))));
    }
    else
    {
        ip_from_uint128(snapdev::saturated_subtract(ip_to_uint128(), static_cast<unsigned __int128>(offset)));
    }
    return *this;
}
#pragma GCC diagnostic pop


/** \brief Mark that the address changed.
 *
 * This functions makes sure that some of the parameters being cached
 * get reset in such a way that checking the cache will again return
 * the correct answer.
 *
 * \sa get_network_type()
 */
void addr::address_changed()
{
    f_private_network = network_type_t::NETWORK_TYPE_UNDEFINED;
}


/** \brief Retrieve the ios_base index for the addr class.
 *
 * In order to allow for flags specific to the addr class in ostream
 * functions, we need an index which this function supplies. The index
 * is allocated whenever you first use one of the addr ostream functions.
 *
 * \return The unique ostream index for the addr class.
 */
int get_ostream_index()
{
    cppthread::guard lock(*cppthread::g_system_mutex);

    if(!g_ostream_index_allocated)
    {
        g_ostream_index_allocated = true;
        g_ostream_index = std::ios_base::xalloc();
    }

    return g_ostream_index;
}


}
// namespace addr
// vim: ts=4 sw=4 et
