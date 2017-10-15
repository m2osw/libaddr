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
 * \brief The addr_range class.
 *
 * This header defines the addr_range class used to handle one range of
 * addresses (with a 'from' and a 'to' pair of addresses.)
 *
 * Also we support CIDR, a CIDR is not a range. A range can define anything
 * that is not a perfect CIDR match. For example, you could have a start
 * address of 192.168.10.5 and an end address of 192.168.10.10.
 */

// self
//
#include "addr.h"

// C++ lib
//
#include <memory>
#include <cstring>

// C lib
//
#include <arpa/inet.h>



namespace addr
{




class addr_range
{
public:
    typedef std::shared_ptr<addr_range>     pointer_t;
    typedef std::vector<addr_range>         vector_t;

    bool                            has_from() const;
    bool                            has_to() const;
    bool                            is_range() const;
    bool                            is_empty() const;
    bool                            is_in(addr const & rhs) const;

    void                            set_from(addr const & from);
    addr &                          get_from();
    addr const &                    get_from() const;
    void                            set_to(addr const & to);
    addr &                          get_to();
    addr const &                    get_to() const;

    addr_range                      intersection(addr_range const & rhs) const;
    bool                            match(addr const & address) const;

private:
    bool                            f_has_from = false;
    bool                            f_has_to = false;
    addr                            f_from;
    addr                            f_to;
};


bool address_match_ranges(addr_range::vector_t ranges, addr const & address);




}
// addr namespace
// vim: ts=4 sw=4 et
