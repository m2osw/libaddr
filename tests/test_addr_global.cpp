/* test_addr_global.cpp
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
 * \brief Check the global functions.
 *
 * This test file includes test that checks the global functions.
 *
 * At this time, the only global functions we check here are the
 * version functions.
 *
 * The address_match_ranges() function is checked in the IPv4
 * and IPv6 tests along with other address tests.
 */

// self
//
#include "test_addr_main.h"
#include "libaddr/addr.h"
#include "libaddr/version.h"




TEST_CASE( "version", "[global]" )
{
    REQUIRE(addr::get_version_major() == LIBADDR_VERSION_MAJOR);
    REQUIRE(addr::get_version_minor() == LIBADDR_VERSION_MINOR);
    REQUIRE(addr::get_version_patch() == LIBADDR_VERSION_PATCH);
    REQUIRE(std::string(addr::get_version_string()) == std::string(LIBADDR_VERSION_STRING));
}


// vim: ts=4 sw=4 et
