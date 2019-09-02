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
#pragma once


/** \file
 * \brief Basic definitions for all of our libaddr unit tests.
 *
 * This files includes and declares basics that are to be used
 * by all our unit tests.
 */


// libaddr library
//
#include    <libaddr/addr_parser.h>
#include    <libaddr/addr_exception.h>
#include    <libaddr/version.h>


// catch
//
#include    <catch2/snapcatch2.hpp>


// C++ library
//
#include    <string>
#include    <cstring>
#include    <cstdlib>
#include    <iostream>
#include    <sstream>


// C library
//
#include    <unistd.h>
#include    <limits.h>
#include    <netdb.h>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{

// Place globals defined by main() from the command line and used
// by various tests in here
//

extern int          g_tcp_port;


}
// namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
