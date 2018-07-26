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
 * \brief The declaration of the parser of the libaddr classes.
 *
 * This header includes the addr_parser class used to parse user input
 * string and convert them to binary IP addresses.
 */

// self
//
#include "libaddr/addr_range.h"



namespace addr
{


class addr_parser
{
public:
    enum class flag_t
    {
        ADDRESS,                            // address (IP)
        REQUIRED_ADDRESS,                   // address cannot be empty
        PORT,                               // port
        REQUIRED_PORT,                      // port cannot be empty
        MASK,                               // mask
        MULTI_ADDRESSES_COMMAS,             // IP:port/mask,IP:port/mask,...
        MULTI_ADDRESSES_SPACES,             // IP:port/mask IP:port/mask ...
        MULTI_ADDRESSES_COMMAS_AND_SPACES,  // IP:port/mask, IP:port/mask, ...

        // the following are not yet implemented
        MULTI_PORTS_SEMICOLONS,             // port1;port2;...
        MULTI_PORTS_COMMAS,                 // port1,port2,...
        PORT_RANGE,                         // port1-port2
        ADDRESS_RANGE,                      // IP-IP

        FLAG_max
    };

    void                    set_default_address(std::string const & address);
    std::string const &     get_default_address4() const;
    std::string const &     get_default_address6() const;

    void                    set_default_port(int const port);
    int                     get_default_port() const;

    void                    set_default_mask(std::string const & mask);
    std::string const &     get_default_mask4() const;
    std::string const &     get_default_mask6() const;

    void                    set_protocol(std::string const & protocol);
    void                    set_protocol(int const protocol);
    void                    clear_protocol();
    int                     get_protocol() const;

    void                    set_allow(flag_t const flag, bool const allow);
    bool                    get_allow(flag_t const flag) const;

    bool                    has_errors() const;
    void                    emit_error(std::string const & msg);
    std::string const &     error_messages() const;
    int                     error_count() const;
    void                    clear_errors();

    addr_range::vector_t    parse(std::string const & in);

private:
    void                    parse_cidr(std::string const & in, addr_range::vector_t & result);
    void                    parse_address(std::string const & in, std::string const & mask, addr_range::vector_t & result);
    void                    parse_address4(std::string const & in, addr_range::vector_t & result);
    void                    parse_address6(std::string const & in, addr_range::vector_t & result);
    void                    parse_address_port(std::string address, std::string const & port_str, addr_range::vector_t & result, std::string const & default_address);
    void                    parse_mask(std::string const & mask, addr & cidr);

    bool                    f_flags[static_cast<int>(flag_t::FLAG_max)] = { true, false, true, false, false, false, false, false, false, false, false, false };
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
// addr namespace
// vim: ts=4 sw=4 et
