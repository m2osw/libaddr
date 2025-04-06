// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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
        CATCH_REQUIRE(address_validator->validate("192.168.1.1\n; and comments are allowed after"));
        CATCH_REQUIRE(address_validator->validate("192.168.1.1-192.168.2.254"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434/24"));
        CATCH_REQUIRE(address_validator->validate(":5/255.255.255.0"));
        CATCH_REQUIRE(address_validator->validate("# comment with a hash and allowed before\n:5/255.255.255.0"));
        CATCH_REQUIRE(address_validator->validate("::"));
        CATCH_REQUIRE(address_validator->validate("::1 # this is the IPv6 loopback"));
        CATCH_REQUIRE(address_validator->validate("f801::5/48"));
        CATCH_REQUIRE(address_validator->validate("f801::5/48 ; a comment at the end of the line works too"));
        CATCH_REQUIRE(address_validator->validate("")); // default address & port
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: allow all (default is IPv6 here)")
    {
        advgetopt::string_list_t const flags{
            "address=commas spaces newlines range lookup",
            "port=yes",
            "mask=address",
            "comment",
            "defaults=[ff01::345a:123b]:8080/32",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
        CATCH_REQUIRE(address_validator->validate("192.168.1.1\n; and comments are allowed after"));
        CATCH_REQUIRE(address_validator->validate("192.168.1.1-192.168.2.254"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434/24"));
        CATCH_REQUIRE(address_validator->validate(":5/255.255.255.0"));
        CATCH_REQUIRE(address_validator->validate("# comment with a hash and allowed before\n:5/255.255.255.0"));
        CATCH_REQUIRE(address_validator->validate("::"));
        CATCH_REQUIRE(address_validator->validate("::1 # this is the IPv6 loopback"));
        CATCH_REQUIRE(address_validator->validate("f801::5/48"));
        CATCH_REQUIRE(address_validator->validate("f801::5/48 ; a comment at the end of the line works too"));
        CATCH_REQUIRE(address_validator->validate("")); // default address & port
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: prevent defaults")
    {
        advgetopt::string_list_t const flags{
            "address=required",
            "port=required",
            "defaults=yes",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1:4040"));
        CATCH_REQUIRE_FALSE(address_validator->validate("192.168.1.1"));
        CATCH_REQUIRE_FALSE(address_validator->validate(":4040"));
        CATCH_REQUIRE_FALSE(address_validator->validate(""));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: default improperly uses a range")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: the default address \"192.168.19.1-192.168.19.254\" could not be parsed properly. Error: Invalid address in \"192.168.19.1-192.168.19.254\" error -2 -- Name or service not known (errno: 22 -- Invalid argument).\n");
        advgetopt::string_list_t const flags{
            "defaults=192.168.19.1-192.168.19.254",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1:4040"));
        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
        CATCH_REQUIRE(address_validator->validate(":4040"));
        CATCH_REQUIRE(address_validator->validate(""));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: allow hash comments only")
    {
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=hash",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1 # this should work, right?"));
        CATCH_REQUIRE(address_validator->validate("  192.168.3.3:401 # this should work, right?"));
        CATCH_REQUIRE_FALSE(address_validator->validate("172.17.0.10 ; however, this one fails"));
        CATCH_REQUIRE_FALSE(address_validator->validate("  10.0.0.10:5434 ; however, this one fails"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: invalid mask (too many values)")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: the \"mask\" option expects one of \"yes\", \"no\", or \"address\".");
        advgetopt::string_list_t const flags{
            "mask=one value or error",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: invalid mask (undefined value)")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: value \"undefined\" is unexpected for the \"mask\" option which expects one of \"yes\", \"no\", or \"address\".");
        advgetopt::string_list_t const flags{
            "mask=undefined",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: port expects one value")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: the port=... option expects one of \"yes\", \"no\", or \"required\".");
        advgetopt::string_list_t const flags{
            "port=1 2 3",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: port undefined value")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: unexpected value \"undefined\" for the port=... option which expects one of \"yes\", \"no\", or \"required\".");
        advgetopt::string_list_t const flags{
            "port=undefined",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: allow semi-colon comments only")
    {
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=semicolon",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1 ; this should work, right?"));
        CATCH_REQUIRE(address_validator->validate("  192.168.3.3:401 ; this should work, right?"));
        CATCH_REQUIRE_FALSE(address_validator->validate("172.17.0.10 # however, this one fails"));
        CATCH_REQUIRE_FALSE(address_validator->validate("  10.0.0.10:5434 # however, this one fails"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: explicit no comments")
    {
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=no",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE_FALSE(address_validator->validate("192.168.1.1 ; this should fail"));
        CATCH_REQUIRE_FALSE(address_validator->validate("  192.168.3.3:401 ; this should fail"));
        CATCH_REQUIRE_FALSE(address_validator->validate("172.17.0.10 # this should fail"));
        CATCH_REQUIRE_FALSE(address_validator->validate("  10.0.0.10:5434 # this should fail"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: explicit comments (i.e. =yes)")
    {
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=yes",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1 ; this works"));
        CATCH_REQUIRE(address_validator->validate("  192.168.3.3:401 ; this works"));
        CATCH_REQUIRE(address_validator->validate("172.17.0.10 # this works"));
        CATCH_REQUIRE(address_validator->validate("  10.0.0.10:5434 # this works"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: explicit all comments supported (i.e. =all)")
    {
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=all",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1 ; this works"));
        CATCH_REQUIRE(address_validator->validate("  192.168.3.3:401 ; this works"));
        CATCH_REQUIRE(address_validator->validate("172.17.0.10 # this works"));
        CATCH_REQUIRE(address_validator->validate("  10.0.0.10:5434 # this works"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: invalid comment=... value")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: value \"unsupported\" is unexpected for the \"comment\" option which expects one of \"yes\", \"all\", \"hash\", \"semicolon\" or \"no\".");
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=unsupported",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate(" 192.168.1.1 "));
        CATCH_REQUIRE_FALSE(address_validator->validate("  192.168.3.3:401 ; this fails"));
        CATCH_REQUIRE(address_validator->validate("172.17.0.10"));
        CATCH_REQUIRE_FALSE(address_validator->validate("  10.0.0.10:5434 # this fails"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: invalid comment=... with multiple values")
    {
        SNAP_CATCH2_NAMESPACE::push_expected_log("error: the \"comment\" option expects one of \"yes\", \"all\", \"hash\", \"semicolon\" or \"no\".");
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
            "comment=hash semicolon",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
        SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate(" 192.168.1.1 "));
        CATCH_REQUIRE_FALSE(address_validator->validate("  192.168.3.3:401 ; this fails"));
        CATCH_REQUIRE(address_validator->validate("172.17.0.10"));
        CATCH_REQUIRE_FALSE(address_validator->validate("  10.0.0.10:5434 # this fails"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: allow \"nothing\"")
    {
        advgetopt::string_list_t const flags{
            "address=no",
            "port=yes",
        };
        advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));

        CATCH_REQUIRE(address_validator != nullptr);
        CATCH_REQUIRE(address_validator->name() == "address");

        CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
        CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
        CATCH_REQUIRE(address_validator->validate("::"));
        CATCH_REQUIRE(address_validator->validate("f801::5"));
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

    CATCH_START_SECTION("validator: address=no prevents any other option")
    {
        std::vector<std::string> options = {
            "commas",
            "lookup",
            "newlines",
            "range",
            "required",
            "spaces",
            "yes",
        };
        for(auto const & opt : options)
        {
            std::string address("address=");
            if((rand() & 1) == 0)
            {
                address += "no " + opt;
            }
            else
            {
                address += opt + " no";
            }
            advgetopt::string_list_t const flags{
                address,
                "port=yes",
            };
            SNAP_CATCH2_NAMESPACE::push_expected_log("error: the \"no\" option in the address=... option must be used by itself.");
            advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
            SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

            CATCH_REQUIRE(address_validator != nullptr);
            CATCH_REQUIRE(address_validator->name() == "address");

            CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
            CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
            CATCH_REQUIRE(address_validator->validate("::"));
            CATCH_REQUIRE(address_validator->validate("f801::5"));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: address=<invalid flag name>")
    {
        // first character of each supported/valid flag to hit that break;
        // statement in the address sub-switch()
        //
        std::vector<std::string> options = {
            "console",
            "loop",
            "now",
            "real",
            "slice",
            "yield",
            "zero", // plus one that has no case ...
        };
        for(auto const & opt : options)
        {
            advgetopt::string_list_t const flags{
                "address=" + opt + " required",
                "port=yes",
            };
            SNAP_CATCH2_NAMESPACE::push_expected_log(
                  "error: the \""
                + opt
                + "\" parameter is not understood by the \"address\" options.");
            advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
            SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

            CATCH_REQUIRE(address_validator != nullptr);
            CATCH_REQUIRE(address_validator->name() == "address");

            CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
            CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
            CATCH_REQUIRE(address_validator->validate("::"));
            CATCH_REQUIRE(address_validator->validate("f801::5"));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: address=<required vs yes>")
    {
        // first character of each supported/valid flag to hit that break;
        // statement in the address sub-switch()
        //
        std::vector<std::string> options = {
            "address=required yes",
            "address=yes required",
        };
        for(auto const & opt : options)
        {
            advgetopt::string_list_t const flags{
                opt,
                "port=yes",
            };
            SNAP_CATCH2_NAMESPACE::push_expected_log(
                  "error: the \"yes\" and \"required\" options are mutually exclusive, use one or the other (or none, which defaults to \"yes\").");
            advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
            SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

            CATCH_REQUIRE(address_validator != nullptr);
            CATCH_REQUIRE(address_validator->name() == "address");

            CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
            CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
            CATCH_REQUIRE(address_validator->validate("::"));
            CATCH_REQUIRE(address_validator->validate("f801::5"));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator: <invalid parameter name>=<whatever>")
    {
        // first character of each supported/valid flag to hit that break;
        // statement in the address sub-switch()
        //
        std::vector<std::string> parameters = {
            "audio",
            "call",
            "drawing",
            "mug",
            "paint",
            "zero", // plus one that has no case ...
        };
        for(auto const & p : parameters)
        {
            advgetopt::string_list_t const flags{
                p + "=yes",
            };
            SNAP_CATCH2_NAMESPACE::push_expected_log(
                  "error: \""
                + p
                + "=yes\" is not a known option for the address validator.");
            advgetopt::validator::pointer_t address_validator(advgetopt::validator::create("address", flags));
            SNAP_CATCH2_NAMESPACE::expected_logs_stack_is_empty();

            CATCH_REQUIRE(address_validator != nullptr);
            CATCH_REQUIRE(address_validator->name() == "address");

            CATCH_REQUIRE(address_validator->validate("192.168.1.1"));
            CATCH_REQUIRE(address_validator->validate("10.0.0.10:5434"));
            CATCH_REQUIRE(address_validator->validate("::"));
            CATCH_REQUIRE(address_validator->validate("f801::5"));
        }
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
