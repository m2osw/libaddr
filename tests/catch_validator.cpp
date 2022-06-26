// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Test the Address Validator.
 *
 * The advgetopt library comes with a way to validate command line options.
 * The validator_address class extends that feature by offering programmers
 * a way to validate input data as an IP address.
 */

// addr
//
#include    <libaddr/validator_address.h>


// self
//
#include    "catch_main.h"


// last include
//
#include    <snapdev/poison.h>




CATCH_TEST_CASE("validator", "[validator]")
{
    CATCH_START_SECTION("validator: default validation")
    {
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", advgetopt::string_list_t()));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
        CATCH_REQUIRE_FALSE(address_validator->validate("10.0.0.10:5434/24"));
        CATCH_REQUIRE(address_validator->validate("::"));
        CATCH_REQUIRE(address_validator->validate("[::]:307"));
        CATCH_REQUIRE(address_validator->validate("f801::5"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: allow all")
    {
        advgetopt::string_list_t const flags{
            "address=commas spaces newlines range lookup",
            "port=yes",
            "mask=address",
            "comment",
            "defaults=192.168.2.1:4040/24",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434/24"));
        CATCH_REQUIRE(address_validator->validate(":5/255.255.255.0"));
        CATCH_REQUIRE(address_validator->validate("::"));
        CATCH_REQUIRE(address_validator->validate("f801::5/48"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: unknown option")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: \"coment=\" is not a known option for the address validator.");
        advgetopt::string_list_t const flags{
            "",             // ignored
            "coment=",      // "comment" mispelled
            "",             // ignored
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("5.6.7.8"));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
