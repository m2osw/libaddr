// Network Address -- get routes and print them, similar to system `route`
// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief A tool to check the system interfaces.
 *
 * This tool is used to verify that our iface class works as expected.
 */

// libaddr
//
#include    <libaddr/iface.h>
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

enum filter_t
{
    FILTER_ALL,             // no filtering
    FILTER_PUBLIC,          // only display interfaces with public IP addresses
    FILTER_PRIVATE,         // only display interfaces with private IP addresses
    FILTER_LOOPBACK,        // only display interfaces with the "lo" interface
    FILTER_DEFAULT,         // only display interfaces with the default route
};

filter_t g_filter = FILTER_ALL;
bool g_hide_headers = false;
bool g_asterisk = false;
bool g_name_only = false;

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
                   "  --default | -d     only print name of default interface.\n"
                   "  --hide-headers     do not print the headers.\n"
                   "  --public           only print name of public interfaces.\n"
                   "  --private          only print name of private interfaces.\n"
                   "  --loopback         only print name of loopback interface.\n";
                   "  --asterisk         print an asterisk for default addresses (instead of 0.0.0.0 or ::).\n";
                   "  --name-only        only print the name of the interface.\n";
            return 1;
        }
        else if(strcmp(argv[idx], "--hide-headers") == 0)
        {
            g_hide_headers = true;
        }
        else if(strcmp(argv[idx], "-d") == 0
             || strcmp(argv[idx], "--default") == 0)
        {
            g_filter = FILTER_DEFAULT;
        }
        else if(strcmp(argv[idx], "--public") == 0)
        {
            g_filter = FILTER_PUBLIC;
        }
        else if(strcmp(argv[idx], "--private") == 0)
        {
            g_filter = FILTER_PRIVATE;
        }
        else if(strcmp(argv[idx], "--loopback") == 0)
        {
            g_filter = FILTER_LOOPBACK;
        }
        else if(strcmp(argv[idx], "--asterisk") == 0)
        {
            g_asterisk = true;
        }
        else if(strcmp(argv[idx], "--name-only") == 0)
        {
            g_name_only = true;
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

    addr::iface::pointer_vector_t interfaces(addr::iface::get_local_addresses());
    if(interfaces->empty())
    {
        std::cerr << "error: no interfaces found, is your network up?" << std::endl;
        return 1;
    }

    // headers
    //
    if(!g_hide_headers && !g_name_only)
    {
        std::cout << "Iface            "
                     "Flags "
                     "Address                                  "
                     "Broadcast                                "
                     "Destination                              "
                     "\n";
    }

    std::string default_interface;
    if(g_filter == FILTER_DEFAULT)
    {
        addr::route::vector_t routes(addr::route::get_ipv4_routes());
        if(routes.empty())
        {
            std::cerr << "error: unknown default route.\n";
            return 1;
        }
        for(auto const & r : routes)
        {
            if(r->get_destination_address().is_default())
            {
                default_interface = r->get_interface_name();
            }
        }
    }

    std::set<std::string> found;
    for(auto const & i : *interfaces)
    {
        addr::addr const & a(i->get_address());
        switch(g_filter)
        {
        case FILTER_ALL:
            // no filtering
            break;

        case FILTER_PUBLIC:
            if(a.get_network_type() != addr::network_type_t::NETWORK_TYPE_PUBLIC)
            {
                continue;
            }
            break;

        case FILTER_PRIVATE:
            if(a.get_network_type() != addr::network_type_t::NETWORK_TYPE_PRIVATE)
            {
                continue;
            }
            break;

        case FILTER_LOOPBACK:
            if(a.get_network_type() != addr::network_type_t::NETWORK_TYPE_LOOPBACK)
            {
                continue;
            }
            break;

        case FILTER_DEFAULT:
            if(a.get_name() != default_interface)
            {
                continue;
            }
            break;

        }

        if(g_name_only)
        {
            // prevent printing the same name twice
            //
            std::pair inserted(found.insert(i->get_name()));
            if(inserted.second)
            {
                std::cout
                    << i->get_name()
                    << '\n';
            }
        }
        else
        {
            addr::addr const & b(i->get_broadcast_address());
            addr::addr const & d(i->get_destination_address());
            bool const a_is_ipv4(a.is_ipv4());

            addr::string_ip_t const mode(addr::STRING_IP_ADDRESS
                | (g_asterisk ? addr::STRING_IP_DEFAULT_AS_ASTERISK : 0)
                | (a_is_ipv4 ? addr::STRING_IP_DEFAULT_AS_IPV4 : 0));

            std::cout
                << std::left << std::setw(17) << i->get_name()
                << std::left << std::setw( 6) << i->get_flags()  // TODO: add conversion to letters
                << std::left << std::setw(41) << a.to_ipv4or6_string(addr::STRING_IP_ADDRESS)
                << std::left << std::setw(41) << b.to_ipv4or6_string(mode)
                << std::left << std::setw(41) << d.to_ipv4or6_string(mode)
                << '\n';
        }
    }

    return 0;
}

// vim: ts=4 sw=4 et

