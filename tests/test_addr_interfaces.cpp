/* test_addr_interfaces.cpp
 * Copyright (C) 2011-2017  Made to Order Software Corporation
 *
 * Project: http://snapwebsites.org/project/libaddr
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


TEST_CASE( "ipv4::interfaces", "[ipv4]" )
{
    GIVEN("addr::get_local_addresses()")
    {
        addr::addr::vector_t list(addr::addr::get_local_addresses());

        SECTION("verify list")
        {
            REQUIRE_FALSE(list.empty()); // at least "lo"

            // add stuff like verify there is an "lo" entry?
        }
    }
}


// vim: ts=4 sw=4 et
