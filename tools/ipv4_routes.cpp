// Network Address -- get routes and print them, similar to system `route`
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
//

/** \file
 * \brief A tool to check the system routes.
 *
 * This tool is used to verify that our route class works as expected.
 */

// libaddr
//
#include    <libaddr/route.h>


// C++
//
#include    <iomanip>
#include    <iostream>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>


namespace
{

bool g_show_default = false;
bool g_hide_headers = false;

}

int main(int argc, char * argv[])
{
    for(int idx(1); idx < argc; ++idx)
    {
        if(strcmp(argv[idx], "-h") == 0
        || strcmp(argv[idx], "--help") == 0)
        {
            std::cout
                << "Usage: " << argv[0] << " [-opts]\n"
                   "where -opts is one or more of:\n"
                   "  --help | -h        print out this help screen.\n"
                   "  --default | -d     only print the default route.\n"
                   "  --hide-headers     do not print the headers.\n";
            return 1;
        }
        else if(strcmp(argv[idx], "-d") == 0
             || strcmp(argv[idx], "--default") == 0)
        {
            g_show_default = true;
        }
        else if(strcmp(argv[idx], "--hide-headers") == 0)
        {
            g_hide_headers = true;
        }
        else
        {
            std::cerr
                << "error: unknown command line option \""
                << argv[idx]
                << "\". Try --help for additional info.\n";
            return 1;
        }
    }

    addr::route::vector_t routes(addr::route::get_ipv4_routes());
    if(routes.empty())
    {
        std::cerr << "error: no routes found, is your network up?\n";
        return 1;
    }

    // headers
    //
    if(!g_hide_headers)
    {
        std::cout << "Iface   "
                     "Destination     "
                     "Gateway         "
                     "Flags   "
                     "RefCnt  "
                     "Use     "
                     "Metric  "
                     "Mask            "
                     "MTU     "
                     "Window  "
                     "IRTT    "
                     "\n";
    }

    for(auto r : routes)
    {
        if(!g_show_default
        || r->get_destination_address().is_default())
        {
            uint8_t mask[16];
            r->get_destination_address().get_mask(mask);
            std::stringstream m;
            m << static_cast<int>(mask[12]) << "."
              << static_cast<int>(mask[13]) << "."
              << static_cast<int>(mask[14]) << "."
              << static_cast<int>(mask[15]);
            std::cout << std::left << std::setw( 8) << r->get_interface_name()
                      << std::left << std::setw(16) << r->get_destination_address().to_ipv4or6_string(addr::STRING_IP_ADDRESS)
                      << std::left << std::setw(16) << r->get_gateway_address().to_ipv4or6_string(addr::STRING_IP_ADDRESS)
                      << std::left << std::setw( 8) << r->flags_to_string()
                      << std::left << std::setw( 8) << r->get_reference_count()
                      << std::left << std::setw( 8) << r->get_use()
                      << std::left << std::setw( 8) << r->get_metric()
                      << std::left << std::setw(16) << m.str()
                      << std::left << std::setw( 8) << r->get_mtu()
                      << std::left << std::setw( 8) << r->get_window()
                      << std::left << std::setw( 8) << r->get_irtt()
                      << '\n';
        }
    }

    return 0;
}

// vim: ts=4 sw=4 et

