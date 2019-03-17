// Network Address -- get routes and print them, similar to system `route`
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

/** \file
 * \brief A tool to check the system routes.
 *
 * This tool is used to verify that our route class works as expected.
 */

// addr library
//
#include "libaddr/route.h"

// C++ library
//
#include <iomanip>
#include <iostream>
#include <sstream>


namespace
{

bool g_show_default = false;
bool g_hide_headers = false;

}

int main(int argc, char * argv[])
{
    addr::route::vector_t routes(addr::route::get_ipv4_routes());

    for(int idx(1); idx < argc; ++idx)
    {
        if(strcmp(argv[idx], "-h") == 0
        || strcmp(argv[idx], "--help") == 0)
        {
            std::cout << "Usage: %s [-opts]" << std::endl;
            std::cout << "where -opts is one or more of:" << std::endl;
            std::cout << "  --help | -h        print out this help screen." << std::endl;
            std::cout << "  --default | -d     only print the default route." << std::endl;
            std::cout << "  --hide-headers     do not print the headers." << std::endl;
            exit(1);
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
            std::cerr << "error: unknown command line option \"" << argv[idx] << "\". Try --help for additional info." << std::endl;
            exit(1);
        }
    }

    if(routes.empty())
    {
        std::cerr << "error: no routes found, is your network up?" << std::endl;
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
                  << std::endl;
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
                      << std::left << std::setw(16) << r->get_destination_address().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)
                      << std::left << std::setw(16) << r->get_gateway_address().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)
                      << std::left << std::setw( 8) << r->flags_to_string()
                      << std::left << std::setw( 8) << r->get_reference_count()
                      << std::left << std::setw( 8) << r->get_use()
                      << std::left << std::setw( 8) << r->get_metric()
                      << std::left << std::setw(16) << m.str()
                      << std::left << std::setw( 8) << r->get_mtu()
                      << std::left << std::setw( 8) << r->get_window()
                      << std::left << std::setw( 8) << r->get_irtt()
                      << std::endl;
        }
    }

    return 0;
}

// vim: ts=4 sw=4 et

