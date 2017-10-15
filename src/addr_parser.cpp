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

/** \file
 * \brief The implementation of the IP address parser.
 *
 * This function is used to parse IP addresses from a string to a
 * vector of ranges.
 */

// self
//
#include "libaddr/addr_parser.h"
#include "libaddr/addr_exceptions.h"

// C++ library
//
#include <algorithm>
//#include <sstream>
//#include <iostream>

// C library
//
#include <ifaddrs.h>
#include <netdb.h>



namespace addr
{


namespace
{


/** \brief Delete an addrinfo structure.
 *
 * This deleter is used to make sure all the addinfo get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ai  The addrinfo structure to free.
 */
void addrinfo_deleter(struct addrinfo * ai)
{
    freeaddrinfo(ai);
}


}





/** \brief Set the default IP addresses.
 *
 * This function sets the default IP addresses to be used by the parser
 * when the input string of the parse() function does not include an IP
 * address.
 *
 * The function expects either an IPv4 or an IPv6 address. It can be
 * called twice if you need to define both types of addresses (which
 * is often a good idea.)
 *
 * For example, the following input is considered valid when a default
 * address is defined:
 *
 * \code
 *      parser.parse(":123");
 * \endcode
 *
 * It returns the default address and port 123. Note that by default
 * an address is mandatory unless a default address is defined.
 *
 * To prevent the parser from working when no default and no address
 * are specified, then make sure to set the REQUIRED_ADDRESS allow
 * flag to true:
 *
 * \code
 *      parser.set_allow(parser.flag_t::REQUIRED_ADDRESS, true);
 *      // now address is mandatory
 * \endcode
 *
 * To completely prevent the use of an address in an input string, set
 * the `ADDRESS` and `REQUIRED_ADDRESS` values to false:
 *
 * \code
 *      parser.set_allow(parser.flag_t::ADDRESS,          false);
 *      parser.set_allow(parser.flag_t::REQUIRED_ADDRESS, false);
 * \endcode
 *
 * To remove both default IP addresses, call this function with an empty
 * string:
 *
 * \code
 *      parser.set_default_address(std::string());
 * \endcode
 *
 * \param[in] addr  The new address.
 */
void addr_parser::set_default_address(std::string const & addr)
{
    if(addr.empty())
    {
        f_default_address4.clear();
        f_default_address6.clear();
    }
    else if(addr[0] == '[')
    {
        // remove the '[' and ']'
        //
        if(addr.back() != ']')
        {
            throw addr_invalid_argument_exception("an IPv6 address starting with '[' must end with ']'.");
        }
        f_default_address6 = addr.substr(1, addr.length() - 2);
    }
    else if(addr.find(':') != std::string::npos)
    {
        f_default_address6 = addr;
    }
    else
    {
        f_default_address4 = addr;
    }
}


/** \brief Retrieve the default IP address for IPv4 parsing.
 *
 * This function returns a copy of the default IP address used by
 * the parser when the input string does not include an IP address.
 *
 * If the function returns an empty string, then no default address
 * is defined.
 *
 * \return The default IPv4 address.
 *
 * \sa get_default_address6()
 * \sa set_default_address()
 */
std::string const & addr_parser::get_default_address4() const
{
    return f_default_address4;
}


/** \brief Retrieve the default IP address for IPv4 parsing.
 *
 * This function returns a copy of the default IP address used by
 * the parser when the input string does not include an IP address.
 *
 * If the function returns an empty string, then no default address
 * is defined.
 *
 * \return The default IPv6 address, without square brackets.
 *
 * \sa get_default_address4()
 * \sa set_default_address()
 */
std::string const & addr_parser::get_default_address6() const
{
    return f_default_address6;
}


/** \brief Define the default port.
 *
 * This function is used to define the default port to use in the address
 * parser object. By default this is set to -1 meaning: no default port.
 *
 * This function accepts any port number from 0 to 65535. It also accepts
 * -1 to reset the port back to "no default".
 *
 * To prevent the parser from working when no default and no port
 * are specified, then make sure to set the REQUIRED_PORT allow
 * flag to true:
 *
 * \code
 *      parser.set_allow(parser.flag_t::REQUIRED_PORT, true);
 *      // now port is mandatory
 * \endcode
 *
 * To completely prevent the use of a port in an input string, set
 * the `PORT` and `REQUIRED_PORT` values to false:
 *
 * \code
 *      parser.set_allow(parser.flag_t::PORT,          false);
 *      parser.set_allow(parser.flag_t::REQUIRED_PORT, false);
 * \endcode
 *
 * \exception addr_invalid_argument_exception
 * If the port number is out of range, then this expcetion is raised.
 * The allowed range for a port is 0 to 65535. This function also
 * accepts -1 meaning that no default port is specified.
 *
 * \param[in] port  The new default port.
 */
void addr_parser::set_default_port(int const port)
{
    if(port < -1
    || port > 65535)
    {
        throw addr_invalid_argument_exception("addr_parser::set_default_port(): port must be in range [-1..65535].");
    }

    f_default_port = port;
}


/** \brief Retrieve the default port.
 *
 * This function retrieves the default port as defined by the
 * set_default_port() function.
 */
int addr_parser::get_default_port() const
{
    return f_default_port;
}


/** \brief Define the default mask.
 *
 * This function is used to define the default mask. Note that the
 * default mask will not be used at all if the flag_t::MASK allow
 * flag is not set to true:
 *
 * \code
 *      parser.set_allow(parser.flag_t::MASK, true);
 *      parser.set_default_mask("255.255.0.0");
 *      parser.set_default_mask("[ffff:ffff:ffff::]");
 * \endcode
 *
 * The IPv6 mask does not require the square brackets (`'['` and `']'`).
 *
 * To remove the default mask, call this function with an empty
 * string:
 *
 * \code
 *      parser.set_default_mask(std::string());
 * \endcode
 *
 * \note
 * As you can see, here we expect the mask to be a string. This is because
 * it gets parsed as if it came from the input string of the parser. This
 * also means that if the mask is invalid, it will not be detected until
 * you attempt to parse an input string that does not include a mask and
 * the default gets used.
 *
 * \todo
 * Add a check of the default mask when it gets set so we can throw on
 * errors and that way it is much more likely that programmers can fix
 * their errors early.
 *
 * \param[in] mask  The mask to use by default.
 */
void addr_parser::set_default_mask(std::string const & mask)
{
    if(mask.empty())
    {
        f_default_mask4.clear();
        f_default_mask6.clear();
    }
    else if(mask[0] == '[')
    {
        // remove the '[' and ']'
        //
        if(mask.back() != ']')
        {
            throw addr_invalid_argument_exception("an IPv6 mask starting with '[' must end with ']'.");
        }
        f_default_mask6 = mask.substr(1, mask.length() - 2);
    }
    else if(mask.find(':') != std::string::npos)
    {
        f_default_mask6 = mask;
    }
    else
    {
        f_default_mask4 = mask;
    }
}


/** \brief Retrieve the default mask.
 *
 * This function returns a reference to the mask as set by the
 * set_default_mask() function. The value is an empty string by
 * default.
 *
 * The default mask will be used if no mask is specified in the
 * input string to the parse() function. When no default mask
 * is defined, the mask is set to all 1s.
 *
 * \note
 * The default mask is a string, not a binary mask. It gets
 * converted by the parser at the time it is required.
 *
 * \return The default mask.
 *
 * \sa get_default_mask6()
 * \sa set_default_mask()
 */
std::string const & addr_parser::get_default_mask4() const
{
    return f_default_mask4;
}


/** \brief Retrieve the default mask.
 *
 * This function returns a reference to the mask as set by the
 * set_default_mask() function. The value is an empty string by
 * default.
 *
 * The default mask will be used if no mask is specified in the
 * input string to the parse() function. When no default mask
 * is defined, the mask is set to all 1s.
 *
 * \note
 * The default mask is a string, not a binary mask. It gets
 * converted by the parser at the time it is required.
 *
 * \return The default mask.
 *
 * \sa get_default_mask4()
 * \sa set_default_mask()
 */
std::string const & addr_parser::get_default_mask6() const
{
    return f_default_mask6;
}


/** \brief Set the protocol to use to filter addresses.
 *
 * This function sets the protocol as one of the following:
 *
 * \li "ip" -- only return IP address supporting the IP protocol
 * (this is offered because getaddrinfo() may return such IP addresses.)
 * \li "tcp" -- only return IP address supporting TCP
 * \li "udp" -- only return IP address supporting UDP
 *
 * Any other value is refused. To reset the protocol to the default,
 * which is "do not filter by protocol", call the clear_protocol().
 *
 * \exception addr_invalid_argument_exception
 * If the string passed to this function is not one of the acceptable
 * protocols (ip, tcp, udp), then this exception is raised.
 *
 * \param[in] protocol  The default protocol for this parser.
 *
 * \sa clear_protocol()
 * \sa get_protocol()
 */
void addr_parser::set_protocol(std::string const & protocol)
{
    if(protocol == "ip")
    {
        f_protocol = IPPROTO_IP;
    }
    else if(protocol == "tcp")
    {
        f_protocol = IPPROTO_TCP;
    }
    else if(protocol == "udp")
    {
        f_protocol = IPPROTO_UDP;
    }
    else
    {
        // not a protocol we support
        //
        throw addr_invalid_argument_exception(
                  std::string("unknown protocol \"")
                + protocol
                + "\", expected \"tcp\" or \"udp\".");
    }
}


/** \brief Set the protocol to use to filter addresses.
 *
 * This function sets the protocol as one of the following:
 *
 * \li IPPROTO_IP -- only return IP address supporting the IP protocol
 * (this is offered because getaddrinfo() may return such IP addresses.)
 * \li IPPROTO_TCP -- only return IP address supporting TCP
 * \li IPPROTO_UDP -- only return IP address supporting UDP
 *
 * Any other value is refused. To reset the protocol to the default,
 * which is "do not filter by protocol", call the clear_protocol().
 *
 * \exception addr_invalid_argument_exception
 * If the string passed to this function is not one of the acceptable
 * protocols (ip, tcp, udp), then this exception is raised.
 *
 * \param[in] protocol  The default protocol for this parser.
 *
 * \sa clear_protocol()
 * \sa get_protocol()
 */
void addr_parser::set_protocol(int const protocol)
{
    // make sure that's a protocol we support
    //
    switch(protocol)
    {
    case IPPROTO_IP:
    case IPPROTO_TCP:
    case IPPROTO_UDP:
        break;

    default:
        throw addr_invalid_argument_exception(
                  std::string("unknown protocol \"")
                + std::to_string(protocol)
                + "\", expected \"tcp\" or \"udp\".");

    }

    f_protocol = protocol;
}


/** \brief Use this function to reset the protocol back to "no default."
 *
 * This function sets the protocol to -1 (which is something you cannot
 * do by callingt he set_protocol() functions above.)
 *
 * The -1 special value means that the protocol is not defined, that
 * there is no default. In most cases this means all the addresses
 * that match, ignoring the protocol, will be returned by the parse()
 * function.
 *
 * \sa set_protocol()
 * \sa get_protocol()
 */
void addr_parser::clear_protocol()
{
    f_protocol = -1;
}


/** \brief Retrieve the protocol as defined by the set_protocol().
 *
 * This function returns the protocol number as defined by the
 * set_protocol.
 *
 * When defined, the protocol is used whenever we call the
 * getaddrinfo() function. In general, this means the IP addresses
 * returned will have  to match that protocol.
 *
 * This function may return -1. The value -1 is used as "do not
 * filter by protocol". The protocol can be set to -1 by calling
 * the clear_protocol() function.
 *
 * \return The parser default protocol.
 *
 * \sa set_protocol()
 * \sa clear_protocol()
 */
int addr_parser::get_protocol() const
{
    return f_protocol;
}


/** \brief Set or clear allow flags in the parser.
 *
 * This parser has a set of flags it uses to know whether the input
 * string can include certain things such as a port or a mask.
 *
 * This function is used to allow or require certain parameters and
 * to disallow others.
 *
 * By default, the ADDRESS and PORT flags are set, meaning that an
 * address and a port can appear, but either or both are optinal.
 * If unspecified, then the default will be used. If not default
 * is defined, then the parser may fail in this situation.
 *
 * One problem is that we include contradictory syntatical features.
 * The parser supports lists of addresses separated by commas and
 * lists of ports separated by commas. Both are not supported
 * simultaneously. This means you want to allow multiple addresses
 * separated by commas, the function makes sure that the multiple
 * port separated by commas support is turned of.
 *
 * \li ADDRESS -- the IP address is allowed, but optional
 * \li REQUIRED_ADDRESS -- the IP address is mandatory
 * \li PORT -- the port is allowed, but optional
 * \li REQUIRED_PORT -- the port is mandatory
 * \li MASK -- the mask is allowed, but optional
 * \li MULTI_ADDRESSES_COMMAS -- the input can have multiple addresses
 * separated by commas, spaces are not allowed (prevents MULTI_PORTS_COMMAS)
 * \li MULTI_ADDRESSES_SPACES -- the input can have multiple addresses
 * separated by spaces
 * \li MULTI_ADDRESSES_COMMAS_AND_SPACES -- the input can have multiple
 * addresses separated by spaces and commas (prevents MULTI_PORTS_COMMAS)
 * \li MULTI_PORTS_SEMICOLONS -- the input can  have multiple ports
 * separated by semicolons _NOT IMPLEMENTED YET_
 * \li MULTI_PORTS_COMMAS -- the input can have multiple ports separated
 * by commas (prevents MULTI_ADDRESSES_COMMAS and
 * MULTI_ADDRESSES_COMMAS_AND_SPACES) _NOT IMPLEMENTED YET_
 * \li PORT_RANGE -- the input supports port ranges (p1-p2) _NOT
 * IMPLEMENTED YET_
 * \li ADDRESS_RANGE -- the input supports address ranges (addr-addr) _NOT
 * IMPLEMENTED YET_
 *
 * \param[in] flag  The flag to set or clear.
 * \param[in] allow  Whether to allow (true) or disallow (false).
 *
 * \sa get_allow()
 */
void addr_parser::set_allow(flag_t const flag, bool const allow)
{
    if(flag < static_cast<flag_t>(0)
    || flag >= flag_t::FLAG_max)
    {
        throw addr_invalid_argument_exception("addr_parser::set_allow(): flag has to be one of the valid flags.");
    }

    f_flags[static_cast<int>(flag)] = allow;

    // if we just set a certain flag, others may need to go to false
    //
    if(allow)
    {
        // we can only support one type of commas
        //
        switch(flag)
        {
        case flag_t::MULTI_ADDRESSES_COMMAS:
        case flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES:
            f_flags[static_cast<int>(flag_t::MULTI_PORTS_COMMAS)] = false;
            break;

        case flag_t::MULTI_PORTS_COMMAS:
            f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS)] = false;
            f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES)] = false;
            break;

        default:
            break;

        }
    }
}


/** \brief Retrieve the current statius of an allow flag.
 *
 * This function returns the current status of the allow flags.
 *
 * By default, the `ADDRESS` and `PORT` flags are set to true.
 * All the other flags are set to false.
 *
 * You may change the value of an allow flag by calling the
 * set_allow() function.
 *
 * \param[in] flag  Which flag is to be checked.
 *
 * \return The value of the flag: true or false.
 *
 * \sa set_allow()
 */
bool addr_parser::get_allow(flag_t const flag) const
{
    if(flag < static_cast<flag_t>(0)
    || flag >= flag_t::FLAG_max)
    {
        throw addr_invalid_argument_exception("addr_parser::get_allow(): flag has to be one of the valid flags.");
    }

    return f_flags[static_cast<int>(flag)];
}


/** \brief Check whether errors were registered so far.
 *
 * This function returns true if the system detected errors in one
 * of the previous calls to parse(). The flag can be cleared using
 * the clear_errors() function.
 *
 * On construction and after a call to clear_error(), this flag is
 * always false. If you are to call parser() multiple times with
 * the same addr_parser object, then you want to make sure to call
 * the clear_errors() function before calling the parse() function.
 * Otherwise you won't know whether errors occurred in a earlier
 * or later call.
 *
 * \code
 *      // first time, not required
 *      parser.parse(...);
 *      ...
 *
 *      // next time, required
 *      parser.clear_errors();
 *      parser.parse(...);
 *      ...
 * \endcode
 *
 * \return true if errors were generated.
 */
bool addr_parser::has_errors() const
{
    return !f_error.empty();
}


/** \brief Emit an error and save it in this class.
 *
 * This function adds the message to the error string part of this
 * object. A newline is also added at the end of the message.
 *
 * Next the function increments the error counter.
 *
 * \note
 * You are expected to emit one error at a time. If you want to
 * emit several messages in a row, that will work and properly
 * count each message.
 *
 * \param[in] msg  The message to add to the parser error messages.
 *
 * \sa error_messages()
 */
void addr_parser::emit_error(std::string const & msg)
{
    f_error += msg;
    f_error += "\n";
    ++f_error_count;
}


/** \brief Return the current error messages.
 *
 * The error messages are added to the addr_parser using the
 * emit_error() function.
 *
 * This function does not clear the list of error messages.
 * To do that, call the clear_errors() function.
 *
 * The number of messages can be determined by counting the
 * number of "\n" characters in the string. The error_count()
 * will return that same number (assuming no message included
 * a '\n' character when emit_error() was called.)
 *
 * \return A string with the list of messages.
 *
 * \sa emit_error()
 * \sa clear_errors()
 */
std::string const & addr_parser::error_messages() const
{
    return f_error;
}


/** \brief Return the number of error messages that were emitted.
 *
 * Each time the emit_error() function is called, the error
 * counter is incremented by 1. This function returns that
 * error counter.
 *
 * The clear_errors() function can be used to clear the
 * counter back to zero.
 *
 * \return The number of errors that were emitted so far.
 *
 * \sa emit_error()
 */
int addr_parser::error_count() const
{
    return f_error_count;
}


/** \brief Clear the error message and error counter.
 *
 * This function clears all the error messages and reset the
 * counter back to zero. In order words, it will be possible
 * to tell how many times the emit_error() was called since
 * the start or the last clear_errors() call.
 *
 * To retrieve a copy of the error counter, use the error_count()
 * function.
 *
 * \sa error_count()
 */
void addr_parser::clear_errors()
{
    f_error.clear();
    f_error_count = 0;
}


/** \brief Parse a string of addresses, ports, and masks.
 *
 * This function is used to parse the list of addresses defined
 * in the \p in parameter.
 *
 * One address is composed of one to three elements:
 *
 * \code
 *          [ address ] [ ':' port ] [ '/' mask ]
 * \endcode
 *
 * Although all three elements are optional (at least by default),
 * a valid address is expected to include at least one of the
 * three elements. (i.e. an empty string is just skipped silently.)
 *
 * ### Multiple Addresses
 *
 * Multiple addresses can be defined if at least one of the
 * `MULTI_ADDRESSES_COMMAS`, `MULTI_ADDRESSES_SPACES`, or
 * `MULTI_ADDRESSES_COMMAS_AND_SPACES` allow flags is set to true.
 *
 * Note that the `MULTI_ADDRESSES_COMMAS_AND_SPACES` has priotity. If
 * set to true, then both, commas and spaces are allowed between
 * addresses.
 *
 * Next comes `MULTI_ADDRESSES_COMMAS`: if set to true, addresses
 * must be separated by commas and spaces are not allowed.
 *
 * Finally we have `MULTI_ADDRESSES_SPACES`. If that one is true, then
 * addresses must be separated by spaces and commas are not allowed.
 *
 * ### Make Address Field Required
 *
 * To make the address field a required field, set the
 * `REQUIRED_ADDRESS` flag (see set_allow()) to true and do not define a
 * default address (see set_default_address()).
 *
 * ### Make Port Field Required
 *
 * To make the port field a required fiel, set the `REQUIRED_PORT`
 * flag (see set_allow()) to true and do not define a default port
 * (see set_default_port()).
 *
 * ### Allow Mask
 *
 * The mask cannot be made mandatory. However, you have to set
 * the `MASK` flag to true to allow it. By default it is not
 * allowed.
 *
 * ### Ranges
 *
 * At this time we do not need support for ranges so it did not yet
 * get implemented.
 *
 * \param[in] in  The input string to be parsed.
 */
addr_range::vector_t addr_parser::parse(std::string const & in)
{
    addr_range::vector_t result;

    if(f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES)])
    {
        std::string comma_space(", ");
        std::string::size_type s(0);
        while(s < in.length())
        {
            // since C++11 we have a way to search for a set of character
            // in a string with an algorithm!
            //
            auto const it(std::find_first_of(in.begin() + s, in.end(), comma_space.begin(), comma_space.end()));
            std::string::size_type const e(it == in.end() ? in.length() : it - in.begin());
            if(e > s)
            {
                parse_cidr(in.substr(s, e - s), result);
            }
            s = e + 1;
        }
    }
    else if(f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS)]
         || f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_SPACES)])
    {
        char sep(f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS)] ? ',' : ' ');
        std::string::size_type s(0);
        while(s < in.length())
        {
            std::string::size_type e(in.find(sep, s));
            if(e == std::string::npos)
            {
                e = in.length();
            }
            if(e > s)
            {
                parse_cidr(in.substr(s, e - s), result);
            }
            s = e + 1;
        }
    }
    else
    {
        parse_cidr(in, result);
    }

    return result;
}


/** \brief Check one address.
 *
 * This function checks one address, although if it is a name, it could
 * represent multiple IP addresses.
 *
 * This function separate the address:port from the mask if the mask is
 * allowed. Then it parses the address:port part and the mask separately.
 *
 * \param[in] in  The address to parse.
 * \param[in,out] result  The list of resulting addresses.
 */
void addr_parser::parse_cidr(std::string const & in, addr_range::vector_t & result)
{
    if(f_flags[static_cast<int>(flag_t::MASK)])
    {
        // check whether there is a mask
        //
        std::string mask;

        std::string address;
        std::string::size_type const p(in.find('/'));
        if(p != std::string::npos)
        {
            address = in.substr(0, p);
            mask = in.substr(p + 1);
        }
        else
        {
            address = in;
        }

        int const errcnt(f_error_count);

        // handle the address first
        //
        addr_range::vector_t addr_mask;
        parse_address(address, mask, addr_mask);

        // now check for the mask
        //
        for(auto & am : addr_mask)
        {
            std::string m(mask);
            if(m.empty())
            {
                // the mask was not defined in the input, then adapt it to
                // the type of address we got in 'am'
                //
                if(am.get_from().is_ipv4())
                {
                    m = f_default_mask4;
                }
                else
                {
                    // parse_mask() expects '[...]' around IPv6 addresses
                    //
                    m = "[" + f_default_mask6 + "]";
                }
            }

            parse_mask(m, am.get_from());
        }

        // now append the list to the result if no errors occurred
        //
        if(errcnt == f_error_count)
        {
            result.insert(result.end(), addr_mask.begin(), addr_mask.end());
        }
    }
    else
    {
        // no mask allowed, if there is one, this call will fail
        //
        parse_address(in, std::string(), result);
    }
}


/** \brief Parse one address.
 *
 * This function is called with one address. It determines whether we
 * are dealing with an IPv4 or an IPv6 address and call the
 * corresponding sub-function.
 *
 * An address is considered an IPv6 address if it starts with a '['
 * character.
 *
 * \note
 * The input cannot include a mask. It has to already have been
 * removed.
 *
 * \note
 * The mask parameter is only used to determine whether this function
 * is being called with an IPv6 or not. It is otherwise ignored.
 *
 * \param[in] in  The input address eventually including a port.
 * \param[in] mask  The mask used to determine whether we are dealing with
 *                  an IPv6 or not.
 * \param[in,out] result  The list of resulting addresses.
 */
void addr_parser::parse_address(std::string const & in, std::string const & mask, addr_range::vector_t & result)
{
    // With our only supported format, ipv6 addresses must be between square
    // brackets. The address may just be a mask in which case the '[' may
    // not be at the very start (i.e. "/[ffff:ffff::]")
    //
    if(in.empty()
    || in[0] == ':')    // if it start with ':' then there is no address
    {
        // if the address is empty, then use the mask to determine the
        // type of IP address (note: if the address starts with ':'
        // it is considered empty since an IPv6 would have a '[' at
        // the start)
        //
        if(!mask.empty())
        {
            if(mask[0] == '[')
            {
                // IPv6 parsing
                //
                parse_address6(in, result);
            }
            else
            {
                // if the number is 33 or more, it has to be IPv6, otherwise
                // we cannot know...
                //
                int mask_count(0);
                for(char const * s(mask.c_str()); *s != '\0'; ++s)
                {
                    if(*s >= '0' && *s <= '9')
                    {
                        mask_count = mask_count * 10 + *s - '0';
                        if(mask_count > 1000)
                        {
                            // not valid
                            //
                            mask_count = -1;
                            break;;
                        }
                    }
                    else
                    {
                        // not a valid decimal number
                        //
                        mask_count = -1;
                        break;
                    }
                }
                if(mask_count > 32)
                {
                    parse_address6(in, result);
                }
                else
                {
                    parse_address4(in, result);
                }
            }
        }
        else
        {
            if(f_default_address4.empty()
            && !f_default_address6.empty())
            {
                parse_address6(in, result);
            }
            else
            {
                parse_address4(in, result);
            }
        }
    }
    else
    {
        // if an address has a ']' then it is IPv6 even if the '['
        // is missing, that being said, it is still considered
        // invalid as per our processes
        //
        if(in[0] == '['
        || in.find(']') != std::string::npos)
        {
            parse_address6(in, result);
        }
        else
        {
            // if there is no port, then a ':' can be viewed as an IPv6
            // address because there is no other ':', but if there are
            // '.' before the ':' then we assume that it is IPv4 still
            //
            if(!f_flags[static_cast<int>(flag_t::PORT)]
            && !f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
            {
                std::string::size_type const p(in.find(':'));
                if(p != std::string::npos
                && in.find('.') > p)
                {
                    parse_address6(in, result);
                }
                else
                {
                    parse_address4(in, result);
                }
            }
            else
            {
                parse_address4(in, result);
            }
        }
    }
}


/** \brief Parse one IPv4 address.
 *
 * This function checks the input parameter \p in and extracts the
 * address and port. There is a port if the input strings includes
 * a `':'` character.
 *
 * If this function detects that a port is not allowed and yet
 * a `':'` character is found, then it generates an error and
 * returns without adding anything to `result`.
 *
 * \param[in] in  The input string with the address and optional port.
 * \param[in,out] result  The list of resulting addresses.
 */
void addr_parser::parse_address4(std::string const & in, addr_range::vector_t & result)
{
    std::string address(f_default_address4);
    std::string port_str(f_default_port == -1 ? std::string() : std::to_string(f_default_port));

    std::string::size_type const p(in.find(':'));

    if(f_flags[static_cast<int>(flag_t::PORT)]
    || f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
    {
        // the address can include a port
        //
        if(p != std::string::npos)
        {
            // get the address only if not empty (otherwise we want to
            // keep the default)
            //
            if(p > 0)
            {
                address = in.substr(0, p);
            }

            // get the port only if not empty (otherwise we want to
            // keep the default)
            //
            if(p + 1 < in.length())
            {
                port_str = in.substr(p + 1);
            }
        }
        else if(!in.empty())
        {
            address = in;
        }
    }
    else
    {
        if(p != std::string::npos
        && !f_flags[static_cast<int>(flag_t::PORT)]
        && !f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
        {
            emit_error("Port not allowed (" + in + ").");
            return;
        }

        if(!in.empty())
        {
            address = in;
        }
    }

    parse_address_port(address, port_str, result, "0.0.0.0");
}


/** \brief Parse one IPv6 address.
 *
 * This function checks the input parameter \p in and extracts the
 * address and port. There is a port if the input strings includes
 * a `':'` character after the closing square bracket (`']'`).
 *
 * If this function detects that a port is not allowed and yet
 * a `':'` character is found, then it generates an error and
 * returns without adding anything to `result`.
 *
 * \note
 * This function can be called with an IPv6
 *
 * \param[in] in  The input string with the address and optional port.
 * \param[in,out] result  The list of resulting addresses.
 */
void addr_parser::parse_address6(std::string const & in, addr_range::vector_t & result)
{
    std::string::size_type p(0);

    std::string address(f_default_address6);
    std::string port_str(f_default_port == -1 ? std::string() : std::to_string(f_default_port));

    // if there is an address extract it otherwise put the default
    //
    if(!in.empty()
    && in[0] == '[')
    {
        p = in.find(']');

        if(p == std::string::npos)
        {
            emit_error("IPv6 is missing the ']' (" + in + ").");
            return;
        }

        if(p != 1)
        {
            // get the address only if not empty (otherwise we want to
            // keep the default) -- so we actually support "[]" to
            // represent "use the default address if defined".
            //
            address = in.substr(1, p - 1);
        }
    }

    // on entry 'p' is either 0 or the position of the ']' character
    //
    p = in.find(':', p);

    if(p != std::string::npos)
    {
        if(f_flags[static_cast<int>(flag_t::PORT)]
        || f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
        {
            // there is also a port, extract it
            //
            port_str = in.substr(p + 1);
        }
        else if(!f_flags[static_cast<int>(flag_t::PORT)]
             && !f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
        {
            emit_error("Port not allowed (" + in + ").");
            return;
        }
    }

    parse_address_port(address, port_str, result, "::");
}


/** \brief Parse the address and port.
 *
 * This function receives an address and a port string and
 * convert them in an addr object which gets saved in
 * the specified result range vector.
 *
 * The address can be an IPv4 or an IPv6 address.
 *
 * The port may be numeric or a name such as `"http"`.
 *
 * \note
 * This function is not responsible for handling the default address
 * and default port. This is expected to be dealt with by the caller
 * if required.
 *
 * \param[in] address  The address to convert to binary.
 */
void addr_parser::parse_address_port(std::string address, std::string const & port_str, addr_range::vector_t & result, std::string const & default_address)
{
    // make sure the port is good
    //
    if(port_str.empty()
    && f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
    {
        emit_error("Required port is missing.");
        return;
    }

    // make sure the address is good
    //
    if(address.empty())
    {
        if(f_flags[static_cast<int>(flag_t::REQUIRED_ADDRESS)])
        {
            emit_error("Required address is missing.");
            return;
        }
        // internal default if no address was defined
        // (TBD: should it be an IPv6 instead?)
        //
        address = default_address;
    }

    // prepare hints for the the getaddrinfo() function
    //
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED;
    hints.ai_family = AF_UNSPEC;

    switch(f_protocol)
    {
    case IPPROTO_TCP:
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        break;

    case IPPROTO_UDP:
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        break;

    }

    // convert address to binary
    //
    struct addrinfo * addrlist(nullptr);
    {
        errno = 0;
        int const r(getaddrinfo(address.c_str(), port_str.c_str(), &hints, &addrlist));
        if(r != 0)
        {
            // break on invalid addresses
            //
            int const e(errno); // if r == EAI_SYSTEM, then 'errno' is consistent here
            emit_error("Invalid address in \""
                     + address
                     + (port_str.empty() ? "" : ":")
                     + port_str
                     + "\" error "
                     + std::to_string(r)
                     + " -- "
                     + gai_strerror(r)
                     + " (errno: "
                     + std::to_string(e)
                     + " -- "
                     + strerror(e)
                     + ").");
            return;
        }
    }
    std::shared_ptr<struct addrinfo> ai(addrlist, addrinfo_deleter);

    bool first(true);
    while(addrlist != nullptr)
    {
        // go through the addresses and create ranges and save that in the result
        //
        if(addrlist->ai_family == AF_INET)
        {
            if(addrlist->ai_addrlen != sizeof(struct sockaddr_in))
            {
                emit_error("Unsupported address size ("                  // LCOV_EXCL_LINE
                         + std::to_string(addrlist->ai_addrlen)          // LCOV_EXCL_LINE
                         + ", expected"                                  // LCOV_EXCL_LINE
                         + std::to_string(sizeof(struct sockaddr_in))    // LCOV_EXCL_LINE
                         + ").");                                        // LCOV_EXCL_LINE
            }
            else
            {
                addr a(*reinterpret_cast<struct sockaddr_in *>(addrlist->ai_addr));
                // in most cases we do not get a protocol from
                // the getaddrinfo() function...
                a.set_protocol(addrlist->ai_protocol);
                addr_range r;
                r.set_from(a);
                result.push_back(r);
            }
        }
        else if(addrlist->ai_family == AF_INET6)
        {
            if(addrlist->ai_addrlen != sizeof(struct sockaddr_in6))
            {
                emit_error("Unsupported address size ("                  // LCOV_EXCL_LINE
                         + std::to_string(addrlist->ai_addrlen)          // LCOV_EXCL_LINE
                         + ", expected "                                 // LCOV_EXCL_LINE
                         + std::to_string(sizeof(struct sockaddr_in6))   // LCOV_EXCL_LINE
                         + ").");                                        // LCOV_EXCL_LINE
            }
            else
            {
                addr a(*reinterpret_cast<struct sockaddr_in6 *>(addrlist->ai_addr));
                a.set_protocol(addrlist->ai_protocol);
                addr_range r;
                r.set_from(a);
                result.push_back(r);
            }
        }
        else if(first)                                                  // LCOV_EXCL_LINE
        {
            // ignore errors from further addresses
            //
            emit_error("Unsupported address family "                     // LCOV_EXCL_LINE
                     + std::to_string(addrlist->ai_family)               // LCOV_EXCL_LINE
                     + ".");                                             // LCOV_EXCL_LINE
        }

        first = false;

        addrlist = addrlist->ai_next;
    }
}


/** \brief Parse a mask.
 *
 * If the input string is a decimal number, then use that as the
 * number of bits to clear.
 *
 * If the mask is not just one decimal number, try to convert it
 * as an address.
 *
 * If the string is neither a decimal number nor a valid IP address
 * then the parser adds an error string to the f_error variable.
 *
 * \param[in] mask  The mask to transform to binary.
 * \param[in,out] cidr  The address to which the mask will be added.
 */
void addr_parser::parse_mask(std::string const & mask, addr & cidr)
{
    // no mask?
    //
    if(mask.empty())
    {
        // in the current implementation this cannot happen since we
        // do not call this function when mask is empty
        //
        // hwoever, the algorithm below expect that 'mask' is not
        // empty (otherwise we get the case of 0 even though it
        // may not be correct.)
        //
        return;  // LCOV_EXCL_LINE
    }

    // the mask may be a decimal number or an address, if just one number
    // then it's not an address, so test that first
    //
    uint8_t mask_bits[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

    // convert the mask to an integer, if possible
    //
    int mask_count(0);
    {
        for(char const * s(mask.c_str()); *s != '\0'; ++s)
        {
            if(*s >= '0' && *s <= '9')
            {
                mask_count = mask_count * 10 + *s - '0';
                if(mask_count > 1000)
                {
                    emit_error("Mask number too large ("
                             + mask
                             + ", expected a maximum of 128).");
                    return;
                }
            }
            else
            {
                mask_count = -1;
                break;
            }
        }
    }

    // the conversion to an integer worked if mask_count != -1
    //
    if(mask_count != -1)
    {
        if(cidr.is_ipv4())
        {
            if(mask_count > 32)
            {
                emit_error("Unsupported mask size ("
                         + std::to_string(mask_count)
                         + ", expected 32 at the most for an IPv4).");
                return;
            }
            mask_count = 32 - mask_count;
        }
        else
        {
            if(mask_count > 128)
            {
                emit_error("Unsupported mask size ("
                         + std::to_string(mask_count)
                         + ", expected 128 at the most for an IPv6).");
                return;
            }
            mask_count = 128 - mask_count;
        }

        // clear a few bits at the bottom of mask_bits
        //
        int idx(15);
        for(; mask_count > 8; mask_count -= 8, --idx)
        {
            mask_bits[idx] = 0;
        }
        mask_bits[idx] = 255 << mask_count;
    }
    else //if(mask_count < 0)
    {
        // prepare hints for the the getaddrinfo() function
        //
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED;
        hints.ai_family = AF_UNSPEC;

        switch(cidr.get_protocol())
        {
        case IPPROTO_TCP:
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            break;

        case IPPROTO_UDP:
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            break;

        }

        std::string const port_str(std::to_string(cidr.get_port()));

        // if the mask is an IPv6, then it has to have the '[...]'
        std::string m(mask);
        if(cidr.is_ipv4())
        {
            if(mask[0] == '[')
            {
                emit_error("The address uses the IPv4 syntax, the mask cannot use IPv6.");
                return;
            }
        }
        else //if(!cidr.is_ipv4())
        {
            if(mask[0] != '[')
            {
                emit_error("The address uses the IPv6 syntax, the mask cannot use IPv4.");
                return;
            }
            if(mask.back() != ']')
            {
                emit_error("The IPv6 mask is missing the ']' (" + mask + ").");
                return;
            }

            // note that we know that mask.length() >= 2 here since
            // we at least have a '[' and ']'
            //
            m = mask.substr(1, mask.length() - 2);
            if(m.empty())
            {
                // an empty mask is valid, it just means keep the default
                // (getaddrinfo() fails on an empty string)
                //
                return;
            }
        }

        // if negative, we may have a full address here, so call the
        // getaddrinfo() on this other string
        //
        struct addrinfo * masklist(nullptr);
        errno = 0;
        int const r(getaddrinfo(m.c_str(), port_str.c_str(), &hints, &masklist));
        if(r != 0)
        {
            // break on invalid addresses
            //
            int const e(errno); // if r == EAI_SYSTEM, then 'errno' is consistent here
            emit_error("Invalid mask in \"/"
                     + mask
                     + "\", error "
                     + std::to_string(r)
                     + " -- "
                     + gai_strerror(r)
                     + " (errno: "
                     + std::to_string(e)
                     + " -- "
                     + strerror(e)
                     + ").");
            return;
        }
        std::shared_ptr<struct addrinfo> mask_ai(masklist, addrinfo_deleter);

        if(cidr.is_ipv4())
        {
            if(masklist->ai_family != AF_INET)
            {
                // this one happens when the user does not put the '[...]'
                // around an IPv6 address
                //
                emit_error("Incompatible address between the address and"
                          " mask address (first was an IPv4 second an IPv6).");
                return;
            }
            if(masklist->ai_addrlen != sizeof(struct sockaddr_in))
            {
                emit_error("Unsupported address size ("                 // LCOV_EXCL_LINE
                        + std::to_string(masklist->ai_addrlen)          // LCOV_EXCL_LINE
                        + ", expected"                                  // LCOV_EXCL_LINE
                        + std::to_string(sizeof(struct sockaddr_in))    // LCOV_EXCL_LINE
                        + ").");                                        // LCOV_EXCL_LINE
                return;                                                 // LCOV_EXCL_LINE
            }
            memcpy(mask_bits + 12, &reinterpret_cast<struct sockaddr_in *>(masklist->ai_addr)->sin_addr.s_addr, 4); // last 4 bytes are the IPv4 address, keep the rest as 1s
        }
        else //if(!cidr.is_ipv4())
        {
            if(masklist->ai_family != AF_INET6)
            {
                // this one happens if the user puts the '[...]'
                // around an IPv4 address
                //
                emit_error("Incompatible address between the address"
                          " and mask address (first was an IPv6 second an IPv4).");
                return;
            }
            if(masklist->ai_addrlen != sizeof(struct sockaddr_in6))
            {
                emit_error("Unsupported address size ("                 // LCOV_EXCL_LINE
                         + std::to_string(masklist->ai_addrlen)         // LCOV_EXCL_LINE
                         + ", expected "                                // LCOV_EXCL_LINE
                         + std::to_string(sizeof(struct sockaddr_in6))  // LCOV_EXCL_LINE
                         + ").");                                       // LCOV_EXCL_LINE
                return;                                                 // LCOV_EXCL_LINE
            }
            memcpy(mask_bits, &reinterpret_cast<struct sockaddr_in6 *>(masklist->ai_addr)->sin6_addr.s6_addr, 16);
        }
    }

    cidr.set_mask(mask_bits);
}




}
// addr namespace
// vim: ts=4 sw=4 et
