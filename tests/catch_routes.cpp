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
 * \brief Verify the route class a little further.
 *
 * This file implements a test that verifies a few additional things
 * in the route table.
 */

// libaddr
//
#include    <libaddr/route.h>


// self
//
#include    "catch_main.h"



// C
//
#include    <net/if.h>
#include    <net/route.h>


// last include
//
#include    <snapdev/poison.h>



CATCH_TEST_CASE("ipv4::routes", "[ipv4]")
{
    CATCH_GIVEN("route::get_ipv4_routes()")
    {
        addr::route::vector_t routes(addr::route::get_ipv4_routes());
        addr::route::vector_t routes_without_default;

        CATCH_START_SECTION("routes: verify list")
        {
            CATCH_REQUIRE_FALSE(routes.empty()); // at least the default route

            // check a few things
            //
            bool found_default(false);
            bool found_gateway(false);
            //std::map<std::string, bool> found;
            for(auto r : routes)
            {
                CATCH_REQUIRE_FALSE(r->get_interface_name().empty());
                CATCH_REQUIRE(r->get_interface_name().length() < IFNAMSIZ); // IFNAMSIZ includes the '\0' so '<' and not '<='

                //if(found[r->get_interface_name()])
                //{
                //    std::cerr
                //        << "WARNING: found interface \""
                //        << r->get_interface_name()
                //        << "\" twice.\n";
                //    continue;
                //}
                //found[r->get_interface_name()] = true;

                // at least one flag is not zero
                int const f(r->get_flags());
                std::string const flags(r->flags_to_string());

                CATCH_REQUIRE(f != 0);
                CATCH_REQUIRE(!flags.empty());

#if 0
// output similar to `route`
std::cout << "Route: Dest: " << r->get_destination_address().to_ipv4or6_string(addr::STRING_IP_ADDRESS)
<< " Gateway: " << r->get_gateway_address().to_ipv4or6_string(addr::STRING_IP_ADDRESS)
<< " Flags: " << r->get_flags()
<< " Metric: " << r->get_metric()
<< " Iface: " << r->get_interface_name()
<< "\n";
#endif
                if(r->get_destination_address().is_default())
                {
                    CATCH_REQUIRE_FALSE(found_default);
                    found_default = true;

                    CATCH_REQUIRE((f & RTF_UP) != 0);

                    if(!r->get_gateway_address().is_default())
                    {
                        CATCH_REQUIRE_FALSE(found_gateway);
                        found_gateway = true;
                    }
                }
                else
                {
                    routes_without_default.push_back(r);
                }

                if(!r->get_gateway_address().is_default())
                {
                    CATCH_REQUIRE((f & RTF_GATEWAY) != 0);
                }

                // Not much I can test on the following?
                //r->get_flags()
                CATCH_REQUIRE(r->get_reference_count() >= 0);
                CATCH_REQUIRE(r->get_use() >= 0);
                CATCH_REQUIRE(r->get_metric() >= 0);
                CATCH_REQUIRE(r->get_mtu() >= 0);
                CATCH_REQUIRE(r->get_window() >= 0);
                CATCH_REQUIRE(r->get_irtt() >= 0);
            }
            CATCH_REQUIRE(found_default);
            CATCH_REQUIRE(found_gateway);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("routes: verify a search without a default route")
        {
            CATCH_REQUIRE(find_default_route(routes_without_default) == nullptr);
        }
        CATCH_END_SECTION()
    }
}


// vim: ts=4 sw=4 et
