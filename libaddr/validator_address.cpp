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
 * * `address=[yes|no|required] [commas] [spaces] [newlines] [range] [lookup]`
 *
 *   If `yes`, then an address is allowed. This is the default if none of
 *   `yes`, `no`, and `required` were specified.
 *
 *   If `no`, an address is not allowed.
 *
 *   If `required`, an address must be specified (it may still come from
 *   the default address). This option is mutually exclusive with the `yes`
 *   and the `no`. Only one of the three can be specified.
 *
 *   The `commas`, `spaces`, `newlines` allow for multiple addresses
 *   separated by commas (,), spaces ( ), and newlines (\\n). None,
 *   one, or a mix can be specified. The newlines should probably not
 *   be used with the command line validators since in all likelihood
 *   users won't pass a parameter with newlines in it.
 *
 *   The `range` allows address ranges (addresses separated by a dash (-)).
 *
 *   The `lookup` -- if a hostname is specified, a lookup is allowed
 *
 * * `default=\<address>:\<port>/\<mask> ...`
 *
 *   Specify a default address, port, and mask.
 *
 *   You can specify two of these: one for IPv4 and one for IPv6. To make
 *   sure that an address is viewed as an IPv6 address, make sure to use
 *   brackets around the address and the mask.
 *
 * * `comment=yes|no`
 *
 *   Whether comments are allowed or not. In most likelihood, it should not
 *   be used with the validator since parameters passed on the command line
 *   cannot be separated by newlines.
 *
 * * `port=yes|no|required`
 *
 *    Whether the port is allowed (`yes`), not allowed (`no`), or must be
 *    specified (`required`).
 *
 * * `mask=yes|no|address`
 *
 *    If `yes` then a mask is allowed.
 *
 *    If `no` then no mask is allowed.
 *
 *    If `address` then a mask is allowed and it can be written as an IP
 *    address (opposed to just a number).
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


// C++
//
#include    <cassert>


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

        std::string name;
        advgetopt::string_list_t values;
        std::string::size_type const equal(p.find('='));
        if(equal == std::string::npos)
        {
            name = p;
            values.push_back("yes");
        }
        else
        {
            name = p.substr(0, equal);
            advgetopt::split_string(p.substr(equal + 1), values, {" "});
            if(values.empty())
            {
                values.push_back("yes");
            }
        }

        assert(!values.empty());

        switch(name[0])
        {
        case 'a':
            if(name == "address")
            {
                // first reset it all
                //
                f_parser.set_allow(allow_t::ALLOW_ADDRESS, false);
                f_parser.set_allow(allow_t::ALLOW_REQUIRED_ADDRESS, false);
                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, false);
                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_SPACES, false);
                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_NEWLINES, false);
                f_parser.set_allow(allow_t::ALLOW_ADDRESS_RANGE, false);
                f_parser.set_allow(allow_t::ALLOW_ADDRESS_LOOKUP, false);

                if(std::find(values.begin(), values.end(), std::string("no")) != values.end())
                {
                    if(values.size() != 1)
                    {
                        cppthread::log
                            << cppthread::log_level_t::error
                            << "the \"no\" option in the address=... option must be used by itsefl."
                            << cppthread::end;
                    }
                }
                else
                {
                    f_parser.set_allow(allow_t::ALLOW_ADDRESS, true);
                    for(auto const & v : values)
                    {
                        switch(v[0])
                        {
                        case 'c':
                            if(v == "commas")
                            {
                                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
                                continue;
                            }
                            break;

                        case 'l':
                            if(v == "lookup")
                            {
                                f_parser.set_allow(allow_t::ALLOW_ADDRESS_LOOKUP, true);
                                continue;
                            }
                            break;

                        case 'n':
                            if(v == "newlines")
                            {
                                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_NEWLINES, true);
                                continue;
                            }
                            break;

                        case 'r':
                            if(v == "range")
                            {
                                f_parser.set_allow(allow_t::ALLOW_ADDRESS_RANGE, true);
                                continue;
                            }
                            if(v == "required")
                            {
                                f_parser.set_allow(allow_t::ALLOW_REQUIRED_ADDRESS, true);
                                continue;
                            }
                            break;

                        case 's':
                            if(v == "spaces")
                            {
                                f_parser.set_allow(allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
                                continue;
                            }
                            break;

                        case 'y':
                            if(v == "yes")
                            {
                                if(std::find(values.begin(), values.end(), std::string("required")) != values.end())
                                {
                                    cppthread::log
                                        << cppthread::log_level_t::error
                                        << "the \"yes\" and \"required\" options are mutually exclusive, use one or the other (or none, which defaults to \"yes\")."
                                        << cppthread::end;
                                }
                                continue;
                            }
                            break;

                        }

                        cppthread::log
                            << cppthread::log_level_t::error
                            << "the \""
                            << v
                            << "\" parameter is not understood by the \"address\" options."
                            << cppthread::end;
                    }
                }
                continue;
            }
            break;

        case 'c':
            if(name == "comment")
            {
                f_parser.set_allow(allow_t::ALLOW_COMMENT, false);

                if(values.size() != 1)
                {
                    cppthread::log
                        << cppthread::log_level_t::error
                        << "the \"comment\" option expects one of \"yes\" or \"no\"."
                        << cppthread::end;
                }
                else if(values[0] != "no")
                {
                    f_parser.set_allow(allow_t::ALLOW_COMMENT, true);

                    if(values[0] != "yes")
                    {
                        cppthread::log
                            << cppthread::log_level_t::error
                            << "value \""
                            << values[0]
                            << "\" is unexpected for the \"comment\" option which expects one of \"yes\" or \"no\"."
                            << cppthread::end;
                    }
                }
                continue;
            }
            break;

        case 'd':
            if(name == "default"
            || name == "defaults")
            {
                // `values` cannot be empty, instead it gets set to "yes"
                // and in case of the defaults, it means clear the defaults
                //
                if(values.size() == 1
                && values[0] == "yes")
                {
                    // remove all defaults
                    //
                    f_parser.set_default_address(std::string());
                    f_parser.set_default_port(std::string());
                    f_parser.set_default_mask(std::string());
                }
                else
                {
                    // TODO we need to properly distinguish between
                    //      IPv4 and IPv6
                    //
                    for(auto const & param : values)
                    {
                        addr_parser parser;
                        parser.set_allow(allow_t::ALLOW_MASK, true);
                        parser.set_protocol(IPPROTO_TCP);
                        addr_range::vector_t v(parser.parse(param));
                        if(v.size() == 1
                        && v[0].has_from()
                        && !v[0].has_to())
                        {
                            // TODO: try to find a way to avoid getting multiple
                            //       IP addresses; by default the system gives
                            //       us one IP per Proto (TCP, UDP, IP)
                            //
                            addr const a(v[0].get_from());

                            if(a.get_network_type() != network_type_t::NETWORK_TYPE_ANY)
                            {
                                f_parser.set_default_address(a.to_ipv4or6_string(STRING_IP_BRACKET_ADDRESS));
                            }

                            if(a.get_port() != 0)
                            {
                                f_parser.set_default_port(a.get_str_port());
                            }

                            int const mask(a.get_mask_size());
                            if(mask != -1
                            && mask != 128)
                            {
                                if(a.is_ipv4())
                                {
                                    f_parser.set_default_mask(std::to_string(mask));
                                }
                                else
                                {
                                    // add the '[...]' to make it an IPv6 mask
                                    //
                                    f_parser.set_default_mask("[" + std::to_string(mask) + "]");
                                }
                            }
                        }
                        else
                        {
                            cppthread::log
                                << cppthread::log_level_t::error
                                << "the default address \""
                                << param
                                << "\" could not be parsed properly. Error: "
                                << parser.error_messages()
                                << cppthread::end;
                        }
                    }
                }
                continue;
            }
            break;

        case 'm':
            if(name == "mask")
            {
                f_parser.set_allow(allow_t::ALLOW_MASK, false);
                f_parser.set_allow(allow_t::ALLOW_ADDRESS_MASK, false);

                if(values.size() != 1)
                {
                    cppthread::log
                        << cppthread::log_level_t::error
                        << "the \"mask\" option expects one of \"yes\", \"no\", or \"address\"."
                        << cppthread::end;
                }
                else if(values[0] != "no")
                {
                    f_parser.set_allow(allow_t::ALLOW_MASK, true);

                    if(values[0] == "address")
                    {
                        f_parser.set_allow(allow_t::ALLOW_ADDRESS_MASK, true);
                    }
                    else if(values[0] != "yes")
                    {
                        cppthread::log
                            << cppthread::log_level_t::error
                            << "value \""
                            << values[0]
                            << "\" is unexpected for the \"mask\" option which expects one of \"yes\", \"no\", or \"address\"."
                            << cppthread::end;
                    }
                }
                continue;
            }
            break;

        case 'p':
            if(name == "port")
            {
                f_parser.set_allow(allow_t::ALLOW_PORT, false);
                f_parser.set_allow(allow_t::ALLOW_REQUIRED_PORT, false);

                if(values.size() != 1)
                {
                    cppthread::log
                        << cppthread::log_level_t::error
                        << "the port=... option expects one of \"yes\", \"no\", or \"required\"."
                        << cppthread::end;
                }
                else if(values[0] != "no")
                {
                    f_parser.set_allow(allow_t::ALLOW_PORT, true);

                    if(values[0] != "yes")
                    {
                        if(values[0] == "required")
                        {
                            f_parser.set_allow(allow_t::ALLOW_REQUIRED_PORT, true);
                        }
                        else
                        {
                            cppthread::log
                                << cppthread::log_level_t::error
                                << "unexpected value \""
                                << values[0]
                                << "\" for the port=... option which expects one of \"yes\", \"no\", or \"required\"."
                                << cppthread::end;
                        }
                    }
                }
                continue;
            }
            break;

        }

        cppthread::log
            << cppthread::log_level_t::error
            << "\""
            << p
            << "\" is not a known option for the address validator."
            << cppthread::end;
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

//if(f_parser.has_errors())
//{
//std::cerr << "--- parser errors: [" << f_parser.error_messages() << "]\n";
//}

    return !f_parser.has_errors();
}



} // namespace advgetopt
// vim: ts=4 sw=4 et
