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
 * \brief Check the addr_range class basics.
 *
 * This set of unit tests do basic verification of the range class.
 *
 * The point here is to test the range functionality and not the
 * addr_parser class.
 */


// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/int128_literal.h>
#include    <snapdev/ostream_int128.h>


// last include
//
#include    <snapdev/poison.h>


using namespace snapdev::literals;



CATCH_TEST_CASE("ipv4::range", "[ipv4]")
{
    CATCH_GIVEN("addr_range()")
    {
        addr::addr_range range;

        CATCH_START_SECTION("addr_range: verify defaults")
        {
            addr::addr a;

            CATCH_REQUIRE_FALSE(range.has_from());
            CATCH_REQUIRE_FALSE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
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
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: test normal range (from <= to)")
        {
            addr::addr a;
            addr::addr other;

            // from 10.0.0.1
            struct sockaddr_in fin = sockaddr_in();
            fin.sin_family = AF_INET;
            int const fport(rand() & 0xFFFF);
            fin.sin_port = htons(fport);
            uint32_t const faddress((10 << 24)
                          | (0 << 16)
                          | (0 << 8)
                          | 1);
            fin.sin_addr.s_addr = htonl(faddress);
            addr::addr f;
            f.set_ipv4(fin);

            // to 10.0.0.254
            struct sockaddr_in tin = sockaddr_in();
            tin.sin_family = AF_INET;
            int const tport(rand() & 0xFFFF);
            tin.sin_port = htons(tport);
            uint32_t const taddress((10 << 24)
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
            CATCH_REQUIRE_FALSE(range.is_ipv4());
            CATCH_REQUIRE(range.get_from() == a);
            CATCH_REQUIRE(range.get_to() == a);
            CATCH_REQUIRE(range.size() == 0);
            auto const & r1(range);
            CATCH_REQUIRE(r1.get_from() == a);
            CATCH_REQUIRE(r1.get_to() == a);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r1.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE(r1.size() == 0);
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
            CATCH_REQUIRE(range.is_ipv4());
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == a);
            CATCH_REQUIRE(range.size() == 1);
            auto const & r2(range);
            CATCH_REQUIRE(r2.get_from() == f);
            CATCH_REQUIRE(r2.get_to() == a);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r2.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE(r2.size() == 1);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            {
                std::stringstream ss;
                ss << range;
                CATCH_REQUIRE(ss.str() == "10.0.0.1:" + std::to_string(fport) + "/32");

                addr::addr::vector_t from_vec(range.to_addresses(1000));
                CATCH_REQUIRE(from_vec.size() == 1);
                CATCH_REQUIRE(from_vec[0] == f);
            }

            range.swap_from_to();

            // swaped "from" & "to", test the results
            //
            CATCH_REQUIRE_FALSE(range.has_from());
            CATCH_REQUIRE(range.has_to());
            CATCH_REQUIRE_FALSE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.is_ipv4());
            CATCH_REQUIRE(range.get_from() == a);
            CATCH_REQUIRE(range.get_to() == f);
            CATCH_REQUIRE(range.size() == 1);
            auto const & r2b(range);
            CATCH_REQUIRE(r2b.get_from() == a);
            CATCH_REQUIRE(r2b.get_to() == f);
            CATCH_REQUIRE_THROWS_AS(range.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE_THROWS_AS(r2b.is_in(other), addr::addr_invalid_state);
            CATCH_REQUIRE(r2b.size() == 1);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE(range.match(f));
            CATCH_REQUIRE_FALSE(range.match(t));

            {
                std::stringstream ss1;
                ss1 << range;
                CATCH_REQUIRE(ss1.str() == "-10.0.0.1:" + std::to_string(fport) + "/32");

                std::stringstream ss2;
                ss2 << addr::setaddrmode(addr::string_ip_t::STRING_IP_ONLY) << range;
                CATCH_REQUIRE(ss2.str() == "-10.0.0.1");

                addr::addr::vector_t from_vec(range.to_addresses(1000));
                CATCH_REQUIRE(from_vec.size() == 1);
                CATCH_REQUIRE(from_vec[0] == f);
            }

            range.swap_from_to(); // restore before we go on
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == a);

            range.set_to(t);

            // defined "to", test the results
            //
            CATCH_REQUIRE(range.has_from());
            CATCH_REQUIRE(range.has_to());
            CATCH_REQUIRE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.is_ipv4());
            CATCH_REQUIRE(range.get_from() == f);
            CATCH_REQUIRE(range.get_to() == t);
            CATCH_REQUIRE(range.size() == 254); // all but .0 and .255, that's 254 addresses
            auto const & r3(range);
            CATCH_REQUIRE(r3.get_from() == f);
            CATCH_REQUIRE(r3.get_to() == t);
            CATCH_REQUIRE(r3.size() == 254);
            CATCH_REQUIRE_FALSE(range.match(a));
            CATCH_REQUIRE(range.match(f));
            CATCH_REQUIRE(range.match(t));

            {
                std::stringstream ss;
                ss << range;
                CATCH_REQUIRE(ss.str() == "10.0.0.1-10.0.0.254:" + std::to_string(tport) + "/32");
            }

            addr::addr_range to_range;  // to-only range
            to_range.set_to(t);

            // defined "to", but not from
            //
            CATCH_REQUIRE_FALSE(to_range.has_from());
            CATCH_REQUIRE(to_range.has_to());
            CATCH_REQUIRE_FALSE(to_range.is_range());
            CATCH_REQUIRE_FALSE(to_range.is_empty());
            CATCH_REQUIRE(to_range.is_ipv4());
            CATCH_REQUIRE(to_range.get_from() == a);
            CATCH_REQUIRE(to_range.get_to() == t);
            CATCH_REQUIRE(to_range.size() == 1);
            auto const & r4(to_range);
            CATCH_REQUIRE(r4.get_from() == a);
            CATCH_REQUIRE(r4.get_to() == t);
            CATCH_REQUIRE(r4.size() == 1);
            CATCH_REQUIRE_FALSE(to_range.match(a));
            CATCH_REQUIRE_FALSE(to_range.match(f));
            CATCH_REQUIRE(to_range.match(t));

            {
                std::stringstream ss;
                ss << to_range;
                CATCH_REQUIRE(ss.str() == "-10.0.0.254:" + std::to_string(tport) + "/32");

                addr::addr::vector_t to_vec(to_range.to_addresses(1000));
                CATCH_REQUIRE(to_vec.size() == 1);
                CATCH_REQUIRE(to_vec[0] == t);
            }

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
                CATCH_REQUIRE(f.is_previous(b));
                CATCH_REQUIRE(b.is_next(f));

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
                CATCH_REQUIRE(after.is_previous(t));
                CATCH_REQUIRE(t.is_next(after));

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

            // range to vector of IPs addresses
            //
            addr::addr::vector_t vec(range.to_addresses(1000));
            for(int idx(1); idx <= 254; ++idx)
            {
                struct sockaddr_in ein = sockaddr_in();
                ein.sin_family = AF_INET;
                int const eport(rand() & 0xFFFF);
                ein.sin_port = htons(eport);
                uint32_t const eaddress((10 << 24)
                              | (0 << 16)
                              | (0 << 8)
                              | idx);
                ein.sin_addr.s_addr = htonl(eaddress);
                addr::addr e;
                e.set_ipv4(ein);
                CATCH_REQUIRE(vec.front() == e);
                vec.erase(vec.begin());
            }
            // same, but use a limit so we get only the first 10 IPs
            CATCH_REQUIRE_THROWS_MATCHES(
                      range.to_addresses(10)
                    , addr::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: too many addresses in this range: 254 > 10"));
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: test empty range (from > to)")
        {
            addr::addr a;
            addr::addr other;

            // from is set to 10.0.0.254
            struct sockaddr_in fin = sockaddr_in();
            fin.sin_family = AF_INET;
            int const fport(rand() & 0xFFFF);
            fin.sin_port = htons(fport);
            uint32_t const faddress((10 << 24)
                          | (0 << 16)
                          | (0 << 8)
                          | 254);
            fin.sin_addr.s_addr = htonl(faddress);
            addr::addr f;
            f.set_ipv4(fin);

            // from is set to 10.0.0.1
            struct sockaddr_in tin = sockaddr_in();
            tin.sin_family = AF_INET;
            int const tport(rand() & 0xFFFF);
            tin.sin_port = htons(tport);
            uint32_t const taddress((10 << 24)
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

            {
                std::stringstream ss;
                ss << range;
                CATCH_REQUIRE(ss.str() == "10.0.0.254:" + std::to_string(fport) + "/32");
            }

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

            {
                std::stringstream ss;
                ss << range;
                CATCH_REQUIRE(ss.str() == "<empty address range>");
            }

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
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: compare \"from\" ranges against each other")
        {
            addr::addr a;
            addr::addr_range other;

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            uint32_t address((10 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255));
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            range.set_from(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_UNORDERED);

            address = (20 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255);
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            other.set_from(a);

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_EQUAL);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_EQUAL);

            a = range.get_from();
            other.set_from(a + 1);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_PRECEDES);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_FOLLOWS);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: compare \"to\" ranges against each other")
        {
            addr::addr a;
            addr::addr_range other;

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            uint32_t address((10 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255));
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            range.set_to(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_UNORDERED);

            address = (20 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255);
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            other.set_to(a);

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_EQUAL);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_EQUAL);

            a = range.get_to();
            other.set_to(a + 1);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_PRECEDES);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_FOLLOWS);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: compare \"from\" against \"to\" ranges")
        {
            addr::addr a;
            addr::addr_range other;

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            uint32_t address((10 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255));
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            range.set_from(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_UNORDERED);

            address = (20 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255);
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            other.set_to(a);

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_EQUAL);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_EQUAL);

            a = range.get_from();
            other.set_to(a + 1);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_PRECEDES);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_FOLLOWS);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: compare full ranges (\"from\" and \"to\" defined)")
        {
            addr::addr a;
            addr::addr_range other;

            struct sockaddr_in in = sockaddr_in();
            in.sin_family = AF_INET;
            in.sin_port = htons(rand());
            uint32_t address((10 << 24)
                          | (5 << 16)
                          | (7 << 8)
                          | (32 << 0));
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            range.set_from(a);

            address = (10 << 24)
                          | (5 << 16)
                          | (7 << 8)
                          | (37 << 0);
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            range.set_to(a);

            CATCH_REQUIRE(range.compare(range) == addr::compare_t::COMPARE_EQUAL);
            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_UNORDERED);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_UNORDERED);

            address = (20 << 24)
                          | (5 << 16)
                          | (7 << 8)
                          | (32 << 0);
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            other.set_from(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_EQUAL);

            other.swap_from_to();

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_EQUAL);

            address = (20 << 24)
                          | (5 << 16)
                          | (7 << 8)
                          | (22 << 0);
            in.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in);
            other.set_from(a);      // it was swapped, so set from again

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_EQUAL);

            other.swap_from_to();

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_FIRST);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_LAST);
            CATCH_REQUIRE(other.compare(other) == addr::compare_t::COMPARE_UNORDERED);

            other.swap_from_to();

            a = range.get_to();
            other.set_from(a + 1);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_PRECEDES);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_FOLLOWS);

            other.set_from(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_OVERLAP_SMALL_VS_LARGE);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_OVERLAP_LARGE_VS_SMALL);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: compare mixed IPs")
        {
            addr::addr a;
            addr::addr_range other;

            struct sockaddr_in in4 = sockaddr_in();
            in4.sin_family = AF_INET;
            in4.sin_port = htons(rand());
            uint32_t address((10 << 24)
                          | (5 << 16)
                          | (7 << 8)
                          | (32 << 0));
            in4.sin_addr.s_addr = htonl(address);
            a.set_ipv4(in4);
            range.set_from(a);

            struct sockaddr_in6 in6 = sockaddr_in6();
            in6.sin6_family = AF_INET6;
            in6.sin6_port = htons(rand());
            in6.sin6_addr.s6_addr[0] = 0xF8;
            a.set_ipv6(in6);
            other.set_from(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_IPV4_VS_IPV6);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_IPV6_VS_IPV4);

            CATCH_REQUIRE(range.compare(other, true) == addr::compare_t::COMPARE_SMALLER);
            CATCH_REQUIRE(other.compare(range, true) == addr::compare_t::COMPARE_LARGER);

            in6.sin6_addr.s6_addr[0] = 0;
            in6.sin6_addr.s6_addr[15] = 1;
            a.set_ipv6(in6);
            other.set_from(a);

            CATCH_REQUIRE(range.compare(other) == addr::compare_t::COMPARE_IPV4_VS_IPV6);
            CATCH_REQUIRE(other.compare(range) == addr::compare_t::COMPARE_IPV6_VS_IPV4);

            CATCH_REQUIRE(range.compare(other, true) == addr::compare_t::COMPARE_LARGER);
            CATCH_REQUIRE(other.compare(range, true) == addr::compare_t::COMPARE_SMALLER);
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: create range from CIDR")
        {
            addr::addr a;

            // from is set to 10.x.x.x
            struct sockaddr_in fin = sockaddr_in();
            fin.sin_family = AF_INET;
            fin.sin_port = htons(rand());
            uint32_t faddress((10 << 24)
                          | ((rand() & 255) << 16)
                          | ((rand() & 255) << 8)
                          | (rand() & 255));
            fin.sin_addr.s_addr = htonl(faddress);
            a.set_ipv4(fin);

            // no mask defined, that means the range will be [a..a]
            range.from_cidr(a);

            CATCH_REQUIRE(range.has_from());
            CATCH_REQUIRE(range.has_to());
            CATCH_REQUIRE(range.is_range());
            CATCH_REQUIRE_FALSE(range.is_empty());
            CATCH_REQUIRE(range.is_ipv4());
            CATCH_REQUIRE(range.get_from() == a);
            CATCH_REQUIRE(range.get_to() == a);
            CATCH_REQUIRE(range.size() == 1);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
            for(int size(96); size <= 128; ++size)
            {
                a.set_mask_count(size);
                range.from_cidr(a);

                CATCH_REQUIRE(range.has_from());
                CATCH_REQUIRE(range.has_to());
                CATCH_REQUIRE(range.is_range());
                CATCH_REQUIRE_FALSE(range.is_empty());
                CATCH_REQUIRE(range.is_ipv4());
                unsigned __int128 mask(0xffffffffffffffffffffffffffffffff_uint128);
                mask <<= 128 - size;
                addr::addr const & f(range.get_from());
                addr::addr const & t(range.get_to());
                unsigned __int128 aa(a.ip_to_uint128());
                unsigned __int128 fa(f.ip_to_uint128());
                unsigned __int128 ta(t.ip_to_uint128());
                unsigned __int128 fm(aa & mask);
                unsigned __int128 tm(aa | ~mask);
                CATCH_REQUIRE(fa == fm);
                CATCH_REQUIRE(ta == tm);
                CATCH_REQUIRE(range.size() == 1ULL << (128 - size));
            }
#pragma GCC diagnostic pop

            // create an invalid mask as far as from_cidr() is concerned
            //
            uint8_t invalid_mask[16] = {};
            for(int i(0); i < 16; ++i)
            {
                do
                {
                    invalid_mask[i] = rand();
                }
                while(invalid_mask[i] == 0 || invalid_mask[i] == 0xFF);
            }
            a.set_mask(invalid_mask);
            CATCH_REQUIRE_THROWS_MATCHES(
                      range.from_cidr(a)
                    , addr::addr_unsupported_as_range
                    , Catch::Matchers::ExceptionMessage(
                              "addr_error: unsupported mask for a range"));
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("compute intersection of two ranges")
    {
        CATCH_START_SECTION("addr_range: intersection of two ranges that overlap")
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

            // to is set to 10.5.255.255
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

            // to is set to 10.10.255.255
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

            addr::addr_range const range3(range1.intersection(range2));

            CATCH_REQUIRE(range3.is_range());
            CATCH_REQUIRE_FALSE(range3.is_empty());

            CATCH_REQUIRE(range3.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.2.0.0");
            CATCH_REQUIRE(range3.get_to().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.5.255.255");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: intersection of two ranges that do not overlap")
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

            // to is set to 10.5.255.255
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

            // to is set to 10.20.255.255
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

            addr::addr_range const range3(range1.intersection(range2));

            CATCH_REQUIRE(range3.is_range());
            CATCH_REQUIRE(range3.is_empty());

            // although it is "empty" we know the IPs and can test them
            //
            CATCH_REQUIRE(range3.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.10.0.0");
            CATCH_REQUIRE(range3.get_to().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.5.255.255");

            addr::addr_range::vector_t rlist;
            rlist.push_back(range1);
            rlist.push_back(range2);
            CATCH_REQUIRE(addr::address_match_ranges(rlist, f1));
            CATCH_REQUIRE(addr::address_match_ranges(rlist, f2));
            CATCH_REQUIRE(addr::address_match_ranges(rlist, t1));
            CATCH_REQUIRE(addr::address_match_ranges(rlist, t2));
        }
        CATCH_END_SECTION()
    }

    CATCH_GIVEN("check for union of two ranges")
    {
        CATCH_START_SECTION("addr_range: union of two ranges that overlap")
        {
            // from 10.1.0.0
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

            // to 10.5.255.255
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
            addr::addr_range range2;

            {
                addr::addr_range const r(range1.union_if_possible(range2));
                CATCH_REQUIRE_FALSE(r.is_defined());
            }
            {
                addr::addr_range const r(range2.union_if_possible(range1));
                CATCH_REQUIRE_FALSE(r.is_defined());
            }

            range1.set_from(f1);

            {
                addr::addr_range const r(range1.union_if_possible(range2));
                CATCH_REQUIRE_FALSE(r.is_defined());
            }
            {
                addr::addr_range const r(range2.union_if_possible(range1));
                CATCH_REQUIRE_FALSE(r.is_defined());
            }
            {
                addr::addr_range const r(range1.union_if_possible(range1));
                CATCH_REQUIRE(r.is_defined());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
                CATCH_REQUIRE(r.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.1.0.0");
            }

            range2.set_from(f2);

            {
                addr::addr_range const r(range1.union_if_possible(range2));
                CATCH_REQUIRE_FALSE(r.is_defined());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                CATCH_REQUIRE_FALSE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
            }
            {
                addr::addr_range const r(range2.union_if_possible(range1));
                CATCH_REQUIRE_FALSE(r.is_defined());
                CATCH_REQUIRE_FALSE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                CATCH_REQUIRE_FALSE(r.has_from());
                CATCH_REQUIRE_FALSE(r.has_to());
            }

            range1.set_to(t1);

            {
                addr::addr_range const r(range1.union_if_possible(range2));
                CATCH_REQUIRE(r.is_defined());
                CATCH_REQUIRE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE(r.has_to());
                CATCH_REQUIRE(r.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.1.0.0");
                CATCH_REQUIRE(r.get_to().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.5.255.255");
            }
            {
                addr::addr_range const r(range2.union_if_possible(range1));
                CATCH_REQUIRE(r.is_defined());
                CATCH_REQUIRE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE(r.has_to());
                CATCH_REQUIRE(r.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.1.0.0");
                CATCH_REQUIRE(r.get_to().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.5.255.255");
            }

            range2.set_to(t2);

            CATCH_REQUIRE(range1.compare(range2) == addr::compare_t::COMPARE_OVERLAP_SMALL_VS_LARGE);
            CATCH_REQUIRE(range2.compare(range1) == addr::compare_t::COMPARE_OVERLAP_LARGE_VS_SMALL);

            CATCH_REQUIRE(range1.is_range());
            CATCH_REQUIRE_FALSE(range1.is_empty());
            CATCH_REQUIRE(range2.is_range());
            CATCH_REQUIRE_FALSE(range2.is_empty());

            {
                addr::addr_range const r(range1.union_if_possible(range2));
                CATCH_REQUIRE(r.is_defined());
                CATCH_REQUIRE(r.is_range());
                CATCH_REQUIRE_FALSE(r.is_empty());
                CATCH_REQUIRE(r.has_from());
                CATCH_REQUIRE(r.has_to());
                CATCH_REQUIRE(r.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.1.0.0");
                CATCH_REQUIRE(r.get_to().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.10.255.255");

                CATCH_REQUIRE(range1.compare(r) == addr::compare_t::COMPARE_INCLUDES);
                CATCH_REQUIRE(r.compare(range1) == addr::compare_t::COMPARE_INCLUDED);
                CATCH_REQUIRE(range2.compare(r) == addr::compare_t::COMPARE_INCLUDES);
                CATCH_REQUIRE(r.compare(range2) == addr::compare_t::COMPARE_INCLUDED);
            }
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: union of two ranges that touch")
        {
            // from 10.1.0.0
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

            // to 10.5.32.21
            struct sockaddr_in t1in = sockaddr_in();
            t1in.sin_family = AF_INET;
            t1in.sin_port = htons(rand());
            uint32_t t1address((10 << 24)
                          | (5 << 16)
                          | (32 << 8)
                          | 21);
            t1in.sin_addr.s_addr = htonl(t1address);
            addr::addr t1;
            t1.set_ipv4(t1in);

            // from is set to 10.5.32.22
            struct sockaddr_in f2in = sockaddr_in();
            f2in.sin_family = AF_INET;
            f2in.sin_port = htons(rand());
            uint32_t f2address((10 << 24)
                          | (5 << 16)
                          | (32 << 8)
                          | 22);
            f2in.sin_addr.s_addr = htonl(f2address);
            addr::addr f2;
            f2.set_ipv4(f2in);

            // from is set to 10.11.255.255
            struct sockaddr_in t2in = sockaddr_in();
            t2in.sin_family = AF_INET;
            t2in.sin_port = htons(rand());
            uint32_t t2address((10 << 24)
                          | (11 << 16)
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

            // the intersection is going to be empty (nothing overlaps)
            //
            addr::addr_range const range3(range1.intersection(range2));

            CATCH_REQUIRE(range3.is_range());
            CATCH_REQUIRE(range3.is_empty());

            // the union works as expected
            //
            addr::addr_range const range4(range1.union_if_possible(range2));

            CATCH_REQUIRE(range4.is_range());
            CATCH_REQUIRE_FALSE(range4.is_empty());

            CATCH_REQUIRE(range4.get_from().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.1.0.0");
            CATCH_REQUIRE(range4.get_to().to_ipv4_string(addr::string_ip_t::STRING_IP_ONLY) == "10.11.255.255");
        }
        CATCH_END_SECTION()

        CATCH_START_SECTION("addr_range: union two ranges that do not overlap")
        {
            // from 10.1.0.0
            struct sockaddr_in f1in = sockaddr_in();
            f1in.sin_family = AF_INET;
            int const f1port(rand() & 0xFFFF);
            f1in.sin_port = htons(f1port);
            uint32_t const f1address((10 << 24)
                          | (1 << 16)
                          | (0 << 8)
                          | 0);
            f1in.sin_addr.s_addr = htonl(f1address);
            addr::addr f1;
            f1.set_ipv4(f1in);

            // to 10.1.0.255
            struct sockaddr_in t1in = sockaddr_in();
            t1in.sin_family = AF_INET;
            int const t1port(rand() & 0xFFFF);
            t1in.sin_port = htons(t1port);
            uint32_t const t1address((10 << 24)
                          | (1 << 16)
                          | (0 << 8)
                          | 255);
            t1in.sin_addr.s_addr = htonl(t1address);
            addr::addr t1;
            t1.set_ipv4(t1in);

            // from 10.10.0.0
            struct sockaddr_in f2in = sockaddr_in();
            f2in.sin_family = AF_INET;
            int const f2port(rand() & 0xFFFF);
            f2in.sin_port = htons(f2port);
            uint32_t const f2address((10 << 24)
                          | (10 << 16)
                          | (0 << 8)
                          | 0);
            f2in.sin_addr.s_addr = htonl(f2address);
            addr::addr f2;
            f2.set_ipv4(f2in);

            // to 10.10.1.255
            struct sockaddr_in t2in = sockaddr_in();
            t2in.sin_family = AF_INET;
            int const t2port(rand() & 0xFFFF);
            t2in.sin_port = htons(t2port);
            uint32_t const t2address((10 << 24)
                          | (10 << 16)
                          | (1 << 8)
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

            addr::addr_range const range3(range1.union_if_possible(range2));

            CATCH_REQUIRE_FALSE(range3.is_range());
            CATCH_REQUIRE_FALSE(range3.is_empty());

            std::string vec_result;
            std::string vec_result_ip_only;
            {
                std::stringstream ss;
                ss << range1;
                std::string const range1_result("10.1.0.0-10.1.0.255:" + std::to_string(t1port) + "/32");
                CATCH_REQUIRE(ss.str() == range1_result);
                vec_result += range1_result;
                vec_result_ip_only += "10.1.0.0-10.1.0.255";
            }
            {
                std::stringstream ss;
                ss << range2;
                std::string const range2_result("10.10.0.0-10.10.1.255:" + std::to_string(t2port) + "/32");
                CATCH_REQUIRE(ss.str() == range2_result);
                vec_result += ',';
                vec_result += range2_result;
                vec_result_ip_only += ',';
                vec_result_ip_only += "10.10.0.0-10.10.1.255";
            }
            {
                std::stringstream ss;
                ss << addr::setaddrmode(addr::string_ip_t::STRING_IP_MASK) << range3;
                std::string const range3_result("<empty address range>");
                CATCH_REQUIRE(ss.str() == range3_result);
                vec_result += ',';
                vec_result += range3_result;
                vec_result_ip_only += ',';
                vec_result_ip_only += "<empty address range>";
            }
            {
                addr::addr_range::vector_t vec{ range1, range2, range3 };
                std::stringstream ss;
                ss << vec;
                CATCH_REQUIRE(ss.str() == vec_result);

                std::stringstream sm;
                sm << addr::setaddrmode(addr::string_ip_t::STRING_IP_ONLY) << vec;
                CATCH_REQUIRE(sm.str() == vec_result_ip_only);

                addr::addr::vector_t all_addresses(addr::addr_range::to_addresses(vec, 1000));

                // the first 256 are 10.1.0.0 to 10.1.0.255
                //
                for(int idx(0); idx < 256; ++idx)
                {
                    struct sockaddr_in v1in = sockaddr_in();
                    v1in.sin_family = AF_INET;
                    int const v1port(rand() & 0xFFFF);
                    v1in.sin_port = htons(v1port);
                    uint32_t const v1address((10 << 24)
                                  | (1 << 16)
                                  | (0 << 8)
                                  | idx);
                    v1in.sin_addr.s_addr = htonl(v1address);
                    addr::addr v1;
                    v1.set_ipv4(v1in);

                    CATCH_REQUIRE(all_addresses[idx] == v1);
                }

                // the following 512 are 10.10.0.0 to 10.10.1.255
                //
                for(int idx(0); idx < 512; ++idx)
                {
                    struct sockaddr_in v2in = sockaddr_in();
                    v2in.sin_family = AF_INET;
                    int const v2port(rand() & 0xFFFF);
                    v2in.sin_port = htons(v2port);
                    uint32_t const v2address((10 << 24)
                                  | (10 << 16)
                                  | (0 << 8)
                                  | idx);       // this one leaks in the next as expected
                    v2in.sin_addr.s_addr = htonl(v2address);
                    addr::addr v2;
                    v2.set_ipv4(v2in);

                    CATCH_REQUIRE(all_addresses[idx + 256] == v2);
                }

                CATCH_REQUIRE_THROWS_MATCHES(
                          addr::addr_range::to_addresses(vec, 256 + 512 - 1)
                        , addr::out_of_range
                        , Catch::Matchers::ExceptionMessage(
                                  "out_of_range: too many addresses in this range: 768 > 767"));
            }
        }
        CATCH_END_SECTION()
    }
}



// vim: ts=4 sw=4 et
