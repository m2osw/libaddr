// Copyright (c) 2011-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Test the IPv4 interface.
 *
 * These test verify that the IPv4 side of things function as expected.
 *
 * Note that some of the tests between the IPv4 and IPv6 overlap. Here
 * you mainly find the IPv4 side of things.
 */

// addr
//
#include    <libaddr/iface.h>


// self
//
#include    "catch_main.h"


// last include
//
#include    <snapdev/poison.h>




/** \brief Details used by the addr class implementation.
 *
 * We have a function to check whether an address is part of
 * the interfaces of your computer. This check requires the
 * use of a `struct ifaddrs` and as such it requires to
 * delete that structure. We define a deleter for that
 * strucure here.
 */
namespace
{

/** \brief Close a socket.
 *
 * This deleter is used to make sure all the socket we test get closed
 * on exit.
 *
 * \param[in] s  The socket to close.
 */
void socket_deleter(int * s)
{
    close(*s);
}


}
// no name namespace



CATCH_TEST_CASE("ipv4::invalid_input", "[ipv4]")
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv4::invalid_input: set IPv4 with an invalid family")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                do
                {
                    in.sin_family = rand();
                }
                while(in.sin_family == AF_INET);
                in.sin_port = htons(rand());
                in.sin_addr.s_addr = htonl(rand() ^ (rand() << 16));
                CATCH_REQUIRE_THROWS_AS(a.set_ipv4(in), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(addr::addr(in), addr::addr_invalid_argument);
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv4 settings")
    {
        addr::addr_parser a;

        CATCH_START_SECTION("addr_parser(): invalid allow flags (too small)")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                // test with a negative number
                //
                int n;
                do
                {
                    n = rand();
                }
                while(n == 0);
                if(n > 0)
                {
                    // all positive numbers have a corresponding negative
                    // number so this always flips the sign as expected
                    //
                    n = -n;
                }
                addr::allow_t const flag(static_cast<addr::allow_t>(n));

                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, true), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, false), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.get_allow(flag), addr::addr_invalid_argument);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): invalid allow flags (too large)")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                // test with a negative number
                //
                int n;
                do
                {
                    n = rand();
                    if(n < 0)
                    {
                        n = -n;
                    }
                }
                while(n < static_cast<int>(addr::allow_t::ALLOW_max));
                addr::allow_t const flag(static_cast<addr::allow_t>(n));

                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, true), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, false), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.get_allow(flag), addr::addr_invalid_argument);
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv4 addresses")
    {
        CATCH_START_SECTION("addr_parser(): bad address")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("{bad-ip}"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Invalid address in \"{bad-ip}\" error -2 -- Name or service not known (errno: 22 -- Invalid argument).\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): required address")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_REQUIRED_ADDRESS, true);
            addr::addr_range::vector_t ips(p.parse(""));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Required address is missing.\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv4 ports")
    {
        CATCH_START_SECTION("addr_parser(): required port")
        {
            // optional + required -> required
            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::allow_t::ALLOW_REQUIRED_PORT, true);
                addr::addr_range::vector_t ips(p.parse("1.2.3.4"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Required port is missing.\n");
                CATCH_REQUIRE(p.has_errors());
                p.clear_errors();
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 0);
            }

            // only required -> required just the same
            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                p.set_allow(addr::allow_t::ALLOW_REQUIRED_PORT, true);
                addr::addr_range::vector_t ips(p.parse("1.2.3.4"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Required port is missing.\n");
                CATCH_REQUIRE(p.has_errors());
                p.clear_errors();
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): port not allowed")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_PORT, false);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_REQUIRED_PORT));
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:123"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Port not allowed (1.2.3.4:123).\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): invalid port")
        {
            addr::addr_parser p;

            // so to a different default value
            //
            int const default_port(rand() & 0xFFFF);
            p.set_default_port(default_port);

            for(int idx(0); idx < 25; ++idx)
            {
                int port(0);
                do
                {
                    port = rand() ^ (rand() << 16);
                }
                while(port >= -1 && port <= 65535); // -1 is valid here, it represents "no default port defined"
                CATCH_REQUIRE_THROWS_AS(p.set_default_port(port), addr::addr_invalid_argument);

                std::string const port_str(std::to_string(port));
                CATCH_REQUIRE_THROWS_AS(p.set_default_port(port_str), addr::addr_invalid_argument);

                // verify port unchanged
                //
                CATCH_REQUIRE(p.get_default_port() == default_port);
            }

            CATCH_REQUIRE_THROWS_AS(p.set_default_port("not-a-number"), addr::addr_invalid_argument);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with invalid masks")
    {
        CATCH_START_SECTION("addr_parser(): really large numbers (over 1000)")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 10001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Mask size too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): ipv4 mask is limited between 0 and 32")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 33);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Unsupported mask size (" + std::to_string(mask) + ", expected 32 at the most for an IPv4).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): ipv4 mask cannot use name")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/localhost"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid mask in \"/localhost\", error -2 -- Name or service not known (errno: 0 -- Success).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): ipv4 mask mismatch (mask uses ipv6)")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/[1:2:3:4:5:6:7:8]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The address uses the IPv4 syntax, the mask cannot use IPv6.\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): ipv4 mask mismatch (mask uses ipv6 without [...])")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/1:2:3:4:5:6:7:8"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Incompatible address between the address and mask address (first was an IPv4 second an IPv6).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with invalid protocols")
    {
        CATCH_START_SECTION("addr_parser(): invalid names")
        {
            addr::addr_parser p;

            // not changing default protocol
            //
            CATCH_REQUIRE(p.get_protocol() == -1);
            CATCH_REQUIRE_THROWS_AS(p.set_protocol("unknown"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_protocol() == -1);

            // change protocol to another valid value first
            //
            p.set_protocol("tcp");
            CATCH_REQUIRE_THROWS_AS(p.set_protocol("another"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_TCP);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_parser(): invalid numbers")
        {
            for(int idx(0); idx < 100; ++idx)
            {
                int protocol;
                do
                {
                    protocol = rand();
                }
                while(protocol == IPPROTO_IP
                   || protocol == IPPROTO_TCP
                   || protocol == IPPROTO_UDP);

                addr::addr_parser p;

                CATCH_REQUIRE_THROWS_AS(p.set_protocol(protocol), addr::addr_invalid_argument);
                CATCH_REQUIRE(p.get_protocol() == -1);

                // change protocol to another valid value first
                //
                p.set_protocol("tcp");
                CATCH_REQUIRE_THROWS_AS(p.set_protocol(protocol), addr::addr_invalid_argument);
                CATCH_REQUIRE(p.get_protocol() == IPPROTO_TCP);
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv4::address_defaults", "[ipv4][ipv6]")
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("addr: not an IPv4")
        {
            CATCH_REQUIRE_FALSE(a.is_ipv4());
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET);
            CATCH_REQUIRE(a.get_family() == AF_INET6);
            CATCH_REQUIRE(a.get_hostname() == std::string());
            CATCH_REQUIRE(a.is_hostname_an_ip());
            CATCH_REQUIRE(a.get_interface() == std::string());

            struct sockaddr_in in;
            CATCH_REQUIRE_THROWS_AS(a.get_ipv4(in), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_ADDRESS),                                                               addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS),                                                       addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT),                                addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_PORT | addr::STRING_IP_MASK),                 addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT | addr::STRING_IP_BRACKET_MASK), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_ALL),                                                                   addr::addr_invalid_state);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: default network type (0.0.0.0)")
        {
            CATCH_REQUIRE(a.is_default());
            CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_ANY);
            CATCH_REQUIRE(a.get_network_type_string() == "Any");
            CATCH_REQUIRE_FALSE(a.is_lan());
            CATCH_REQUIRE_FALSE(a.is_lan(true));
            CATCH_REQUIRE_FALSE(a.is_lan(false));
            CATCH_REQUIRE(a.is_wan());
            CATCH_REQUIRE(a.is_wan(true));
            CATCH_REQUIRE_FALSE(a.is_wan(false));
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: IPv6 ANY")
        {
            struct sockaddr_in6 in6;
            a.get_ipv6(in6);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[0] == 0);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[1] == 0);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[2] == 0);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[3] == 0);
            CATCH_REQUIRE(a.to_ipv6_string(addr::STRING_IP_ADDRESS)                                         == "::");
            CATCH_REQUIRE(a.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS)                                 == "[::]");
            CATCH_REQUIRE(a.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)          == "[::]:0");
            CATCH_REQUIRE(a.to_ipv6_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK)                  == "::/128");
            CATCH_REQUIRE(a.to_ipv6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK)  == "[::]/128");
            CATCH_REQUIRE(a.to_ipv6_string(addr::STRING_IP_ALL)                                             == "[::]:0/128");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: IPv4 or IPv6 string")
        {
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::STRING_IP_ADDRESS)                                          == "::");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS)                                  == "[::]");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)           == "[::]:0");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK)                   == "::/128");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK)   == "[::]/128");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::STRING_IP_ALL)                                              == "[::]:0/128");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: interface determination")
        {
            CATCH_REQUIRE(addr::find_addr_interface(a, false) == nullptr);
            CATCH_REQUIRE(addr::find_addr_interface(a, true) != nullptr);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: default name/service/port/protocol")
        {
            CATCH_REQUIRE(a.get_name() == std::string());
            CATCH_REQUIRE(a.get_service() == std::string());
            CATCH_REQUIRE(a.get_port() == 0);
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: compare with self")
        {
            CATCH_REQUIRE(a == a);
            CATCH_REQUIRE_FALSE(a != a);
            CATCH_REQUIRE_FALSE(a < a);
            CATCH_REQUIRE(a <= a);
            CATCH_REQUIRE_FALSE(a > a);
            CATCH_REQUIRE(a >= a);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: compare with another 0.0.0.0")
        {
            {
                addr::addr b;

                CATCH_REQUIRE(a == b);
                CATCH_REQUIRE_FALSE(a != b);
                CATCH_REQUIRE_FALSE(a < b);
                CATCH_REQUIRE(a <= b);
                CATCH_REQUIRE_FALSE(a > b);
                CATCH_REQUIRE(a >= b);
            }

            {
                struct sockaddr_in6 in6 = sockaddr_in6();
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(0);
                in6.sin6_addr.s6_addr32[0] = htonl(0);
                in6.sin6_addr.s6_addr32[1] = htonl(0);
                in6.sin6_addr.s6_addr32[2] = htonl(0);
                in6.sin6_addr.s6_addr32[3] = htonl(0);
                addr::addr b(in6);

                CATCH_REQUIRE(a == b);
                CATCH_REQUIRE_FALSE(a != b);
                CATCH_REQUIRE_FALSE(a < b);
                CATCH_REQUIRE(a <= b);
                CATCH_REQUIRE_FALSE(a > b);
                CATCH_REQUIRE(a >= b);
            }

            // ANY in IPv4 != ANY in IPv6...
            // (i.e. IPv4 sets addr.sin6_addr.s6_addr16[5] == 0xFFFF)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(0);
                in.sin_addr.s_addr = htonl(0);
                addr::addr b(in);

                CATCH_REQUIRE_FALSE(a == b);
                CATCH_REQUIRE(a != b);
                CATCH_REQUIRE(a < b);
                CATCH_REQUIRE(a <= b);
                CATCH_REQUIRE_FALSE(a > b);
                CATCH_REQUIRE_FALSE(a >= b);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: compare with IPv4 127.0.0.1")
        {
            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(80);
            in.sin_addr.s_addr = htonl((127 << 24) | 1);
            addr::addr b(in);

            CATCH_REQUIRE_FALSE(a == b);
            CATCH_REQUIRE(a != b);
            CATCH_REQUIRE(a < b);
            CATCH_REQUIRE(a <= b);
            CATCH_REQUIRE_FALSE(a > b);
            CATCH_REQUIRE_FALSE(a >= b);
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv4::address", "[ipv4]")
{
    CATCH_GIVEN("addr() with an IPv4")
    {
        addr::addr a;

        CATCH_START_SECTION("addr: set_ipv4() / get_ipv4()")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                in.sin_addr.s_addr = htonl(rand() ^ (rand() << 16));

                // test constructor
                //
                addr::addr b(in);
                struct sockaddr_in out;
                b.get_ipv4(out);
                CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

                // test set
                //
                a.set_ipv4(in);
                a.get_ipv4(out);
                CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: set_ipv4() / to_ipv4_string()")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                uint16_t const port(rand());
                in.sin_port = htons(port);
                uint32_t const address(rand() ^ (rand() << 16));
                in.sin_addr.s_addr = htonl(address);

                std::string ip(
                          std::to_string((address >> 24) & 255)
                        + "."
                        + std::to_string((address >> 16) & 255)
                        + "."
                        + std::to_string((address >>  8) & 255)
                        + "."
                        + std::to_string((address >>  0) & 255)
                        );
                std::string port_str(std::to_string(static_cast<int>(port)));

                // check IPv4 as a string
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.to_ipv4_string(addr::STRING_IP_ADDRESS)                                         == ip);
                CATCH_REQUIRE(a.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS)                                 == ip);
                CATCH_REQUIRE(a.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)          == ip + ":" + port_str);
                CATCH_REQUIRE(a.to_ipv4_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_MASK)                  == ip + "/32");
                CATCH_REQUIRE(a.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_BRACKET_MASK)  == ip + "/32");
                CATCH_REQUIRE(a.to_ipv4_string(addr::STRING_IP_ALL)                                             == ip + ":" + port_str + "/32");

                // if the mask is not a valid IPv4 mask, then we get an exception
                //
                std::uint8_t invalid_mask[16] = { 255, 255 };
                a.set_mask(invalid_mask);
                CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_ALL), addr::addr_unexpected_mask);
                invalid_mask[3] = 255;
                a.set_mask(invalid_mask);
                CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::STRING_IP_ALL), addr::addr_unexpected_mask);
                std::uint8_t const valid_mask[16] = {
                           255,    255,    255,    255,
                           255,    255,    255,    255,
                           255,    255,    255,    255,
                        static_cast<std::uint8_t>(rand()),
                        static_cast<std::uint8_t>(rand()),
                        static_cast<std::uint8_t>(rand()),
                        static_cast<std::uint8_t>(rand()),
                    };
                a.set_mask(valid_mask);
                a.set_mask_count(128);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: name of various IPs")
        {
            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            in.sin_addr.s_addr = 0;

            // verify network type
            //
            a.set_ipv4(in);
            CATCH_REQUIRE(a.get_name() == std::string()); // no name for "any" (TCP)

            a.set_protocol(IPPROTO_UDP);
            CATCH_REQUIRE(a.get_name() == std::string()); // no name for "any" (UDP)

            in.sin_addr.s_addr = htonl(0x7f000001);
            a.set_ipv4(in);
            char hostname[HOST_NAME_MAX + 1];
            hostname[HOST_NAME_MAX] = '\0';
            CATCH_REQUIRE(gethostname(hostname, sizeof(hostname)) == 0);
            CATCH_REQUIRE(hostname[0] != '\0');
            std::string localhost(a.get_name());
            bool const localhost_flag(localhost == hostname || localhost == "localhost");
            CATCH_REQUIRE(localhost_flag);

            CATCH_REQUIRE(addr::find_addr_interface(a, false) != nullptr);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv4 addresses")
    {
        CATCH_START_SECTION("addr: verify basics")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "");
            CATCH_REQUIRE(p.get_default_port() == -1);
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "");
            addr::addr_range::vector_t ips(p.parse("1.2.3.4"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            CATCH_REQUIRE(r.has_from());
            CATCH_REQUIRE_FALSE(r.has_to());
            CATCH_REQUIRE_FALSE(r.is_range());
            CATCH_REQUIRE_FALSE(r.is_empty());
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            CATCH_REQUIRE(f.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
            CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
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
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: verify default address")
        {
            addr::addr_parser p;

            p.set_default_address("1.5.19.200");
            CATCH_REQUIRE(p.get_default_address4() == "1.5.19.200");
            CATCH_REQUIRE(p.get_default_address6() == "");
            p.set_default_address("");
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "");

            p.set_default_address("1.5.19.200");
            CATCH_REQUIRE(p.get_default_address4() == "1.5.19.200");
            CATCH_REQUIRE(p.get_default_address6() == "");
            p.set_default_address("[4:5:4:5:7:8:7:8]");
            CATCH_REQUIRE(p.get_default_address4() == "1.5.19.200");
            CATCH_REQUIRE(p.get_default_address6() == "4:5:4:5:7:8:7:8");
            p.set_default_address("");
            CATCH_REQUIRE(p.get_default_address4() == "");
            CATCH_REQUIRE(p.get_default_address6() == "");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: verify default mask")
        {
            addr::addr_parser p;

            p.set_default_mask("1.5.19.200");
            CATCH_REQUIRE(p.get_default_mask4() == "1.5.19.200");
            CATCH_REQUIRE(p.get_default_mask6() == "");
            p.set_default_mask("");
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "");

            p.set_default_mask("1.5.19.200");
            CATCH_REQUIRE(p.get_default_mask4() == "1.5.19.200");
            CATCH_REQUIRE(p.get_default_mask6() == "");
            p.set_default_mask("[4:5:4:5:7:8:7:8]");
            CATCH_REQUIRE(p.get_default_mask4() == "1.5.19.200");
            CATCH_REQUIRE(p.get_default_mask6() == "4:5:4:5:7:8:7:8");
            p.set_default_mask("");
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: verify default allow flags")
        {
            addr::addr_parser p;

            for(int idx(0); idx < static_cast<int>(addr::allow_t::ALLOW_max); ++idx)
            {
                switch(static_cast<addr::allow_t>(idx))
                {
                case addr::allow_t::ALLOW_ADDRESS:
                case addr::allow_t::ALLOW_ADDRESS_LOOKUP:
                case addr::allow_t::ALLOW_PORT:
                    // only these are true by default
                    //
                    CATCH_REQUIRE(p.get_allow(static_cast<addr::allow_t>(idx)));
                    break;

                default:
                    CATCH_REQUIRE_FALSE(p.get_allow(static_cast<addr::allow_t>(idx)));
                    break;

                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: verify contradictory flags")
        {
            addr::addr_parser p;

            // by default these are set to false
            //
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // check setting MULTI_ADDRESSES_COMMAS to true
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_SPACES
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // add MULTI_PORTS_COMMAS
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS, true);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_COMMAS again
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // remove MULTI_ADDRESSES_SPACES
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, false);
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // back to MULTI_PORTS_COMMAS
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS, true);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_COMMAS first now
            //
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            CATCH_REQUIRE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::allow_t::ALLOW_MULTI_PORTS_COMMAS));
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: default address")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_default_address("5.5.5.5");
            CATCH_REQUIRE(p.get_default_address4() == "5.5.5.5");
            CATCH_REQUIRE(p.get_default_address6() == "");
            addr::addr_range::vector_t ips(p.parse(""));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            CATCH_REQUIRE(r.has_from());
            CATCH_REQUIRE_FALSE(r.has_to());
            CATCH_REQUIRE_FALSE(r.is_range());
            CATCH_REQUIRE_FALSE(r.is_empty());
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            CATCH_REQUIRE(f.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
            CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.5.5.5");
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

        CATCH_START_SECTION("addr: address, no port allowed")
        {
            // specific address with a default
            {
                addr::addr_parser p;
                p.set_allow(addr::allow_t::ALLOW_PORT, false);
                p.set_protocol(IPPROTO_TCP);
                p.set_default_address("5.5.5.5");
                CATCH_REQUIRE(p.get_default_address4() == "5.5.5.5");
                CATCH_REQUIRE(p.get_default_address6() == "");
                addr::addr_range::vector_t ips(p.parse("9.9.9.9"));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "9.9.9.9");
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
                p.set_default_address("5.5.5.5");
                CATCH_REQUIRE(p.get_default_address4() == "5.5.5.5");
                CATCH_REQUIRE(p.get_default_address6() == "");
                addr::addr_range::vector_t ips(p.parse(""));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.5.5.5");
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

    CATCH_GIVEN("addr_parser() with multiple IPv4 addresses in one string")
    {
        CATCH_START_SECTION("addr: 3 IPs separated by commas")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:55,5.6.7.8,,,,10.11.12.99:77"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 3);

            uint8_t mask[16] = {};

            // 1.2.3.4:55
            {
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 5.6.7.8
            {
                addr::addr_range const & r(ips[1]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 10.11.12.99:77
            {
                addr::addr_range const & r(ips[2]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: 3 IPs separated by spaces")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:55 5.6.7.8   10.11.12.99:77"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 3);

            uint8_t mask[16] = {};

            // 1.2.3.4:55
            {
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 5.6.7.8
            {
                addr::addr_range const & r(ips[1]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 10.11.12.99:77
            {
                addr::addr_range const & r(ips[2]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: 3 IPs separated by commas and/or spaces")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
            addr::addr_range::vector_t ips(p.parse(" 1.2.3.4:55,, 5.6.7.8 , 10.11.12.99:77 "));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 3);

            uint8_t mask[16] = {};

            // 1.2.3.4:55
            {
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 5.6.7.8
            {
                addr::addr_range const & r(ips[1]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 10.11.12.99:77
            {
                addr::addr_range const & r(ips[2]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: 3 IPs with hash (#) comments")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            p.set_allow(addr::allow_t::ALLOW_COMMENT_HASH, true);
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:55#first,5.6.7.8#second,10.11.12.99:77#third"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 3);

            uint8_t mask[16] = {};

            // 1.2.3.4:55
            {
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 5.6.7.8
            {
                addr::addr_range const & r(ips[1]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 10.11.12.99:77
            {
                addr::addr_range const & r(ips[2]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: 3 IPs with semicolon (;) comments")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            p.set_allow(addr::allow_t::ALLOW_COMMENT_SEMICOLON, true);
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:55;first,5.6.7.8;second,10.11.12.99:77;third"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 3);

            uint8_t mask[16] = {};

            // 1.2.3.4:55
            {
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 5.6.7.8
            {
                addr::addr_range const & r(ips[1]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 10.11.12.99:77
            {
                addr::addr_range const & r(ips[2]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: 3 IPs with hash (#) and semicolon (;) comments")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            p.set_allow(addr::allow_t::ALLOW_COMMENT_HASH, true);
            p.set_allow(addr::allow_t::ALLOW_COMMENT_SEMICOLON, true);
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:55;first,5.6.7.8#second,10.11.12.99:77;third"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 3);

            uint8_t mask[16] = {};

            // 1.2.3.4:55
            {
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 5.6.7.8
            {
                addr::addr_range const & r(ips[1]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }

            // 10.11.12.99:77
            {
                addr::addr_range const & r(ips[2]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with numeric only IPv4 addresses")
    {
        CATCH_START_SECTION("addr: simple numeric IPv4")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            addr::addr_range::vector_t ips(p.parse("4.3.1.2:3003"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);

            addr::addr_range const & r(ips[0]);
            CATCH_REQUIRE(r.has_from());
            CATCH_REQUIRE_FALSE(r.has_to());
            CATCH_REQUIRE_FALSE(r.is_range());
            CATCH_REQUIRE_FALSE(r.is_empty());
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            CATCH_REQUIRE(f.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
            CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "4.3.1.2");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "4.3.1.2");
            CATCH_REQUIRE(f.get_port() == 3003);
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
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: invalid domain name address when we only accept numeric IPs")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            addr::addr_range::vector_t ips(p.parse("www.example.com:4471"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Unknown address in \"www.example.com\" (no DNS lookup was allowed).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: invalid port: service name not allowed")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            addr::addr_range::vector_t ips(p.parse("192.168.255.32:https"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Invalid port in \"https\" (no service name lookup allowed).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }

    CATCH_START_SECTION("addr: IP as hostname")
    {
        for(int idx(0); idx < 10; ++idx)
        {
            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            in.sin_addr.s_addr = htonl(rand() ^ (rand() << 16));
            addr::addr a(in);
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);
            CATCH_REQUIRE(a.get_hostname().empty());
            std::string const ip(a.to_ipv4or6_string(addr::STRING_IP_ADDRESS));
            a.set_hostname(ip);
            CATCH_REQUIRE(a.get_hostname() == ip);
            CATCH_REQUIRE(a.is_hostname_an_ip());
            a.set_hostname("no.an.ip");
            CATCH_REQUIRE(a.get_hostname() == "no.an.ip");
            CATCH_REQUIRE_FALSE(a.is_hostname_an_ip());
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr: set interface")
    {
        addr::addr a;
        CATCH_REQUIRE(a.get_interface().empty());
        a.set_interface("eth0");
        CATCH_REQUIRE(a.get_interface() == "eth0");
        a.set_interface("epn3");
        CATCH_REQUIRE(a.get_interface() == "epn3");
        a.set_interface(std::string());
        CATCH_REQUIRE(a.get_interface().empty());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("ipv4::ports", "[ipv4]")
{
    CATCH_GIVEN("addr_parser() with IPv4 addresses and port")
    {
        CATCH_START_SECTION("addr: verify port")
        {
            for(int port(0); port < 65536; ++port)
            {
                int proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                addr::addr_parser p;
                p.set_protocol(proto);
                addr::addr_range::vector_t ips(p.parse("192.168.12.199:" + std::to_string(port)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "192.168.12.199");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "192.168.12.199");
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: default address with various port")
        {
            for(int idx(0); idx < 100; ++idx)
            {
                uint16_t const port(rand());
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_default_address("5.5.5.5");
                CATCH_REQUIRE(p.get_default_address4() == "5.5.5.5");
                CATCH_REQUIRE(p.get_default_address6() == "");
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(static_cast<int>(port))));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
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

        CATCH_START_SECTION("addr: address with default port")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                std::uint16_t const port(rand());
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                if((rand() & 1) == 0)
                {
                    p.set_default_port(port);
                }
                else
                {
                    std::string const port_str(std::to_string(static_cast<int>(port)));
                    p.set_default_port(port_str);
                }
                CATCH_REQUIRE(p.get_default_port() == port);
                addr::addr_range::vector_t ips(p.parse("5.5.5.5"));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PUBLIC);
                CATCH_REQUIRE_FALSE(f.is_lan());
                CATCH_REQUIRE_FALSE(f.is_lan(true));
                CATCH_REQUIRE_FALSE(f.is_lan(false));
                CATCH_REQUIRE(f.is_wan());
                CATCH_REQUIRE(f.is_wan(true));
                CATCH_REQUIRE(f.is_wan(false));
            }

            for(int idx(0); idx < 25; ++idx)
            {
                std::uint16_t const port(rand());
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                if((rand() & 1) == 0)
                {
                    p.set_default_port(port);
                }
                else
                {
                    std::string const port_str(std::to_string(static_cast<int>(port)));
                    p.set_default_port(port_str);
                }
                CATCH_REQUIRE(p.get_default_port() == port);
                addr::addr_range::vector_t ips(p.parse("5.5.5.5:"));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
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
}


CATCH_TEST_CASE("ipv4::masks", "[ipv4]")
{
    CATCH_GIVEN("addr_parser() of address:port/mask")
    {
        CATCH_START_SECTION("addr: mask allowed, but no mask")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port)));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            CATCH_REQUIRE(f.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
            std::string result("172.19.6.91:" + std::to_string(port) + "/32");
            CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
            CATCH_REQUIRE(f.is_lan());
            CATCH_REQUIRE(f.is_lan(true));
            CATCH_REQUIRE(f.is_lan(false));
            CATCH_REQUIRE_FALSE(f.is_wan());
            CATCH_REQUIRE_FALSE(f.is_wan(true));
            CATCH_REQUIRE_FALSE(f.is_wan(false));
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: empty mask")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.18.5.91:" + std::to_string(port) + "/"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            CATCH_REQUIRE(f.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
            std::string result("172.18.5.91:" + std::to_string(port) + "/32");
            CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
            CATCH_REQUIRE(f.is_lan());
            CATCH_REQUIRE(f.is_lan(true));
            CATCH_REQUIRE(f.is_lan(false));
            CATCH_REQUIRE_FALSE(f.is_wan());
            CATCH_REQUIRE_FALSE(f.is_wan(true));
            CATCH_REQUIRE_FALSE(f.is_wan(false));
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: one number masks")
        {
            for(int idx(0); idx <= 32; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port) + "/" + std::to_string(idx)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                //uint64_t const mask(std::numeric_limits<uint64_t>::max() << (32 - idx));
                //std::string mask_str(
                //          std::to_string((mask >> 24) & 255)
                //        + "."
                //        + std::to_string((mask >> 16) & 255)
                //        + "."
                //        + std::to_string((mask >>  8) & 255)
                //        + "."
                //        + std::to_string((mask >>  0) & 255));
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + std::to_string(idx));
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                CATCH_REQUIRE(f.get_mask_size() == 96 + idx);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: address like mask")
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
                uint8_t mask[4];
                do
                {
                    for(int j(0); j < 4; ++j)
                    {
                        mask[j] = rand();
                    }
                }
                while(mask[0] == 0      // make sure the mask is no just a number
                   && mask[1] == 0
                   && mask[2] == 0
                   && mask[3] == 0);
                switch(mask[0])
                {
                case 0xFF:
                    mask[0] &= ~(1 << (rand() & 7));
                    break;

                case 0xFE:
                case 0xFC:
                case 0xF8:
                case 0xF0:
                case 0xE0:
                case 0xC0:
                    mask[0] &= 0x7F;
                    break;

                case 0x80:
                    mask[0] |= 1 << (rand() % 6);
                    break;

                }
                std::string const mask_str(
                              std::to_string(static_cast<int>(mask[0]))
                            + "."
                            + std::to_string(static_cast<int>(mask[1]))
                            + "."
                            + std::to_string(static_cast<int>(mask[2]))
                            + "."
                            + std::to_string(static_cast<int>(mask[3])));
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port) + "/" + mask_str));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));

                // above we made sure that the mask was not just a number so
                // here we should always get -1
                //
                CATCH_REQUIRE(f.get_mask_size() == -1);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: address like mask when not allowed")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, false);
                // when specified as an IP, the mask can be absolutely anything
                uint8_t mask[4];
                do
                {
                    for(int j(0); j < 4; ++j)
                    {
                        mask[j] = rand();
                    }
                }
                while(mask[0] == 0      // make sure the mask is no just a number
                   && mask[1] == 0
                   && mask[2] == 0
                   && mask[3] == 0);
                switch(mask[0])
                {
                case 0xFF:
                    mask[0] &= ~(1 << (rand() & 7));
                    break;

                case 0xFE:
                case 0xFC:
                case 0xF8:
                case 0xF0:
                case 0xE0:
                case 0xC0:
                    mask[0] &= 0x7F;
                    break;

                case 0x80:
                    mask[0] |= 1 << (rand() % 6);
                    break;

                }
                std::string const mask_str(
                              std::to_string(static_cast<int>(mask[0]))
                            + "."
                            + std::to_string(static_cast<int>(mask[1]))
                            + "."
                            + std::to_string(static_cast<int>(mask[2]))
                            + "."
                            + std::to_string(static_cast<int>(mask[3])));
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port) + "/" + mask_str));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: address like default mask")
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
                uint8_t mask[4];
                for(int j(0); j < 4; ++j)
                {
                    mask[j] = rand();
                }
                std::string const mask_str(
                              std::to_string(static_cast<int>(mask[0]))
                            + "."
                            + std::to_string(static_cast<int>(mask[1]))
                            + "."
                            + std::to_string(static_cast<int>(mask[2]))
                            + "."
                            + std::to_string(static_cast<int>(mask[3])));
                p.set_default_mask(mask_str);
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                uint8_t verify_mask[16];
                f.get_mask(verify_mask);
                for(int j(0); j < 16 - 4; ++j)
                {
                    CATCH_REQUIRE(verify_mask[j] == 255);
                }
                for(int j(12); j < 16; ++j)
                {
                    CATCH_REQUIRE(verify_mask[j] == mask[j - 12]);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: address like mask with a default")
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
                uint8_t mask[4];
                for(int j(0); j < 4; ++j)
                {
                    mask[j] = rand();
                }
                std::string const mask_str(
                              std::to_string(static_cast<int>(mask[0]))
                            + "."
                            + std::to_string(static_cast<int>(mask[1]))
                            + "."
                            + std::to_string(static_cast<int>(mask[2]))
                            + "."
                            + std::to_string(static_cast<int>(mask[3])));

                uint8_t default_mask[4];
                for(int j(0); j < 4; ++j)
                {
                    default_mask[j] = rand();
                }
                std::string const default_mask_str(
                              std::to_string(static_cast<int>(default_mask[0]))
                            + "."
                            + std::to_string(static_cast<int>(default_mask[1]))
                            + "."
                            + std::to_string(static_cast<int>(default_mask[2]))
                            + "."
                            + std::to_string(static_cast<int>(default_mask[3])));
                p.set_default_mask(default_mask_str);

                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port) + "/" + mask_str));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                CATCH_REQUIRE(f.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(f.is_lan());
                CATCH_REQUIRE(f.is_lan(true));
                CATCH_REQUIRE(f.is_lan(false));
                CATCH_REQUIRE_FALSE(f.is_wan());
                CATCH_REQUIRE_FALSE(f.is_wan(true));
                CATCH_REQUIRE_FALSE(f.is_wan(false));
                uint8_t verify_mask[16];
                f.get_mask(verify_mask);
                for(int j(0); j < 16 - 4; ++j)
                {
                    CATCH_REQUIRE(verify_mask[j] == 255);
                }
                for(int j(12); j < 16; ++j)
                {
                    CATCH_REQUIRE(verify_mask[j] == mask[j - 12]);
                }
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: address like default mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);

                //p.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true); -- if we forget this one, the parsing fails with an error

                // when specified as an IP, the mask can be absolutely anything
                // (here the mask is a string an it will be parsed by the
                // parser if required)
                //
                uint8_t mask[4];
                for(int j(0); j < 4; ++j)
                {
                    mask[j] = rand();
                }
                std::string const mask_str(
                              std::to_string(static_cast<int>(mask[0]))
                            + "."
                            + std::to_string(static_cast<int>(mask[1]))
                            + "."
                            + std::to_string(static_cast<int>(mask[2]))
                            + "."
                            + std::to_string(static_cast<int>(mask[3])));
                p.set_default_mask(mask_str);
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: two addresses and a mask for a match / no match")
        {
            int const port1(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_allow(addr::allow_t::ALLOW_MASK, true);

            // parse the IP with a mask
            //
            int proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            p.set_protocol(proto);
            addr::addr_range::vector_t ips1(p.parse("192.168.0.0:" + std::to_string(port1) + "/16"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips1.size() == 1);
            addr::addr_range const & r1(ips1[0]);
            addr::addr f1(r1.get_from());
            CATCH_REQUIRE(f1.is_ipv4());
            CATCH_REQUIRE(f1.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f1.get_family() == AF_INET6);
            CATCH_REQUIRE(f1.to_ipv4_string(addr::STRING_IP_ALL) == "192.168.0.0:" + std::to_string(port1) + "/16");
            CATCH_REQUIRE(f1.to_ipv4or6_string(addr::STRING_IP_ALL) == "192.168.0.0:" + std::to_string(port1) + "/16");
            CATCH_REQUIRE(f1.get_port() == port1);
            CATCH_REQUIRE(f1.get_protocol() == proto);
            CATCH_REQUIRE(f1.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
            CATCH_REQUIRE(f1.is_lan());
            CATCH_REQUIRE(f1.is_lan(true));
            CATCH_REQUIRE(f1.is_lan(false));
            CATCH_REQUIRE_FALSE(f1.is_wan());
            CATCH_REQUIRE_FALSE(f1.is_wan(true));
            CATCH_REQUIRE_FALSE(f1.is_wan(false));

            // reuse parser
            //
            proto = rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP;
            p.set_protocol(proto);
            int const port2(rand() & 0xFFFF);
            addr::addr_range::vector_t ips2(p.parse("192.168.5.36:" + std::to_string(port2)));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips2.size() == 1);
            addr::addr_range const & r2(ips2[0]);
            addr::addr f2(r2.get_from());
            CATCH_REQUIRE(f2.is_ipv4());
            CATCH_REQUIRE(f2.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f2.get_family() == AF_INET6);
            CATCH_REQUIRE(f2.to_ipv4_string(addr::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port2) + "/32");
            CATCH_REQUIRE(f2.to_ipv4or6_string(addr::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port2) + "/32");
            CATCH_REQUIRE(f2.get_port() == port2);
            CATCH_REQUIRE(f2.get_protocol() == proto);
            CATCH_REQUIRE(f2.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
            CATCH_REQUIRE(f2.is_lan());
            CATCH_REQUIRE(f2.is_lan(true));
            CATCH_REQUIRE(f2.is_lan(false));
            CATCH_REQUIRE_FALSE(f2.is_wan());
            CATCH_REQUIRE_FALSE(f2.is_wan(true));
            CATCH_REQUIRE_FALSE(f2.is_wan(false));

            // 3rd with a mask along the full IP
            //
            proto = rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP;
            p.set_protocol(proto);
            int const port3(rand() & 0xFFFF);
            addr::addr_range::vector_t ips3(p.parse("192.168.5.36:" + std::to_string(port3) + "/16"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips3.size() == 1);
            addr::addr_range const & r3(ips3[0]);
            addr::addr f3(r3.get_from());
            CATCH_REQUIRE(f3.is_ipv4());
            CATCH_REQUIRE(f3.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(f3.get_family() == AF_INET6);
            CATCH_REQUIRE(f3.to_ipv4_string(addr::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port3) + "/16");
            CATCH_REQUIRE(f3.to_ipv4or6_string(addr::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port3) + "/16");
            CATCH_REQUIRE(f3.get_port() == port3);
            CATCH_REQUIRE(f3.get_protocol() == proto);
            CATCH_REQUIRE(f3.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
            CATCH_REQUIRE(f3.is_lan());
            CATCH_REQUIRE(f3.is_lan(true));
            CATCH_REQUIRE(f3.is_lan(false));
            CATCH_REQUIRE_FALSE(f3.is_wan());
            CATCH_REQUIRE_FALSE(f3.is_wan(true));
            CATCH_REQUIRE_FALSE(f3.is_wan(false));

            // just a side test
            //
            CATCH_REQUIRE(f1 != f2);
            CATCH_REQUIRE(f1 != f3);
            CATCH_REQUIRE(f2 == f3);

            // check whether p1 matches p2 and vice versa
            //
            CATCH_REQUIRE(f1.match(f2));          // f2 & mask1 == f1 & mask1
            CATCH_REQUIRE(f1.match(f3));          // f3 & mask1 == f1 & mask1

            CATCH_REQUIRE_FALSE(f2.match(f1));    // f1 & mask2 != f2 & mask2
            CATCH_REQUIRE(f2.match(f3));          // f3 & mask2 == f2 & mask2  (because f2 == f3 anyway)

            CATCH_REQUIRE(f3.match(f1));          // f1 & mask3 == f3 & mask3
            CATCH_REQUIRE(f3.match(f2));          // f2 & mask3 == f3 & mask3

            f3.apply_mask();

            CATCH_REQUIRE(f1 != f2);
            CATCH_REQUIRE(f1 == f3);
            CATCH_REQUIRE(f2 != f3);

            // re-run the match() calls with f3 since it changed...
            //
            CATCH_REQUIRE(f1.match(f3));          // f3 & mask1 == f1 & mask1

            CATCH_REQUIRE_FALSE(f2.match(f3));    // f3 & mask2 == f2 & mask2  (because f2 != f3 anymore)

            CATCH_REQUIRE(f3.match(f1));          // f1 & mask3 == f3 & mask3
            CATCH_REQUIRE(f3.match(f2));          // f2 & mask3 == f3 & mask3

            addr::addr fa; // by default an address is ANY and it matches everything
            CATCH_REQUIRE(fa.match(f1, true));
            CATCH_REQUIRE(fa.match(f2, true));
            CATCH_REQUIRE(fa.match(f3, true));
            std::uint8_t const verify_match[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
            std::uint8_t any_mask[16] = {};
            fa.get_mask(any_mask);
            CATCH_REQUIRE(memcmp(verify_match, any_mask, 16) == 0);
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv4::protocol", "[ipv4]")
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("addr: default protocol")
        {
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: set_protocol()")
        {
            // setup a random protocol
            //
            int const start_protocol([]()
                {
                    switch(rand() % 3)
                    {
                    case 0:
                        return IPPROTO_IP;

                    case 1:
                        return IPPROTO_TCP;

                    //case 2:
                    default:
                        return IPPROTO_UDP;

                    }
                }());
            a.set_protocol(start_protocol);

            // test 100 invalid protocols
            //
            for(int idx(0); idx < 100; ++idx)
            {
                int invalid_protocol;
                do
                {
                    invalid_protocol = rand();
                }
                while(invalid_protocol == IPPROTO_IP
                   || invalid_protocol == IPPROTO_TCP
                   || invalid_protocol == IPPROTO_UDP);
                CATCH_REQUIRE_THROWS_AS(a.set_protocol(invalid_protocol), addr::addr_invalid_argument);

                // make sure the protocol does not change on errors
                CATCH_REQUIRE(a.get_protocol() == start_protocol);
            }

            // null string is not allowed
            //
            CATCH_REQUIRE_THROWS_AS(a.set_protocol(nullptr), addr::addr_invalid_argument);

            // other "invalid" (unsupported, really) string protocols
            //
            CATCH_REQUIRE_THROWS_AS(a.set_protocol("unknown"), addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(a.set_protocol("invalid"), addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(a.set_protocol("other"), addr::addr_invalid_argument);

            // test all valid protocols (numeric)
            //
            a.set_protocol(IPPROTO_IP);
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_IP);
            a.set_protocol(IPPROTO_TCP);
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
            a.set_protocol(IPPROTO_UDP);
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_UDP);

            // test all valid protocols (ascii)
            //
            a.set_protocol("ip");
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_IP);
            a.set_protocol("tcp");
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
            a.set_protocol("udp");
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_UDP);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser()")
    {
        addr::addr_parser p;

        CATCH_START_SECTION("addr: verify default")
        {
            CATCH_REQUIRE(p.get_protocol() == -1);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: test 3 allowed protocols")
        {
            // by string
            //
            p.set_protocol("ip");
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_IP);
            p.set_protocol("tcp");
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_TCP);
            p.set_protocol("udp");
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_UDP);

            // numerically
            //
            p.set_protocol(IPPROTO_IP);
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_IP);
            p.set_protocol(IPPROTO_TCP);
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_TCP);
            p.set_protocol(IPPROTO_UDP);
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_UDP);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: verify clearing works")
        {
            p.set_protocol("ip");
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_IP);
            p.clear_protocol();
            CATCH_REQUIRE(p.get_protocol() == -1);

            p.set_protocol("tcp");
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_TCP);
            p.clear_protocol();
            CATCH_REQUIRE(p.get_protocol() == -1);

            p.set_protocol("udp");
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_UDP);
            p.clear_protocol();
            CATCH_REQUIRE(p.get_protocol() == -1);
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser with any protocol")
    {
        addr::addr a;

        CATCH_START_SECTION("addr: get address with all protocols")
        {
            addr::addr_parser p;
            //p.set_protocol(...); -- by default we'll get all the protocols supported
            addr::addr_range::vector_t ips(p.parse("127.0.0.1"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(!ips.empty());
            for(size_t idx(0); idx < ips.size(); ++idx)
            {
                addr::addr_range const & r(ips[idx]);
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                addr::addr f(r.get_from());
                if(f.is_ipv4())
                {
                    CATCH_REQUIRE(f.get_family() == AF_INET);
                    CATCH_REQUIRE_FALSE(f.get_family() == AF_INET6);
                    CATCH_REQUIRE(f.to_ipv4_string(addr::STRING_IP_ADDRESS) == "127.0.0.1");
                    CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "127.0.0.1");
                    CATCH_REQUIRE(f.get_port() == 0);
                    //CATCH_REQUIRE(f.get_protocol() == ...); -- may be TCP, UDP, IP
                    CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                    CATCH_REQUIRE(f.is_lan());
                    CATCH_REQUIRE(f.is_lan(true));
                    CATCH_REQUIRE(f.is_lan(false));
                    CATCH_REQUIRE_FALSE(f.is_wan());
                    CATCH_REQUIRE_FALSE(f.is_wan(true));
                    CATCH_REQUIRE_FALSE(f.is_wan(false));
                }
                else
                {
                    CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
                    CATCH_REQUIRE(f.get_family() == AF_INET6);
                    CATCH_REQUIRE(f.to_ipv6_string(addr::STRING_IP_ADDRESS) == "::1");
                    CATCH_REQUIRE(f.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::1");
                    CATCH_REQUIRE(f.get_port() == 0);
                    //CATCH_REQUIRE(f.get_protocol() == ...); -- may be TCP, UDP, IP
                    CATCH_REQUIRE(f.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                    CATCH_REQUIRE(f.is_lan());
                    CATCH_REQUIRE(f.is_lan(true));
                    CATCH_REQUIRE(f.is_lan(false));
                    CATCH_REQUIRE_FALSE(f.is_wan());
                    CATCH_REQUIRE_FALSE(f.is_wan(true));
                    CATCH_REQUIRE_FALSE(f.is_wan(false));
                }
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv4::network_type", "[ipv4]")
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("addr: any (0.0.0.0)")
        {
            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            in.sin_addr.s_addr = 0;

            // verify network type
            //
            a.set_ipv4(in);

            CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_ANY);
            CATCH_REQUIRE(a.get_network_type_string() == "Any");
            CATCH_REQUIRE_FALSE(a.is_lan());
            CATCH_REQUIRE_FALSE(a.is_lan(true));
            CATCH_REQUIRE_FALSE(a.is_lan(false));
            CATCH_REQUIRE(a.is_wan());
            CATCH_REQUIRE(a.is_wan(true));
            CATCH_REQUIRE_FALSE(a.is_wan(false));
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 10.x.x.x/8")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((10 << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 172.16.x.x/12")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((172 << 24)
                              | (((rand() & 15) | 16) << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 192.168.x.x/16")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((192 << 24)
                              |  (168 << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 100.66.x.x/10")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((100 << 24)
                              | (((rand() & 63) | 64) << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_CARRIER);
                CATCH_REQUIRE(a.get_network_type_string() == "Carrier");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 169.254.x.x/16")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((169 << 24)
                              |  (254 << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE(a.get_network_type_string() == "Local Link");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 224.x.x.x/4")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((((rand() & 15) | 224) << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_MULTICAST);
                CATCH_REQUIRE(a.get_network_type_string() == "Multicast");
                CATCH_REQUIRE_FALSE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE_FALSE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));

                // make sure no interface uses that IP
                //
                CATCH_REQUIRE(addr::find_addr_interface(a, false) == nullptr);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: private address 127.x.x.x/8")
        {
            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in in = sockaddr_in();
                in.sin_family = AF_INET;
                in.sin_port = htons(rand());
                uint32_t address((127 << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | ((rand() & 255) << 0));
                in.sin_addr.s_addr = htonl(address);

                // verify network type
                //
                a.set_ipv4(in);
                CATCH_REQUIRE(a.get_network_type() == addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(a.get_network_type_string() == "Loopback");
                CATCH_REQUIRE(a.is_lan());
                CATCH_REQUIRE(a.is_lan(true));
                CATCH_REQUIRE(a.is_lan(false));
                CATCH_REQUIRE_FALSE(a.is_wan());
                CATCH_REQUIRE_FALSE(a.is_wan(true));
                CATCH_REQUIRE_FALSE(a.is_wan(false));
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv4::network", "[ipv4]")
{
    CATCH_GIVEN("set_from_socket()")
    {
        CATCH_START_SECTION("addr: invalid socket")
        {
            addr::addr a;
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(-1, true),  addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(-1, false), addr::addr_invalid_argument);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: non-opened file descriptor")
        {
            addr::addr a;

            // unless we have a bug, there should not be any file descriptor
            // currently open with an ID of 1,000
            //
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(1000, true),  addr::addr_io_error);
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(1000, false), addr::addr_io_error);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr: unknown socket type")
        {
            addr::addr a;

            int s(socket(AF_UNIX, SOCK_STREAM, 0));
            CATCH_REQUIRE(s >= 0);
            std::shared_ptr<int> auto_free(&s, socket_deleter);

            // unless we have a bug, there should not be any file descriptor
            // currently open with an ID of 1,000
            //
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(s, true),  addr::addr_io_error);
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(s, false), addr::addr_invalid_state);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr/addr_parser: create a server (bind), but do not test it (yet)...")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("127.0.0.1:49999"));
            CATCH_REQUIRE(ips.size() >= 1);

            addr::addr & a(ips[0].get_from());
            int s(a.create_socket(addr::addr::SOCKET_FLAG_NONBLOCK | addr::addr::SOCKET_FLAG_CLOEXEC | addr::addr::SOCKET_FLAG_REUSE));
            CATCH_REQUIRE(s >= 0);
            std::shared_ptr<int> auto_free(&s, socket_deleter);

            CATCH_REQUIRE(a.bind(s) == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr/addr_parser: connect() with TCP to 127.0.0.1")
        {
            if(SNAP_CATCH2_NAMESPACE::g_tcp_port != -1)
            {
                addr::addr_parser p;
                addr::addr_range::vector_t ips(p.parse("127.0.0.1:" + std::to_string(SNAP_CATCH2_NAMESPACE::g_tcp_port)));
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
                CATCH_REQUIRE(b.is_ipv4());
                CATCH_REQUIRE(b.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(b.get_family() == AF_INET6);
                CATCH_REQUIRE(b.to_ipv4_string(addr::STRING_IP_ADDRESS)    == "127.0.0.1");
                CATCH_REQUIRE(b.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "127.0.0.1");

                // in this case we know what the port is since we specified
                // that when connecting
                //
                CATCH_REQUIRE(b.get_port() == SNAP_CATCH2_NAMESPACE::g_tcp_port);

                // now try this side (peer == false)
                //
                addr::addr c;
                c.set_from_socket(s, false);
                CATCH_REQUIRE(c.is_ipv4());
                CATCH_REQUIRE(c.get_family() == AF_INET);
                CATCH_REQUIRE_FALSE(c.get_family() == AF_INET6);
                CATCH_REQUIRE(c.to_ipv4_string(addr::STRING_IP_ADDRESS)    == "127.0.0.1");
                CATCH_REQUIRE(c.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "127.0.0.1");

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
                std::cout << "connect to 127.0.0.1 test skipped as no TCP port was specified on the command line." << std::endl;
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr/addr_parser: connect() with UDP to 127.0.0.1:53 which fails")
        {
            addr::addr_parser p;
            p.set_protocol("udp");
            addr::addr_range::vector_t ips(p.parse("127.0.0.1:53"));
            CATCH_REQUIRE(ips.size() >= 1);

            addr::addr & a(ips[0].get_from());
            int s(a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC));// | addr::addr::SOCKET_FLAG_REUSE));
            CATCH_REQUIRE(s >= 0);
            std::shared_ptr<int> auto_free(&s, socket_deleter);

            // addr::connect() does not support UDP
            //
            CATCH_REQUIRE(a.connect(s) == -1);

            addr::addr b;
            CATCH_REQUIRE_THROWS_AS(b.set_from_socket(s, true), addr::addr_io_error);
            CATCH_REQUIRE_FALSE(b.is_ipv4());
            CATCH_REQUIRE_FALSE(b.get_family() == AF_INET);
            CATCH_REQUIRE(b.get_family() == AF_INET6);
            CATCH_REQUIRE(b.to_ipv6_string(addr::STRING_IP_ADDRESS)    == "::");
            CATCH_REQUIRE(b.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(b.get_port() == 0);

            addr::addr c;
            c.set_from_socket(s, false);
            CATCH_REQUIRE(c.is_ipv4());
            CATCH_REQUIRE(c.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(c.get_family() == AF_INET6);
            CATCH_REQUIRE(c.to_ipv4_string(addr::STRING_IP_ADDRESS)    == "0.0.0.0");
            CATCH_REQUIRE(c.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "0.0.0.0");
            CATCH_REQUIRE(c.get_port() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr/addr_parser: bind() with UDP to 127.0.0.1:<auto> (as a \"client\") which works")
        {
            addr::addr_parser p;
            p.set_protocol("udp");
            addr::addr_range::vector_t ips(p.parse("127.0.0.1"));
            CATCH_REQUIRE(ips.size() >= 1);

            addr::addr & a(ips[0].get_from());
            int s(a.create_socket(addr::addr::SOCKET_FLAG_CLOEXEC));// | addr::addr::SOCKET_FLAG_REUSE));
            CATCH_REQUIRE(s >= 0);
            std::shared_ptr<int> auto_free(&s, socket_deleter);

            // succeeds, but the port is not known
            //
            CATCH_REQUIRE(a.bind(s) == 0);

            // this is a UDP socket, there is no other side so we get ANY
            //
            addr::addr b;
            CATCH_REQUIRE_THROWS_AS(b.set_from_socket(s, true), addr::addr_io_error);
            CATCH_REQUIRE_FALSE(b.is_ipv4());
            CATCH_REQUIRE_FALSE(b.get_family() == AF_INET);
            CATCH_REQUIRE(b.get_family() == AF_INET6);
            CATCH_REQUIRE(b.to_ipv6_string(addr::STRING_IP_ADDRESS)    == "::");
            CATCH_REQUIRE(b.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "::");
            CATCH_REQUIRE(b.get_port() == 0);

            // now try this side (peer == false) and again it is "any"
            // since it failed connecting
            //
            addr::addr c;
            c.set_from_socket(s, false);
            CATCH_REQUIRE(c.is_ipv4());
            CATCH_REQUIRE(c.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(c.get_family() == AF_INET6);
            CATCH_REQUIRE(c.to_ipv4_string(addr::STRING_IP_ADDRESS)    == "127.0.0.1");
            CATCH_REQUIRE(c.to_ipv4or6_string(addr::STRING_IP_ADDRESS) == "127.0.0.1");

            // if this worked, the port is > 1023 (it was auto-allocated)
            //
            CATCH_REQUIRE(c.get_port() > 1023);

            // since the a.bind() is expected to read that port, it should
            // equal the one in `a`
            //
            CATCH_REQUIRE(a.get_port() == c.get_port());
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv4::string_to_addr", "[ipv4]")
{
    CATCH_GIVEN("string_to_addr() ipv4")
    {
        CATCH_START_SECTION("string_to_addr: empty address without defaults")
        {
            addr::addr a(addr::string_to_addr(std::string()));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = 0;
            in.sin_addr.s_addr = htonl((0 << 24) | (0 << 16) | (0 << 8) | (0 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: explicit defaults")
        {
            addr::addr a(addr::string_to_addr("5.14.34.111", std::string(), -1, std::string(), false));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = 0;
            in.sin_addr.s_addr = htonl((5 << 24) | (14 << 16) | (34 << 8) | (111 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: defaults")
        {
            addr::addr a(addr::string_to_addr("7.149.104.211"));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = 0;
            in.sin_addr.s_addr = htonl((7 << 24) | (149 << 16) | (104 << 8) | (211 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr & default addr")
        {
            addr::addr a(addr::string_to_addr("37.149.174.11", "1.205.32.11"));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = 0;
            in.sin_addr.s_addr = htonl((37 << 24) | (149 << 16) | (174 << 8) | (11 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: no addr, expect default addr")
        {
            addr::addr a(addr::string_to_addr("", "1.205.32.11"));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = 0;
            in.sin_addr.s_addr = htonl((1 << 24) | (205 << 16) | (32 << 8) | (11 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr and port, with a default port")
        {
            addr::addr a(addr::string_to_addr("69.109.223.17:697", "1.205.32.11", 123));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(697);
            in.sin_addr.s_addr = htonl((69 << 24) | (109 << 16) | (223 << 8) | (17 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr without port, with a default port")
        {
            addr::addr a(addr::string_to_addr("169.209.23.217", "1.205.32.11", 123));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(123);
            in.sin_addr.s_addr = htonl((169 << 24) | (209 << 16) | (23 << 8) | (217 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr without port but protocol")
        {
            addr::addr a(addr::string_to_addr("4.5.123.7", "1.205.32.11", 60000, "tcp"));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(60000);
            in.sin_addr.s_addr = htonl((4 << 24) | (5 << 16) | (123 << 8) | (7 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr with port and protocol")
        {
            addr::addr a(addr::string_to_addr("204.105.13.9:65", "1.205.32.11", 60000, "tcp"));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(65);
            in.sin_addr.s_addr = htonl((204 << 24) | (105 << 16) | (13 << 8) | (9 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr with port and protocol but no mask, albeit allowed")
        {
            addr::addr a(addr::string_to_addr("94.95.131.18:765", "11.205.32.21", 54003, "tcp", true));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(765);
            in.sin_addr.s_addr = htonl((94 << 24) | (95 << 16) | (131 << 8) | (18 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFF);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr with port and protocol and mask, albeit allowed")
        {
            addr::addr a(addr::string_to_addr("44.45.141.48:765/30", "11.205.32.21", 54003, "tcp", true));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(765);
            in.sin_addr.s_addr = htonl((44 << 24) | (45 << 16) | (141 << 8) | (48 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xFF);
            CATCH_REQUIRE(mask[13] == 0xFF);
            CATCH_REQUIRE(mask[14] == 0xFF);
            CATCH_REQUIRE(mask[15] == 0xFC);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr with port and protocol and mask, albeit allowed")
        {
            addr::addr a(addr::string_to_addr("160.0.0.0:1675/4", "11.205.32.21", 14003, "udp", true));

            CATCH_REQUIRE(a.is_ipv4());
            CATCH_REQUIRE(a.get_family() == AF_INET);
            CATCH_REQUIRE_FALSE(a.get_family() == AF_INET6);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(1675);
            in.sin_addr.s_addr = htonl((160 << 24) | (0 << 16) | (0 << 8) | (0 << 0));

            // test string_to_addr accuracy
            //
            struct sockaddr_in out;
            a.get_ipv4(out);
            CATCH_REQUIRE(memcmp(&out, &in, sizeof(struct sockaddr_in)) == 0);

            uint8_t mask[16];
            a.get_mask(mask);
            CATCH_REQUIRE(mask[ 0] == 0xFF);
            CATCH_REQUIRE(mask[ 1] == 0xFF);
            CATCH_REQUIRE(mask[ 2] == 0xFF);
            CATCH_REQUIRE(mask[ 3] == 0xFF);
            CATCH_REQUIRE(mask[ 4] == 0xFF);
            CATCH_REQUIRE(mask[ 5] == 0xFF);
            CATCH_REQUIRE(mask[ 6] == 0xFF);
            CATCH_REQUIRE(mask[ 7] == 0xFF);
            CATCH_REQUIRE(mask[ 8] == 0xFF);
            CATCH_REQUIRE(mask[ 9] == 0xFF);
            CATCH_REQUIRE(mask[10] == 0xFF);
            CATCH_REQUIRE(mask[11] == 0xFF);
            CATCH_REQUIRE(mask[12] == 0xF0);
            CATCH_REQUIRE(mask[13] == 0x00);
            CATCH_REQUIRE(mask[14] == 0x00);
            CATCH_REQUIRE(mask[15] == 0x00);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: addr with port and invalid protocol so we get an exception")
        {
            CATCH_REQUIRE_THROWS_AS(addr::string_to_addr("169.60.33.0:9322/24", std::string(), -1, "icmp", true),
                                                                        addr::addr_invalid_argument);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("string_to_addr: definitely not a valid address")
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  addr::string_to_addr("not an address")
                , addr::addr_invalid_argument
                , Catch::Matchers::ExceptionMessage("addr_error: the address \"not an address\" could not be converted to a single address in string_to_addr(), found 0 entries instead."));
        }
        CATCH_END_SECTION()
    }
}


// vim: ts=4 sw=4 et
