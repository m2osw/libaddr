// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The various route class.
 *
 * This header includes the route class used to handle routes as found
 * in the routing tables of the kernel.
 */

// addr lib
//
#include    <libaddr/addr.h>



namespace addr
{



class route
{
public:
    typedef std::shared_ptr<route>  pointer_t;
    typedef std::vector<pointer_t>  vector_t;

    static vector_t                 get_ipv4_routes();

    std::string const &             get_interface_name() const;
    addr const &                    get_destination_address() const;
    addr const &                    get_gateway_address() const;
    int                             get_flags() const;
    std::string                     flags_to_string() const;
    int                             get_reference_count() const;
    int                             get_use() const;
    int                             get_metric() const;
    int                             get_mtu() const;
    int                             get_window() const;
    int                             get_irtt() const;

private:
    std::string                     f_interface_name = std::string();
    addr                            f_destination_address = addr();  // Destination + Mask
    addr                            f_gateway_address = addr();
    int                             f_flags = 0;
    int                             f_reference_count = 0;
    int                             f_use = 0;
    int                             f_metric = 0;
    int                             f_mtu = 0;
    int                             f_window = 0;
    int                             f_irtt = 0;
};


route::pointer_t find_default_route(route::vector_t const & routes);


}
// namespace addr
// vim: ts=4 sw=4 et
