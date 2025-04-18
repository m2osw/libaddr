// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/libaddr
// contact@m2osw.com
//
// Permission is hereby granted, free of charge, to any
// person obtaining a copy of this software and
// associated documentation files (the "Software"), to
// deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial
// portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
// EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/** \file
 * \brief Verify the interfaces function.
 *
 * This file implements a test that verifies the function that
 * reads the list of IP addresses as defined in your local
 * interfaces.
 */

// addr
//
#include    <libaddr/iface.h>


// self
//
#include    "catch_main.h"


// C
//
#include    <net/if.h>


// last include
//
#include    <snapdev/poison.h>



CATCH_TEST_CASE( "iface::interfaces", "[iface]" )
{
    CATCH_GIVEN("iface::get_local_addresses()")
    {
        addr::iface::set_local_addresses_cache_ttl(1500);

        addr::iface::pointer_vector_t list(addr::iface::get_local_addresses());
        addr::iface_index_name::vector_t name_index(addr::get_interface_name_index());

        CATCH_START_SECTION("iface::interfaces: verify list")
        {
            CATCH_REQUIRE_FALSE(list->empty()); // at least "lo"

            // add stuff like verify there is an "lo" entry?
            for(auto i : *list)
            {
                CATCH_REQUIRE_FALSE(i->get_name().empty());
                CATCH_REQUIRE(i->get_flags() != 0);

                switch(i->get_address().get_network_type())
                {
                case addr::network_type_t::NETWORK_TYPE_PRIVATE:
                case addr::network_type_t::NETWORK_TYPE_PUBLIC:
                case addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                case addr::network_type_t::NETWORK_TYPE_LINK_LOCAL:
                    break;

                default:
//std::cerr << "unexpected interface type " << static_cast<int>(i->get_address().get_network_type()) << "\n";
                    CATCH_REQUIRE(i->get_address().get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                    break;

                }

                CATCH_REQUIRE(i->has_broadcast_address() == ((i->get_flags() & IFF_BROADCAST) != 0));
                CATCH_REQUIRE(i->has_destination_address() == ((i->get_flags() & IFF_POINTOPOINT) != 0));

                addr::addr const & b(i->get_broadcast_address());
                if(i->has_broadcast_address())
                {
                    if(b.is_ipv4())
                    {
                        CATCH_REQUIRE(addr::is_broadcast_address(b));
                    }
                    else
                    {
                        // IPv6 is not offering broadcast IPs so we always
                        // get the default IP here
                        //
                        CATCH_REQUIRE(b.is_default());
                        CATCH_REQUIRE_FALSE(addr::is_broadcast_address(b));
                    }
                }
                else
                {
                    CATCH_REQUIRE(b.is_default());
                }

                addr::addr const & d(i->get_destination_address());
                if(!i->has_destination_address())
                {
                    CATCH_REQUIRE(d.is_default());
                }

                unsigned int const index(addr::get_interface_index_by_name(i->get_name()));
                for(auto & ni : name_index)
                {
                    if(ni.get_name() == i->get_name())
                    {
                        CATCH_REQUIRE(ni.get_index() == index);
                    }
                }
            }

            addr::iface::reset_local_addresses_cache();
        }
        CATCH_END_SECTION()
    }
}


// vim: ts=4 sw=4 et
