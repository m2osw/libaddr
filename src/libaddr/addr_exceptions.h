// Network Address -- classes functions to ease handling IP addresses
// Copyright (c) 2012-2018  Made to Order Software Corp.  All Rights Reserved
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
//
#pragma once

/** \file
 * \brief The libaddr exceptions.
 *
 * This header defines various exceptions used throughout the addr library.
 */

// libexcept library
//
#include "libexcept/exception.h"


namespace addr
{


class addr_invalid_argument_exception : public libexcept::exception_t
{
public:
    addr_invalid_argument_exception(char const *        what_msg) : exception_t(what_msg) {}
    addr_invalid_argument_exception(std::string const & what_msg) : exception_t(what_msg) {}
};

class addr_invalid_state_exception : public libexcept::exception_t
{
public:
    addr_invalid_state_exception(char const *        what_msg) : exception_t(what_msg) {}
    addr_invalid_state_exception(std::string const & what_msg) : exception_t(what_msg) {}
};

class addr_io_exception : public libexcept::exception_t
{
public:
    addr_io_exception(char const *        what_msg) : exception_t(what_msg) {}
    addr_io_exception(std::string const & what_msg) : exception_t(what_msg) {}
};

class addr_invalid_structure_exception : public libexcept::logic_exception_t
{
public:
    addr_invalid_structure_exception(char const *        what_msg) : logic_exception_t(what_msg) {}
    addr_invalid_structure_exception(std::string const & what_msg) : logic_exception_t(what_msg) {}
};

class addr_invalid_parameter_exception : public libexcept::logic_exception_t
{
public:
    addr_invalid_parameter_exception(char const *        what_msg) : logic_exception_t(what_msg) {}
    addr_invalid_parameter_exception(std::string const & what_msg) : logic_exception_t(what_msg) {}
};


}
// addr namespace
// vim: ts=4 sw=4 et
