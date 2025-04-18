// Copyright (c) 2022-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/libaddr
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

/** \file
 * \brief Declaration of a validator to verify that a parameter is an address.
 *
 * The advgetopt library offers a way to validate command line and
 * configuration parameters through the use of validators. This validator
 * is an extension which verifies that the parameter is considered to be
 * a valid address or hostname.
 */

// self
//
#include    "libaddr/addr_parser.h"


// advgetopt
//
#include    "advgetopt/validator.h"


// C++
//
#include    <regex>



namespace addr
{



class validator_address
    : public advgetopt::validator
{
public:
                                validator_address(advgetopt::string_list_t const & data);

    // validator implementation
    //
    virtual std::string         name() const override;
    virtual bool                validate(std::string const & value) const override;

    static bool                 convert_string(std::string const & address
                                             , addr_range::vector_t & result);

private:
    mutable addr_parser         f_parser = addr_parser();
};



}   // namespace advgetopt
// vim: ts=4 sw=4 et
