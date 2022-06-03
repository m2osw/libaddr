/* test_addr_routes.cpp
 * Copyright (c) 2011-2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Verify the route class a little further.
 *
 * This file implements a test that verifies a few additional things
 * in the route table.
 */

// addr lib
//
#include    <libaddr/route.h>


// self
//
#include    "catch_main.h"



// C lib
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

        CATCH_SECTION("verify list")
        {
            CATCH_REQUIRE_FALSE(routes.empty()); // at least the default route

            // check a few things
            //
            bool found_default(false);
            bool found_gateway(false);
            for(auto r : routes)
            {
                CATCH_REQUIRE_FALSE(r->get_interface_name().empty());
                CATCH_REQUIRE(r->get_interface_name().length() < IFNAMSIZ); // IFNAMSIZ includes the '\0' so '<' and not '<='

                // at least one flag is not zero
                int const f(r->get_flags());
                std::string const flags(r->flags_to_string());

                CATCH_REQUIRE(f != 0);
                CATCH_REQUIRE(!flags.empty());

                if(r->get_destination_address().is_default())
                {
                    CATCH_REQUIRE_FALSE(found_default);
                    found_default = true;

                    CATCH_REQUIRE((f & RTF_UP) != 0);
                }
                else
                {
                    routes_without_default.push_back(r);
                }

                if(!r->get_gateway_address().is_default())
                {
                    CATCH_REQUIRE_FALSE(found_gateway);
                    found_gateway = true;

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

        CATCH_SECTION("verify a search without a default route")
        {
            CATCH_REQUIRE(find_default_route(routes_without_default) == nullptr);
        }
    }
}


// vim: ts=4 sw=4 et
