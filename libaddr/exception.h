// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/libaddr
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

/** \file
 * \brief The list of libaddr exceptions.
 *
 * This header defines various exceptions used throughout the addr library.
 */

// libexcept library
//
#include    <libexcept/exception.h>


namespace addr
{


DECLARE_LOGIC_ERROR(logic_error); // LCOV_EXCL_LINE
DECLARE_OUT_OF_RANGE(out_of_range);

DECLARE_MAIN_EXCEPTION(addr_error);

DECLARE_EXCEPTION(addr_error, addr_io_error);
DECLARE_EXCEPTION(addr_error, addr_invalid_argument);
DECLARE_EXCEPTION(addr_error, addr_invalid_state);
DECLARE_EXCEPTION(addr_error, addr_invalid_structure);
DECLARE_EXCEPTION(addr_error, addr_unexpected_error);
DECLARE_EXCEPTION(addr_error, addr_unexpected_mask);
DECLARE_EXCEPTION(addr_error, addr_unsupported_as_range);



}
// namespace addr
// vim: ts=4 sw=4 et
