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
