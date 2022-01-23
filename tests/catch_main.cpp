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
 * \brief The main() function of the addr library tests.
 *
 * This file implements the main() function of the unit tests used to
 * verify the addr library.
 *
 * It defines any globals (none at this point) and a few basic command
 * line options such as --help and --version.
 */

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

// self
//
#include    "catch_main.h"


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{

int         g_tcp_port = -1;

}


namespace
{


Catch::Clara::Parser add_command_line_options(Catch::Clara::Parser const & cli)
{
    return cli
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_tcp_port, "port")
              ["--tcp-port"]
              ("define a TCP port we can connect to to test the get_from_socket() function");
}


}
// namespace


int main(int argc, char * argv[])
{
    return SNAP_CATCH2_NAMESPACE::snap_catch2_main(
              "libaddr"
            , LIBADDR_VERSION_STRING
            , argc
            , argv
            , nullptr
            , &add_command_line_options
            , nullptr
            , nullptr
        );
}

// vim: ts=4 sw=4 et
