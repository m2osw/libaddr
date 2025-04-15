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
#pragma once

/** \file
 * \brief The declaration of the parser of the libaddr classes.
 *
 * This header includes the addr_parser class used to parse user input
 * string and convert them to binary IP addresses.
 */

// self
//
#include    <libaddr/addr_range.h>



namespace addr
{


enum class allow_t
{
    ALLOW_ADDRESS,                          // address (IP)
    ALLOW_REQUIRED_ADDRESS,                 // address cannot be empty
    ALLOW_MULTI_ADDRESSES_COMMAS,           // IP:port/mask,IP:port/mask,...
    ALLOW_MULTI_ADDRESSES_SPACES,           // IP:port/mask IP:port/mask ...
    ALLOW_MULTI_ADDRESSES_NEWLINES,         // IP:port/mask\nIP:port/mask\n...
    ALLOW_ADDRESS_LOOKUP,                   // whether DNS lookup is allowed
    ALLOW_ADDRESS_RANGE,                    // IP-IP:port/mask

    ALLOW_PORT,                             // port
    ALLOW_REQUIRED_PORT,                    // port must be defined

    ALLOW_MASK,                             // mask
    ALLOW_ADDRESS_MASK,                     // mask like an address (opposed to just a number, which is the only new valid version)

    ALLOW_COMMENT_HASH,                     // if address starts with '#', it's a comment, ignore; useful with ALLOW_MULTI_ADDRESSES_NEWLINES
    ALLOW_COMMENT_SEMICOLON,                // if address starts with ':', it's a comment, ignore; useful with ALLOW_MULTI_ADDRESSES_NEWLINES

    // TODO: the following are not yet implemented
    ALLOW_MULTI_PORTS_SEMICOLONS,           // port1;port2;...
    ALLOW_MULTI_PORTS_COMMAS,               // port1,port2,...
    ALLOW_PORT_RANGE,                       // port1-port2

    ALLOW_max
};


typedef std::uint_fast16_t                  sort_t;

constexpr sort_t const                      SORT_NO             = 0x0000;       // keep IPs as found
constexpr sort_t const                      SORT_IPV6_FIRST     = 0x0001;       // put IPv6 first (IPv6, IPv4, empty)
constexpr sort_t const                      SORT_IPV4_FIRST     = 0x0002;       // put IPv4 first (IPv4, IPv6, empty)
constexpr sort_t const                      SORT_FULL           = 0x0004;       // sort IPs between each others (default keep in order found)
constexpr sort_t const                      SORT_MERGE          = 0x0008;       // merge ranges which support a union (implies SORT_FULL)
constexpr sort_t const                      SORT_NO_EMPTY       = 0x0010;       // remove empty entries


class addr_parser
{
public:
                            addr_parser();

    void                    set_default_address(std::string const & address);
    std::string const &     get_default_address4() const;
    std::string const &     get_default_address6() const;

    void                    set_default_port(std::string const & port);
    void                    set_default_port(int const port);
    int                     get_default_port() const;

    void                    set_default_mask(std::string const & mask);
    std::string const &     get_default_mask4() const;
    std::string const &     get_default_mask6() const;

    void                    set_protocol(std::string const & protocol);
    void                    set_protocol(int const protocol);
    void                    clear_protocol();
    int                     get_protocol() const;

    void                    set_sort_order(sort_t const sort);
    sort_t                  get_sort_order() const;

    void                    set_allow(allow_t const flag, bool const allow);
    bool                    get_allow(allow_t const flag) const;

    bool                    has_errors() const;
    void                    emit_error(std::string const & msg);
    std::string const &     error_messages() const;
    int                     error_count() const;
    void                    clear_errors();

    addr_range::vector_t    parse(std::string const & in);

private:
    void                    parse_address_range(std::string const & in, addr_range::vector_t & result);
    void                    parse_cidr(std::string const & in, addr_range::vector_t & result);
    bool                    parse_address(std::string const & in, std::string const & mask, addr_range::vector_t & result);
    void                    parse_address4(std::string const & in, addr_range::vector_t & result);
    void                    parse_address6(std::string const & in, std::size_t const colons, addr_range::vector_t & result);
    void                    parse_address_range_port(std::string const & addresses, std::string const & port_str, addr_range::vector_t & result, bool ipv6);
    void                    parse_address_port(std::string address, std::string port_str, addr_range::vector_t & result, bool ipv6);
    void                    parse_address_port_ignore_duplicates(std::string address, std::string port_str, addr_range::vector_t & result, bool ipv6);
    void                    parse_mask(std::string const & mask, addr & cidr, bool is_ipv4);

    bool                    f_flags[static_cast<int>(allow_t::ALLOW_max)] = {};
    sort_t                  f_sort = SORT_NO;
    std::string             f_default_address4 = std::string();
    std::string             f_default_address6 = std::string();
    std::string             f_default_mask4 = std::string();
    std::string             f_default_mask6 = std::string();
    int                     f_protocol = -1;
    int                     f_default_port = -1;
    std::string             f_error = std::string();
    int                     f_error_count = 0;
};

addr string_to_addr(
          std::string const & a
        , std::string const & default_address = std::string()
        , int default_port = -1
        , std::string const & protocol = std::string()
        , bool mask = false);



}
// namespace addr
// vim: ts=4 sw=4 et
