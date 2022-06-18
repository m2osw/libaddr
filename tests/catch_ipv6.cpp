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

// addr lib
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

        CATCH_START_SECTION("ipv6::addr: set IPv6 with an invalid family")
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
        CATCH_START_SECTION("ipv6::addr: bad address")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("[{bad-ip}]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Invalid address in \"{bad-ip}\" error -2 -- Name or service not known (errno: 2 -- No such file or directory).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: missing ']'")
        {
            addr::addr_parser p;
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "IPv6 is missing the ']' ([1:2:3:4:5:6:7).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: required address")
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
        CATCH_START_SECTION("ipv6::addr: required port")
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

        CATCH_START_SECTION("ipv6::addr: port not allowed")
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
                CATCH_REQUIRE(p.error_messages() == "Port not allowed (1:2:3:4:5:6:7:8:123.5).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with invalid masks")
    {
        CATCH_START_SECTION("ipv6::addr: really large numbers (over 1000)")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 1001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Mask number too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }

            // in case the number is between square brackets it looks like
            // an IPv4 to getaddrinfo() so we get a different error...
            // (i.e. the '[' is not a digit so we do not get the "too large"
            // error...)
            //
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 1001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[" + std::to_string(mask) + "]"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Incompatible address between the address and mask address (first was an IPv6 second an IPv4).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }

            // an empty address with a mask too large gets us to another place
            //
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                int const mask((rand() & 0xFF) + 1001);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(port) + "/" + std::to_string(mask)));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Mask number too large (" + std::to_string(mask) + ", expected a maximum of 128).\n");
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
                addr::addr_range::vector_t ips(p.parse(":" + std::to_string(port) + "/" + std::to_string(mask) + "q"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid mask in \"/" + std::to_string(mask) + "q\", error -2 -- Name or service not known (errno: 0 -- Success).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: ipv6 mask is limited between 0 and 128")
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

        CATCH_START_SECTION("ipv6::addr: ipv6 mask cannot use name")
        {
            for(int idx(0); idx < 5; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
                addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[localhost]"));
                CATCH_REQUIRE(p.has_errors());
                CATCH_REQUIRE(p.error_count() == 1);
                CATCH_REQUIRE(p.error_messages() == "Invalid mask in \"/[localhost]\", error -2 -- Name or service not known (errno: 0 -- Success).\n");
                CATCH_REQUIRE(ips.size() == 0);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: ipv6 mask must be between '[...]'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/::3"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The address uses the IPv6 syntax, the mask cannot use IPv4.\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: ipv6 mask missing the ']'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[::3"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "The IPv6 mask is missing the ']' ([::3).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: ipv6 mask with an ipv4 in the '[...]'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[1:2:3:4:5:6:7:8]:" + std::to_string(port) + "/[1.2.3.4]"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Incompatible address between the address and mask address (first was an IPv6 second an IPv4).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: verify default address")
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

        CATCH_START_SECTION("ipv6::addr: verify default mask")
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

            p.set_default_mask("");
            CATCH_REQUIRE(p.get_default_mask4() == "");
            CATCH_REQUIRE(p.get_default_mask6() == "");
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::address", "[ipv6]")
{
    CATCH_GIVEN("addr() with an IPv6")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv6::addr: set_ipv6() / get_ipv6()")
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

                // verify network type
                //
                a.set_ipv6(in6);

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
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: set_ipv6() check to_ipv6_string()")
        {
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
                CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY)          == ip);
                CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS)      == "[" + ip + "]");
                CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_PORT)          == "[" + ip + "]:" + port_str);
                CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_MASK)          == ip + "/128"); // will change to 128 at some point
                CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS_MASK) == "[" + ip + "]/128");
                CATCH_REQUIRE(a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL)           == "[" + ip + "]:" + port_str + "/128");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: name of various IPs")
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
        CATCH_START_SECTION("ipv6::addr: verify basics")
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
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1:2:3:4:5:6:7:8");
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS) == "[1:2:3:4:5:6:7:8]");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "1:2:3:4:5:6:7:8");
            CATCH_REQUIRE(f.get_port() == 0);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            uint8_t mask[16] = {};
            f.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: default address")
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
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5:5:5:5:5:5:5:5");
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS) == "[5:5:5:5:5:5:5:5]");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5:5:5:5:5:5:5:5");
            CATCH_REQUIRE(f.get_port() == 0);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: address, not port allowed")
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "9:9:9:9:4:3:2:1");
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS) == "[9:9:9:9:4:3:2:1]");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "9:9:9:9:4:3:2:1");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5:1:6:2:7:3:8:4");
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS) == "[5:1:6:2:7:3:8:4]");
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "5:1:6:2:7:3:8:4");
                CATCH_REQUIRE(f.get_port() == 0);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            }
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with numeric only IPv6 addresses")
    {
        CATCH_START_SECTION("ipv6::addr: simple numeric IPv6")
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
            //CATCH_REQUIRE(f.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "");
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "4::f003:3001:20af");
            CATCH_REQUIRE(f.get_port() == 5093);
            CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
            CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PUBLIC);
            uint8_t mask[16] = {};
            f.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: invalid IPv6 domain name address when we only accept numeric IPs")
        {
            // this is exactly the same path as the IPv4 test...
            // if we have a named domain then IPv4 fails, IPv6 fails, then we err on it
            //
            addr::addr_parser p;
            p.set_protocol(IPPROTO_TCP);
            p.set_allow(addr::allow_t::ALLOW_ADDRESS_LOOKUP, false);
            addr::addr_range::vector_t ips(p.parse("ipv6.example.com:4471"));
            CATCH_REQUIRE(p.has_errors());
            CATCH_REQUIRE(p.error_count() == 1);
            CATCH_REQUIRE(p.error_messages() == "Unknown address in \"ipv6.example.com\" (no DNS lookup was allowed).\n");
            CATCH_REQUIRE(ips.size() == 0);
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::ports", "[ipv6]")
{
    // by default addr() is an IPv6 address so we test the basic port
    // functions here, although it could be in a common place instead...
    //
    CATCH_GIVEN("addr()")
    {
        addr::addr a;

        CATCH_START_SECTION("ipv6::addr: default port")
        {
            CATCH_REQUIRE(a.get_port() == 0);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: set_port()")
        {
            // setup a random port to start with
            //
            int const start_port(rand() & 0xFFFF);
            a.set_port(start_port);

            // test 100 invalid ports
            //
            for(int idx(0); idx < 100; ++idx)
            {
                // first try a negative port
                int port_too_small;
                do
                {
                    port_too_small = -(rand() & 0xFFFF);
                }
                while(port_too_small == 0);
                CATCH_REQUIRE_THROWS_AS(a.set_port(port_too_small), addr::addr_invalid_argument);

                // first try a negative port
                int const port_too_large = (rand() & 0xFFFF) + 65536;
                CATCH_REQUIRE_THROWS_AS(a.set_port(port_too_large), addr::addr_invalid_argument);

                // make sure port does not get modified on errors
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

        CATCH_START_SECTION("ipv6::addr: known ports to test get_service()")
        {
            a.set_port(80);
            CATCH_REQUIRE(a.get_service() == "http");

            a.set_port(443);
            CATCH_REQUIRE(a.get_service() == "https");

            // again with UDP
            // 
            a.set_protocol(IPPROTO_UDP);

            a.set_port(80);
            std::string service(a.get_service());
            CATCH_REQUIRE((service == "http" || service == "80"));

            a.set_port(443);
            service = a.get_service();
            CATCH_REQUIRE((service == "https"|| service == "443"));
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("addr_parser() with IPv6 addresses and port")
    {
        CATCH_START_SECTION("ipv6::addr: verify port")
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "ff01:2f3:f041:e301:f:10:11:12");
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS) == "[ff01:2f3:f041:e301:f:10:11:12]");
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "[ff01:2f3:f041:e301:f:10:11:12]:" + std::to_string(port));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "[ff01:2f3:f041:e301:f:10:11:12]:" + std::to_string(port));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: default address with various port")
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "[ff02:23:f41:e31:20:30:40:50]:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT) == "[ff02:23:f41:e31:20:30:40:50]:" + std::to_string(static_cast<int>(port)));
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == IPPROTO_TCP);
                CATCH_REQUIRE(f.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
            }
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

        CATCH_START_SECTION("ipv6::addr: default mask")
        {
            uint8_t mask[16] = {};
            a.get_mask(mask);
            for(int idx(0); idx < 16; ++idx)
            {
                CATCH_REQUIRE(mask[idx] == 255);
            }
            CATCH_REQUIRE(a.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: set_mask()")
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

        CATCH_START_SECTION("ipv6::addr: set_mask()")
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
        CATCH_START_SECTION("ipv6::addr: mask allowed, but no mask")
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
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: empty mask")
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
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: empty mask including the '[]'")
        {
            int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
            int const port(rand() & 0xFFFF);
            addr::addr_parser p;
            p.set_protocol(proto);
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t ips(p.parse("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/[]"));
            CATCH_REQUIRE_FALSE(p.has_errors());
            CATCH_REQUIRE(ips.size() == 1);
            addr::addr_range const & r(ips[0]);
            addr::addr f(r.get_from());
            CATCH_REQUIRE_FALSE(f.is_ipv4());
            CATCH_REQUIRE_FALSE(f.get_family() == AF_INET);
            CATCH_REQUIRE(f.get_family() == AF_INET6);
            std::string result("[55:33:22:11:0:cc:bb:aa]:" + std::to_string(port) + "/128");
            CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
            CATCH_REQUIRE(f.get_port() == port);
            CATCH_REQUIRE(f.get_protocol() == proto);
            CATCH_REQUIRE(f.get_mask_size() == 128);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: one number masks")
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_mask_size() == idx);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: address like mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: address like default mask")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
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

        CATCH_START_SECTION("ipv6::addr: address like mask with a default")
        {
            for(int idx(0); idx < 25; ++idx)
            {
                int const proto(rand() & 1 ? IPPROTO_TCP : IPPROTO_UDP);
                int const port(rand() & 0xFFFF);
                addr::addr_parser p;
                p.set_protocol(proto);
                p.set_allow(addr::allow_t::ALLOW_MASK, true);

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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
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

        CATCH_START_SECTION("ipv6::addr: no address, but one IPv6 number masks")
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.get_port() == port);
                CATCH_REQUIRE(f.get_protocol() == proto);
                CATCH_REQUIRE(f.get_mask_size() == idx);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: no address, but one IPv6 masks")
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
                CATCH_REQUIRE(f.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
                CATCH_REQUIRE(f.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL) == result);
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

        CATCH_START_SECTION("ipv6::addr: any (::)")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_ANY);
                CATCH_REQUIRE(a.get_network_type_string() == "Any");
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

                CATCH_REQUIRE(a.get_network_type() != addr::addr::network_type_t::NETWORK_TYPE_ANY);
                CATCH_REQUIRE(a.get_network_type_string() != "Any");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: private address fd00::/8")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_PRIVATE);
                CATCH_REQUIRE(a.get_network_type_string() == "Private");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: private address fe80::/10")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE(a.get_network_type_string() == "Local Link");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: private address ff02::/16")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LINK_LOCAL);
                CATCH_REQUIRE(a.get_network_type_string() == "Local Link");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: private address ff00::/8")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_MULTICAST);
                CATCH_REQUIRE(a.get_network_type_string() == "Multicast");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: private address ffx1::/8")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(a.get_network_type_string() == "Loopback");
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("ipv6::addr: private address ::1")
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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(a.get_network_type_string() == "Loopback");

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
                CATCH_REQUIRE(a.get_network_type() == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK);
                CATCH_REQUIRE(a.get_network_type_string() == "Loopback");
                freeaddrinfo(addrlist);
            }
        }
        CATCH_END_SECTION()
    }
}


CATCH_TEST_CASE("ipv6::network", "[ipv6]")
{
    CATCH_GIVEN("set_from_socket()")
    {
        CATCH_START_SECTION("ipv6::addr: create a server, but do not test it (yet)...")
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

        CATCH_START_SECTION("ipv6::addr: connect with TCP to [::1]")
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
                CATCH_REQUIRE(b.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "::1");
                CATCH_REQUIRE(b.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "::1");

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
                CATCH_REQUIRE(c.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "::1");
                CATCH_REQUIRE(c.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "::1");

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

        CATCH_START_SECTION("ipv6::addr: connect with UDP to [::1]")
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
            CATCH_REQUIRE_FALSE(c.is_ipv4());
            CATCH_REQUIRE_FALSE(c.get_family() == AF_INET);
            CATCH_REQUIRE(c.get_family() == AF_INET6);
            CATCH_REQUIRE(c.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_ONLY)    == "::");
            CATCH_REQUIRE(c.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "::");

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


// vim: ts=4 sw=4 et
