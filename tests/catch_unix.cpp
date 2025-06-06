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
 * \brief Test the Unix address.
 *
 * These tests verify that the Unix address functions as expected.
 */

// addr
//
#include    <libaddr/addr_unix.h>


// self
//
#include    "catch_main.h"


// libutf8
//
#include    <libutf8/libutf8.h>
#include    <libutf8/exception.h>


// snapdev
//
#include    <snapdev/raii_generic_deleter.h>


// C
//
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>







CATCH_TEST_CASE("addr_unix::unnamed", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::unnamed: addr_unix() defaults (a.k.a. unnamed address)")
    {
        addr::addr_unix u;

        CATCH_REQUIRE_FALSE(u.is_file());
        CATCH_REQUIRE_FALSE(u.is_abstract());
        CATCH_REQUIRE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == std::string());
        CATCH_REQUIRE(u.to_uri() == "unix:");

        sockaddr_un un;
        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::unnamed: addr_unix() with an unnamed address")
    {
        sockaddr_un init = addr::init_un();
        CATCH_REQUIRE(init.sun_family == AF_UNIX);
        for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
        {
            CATCH_CHECK(init.sun_path[idx] == 0);
        }

        addr::addr_unix u(init);

        CATCH_REQUIRE_FALSE(u.is_file());
        CATCH_REQUIRE_FALSE(u.is_abstract());
        CATCH_REQUIRE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == std::string());
        CATCH_REQUIRE(u.to_uri() == "unix:");

        sockaddr_un un;
        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::unnamed: addr_unix() with an unnamed string")
    {
        std::string no_name;
        addr::addr_unix u(no_name);

        CATCH_REQUIRE_FALSE(u.is_file());
        CATCH_REQUIRE_FALSE(u.is_abstract());
        CATCH_REQUIRE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == std::string());
        CATCH_REQUIRE(u.to_uri() == "unix:");

        sockaddr_un un;
        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::unnamed: addr_unix() with a forced unnamed URI")
    {
        addr::addr_unix u;

        u.set_uri("unix:?unnamed");

        CATCH_REQUIRE_FALSE(u.is_file());
        CATCH_REQUIRE_FALSE(u.is_abstract());
        CATCH_REQUIRE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == std::string());
        CATCH_REQUIRE(u.to_uri() == "unix:");
        CATCH_REQUIRE(u.unlink() == 0);

        sockaddr_un un;
        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::unnamed: addr_unix() with an unnamed which we re-collect from socket")
    {
        sockaddr_un un;

        addr::addr_unix u;

        CATCH_REQUIRE_FALSE(u.is_file());
        CATCH_REQUIRE_FALSE(u.is_abstract());
        CATCH_REQUIRE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == std::string());
        CATCH_REQUIRE(u.to_uri() == "unix:");

        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }

        snapdev::raii_fd_t s(socket(AF_UNIX, SOCK_STREAM, 0));
        CATCH_REQUIRE(s != nullptr);

        // unnamed sockets are unbound...

        addr::addr_unix retrieve;
        retrieve.set_from_socket(s.get());
        CATCH_REQUIRE(retrieve == u);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("addr_unix::file", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::file: addr_unix() with a relative file name")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("test");
            int const max(rand() % 5 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path, name.c_str(), sizeof(init.sun_path) - 1);

            addr::addr_unix u(init);

            CATCH_REQUIRE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name);

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
            for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::file: addr_unix() with a relative file name; string constructor")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("test");
            int const max(rand() % 5 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            addr::addr_unix u(name);

            CATCH_REQUIRE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name);

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
            for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::file: addr_unix() with a relative file name using set_file()")
    {
        addr::addr_unix u;

        for(int count(0); count < 10; ++count)
        {
            std::string name("test");
            int const max(rand() % 25 + 3); // vary more to correctly verify that we clear the end of the buffer
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path, name.c_str(), sizeof(init.sun_path) - 1);

            u.set_file(name);

            CATCH_REQUIRE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name);

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
            for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::file: addr_unix() with a full file name")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("/run/snapwebsites/sockets/test");
            int const max(rand() % 25 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path, name.c_str(), sizeof(init.sun_path) - 1);

            addr::addr_unix u(init);

            CATCH_REQUIRE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix://" + name);

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
            for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::file: addr_unix() with a long file name")
    {
        sockaddr_un un;
        for(int count(1); count < static_cast<int>(sizeof(un.sun_path) - 1); ++count)
        {
            std::string name;
            for(int id(0); id < count; ++id)
            {
                name += '0' + rand() % 10;
            }

            addr::addr_unix u(name);

            CATCH_REQUIRE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name);

            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
            for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::file: addr_unix() verify that file gets unlink()'ed")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("test");
            int const max(rand() % 5 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            addr::addr_unix u(name);

            std::string cmd("touch ");
            cmd += name;
            CATCH_REQUIRE(system(cmd.c_str()) == 0);

            struct stat s;
            CATCH_REQUIRE(stat(name.c_str(), &s) == 0);

            CATCH_REQUIRE(u.unlink() == 0);

            CATCH_REQUIRE(stat(name.c_str(), &s) != 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::file: addr_unix() with a file name which we re-collect from socket")
    {
        sockaddr_un un;

        std::string name("socket-test");
        int count(rand() % 5 + 3);
        for(int id(0); id < count; ++id)
        {
            name += '0' + rand() % 10;
        }
        unlink(name.c_str());

        addr::addr_unix u(name);

        CATCH_REQUIRE(u.is_file());
        CATCH_REQUIRE_FALSE(u.is_abstract());
        CATCH_REQUIRE_FALSE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == name);
        CATCH_REQUIRE(u.to_uri() == "unix:" + name);

        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
        for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }

        snapdev::raii_fd_t s(socket(AF_UNIX, SOCK_STREAM, 0));
        CATCH_REQUIRE(s != nullptr);
        sockaddr_un address;
        u.get_un(address);
        CATCH_REQUIRE(bind(s.get(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);

        addr::addr_unix retrieve;
        retrieve.set_from_socket(s.get());
        CATCH_REQUIRE(retrieve == u);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("addr_unix::abstract", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::abstract: addr_unix() with a relative abstract name")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("abstract/test");
            int const max(rand() % 25 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path + 1, name.c_str(), sizeof(init.sun_path) - 2);

            addr::addr_unix u(init);

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name + "?abstract");
            CATCH_REQUIRE(u.unlink() == 0);

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(un.sun_path[0] == '\0');
            CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
            for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::abstract: addr_unix() with a relative abstract name with string constructor")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("abstract/test");
            int const max(rand() % 25 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            addr::addr_unix u(name, true);

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name + "?abstract");

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(un.sun_path[0] == '\0');
            CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
            for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::abstract: addr_unix() with an abstract name using set_abstract()")
    {
        addr::addr_unix u;

        for(int count(0); count < 10; ++count)
        {
            std::string name("test");
            int const max(rand() % 25 + 3); // vary more to correctly verify that we clear the end of the buffer
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path, name.c_str(), sizeof(init.sun_path) - 1);

            u.set_abstract(name);

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name + "?abstract");

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(un.sun_path[0] == '\0');
            CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
            for(std::size_t idx(name.length() +1); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::abstract: addr_unix() with a full abstract name")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("/net/snapwebsites/settings/test");
            int const max(rand() % 25 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path + 1, name.c_str(), sizeof(init.sun_path) - 2);

            addr::addr_unix u(init);

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix://" + name + "?abstract");

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(un.sun_path[0] == '\0');
            CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
            for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::abstract: addr_unix() with a long abstract name")
    {
        sockaddr_un un;
        for(int count(1); count < static_cast<int>(sizeof(un.sun_path) - 2); ++count)
        {
            std::string name;
            for(int id(0); id < count; ++id)
            {
                name += '0' + rand() % 10;
            }

            addr::addr_unix u(name, true);

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name + "?abstract");

            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(un.sun_path[0] == '\0');
            CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
            for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::abstract: addr_unix() with an abstract name which we re-collect from socket")
    {
        sockaddr_un un;

        std::string name("/net/snapwebsites/test");
        int count(rand() % 5 + 3);
        for(int id(0); id < count; ++id)
        {
            name += '0' + rand() % 10;
        }

        addr::addr_unix u(name, true);

        CATCH_REQUIRE_FALSE(u.is_file());
        CATCH_REQUIRE(u.is_abstract());
        CATCH_REQUIRE_FALSE(u.is_unnamed());
        CATCH_REQUIRE(u.to_string() == name);
        CATCH_REQUIRE(u.to_uri() == "unix://" + name + "?abstract");

        u.get_un(un);
        CATCH_REQUIRE(un.sun_family == AF_UNIX);
        CATCH_REQUIRE(un.sun_path[0] == '\0');
        CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
        for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
        {
            CATCH_CHECK(un.sun_path[idx] == 0);
        }

        snapdev::raii_fd_t s(socket(AF_UNIX, SOCK_STREAM, 0));
        CATCH_REQUIRE(s != nullptr);
        sockaddr_un address;
        u.get_un(address);
        socklen_t const len(sizeof(address.sun_family) + 1 + strlen(address.sun_path + 1));
        CATCH_REQUIRE(bind(s.get(), reinterpret_cast<sockaddr *>(&address), len) == 0);

        addr::addr_unix retrieve;
        retrieve.set_from_socket(s.get());
        CATCH_REQUIRE(retrieve == u);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("addr_unix::compare", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::compare: two addr_unix() to compare with ==, !=, <, <=, >, >=")
    {
        addr::addr_unix a;
        addr::addr_unix b;

        CATCH_REQUIRE(a == b);
        CATCH_REQUIRE_FALSE(a != b);
        CATCH_REQUIRE_FALSE(a < b);
        CATCH_REQUIRE(a <= b);
        CATCH_REQUIRE_FALSE(a > b);
        CATCH_REQUIRE(a >= b);

        a.set_uri("unix:flowers");
        b.set_uri("unix:oranges");

        CATCH_REQUIRE_FALSE(a == b);
        CATCH_REQUIRE(a != b);
        CATCH_REQUIRE(a < b);
        CATCH_REQUIRE(a <= b);
        CATCH_REQUIRE_FALSE(a > b);
        CATCH_REQUIRE_FALSE(a >= b);

        std::swap(a, b);

        CATCH_REQUIRE_FALSE(a == b);
        CATCH_REQUIRE(a != b);
        CATCH_REQUIRE_FALSE(a < b);
        CATCH_REQUIRE_FALSE(a <= b);
        CATCH_REQUIRE(a > b);
        CATCH_REQUIRE(a >= b);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::compare: two sockaddr_un to compare with ==, !=, <, <=, >, >=")
    {
        sockaddr_un a = addr::init_un();
        sockaddr_un b = addr::init_un();

        CATCH_REQUIRE(a == b);
        CATCH_REQUIRE_FALSE(a != b);
        CATCH_REQUIRE_FALSE(a < b);
        CATCH_REQUIRE(a <= b);
        CATCH_REQUIRE_FALSE(a > b);
        CATCH_REQUIRE(a >= b);

        strncpy(a.sun_path, "unix:flowers", sizeof(a.sun_path) - 1);
        strncpy(b.sun_path, "unix:oranges", sizeof(a.sun_path) - 1);

        CATCH_REQUIRE_FALSE(a == b);
        CATCH_REQUIRE(a != b);
        CATCH_REQUIRE(a < b);
        CATCH_REQUIRE(a <= b);
        CATCH_REQUIRE_FALSE(a > b);
        CATCH_REQUIRE_FALSE(a >= b);

        std::swap(a, b);

        CATCH_REQUIRE_FALSE(a == b);
        CATCH_REQUIRE(a != b);
        CATCH_REQUIRE_FALSE(a < b);
        CATCH_REQUIRE_FALSE(a <= b);
        CATCH_REQUIRE(a > b);
        CATCH_REQUIRE(a >= b);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("addr_unix::mix", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::mix: addr_unix() with a relative file name then unnamed")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("test");
            int const max(rand() % 5 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path, name.c_str(), sizeof(init.sun_path) - 1);

            addr::addr_unix u(init);

            CATCH_REQUIRE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix:" + name);

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
            for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }

            u.make_unnamed();

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == std::string());
            CATCH_REQUIRE(u.to_uri() == "unix:");

            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::mix: addr_unix() with a full abstract name then unnamed")
    {
        for(int count(0); count < 10; ++count)
        {
            std::string name("/net/snapwebsites/settings/test");
            int const max(rand() % 25 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }

            // verify the init_un() as we're at it
            //
            sockaddr_un init = addr::init_un();
            CATCH_REQUIRE(init.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
            {
                CATCH_CHECK(init.sun_path[idx] == 0);
            }
            strncpy(init.sun_path + 1, name.c_str(), sizeof(init.sun_path) - 2);

            addr::addr_unix u(init);

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE(u.is_abstract());
            CATCH_REQUIRE_FALSE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == name);
            CATCH_REQUIRE(u.to_uri() == "unix://" + name + "?abstract");

            sockaddr_un un;
            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            CATCH_REQUIRE(un.sun_path[0] == '\0');
            CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
            for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }

            u.make_unnamed();

            CATCH_REQUIRE_FALSE(u.is_file());
            CATCH_REQUIRE_FALSE(u.is_abstract());
            CATCH_REQUIRE(u.is_unnamed());
            CATCH_REQUIRE(u.to_string() == std::string());
            CATCH_REQUIRE(u.to_uri() == "unix:");

            u.get_un(un);
            CATCH_REQUIRE(un.sun_family == AF_UNIX);
            for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
            {
                CATCH_CHECK(un.sun_path[idx] == 0);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::mix: addr_unix() with various set_uri()")
    {
        addr::addr_unix u;

        for(int count(0); count < 222; ++count)
        {
            int type(count % 3);

            std::string name;
            if(type != 2)
            {
                name = "/run/snapwebsites/sockets/test";
                int const max(rand() % 25 + 3);
                switch(rand() % 4)
                {
                case 0:
                    break;

                case 1:
                    name += '-';
                    break;

                case 2:
                    name += '+';
                    break;

                case 3:
                    name += '.';
                    break;

                default:
                    throw std::runtime_error("unexpected case");

                }
                for(int id(0); id < max; ++id)
                {
                    name += '0' + rand() % 10;
                }
            }

            bool force(count % 6 != type);

            sockaddr_un un;
            switch(count % 3)
            {
            case 0:
                u.set_uri("unix://" + name + (force ? "?file" : ""));

                CATCH_REQUIRE(u.is_file());
                CATCH_REQUIRE_FALSE(u.is_abstract());
                CATCH_REQUIRE_FALSE(u.is_unnamed());
                CATCH_REQUIRE(u.to_string() == name);
                CATCH_REQUIRE(u.to_uri() == "unix://" + name);

                u.get_un(un);
                CATCH_REQUIRE(un.sun_family == AF_UNIX);
                CATCH_REQUIRE(strcmp(un.sun_path, name.c_str()) == 0);
                for(std::size_t idx(name.length()); idx < sizeof(un.sun_path); ++idx)
                {
                    CATCH_CHECK(un.sun_path[idx] == 0);
                }
                break;

            case 1:
                u.set_uri("unix://" + name + "?abstract");

                CATCH_REQUIRE_FALSE(u.is_file());
                CATCH_REQUIRE(u.is_abstract());
                CATCH_REQUIRE_FALSE(u.is_unnamed());
                CATCH_REQUIRE(u.to_string() == name);
                CATCH_REQUIRE(u.to_uri() == "unix://" + name + "?abstract");

                u.get_un(un);
                CATCH_REQUIRE(un.sun_family == AF_UNIX);
                CATCH_REQUIRE(un.sun_path[0] == '\0');
                CATCH_REQUIRE(strcmp(un.sun_path + 1, name.c_str()) == 0);
                for(std::size_t idx(name.length() + 1); idx < sizeof(un.sun_path); ++idx)
                {
                    CATCH_CHECK(un.sun_path[idx] == 0);
                }
                break;

            case 2:
                u.set_uri(force ? "unix:?unnamed" : "unix:");

                CATCH_REQUIRE_FALSE(u.is_file());
                CATCH_REQUIRE_FALSE(u.is_abstract());
                CATCH_REQUIRE(u.is_unnamed());
                CATCH_REQUIRE(u.to_string() == std::string());
                CATCH_REQUIRE(u.to_uri() == "unix:");

                u.get_un(un);
                CATCH_REQUIRE(un.sun_family == AF_UNIX);
                for(std::size_t idx(0); idx < sizeof(un.sun_path); ++idx)
                {
                    CATCH_CHECK(un.sun_path[idx] == 0);
                }
                break;

            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("addr_unix::owner", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::owner: addr_unix() check group")
    {
        addr::addr_unix u;

        for(int count(0); count < 10; ++count)
        {
            std::string name("grp");
            int const max(rand() % 5 + 3);
            for(int id(0); id < max; ++id)
            {
                name += '0' + rand() % 10;
            }
            u.set_group(name);
            CATCH_REQUIRE(u.get_group() == name);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::owner: addr_unix() check mode")
    {
        addr::addr_unix u;

        for(int m(0); m < 01777; ++m)
        {
            u.set_mode(m);
            CATCH_REQUIRE(u.get_mode() == m);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("addr_unix::invalid", "[addr_unix]")
{
    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with an invalid address family")
    {
        for(int family(-25); family <= 25; ++family)
        {
            if(family == AF_UNIX)
            {
                continue;
            }

            sockaddr_un init = addr::init_un();
            init.sun_family = family;

            // constructor
            //
            CATCH_REQUIRE_THROWS_AS(addr::addr_unix(init), addr::addr_invalid_structure);

            // set_un()
            //
            addr::addr_unix u;
            CATCH_REQUIRE_THROWS_AS(u.set_un(init), addr::addr_invalid_structure);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with an unnamed string but marked abstract")
    {
        CATCH_REQUIRE_THROWS_AS(addr::addr_unix(std::string(), true), addr::addr_invalid_argument);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with a URI but a missing path")
    {
        addr::addr_unix u;
        CATCH_REQUIRE_THROWS_AS(u.set_uri("unix://"), addr::addr_invalid_argument);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with a URI but a missing scheme")
    {
        addr::addr_unix u;
        CATCH_REQUIRE_THROWS_AS(u.set_uri("://address"), addr::addr_invalid_argument);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with a URI but an invalid scheme")
    {
        addr::addr_unix u;

        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("bad(scheme)://address")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: \"bad(scheme)\" is not a supported URI scheme for a Unix address; supported scheme are limited to `[a-zA-Z][-+.a-zA-Z0-9]*`."));

        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("other;scheme://address")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: \"other;scheme\" is not a supported URI scheme for a Unix address; supported scheme are limited to `[a-zA-Z][-+.a-zA-Z0-9]*`."));

        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("unknown|scheme://address")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: \"unknown|scheme\" is not a supported URI scheme for a Unix address; supported scheme are limited to `[a-zA-Z][-+.a-zA-Z0-9]*`."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with too long a file name")
    {
        sockaddr_un un;
        for(int count(static_cast<int>(sizeof(un.sun_path)));
            count < static_cast<int>(sizeof(un.sun_path) + 11);
            ++count)
        {
            std::string name;
            for(int id(0); id < count; ++id)
            {
                name += '0' + rand() % 10;
            }

            CATCH_REQUIRE_THROWS_AS(addr::addr_unix(name), addr::addr_invalid_argument);

            addr::addr_unix u;
            CATCH_REQUIRE_THROWS_AS(u.set_uri("unix:" + name), addr::addr_invalid_argument);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with too long an abstract name")
    {
        sockaddr_un un;
        for(int count(static_cast<int>(sizeof(un.sun_path) - 1));
            count < static_cast<int>(sizeof(un.sun_path) + 10);
            ++count)
        {
            std::string name;
            for(int id(0); id < count; ++id)
            {
                name += '0' + rand() % 10;
            }

            CATCH_REQUIRE_THROWS_AS(addr::addr_unix(name, true), addr::addr_invalid_argument);

            addr::addr_unix u;
            CATCH_REQUIRE_THROWS_AS(u.set_uri("unix:" + name + "?abstract"), addr::addr_invalid_argument);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with a long filename (missing '\\0')")
    {
        sockaddr_un init = addr::init_un();
        for(std::size_t idx(0); idx < sizeof(init.sun_path); ++idx)
        {
            init.sun_path[idx] = '0' + rand() % 10;
        }

        // constructor
        //
        CATCH_REQUIRE_THROWS_AS(addr::addr_unix(init), addr::addr_invalid_argument);

        // set_un()
        //
        addr::addr_unix u;
        CATCH_REQUIRE_THROWS_AS(u.set_un(init), addr::addr_invalid_argument);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with a long abstract name (missing '\\0')")
    {
        sockaddr_un init = addr::init_un();
        for(std::size_t idx(1); idx < sizeof(init.sun_path); ++idx)
        {
            init.sun_path[idx] = '0' + rand() % 10;
        }

        // constructor
        //
        CATCH_REQUIRE_THROWS_AS(addr::addr_unix(init), addr::addr_invalid_argument);

        // set_un()
        //
        addr::addr_unix u;
        CATCH_REQUIRE_THROWS_AS(u.set_un(init), addr::addr_invalid_argument);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with a long abstract name (missing '\\0')")
    {
        addr::addr_unix u;

        // missing ":"
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("cu/run/snapwebsites/sockets")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: invalid URI for a Unix address, scheme not found (':' missing)."));

        // "?alexis"
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("unix:/run/snapwebsites/sockets?alexis")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: \"alexis\" is not a supported URI query string for a Unix address; supported query strings are one of: \"unnamed\", \"file\" and \"abstract\"."));

        // "_test:"
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("_test:/run/snapwebsites/sockets?abstract")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: \"_test\" is not a supported URI scheme for a Unix address; supported scheme are limited to `[a-zA-Z][-+.a-zA-Z0-9]*`."));

        // name with "?unnamed"
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              u.set_uri("cd:not-empty?unnamed")
            , addr::addr_invalid_argument
            , Catch::Matchers::ExceptionMessage("addr_error: address \"not-empty\" must be empty to represent an unnamed Unix address."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with invalid characters (controls)")
    {
        for(int c(1); c < 0x20; ++c)
        {
            std::string name;
            name += c;
            CATCH_REQUIRE_THROWS_AS(addr::addr_unix(name), addr::addr_invalid_argument);
        }

        for(int c(0x7F); c < 0xA0; ++c)
        {
            std::u32string u32;
            u32 += c;
            std::string name(libutf8::to_u8string(u32));
            CATCH_REQUIRE_THROWS_AS(addr::addr_unix(name), addr::addr_invalid_argument);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: addr_unix() with invalid UTF-8 characters")
    {
        std::string name;
        for(int c(0); c < 10; ++c)
        {
            name += static_cast<char>(0x80);
        }
        CATCH_REQUIRE_THROWS_AS(addr::addr_unix(name), libutf8::libutf8_exception_decoding);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: get addr_unix() of socket set to -1")
    {
        addr::addr_unix u;
        bool const result(u.set_from_socket(-1));
        int const e(errno);
        CATCH_REQUIRE_FALSE(result);
        CATCH_REQUIRE(e == EBADF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("addr_unix::invalid: get addr_unix() of socket set to UDP")
    {
        addr::addr udp(addr::string_to_addr(
                  "127.0.0.1:3999"
                , "127.0.0.1"
                , 3999
                , "udp"));
        snapdev::raii_fd_t s(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
        CATCH_REQUIRE(s != nullptr);
        sockaddr_in in;
        udp.get_ipv4(in);
        CATCH_REQUIRE(bind(s.get(), reinterpret_cast<sockaddr *>(&in), sizeof(in)) == 0);

        addr::addr_unix u;
        bool const result(u.set_from_socket(s.get()));
        int const e(errno);
        CATCH_REQUIRE_FALSE(result);
        CATCH_REQUIRE(e == EADDRNOTAVAIL);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
