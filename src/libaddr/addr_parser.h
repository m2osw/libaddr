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

    void                    set_default_address(std::string const & addr);
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
    std::string             f_default_address4;
    std::string             f_default_address6;
    std::string             f_default_mask4;
    std::string             f_default_mask6;
    int                     f_protocol = -1;
    int                     f_default_port = -1;
    std::string             f_error;
    int                     f_error_count = 0;
};



}
// addr namespace
// vim: ts=4 sw=4 et
