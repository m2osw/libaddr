// Copyright (c) 2018-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The implementation of the route class.
 *
 * This file includes the implementation of the route class.
 *
 * The route class registers information about a route. It is created
 * by using the get_routes() function.
 */

// self
//
#include    "libaddr/exception.h"
#include    "libaddr/route.h"


// C++
//
#include    <algorithm>
#include    <fstream>
#include    <iostream>


// C
//
#include    <net/route.h>


// last include
//
#include    <snapdev/poison.h>



namespace addr
{


typedef std::vector<std::string> words_t;


/** \brief Details used by the route class implementation.
 *
 * The following handles the route class and gathering of the routes.
 */
namespace
{


/** \brief Read one line of content from a file.
 *
 * This function reads one line of content up to the next '\n'.
 *
 * \param[in,out] in  The input stream.
 * \param[out] line  The line just read.
 *
 * \return 0 on success, -1 on error (generally EOF)
 */
int readwords(std::ifstream & in, words_t & words)
{
    words.clear();
    std::string w;
    for(;;)
    {
        int const c(in.get());
        if(c < 0)
        {
            return -1;
        }
        if(c == '\n')
        {
            return 0;
        }
        if(c == '\t' || c == ' ')
        {
            // there should always be a first word, however, there can
            // be multiple '\t' or ' ' after one another
            if(!w.empty())
            {
                words.push_back(w);
                w.clear();
            }
        }
        else
        {
            w += c;
        }
    }
}


int get_position(words_t const & headers, std::string const & column_name)
{
    auto it(std::find(
          headers.cbegin()
        , headers.cend()
        , column_name));

    if(it == headers.end())
    {
        return -1; // LCOV_EXCL_LINE
    }

    return it - headers.begin();
}


std::string const & get_value(words_t const & entries, int pos)
{
    static std::string const    not_found;

    if(pos < static_cast<int>(entries.size()))
    {
        return entries[pos];
    }

    return not_found; // LCOV_EXCL_LINE
}


int hex_to_number(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10; // LCOV_EXCL_LINE
    }
    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    throw addr_invalid_argument("invalid hexadecimal digit"); // LCOV_EXCL_LINE
}


addr hex_to_addr(std::string const & address)
{
    if(address.length() != 8)
    {
        throw addr_invalid_argument("invalid length for a hex address"); // LCOV_EXCL_LINE
    }

    struct sockaddr_in in = sockaddr_in();
    in.sin_family = AF_INET;
    in.sin_port = 0;
    in.sin_addr.s_addr =
                  (hex_to_number(address[7]) <<  0)
                | (hex_to_number(address[6]) <<  4)
                | (hex_to_number(address[5]) <<  8)
                | (hex_to_number(address[4]) << 12)
                | (hex_to_number(address[3]) << 16)
                | (hex_to_number(address[2]) << 20)
                | (hex_to_number(address[1]) << 24)
                | (hex_to_number(address[0]) << 28)
            ;

    return addr(in);
}


struct flag_name_t
{
    uint32_t const  f_flag;
    char const      f_name;
};

/** \brief Flags used by route tables.
 *
 * \note
 * The list of flags presented here includes IPv4 and IPv6 flags.
 *
 * \note
 * Some of the flags are not defined in Ubuntu 16.04.
 */
flag_name_t const g_rtf_flag_names[] =
{
    { RTF_UP,        'U' },
    { RTF_GATEWAY,   'G' },
    { RTF_REJECT,    '!' },        // may not be defined
    { RTF_HOST,      'H' },
    { RTF_REINSTATE, 'R' },
    { RTF_DYNAMIC,   'D' },
    { RTF_MODIFIED,  'M' },
    { RTF_DEFAULT,   'd' },
    { RTF_ALLONLINK, 'a' },
    { RTF_ADDRCONF,  'c' },
    { RTF_NONEXTHOP, 'o' },
    //{ RTF_EXPIRES,   'e' },
    { RTF_CACHE,     'C' },
    { RTF_FLOW,      'f' },
    { RTF_POLICY,    'p' },
    { RTF_LOCAL,     'l' },
    { RTF_MTU,       'u' },
    { RTF_WINDOW,    'w' },
    { RTF_IRTT,      'i' },
    //{ RTF_NOTCACHED, 'n' },
};



}
// no name namespace



/** \brief Read the list of routes.
 *
 * This function reads the list of routes using the /proc/net/routes
 * file. It returns a vector of easy to use route objects.
 *
 * The content of the route table is scanned using the column names
 * so it makes sure that it does not use the wrong column (i.e.
 * expect that the columns never change over time.)
 *
 * \note
 * If an error occurs, the reurned vector is empty and errno is
 * set to the error that happened. The ENODATA error is used
 * if some mandatory columns are missing and thus this function
 * cannot properly load the columns.
 *
 * \todo
 * Write the IPv6 function. It's similar only there are no headers
 * and (obviously?!) the IPs are IPv6 instead of IPv4.
 *
 * \return A vector of the routes found in the file.
 */
route::vector_t route::get_ipv4_routes()
{
    // the 'route' tool uses '/proc/net/route' so we do that too here
    //
    route::vector_t routes;

    std::ifstream in("/proc/net/route");

    // the first line is a set of headers, we use that to make sure that
    // we know what each column is
    //
    words_t headers;
    int e(readwords(in, headers));
    if(e < 0)
    {
        return routes; // LCOV_EXCL_LINE
    }

    // TODO: we may want to remove case although I don't think it will
    //       change over time, it could be one more thing that could...

    int const pos_iface      (get_position(headers, "Iface"));
    int const pos_destination(get_position(headers, "Destination"));
    int const pos_gateway    (get_position(headers, "Gateway"));
    int const pos_flags      (get_position(headers, "Flags"));
    int const pos_refcnt     (get_position(headers, "RefCnt"));
    int const pos_use        (get_position(headers, "Use"));
    int const pos_metric     (get_position(headers, "Metric"));
    int const pos_mask       (get_position(headers, "Mask"));
    int const pos_mtu        (get_position(headers, "MTU"));
    int const pos_window     (get_position(headers, "Window"));
    int const pos_irtt       (get_position(headers, "IRTT"));

    if(pos_iface == -1
    || pos_destination == -1
    || pos_gateway == -1)
    {
        errno = ENODATA; // LCOV_EXCL_LINE
        return routes;   // LCOV_EXCL_LINE
    }

    for(;;)
    {
        // read one entry
        //
        words_t entries;
        e = readwords(in, entries);
        if(e < 0)
        {
            break;
        }

        // convert each column to data in a 'route' object
        //
        route r;

        r.f_interface_name      = get_value(entries, pos_iface);
        r.f_destination_address = hex_to_addr(get_value(entries, pos_destination));
        r.f_gateway_address     = hex_to_addr(get_value(entries, pos_gateway));
        r.f_flags               = std::stol(get_value(entries, pos_flags));
        r.f_reference_count     = std::stol(get_value(entries, pos_refcnt));
        r.f_use                 = std::stol(get_value(entries, pos_use));
        r.f_metric              = std::stol(get_value(entries, pos_metric));
        r.f_mtu                 = std::stol(get_value(entries, pos_mtu));
        r.f_window              = std::stol(get_value(entries, pos_window));
        r.f_irtt                = std::stol(get_value(entries, pos_irtt));

        // the mask is handled specially
        //
        std::string mask_str(get_value(entries, pos_mask));
        if(!mask_str.empty())
        {
            addr mask = hex_to_addr(mask_str);
            struct sockaddr_in ipv4 = sockaddr_in();
            mask.get_ipv4(ipv4);
            uint8_t m[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
            m[12] = ipv4.sin_addr.s_addr >>  0;
            m[13] = ipv4.sin_addr.s_addr >>  8;
            m[14] = ipv4.sin_addr.s_addr >> 16;
            m[15] = ipv4.sin_addr.s_addr >> 24;
            r.f_destination_address.set_mask(m);
        }

        routes.push_back(pointer_t(new route(r)));
    }

    return routes;
}


std::string const & route::get_interface_name() const
{
    return f_interface_name;
}


addr const & route::get_destination_address() const
{
    return f_destination_address;
}


addr const & route::get_gateway_address() const
{
    return f_gateway_address;
}


int route::get_flags() const
{
    return f_flags;
}


std::string route::flags_to_string() const
{
    std::string result;

    std::for_each(
          g_rtf_flag_names
        , g_rtf_flag_names + sizeof(g_rtf_flag_names) / sizeof(g_rtf_flag_names[0])
        , [&result, this](auto const & fn)
        {
            if((f_flags & fn.f_flag) != 0)
            {
                result += fn.f_name;
            }
        });

    return result;
} // LCOV_EXCL_LINE


int route::get_reference_count() const
{
    return f_reference_count;
}


int route::get_use() const
{
    return f_use;
}


int route::get_metric() const
{
    return f_metric;
}


int route::get_mtu() const
{
    return f_mtu;
}


int route::get_window() const
{
    return f_window;
}


int route::get_irtt() const
{
    return f_irtt;
}


route::pointer_t find_default_route(route::vector_t const & routes)
{
    auto it(std::find_if(
          routes.cbegin()
        , routes.cend()
        , [](auto const & r)
        {
            return r->get_destination_address().is_default();
        }));

    if(it == routes.cend())
    {
        return nullptr;
    }

    return *it;
}



}
// namespace addr
// vim: ts=4 sw=4 et
