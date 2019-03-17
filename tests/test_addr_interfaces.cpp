/* test_addr_interfaces.cpp
 * Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 *
 * Project: https://snapwebsites.org/project/libaddr
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and
 * associated documentation files (the "Software"), to
 * deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 * \brief Verify the interfaces function.
 *
 * This file implements a test that verifies the function that
 * reads the list of IP addresses as defined in your local
 * interfaces.
 */

// self
//
#include "test_addr_main.h"

// addr lib
//
#include "libaddr/iface.h"

// C lib
//
#include <net/if.h>


TEST_CASE( "ipv4::interfaces", "[ipv4]" )
{
    GIVEN("iface::get_local_addresses()")
    {
        addr::iface::vector_t list(addr::iface::get_local_addresses());

        SECTION("verify list")
        {
            REQUIRE_FALSE(list.empty()); // at least "lo"

            // add stuff like verify there is an "lo" entry?
            for(auto i : list)
            {
                REQUIRE_FALSE(i.get_name().empty());
                REQUIRE(i.get_flags() != 0);

                switch(i.get_address().get_network_type())
                {
                case addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                case addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                case addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                case addr::addr::network_type_t::NETWORK_TYPE_LINK_LOCAL:
                    break;

                default:
std::cerr << "unexpected interface type " << static_cast<int>(i.get_address().get_network_type()) << "\n";
                    REQUIRE_FALSE("unexpected network type for interface");
                    break;

                }

                REQUIRE(i.has_broadcast_address() == ((i.get_flags() & IFF_BROADCAST) != 0));
                REQUIRE(i.has_destination_address() == ((i.get_flags() & IFF_POINTOPOINT) != 0));

                addr::addr const & b(i.get_broadcast_address());
                if(!i.has_broadcast_address())
                {
                    REQUIRE(b.is_default());
                }

                addr::addr const & d(i.get_destination_address());
                if(!i.has_destination_address())
                {
                    REQUIRE(d.is_default());
                }
            }
        }
    }
}


// vim: ts=4 sw=4 et
