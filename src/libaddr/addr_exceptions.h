// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
//
// http://snapwebsites.org/project/libaddr
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
