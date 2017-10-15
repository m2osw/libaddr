// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
//
// http://snapwebsites.org/project/libaddr
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief The implementation of the addr class.
 *
 * This file includes the implementation of the addr class. The one that
 * deals with low level classes.
 */

// self
//
#include "libaddr/addr.h"
#include "libaddr/addr_exceptions.h"

// C++ library
//
//#include <algorithm>
#include <sstream>
#include <iostream>

// C library
//
#include <ifaddrs.h>
#include <netdb.h>



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


// IPv4
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct in_addr {
	__be32	s_addr;
};


// IPv6
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // address family, AF_INET6
    u_int16_t       sin6_port;     // port number, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // Scope ID
};

struct in6_addr
  {
    union
      {
	uint8_t	__u6_addr8[16];
#ifdef __USE_MISC
	uint16_t __u6_addr16[8];
	uint32_t __u6_addr32[4];
#endif
      } __in6_u;
#define s6_addr			__in6_u.__u6_addr8
#ifdef __USE_MISC
# define s6_addr16		__in6_u.__u6_addr16
# define s6_addr32		__in6_u.__u6_addr32
#endif
  };


*/


/** \brief Details used by the addr class implementation.
 *
 * We have a function to check whether an address is part of
 * the interfaces of your computer. This check requires the
 * use of a `struct ifaddrs` and as such it requires to
 * delete that structure. We define a deleter for that
 * strucure here.
 */
namespace
{

/** \brief Delete an ifaddrs structure.
 *
 * This deleter is used to make sure all the ifaddrs get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ia  The ifaddrs structure to free.
 */
void ifaddrs_deleter(struct ifaddrs * ia)
{
    freeifaddrs(ia);
}


}
// no name namespace


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
addr::addr(struct sockaddr_in const & in)
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
addr::addr(struct sockaddr_in6 const & in6)
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
void addr::set_ipv4(struct sockaddr_in const & in)
{
    if(in.sin_family != AF_INET)
    {
        // although we convert the IPv4 to an IPv6 below, we really only
        // support AF_INET on entry
        //
        throw addr_invalid_argument_exception("addr::set_ipv4(): the input address does not represent an IPv4 address (family is not AF_INET).");
    }

    // reset the address first
    memset(&f_address, 0, sizeof(f_address));

    // then transform the IPv4 to an IPv6
    //
    // Note: this is not an IPv6 per se, it is an IPv4 mapped within an
    //       IPv6 and your network anwway stack needs to support IPv4
    //       in order to use that IP...
    //
    f_address.sin6_family = AF_INET6;
    f_address.sin6_port = in.sin_port;
    f_address.sin6_addr.s6_addr16[5] = 0xFFFF;
    f_address.sin6_addr.s6_addr32[3] = in.sin_addr.s_addr;

    address_changed();
}


/** \brief Set the port of this address.
 *
 * This function changes the port of this address to \p port.
 *
 * \exception addr_invalid_argument_exception
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
        throw addr_invalid_argument_exception("port to set_port() cannot be out of the allowed range [0..65535].");
    }
    f_address.sin6_port = htons(port);
}


/** \brief Change the protocol using a string.
 *
 * This function is used to change the current protocol defined in
 * this addr object.
 *
 * \exception addr_invalid_argument_exception
 * We currently support "tcp", "udp", and "ip". Any other protocol
 * name generates this exception.
 *
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or "ip")
 */
void addr::set_protocol(char const * protocol)
{
    if(protocol == nullptr)
    {
        throw addr_invalid_argument_exception("protocol pointer to set_protocol() cannot be a nullptr.");
    }

    if(strcmp(protocol, "ip") == 0)
    {
        f_protocol = IPPROTO_IP;
    }
    else if(strcmp(protocol, "tcp") == 0)
    {
        f_protocol = IPPROTO_TCP;
    }
    else if(strcmp(protocol, "udp") == 0)
    {
        f_protocol = IPPROTO_UDP;
    }
    else
    {
        throw addr_invalid_argument_exception(
                          std::string("unknown protocol \"")
                        + protocol
                        + "\", expected \"tcp\" or \"udp\" (string).");
    }

    address_changed();
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
 * \exception addr_invalid_argument_exception
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
        f_protocol = protocol;
        break;

    default:
        throw addr_invalid_argument_exception(
                          "unknown protocol number "
                        + std::to_string(protocol)
                        + ", expected \"tcp\" ("
                        + std::to_string(static_cast<int>(IPPROTO_TCP))
                        + ") or \"udp\" ("
                        + std::to_string(static_cast<int>(IPPROTO_UDP))
                        + ") (numeric).");

    }
}


/** \brief Set the mask.
 *
 * The input mask must be exactly 16 bytes. If you are dealing with an
 * IPv4, make sure the first 12 bytes are 255.
 *
 * \param[in] mask  The mask to save in this address.
 */
void addr::set_mask(uint8_t const * mask)
{
    memcpy(f_mask, mask, sizeof(f_mask));
}


/** \brief Apply the mask to the IP address.
 *
 * This function applies the mask to this address IP address. This means
 * the bits that are 0 in the mask will also be 0 in the address once
 * the function returns.
 *
 * This should be called if you are trying to canonicalize an IP/mask.
 */
void addr::apply_mask()
{
    for(int idx(0); idx < 16; ++idx)
    {
        f_address.sin6_addr.s6_addr[idx] &= f_mask[idx];
    }
}


/** \brief Get the mask.
 *
 * The output buffer for the mask must be at least 16 bytes. If you are
 * dealing with an IPv4, all the bytes are expected to be 255 except
 * the bottom 4 bytes (offset 12, 13, 14, 15).
 *
 * \param[out] mask  The buffer where the mask gets copied.
 */
void addr::get_mask(uint8_t * mask)
{
    memcpy(mask, f_mask, sizeof(f_mask));
}


/** \brief Check whether this address represents an IPv4 address.
 *
 * The IPv6 format supports embedding IPv4 addresses. This function
 * returns true if the embedded address is an IPv4. When this function
 * returns true, the get_ipv4() can be called. Otherwise, the get_ipv4()
 * function throws an error.
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
 * \exception addr_invalid_structure_exception
 * This exception is raised if the address is not an IPv4 address.
 *
 * \param[out] in  The structure where the IPv4 Internet address gets saved.
 */
void addr::get_ipv4(struct sockaddr_in & in) const
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

    throw addr_invalid_state_exception("Not an IPv4 compatible address.");
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
void addr::set_ipv6(struct sockaddr_in6 const & in6)
{
    if(in6.sin6_family != AF_INET6)
    {
        throw addr_invalid_argument_exception("addr::set_ipv6(): the input address does not represent an IPv6 address (family is not AF_INET6).");
    }
    memcpy(&f_address, &in6, sizeof(in6));

    address_changed();
}


/** \brief Retrieve a copy of this addr IP address.
 *
 * This function returns the current IP address saved in this
 * addr object. The IP may represent an IPv4 address in which
 * case the is_ipv4() returns true.
 *
 * \param[out] in6  The structure where the address gets saved.
 */
void addr::get_ipv6(struct sockaddr_in6 & in6) const
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
 * \li ip_string_t::IP_STRING_ONLY -- only the IP address
 * \li ip_string_t::IP_STRING_PORT -- the IP and port
 * \li ip_string_t::IP_STRING_MASK -- the IP and mask
 * \li ip_string_t::IP_STRING_ALL -- the IP, port, and mask
 *
 * The ip_string_t::IP_STRING_BRACKET is viewed as
 * ip_string_t::IP_STRING_ONLY.
 *
 * The ip_string_t::IP_STRING_BRACKET_MASK is viewed as
 * ip_string_t::IP_STRING_MASK.
 *
 * \exception addr_invalid_state_exception
 * If the addr object does not currently represent an IPv4 then
 * this exception is raised.
 *
 * \param[in] mode  How the output string is to be built.
 */
std::string addr::to_ipv4_string(string_ip_t mode) const
{
    if(is_ipv4())
    {
        // this is an IPv4 mapped in an IPv6, "unmap" that IP
        // so the inet_ntop() can correctly generate an output IP
        //
        struct in_addr in;
        memset(&in, 0, sizeof(in));
        in.s_addr = f_address.sin6_addr.s6_addr32[3];
        char buf[INET_ADDRSTRLEN + 1];
        if(inet_ntop(AF_INET, &in, buf, sizeof(buf)) != nullptr)
        {
            if(mode != string_ip_t::STRING_IP_ONLY)
            {
                std::stringstream result;
                result << buf;
                if(mode == string_ip_t::STRING_IP_PORT
                || mode == string_ip_t::STRING_IP_ALL)
                {
                    result << ":";
                    result << ntohs(f_address.sin6_port);
                }
                if(mode == string_ip_t::STRING_IP_MASK
                || mode == string_ip_t::STRING_IP_BRACKETS_MASK
                || mode == string_ip_t::STRING_IP_ALL)
                {
                    memset(&in, 0, sizeof(in));
                    in.s_addr = htonl((f_mask[12] << 24) | (f_mask[13] << 16) | (f_mask[14] << 8) | f_mask[15]);
                    if(inet_ntop(AF_INET, &in, buf, sizeof(buf)) != nullptr)
                    {
                        result << "/";
                        result << buf; // TODO: convert to simple number if possible
                    }
                }
                return result.str();
            }
            return std::string(buf);
        }
        // IPv4 should never fail converting the address unless the
        // buffer was too small...
    }

    throw addr_invalid_state_exception("Not an IPv4 compatible address.");
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
 * \exception addr_invalid_argument_exception
 * If the binary IP address cannot be converted to ASCII, this exception
 * is raised.
 *
 * \param[in] mode  How the output string is to be built.
 *
 * \return The addr object converted to an IPv6 address.
 */
std::string addr::to_ipv6_string(string_ip_t mode) const
{
    char buf[INET6_ADDRSTRLEN + 1];
    if(inet_ntop(AF_INET6, &f_address.sin6_addr, buf, sizeof(buf)) != nullptr)
    {
        bool const include_brackets(mode == string_ip_t::STRING_IP_BRACKETS
                                 || mode == string_ip_t::STRING_IP_BRACKETS_MASK
                                 || mode == string_ip_t::STRING_IP_PORT // port requires us to add brackets
                                 || mode == string_ip_t::STRING_IP_ALL);

        std::stringstream result;

        // always insert the IP, even if ANY or "BROADCAST"
        //
        if(include_brackets)
        {
            result << "[";
        }
        result << buf;
        if(include_brackets)
        {
            result << "]";
        }

        // got a port?
        //
        if(mode == string_ip_t::STRING_IP_PORT
        || mode == string_ip_t::STRING_IP_ALL)
        {
            result << ":";
            result << ntohs(f_address.sin6_port);
        }

        // got a mask?
        //
        if(mode == string_ip_t::STRING_IP_MASK
        || mode == string_ip_t::STRING_IP_BRACKETS_MASK
        || mode == string_ip_t::STRING_IP_ALL)
        {
            if(inet_ntop(AF_INET6, f_mask, buf, sizeof(buf)) != nullptr)
            {
                result << "/";
                if(include_brackets)
                {
                    result << "[";
                }
                result << buf; // TODO: convert to simple number if possible
                if(include_brackets)
                {
                    result << "]";
                }
            }
        }

        return result.str();
    }

    throw addr_invalid_argument_exception("The address from this addr could not be converted to a valid canonicalized IPv6 address.");  // LCOV_EXCL_LINE
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
 * \li https://tools.ietf.org/html/rfc3330
 * \li https://tools.ietf.org/html/rfc5735 (IPv4)
 * \li https://tools.ietf.org/html/rfc5156 (IPv6)
 *
 * \return One of the possible network types as defined in the
 *         network_type_t enumeration.
 */
addr::network_type_t addr::get_network_type() const
{
    if(f_private_network_defined == network_type_t::NETWORK_TYPE_UNDEFINED)
    {
        f_private_network_defined = network_type_t::NETWORK_TYPE_UNKNOWN;

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
                f_private_network_defined = network_type_t::NETWORK_TYPE_PRIVATE;
            }
            else if((host_ip & 0xFFC00000) == 0x64400000)   // 100.64.0.0/10
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_CARRIER;
            }
            else if((host_ip & 0xFFFF0000) == 0xA9FE0000)   // 169.254.0.0/16
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_LINK_LOCAL; // i.e. DHCP
            }
            else if((host_ip & 0xF0000000) == 0xE0000000)   // 224.0.0.0/4
            {
                // there are many sub-groups on this one which are probably
                // still in use...
                //
                f_private_network_defined = network_type_t::NETWORK_TYPE_MULTICAST;
            }
            else if((host_ip & 0xFF000000) == 0x7F000000)   // 127.0.0.0/8
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_LOOPBACK; // i.e. localhost
            }
            else if(host_ip == 0x00000000)
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_ANY; // i.e. 0.0.0.0
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
                f_private_network_defined = network_type_t::NETWORK_TYPE_ANY;
            }
            else
            {
                uint16_t const prefix(ntohs(f_address.sin6_addr.s6_addr16[0]));

                if((prefix & 0xFF00) == 0xFD00)                 // fd00::/8
                {
                    f_private_network_defined = network_type_t::NETWORK_TYPE_PRIVATE;
                }
                else if((prefix & 0xFFC0) == 0xFE80    // fe80::/10
                     || (prefix & 0xFF0F) == 0xFF02)   // ffx2::/16
                {
                    f_private_network_defined = network_type_t::NETWORK_TYPE_LINK_LOCAL; // i.e. DHCP
                }
                else if((prefix & 0xFF0F) == 0xFF01    // ffx1::/16
                     || (f_address.sin6_addr.s6_addr32[0] == 0      // ::1
                      && f_address.sin6_addr.s6_addr32[1] == 0
                      && f_address.sin6_addr.s6_addr32[2] == 0
                      && f_address.sin6_addr.s6_addr16[6] == 0
                      && f_address.sin6_addr.s6_addr16[7] == htons(1)))
                {
                    f_private_network_defined = network_type_t::NETWORK_TYPE_LOOPBACK;
                }
                else if((prefix & 0xFF00) == 0xFF00)   // ff00::/8
                {
                    // this one must be after the link-local and loopback networks
                    f_private_network_defined = network_type_t::NETWORK_TYPE_MULTICAST;
                }
            }
        }
    }

    return f_private_network_defined;
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
std::string addr::get_network_type_string() const
{
    std::string name;
    switch( get_network_type() )
    {
    case addr::network_type_t::NETWORK_TYPE_UNDEFINED  : name= "Undefined";  break; // LCOV_EXCL_LINE -- get_network_type() defines it...
    case addr::network_type_t::NETWORK_TYPE_PRIVATE    : name= "Private";    break;
    case addr::network_type_t::NETWORK_TYPE_CARRIER    : name= "Carrier";    break;
    case addr::network_type_t::NETWORK_TYPE_LINK_LOCAL : name= "Local Link"; break;
    case addr::network_type_t::NETWORK_TYPE_MULTICAST  : name= "Multicast";  break;
    case addr::network_type_t::NETWORK_TYPE_LOOPBACK   : name= "Loopback";   break;
    case addr::network_type_t::NETWORK_TYPE_ANY        : name= "Any";        break;
    case addr::network_type_t::NETWORK_TYPE_UNKNOWN    : name= "Unknown";    break; // == NETWORK_TYPE_PUBLIC
    }
    return name;
}


/** \brief Retrieve the interface name
 *
 * This function retrieves the name of the interface of the address.
 * This is set using the get_local_addresses() static method.
 */
std::string addr::get_iface_name() const
{
    return f_iface_name;
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
 * \todo
 * Move this to our libsnapnetwork once we create that separate library.
 * Probably within a form of low level socket class.
 *
 * \param[in] flags  A set of socket flags to use when creating the socket.
 * \param[in] reuse_address  Set the reuse address flag.
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

    default:
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
 * \todo
 * Move this to our libsnapnetwork once we create that separate library.
 * Probably within a form of low level socket class.
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
            return ::connect(s, reinterpret_cast<sockaddr const *>(&f_address), sizeof(struct sockaddr_in6));
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
 * If the IP address represents an IPv4 addressm then the bind() is done
 * with an IPv4 address and not the IPv6 as it is stored.
 *
 * \todo
 * Move this to our libsnapnetwork once we create that separate library.
 * Probably within a form of low level socket class.
 *
 * \param[in] s  The socket to bind to this address.
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
        return ::bind(s, reinterpret_cast<sockaddr const *>(&f_address), sizeof(struct sockaddr_in6));
    }
}


/** \brief Initializes this addr object from a socket information.
 *
 * When you connect to a server or a clients connect to your server, the
 * socket defines two IP addresses and ports: one on your side and one on
 * the other side.
 *
 * The other side is called the _peer name_.
 *
 * You side is called the _socket name_ (i.e. the IP address of your computer,
 * representing the interface used to perform that connection.)
 *
 * If you call this function with \p peer set to false then you get the
 * address and port from your side. If you set \p peer to true,
 * you get the other side address and port details.
 *
 * \todo
 * Move this to our libsnapnetwork once we create that separate library.
 * Probably within a form of low level socket class.
 *
 * \param[in] s  The socket from which you want to retrieve peer information.
 * \param[in] peer  Whether to retrieve the peer or socket name.
 */
void addr::set_from_socket(int s, bool peer)
{
    // make sure the socket is defined and well
    //
    if(s < 0)
    {
        throw addr_invalid_argument_exception("addr::set_from_socket(): the socket cannot be a negative number.");
    }

    struct sockaddr_storage address = sockaddr_storage();
    socklen_t length(sizeof(address));
    int r;
    if(peer)
    {
        // this retrieves the information from the other side
        //
        r = getpeername(s, reinterpret_cast<struct sockaddr *>(&address), &length);
    }
    else
    {
        // retrieve the local socket information
        //
        r = getsockname(s, reinterpret_cast<struct sockaddr *>(&address), &length);
    }
    if(r != 0)
    {
        int const e(errno);
        throw addr_io_exception(
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
        set_ipv4(reinterpret_cast<struct sockaddr_in &>(address));
        break;

    case AF_INET6:
        set_ipv6(reinterpret_cast<struct sockaddr_in6 &>(address));
        break;

    default:
        throw addr_invalid_state_exception(
                  std::string("addr::set_from_socket(): ")
                + (peer ? "getpeername()" : "getsockname()")
                + " returned a type of address, which is not understood, i.e. not AF_INET or AF_INET6.");

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
    int const r(getnameinfo(reinterpret_cast<sockaddr const *>(&f_address), sizeof(f_address), host, sizeof(host), nullptr, 0, flags));

    // return value is 0, then it worked
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
 * the port.) For port 0, the function always returns an empty string.
 *
 * \return The service name. If not available, an empty string.
 */
std::string addr::get_service() const
{
    if(f_address.sin6_port == 0)
    {
        return std::string();
    }

    char service[NI_MAXSERV];

    int flags(NI_NAMEREQD);
    if(f_protocol == IPPROTO_UDP)
    {
        flags |= NI_DGRAM;
    }
    int const r(getnameinfo(reinterpret_cast<sockaddr const *>(&f_address), sizeof(f_address), nullptr, 0, service, sizeof(service), flags));

    // return value is 0, then it worked
    //
    return r == 0 ? service
                  : std::string();
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


/** \brief Retrieve the protocol.
 *
 * This function retrieves the protocol as specified on the
 * set_addr_port() function or corresponding constructor.
 *
 * You may change the protocol with the set_protocol() function.
 *
 * \return protocol such as IPPROTO_TCP or IPPROTO_UDP.
 */
int addr::get_protocol() const
{
    return f_protocol;
}


/** \brief Check whether an IP matches a CIDR.
 *
 * When an IP address is defined along a mask, it can match a set of
 * other IP addresses. This function can be used to see whether
 * \p ip matches \p this IP address and mask.
 *
 * \warning
 * This function only checks the IP address. It totally ignores the
 * port, family, protocol and other peripheral details.
 *
 * \param[in] ip  The address to match against this IP/mask CIDR.
 *
 * \return true if \p ip is a match.
 */
bool addr::match(addr const & ip) const
{
    for(int idx(0); idx < 16; ++idx)
    {
        if((f_address.sin6_addr.s6_addr[idx] & f_mask[idx]) != (ip.f_address.sin6_addr.s6_addr[idx] & f_mask[idx]))
        {
            return false;
        }
    }

    return true;
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
    f_private_network_defined = network_type_t::NETWORK_TYPE_UNDEFINED;
}


/** \brief Return a list of local addresses on this machine.
 *
 * Peruse the list of available interfaces, and return any detected ip addresses
 * in a vector.
 *
 * \return A vector of all the local interface IP addresses.
 */
addr::vector_t addr::get_local_addresses()
{
    // get the list of interface addresses
    //
    struct ifaddrs * ifa_start(nullptr);
    if(getifaddrs(&ifa_start) != 0)
    {
        // TODO: Should this throw, or just return an empty list quietly?
        //
        return vector_t(); // LCOV_EXCL_LINE
    }

    std::shared_ptr<struct ifaddrs> auto_free(ifa_start, ifaddrs_deleter);

    vector_t addr_list;
    for(struct ifaddrs * ifa(ifa_start); ifa != nullptr; ifa = ifa->ifa_next)
    {
        if( ifa->ifa_addr == nullptr ) continue;

        addr the_address;

        the_address.f_iface_name = ifa->ifa_name;
        uint16_t const family( ifa->ifa_addr->sa_family );
        if( family == AF_INET )
        {
            the_address.set_ipv4( *(reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)) );
        }
        else if( family == AF_INET6 )
        {
            the_address.set_ipv6( *(reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)) );
        }
        else
        {
            // TODO: can we just ignore invalid addresses?
            //throw addr_invalid_structure_exception( "Unknown address family!" );
            continue;
        }

        addr_list.push_back( the_address );
    }

    return addr_list;
}


/** \brief Check whether this address represents this computer.
 *
 * This function reads the addresses as given to us by the getifaddrs()
 * function. This is a system function that returns a complete list of
 * all the addresses this computer is managing / represents. In other
 * words, a list of address that other computers can use to connect
 * to this computer (assuming proper firewall, of course.)
 *
 * \warning
 * The list of addresses from getifaddrs() is not being cached. So you
 * probably do not want to call this function in a loop. That being
 * said, I still would imagine that retrieving that list is fast.
 *
 * \return a computer_interface_address_t enumeration: error, true, or
 *         false at this time; on error errno should be set to represent
 *         what the error was.
 */
addr::computer_interface_address_t addr::is_computer_interface_address() const
{
    // TBD: maybe we could cache the ifaddrs for a small amount of time
    //      (i.e. 1 minute) so additional calls within that time
    //      can go even faster?
    //

    // get the list of interface addresses
    //
    struct ifaddrs * ifa_start(nullptr);
    if(getifaddrs(&ifa_start) != 0)
    {
        return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_ERROR; // LCOV_EXCL_LINE
    }
    std::shared_ptr<struct ifaddrs> auto_free(ifa_start, ifaddrs_deleter);

    bool const ipv4(is_ipv4());
    uint16_t const family(ipv4 ? AF_INET : AF_INET6);
    for(struct ifaddrs * ifa(ifa_start); ifa != nullptr; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr != nullptr
        && ifa->ifa_addr->sa_family == family)
        {
            if(ipv4)
            {
                // the interface address structure is a 'struct sockaddr_in'
                //
                if(memcmp(&reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)->sin_addr,
                            f_address.sin6_addr.s6_addr32 + 3, //&reinterpret_cast<struct sockaddr_in const *>(&f_address)->sin_addr,
                            sizeof(struct in_addr)) == 0)
                {
                    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE;
                }
            }
            else
            {
                // the interface address structure is a 'struct sockaddr_in6'
                //
                if(memcmp(&reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)->sin6_addr, &f_address.sin6_addr, sizeof(f_address.sin6_addr)) == 0)
                {
                    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE;
                }
            }
        }
    }

    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_FALSE;
}




}
// addr namespace
// vim: ts=4 sw=4 et
