// Copyright (c) 2022  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation of the address validator.
 *
 * The address validator allows us to verify that a parameter represents
 * an address as supported by the addr_parser class.
 *
 * The input parameters understood by the address validator are:
 *
 * * `address` -- an address is allowed
 * * `no_address` -- an address is not allowed
 * * `required_address` -- an address is required
 * * `addresses_separated_by_commas` -- one or more addresses separated by
 *                                      commas are allowed
 * * `addresses_separated_by_spaces` -- one or more addresses separated by
 *                                      spaces are allowed
 * * `lookup` -- if a hostname is specified, a lookup is allowed
 * * `no_lookup` -- hostnames are not acceptable
 * * `port` -- a port is allowed
 * * `no_port` -- a port is not allowed
 * * `required-port` -- a port is required
 * * `mask` -- a mask is allowed
 * * `no_mask` -- a mask is not allowed
 * * `"\<address>:\<port>/\<mask>"` -- defaults
 *
 * \todo
 * Once support for multi-ports and ranges is available, add such to the
 * validator.
 */

// self
//
#include    "libaddr/validator_address.h"


// cppthread
//
#include    <cppthread/log.h>


// snapdev
//
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>




namespace addr
{



namespace
{



class validator_address_factory
    : public advgetopt::validator_factory
{
public:
    validator_address_factory()
    {
        advgetopt::validator::register_validator(*this);
    }

    virtual std::string get_name() const override
    {
        return std::string("address");
    }

    virtual std::shared_ptr<advgetopt::validator> create(advgetopt::string_list_t const & data) const override
    {
        return std::make_shared<validator_address>(data);
    }
};

validator_address_factory         g_validator_address_factory;



} // no name namespace





/** \brief Initialize a validator_address object.
 *
 * This function parses the specified \p address as a default set of
 * parameters to the addr_parser class. Note that the function accepts
 * a list but the list is expected to be empty or have exactly one
 * parameter.
 *
 * \param[in] data  The address defining defaults.
 */
validator_address::validator_address(advgetopt::string_list_t const & data)
{
    for(auto const & p : data)
    {
        // this should not happen in the regular advgetopt since the empty
        // string does not get added to the string_list_t vector; but a
        // user could do that
        //
        if(p.empty())
        {
            continue;
        }

        switch(p[0])
        {
        case '"':
        case '\'':
            // defaults defined as "<address>:<port>/<mask>" where each part
            // is optional and if not present has no effect on the internal
            // defaults of the addr_parser object
            //
            if(p.length() >= 2
            && p.back() == p[0])
            {
                std::string param(advgetopt::unquote(p));

                std::string address;
                std::string port;
                std::string mask;

                // extract each part
                //
                std::string::size_type colon(param.find(':', 1));
                std::string::size_type slash(param.find('/', colon == std::string::npos ? 1 : colon));
                if(colon != std::string::npos)
                {
                    address = param.substr(1, colon);
                    if(slash != std::string::npos)
                    {
                        port = param.substr(colon + 1, slash - colon - 1);
                        mask = param.substr(slash + 1, param.length() - slash - 1 - 2);
                    }
                    else
                    {
                        port = param.substr(colon + 1, param.length() - colon - 1 - 2);
                    }
                }
                else if(slash != std::string::npos)
                {
                    address = param.substr(1, slash);
                    mask = param.substr(slash + 1, param.length() - slash - 1 - 2);
                }
                else
                {
                    address = param.substr(1, param.length() - 2);
                }

                if(!address.empty())
                {
                    f_parser.set_default_address(address);
                }
                if(!port.empty())
                {
                    f_parser.set_default_port(port);
                }
                if(!mask.empty())
                {
                    f_parser.set_default_mask(mask);
                }
            }
            break;

        case 'a':
            if(p == "address")
            {
                f_parser.set_allow(allow_t::ALLOW_ADDRESS, true);
            }
            else if(p == "addresses_separated_by_commas")
            {
                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
            }
            else if(p == "addresses_separated_by_spaces")
            {
                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
            }
            break;

        case 'l':
            if(p == "lookup")
            {
                f_parser.set_allow(allow_t::ALLOW_ADDRESS_LOOKUP, true);
            }
            break;

        case 'm':
            if(p == "mask")
            {
                f_parser.set_allow(allow_t::ALLOW_MASK, true);
            }
            break;

        case 'n':
            if(p == "no_address"
            || p == "no_addresses")
            {
                f_parser.set_allow(allow_t::ALLOW_ADDRESS, false);
            }
            else if(p == "no-lookup")
            {
                f_parser.set_allow(allow_t::ALLOW_ADDRESS_LOOKUP, false);
            }
            else if(p == "no-mask")
            {
                f_parser.set_allow(allow_t::ALLOW_MASK, false);
            }
            else if(p == "no-port")
            {
                f_parser.set_allow(allow_t::ALLOW_PORT, false);
            }
            break;

        case 'p':
            if(p == "port")
            {
                f_parser.set_allow(allow_t::ALLOW_PORT, true);
            }
            break;

        case 'r':
            if(p == "required_address")
            {
                f_parser.set_allow(allow_t::ALLOW_REQUIRED_ADDRESS, true);
            }
            else if(p == "required_port")
            {
                f_parser.set_allow(allow_t::ALLOW_REQUIRED_PORT, true);
            }
            break;

        }
    }
}


/** \brief Return the name of this validator.
 *
 * This function returns "address".
 *
 * \return "address".
 */
std::string validator_address::name() const
{
    return std::string("address");
}


/** \brief Check the value against a regular expression.
 *
 * This function is used to match the value of an argument against a
 * regular expression. It returns true when it does match.
 *
 * \param[in] value  The value to be validated.
 *
 * \return true on a match.
 */
bool validator_address::validate(std::string const & value) const
{
    f_parser.clear_errors();
    snapdev::NOT_USED(f_parser.parse(value));
    return !f_parser.has_errors();
}



} // namespace advgetopt
// vim: ts=4 sw=4 et
