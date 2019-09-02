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
 * \brief Check the addr_range class basics.
 *
 * This set of unit tests do basic verification of the range class.
 *
 * The point here is to test the range functionality and not the
 * addr_parser class.
 */

#include "test_addr_main.h"




CATCH_TEST_CASE( "ipv4::range", "[ipv4]" )
{
    CATCH_GIVEN("addr_range()")
    {
        addr::addr_range range;

        CATCH_SECTION("verify defaults")
        {
            addr::addr a;

            CATCH_REQUIRE_FALSE(range.has_from());
            CATCH_REQUIRE_FALSE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
//std::cerr << "++++++++++++++++++ " << a.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_PORT) << "\n";
//std::cerr << "++++++++++++++++++ " << range.get_from().to_ipv6_string(addr::addr::string_ip_t::STRING_IP_PORT) << "\n";
//
//struct sockaddr_in6 in6_a;
//a.get_ipv6(in6_a);
//std::cerr << "++++++++++++++++++ in6 --" << std::hex;
//for(size_t idx(0); idx < sizeof(in6_a); ++idx)
//    std::cerr << " " << static_cast<int>(reinterpret_cast<char *>(&in6_a)[idx]);
//std::cerr << "\n";
//
//struct sockaddr_in6 in6_b;
//range.get_from().get_ipv6(in6_b);
//std::cerr << "++++++++++++++++++ in6 --" << std::hex;
//for(size_t idx(0); idx < sizeof(in6_b); ++idx)
//    std::cerr << " " << static_cast<int>(reinterpret_cast<char *>(&in6_b)[idx]);
//std::cerr << "\n";
            CATCH_REQUIRE(range.get_from() == a);
            CATCH_REQUIRE(range.get_to() == a);

            // to use the const version of the get_from/to() functions
            // we have to define a const refence to range
            //
            auto const & r(range);
            CATCH_REQUIRE(r.get_from() == a);
            CATCH_REQUIRE(r.get_to() == a);

            addr::addr other;
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r.is_in(other), addr::addr_invalid_state);
        }

        CATCH_SECTION("test normal range (from <= to)")
        {
            addr::addr a;
            addr::addr other;

            // from is set to 10.0.0.1
            struct sockaddr_in fin = sockaddr_in();
            fin.sin_family = AF_INET;
            fin.sin_port = htons(rand());
            uint32_t faddress((10 << 24)
                          | (0 << 16)
                          | (0 << 8)
                          | 1);
            fin.sin_addr.s_addr = htonl(faddress);
            addr::addr f;
            f.set_ipv4(fin);

            // from is set to 10.0.0.254
            struct sockaddr_in tin = sockaddr_in();
            tin.sin_family = AF_INET;
            tin.sin_port = htons(rand());
            uint32_t taddress((10 << 24)
                          | (0 << 16)
                          | (0 << 8)
                          | 254);
            tin.sin_addr.s_addr = htonl(taddress);
            addr::addr t;
            t.set_ipv4(tin);

            // test defaults first
            //
            CATCH_REQUIRE_FALSE(range.has_from());
            CATCH_REQUIRE_FALSE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.get_from() == a);
            CATCH_REQUIRE(range.get_to() == a);
            auto const & r1(range);
            CATCH_REQUIRE(r1.get_from() == a);
            CATCH_REQUIRE(r1.get_to() == a);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r1.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE(range.match(a));
            CATCH_REQUIRE_FALSE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            range.set_from(f);

            // defined "from", test the results
            //
            CATCH_REQUIRE(range.has_from());
            CATCH_REQUIRE_FALSE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == a);
            auto const & r2(range);
            CATCH_REQUIRE(r2.get_from() == f);
            CATCH_REQUIRE(r2.get_to() == a);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r2.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            range.set_to(t);

            // defined "to", test the results
            //
            CATCH_REQUIRE(range.has_from());
            CATCH_REQUIRE(range.has_to());
            CATCH_REQUIRE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == t);
            auto const & r3(range);
            CATCH_REQUIRE(r3.get_from() == f);
            CATCH_REQUIRE(r3.get_to() == t);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE(range.match(f));
            CATCH_REQUIRE(range.match(t));

            // IP before range
            {
                struct sockaddr_in bin = sockaddr_in();
                bin.sin_family = AF_INET;
                bin.sin_port = htons(rand());
                uint32_t baddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | 0);
                bin.sin_addr.s_addr = htonl(baddress);
                addr::addr b;
                b.set_ipv4(bin);

                CATCH_REQUIRE_FALSE(range.is_in(b));
            }

            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in bin = sockaddr_in();
                bin.sin_family = AF_INET;
                bin.sin_port = htons(rand());
                uint32_t baddress(((rand() % 10) << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | (rand() & 255));
                bin.sin_addr.s_addr = htonl(baddress);
                addr::addr b;
                b.set_ipv4(bin);

                CATCH_REQUIRE_FALSE(range.is_in(b));
            }

            // IP after range
            {
                struct sockaddr_in ain = sockaddr_in();
                ain.sin_family = AF_INET;
                ain.sin_port = htons(rand());
                uint32_t aaddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | 255);
                ain.sin_addr.s_addr = htonl(aaddress);
                addr::addr after;
                after.set_ipv4(ain);

                CATCH_REQUIRE_FALSE(range.is_in(after));
            }

            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in ain = sockaddr_in();
                ain.sin_family = AF_INET;
                ain.sin_port = htons(rand());
                uint32_t aaddress(((rand() % (256 - 11) + 11) << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | (rand() & 255));
                ain.sin_addr.s_addr = htonl(aaddress);
                addr::addr after;
                after.set_ipv4(ain);

                CATCH_REQUIRE_FALSE(range.is_in(after));
            }

            // IP in range
            //
            for(int idx(1); idx < 255; ++idx)
            {
                struct sockaddr_in iin = sockaddr_in();
                iin.sin_family = AF_INET;
                iin.sin_port = htons(rand());
                uint32_t iaddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | idx);
                iin.sin_addr.s_addr = htonl(iaddress);
                addr::addr i;
                i.set_ipv4(iin);

                CATCH_REQUIRE(range.is_in(i));
            }
        }

        CATCH_SECTION("test empty range (from > to)")
        {
            addr::addr a;
            addr::addr other;

            // from is set to 10.0.0.254
            struct sockaddr_in fin = sockaddr_in();
            fin.sin_family = AF_INET;
            fin.sin_port = htons(rand());
            uint32_t faddress((10 << 24)
                          | (0 << 16)
                          | (0 << 8)
                          | 254);
            fin.sin_addr.s_addr = htonl(faddress);
            addr::addr f;
            f.set_ipv4(fin);

            // from is set to 10.0.0.1
            struct sockaddr_in tin = sockaddr_in();
            tin.sin_family = AF_INET;
            tin.sin_port = htons(rand());
            uint32_t taddress((10 << 24)
                          | (0 << 16)
                          | (0 << 8)
                          | 1);
            tin.sin_addr.s_addr = htonl(taddress);
            addr::addr t;
            t.set_ipv4(tin);

            // test defaults first
            //
            CATCH_REQUIRE_FALSE(range.has_from());
            CATCH_REQUIRE_FALSE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.get_from() == a);
            CATCH_REQUIRE(range.get_to() == a);
            auto const & r1(range);
            CATCH_REQUIRE(r1.get_from() == a);
            CATCH_REQUIRE(r1.get_to() == a);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r1.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE(range.match(a));
            CATCH_REQUIRE_FALSE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            range.set_from(f);

            // defined "from", test the results
            //
            CATCH_REQUIRE(range.has_from());
            CATCH_REQUIRE_FALSE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == a);
            auto const & r2(range);
            CATCH_REQUIRE(r2.get_from() == f);
            CATCH_REQUIRE(r2.get_to() == a);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r2.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            range.set_to(t);

            // defined "to", test the results
            //
            CATCH_REQUIRE(range.has_from());
            CATCH_REQUIRE(range.has_to());
            CATCH_REQUIRE(range.is_range());
            CATCH_REQUIRE(range.is_empty());
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == t);
            auto const & r3(range);
            CATCH_REQUIRE(r3.get_from() == f);
            CATCH_REQUIRE(r3.get_to() == t);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE_FALSE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            // IP before range
            {
                struct sockaddr_in bin = sockaddr_in();
                bin.sin_family = AF_INET;
                bin.sin_port = htons(rand());
                uint32_t baddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | 0);
                bin.sin_addr.s_addr = htonl(baddress);
                addr::addr b;
                b.set_ipv4(bin);

                CATCH_REQUIRE_FALSE(range.is_in(b));
            }

            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in bin = sockaddr_in();
                bin.sin_family = AF_INET;
                bin.sin_port = htons(rand());
                uint32_t baddress(((rand() % 10) << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | (rand() & 255));
                bin.sin_addr.s_addr = htonl(baddress);
                addr::addr b;
                b.set_ipv4(bin);

                CATCH_REQUIRE_FALSE(range.is_in(b));
            }

            // IP after range
            {
                struct sockaddr_in ain = sockaddr_in();
                ain.sin_family = AF_INET;
                ain.sin_port = htons(rand());
                uint32_t aaddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | 255);
                ain.sin_addr.s_addr = htonl(aaddress);
                addr::addr after;
                after.set_ipv4(ain);

                CATCH_REQUIRE_FALSE(range.is_in(after));
            }

            for(int idx(0); idx < 10; ++idx)
            {
                struct sockaddr_in ain = sockaddr_in();
                ain.sin_family = AF_INET;
                ain.sin_port = htons(rand());
                uint32_t aaddress(((rand() % (256 - 11) + 11) << 24)
                              | ((rand() & 255) << 16)
                              | ((rand() & 255) << 8)
                              | (rand() & 255));
                ain.sin_addr.s_addr = htonl(aaddress);
                addr::addr after;
                after.set_ipv4(ain);

                CATCH_REQUIRE_FALSE(range.is_in(after));
            }

            // IP in range
            //
            for(int idx(0); idx < 100; ++idx)
            {
                struct sockaddr_in iin = sockaddr_in();
                iin.sin_family = AF_INET;
                iin.sin_port = htons(rand());
                uint32_t iaddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | ((rand() & 253) + 1));
                iin.sin_addr.s_addr = htonl(iaddress);
                addr::addr i;
                i.set_ipv4(iin);

                CATCH_REQUIRE_FALSE(range.is_in(i));
            }
        }
    }

    CATCH_GIVEN("compute intersection of two ranges")
    {
        CATCH_SECTION("two ranges that overlap")
        {
            // from is set to 10.1.0.0
            struct sockaddr_in f1in = sockaddr_in();
            f1in.sin_family = AF_INET;
            f1in.sin_port = htons(rand());
            uint32_t f1address((10 << 24)
                          | (1 << 16)
                          | (0 << 8)
                          | 0);
            f1in.sin_addr.s_addr = htonl(f1address);
            addr::addr f1;
            f1.set_ipv4(f1in);

            // from is set to 10.5.255.255
            struct sockaddr_in t1in = sockaddr_in();
            t1in.sin_family = AF_INET;
            t1in.sin_port = htons(rand());
            uint32_t t1address((10 << 24)
                          | (5 << 16)
                          | (255 << 8)
                          | 255);
            t1in.sin_addr.s_addr = htonl(t1address);
            addr::addr t1;
            t1.set_ipv4(t1in);

            // from is set to 10.2.0.0
            struct sockaddr_in f2in = sockaddr_in();
            f2in.sin_family = AF_INET;
            f2in.sin_port = htons(rand());
            uint32_t f2address((10 << 24)
                          | (2 << 16)
                          | (0 << 8)
                          | 0);
            f2in.sin_addr.s_addr = htonl(f2address);
            addr::addr f2;
            f2.set_ipv4(f2in);

            // from is set to 10.10.255.255
            struct sockaddr_in t2in = sockaddr_in();
            t2in.sin_family = AF_INET;
            t2in.sin_port = htons(rand());
            uint32_t t2address((10 << 24)
                          | (10 << 16)
                          | (255 << 8)
                          | 255);
            t2in.sin_addr.s_addr = htonl(t2address);
            addr::addr t2;
            t2.set_ipv4(t2in);

            addr::addr_range range1;
            range1.set_from(f1);
            range1.set_to(t1);

            addr::addr_range range2;
            range2.set_from(f2);
            range2.set_to(t2);

            CATCH_REQUIRE(range1.is_range());
            CATCH_REQUIRE_FALSE(range1.is_empty());
            CATCH_REQUIRE(range2.is_range());
            CATCH_REQUIRE_FALSE(range2.is_empty());

            addr::addr_range range3(range1.intersection(range2));

            CATCH_REQUIRE(range3.is_range());
            CATCH_REQUIRE_FALSE(range3.is_empty());

            CATCH_REQUIRE(range3.get_from().to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.2.0.0");
            CATCH_REQUIRE(range3.get_to().to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.5.255.255");
        }

        CATCH_SECTION("two ranges that do not overlap")
        {
            // from is set to 10.1.0.0
            struct sockaddr_in f1in = sockaddr_in();
            f1in.sin_family = AF_INET;
            f1in.sin_port = htons(rand());
            uint32_t f1address((10 << 24)
                          | (1 << 16)
                          | (0 << 8)
                          | 0);
            f1in.sin_addr.s_addr = htonl(f1address);
            addr::addr f1;
            f1.set_ipv4(f1in);

            // from is set to 10.5.255.255
            struct sockaddr_in t1in = sockaddr_in();
            t1in.sin_family = AF_INET;
            t1in.sin_port = htons(rand());
            uint32_t t1address((10 << 24)
                          | (5 << 16)
                          | (255 << 8)
                          | 255);
            t1in.sin_addr.s_addr = htonl(t1address);
            addr::addr t1;
            t1.set_ipv4(t1in);

            // from is set to 10.10.0.0
            struct sockaddr_in f2in = sockaddr_in();
            f2in.sin_family = AF_INET;
            f2in.sin_port = htons(rand());
            uint32_t f2address((10 << 24)
                          | (10 << 16)
                          | (0 << 8)
                          | 0);
            f2in.sin_addr.s_addr = htonl(f2address);
            addr::addr f2;
            f2.set_ipv4(f2in);

            // from is set to 10.20.255.255
            struct sockaddr_in t2in = sockaddr_in();
            t2in.sin_family = AF_INET;
            t2in.sin_port = htons(rand());
            uint32_t t2address((10 << 24)
                          | (20 << 16)
                          | (255 << 8)
                          | 255);
            t2in.sin_addr.s_addr = htonl(t2address);
            addr::addr t2;
            t2.set_ipv4(t2in);

            addr::addr_range range1;
            range1.set_from(f1);
            range1.set_to(t1);

            addr::addr_range range2;
            range2.set_from(f2);
            range2.set_to(t2);

            CATCH_REQUIRE(range1.is_range());
            CATCH_REQUIRE_FALSE(range1.is_empty());
            CATCH_REQUIRE(range2.is_range());
            CATCH_REQUIRE_FALSE(range2.is_empty());

            addr::addr_range range3(range1.intersection(range2));

            CATCH_REQUIRE(range3.is_range());
            CATCH_REQUIRE(range3.is_empty());

            // although it is "empty" we know the IPs and can test them
            //
            CATCH_REQUIRE(range3.get_from().to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.10.0.0");
            CATCH_REQUIRE(range3.get_to().to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ONLY) == "10.5.255.255");

            addr::addr_range::vector_t rlist;
            rlist.push_back(range1);
            rlist.push_back(range2);
            CATCH_REQUIRE(addr::address_match_ranges(rlist, f1));
            CATCH_REQUIRE(addr::address_match_ranges(rlist, f2));
            CATCH_REQUIRE(addr::address_match_ranges(rlist, t1));
            CATCH_REQUIRE(addr::address_match_ranges(rlist, t2));
        }
    }
}





// vim: ts=4 sw=4 et
