// Network Address -- classes functions to ease handling IP addresses
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The various libaddr classes.
 *
 * This header includes the base addr class used to handle one binary
 * address.
 */

// addr library
//
#include <libaddr/addr.h>



namespace addr
{


class iface_index_name
{
public:
    typedef std::vector<iface_index_name>   vector_t;

                                    iface_index_name(int index, std::string const & name);

    int                             get_index() const;
    std::string const &             get_name() const;

private:
    int                             f_index;
    std::string                     f_name;
};

iface_index_name::vector_t          get_interface_name_index();


class iface
{
public:
    typedef std::shared_ptr<iface>  pointer_t;
    typedef std::vector<iface>      vector_t;

    static iface::vector_t          get_local_addresses();

    std::string                     get_name() const;
    unsigned int                    get_flags() const;
    addr const &                    get_address() const;
    addr const &                    get_broadcast_address() const;
    addr const &                    get_destination_address() const;

    bool                            has_broadcast_address() const;
    bool                            has_destination_address() const;

private:
    std::string                     f_name = std::string();
    unsigned int                    f_flags = 0;
    addr                            f_address = addr();
    addr                            f_broadcast_address = addr();
    addr                            f_destination_address = addr();
};



iface::pointer_t find_addr_interface(addr const & a, bool allow_default_destination = true);



}
// addr namespace
// vim: ts=4 sw=4 et
