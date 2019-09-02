// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
//
// Project: https://snapwebsites.org/project/libaddr
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
 * Also, the IPv6 test include a certain number of default/global
 * test because by default the addr class implements an IPv6 object.
 */

// self
//
#include    "test_addr_main.h"


// addr lib
//
#include    <libaddr/iface.h>


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

/** \brief Delete an ifaddrs structure.
 *
 * This deleter is used to make sure all the ifaddrs get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ia  The ifaddrs structure to free.
 */
void socket_deleter(int * s)
{
    close(*s);
}


}
// no name namespace



CATCH_TEST_CASE( "ipv4::invalid_input", "[ipv4]" )
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_SECTION("set IPv4 with an invalid family")
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
    }

    CATCH_GIVEN("addr_parser() with IPv4 settings")
    {
        addr::addr_parser a;

        CATCH_SECTION("invalid allow flags (too small)")
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
                addr::addr_parser::flag_t const flag(static_cast<addr::addr_parser::flag_t>(n));

                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, true), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, false), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.get_allow(flag), addr::addr_invalid_argument);
            }
        }

        CATCH_SECTION("invalid allow flags (too large)")
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
                while(n < static_cast<int>(addr::addr_parser::flag_t::FLAG_max));
                addr::addr_parser::flag_t const flag(static_cast<addr::addr_parser::flag_t>(n));

                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, true), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.set_allow(flag, false), addr::addr_invalid_argument);
                CATCH_REQUIRE_THROWS_AS(a.get_allow(flag), addr::addr_invalid_argument);
            }
        }
    }

    CATCH_GIVEN("addr_parser() with IPv4 addresses")
    {
        CATCH_SECTION("bad address")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("{bad-ip}"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Invalid address in \"{bad-ip}\" error -2 -- Name or service not known (errno: 2 -- No such file or directory).\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }

        CATCH_SECTION("required address")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::addr_parser::flag_t::REQUIRED_ADDRESS, true);
            addr::addr_range::vector_t ips(p.parse(""));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Required address is missing.\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }
    }

    CATCH_GIVEN("addr_parser() with IPv4 ports")
    {
        CATCH_SECTION("required port")
        {
            // optional + required -> required
            {
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_allow(addr::addr_parser::flag_t::REQUIRED_PORT, true);
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
                p.set_allow(addr::addr_parser::flag_t::PORT, false);
                p.set_allow(addr::addr_parser::flag_t::REQUIRED_PORT, true);
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

        CATCH_SECTION("port not allowed")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::addr_parser::flag_t::PORT, false);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::REQUIRED_PORT));
            addr::addr_range::vector_t ips(p.parse("1.2.3.4:123"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Port not allowed (1.2.3.4:123).\n");
            CATCH_REQUIRE(p.has_errors());
            p.clear_errors();
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 0);
        }

        CATCH_SECTION("invalid port")
        {
            addr::addr_parser p;

            // so to a different default value
            //
            int const default_port(rand() & 0xFFFF);
            p.set_default_port(default_port);

            for(int idx(0); idx < 25; ++idx)
            {
                int port;
                do
                {
                    port = rand() ^ (rand() << 16);
                }
                while(port >= -1 && port <= 65535); // -1 is valid here, it represents "no default port defined"
                CATCH_REQUIRE_THROWS_AS(p.set_default_port(port), addr::addr_invalid_argument);

                // verify port unchanged
                //
                CATCH_REQUIRE(p.get_default_port() == default_port);
            }
        }
    }

    CATCH_GIVEN("addr_parser() with invalid masks")
    {
        CATCH_SECTION("really large numbers (over 1000)")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 1001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(p.flag_t::MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Mask number too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }

        CATCH_SECTION("ipv4 mask is limited between 0 and 32")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 33);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(p.flag_t::MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Unsupported mask size (" + std::to_string(mask) + ", expected 32 at the most for an IPv4).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }

        CATCH_SECTION("ipv4 mask cannot use name")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::addr_parser::flag_t::MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/localhost"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid mask in \"/localhost\", error -2 -- Name or service not known (errno: 0 -- Success).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }

        CATCH_SECTION("ipv4 mask mismatch (mask uses ipv6)")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::addr_parser::flag_t::MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/[1:2:3:4:5:6:7:8]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The address uses the IPv4 syntax, the mask cannot use IPv6.\n");
            CATCH_REQUIRE(ips.size() == 0);
        }

        CATCH_SECTION("ipv4 mask mismatch (mask uses ipv6 without [...])")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::addr_parser::flag_t::MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port) + "/1:2:3:4:5:6:7:8"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Incompatible address between the address and mask address (first was an IPv4 second an IPv6).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
    }

    CATCH_GIVEN("addr_parser() with invalid protocols")
    {
        CATCH_SECTION("invalid names")
        {
            addr::addr_parser p;

            // not changing default protocol
            //
            CATCH_REQUIRE(p.get_protocol() == -1);
            CATCH_REQUIRE_THROWS_AS(p.set_protocol("igmp"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_protocol() == -1);

            // change protocol to another valid value first
            //
            p.set_protocol("tcp");
            CATCH_REQUIRE_THROWS_AS(p.set_protocol("icmp"), addr::addr_invalid_argument);
            CATCH_REQUIRE(p.get_protocol() == IPPROTO_TCP);
        }

        CATCH_SECTION("invalid numbers")
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
    }
}


CATCH_TEST_CASE( "ipv4::addr", "[ipv4]" )
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_SECTION("not an IPv4")
        {
            CATCH_REQUIRE_FALSE(a.is_ipv4());

            struct sockaddr_in in;
            CATCH_REQUIRE_THROWS_AS(a.get_ipv4(in), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY),          addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_BRACKETS),      addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_PORT),          addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_MASK),          addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_BRACKETS_MASK), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL),           addr::addr_invalid_state);
        }

        CATCH_SECTION("default network type (0.0.0.0)")
        {
            CATCH_REQUIRE(a.is_default());
            CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_ANY);
            CATCH_REQUIRE(a.get_network_type_string() == "Any");
        }

        CATCH_SECTION("IPv6 ANY")
        {
            struct sockaddr_in6 in6;
            a.get_ipv6(in6);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[0] == 0);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[1] == 0);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[2] == 0);
            CATCH_REQUIRE(in6.sin6_addr.s6_addr32[3] == 0);
            CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY)          == "::");
            CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS)      == "[::]");
            CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_PORT)          == "[::]:0");
            CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_MASK)          == "::/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
            CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS_MASK) == "[::]/[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
            CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL)           == "[::]:0/[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
        }

        CATCH_SECTION("IPv4 or IPv6 string")
        {
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)          == "::");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS)      == "[::]");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)          == "[::]:0");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_MASK)          == "::/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS_MASK) == "[::]/[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
            CATCH_REQUIRE(a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL)           == "[::]:0/[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
        }

        CATCH_SECTION("interface determination")
        {
            CATCH_REQUIRE(addr::find_addr_interface(a, false) == nullptr);
            CATCH_REQUIRE(addr::find_addr_interface(a, true) != nullptr);
        }

        CATCH_SECTION("default name/service/port/protocol")
        {
            CATCH_REQUIRE(a.get_name() == std::string());
            CATCH_REQUIRE(a.get_service() == std::string());
            CATCH_REQUIRE(a.get_port() == 0);
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
        }

        CATCH_SECTION("compare with self")
        {
            CATCH_REQUIRE(a == a);
            CATCH_REQUIRE_FALSE(a != a);
            CATCH_REQUIRE_FALSE(a < a);
            CATCH_REQUIRE(a <= a);
            CATCH_REQUIRE_FALSE(a > a);
            CATCH_REQUIRE(a >= a);
        }

        CATCH_SECTION("compare with another 0.0.0.0")
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

        CATCH_SECTION("compare with IPv4 127.0.0.1")
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
    }
}


CATCH_TEST_CASE( "ipv4::address", "[ipv4]" )
{
    CATCH_GIVEN("addr() with an IPv4")
    {
        addr::addr a;

        CATCH_SECTION("set_ipv4() / get_ipv4()")
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

        CATCH_SECTION("set_ipv4() / to_ipv4_string()")
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
                CATCH_REQUIRE(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY)          == ip);
                CATCH_REQUIRE(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_BRACKETS)      == ip);
                CATCH_REQUIRE(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_PORT)          == ip + ":" + port_str);
                CATCH_REQUIRE(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_MASK)          == ip + "/255.255.255.255"); // will change to 32 at some point
                CATCH_REQUIRE(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_BRACKETS_MASK) == ip + "/255.255.255.255");
                CATCH_REQUIRE(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL)           == ip + ":" + port_str + "/255.255.255.255");
            }
        }

        CATCH_SECTION("name of various IPs")
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
    }

    CATCH_GIVEN("addr_parser() with IPv4 addresses")
    {
        CATCH_SECTION("verify basics")
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
            CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
            CATCH_REQUIRE(f.get_port() == 0);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            uint8_t mask[16] = {};
            f.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
        }

        CATCH_SECTION("verify default address")
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

        CATCH_SECTION("verify default mask")
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

        CATCH_SECTION("verify default allow flags")
        {
            addr::addr_parser p;

            for(int idx(0); idx < static_cast<int>(addr::addr_parser::flag_t::FLAG_max); ++idx)
            {
                switch(static_cast<addr::addr_parser::flag_t>(idx))
                {
                case addr::addr_parser::flag_t::ADDRESS:
                case addr::addr_parser::flag_t::PORT:
                    // only the ADDRESS and PORT are true by default
                    //
                    CATCH_REQUIRE(p.get_allow(static_cast<addr::addr_parser::flag_t>(idx)));
                    break;

                default:
                    CATCH_REQUIRE_FALSE(p.get_allow(static_cast<addr::addr_parser::flag_t>(idx)));
                    break;

                }
            }
        }

        CATCH_SECTION("verify contradictory flags")
        {
            addr::addr_parser p;

            // by default we start with false
            //
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // check setting MULTI_ADDRESSES_COMMAS to true
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS, true);
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_COMMAS_AND_SPACES
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES, true);
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // add MULTI_PORTS_COMMAS
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS, true);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_COMMAS_AND_SPACES only
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES, true);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_COMMAS second, order should not affect anything
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS, true);
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // back to MULTI_PORTS_COMMAS
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS, true);
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));

            // add MULTI_ADDRESSES_COMMAS first now
            //
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS, true);
            CATCH_REQUIRE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES));
            CATCH_REQUIRE_FALSE(p.get_allow(addr::addr_parser::flag_t::MULTI_PORTS_COMMAS));
        }

        CATCH_SECTION("default address")
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
            CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.5.5.5");
            CATCH_REQUIRE(f.get_port() == 0);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
        }

        CATCH_SECTION("address, not port allowed")
        {
            // specific address with a default
            {
                addr::addr_parser p;
                p.set_allow(addr::addr_parser::flag_t::PORT, false);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "9.9.9.9");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            }

            // only a default address
            {
                addr::addr_parser p;
                p.set_allow(addr::addr_parser::flag_t::PORT, false);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.5.5.5");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            }
        }
    }

    CATCH_GIVEN("addr_parser() with multiple IPv4 addresses in one string")
    {
        CATCH_SECTION("3 IPs separated by commas")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS, true);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }

        CATCH_SECTION("3 IPs separated by spaces")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_SPACES, true);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }

        CATCH_SECTION("3 IPs separated by commas and/or spaces")
        {
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES, true);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1.2.3.4");
                CATCH_REQUIRE(f.get_port() == 55);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.6.7.8");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5.6.7.8");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.11.12.99");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.11.12.99");
                CATCH_REQUIRE(f.get_port() == 77);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                f.get_mask(mask);
                for(int idx(0); idx < 16; ++idx)
                {
                    CATCH_REQUIRE(mask[idx] == 255);
                }
            }
        }
    }
}


CATCH_TEST_CASE( "ipv4::ports", "[ipv4]" )
{
    CATCH_GIVEN("addr_parser() with IPv4 addresses and port")
    {
        CATCH_SECTION("verify port")
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "192.168.12.199");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "192.168.12.199");
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
            }
        }

        CATCH_SECTION("default address with various port")
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            }
        }

        CATCH_SECTION("address with default port")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                uint16_t const port(rand());
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_default_port(port);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            }

            for(int idx(0); idx < 25; ++idx)
            {
                uint16_t const port(rand());
                addr::addr_parser p;
                p.set_protocol(IPPROTO_TCP);
                p.set_default_port(port);
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
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "5.5.5.5:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            }
        }
    }
}


CATCH_TEST_CASE( "ipv4::masks", "[ipv4]" )
{
    CATCH_GIVEN("addr_parser() of address:port/mask")
    {
        CATCH_SECTION("mask allowed, but no mask")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::addr_parser::flag_t::MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.19.6.91:" + std::to_string(port)));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            std::string result("172.19.6.91:" + std::to_string(port) + "/255.255.255.255");
            CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
        }

        CATCH_SECTION("empty mask")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::addr_parser::flag_t::MASK, true);
            addr::addr_range::vector_t ips(p.parse("172.18.5.91:" + std::to_string(port) + "/"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE(f.is_ipv4());
            std::string result("172.18.5.91:" + std::to_string(port) + "/255.255.255.255");
            CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
        }

        CATCH_SECTION("one number masks")
        {
            for(int idx(0); idx <= 32; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::addr_parser::flag_t::MASK, true);
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port) + "/" + std::to_string(idx)));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                uint64_t const mask(std::numeric_limits<uint64_t>::max() << (32 - idx));
                std::string mask_str(
                          std::to_string((mask >> 24) & 255)
                        + "."
                        + std::to_string((mask >> 16) & 255)
                        + "."
                        + std::to_string((mask >>  8) & 255)
                        + "."
                        + std::to_string((mask >>  0) & 255));
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
            }
        }

        CATCH_SECTION("address like mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::addr_parser::flag_t::MASK, true);
                // when specified as an IP, the mask can be absolutely anything
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
                addr::addr_range::vector_t ips(p.parse("172.17.3.91:" + std::to_string(port) + "/" + mask_str));
                CATCH_REQUIRE_FALSE(p.has_errors());
                CATCH_REQUIRE(ips.size() == 1);
                addr::addr_range const & r(ips[0]);
                addr::addr f(r.get_from());
                CATCH_REQUIRE(f.is_ipv4());
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
            }
        }

        CATCH_SECTION("address like default mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::addr_parser::flag_t::MASK, true);
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
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
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

        CATCH_SECTION("address like mask with a default")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::addr_parser::flag_t::MASK, true);

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
                std::string result("172.17.3.91:" + std::to_string(port) + "/" + mask_str);
                CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
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

        CATCH_SECTION("two addresses and a mask for a match / no match")
        {
            int const port1(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_allow(addr::addr_parser::flag_t::MASK, true);

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
            CATCH_REQUIRE(f1.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == "192.168.0.0:" + std::to_string(port1) + "/255.255.0.0");
            CATCH_REQUIRE(f1.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == "192.168.0.0:" + std::to_string(port1) + "/255.255.0.0");
            CATCH_REQUIRE(f1.get_port() == port1);
            CATCH_REQUIRE(f1.get_protocol() == proto);
            CATCH_REQUIRE(f1.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);

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
            CATCH_REQUIRE(f2.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port2) + "/255.255.255.255");
            CATCH_REQUIRE(f2.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port2) + "/255.255.255.255");
            CATCH_REQUIRE(f2.get_port() == port2);
            CATCH_REQUIRE(f2.get_protocol() == proto);
            CATCH_REQUIRE(f2.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);

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
            CATCH_REQUIRE(f3.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port3) + "/255.255.0.0");
            CATCH_REQUIRE(f3.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == "192.168.5.36:" + std::to_string(port3) + "/255.255.0.0");
            CATCH_REQUIRE(f3.get_port() == port3);
            CATCH_REQUIRE(f3.get_protocol() == proto);
            CATCH_REQUIRE(f3.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);

            // just a side test
            //
            CATCH_REQUIRE(f1 != f2);
            CATCH_REQUIRE(f1 != f3);
            CATCH_REQUIRE(f2 == f3);

            // check whether p1 matches p2 and vice versa
            //
            CATCH_REQUIRE(f1.match(f2));          // f2 & mask1 == f1
            CATCH_REQUIRE(f1.match(f3));          // f3 & mask1 == f1

            CATCH_REQUIRE_FALSE(f2.match(f1));    // f1 & mask2 != f2
            CATCH_REQUIRE(f2.match(f3));          // f3 & mask2 == f2  (because f2 == f3 anyway)

            CATCH_REQUIRE(f3.match(f1));          // f1 & mask3 == f3
            CATCH_REQUIRE(f3.match(f2));          // f2 & mask3 == f3

            f3.apply_mask();

            CATCH_REQUIRE(f1 != f2);
            CATCH_REQUIRE(f1 == f3);
            CATCH_REQUIRE(f2 != f3);

            // re-run the match() calls with f3 since it changed...
            //
            CATCH_REQUIRE(f1.match(f3));          // f3 & mask1 == f1

            CATCH_REQUIRE_FALSE(f2.match(f3));    // f3 & mask2 == f2  (because f2 != f3 anymore)

            CATCH_REQUIRE(f3.match(f1));          // f1 & mask3 == f3
            CATCH_REQUIRE(f3.match(f2));          // f2 & mask3 == f3
        }
    }
}


CATCH_TEST_CASE( "ipv4::protocol", "[ipv4]" )
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_SECTION("default protocol")
        {
            CATCH_REQUIRE(a.get_protocol() == IPPROTO_TCP);
        }

        CATCH_SECTION("set_protocol()")
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
            CATCH_REQUIRE_THROWS_AS(a.set_protocol("icmp"), addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(a.set_protocol("raw"), addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(a.set_protocol("hmp"), addr::addr_invalid_argument);

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
    }

    CATCH_GIVEN("addr_parser()")
    {
        addr::addr_parser p;

        CATCH_SECTION("verify default")
        {
            CATCH_REQUIRE(p.get_protocol() == -1);
        }

        CATCH_SECTION("test 3 allowed protocols")
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

        CATCH_SECTION("verify clearing works")
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
    }

    CATCH_GIVEN("addr_parser with any protocol")
    {
        addr::addr a;

        CATCH_SECTION("get address with all protocols")
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
                    CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "127.0.0.1");
                    CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "127.0.0.1");
                    CATCH_REQUIRE(f.get_port() == 0);
                    //CATCH_REQUIRE(f.get_protocol() == ...); -- may be TCP, UDP, IP
                    CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                }
                else
                {
                    CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "::1");
                    CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "::1");
                    CATCH_REQUIRE(f.get_port() == 0);
                    //CATCH_REQUIRE(f.get_protocol() == ...); -- may be TCP, UDP, IP
                    CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                }
            }
        }
    }
}


CATCH_TEST_CASE( "ipv4::network_type", "[ipv4]" )
{
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_SECTION("any (0.0.0.0)")
        {
            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            in.sin_addr.s_addr = 0;

            // verify network type
            //
            a.set_ipv4(in);

            CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_ANY);
            CATCH_REQUIRE(a.get_network_type_string() == "Any");
        }

        CATCH_SECTION("private address 10.x.x.x/8")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
            }
        }

        CATCH_SECTION("private address 172.16.x.x/12")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
            }
        }

        CATCH_SECTION("private address 192.168.x.x/16")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
            }
        }

        CATCH_SECTION("private address 100.66.x.x/10")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_CARRIER);
                CATCH_REQUIRE(a.get_network_type_string() == "Carrier");
            }
        }

        CATCH_SECTION("private address 169.254.x.x/16")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE(a.get_network_type_string() == "Local Link");
            }
        }

        CATCH_SECTION("private address 224.x.x.x/4")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_MULTICAST);
                CATCH_REQUIRE(a.get_network_type_string() == "Multicast");

                // make sure no interface uses that IP
                //
                CATCH_REQUIRE(addr::find_addr_interface(a, false) == nullptr);
            }
        }

        CATCH_SECTION("private address 127.x.x.x/8")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(a.get_network_type_string() == "Loopback");
            }
        }
    }
}


CATCH_TEST_CASE( "ipv4::network", "[ipv4]" )
{
    CATCH_GIVEN("set_from_socket()")
    {
        CATCH_SECTION("invalid socket")
        {
            addr::addr a;
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(-1, true),  addr::addr_invalid_argument);
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(-1, false), addr::addr_invalid_argument);
        }

        CATCH_SECTION("non-opened file descriptor")
        {
            addr::addr a;

            // unless we have a bug, there should not be any file descriptor
            // currently open with an ID of 1,000
            //
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(1000, true),  addr::addr_io_error);
            CATCH_REQUIRE_THROWS_AS(a.set_from_socket(1000, false), addr::addr_io_error);
        }

        CATCH_SECTION("unknown socket type")
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

        CATCH_SECTION("create a server, but do not test it (yet)...")
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

        CATCH_SECTION("connect with TCP to 127.0.0.1")
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
                CATCH_REQUIRE(b.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "127.0.0.1");
                CATCH_REQUIRE(b.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "127.0.0.1");

                // in this case we know what the port is since we specified
                // that when connecting
                //
                CATCH_REQUIRE(b.get_port() == SNAP_CATCH2_NAMESPACE::g_tcp_port);

                // now try this side (peer == false)
                //
                addr::addr c;
                c.set_from_socket(s, false);
                CATCH_REQUIRE(c.is_ipv4());
                CATCH_REQUIRE(c.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "127.0.0.1");
                CATCH_REQUIRE(c.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "127.0.0.1");

                // we cannot be sure of the port, there is a range we could
                // test better (more constraining) but for this test is
                // certainly does not matter much; it has to be more than
                // 1023, though
                //
                CATCH_REQUIRE(c.get_port() > 1023);
            }
            else
            {
                std::cout << "connect to 127.0.0.1 test skipped as no TCP port was specified on the command line." << std::endl;
            }
        }

        CATCH_SECTION("connect with UDP to 127.0.0.1")
        {
            addr::addr_parser p;
            p.set_protocol("udp");
            addr::addr_range::vector_t ips(p.parse("127.0.0.1:53"));
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
            CATCH_REQUIRE(b.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "::");
            CATCH_REQUIRE(b.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "::");

            // in this case we know what the port is since we specified
            // that when connecting
            //
            CATCH_REQUIRE(b.get_port() == 0);

            // now try this side (peer == false)
            //
            addr::addr c;
            c.set_from_socket(s, false);
            CATCH_REQUIRE(c.is_ipv4());
            CATCH_REQUIRE(c.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "0.0.0.0");
            CATCH_REQUIRE(c.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "0.0.0.0");

            // we cannot be sure of the port, there is a range we could
            // test better (more constraining) but for this test is
            // certainly does not matter much; it has to be more than
            // 1023, though
            //
            CATCH_REQUIRE(c.get_port() == 0);
        }
    }
}


CATCH_TEST_CASE( "ipv4::string_to_addr", "[ipv4]" )
{
    CATCH_GIVEN("string_to_addr() ipv4")
    {
        CATCH_SECTION("empty address without defaults")
        {
            addr::addr a(addr::string_to_addr(std::string()));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("explicit defaults")
        {
            addr::addr a(addr::string_to_addr("5.14.34.111", std::string(), -1, std::string(), false));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("defaults")
        {
            addr::addr a(addr::string_to_addr("7.149.104.211"));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr & default addr")
        {
            addr::addr a(addr::string_to_addr("37.149.174.11", "1.205.32.11"));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("no addr, expect default addr")
        {
            addr::addr a(addr::string_to_addr("", "1.205.32.11"));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr and port, with a default port")
        {
            addr::addr a(addr::string_to_addr("69.109.223.17:697", "1.205.32.11", 123));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr without port, with a default port")
        {
            addr::addr a(addr::string_to_addr("169.209.23.217", "1.205.32.11", 123));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr without port but protocol")
        {
            addr::addr a(addr::string_to_addr("4.5.123.7", "1.205.32.11", 60000, "tcp"));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr with port and protocol")
        {
            addr::addr a(addr::string_to_addr("204.105.13.9:65", "1.205.32.11", 60000, "tcp"));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr with port and protocol but no mask, albeit allowed")
        {
            addr::addr a(addr::string_to_addr("94.95.131.18:765", "11.205.32.21", 54003, "tcp", true));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr with port and protocol and mask, albeit allowed")
        {
            addr::addr a(addr::string_to_addr("44.45.141.48:765/30", "11.205.32.21", 54003, "tcp", true));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr with port and protocol and mask, albeit allowed")
        {
            addr::addr a(addr::string_to_addr("160.0.0.0:1675/4", "11.205.32.21", 14003, "udp", true));

            CATCH_REQUIRE(a.is_ipv4());

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
        CATCH_SECTION("addr with port and invalid protocol so we get an exception")
        {
            CATCH_REQUIRE_THROWS_AS(addr::string_to_addr("169.60.33.0:9322/24", std::string(), -1, "icmp", true),
                                                                        addr::addr_invalid_argument);
        }
    }
    // TODO: add ipv6 tests, although at this point it's not too
    //       important here, it may change in the future
    //

//addr string_to_addr(
//          std::string const & a
//        , std::string const & default_address = std::string()
//        , int default_port = -1
//        , std::string const & protocol = std::string()
//        , bool mask = false);
}


// vim: ts=4 sw=4 et
