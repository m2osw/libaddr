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
 * \brief Test the IPv6 interface.
 *
 * These test verify that the IPv6 side of things function as expected.
 *
 * Note that some of the tests between the IPv4 and IPv6 overlap. Here
 * you mainly find the IPv6 side of things.
 *
 * Also, the IPv6 tests include a certain number of default/global
 * tests because internally the addr class implements an IPv6 object.
 */

// addr
//
#include    <libaddr/iface.h>


// self
//
#include    "catch_main.h"


// libutf8
//
#include    <libutf8/locale.h>


// snapdev
//
#include    <snapdev/int128_literal.h>
#include    <snapdev/ostream_int128.h>
#include    <snapdev/string_replace_many.h>


// last include
//
#include    <snapdev/poison.h>


using namespace snapdev::literals;



/** \brief Details used by the addr class implementation.
 *
 * We have a function to check whether an address is part of
 * the interfaces of your computer. This check requires the
 * use of a `struct ifaddrs` and as such it requires to
 * delete that structure. We define a deleter for that
 * structure here.
 */
namespace
{

/** \brief Close a socket.
 *
 * This deleter is used to make sure all the sockets get closed on exit.
 *
 * \param[in] s  The socket to close.
 */
void socket_deleter(int * s)
{
    close(*s);
}


}
// no name namespace



CATCH_TEST_CASE("ipv6::invalid_input", "[ipv6]")
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv6::invalid_input: set IPv6 with an invalid family")
        {
            struct sockaddr_in6 in6 = sockaddr_in6();
            do
            {
                in6.sin6_family = rand();
            }
            while(in6.sin6_family == AF_INET6);
            in6.sin6_port = rand();
            for(int idx(0); idx < 8; ++idx)
            {
                in6.sin6_addr.s6_addr16[idx] = rand();
            }
            CATCH_REQUIRE_THROWS_AS(a.set_ipv6(in6), addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(addr::addr(in6), addr::addr_invalid_argument);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv6 addresses")
    {
        CATCH_START_SECTION("ipv6::invalid_input: bad address")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("[{bad-ip}]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Invalid address in \"{bad-ip}\" error -2 -- Name or service not known (errno: 22 -- Invalid argument).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: missing ']'")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "IPv6 is missing the ']' ([1:2:3:4:5:6:7).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: required address")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_REQUIRED_ADDRESS, true);
            addr::addr_range::vector_t ips(p.parse("[]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Required address is missing.\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv4 ports")
    {
        CATCH_START_SECTION("ipv6::invalid_input: required port")
        {
            // optional + required -> required
            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::allow_t::ALLOW_REQUIRED_PORT, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Required port is missing.\n");
                CATCH_REQUIRE(ips.size() == 0);
            }

            // only required -> required just the same
            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                p.set_allow(addr::allow_t::ALLOW_REQUIRED_PORT, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Required port is missing.\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: port not allowed")
        {
            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:123"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Port not allowed ([1:2:3:4:5:6:7:8]:123).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }

            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                addr::addr_range::vector_t ips(p.parse("1:2:3:4:5:6:7:8:123.5"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid address in \"1:2:3:4:5:6:7:8:123.5\" error -2 -- Name or service not known (errno: 22 -- Invalid argument).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with invalid masks")
    {
        CATCH_START_SECTION("ipv6::invalid_input: really large numbers (over 1000)")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 10001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Mask size too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }

            // WARNING: I removed the first removal of the '[' and ']'
            //          the mask as a number should never have to be between
            //          square brackets, only addresses so here we have the
            //          same case as the block beforehand
            //
            // in case the number is between square brackets it looks like
            // an IPv4 to getaddrinfo() so we get a different error...
            // (i.e. the '[' is not a digit so we do not get the "too large"
            // error...)
            //
            //for(int idx(0); idx < 5; ++idx)
            //{
            //    int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            //    int const port(rand() & 0xFFFF);
            //    int const mask((rand() & 0xFF) + 10001);
            //    addr::addr_parser p;
            //    p.set_protocol(proto);
            //    p.set_allow(addr::allow_t::ALLOW_MASK, true);
            //    p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            //    addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/" + std::to_string(mask)));
            //    CATCH_REQUIRE(p.has_errors());
            //    CATCH_REQUIRE(p.error_count() == 1);
            //    CATCH_REQUIRE(p.error_messages() == "Mask size too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
            //    CATCH_REQUIRE(ips.size() == 0);
            //}

            // an empty address with a mask too large gets us to another place
            //
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 10001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Mask size too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }

            // an empty address with a mask too large gets us to another place
            //
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF));
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(port) + "/" + std::to_string(mask) + "q"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid mask in \"/" + std::to_string(mask) + "q\", error -2 -- Name or service not known (errno: 0 -- Success).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: ipv6 mask is limited between 0 and 128")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 129);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Unsupported mask size (" + std::to_string(mask) + ", expected 128 at the most for an IPv6).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: ipv6 mask cannot use name")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[localhost]"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid mask in \"/[localhost]\", error -2 -- Name or service not known (errno: 0 -- Success).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: ipv6 mask must be between '[...]'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/::3"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The address uses the IPv6 syntax, the mask cannot use IPv4.\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: ipv6 mask missing the ']'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[::3"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The IPv6 mask is missing the ']' ([::3).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: ipv6 mask with an ipv4 in the '[...]'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[1.2.3.4]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Incompatible address between the address and mask address (first was an IPv6 second an IPv4).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: verify default address")
        {
            addr::addr_parser p;

            CATCH_REQUIRE_THROWS_AS(p.set_default_address("[4:5:4:5:7:8:7:8"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "");

            p.set_default_address("[1:7:1:7:1:7:1:7]");
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "1:7:1:7:1:7:1:7");

            CATCH_REQUIRE_THROWS_AS(p.set_default_address("[9:5:9:5:9:8:9:8"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "1:7:1:7:1:7:1:7");

            p.set_default_address("12.55.1.9");
            CATCH_REQUIRE(p.get_default_address4() == "12.55.1.9");
            CATCH_REQUIRE(p.get_default_address6() == "1:7:1:7:1:7:1:7");

            CATCH_REQUIRE_THROWS_AS(p.set_default_address("[9:f00f:9:e00e:9:d00d:9:c00c"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_default_address4() == "12.55.1.9");
            CATCH_REQUIRE(p.get_default_address6() == "1:7:1:7:1:7:1:7");

            p.set_default_address("");
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::invalid_input: verify default mask")
        {
            addr::addr_parser p;

            CATCH_REQUIRE_THROWS_AS(p.set_default_mask("[4:5:4:5:7:8:7:8"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "");

            p.set_default_mask("[1:7:1:7:1:7:1:7]");
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "1:7:1:7:1:7:1:7");

            CATCH_REQUIRE_THROWS_AS(p.set_default_mask("[9:5:9:5:9:8:9:8"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "1:7:1:7:1:7:1:7");

            p.set_default_mask("12.55.1.9");
            CATCH_REQUIRE(p.get_default_mask4() == "12.55.1.9");
            CATCH_REQUIRE(p.get_default_mask6() == "1:7:1:7:1:7:1:7");

            CATCH_REQUIRE_THROWS_AS(p.set_default_mask("[9:f00f:9:e00e:9:d00d:9:c00c"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_default_mask4() == "12.55.1.9");
            CATCH_REQUIRE(p.get_default_mask6() == "1:7:1:7:1:7:1:7");

            p.set_default_mask("55");
            CATCH_REQUIRE(p.get_default_mask4() == "12.55.1.9");
            CATCH_REQUIRE(p.get_default_mask6() == "55");

            p.set_default_mask("16");
            CATCH_REQUIRE(p.get_default_mask4() == "16");
            CATCH_REQUIRE(p.get_default_mask6() == "55");

            p.set_default_mask("");
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "");

            for(int idx(-10); idx < 0; ++idx)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                          p.set_default_mask(std::to_string(idx))
                        , addr::addr_invalid_argument
                        , Catch::Matchers::ExceptionMessage(
                                  "addr_error: a mask number must be between 0 and 128."));
            }

            for(int idx(129); idx <= 135; ++idx)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                          p.set_default_mask(std::to_string(idx))
                        , addr::addr_invalid_argument
                        , Catch::Matchers::ExceptionMessage(
                                  "addr_error: a mask number must be between 0 and 128."));
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::address", "[ipv6]")
{
    CATCH_GIVEN("addr() with an IPv6")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv6::address: default is 128 bit set to zero")
        {
            CATCH_REQUIRE(a.ip_to_uint128() == 0_uint128);

            // any address is not the next/previous of itself, even 0
            CATCH_REQUIRE_FALSE(a.is_next(a));
            CATCH_REQUIRE_FALSE(a.is_previous(a));

            // first address -N is still the first address
            //
            addr::addr b(a);
            CATCH_REQUIRE(a == b);
            b--;
            CATCH_REQUIRE(a == b);
            --b;
            CATCH_REQUIRE(a == b);
            for(int idx(0); idx < 10; ++idx)
            {
                b -= rand() % 0xFFFF;
                CATCH_REQUIRE(a == b);

                addr::addr c(b - rand() % 0xFFFF);
                CATCH_REQUIRE(a == c);
            }

            __int128 diff(a - b);
            CATCH_REQUIRE(diff == 0_int128);
        }
        CATCH_END_SECTION();

        CATCH_START_SECTION("ipv6::address: parse the default IPv6 address \"[::]\" and \"::\"")
        {
            int proto[] = { IPPROTO_TCP, IPPROTO_UDP, IPPROTO_IP };

            CATCH_REQUIRE(a.ip_to_uint128() == 0_uint128);

            {
                // we do not specify the protocol so we receive one response
                // per protocol (TCP, UDP, and IP)
                //
                addr::addr_parser p;
                addr::addr_range::vector_t ips(p.parse("[::]"));
                CATCH_REQUIRE(ips.size() == std::size(proto));

                // the IP address itself is ANY
                // the protocol varies, however
                //
                for(std::size_t idx(0); idx < std::size(proto); ++idx)
                {
                    CATCH_REQUIRE(ips[idx].has_from());
                    CATCH_REQUIRE_FALSE(ips[idx].has_to());
                    CATCH_REQUIRE(ips[idx].get_from() == a);
                    CATCH_REQUIRE(ips[idx].get_from().get_protocol() == proto[idx]);
                }
            }

            {
                addr::addr_parser p;
                addr::addr_range::vector_t ips(p.parse("::"));
                CATCH_REQUIRE(ips.size() == std::size(proto));

                // the IP address itself is ANY
                // the protocol varies, however
                //
                for(std::size_t idx(0); idx < std::size(proto); ++idx)
                {
                    CATCH_REQUIRE(ips[idx].has_from());
                    CATCH_REQUIRE_FALSE(ips[idx].has_to());
                    CATCH_REQUIRE(ips[idx].get_from() == a);
                    CATCH_REQUIRE(ips[idx].get_from().get_protocol() == proto[idx]);
                }
            }
        }
        CATCH_END_SECTION();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        CATCH_START_SECTION("ipv6::address: verify last IPv6 address")
        {
            CATCH_REQUIRE(a.ip_to_uint128() == 0_uint128);

            struct sockaddr_in6 in6 = sockaddr_in6();
            in6.sin6_family = AF_INET6;
            in6.sin6_port = htons(rand());
            in6.sin6_addr.s6_addr32[0] = 0xFFFFFFFF;
            in6.sin6_addr.s6_addr32[1] = 0xFFFFFFFF;
            in6.sin6_addr.s6_addr32[2] = 0xFFFFFFFF;
            in6.sin6_addr.s6_addr32[3] = 0xFFFFFFFF;
            a.set_ipv6(in6);

            // any address is not the next/previous of itself, even "-1"
            CATCH_REQUIRE_FALSE(a.is_next(a));
            CATCH_REQUIRE_FALSE(a.is_previous(a));

            // last address +N is still the last address
            //
            addr::addr b(a);
            CATCH_REQUIRE(a == b);
            b++;
            CATCH_REQUIRE(a == b);
            ++b;
            CATCH_REQUIRE(a == b);
            for(int idx(0); idx < 10; ++idx)
            {
                b += rand() % 0xFFFF;
                CATCH_REQUIRE(a == b);

                addr::addr c(b + rand() % 0xFFFF);
                CATCH_REQUIRE(a == c);
            }

            __int128 diff(a - b);
            CATCH_REQUIRE(diff == 0_int128);
        }
        CATCH_END_SECTION();
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        CATCH_START_SECTION("ipv6::address: set_ipv6() / get_ipv6()")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = rand();
                in6.sin6_addr.s6_addr16[1] = rand();
                in6.sin6_addr.s6_addr16[2] = rand();
                in6.sin6_addr.s6_addr16[3] = rand();
                in6.sin6_addr.s6_addr16[4] = rand();
                in6.sin6_addr.s6_addr16[5] = rand();
                in6.sin6_addr.s6_addr16[6] = rand();
                in6.sin6_addr.s6_addr16[7] = rand();

                unsigned __int128 address(0_uint128);
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 0]) << 120;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 1]) << 112;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 2]) << 104;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 3]) <<  96;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 4]) <<  88;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 5]) <<  80;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 6]) <<  72;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 7]) <<  64;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 8]) <<  56;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[ 9]) <<  48;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[10]) <<  40;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[11]) <<  32;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[12]) <<  24;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[13]) <<  16;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[14]) <<   8;
                address |= static_cast<unsigned __int128>(in6.sin6_addr.s6_addr[15]) <<   0;

                // set the address
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.ip_to_uint128() == address);

                // test constructor
                //
                addr::addr b(in6);
                struct sockaddr_in6 out6;
                b.get_ipv6(out6);
                CATCH_REQUIRE(memcmp(&out6, &in6, sizeof(struct sockaddr_in)) == 0);

                // test set
                //
                a.set_ipv6(in6);
                a.get_ipv6(out6);
                CATCH_REQUIRE(memcmp(&out6, &in6, sizeof(struct sockaddr_in)) == 0);
                CATCH_REQUIRE(a.ip_to_uint128() == address);

                struct sockaddr_in6 in6b = sockaddr_in6();
                in6b.sin6_family = AF_INET6;
                in6b.sin6_port = htons(rand());
                in6b.sin6_addr.s6_addr16[0] = rand();
                in6b.sin6_addr.s6_addr16[1] = rand();
                in6b.sin6_addr.s6_addr16[2] = rand();
                in6b.sin6_addr.s6_addr16[3] = rand();
                in6b.sin6_addr.s6_addr16[4] = rand();
                in6b.sin6_addr.s6_addr16[5] = rand();
                in6b.sin6_addr.s6_addr16[6] = rand();
                in6b.sin6_addr.s6_addr16[7] = rand();

                unsigned __int128 new_address(0_uint128);
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 0]) << 120;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 1]) << 112;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 2]) << 104;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 3]) <<  96;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 4]) <<  88;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 5]) <<  80;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 6]) <<  72;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 7]) <<  64;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 8]) <<  56;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[ 9]) <<  48;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[10]) <<  40;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[11]) <<  32;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[12]) <<  24;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[13]) <<  16;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[14]) <<   8;
                new_address |= static_cast<unsigned __int128>(in6b.sin6_addr.s6_addr[15]) <<   0;

                a.ip_from_uint128(new_address);
                CATCH_REQUIRE(a.ip_to_uint128() == new_address);

                if(new_address >= 10)
                {
                    addr::addr const e(a + -10);
                    CATCH_REQUIRE(e.ip_to_uint128() == new_address - 10);

                    addr::addr f(a);
                    f += -10;
                    CATCH_REQUIRE(e.ip_to_uint128() == new_address - 10);
                }
                if(new_address <= 0xffffffffffffffffffffffffffffffff_uint128 - 10)
                {
                    addr::addr const e(a - -10);
                    CATCH_REQUIRE(e.ip_to_uint128() == new_address + 10);

                    addr::addr f(a);
                    f -= -10;
                    CATCH_REQUIRE(e.ip_to_uint128() == new_address + 10);
                }

                struct sockaddr_in6 in6c(in6b);
                for(int p(16); p > 0; )
                {
                    --p;
                    ++in6c.sin6_addr.s6_addr[p];
                    if(in6c.sin6_addr.s6_addr[p] != 0)
                    {
                        break;
                    }
                }

                addr::addr const c(in6c);
                CATCH_REQUIRE(a.is_next(c));

                struct sockaddr_in6 in6d(in6b);
                for(int p(16); p > 0; )
                {
                    --p;
                    --in6d.sin6_addr.s6_addr[p];
                    if(in6d.sin6_addr.s6_addr[p] != 0xFF)
                    {
                        break;
                    }
                }

                addr::addr const d(in6d);
                CATCH_REQUIRE(a.is_previous(d));
            }
        }
        CATCH_END_SECTION()
#pragma GCC diagnostic pop

        CATCH_START_SECTION("ipv6::address: set_ipv6() check to_ipv6_string()")
        {
            // this requires those locales to be installed
            // in my case, I install them all with:
            //
            //    sudo dpkg-reconfigure locales
            //    (and in the dialog, choose "Select All" to get everything)
            //
            // you can also do them one at a time
            //
            //    sudo locale-gen fr_FR.utf8
            //    sudo update-locale
            //
            // to see the list of locales you have defined use:
            //
            //    locale -a
            //
            std::vector<char const *> locales = {
                "en_US.utf8",
                "fr_FR.utf8",
                "ja_JP.utf8",
                "zh_SG.utf8",
            };
            for(auto const & l : locales)
            {
                if(!libutf8::has_system_locale(l))
                {
                    std::cout << "--- skipping locale \"" << l << "\" (not found on this system)." << std::endl;
                    continue;
                }
                std::cout << "--- testing locale \"" << l << "\"" << std::endl;
                std::locale const loc(l);

                std::map<addr::string_ip_t, std::string> addr_vec;
                addr::addr::vector_t addresses;
                for(int idx(0); idx < 10; ++idx)
                {
                    struct sockaddr_in6 in6 = sockaddr_in6();
                    in6.sin6_family = AF_INET6;
                    in6.sin6_port = htons(rand());
                    for(int j(0); j < 8; ++j)
                    {
                        // avoid any zeroes so that way we do not have
                        // to handle the "::" syntax
                        do
                        {
                            in6.sin6_addr.s6_addr16[j] = rand();
                        }
                        while(in6.sin6_addr.s6_addr16[j] == 0);
                    }

                    std::stringstream ip_buf;
                    ip_buf << std::hex
                           << ntohs(in6.sin6_addr.s6_addr16[0])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[1])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[2])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[3])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[4])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[5])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[6])
                           << ":"
                           << ntohs(in6.sin6_addr.s6_addr16[7]);
                    std::string const ip(ip_buf.str());

                    std::string port_str(std::to_string(static_cast<int>(htons(in6.sin6_port))));

                    // check IPv6 as a string
                    //
                    a.set_ipv6(in6);
                    addresses.push_back(a);
                    CATCH_REQUIRE(a.get_str_port() == port_str);
                    {
                        std::string const str(a.to_ipv6_string(addr::STRING_IP_ADDRESS));
                        if(addr_vec[addr::STRING_IP_ADDRESS] != std::string())
                        {
                            addr_vec[addr::STRING_IP_ADDRESS] += ",";
                        }
                        addr_vec[addr::STRING_IP_ADDRESS] += str;
                        CATCH_REQUIRE(str == ip);
                    }
                    {
                        std::string const str(a.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS));
                        if(addr_vec[addr::STRING_IP_BRACKET_ADDRESS] != std::string())
                        {
                            addr_vec[addr::STRING_IP_BRACKET_ADDRESS] += ",";
                        }
                        addr_vec[addr::STRING_IP_BRACKET_ADDRESS] += str;
                        CATCH_REQUIRE(str == "[" + ip + "]");
                    }
                    {
                        std::string const str(a.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT));
                        if(addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT] != std::string())
                        {
                            addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT] += ",";
                        }
                        addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT] += str;
                        CATCH_REQUIRE(str == "[" + ip + "]:" + port_str);
                    }
                    {
                        std::string const str(a.to_ipv6_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK));
                        if(addr_vec[addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK] != std::string())
                        {
                            addr_vec[addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK] += ",";
                        }
                        addr_vec[addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK] += str;
                        CATCH_REQUIRE(str == ip + "/128");
                    }
                    {
                        std::string const str(a.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK));
                        if(addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK] != std::string())
                        {
                            addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK] += ",";
                        }
                        addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK] += str;
                        CATCH_REQUIRE(str == "[" + ip + "]/128");
                    }
                    {
                        std::string const str(a.to_ipv6_string(addr::STRING_IP_ALL));
                        if(addr_vec[addr::STRING_IP_ALL] != std::string())
                        {
                            addr_vec[addr::STRING_IP_ALL] += ",";
                        }
                        addr_vec[addr::STRING_IP_ALL] += str;
                        CATCH_REQUIRE(str == "[" + ip + "]:" + port_str + "/128");
                    }

                    // the ostream functions
                    {
                        std::stringstream ss;
                        ss << a; // mode defaults to ALL
                        CATCH_REQUIRE(ss.str() == "[" + ip + "]:" + port_str + "/128");
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_ADDRESS) << a;
                        CATCH_REQUIRE(ss.str() == ip);
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS) << a;
                        CATCH_REQUIRE(ss.str() == "[" + ip + "]");
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) << a;
                        CATCH_REQUIRE(ss.str() == "[" + ip + "]:" + port_str);
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK) << a;
                        CATCH_REQUIRE(ss.str() == ip + "/128");
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK) << a;
                        CATCH_REQUIRE(ss.str() == "[" + ip + "]/128");
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_ALL) << a;
                        CATCH_REQUIRE(ss.str() == "[" + ip + "]:" + port_str + "/128");
                    }
                    {
                        std::stringstream ss;
                        ss << addr::setaddrmode(addr::STRING_IP_PORT);
                        ss.copyfmt(std::cout); // we did not change the mode of std::cout so here we expect STRING_IP_ALL after the copy
                        ss << a;
                        CATCH_REQUIRE(ss.str() == "[" + ip + "]:" + port_str + "/128");
                    }
                    {
                        std::stringstream sss;
                        std::stringstream ssd;
                        sss << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT);
                        ssd << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS);
                        ssd.copyfmt(sss);
                        ssd << a;
                        CATCH_REQUIRE(ssd.str() == "[" + ip + "]:" + port_str);
                    }
                }

                {
                    std::stringstream ss;
                    ss << addresses;
                    ss.imbue(loc);
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_ALL]);
                }
                {
                    std::stringstream ss;
                    ss.imbue(loc);
                    ss << addr::setaddrsep(" ");
                    ss.imbue(loc);
                    ss << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_ALL], {{",", " "}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrmode(addr::STRING_IP_ADDRESS);
                    ss.imbue(loc);
                    ss << addresses;
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_ADDRESS]);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrsep("|") << addr::setaddrmode(addr::STRING_IP_ADDRESS) << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_ADDRESS], {{",", "|"}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS) << addresses;
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_BRACKET_ADDRESS]);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrsep(";") << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS) << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_BRACKET_ADDRESS], {{",", ";"}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrmode(addr::STRING_IP_ADDRESS | addr::STRING_IP_PORT) << addresses;
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT]);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrsep("+") << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT);
                    ss.imbue(loc);
                    ss << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT], {{",", "+"}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
                {
                    std::stringstream ss;
                    ss.imbue(loc);
                    ss << addr::setaddrmode(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK) << addresses;
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK]);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrsep(", ") << addr::setaddrmode(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK) << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK], {{",", ", "}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK) << addresses;
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK]);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrsep("$") << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK) << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK], {{",", "$"}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
                {
                    std::stringstream ss;
                    ss << addr::setaddrmode(addr::STRING_IP_ALL);
                    ss.imbue(loc);
                    ss << addresses;
                    CATCH_REQUIRE(ss.str() == addr_vec[addr::STRING_IP_ALL]);
                }
                {
                    std::stringstream ss;
                    ss.imbue(loc);
                    ss << addr::setaddrsep("\n") << addr::setaddrmode(addr::STRING_IP_ALL) << addresses;
                    std::string const expected(snapdev::string_replace_many(addr_vec[addr::STRING_IP_ALL], {{",", "\n"}}));
                    CATCH_REQUIRE(ss.str() == expected);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: name of various IPs")
        {
            struct sockaddr_in6 in6 = sockaddr_in6();
            in6.sin6_family = AF_INET6;
            in6.sin6_port = htons(rand());

            // verify network type
            //
            a.set_ipv6(in6);
            CATCH_REQUIRE(a.get_name() == std::string()); // no name for "any" (TCP)

            a.set_protocol(IPPROTO_UDP);
            CATCH_REQUIRE(a.get_name() == std::string()); // no name for "any" (UDP)

            in6 = sockaddr_in6();
            in6.sin6_family = AF_INET6;
            in6.sin6_port = htons(rand());
            in6.sin6_addr.s6_addr16[7] = htons(1);
            a.set_ipv6(in6);
            char hostname[HOST_NAME_MAX + 1];
            hostname[HOST_NAME_MAX] = '\0';
            CATCH_REQUIRE(gethostname(hostname, sizeof(hostname)) == 0);
            CATCH_REQUIRE(hostname[0] != '\0');
            std::string localhost(a.get_name());
            bool const localhost_flag(localhost == hostname || localhost == "ip6-localhost");
            CATCH_REQUIRE(localhost_flag);

            CATCH_REQUIRE(addr::find_addr_interface(a, false) != nullptr);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv6 addresses")
    {
        CATCH_START_SECTION("ipv6::address: verify basics")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            CATCH_REQUIRE(r.has_from());
            CATCH_REQUIRE_FALSE(r.has_to());
            CATCH_REQUIRE_FALSE(r.is_range());
            CATCH_REQUIRE_FALSE(r.is_empty());
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ADDRESS) == "1:2:3:4:5:6:7:8");
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS) == "[1:2:3:4:5:6:7:8]");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1:2:3:4:5:6:7:8");
            CATCH_REQUIRE(f.get_port() == 0);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
            CATCH_REQUIRE_FALSE(f.is_lan());
            CATCH_REQUIRE_FALSE(f.is_lan(true));
            CATCH_REQUIRE_FALSE(f.is_lan(false));
            CATCH_REQUIRE(f.is_wan());
            CATCH_REQUIRE(f.is_wan(true));
            CATCH_REQUIRE(f.is_wan(false));
            uint8_t mask[16] = {};
            f.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: default address")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_default_address("5:5:5:5:5:5:5:5");
            addr::addr_range::vector_t ips(p.parse(""));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            CATCH_REQUIRE(r.has_from());
            CATCH_REQUIRE_FALSE(r.has_to());
            CATCH_REQUIRE_FALSE(r.is_range());
            CATCH_REQUIRE_FALSE(r.is_empty());
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ADDRESS) == "5:5:5:5:5:5:5:5");
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS) == "[5:5:5:5:5:5:5:5]");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5:5:5:5:5:5:5:5");
            CATCH_REQUIRE(f.get_port() == 0);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
            CATCH_REQUIRE_FALSE(f.is_lan());
            CATCH_REQUIRE_FALSE(f.is_lan(true));
            CATCH_REQUIRE_FALSE(f.is_lan(false));
            CATCH_REQUIRE(f.is_wan());
            CATCH_REQUIRE(f.is_wan(true));
            CATCH_REQUIRE(f.is_wan(false));
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: address, no port allowed")
        {
            // specific address with a default
            {
                addr::addr_parser p;
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                p.set_protocol(IPPROTO_TCP);
                p.set_default_address("8:7:6:5:4:3:2:1");
                addr::addr_range::vector_t ips(p.parse("[9:9:9:9:4:3:2:1]"));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ADDRESS) == "9:9:9:9:4:3:2:1");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS) == "[9:9:9:9:4:3:2:1]");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "9:9:9:9:4:3:2:1");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
            }

            // only a default address
            {
                addr::addr_parser p;
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                p.set_protocol(IPPROTO_TCP);
                p.set_default_address("5:1:6:2:7:3:8:4");
                addr::addr_range::vector_t ips(p.parse(""));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ADDRESS) == "5:1:6:2:7:3:8:4");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS) == "[5:1:6:2:7:3:8:4]");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5:1:6:2:7:3:8:4");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with numeric only IPv6 addresses")
    {
        CATCH_START_SECTION("ipv6::address: simple numeric IPv6")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            addr::addr_range::vector_t ips(p.parse("[4::f003:3001:20af]:5093"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);

            addr::addr_range const & r(ips[0]);
            CATCH_REQUIRE(r.has_from());
            CATCH_REQUIRE_FALSE(r.has_to());
            CATCH_REQUIRE_FALSE(r.is_range());
            CATCH_REQUIRE_FALSE(r.is_empty());
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            // getting an IPv4 would throw, which is checked somewhere else
            //CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "4::f003:3001:20af");
            CATCH_REQUIRE(f.get_port() == 5093);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
            CATCH_REQUIRE_FALSE(f.is_lan());
            CATCH_REQUIRE_FALSE(f.is_lan(true));
            CATCH_REQUIRE_FALSE(f.is_lan(false));
            CATCH_REQUIRE(f.is_wan());
            CATCH_REQUIRE(f.is_wan(true));
            CATCH_REQUIRE(f.is_wan(false));
            uint8_t mask[16] = {};
            f.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: invalid IPv6 domain name address when we only accept numeric IPs")
        {
            // this is exactly the same path as the IPv4 test...
            // if we have a named domain then IPv4 fails, IPv6 fails, then we err on it
            //
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            addr::addr_range::vector_t const ips(p.parse("ipv6.example.com:4471"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Unknown address in \"ipv6.example.com\" (no DNS lookup was allowed).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: invalid IPv6 domain name address when we only accept numeric IPs")
        {
            // this is exactly the same path as the IPv4 test...
            // if we have a named domain then IPv4 fails, IPv6 fails, then we err on it
            //
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            p.set_allow(addr::allow_t::ALLOW_PORT, false);
            p.set_default_port(4471);
            addr::addr_range::vector_t const ips(p.parse("[f801::31]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Found a port (\"4471\") when it is not allowed.\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("ipv6::address: a set of IPs and a sort")
    {
        std::string const ip_list("7::-3::,10.0.0.32,192.168.2.15-192.168.2.23,::,5.8.9.11,f801::5553,192.168.2.1-192.168.2.14,::3000-::2000");
        addr::addr_parser p;
        p.set_protocol(IPPROTO_TCP);
        p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
        p.set_allow(addr::allow_t::ALLOW_ADDRESS_RANGE, true);
        CATCH_REQUIRE(p.get_sort_order() == addr::SORT_NO);

        CATCH_START_SECTION("ipv6::address: parse and no sort")
        {
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 8);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.15-192.168.2.23");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
            CATCH_REQUIRE(ips[7].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse and ignore empty")
        {
            addr::sort_t const order(addr::SORT_NO_EMPTY);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 6);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.15-192.168.2.23");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse and full sort")
        {
            addr::sort_t const order(addr::SORT_FULL);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 8);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.15-192.168.2.23");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[7].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse and put IPv6 addresses first")
        {
            addr::sort_t const order(addr::SORT_IPV6_FIRST);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 8);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.15-192.168.2.23");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[7].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse and put IPv4 addresses first")
        {
            addr::sort_t const order(addr::SORT_IPV4_FIRST);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 8);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.15-192.168.2.23");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[7].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse, sort, and merge")
        {
            addr::sort_t const order(addr::SORT_MERGE);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 7);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.23");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse, sort, merge, and put IPv4 first")
        {
            addr::sort_t const order(addr::SORT_MERGE | addr::SORT_IPV4_FIRST);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 7);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.23");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse, sort, merge, and put IPv6 first")
        {
            addr::sort_t const order(addr::SORT_MERGE | addr::SORT_IPV6_FIRST);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 7);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.23");
            CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
            CATCH_REQUIRE(ips[6].to_string(addr::STRING_IP_ADDRESS) == "<empty address range>");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::address: parse, sort, merge, and put IPv6 first")
        {
            // this is the one we expect most users to make use of to
            //   1. ignore empty entries (useless)
            //   2. merge when possible to reduce the number of items
            //   3. handle IPv6 first, then try IPv4 is any available
            //
            addr::sort_t const order(addr::SORT_NO_EMPTY | addr::SORT_MERGE | addr::SORT_IPV6_FIRST);
            p.set_sort_order(order);
            CATCH_REQUIRE(p.get_sort_order() == order);
            addr::addr_range::vector_t const ips(p.parse(ip_list));

            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 5);
            CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
            CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "5.8.9.11");
            CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "10.0.0.32");
            CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.23");
        }
        CATCH_END_SECTION()
    }

    CATCH_START_SECTION("ipv6::address: one side ranges")
    {
        addr::addr_parser p;
        p.set_protocol(IPPROTO_TCP);
        p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
        p.set_allow(addr::allow_t::ALLOW_ADDRESS_RANGE, true);

        std::string const ip_list("-::1,-10.0.0.32,f801::5553-,192.168.2.1-");
        addr::addr_range::vector_t const ips(p.parse(ip_list));

        CATCH_REQUIRE_FALSE(p.has_errors());
        CATCH_REQUIRE(ips.size() == 4);
        CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "-::1");
        CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "-10.0.0.32");
        CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "f801::5553");
        CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::address: test invalid sort (IPv4 vs IPv6)")
    {
        addr::addr_parser p;

        // set something valid
        //
        addr::sort_t const order(addr::SORT_NO_EMPTY);
        p.set_sort_order(order);
        CATCH_REQUIRE(p.get_sort_order() == order);

        // try to set something invalid
        //
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.set_sort_order(addr::SORT_IPV6_FIRST | addr::SORT_IPV4_FIRST)
                , addr::addr_invalid_argument
                , Catch::Matchers::ExceptionMessage(
                          "addr_error: addr_parser::set_sort_order(): flags SORT_IPV6_FIRST and SORT_IPV4_FIRST are mutually exclusive."));

        // verify that the invalid attempt did not change anything
        //
        CATCH_REQUIRE(p.get_sort_order() == order);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::address: parse & sort multi-address separated by '\\n' with '#' comments")
    {
        addr::addr_parser p;
        p.set_protocol(IPPROTO_TCP);
        p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_NEWLINES, true);
        p.set_allow(addr::allow_t::ALLOW_ADDRESS_RANGE, true);
        p.set_allow(addr::allow_t::ALLOW_COMMENT_HASH, true);

        CATCH_REQUIRE(p.get_sort_order() == addr::SORT_NO); // verify default
        addr::sort_t const order(addr::SORT_NO_EMPTY | addr::SORT_MERGE | addr::SORT_IPV6_FIRST);
        p.set_sort_order(order);
        CATCH_REQUIRE(p.get_sort_order() == order);

        std::string const ip_list(
                "9::-a::\n"
                "10.1.0.32\n"
                "192.168.2.23-192.168.2.18\n"
                "#0:0:300f:f00f:3355::3\n"  // commented
                "::1\n"
                "25.8.9.11\n\n"             // extra empty line
                "f801::3332\n"
                "192.168.2.1-192.168.2.14\n"
                "-:45\n"                    // for a range, at least one IP is required
                "a::1-b::3\n\n"             // extra empty line at the end too
                "# an actual comment\n");
        addr::addr_range::vector_t const ips(p.parse(ip_list));

        // note that even though we had errors, the valid IP entries
        // appear in the ips vector and we can test them
        //
        CATCH_REQUIRE(p.has_errors());
        CATCH_REQUIRE(p.error_messages() == "An address range requires at least one of the \"from\" or \"to\" addresses.\n");

        CATCH_REQUIRE(ips.size() == 6);
        CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::1");
        CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "9::-b::3");
        CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "f801::3332");
        CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "10.1.0.32");
        CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "25.8.9.11");
        CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::address: parse & sort multi-address separated by '\\n' with ';' comments")
    {
        addr::addr_parser p;
        p.set_protocol(IPPROTO_TCP);
        p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_NEWLINES, true);
        p.set_allow(addr::allow_t::ALLOW_ADDRESS_RANGE, true);
        p.set_allow(addr::allow_t::ALLOW_COMMENT_SEMICOLON, true);

        CATCH_REQUIRE(p.get_sort_order() == addr::SORT_NO); // verify default
        addr::sort_t const order(addr::SORT_NO_EMPTY | addr::SORT_MERGE | addr::SORT_IPV6_FIRST);
        p.set_sort_order(order);
        CATCH_REQUIRE(p.get_sort_order() == order);

        std::string const ip_list(
                "; list of IPs\n"
                "9::-a::\n"
                "10.1.0.32\n"
                "192.168.2.23-192.168.2.18\n"
                ";0:0:300f:f00f:3355::3\n"  // commented
                "::1\n"
                "25.8.9.11\n\n"             // extra empty line
                "f801::3332\n"
                "192.168.2.1-192.168.2.14\n"
                "-:45\n"                    // for a range, at least one IP is required
                "a::1-b::3\n\n");           // extra empty line at the end too
        addr::addr_range::vector_t const ips(p.parse(ip_list));

        // note that even though we had errors, the valid IP entries
        // appear in the ips vector and we can test them
        //
        CATCH_REQUIRE(p.has_errors());
        CATCH_REQUIRE(p.error_messages() == "An address range requires at least one of the \"from\" or \"to\" addresses.\n");

        CATCH_REQUIRE(ips.size() == 6);
        CATCH_REQUIRE(ips[0].to_string(addr::STRING_IP_ADDRESS) == "::1");
        CATCH_REQUIRE(ips[1].to_string(addr::STRING_IP_ADDRESS) == "9::-b::3");
        CATCH_REQUIRE(ips[2].to_string(addr::STRING_IP_ADDRESS) == "f801::3332");
        CATCH_REQUIRE(ips[3].to_string(addr::STRING_IP_ADDRESS) == "10.1.0.32");
        CATCH_REQUIRE(ips[4].to_string(addr::STRING_IP_ADDRESS) == "25.8.9.11");
        CATCH_REQUIRE(ips[5].to_string(addr::STRING_IP_ADDRESS) == "192.168.2.1-192.168.2.14");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::address: parse invalid range (unknown domain name)")
    {
        addr::addr_parser p;
        p.set_protocol(IPPROTO_TCP);
        p.set_allow(addr::allow_t::ALLOW_ADDRESS_RANGE, true);

// the ::1 address in /etc/hosts used to also be named "localhost"; so we
// were testing a system bug... [the error is still happening if we do not
// set the protocol, although that was not the purpose of this test and
// now we ignore the different protocols in a parsed range]
//
//        addr::addr_range::vector_t const ips1(p.parse("localhost-:45"));
//std::cerr << "got localhost as just one IP?! [" << ips1 << "]\n";
//
//        CATCH_REQUIRE(p.has_errors());
//        CATCH_REQUIRE(p.error_messages() == "The \"from\" of an address range must be exactly one address.\n");
//
//        CATCH_REQUIRE(ips1.empty());
//
//        p.clear_errors();
//        addr::addr_range::vector_t const ips2(p.parse("-localhost:45"));
//
//        CATCH_REQUIRE(p.has_errors());
//        CATCH_REQUIRE(p.error_messages() == "The \"to\" of an address range must be exactly one address.\n");
//
//        CATCH_REQUIRE(ips2.empty());
//
//        p.clear_errors();
        addr::addr_range::vector_t const ips3(p.parse("invalid.from-:45"));

        CATCH_REQUIRE(p.has_errors());
        bool expected(
               p.error_messages() == "Invalid address in \"invalid.from:45\" error -2 -- Name or service not known (errno: 22 -- Invalid argument).\n"
            || p.error_messages() == "Invalid address in \"invalid.from:45\" error -2 -- Name or service not known (errno: 6 -- No such device or address).\n");
        CATCH_REQUIRE(expected);

        CATCH_REQUIRE(ips3.empty());

        // .to is a valid TLD (Tonga) so here I use .tom instead
        //
        p.clear_errors();
        addr::addr_range::vector_t const ips4(p.parse("-invalid.tom:45"));

        CATCH_REQUIRE(p.has_errors());
        expected =
               p.error_messages() == "Invalid address in \"invalid.tom:45\" error -2 -- Name or service not known (errno: 22 -- Invalid argument).\n"
            || p.error_messages() == "Invalid address in \"invalid.tom:45\" error -2 -- Name or service not known (errno: 6 -- No such device or address).\n";
        CATCH_REQUIRE(expected);

        CATCH_REQUIRE(ips4.empty());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("ipv6::ports", "[ipv6]")
{
    CATCH_START_SECTION("ipv6::addr: verify port names")
    {
        // test a few ports that we know are and are not defined in /etc/services
        //
        addr::addr a;

        struct sockaddr_in6 in6 = sockaddr_in6();
        in6.sin6_family = AF_INET6;
        in6.sin6_port = htons(rand());
        do
        {
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[0]);
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[1]);
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[2]);
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[3]);
            a.set_ipv6(in6);
        }
        while(!a.is_valid() || a.is_ipv4());
        CATCH_REQUIRE(!a.is_ipv4());

        struct port_name_t
        {
            int             f_port = 0;
            char const *    f_name = nullptr;
        };
        port_name_t names[] = {
            { 21, "ftp" },
            { 22, "ssh" },
            { 23, "telnet" },
            { 80, "http" },
            { 443, "https" },
            { 4004, "" },
        };

        for(auto n : names)
        {
            a.set_port(n.f_port);
            CATCH_REQUIRE(a.get_port_name() == n.f_name);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::addr: verify protocol names")
    {
        // test a few ports that we know are and are not defined in /etc/services
        //
        addr::addr a;

        struct sockaddr_in6 in6 = sockaddr_in6();
        in6.sin6_family = AF_INET6;
        in6.sin6_port = htons(rand());
        do
        {
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[0]);
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[1]);
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[2]);
            SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[3]);
            a.set_ipv6(in6);
        }
        while(!a.is_valid() || a.is_ipv4());
        CATCH_REQUIRE(!a.is_ipv4());

        struct protocol_name_t
        {
            int             f_protocol = 0;
            char const *    f_name = nullptr;
        };
        protocol_name_t names[] = {
            { 0, "ip" },
            { 6, "tcp" },
            { 17, "udp" },
        };

        for(auto n : names)
        {
            a.set_protocol(n.f_protocol);
            CATCH_REQUIRE(a.get_protocol_name() == n.f_name);
        }

        for(auto n : names)
        {
            a.set_protocol(n.f_name);
            CATCH_REQUIRE(a.get_protocol() == n.f_protocol);
            CATCH_REQUIRE(a.get_protocol_name() == n.f_name);
        }
    }
    CATCH_END_SECTION()

    // by default addr() is an IPv6 address so we test the basic port
    // functions here, although it could be in a common place instead...
    //
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv6::ports: default port")
        {
            CATCH_REQUIRE_FALSE(a.is_port_defined());
            CATCH_REQUIRE(a.get_port() == 0);

            CATCH_REQUIRE_FALSE(a.is_protocol_defined());
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::ports: named ports")
        {
            CATCH_REQUIRE(a.set_port("ftp"));
            CATCH_REQUIRE(a.get_port() == 21);
            CATCH_REQUIRE(a.set_port("ssh"));
            CATCH_REQUIRE(a.get_port() == 22);
            CATCH_REQUIRE(a.set_port("http"));
            CATCH_REQUIRE(a.get_port() == 80);
            CATCH_REQUIRE(a.set_port("https"));
            CATCH_REQUIRE(a.get_port() == 443);

            // and a couple of invalid ones
            //
            CATCH_REQUIRE_FALSE(a.set_port("invalid"));
            CATCH_REQUIRE(a.get_port() == 443); // port not updated
            CATCH_REQUIRE_FALSE(a.set_port("32.5"));
            CATCH_REQUIRE(a.get_port() == 443); // port not updated
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::ports: set_port()")
        {
            // setup a valid random port to start with
            //
            int const start_port(rand() & 0xFFFF);
            a.set_port(start_port);
            CATCH_REQUIRE(a.is_port_defined());

            // try again with a string
            //
            a.set_port(std::to_string(start_port).c_str());
            CATCH_REQUIRE(a.get_port() == start_port);

            // test 100 invalid ports
            //
            for(int idx(0); idx < 100; ++idx)
            {
                // first try a negative port
                //
                int port_too_small;
                do
                {
                    port_too_small = -(rand() & 0xFFFF);
                }
                while(port_too_small == 0);
                CATCH_REQUIRE_THROWS_AS(a.set_port(port_too_small), addr::addr_invalid_argument);

                // second try too large a port
                //
                int const port_too_large = (rand() & 0xFFFF) + 65536;
                CATCH_REQUIRE_THROWS_AS(a.set_port(port_too_large), addr::addr_invalid_argument);

                // make sure port does not get modified on errors
                //
                CATCH_REQUIRE(a.get_port() == start_port);
            }

            // test all ports
            //
            for(int port(0); port < 65536; ++port)
            {
                a.set_port(port);

                CATCH_REQUIRE(a.get_port() == port);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::ports: known ports to test get_service()")
        {
            // the default is TCP, but it's considered undefined
            //
            CATCH_REQUIRE_FALSE(a.is_protocol_defined());

            a.set_port(80);
            CATCH_REQUIRE(a.get_service() == "http");

            a.set_port(443);
            CATCH_REQUIRE(a.get_service() == "https");

            // again with UDP
            // 
            CATCH_REQUIRE_FALSE(a.is_protocol_defined());
            a.set_protocol(IPPROTO_UDP);
            CATCH_REQUIRE(a.is_protocol_defined());

            a.set_port(80);
            std::string service(a.get_service());
            CATCH_REQUIRE((service == "http" || service == "80"));

            a.set_port(443);
            service = a.get_service();
            CATCH_REQUIRE((service == "https"|| service == "443"));

            // to change the default we offer the user to mark the protocol as undefined
            //
            CATCH_REQUIRE(a.is_protocol_defined());
            a.set_protocol_defined(false);
            CATCH_REQUIRE_FALSE(a.is_protocol_defined());
            a.set_protocol_defined(true);
            CATCH_REQUIRE(a.is_protocol_defined());
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv6 addresses and port")
    {
        CATCH_START_SECTION("ipv6::ports: verify port by parser")
        {
            for(int port(0); port < 65536; ++port)
            {
                int proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                addr::addr_parser p;
                p.set_protocol(proto);
                addr::addr_range::vector_t ips(p.parse("[ff01:2f3:f041:e301:f:10:11:12]:" + std::to_string(port)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ADDRESS) == "ff01:2f3:f041:e301:f:10:11:12");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS) == "[ff01:2f3:f041:e301:f:10:11:12]");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "[ff01:2f3:f041:e301:f:10:11:12]:" + std::to_string(port));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "[ff01:2f3:f041:e301:f:10:11:12]:" + std::to_string(port));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::port: default address with various port")
        {
            for(int idx(0); idx < 100; ++idx)
            {
                uint16_t const port(rand());
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_default_address("ff02:23:f41:e31:20:30:40:50");
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(static_cast<int>(port))));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "[ff02:23:f41:e31:20:30:40:50]:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "[ff02:23:f41:e31:20:30:40:50]:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::port: port when not allowed check as IPv6")
        {
            addr::addr_parser p;
            p.set_allow(addr::allow_t::ALLOW_PORT, false);
            addr::addr_range::vector_t ips(p.parse("localhost:33.5"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Invalid address in \"localhost:33.5\" error -2 -- Name or service not known\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::port: space before port")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("[fafa:fefe:ffaa:ffee::3] :456"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The IPv6 address \"[fafa:fefe:ffaa:ffee::3] :456\" is followed by unknown data.\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE( "ipv6::masks", "[ipv6]" )
{
    CATCH_GIVEN("addr()")
    {
        // technically, a default addr object represents and IPv6 so the
        // dealing with the mask without an IPv4 is done by IPv6 tests
        //
        addr::addr a;

        CATCH_START_SECTION("ipv6::masks: default mask")
        {
            CATCH_REQUIRE_FALSE(a.is_mask_defined());

            uint8_t mask[16] = {};
            a.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
            CATCH_REQUIRE(a.get_mask_size() == 128);

            CATCH_REQUIRE_FALSE(a.is_mask_defined());
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: set_mask_count()")
        {
            CATCH_REQUIRE_FALSE(a.is_mask_defined());

            for(int idx(0); idx <= 128; ++idx)
            {
                a.set_mask_count(idx);
                CATCH_REQUIRE(a.is_mask_defined());
                CATCH_REQUIRE(a.get_mask_size() == idx);
            }

            CATCH_REQUIRE(a.is_mask_defined());
            a.set_mask_defined(false);
            CATCH_REQUIRE_FALSE(a.is_mask_defined());

            for(int idx(-10); idx < 0; ++idx)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                          a.set_mask_count(idx)
                        , addr::out_of_range
                        , Catch::Matchers::ExceptionMessage(
                                  "out_of_range: the mask size " + std::to_string(idx) + " is out of range."));

                CATCH_REQUIRE_FALSE(a.is_mask_defined());
            }

            for(int idx(129); idx <= 130; ++idx)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                          a.set_mask_count(idx)
                        , addr::out_of_range
                        , Catch::Matchers::ExceptionMessage(
                                  "out_of_range: the mask size " + std::to_string(idx) + " is out of range."));

                CATCH_REQUIRE_FALSE(a.is_mask_defined());
            }

            a.set_mask_defined(true);
            CATCH_REQUIRE(a.is_mask_defined());
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: set_mask()")
        {
            uint8_t mask[16], verify_mask[16];
            for(int idx(0); idx < 5; ++idx)
            {
                for(int j(0); j < 16; ++j)
                {
                    mask[j] = rand();
                }
                a.set_mask(mask);
                a.get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(mask[j] == verify_mask[j]);
                }

                // verify that a copy does copy the mask as expected
                //
                addr::addr b(a);
                b.get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(mask[j] == verify_mask[j]);
                }

                // since it's completely random, it should be -1 but it could
                // also be a number, in any event a and b have the same mask
                // so the function has to return the same value
                //
                CATCH_REQUIRE(a.get_mask_size() == b.get_mask_size());
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: set_mask()")
        {
            uint8_t mask[16];
            uint8_t verify_mask[16];
            for(int idx(0); idx < 5; ++idx)
            {
                for(int j(0); j < 16; ++j)
                {
                    mask[j] = rand();
                }
                a.set_mask(mask);
                a.get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(mask[j] == verify_mask[j]);
                    verify_mask[j] = rand();
                }

                // verify that a copy does copy the mask as expected
                //
                addr::addr b(a);
                b.get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(mask[j] == verify_mask[j]);
                    verify_mask[j] = rand();
                }

                // verify that copying inside a range works too
                //
                addr::addr_range r;
                r.set_from(a);
                r.get_from().get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(mask[j] == verify_mask[j]);
                    verify_mask[j] = rand();
                }

                // then that a range copy works as expected
                //
                addr::addr_range c(r);
                c.get_from().get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(mask[j] == verify_mask[j]);
                    verify_mask[j] = rand();
                }
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() of address:port/mask")
    {
        CATCH_START_SECTION("ipv6::masks: mask allowed, but no mask")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port)));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/128");
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: empty mask")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/128");
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: empty mask including the '[]'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/[]"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            std::string const result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/128");
            CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: empty mask '[]' with address mask not allowed")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[66:33:cc:11:7:11:bb:dd]:" + std::to_string(port) + "/[]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: one number masks")
        {
            for(int idx(0); idx <= 128; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/" + std::to_string(idx)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                uint8_t mask[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
                int j(15);
                int m(128 - idx);
                for(; m > 8; m -= 8, --j)
                {
                    mask[j] = 0;
                }
                if(j < 0)
                {
                    throw std::logic_error("invalid j here");
                }
                mask[j] = 255 << m;
                char buf[1024]; // really large buffer to make sure it does not get truncated
                if(inet_ntop(AF_INET6, mask, buf, sizeof(buf)) == nullptr)
                {
                    throw std::logic_error("somehow we could not convert our mask to an IPv6 address.");
                }
                std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/" + std::to_string(idx));
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_mask_size() == idx);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: address like mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
                // when specified as an IP, the mask can be absolutely anything
                uint8_t mask[16];
                for(int j(0); j < 16; ++j)
                {
                    mask[j] = rand();
                }
                std::stringstream smask;
                smask << std::hex
                      << htons((mask[ 1] << 8) | mask[ 0])
                      << ":"                            
                      << htons((mask[ 3] << 8) | mask[ 2])
                      << ":"                            
                      << htons((mask[ 5] << 8) | mask[ 4])
                      << ":"                            
                      << htons((mask[ 7] << 8) | mask[ 6])
                      << ":"                            
                      << htons((mask[ 9] << 8) | mask[ 8])
                      << ":"                            
                      << htons((mask[11] << 8) | mask[10])
                      << ":"                            
                      << htons((mask[13] << 8) | mask[12])
                      << ":"                            
                      << htons((mask[15] << 8) | mask[14]);
                char buf[1024]; // really large buffer to make sure it does not get truncated
                if(inet_ntop(AF_INET6, mask, buf, sizeof(buf)) == nullptr)
                {
                    throw std::logic_error("somehow we could not convert our mask to an IPv6 address.");
                }
                addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/[" + smask.str() + "]"));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/[" + buf + "]");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: address like default mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
                // when specified as an IP, the mask can be absolutely anything
                // (here the mask is a string an it will be parsed by the
                // parser if required)
                //
                uint8_t mask[16];
                for(int j(0); j < 16; ++j)
                {
                    mask[j] = rand();
                }
                std::stringstream smask;
                smask << std::hex
                      << "["
                      << htons((mask[ 1] << 8) | mask[ 0])
                      << ":"                            
                      << htons((mask[ 3] << 8) | mask[ 2])
                      << ":"                            
                      << htons((mask[ 5] << 8) | mask[ 4])
                      << ":"                            
                      << htons((mask[ 7] << 8) | mask[ 6])
                      << ":"                            
                      << htons((mask[ 9] << 8) | mask[ 8])
                      << ":"                            
                      << htons((mask[11] << 8) | mask[10])
                      << ":"                            
                      << htons((mask[13] << 8) | mask[12])
                      << ":"                            
                      << htons((mask[15] << 8) | mask[14])
                      << "]";
                char buf[1024]; // really large buffer to make sure it does not get truncated
                if(inet_ntop(AF_INET6, mask, buf, sizeof(buf)) == nullptr)
                {
                    throw std::logic_error("somehow we could not convert our mask to an IPv6 address.");
                }
                p.set_default_mask(smask.str());
                addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/[" + buf + "]");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                uint8_t verify_mask[16];
                f.get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(verify_mask[j] == mask[j]);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: address like mask with a default")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);

                // here we want a default and an IP with a specific mask
                // to make sure that the specific mask has priority
                //
                uint8_t mask[16];
                for(int j(0); j < 16; ++j)
                {
                    mask[j] = rand();
                }
                std::stringstream smask;
                smask << std::hex
                      << "["
                      << htons((mask[ 1] << 8) | mask[ 0])
                      << ":"                            
                      << htons((mask[ 3] << 8) | mask[ 2])
                      << ":"                            
                      << htons((mask[ 5] << 8) | mask[ 4])
                      << ":"                            
                      << htons((mask[ 7] << 8) | mask[ 6])
                      << ":"                            
                      << htons((mask[ 9] << 8) | mask[ 8])
                      << ":"                            
                      << htons((mask[11] << 8) | mask[10])
                      << ":"                            
                      << htons((mask[13] << 8) | mask[12])
                      << ":"                            
                      << htons((mask[15] << 8) | mask[14])
                      << "]";
                char buf[1024]; // really large buffer to make sure it does not get truncated
                if(inet_ntop(AF_INET6, mask, buf, sizeof(buf)) == nullptr)
                {
                    throw std::logic_error("somehow we could not convert our mask to an IPv6 address.");
                }

                uint8_t default_mask[16];
                for(int j(0); j < 16; ++j)
                {
                    default_mask[j] = rand();
                }
                //std::stringstream default_smask;
                //default_smask << std::hex
                //      << "["
                //      << htons((default_mask[ 1] << 8) | default_mask[ 0])
                //      << ":"                            
                //      << htons((default_mask[ 3] << 8) | default_mask[ 2])
                //      << ":"                            
                //      << htons((default_mask[ 5] << 8) | default_mask[ 4])
                //      << ":"                            
                //      << htons((default_mask[ 7] << 8) | default_mask[ 6])
                //      << ":"                            
                //      << htons((default_mask[ 9] << 8) | default_mask[ 8])
                //      << ":"                            
                //      << htons((default_mask[11] << 8) | default_mask[10])
                //      << ":"                            
                //      << htons((default_mask[13] << 8) | default_mask[12])
                //      << ":"                            
                //      << htons((default_mask[15] << 8) | default_mask[14])
                //      << "]";
                char default_buf[1024]; // really large buffer to make sure it does not get truncated
                if(inet_ntop(AF_INET6, default_mask, default_buf, sizeof(buf)) == nullptr)
                {
                    throw std::logic_error("somehow we could not convert our mask to an IPv6 address.");
                }
                p.set_default_mask(default_buf);

                addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/" + smask.str()));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/[" + buf + "]");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                uint8_t verify_mask[16];
                f.get_mask(verify_mask);
                for(int j(0); j < 16; ++j)
                {
                    CATCH_REQUIRE(verify_mask[j] == mask[j]);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: no address, but one IPv6 number masks")
        {
            // with just a number, the mask is considered an IPv6 mask
            // if it is 33 or more
            //
            for(int idx(33); idx <= 128; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                //p.set_default_address("55:33:22:11:0:cc:bb:aa");
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(port) + "/" + std::to_string(idx)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                uint8_t mask[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
                int j(15);
                int m(128 - idx);
                for(; m > 8; m -= 8, --j)
                {
                    mask[j] = 0;
                }
                if(j < 0)
                {
                    throw std::logic_error("invalid j here");
                }
                mask[j] = 255 << m;
                char buf[1024]; // really large buffer to make sure it does not get truncated
                if(inet_ntop(AF_INET6, mask, buf, sizeof(buf)) == nullptr)
                {
                    throw std::logic_error("somehow we could not convert our mask to an IPv6 address.");
                }
                std::string result("[::]:" + std::to_string(port) + "/" + std::to_string(idx));
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_mask_size() == idx);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::masks: no address, but one IPv6 masks")
        {
            // with just a number, the mask is considered an IPv6 mask
            // if it is 33 or more
            //
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
                //p.set_default_address("55:33:22:11:0:cc:bb:aa");
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(port) + "/[1:2:3:4:5:6:7:8]"));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE_FALSE(f.is_ipv4());
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                CATCH_REQUIRE(f.get_family() == AF_INET6);
                std::string result("[::]:" + std::to_string(port) + "/[1:2:3:4:5:6:7:8]");
                CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::network_type", "[ipv6]")
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv6::network_type: any (::)")
        {
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr32[0] = 0;
                in6.sin6_addr.s6_addr32[1] = 0;
                in6.sin6_addr.s6_addr32[2] = 0;
                in6.sin6_addr.s6_addr32[3] = 0;

                // verify network type
                //
                a.set_ipv6(in6);

                CATCH_REQUIRE(a.is_default());
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_ANY);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Any");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE_FALSE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE(a.is_wan());
                CATCH_REQUIRE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
                CATCH_REQUIRE(a.is_valid());
            }

            // make sure that if any byte is set to non-zero it is not
            // viewed as the ANY address
            //
            for(int idx(0); idx < 16; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr32[0] = 0;
                in6.sin6_addr.s6_addr32[1] = 0;
                in6.sin6_addr.s6_addr32[2] = 0;
                in6.sin6_addr.s6_addr32[3] = 0;

                // change one byte only
                //
                do
                {
                    in6.sin6_addr.s6_addr[idx] = rand();
                }
                while(in6.sin6_addr.s6_addr[idx] == 0);

                // verify network type
                //
                a.set_ipv6(in6);

                CATCH_REQUIRE(std::string(a.get_network_type_string()) != "Any");

                // addresses that start with 0xFD are private
                //
                // note that the test algorithm prevents IPv4 addresses so
                // not need to bother with those
                //
                switch(a.get_network_type())
                {
                case addr::network_type_t::NETWORK_TYPE_UNDEFINED:
                case addr::network_type_t::NETWORK_TYPE_ANY:
                    // the address is always defined
                    // the address is never all zeroes
                    //
                    CATCH_REQUIRE(false);
                    break;

                case addr::network_type_t::NETWORK_TYPE_PRIVATE:
                case addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                    CATCH_REQUIRE(a.is_valid());
                    CATCH_REQUIRE(a.is_lan());
                    CATCH_REQUIRE(a.is_lan(true));
                    CATCH_REQUIRE(a.is_lan(false));
                    CATCH_REQUIRE_FALSE(a.is_wan());
                    CATCH_REQUIRE_FALSE(a.is_wan(true));
                    CATCH_REQUIRE_FALSE(a.is_wan(false));
                    break;

                case addr::network_type_t::NETWORK_TYPE_CARRIER:
                case addr::network_type_t::NETWORK_TYPE_LINK_LOCAL:
                case addr::network_type_t::NETWORK_TYPE_MULTICAST:
                    CATCH_REQUIRE(a.is_valid());
                    CATCH_REQUIRE_FALSE(a.is_lan());
                    CATCH_REQUIRE(a.is_lan(true));
                    CATCH_REQUIRE_FALSE(a.is_lan(false));
                    CATCH_REQUIRE_FALSE(a.is_wan());
                    CATCH_REQUIRE_FALSE(a.is_wan(true));
                    CATCH_REQUIRE_FALSE(a.is_wan(false));
                    break;

                case addr::network_type_t::NETWORK_TYPE_DOCUMENTATION:
                    CATCH_REQUIRE_FALSE(a.is_valid());
                    CATCH_REQUIRE_FALSE(a.is_lan());
                    CATCH_REQUIRE_FALSE(a.is_lan(true));
                    CATCH_REQUIRE_FALSE(a.is_lan(false));
                    CATCH_REQUIRE_FALSE(a.is_wan());
                    CATCH_REQUIRE_FALSE(a.is_wan(true));
                    CATCH_REQUIRE_FALSE(a.is_wan(false));
                    break;

                case addr::network_type_t::NETWORK_TYPE_PUBLIC:
                    CATCH_REQUIRE(a.is_valid());
                    CATCH_REQUIRE_FALSE(a.is_lan());
                    CATCH_REQUIRE_FALSE(a.is_lan(true));
                    CATCH_REQUIRE_FALSE(a.is_lan(false));
                    CATCH_REQUIRE(a.is_wan());
                    CATCH_REQUIRE(a.is_wan(true));
                    CATCH_REQUIRE(a.is_wan(false));
                    break;

                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: private address fd00::/8")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = htons(0xFD00 | (rand() & 255));
                in6.sin6_addr.s6_addr16[1] = rand();
                in6.sin6_addr.s6_addr16[2] = rand();
                in6.sin6_addr.s6_addr16[3] = rand();
                in6.sin6_addr.s6_addr16[4] = rand();
                in6.sin6_addr.s6_addr16[5] = rand();
                in6.sin6_addr.s6_addr16[6] = rand();
                in6.sin6_addr.s6_addr16[7] = rand();

                // verify network type
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Private");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: private address fe80::/10")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = htons(0xFE80 | (rand() & 63));
                in6.sin6_addr.s6_addr16[1] = rand();
                in6.sin6_addr.s6_addr16[2] = rand();
                in6.sin6_addr.s6_addr16[3] = rand();
                in6.sin6_addr.s6_addr16[4] = rand();
                in6.sin6_addr.s6_addr16[5] = rand();
                in6.sin6_addr.s6_addr16[6] = rand();
                in6.sin6_addr.s6_addr16[7] = rand();

                // verify network type
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Local Link");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: private address ff02::/16")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = htons(0xFF02);
                in6.sin6_addr.s6_addr16[1] = rand();
                in6.sin6_addr.s6_addr16[2] = rand();
                in6.sin6_addr.s6_addr16[3] = rand();
                in6.sin6_addr.s6_addr16[4] = rand();
                in6.sin6_addr.s6_addr16[5] = rand();
                in6.sin6_addr.s6_addr16[6] = rand();
                in6.sin6_addr.s6_addr16[7] = rand();

                // verify network type
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Local Link");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: private address ff00::/8")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                do
                {
                    in6.sin6_addr.s6_addr16[0] = htons(0xFF00 | (rand() & 255));
                }
                while((in6.sin6_addr.s6_addr16[0] & htons(0xFF0F)) == htons(0xFF01)       // ffx1::/16
                   || (in6.sin6_addr.s6_addr16[0] & htons(0xFF0F)) == htons(0xFF02)       // ffx2::/16
                   || (in6.sin6_addr.s6_addr16[0] & htons(0xFFC0)) == htons(0xFE80)       // fe80::/10
                   || (in6.sin6_addr.s6_addr16[0] & htons(0xFF00)) == htons(0xFD00));     // fd00::/8
                in6.sin6_addr.s6_addr16[1] = rand();
                in6.sin6_addr.s6_addr16[2] = rand();
                in6.sin6_addr.s6_addr16[3] = rand();
                in6.sin6_addr.s6_addr16[4] = rand();
                in6.sin6_addr.s6_addr16[5] = rand();
                in6.sin6_addr.s6_addr16[6] = rand();
                in6.sin6_addr.s6_addr16[7] = rand();

                // verify network type
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_MULTICAST);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Multicast");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: private address ffx1::/8")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = htons(0xFF01 | ((rand() & 15) << 4));
                in6.sin6_addr.s6_addr16[1] = rand();
                in6.sin6_addr.s6_addr16[2] = rand();
                in6.sin6_addr.s6_addr16[3] = rand();
                in6.sin6_addr.s6_addr16[4] = rand();
                in6.sin6_addr.s6_addr16[5] = rand();
                in6.sin6_addr.s6_addr16[6] = rand();
                in6.sin6_addr.s6_addr16[7] = rand();

                // verify network type
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Loopback");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: private address ::1")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = 0;
                in6.sin6_addr.s6_addr16[1] = 0;
                in6.sin6_addr.s6_addr16[2] = 0;
                in6.sin6_addr.s6_addr16[3] = 0;
                in6.sin6_addr.s6_addr16[4] = 0;
                in6.sin6_addr.s6_addr16[5] = 0;
                in6.sin6_addr.s6_addr16[6] = 0;
                in6.sin6_addr.s6_addr16[7] = htons(1);

                // verify network type
                //
                a.set_ipv6(in6);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Loopback");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));

                // try again from a string to confirm
                //
                struct addrinfo * addrlist(nullptr);
                int const port(rand() & 65535);
                int const r(getaddrinfo("::1", std::to_string(port).c_str(), nullptr, &addrlist));
                CATCH_REQUIRE(r == 0);
                CATCH_REQUIRE(addrlist != nullptr);
                CATCH_REQUIRE(addrlist->ai_family == AF_INET6);
                CATCH_REQUIRE(addrlist->ai_addrlen == sizeof(struct sockaddr_in6));
                a.set_ipv6(*reinterpret_cast<sockaddr_in6 *>(addrlist->ai_addr));
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Loopback");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
                freeaddrinfo(addrlist);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network_type: documentation (2001:db8:: and 3fff:0XXX::)")
        {
            for(int count(0); count < 100; ++count)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = htons(0x2001);
                in6.sin6_addr.s6_addr16[1] = htons(0xdb8);
                SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[1]);
                SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[2]);
                SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[3]);

                // verify network type
                //
                a.set_ipv6(in6);

                CATCH_REQUIRE_FALSE(a.is_default());
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_DOCUMENTATION);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Documentation");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE_FALSE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
                CATCH_REQUIRE_FALSE(a.is_valid());
            }

            for(int count(0); count < 100; ++count)
            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(rand());
                in6.sin6_addr.s6_addr16[0] = htons(0x3fff);
                in6.sin6_addr.s6_addr16[1] = htons(rand() & 0xfff);
                SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[1]);
                SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[2]);
                SNAP_CATCH2_NAMESPACE::random(in6.sin6_addr.s6_addr32[3]);

                // verify network type
                //
                a.set_ipv6(in6);

                CATCH_REQUIRE_FALSE(a.is_default());
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_DOCUMENTATION);
                CATCH_REQUIRE(std::string(a.get_network_type_string()) == "Documentation");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE_FALSE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
                CATCH_REQUIRE_FALSE(a.is_valid());
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::network", "[ipv6]")
{
    CATCH_GIVEN("set_from_socket()")
    {
        CATCH_START_SECTION("ipv6::network: create a server, but do not test it (yet)...")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("[::1]:49999"));
            CATCH_REQUIRE(ips.size() >= 1);

            addr::addr & a(ips[0].get_from());
            int s(a.create_socket(addr::addr::SOCKET_FLAG_NONBLOCK | addr::addr::SOCKET_FLAG_CLOEXEC | addr::addr::SOCKET_FLAG_REUSE));
            CATCH_REQUIRE(s >= 0);
            std::shared_ptr<int> auto_free(&s, socket_deleter);

            CATCH_REQUIRE(a.bind(s) == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network: connect with TCP to [::1]")
        {
            if(SNAP_CATCH2_NAMESPACE::g_tcp_port != -1)
            {
                addr::addr_parser p;
                addr::addr_range::vector_t ips(p.parse("[::1]:" + std::to_string(SNAP_CATCH2_NAMESPACE::g_tcp_port)));
                CATCH_REQUIRE(ips.size() >= 1);

                addr::addr & a(ips[0].get_from());
                int s(a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC));// | addr::addr::SOCKET_FLAG_REUSE));
                CATCH_REQUIRE(s >= 0);
                std::shared_ptr<int> auto_free(&s, socket_deleter);

                CATCH_REQUIRE(a.connect(s) == 0);

                // get socket info from the other side (peer == true)
                //
                addr::addr b;
                b.set_from_socket(s, true);
                CATCH_REQUIRE_FALSE(b.is_ipv4());
                CATCH_REQUIRE_FALSE(b.get_family() == AF_INET);
                CATCH_REQUIRE(b.get_family() == AF_INET6);
                CATCH_REQUIRE(b.to_ipv6_string(addr::STRING_IP_ADDRESS)    == "::1");
                CATCH_REQUIRE(b.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::1");

                // in this case we know what the port is since we specified
                // that when connecting
                //
                CATCH_REQUIRE(b.get_port() == SNAP_CATCH2_NAMESPACE::g_tcp_port);

                // now try this side (peer == false)
                //
                addr::addr c;
                c.set_from_socket(s, false);
                CATCH_REQUIRE_FALSE(c.is_ipv4());
                CATCH_REQUIRE_FALSE(c.get_family() == AF_INET);
                CATCH_REQUIRE(c.get_family() == AF_INET6);
                CATCH_REQUIRE(c.to_ipv6_string(addr::STRING_IP_ADDRESS)    == "::1");
                CATCH_REQUIRE(c.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::1");

                // we cannot be sure of the port, there is a range we could
                // test better (more constraining) but for this test is
                // certainly does not matter much; it has to be more than
                // 1023, though
                //
                CATCH_REQUIRE(c.get_port() > 1023);
            }
            else
            {
                // avoid issue of no assertions
                //
                CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::g_tcp_port == -1);
                std::cout << "connect to [::1] test skipped as no TCP port was specified on the command line." << std::endl;
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::network: connect with UDP to [::1]")
        {
            addr::addr_parser p;
            p.set_protocol("udp");
            addr::addr_range::vector_t ips(p.parse("[::1]:53"));
            CATCH_REQUIRE(ips.size() >= 1);

            addr::addr & a(ips[0].get_from());
            int s(a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC));// | addr::addr::SOCKET_FLAG_REUSE));
            CATCH_REQUIRE(s >= 0);
            std::shared_ptr<int> auto_free(&s, socket_deleter);

            CATCH_REQUIRE(a.connect(s) == -1);

            // get socket info from the other side (peer == true)
            //
            addr::addr b;
            CATCH_REQUIRE_THROWS_AS(b.set_from_socket(s, true), addr::addr_io_error);
            CATCH_REQUIRE_FALSE(b.is_ipv4());
            CATCH_REQUIRE_FALSE(b.get_family() == AF_INET);
            CATCH_REQUIRE(b.get_family() == AF_INET6);
            CATCH_REQUIRE(b.to_ipv6_string(addr::STRING_IP_ADDRESS)    == "::");
            CATCH_REQUIRE(b.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::");

            // in this case we know what the port is since we specified
            // that when connecting
            //
            CATCH_REQUIRE(b.get_port() == 0);

            // now try this side (peer == false)
            //
            addr::addr c;
            c.set_from_socket(s, false);
            CATCH_REQUIRE_FALSE(c.is_ipv4());
            CATCH_REQUIRE_FALSE(c.get_family() == AF_INET);
            CATCH_REQUIRE(c.get_family() == AF_INET6);
            CATCH_REQUIRE(c.to_ipv6_string(addr::STRING_IP_ADDRESS)    == "::");
            CATCH_REQUIRE(c.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::");

            // we cannot be sure of the port, there is a range we could
            // test better (more constraining) but for this test is
            // certainly does not matter much; it has to be more than
            // 1023, though
            //
            CATCH_REQUIRE(c.get_port() == 0);
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::udp", "[ipv6]")
{
    constexpr int const TEST_PORT(4004);
    constexpr int const TEST_PROTOCOL(IPPROTO_UDP);
    constexpr int const TEST_COUNT(10);

    CATCH_START_SECTION("ipv6::udp: sendto() and recvfrom()")
    {
        class sr
        {
        public:
            sr()
            {
                f_a.set_protocol(TEST_PROTOCOL);
                f_a.set_ipv6_loopback();
                f_a.set_port(TEST_PORT);
                f_sa = f_a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC | addr::addr::SOCKET_FLAG_REUSE);
                CATCH_REQUIRE(f_sa != -1);

                f_b.set_protocol(TEST_PROTOCOL);
                f_b.set_ipv6_loopback();
                f_b.set_port(TEST_PORT);
                f_sb = f_b.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC | addr::addr::SOCKET_FLAG_REUSE);
                CATCH_REQUIRE(f_sb != -1);
                f_b.bind(f_sb); // the receive has to be bound

                for(int i(0); i < TEST_COUNT; ++i)
                {
                    std::size_t const size(rand() % 350 + 50);
                    f_buf[i].resize(size);
                    for(std::size_t idx(0); idx < size; ++idx)
                    {
                        f_buf[i][idx] = rand();
                    }
                }
            }

            void run()
            {
                int client_port(-1);
                for(int i(0); i < TEST_COUNT; ++i)
                {
                    sendto(i);

                    std::vector<std::uint8_t> bb(f_buf[i].size());
                    int const r(f_b.recvfrom(f_sb, reinterpret_cast<char *>(bb.data()), bb.size()));
                    if(r == -1)
                    {
                        int const e(errno);
                        std::cerr << "--- recvfrom() returned an error: " << strerror(e) << std::endl;
                        return;
                    }

                    CATCH_REQUIRE(r == static_cast<int>(f_buf[i].size()));
                    CATCH_REQUIRE(f_buf[i] == bb);
                    CATCH_REQUIRE_FALSE(f_b.is_ipv4());
                    if(client_port == -1)
                    {
                        client_port = f_b.get_port();
                    }
                    else
                    {
                        // the ephemeral port does not change once we sent
                        // the first packet
                        //
                        CATCH_REQUIRE(f_b.get_port() == client_port);
                    }
                    CATCH_REQUIRE(f_b.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                }
            }

            void sendto(int idx)
            {
                int const r(f_a.sendto(f_sa, reinterpret_cast<char const *>(f_buf[idx].data()), f_buf[idx].size()));
                if(r == -1)
                {
                    int const e(errno);
                    std::cerr << "--- sendto() returned an error: " << strerror(e) << std::endl;
                    return;
                }
                CATCH_REQUIRE(r == static_cast<int>(f_buf[idx].size()));
            }

            addr::addr                  f_a = addr::addr();
            addr::addr                  f_b = addr::addr();
            int                         f_sa = -1;
            int                         f_sb = -1;
            std::vector<std::uint8_t>   f_buf[TEST_COUNT] = {};
        };

        sr run;
        run.run();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::udp: sendto() with wrong protocol")
    {
        addr::addr a;
        a.set_protocol(IPPROTO_TCP);
        a.set_ipv6_loopback();
        a.set_port(TEST_PORT);
        int sa(a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC | addr::addr::SOCKET_FLAG_REUSE));
        CATCH_REQUIRE(sa != -1);

        char buf[256];
        int const r(a.sendto(sa, buf, sizeof(buf)));
        int const e(errno);
        CATCH_REQUIRE(r == -1);
        CATCH_REQUIRE(e == EINVAL);

        close(sa);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ipv6::udp: recvfrom() with wrong protocol")
    {
        addr::addr a;
        a.set_protocol(IPPROTO_TCP);
        a.set_ipv6_loopback();
        a.set_port(TEST_PORT);
        int sa(a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC | addr::addr::SOCKET_FLAG_REUSE));
        CATCH_REQUIRE(sa != -1);
        a.bind(sa); // the receive has to be bound

        char buf[256];
        int const r(a.recvfrom(sa, buf, sizeof(buf)));
        int const e(errno);
        CATCH_REQUIRE(r == -1);
        CATCH_REQUIRE(e == EINVAL);

        close(sa);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
