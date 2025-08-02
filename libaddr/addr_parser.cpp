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


/** \file
 * \brief The implementation of the IP address parser.
 *
 * This function is used to parse IP addresses from a string to a
 * vector of ranges.
 *
 * The type of addresses support is really wide:
 *
 * * \<domain name> -- if allowed to do a lookup
 * * \<ipv4> -- an IPv4 with syntax x.x.x.x
 * * \<ipv6> -- an IPv6 with syntax x:x:x:...:x
 * * \<port> -- a decimal number from 0 to 65535
 * * \<mask> -- a number from 0 to 128 or an \<ipv4> or an \<ipv6>
 * * \<ip>-\<ip> -- a range of \<ipv4> or \<ipv6> addresses
 *
 * The port is separated from the address by a colon (:). For IPv6, this means
 * the IPv6 address itself must be defined between square brackets as in
 * `[x:x:...:x]`. The square brackets are not required if the port is not
 * allowed.
 *
 * The `<mask>` appears after a slash (/). It is expected to be a number
 * from 0 to 128 (0 to 32 for IPv4 addresses). It can be written as an
 * address only if the ALLOW_ADDRESS_MASK flag is set to true.
 *
 * \code
 * start: ips
 *
 * ips: domain port mask
 *    | port mask
 *    | ip port mask
 *    | ip '-' ip port mask
 *    | ip '-' port mask
 *    | '-' ip port mask
 *
 * ip: '[' ipv6 ']'
 *   | ipv6
 *   | ipv4
 *
 * ipv4: number '.' number '.' number '.' number
 *
 * ipv6: hex
 *     | ':'
 *     | ipv6 ipv6
 *
 * port: <empty>
 *     | ':' number
 *
 * mask: <empty>
 *     | '/' number
 *     | '/' ip
 *
 * number: number number
 *       | '0' | '1' | '2' | ... | '9'
 *
 * hex: hex hex
 *    | number
 *    | 'a' | 'b' | ... | 'f'
 *    | 'A' | 'B' | ... | 'F'
 *
 * domain: domain domain
 *       | number
 *       | 'a' | 'b' | ... | 'z'
 *       | 'A' | 'B' | ... | 'Z'
 *       | '.' | '-'
 *       | UTF8_CHARACTER (most domain systems do not support all UTF-8)
 * \endcode
 */

// self
//
#include    "libaddr/addr_parser.h"
#include    "libaddr/exception.h"


// advgetopt
//
#include    <advgetopt/validator_integer.h>


// snapdev
//
#include    <snapdev/trim_string.h>


// C++
//
#include    <algorithm>
#include    <iostream>


// C
//
#include    <ifaddrs.h>
#include    <netdb.h>


// last include
//
#include    <snapdev/poison.h>



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
void addrinfo_deleter(addrinfo * ai)
{
    freeaddrinfo(ai);
}


}





/** \brief Initialize an addr_parser object.
 *
 * This function initializes the addr_parser object.
 *
 * Especially, it calls the set_allow() functions a few times to set
 * flags which are expected to be true on initialization.
 */
addr_parser::addr_parser()
{
    // allow addresses & DNS lookups by default
    //
    set_allow(allow_t::ALLOW_ADDRESS, true);
    set_allow(allow_t::ALLOW_ADDRESS_LOOKUP, true);

    // allow port after address
    //
    set_allow(allow_t::ALLOW_PORT, true);
}


/** \brief Set the default IP addresses.
 *
 * This function sets the default IP addresses to be used by the parser
 * when the input string of the parse() function does not include an IP
 * address.
 *
 * The \p address parameter cannot include a port. See
 * set_default_port() as a way to change the default port.
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
 *      parser.set_allow(parser.allow_t::ALLOW_REQUIRED_ADDRESS, true);
 *      // now address is mandatory
 * \endcode
 *
 * To completely prevent the use of an address in an input string, set
 * the `ADDRESS` and `REQUIRED_ADDRESS` values to false:
 *
 * \code
 *      parser.set_allow(parser.allow_t::ALLOW_ADDRESS,          false);
 *      parser.set_allow(parser.allow_t::ALLOW_REQUIRED_ADDRESS, false);
 * \endcode
 *
 * To remove both default IP addresses, call this function with an empty
 * string:
 *
 * \code
 *      parser.set_default_address(std::string());
 * \endcode
 *
 * \todo
 * Consider saving the default IPs as addr structures and allow such
 * as input (keep in mind that the default could also represent multiple
 * addresses).
 *
 * \param[in] address  The new address.
 */
void addr_parser::set_default_address(std::string const & address)
{
    if(address.empty())
    {
        f_default_address4.clear();
        f_default_address6.clear();
    }
    else if(address[0] == '[')
    {
        // remove the '[' and ']'
        //
        if(address.back() != ']')
        {
            throw addr_invalid_argument("an IPv6 address starting with '[' must end with ']'.");
        }
        f_default_address6 = address.substr(1, address.length() - 2);
    }
    else if(address.find(':') != std::string::npos)
    {
        f_default_address6 = address;
    }
    else
    {
        f_default_address4 = address;
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


/** \brief Set the default port using a string.
 *
 * By default, you are expected to call the set_default_port() function
 * with an integer. If you do not have a number for the port (which
 * happens quite frequently) we offer a string version. The string is
 * expected to be an exact integer between 0 and 65535 inclusive.
 *
 * The function will also accept -1 and the empty string to reset the
 * default port to its default value (i.e. "no default port").
 *
 * \exception addr_invalid_argument
 * When the input \p port_str is not a valid integer, this exception is
 * raised.
 *
 * \param[in] port_str  The string to convert as a port.
 */
void addr_parser::set_default_port(std::string const & port_str)
{
    std::int64_t port(-1);
    if(!port_str.empty())
    {
        if(!advgetopt::validator_integer::convert_string(port_str, port))
        {
            // TODO: add a lookup for string to port number via /etc/service
            throw addr_invalid_argument(
                      "invalid port in \""
                    + port_str
                    + "\" (no service name lookup allowed).");
        }
    }

    set_default_port(port);
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
 *      parser.set_allow(parser.allow_t::ALLOW_REQUIRED_PORT, true);
 *      // now port is mandatory
 * \endcode
 *
 * To completely prevent the use of a port in an input string, set
 * the `PORT` and `REQUIRED_PORT` values to false:
 *
 * \code
 *      parser.set_allow(parser.allow_t::ALLOW_PORT,          false);
 *      parser.set_allow(parser.allow_t::ALLOW_REQUIRED_PORT, false);
 * \endcode
 *
 * \exception addr_invalid_argument_exception
 * If the port number is out of range, then this exception is raised.
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
        throw addr_invalid_argument("addr_parser::set_default_port(): port must be in range [-1..65535].");
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
 * default mask will not be used at all if the allow_t::ALLOW_MASK allow
 * flag is not set to true:
 *
 * \code
 *      parser.set_allow(parser.allow_t::ALLOW_MASK, true);
 *      parser.set_default_mask("16");  // IPv4 is 0 to 32
 *      parser.set_default_mask("48");  // IPv6 is 0 to 128
 * \endcode
 *
 * If you want to allow the old syntax (i.e. the mask as an IP address
 * instead of just a number), make sure to also allow that:
 *
 * \code
 *      parser.set_allow(parser.allow_t::ALLOW_MASK, true);
 *      parser.set_allow(parser.allow_t::ALLOW_ADDRESS_MASK, true);
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
 * \warning
 * This function accepts address like values as the default mask. This
 * generates an error if no mask is defined by the user and you did not
 * do:
 *
 * \code
 *     parser.set_allow(addr::allow_t::ALLOW_ADDRESS_MASK, true);
 * \endcode
 *
 * \exception addr_invalid_argument
 * An IPv6 address that start with a '[' must end with a ']'. If only one
 * of these characters appears in the string, then it is an error and this
 * exception is raised.
 *
 * \todo
 * The mask accepts a simple number from 0 to 128. This function is not
 * capable of understand whether a smaller number (0 to 32) is an IPv4
 * or an IPv6 mask. At the moment, small numbers are viewed as an IPv4
 * mask.
 *
 * \todo
 * Add a check of the default mask when it gets set so we can throw on
 * errors and that way it is much more likely that programmers can fix
 * their errors early. (Actually by pre-parsing we could save it as
 * an addr and allow a `set_default_mask(addr ...)`!)
 *
 * \param[in] mask  The mask to use by default.
 */
void addr_parser::set_default_mask(std::string const & mask)
{
    if(mask.empty())
    {
        f_default_mask4.clear();
        f_default_mask6.clear();
        return;
    }

    bool const front_ipv6(mask.front() == '[');
    bool const back_ipv6(mask.back() == ']');
    if(front_ipv6 && back_ipv6)
    {
        // remove the '[' and ']'
        //
        f_default_mask6 = mask.substr(1, mask.length() - 2);
        return;
    }

    if(front_ipv6 || back_ipv6)
    {
        throw addr_invalid_argument("an IPv6 mask starting with '[' must end with ']' and vice versa.");
    }

    if(mask.find(':') != std::string::npos)
    {
        f_default_mask6 = mask;
        return;
    }

    std::int64_t m(0);
    bool const valid(advgetopt::validator_integer::convert_string(mask, m));
    if(valid)
    {
        if(m < 0 || m > 128)
        {
            throw addr_invalid_argument("a mask number must be between 0 and 128.");
        }
        if(m > 32)
        {
            f_default_mask6 = mask;
            return;
        }
    }

    f_default_mask4 = mask;
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
 * This function sets the protocol. The accepted names are defined in
 * the /etc/protocols file. In most cases, we support "tcp" and
 * "udp". Other transfer protocols may work too, but we have not
 * tested them.
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
    char buf[1024];
    protoent p = {};
    protoent * ptr(&p);
    if(getprotobyname_r(
              protocol.c_str()
            , &p
            , buf
            , sizeof(buf)
            , &ptr) != 0
    || ptr == nullptr)
    {
        throw addr_invalid_argument(
                  "unknown protocol named \""
                + protocol
                + "\", expected \"tcp\" or \"udp\" or another name from /etc/protocols.");
    }
    f_protocol = p.p_proto;
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
        throw addr_invalid_argument(
                  std::string("unknown protocol number \"")
                + std::to_string(protocol)
                + "\", expected \"tcp\" or \"udp\".");

    }

    f_protocol = protocol;
}


/** \brief Use this function to reset the protocol back to "no default."
 *
 * This function sets the protocol to -1 (which is something you cannot
 * do by calling the set_protocol() functions above.)
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


/** \brief Change the set of flags defining the sorting order.
 *
 * The parser, once done parsing all the input, will sort the addresses
 * according to these flags.
 *
 * By default, it does not re-arrange the order in which the addresses
 * were defined.
 *
 * The parser is capable of sorting by IP type (IPv6 or IPv4 first),
 * and simply by IP addresses. It can also merge adjacent or overlapping
 * ranges into a single range.
 *
 * \exception addr_invalid_argument
 * This exception is raised you set SORT_IPV6_FIRST and SORT_IPV4_FIRST
 * at the same time because these flags are mutually exclusive.
 *
 * \param[in] sort  The sort parameters.
 *
 * \sa get_sort_order()
 */
void addr_parser::set_sort_order(sort_t const sort)
{
    if((sort & (SORT_IPV6_FIRST | SORT_IPV4_FIRST)) == (SORT_IPV6_FIRST | SORT_IPV4_FIRST))
    {
        throw addr_invalid_argument("addr_parser::set_sort_order(): flags SORT_IPV6_FIRST and SORT_IPV4_FIRST are mutually exclusive.");
    }

    f_sort = sort;
}


/** \brief Get the flags defining the sort order of the parser.
 *
 * This function returns the sort order of the parser as a set of flags.
 *
 * By default this value is set to NO_SORT meaning that the input is
 * kept as is.
 *
 * \return The sort order flags.
 *
 * \sa set_sort_order()
 */
sort_t addr_parser::get_sort_order() const
{
    return f_sort;
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
 * port separated by commas support is turned off.
 *
 * \li `ADDRESS` -- the IP address is allowed, but optional
 * \li `REQUIRED_ADDRESS` -- the IP address is mandatory
 * \li `PORT` -- the port is allowed, but optional
 * \li `REQUIRED_PORT` -- the port is mandatory
 * \li `MASK` -- the mask is allowed, but optional
 * \li `MULTI_ADDRESSES_COMMAS` -- the input can have multiple addresses
 * separated by commas (prevents MULTI_PORTS_COMMAS)
 * \li `MULTI_ADDRESSES_SPACES` -- the input can have multiple addresses
 * separated by spaces
 * \li `MULTI_ADDRESSES_NEWLINES` -- the input can have multiple addresses
 * separated by newlines (one address:port per line)
 * \li `MULTI_PORTS_SEMICOLONS` -- the input can  have multiple ports
 * separated by semicolons _NOT IMPLEMENTED YET_
 * \li `MULTI_PORTS_COMMAS` -- the input can have multiple ports separated
 * by commas (prevents MULTI_ADDRESSES_COMMAS) _NOT IMPLEMENTED YET_
 * \li `PORT_RANGE` -- the input supports port ranges (p1-p2) _NOT
 * IMPLEMENTED YET_
 * \li `ADDRESS_RANGE` -- the input supports address ranges (addr-addr) _NOT
 * IMPLEMENTED YET_
 *
 * The `MULTI_ADDRESSES_COMMAS`, `MULTI_ADDRESSES_SPACES`, and
 * `MULTI_ADDRESSES_NEWLINES` can be used together in which case any
 * number of both characters are accepted between addresses.
 *
 * Note that the `MULTI_ADDRESSES_COMMAS` and `MULTI_PORTS_COMMAS` are
 * mutually exclusive. The last set_allow() counts as the one you are
 * interested in.
 *
 * \param[in] flag  The flag to set or clear.
 * \param[in] allow  Whether to allow (true) or disallow (false).
 *
 * \sa get_allow()
 */
void addr_parser::set_allow(allow_t const flag, bool const allow)
{
    if(flag < static_cast<allow_t>(0)
    || flag >= allow_t::ALLOW_max)
    {
        throw addr_invalid_argument("addr_parser::set_allow(): flag has to be one of the valid flags.");
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
        case allow_t::ALLOW_MULTI_ADDRESSES_COMMAS:
            f_flags[static_cast<int>(allow_t::ALLOW_MULTI_PORTS_COMMAS)] = false;
            break;

        case allow_t::ALLOW_MULTI_PORTS_COMMAS:
            f_flags[static_cast<int>(allow_t::ALLOW_MULTI_ADDRESSES_COMMAS)] = false;
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
bool addr_parser::get_allow(allow_t const flag) const
{
    if(flag < static_cast<allow_t>(0)
    || flag >= allow_t::ALLOW_max)
    {
        throw addr_invalid_argument("addr_parser::get_allow(): flag has to be one of the valid flags.");
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
 * `MULTI_ADDRESSES_NEWLINES` allow flags is set to true.
 *
 * The separator characters are limited to what is allowed. If all three
 * flags are set, then all the characters are allowed and are viewed as
 * valid address separators.
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
 * Ranges are not yet implemented.
 *
 * ### Sort
 *
 * After the function parsed all the input, it sorts the results in
 * the vector of ranges. Ranges are sorted using the addr_range::compare()
 * function. Sorting can also be used to merge ranges. So if two ranges
 * have an overlap or are adjacent, the union of those two ranges will
 * be kept and the two ranges are otherwise removed from the result.
 *
 * The sort is particularly useful if you first want to connect with
 * IPv6 addresses instead of IPv4 (which is the current expected behavior
 * of your services and tools).
 *
 * \todo
 * The ALLOW_COMMENT_HASH and ALLOW_COMMENT_SEMICOLON currently gives the
 * user a way to comment with any type of separator (commas, spaces or new
 * lines). I think that in the following, everything after the # should be
 * viewed as a comment. Right now, the 3rd IP:port is viewed as a valid entry:
 * \todo
 * \code
 *     127.0.0.1,#10.0.0.1,192.168.0.1
 * \endcode
 * \todo
 * So the result is: 127.0.0.1 and 192.168.0.1. Some day, though, that
 * line would be viewed as just 127.0.0.1.
 *
 * \param[in] in  The input string to be parsed.
 *
 * \return A vector of address ranges, see has_errors() to determine whether
 * errors occurred while parsing the input.
 *
 * \sa has_errors()
 */
addr_range::vector_t addr_parser::parse(std::string const & in)
{
    addr_range::vector_t result;
    bool new_lines_allowed(get_allow(allow_t::ALLOW_MULTI_ADDRESSES_NEWLINES));

    std::string separators;
    if(get_allow(allow_t::ALLOW_MULTI_ADDRESSES_COMMAS))
    {
        separators += ',';
    }
    if(get_allow(allow_t::ALLOW_MULTI_ADDRESSES_SPACES))
    {
        separators += ' ';
    }
    if(new_lines_allowed)
    {
        // TBD: consider supporting '\r'?
        //
        separators += '\n';
    }

    std::string const comment_chars(
              std::string(get_allow(allow_t::ALLOW_COMMENT_HASH) ? "#" : "")
                       + (get_allow(allow_t::ALLOW_COMMENT_SEMICOLON) ? ";" : ""));

    if(separators.empty())
    {
        std::string::size_type ec(in.length());
        std::string::size_type s(0);
        while(s < in.length() && isspace(in[s]))
        {
            ++s;
        }
        if(!comment_chars.empty())
        {
            auto const comment(std::find_first_of(in.begin() + s, in.end(), comment_chars.begin(), comment_chars.end()));
            if(comment != in.end())
            {
                ec = comment - in.begin();
            }
        }
        while(ec > 0 && isspace(in[ec - 1]))
        {
            --ec;
        }

        if(s != 0
        || ec != in.length())
        {
            parse_cidr(in.substr(s, ec - s), result);
        }
        else
        {
            parse_cidr(in, result);
        }
    }
    else
    {
        std::string::size_type s(0);
        while(s < in.length())
        {
            auto const it(std::find_first_of(in.begin() + s, in.end(), separators.begin(), separators.end()));
            std::string::size_type e(it - in.begin());
            if(e > s)
            {
                std::string::size_type ec(e);
                if(!comment_chars.empty())
                {
                    auto const comment(std::find_first_of(in.begin() + s, in.begin() + ec, comment_chars.begin(), comment_chars.end()));
                    if(comment != in.begin() + ec)
                    {
                        ec = comment - in.begin();
                    }
                    if(new_lines_allowed
                    && e < in.length()
                    && in[e] != '\n')
                    {
                        auto const nl(std::find(in.begin() + e, in.end(), '\n'));
                        e = nl - in.begin();
                    }
                }

                // ignore lines with just a comment
                //
                if(ec > s)
                {
                    parse_cidr(in.substr(s, ec - s), result);
                }
            }
            s = e + 1;
        }
    }

    // run a normal sort first then attempt a merge if requested
    //
    if((f_sort & (SORT_FULL | SORT_MERGE)) != 0)
    {
        std::stable_sort(result.begin(), result.end());
    }

    if((f_sort & SORT_MERGE) != 0)
    {
        std::size_t max(result.size());
        if(max > 1)
        {
            for(std::size_t idx(0); idx < max - 1; ++idx)
            {
                addr_range const r(result[idx].union_if_possible(result[idx + 1]));
                if(r.is_defined()
                && !r.is_empty())
                {
                    // merge worked, update the vector
                    //
                    result[idx] = r;
                    result.erase(result.begin() + idx + 1);
                    --max;
                    --idx;
                }
            }
        }
    }

    // move IPv4 or IPv6 first (should be IPv6 in newer systems)
    //
    if((f_sort & SORT_IPV4_FIRST) != 0)
    {
        std::stable_sort(
              result.begin()
            , result.end()
            , [](auto const & a, auto const & b)
            {
                switch(a.compare(b))
                {
                case compare_t::COMPARE_IPV4_VS_IPV6:
                case compare_t::COMPARE_FIRST:
                    return true;

                default:
                    return false;

                }
            });
    }
    else if((f_sort & SORT_IPV6_FIRST) != 0)
    {
        std::stable_sort(
              result.begin()
            , result.end()
            , [](auto const & a, auto const & b)
            {
                switch(a.compare(b))
                {
                case compare_t::COMPARE_IPV6_VS_IPV4:
                case compare_t::COMPARE_FIRST:
                    return true;

                default:
                    return false;

                }
            });
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
    std::string address(snapdev::trim_string(in));
    if(get_allow(allow_t::ALLOW_MASK))
    {
        // check whether there is a mask
        //
        std::string mask;

        std::string::size_type const p(address.find('/'));
        if(p != std::string::npos)
        {
            mask = address.substr(p + 1);
            address = address.substr(0, p);
        }

        int const errcnt(f_error_count);

        // handle the address first
        //
        addr_range::vector_t addr_mask;
        bool const is_ipv4(parse_address(address, mask, addr_mask));

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
                if(is_ipv4)
                {
                    m = f_default_mask4;
                }
                else if(!f_default_mask6.empty())
                {
                    // parse_mask() expects '[...]' around IPv6 addresses
                    // we remove them when set_mask() is called
                    //
                    // however, we have to make sure that we do not add
                    // brackets around a simple decimal number (i.e. a
                    // CIDR opposed to an IPv6 address)
                    //
                    if(f_default_mask6.find(':') != std::string::npos)
                    {
                        m = "[" + f_default_mask6 + "]";
                    }
                }
            }

            // TODO: the am.get_from() may be wrong since now we support
            //       ranges so it could require am.get_to() instead
            //
            parse_mask(m, am.get_from(), is_ipv4 && am.get_from().is_ipv4());
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
        parse_address(address, std::string(), result);
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
 *
 * \return true if the address was parsed as an IPv4 address, false if it
 * was determined to be an IPv6 address.
 */
bool addr_parser::parse_address(
      std::string const & in
    , std::string const & mask
    , addr_range::vector_t & result)
{
    // if the number of colons is 2 or more, the address has to be an
    // IPv6 address so we have a very special case at the start for that
    //
    std::ptrdiff_t const colons(std::count(in.begin(), in.end(), ':'));
    if(colons >= 2LL)
    {
        parse_address6(in, colons, result);
        return false;
    }

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
                parse_address6(in, colons, result);
                return false;
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
                    parse_address6(in, colons, result);
                    return false;
                }
                else
                {
                    parse_address4(in, result);
                    return true;
                }
            }
        }
        else
        {
            if(f_default_address4.empty()
            && !f_default_address6.empty())
            {
                parse_address6(in, colons, result);
                return false;
            }
            else
            {
                parse_address4(in, result);
                return true;
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
            parse_address6(in, colons, result);
            return false;
        }
        else
        {
            // if there is no port, then a ':' can be viewed as an IPv6
            // address because there is no other ':', but if there are
            // '.' before the ':' then we assume that it is IPv4 still
            //
            if(!get_allow(allow_t::ALLOW_PORT)
            && !get_allow(allow_t::ALLOW_REQUIRED_PORT))
            {
                std::string::size_type const p(in.find(':'));
                if(p != std::string::npos
                && in.find('.') > p)
                {
                    parse_address6(in, colons, result);
                    return false;
                }
                else
                {
                    parse_address4(in, result);
                    return true;
                }
            }
            else
            {
                parse_address4(in, result);
                return true;
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
    std::string address;
    std::string port_str;

    std::string::size_type const p(in.find(':'));

    if(get_allow(allow_t::ALLOW_PORT)
    || get_allow(allow_t::ALLOW_REQUIRED_PORT))
    {
        // the address can include a port
        //
        if(p != std::string::npos)
        {
            address = in.substr(0, p);
            port_str = in.substr(p + 1);
        }
        else
        {
            address = in;
        }
    }
    else if(p == std::string::npos)
    {
        address = in;
    }
    else
    {
        emit_error("Port not allowed (" + in + ").");
        return;
    }

    parse_address_range_port(address, port_str, result, false);
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
 * This function is expected to be called with an IPv6.
 *
 * \param[in] in  The input string with the address and optional port.
 * \param[in] colons  The number of colons in \p in.
 * \param[in,out] result  The list of resulting addresses.
 */
void addr_parser::parse_address6(std::string const & in, std::size_t const colons, addr_range::vector_t & result)
{
    std::string address;
    std::string port_str;

    // remove the square brackets if present
    //
    if(!in.empty()
    && in[0] == '[')
    {
        std::string::size_type p(in.find(']'));

        if(p == std::string::npos)
        {
            emit_error("IPv6 is missing the ']' (" + in + ").");
            return;
        }

        address = in.substr(1, p - 1);

        ++p;
        if(p < in.length())
        {
            if(in[p] != ':')
            {
                emit_error("The IPv6 address \"" + in + "\" is followed by unknown data.");
                return;
            }

            if(!get_allow(allow_t::ALLOW_PORT)
            && !get_allow(allow_t::ALLOW_REQUIRED_PORT))
            {
                // even just a ':' is no allowed in this case
                //
                emit_error("Port not allowed (" + in + ").");
                return;
            }

            port_str = in.substr(p + 1);
        }
    }
    else if(colons == 1)
    {
        // this usually happens when only a port was specified
        // (so here p == 0 will be true 99% of the time)
        //
        std::string::size_type const p(in.find(':'));
        if(p == std::string::npos)
        {
            throw logic_error("colons == 1 & we did not find it!"); // LCOV_EXCL_LINE
        }
        address = in.substr(0, p);
        port_str = in.substr(p + 1);
    }
    else
    {
        address = in;
    }

    parse_address_range_port(address, port_str, result, true);
}


/** \brief Parse an address range and a port.
 *
 * This function checks whether the address part includes a dash, if so, it
 * is considered an address range and the function transforms it in a "from"
 * and a "to" set of addresses.
 *
 * This function emits an error if the address is just a dash (-). If you
 * want to get the default IP address, use the empty string instead.
 *
 * \note
 * The address range has to be enabled for it to be active. If a range is
 * not allowed, then any '-' is ignored. That also allows you to use domain
 * names that may otherwise include a dash character (i.e. "bad-domain.com").
 *
 * \param[in] addresses  One or two addresses separated by a dash (-).
 * \param[in] port_str  The port as a string.
 * \param[out] result  The vector where the results get saved.
 * \param[in] ipv6  Whether the parser needs to use AF_INET or AF_INET6.
 */
void addr_parser::parse_address_range_port(
      std::string const & addresses
    , std::string const & port_str
    , addr_range::vector_t & result
    , bool ipv6)
{
    std::string::size_type p(std::string::npos);
    if(get_allow(allow_t::ALLOW_ADDRESS_RANGE))
    {
        p = addresses.find('-');
    }
    if(p == std::string::npos)
    {
        parse_address_port(addresses, port_str, result, ipv6);
        return;
    }

    std::string const from(addresses.substr(0, p));
    std::string const to(addresses.substr(p + 1));

    if(from.empty()
    && to.empty())
    {
        emit_error("An address range requires at least one of the \"from\" or \"to\" addresses.");
        return;
    }

    addr_range::vector_t from_result;
    if(!from.empty())
    {
        parse_address_port_ignore_duplicates(from, port_str, from_result, ipv6);
        if(from_result.size() > 1)
        {
            emit_error("The \"from\" of an address range must be exactly one address."); // LCOV_EXCL_LINE
            return; // LCOV_EXCL_LINE
        }
        if(from_result.empty())
        {
            // parse_address_port_ignore_duplicates() failed
            //
            return;
        }
    }

    addr_range::vector_t to_result;
    if(!to.empty())
    {
        // our parse_address_port_ignore_duplicates() function sees the input address as
        // the "from" address; the following moves that result to the
        // "to" address of our own result (assuming the parsing worked
        // as expected)
        //
        parse_address_port_ignore_duplicates(to, port_str, to_result, ipv6);
        if(to_result.size() > 1)
        {
            emit_error("The \"to\" of an address range must be exactly one address."); // LCOV_EXCL_LINE
            return; // LCOV_EXCL_LINE
        }
        if(to_result.empty())
        {
            // parse_address_port_ignore_duplicates() failed
            //
            return;
        }
        to_result[0].swap_from_to();
    }

    if(!from_result.empty()
    && !to_result.empty())
    {
        from_result[0].set_to(to_result[0].get_to());
        if((f_sort & SORT_NO_EMPTY) == 0
        || !from_result[0].is_empty())
        {
            result.push_back(from_result[0]);
        }
    }
    else if(!from_result.empty())
    {
        result.push_back(from_result[0]);
    }
    else //if(!to_result.empty())
    {
        result.push_back(to_result[0]);
    }
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
 * When this function gets called with an empty string as the address or
 * the port, then it makes use of the user defined default unless that
 * default string is also empty in which case it uses a system default.
 *
 * \param[in] address  The address to convert to binary.
 * \param[in] port_str  The port as a string.
 * \param[out] result  The range where we save the results.
 * \param[in] ipv6  Use the default IPv6 address if the address is empty.
 */
void addr_parser::parse_address_port(
      std::string address
    , std::string port_str
    , addr_range::vector_t & result
    , bool ipv6)
{
    // make sure the port is good
    //
    bool const defined_port(!port_str.empty());
    if(!defined_port)
    {
        if(get_allow(allow_t::ALLOW_REQUIRED_PORT))
        {
            emit_error("Required port is missing.");
            return;
        }
        if(f_default_port != -1)
        {
            port_str = std::to_string(f_default_port);
        }
    }

    // make sure the address is good
    //
    if(address.empty())
    {
        if(get_allow(allow_t::ALLOW_REQUIRED_ADDRESS))
        {
            emit_error("Required address is missing.");
            return;
        }
        // internal default if no address was defined
        //
        if(ipv6)
        {
            if(f_default_address6.empty())
            {
                address = "::";
            }
            else
            {
                address = f_default_address6;
            }
        }
        else
        {
            if(f_default_address4.empty())
            {
                address = "0.0.0.0";
            }
            else
            {
                address = f_default_address4;
            }
        }
    }

    // prepare hints for the the getaddrinfo() function
    //
    addrinfo hints = {};
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
    if(get_allow(allow_t::ALLOW_ADDRESS_LOOKUP))
    {
        addrinfo * addrlist(nullptr);
        {
            errno = 0;
            char const * service(port_str.c_str());
            if(port_str.empty())
            {
                service = "0"; // fallback to port 0 when unspecified
            }
            int const r(getaddrinfo(address.c_str(), service, &hints, &addrlist));
            if(r != 0)
            {
                // break on invalid addresses
                //
                int const e(errno); // if r == EAI_SYSTEM, then 'errno' is consistent here
                emit_error(
                          "Invalid address in \""
                        + address
                        + (port_str.empty() ? "" : ":")
                        + port_str
                        + "\" error "
                        + std::to_string(r)
                        + " -- "
                        + gai_strerror(r)
                        + (e == 0
                            ? ""
                            : " (errno: "
                            + std::to_string(e)
                            + " -- "
                            + strerror(e)
                            + ")."));
                return;
            }
        }
        std::shared_ptr<addrinfo> ai(addrlist, addrinfo_deleter);

        bool first(true);
        while(addrlist != nullptr)
        {
            // go through the addresses and create ranges and save that in the result
            //
            if(addrlist->ai_family == AF_INET)
            {
                if(addrlist->ai_addrlen != sizeof(sockaddr_in))
                {
                    emit_error("Unsupported address size ("                  // LCOV_EXCL_LINE
                             + std::to_string(addrlist->ai_addrlen)          // LCOV_EXCL_LINE
                             + ", expected"                                  // LCOV_EXCL_LINE
                             + std::to_string(sizeof(sockaddr_in))           // LCOV_EXCL_LINE
                             + ").");                                        // LCOV_EXCL_LINE
                }
                else
                {
                    addr a(*reinterpret_cast<sockaddr_in *>(addrlist->ai_addr));
                    a.set_hostname(address);
                    // in most cases we do not get a protocol from
                    // the getaddrinfo() function...
                    if(addrlist->ai_protocol != -1)
                    {
                        a.set_protocol(addrlist->ai_protocol);
                    }
                    a.set_port_defined(defined_port);
                    addr_range r;
                    r.set_from(a);
                    result.push_back(r);
                }
            }
            else if(addrlist->ai_family == AF_INET6)
            {
                if(addrlist->ai_addrlen != sizeof(sockaddr_in6))
                {
                    emit_error("Unsupported address size ("                  // LCOV_EXCL_LINE
                             + std::to_string(addrlist->ai_addrlen)          // LCOV_EXCL_LINE
                             + ", expected "                                 // LCOV_EXCL_LINE
                             + std::to_string(sizeof(sockaddr_in6))          // LCOV_EXCL_LINE
                             + ").");                                        // LCOV_EXCL_LINE
                }
                else
                {
                    addr a(*reinterpret_cast<sockaddr_in6 *>(addrlist->ai_addr));
                    a.set_hostname(address);
                    if(addrlist->ai_protocol != -1)
                    {
                        a.set_protocol(addrlist->ai_protocol);
                    }
                    a.set_port_defined(defined_port);
                    addr_range r;
                    r.set_from(a);
                    result.push_back(r);
                }
            }
            else if(first)                                                  // LCOV_EXCL_LINE
            {
                // ignore errors from other unsupported addresses
                //
                first = false;                                              // LCOV_EXCL_LINE

                emit_error("Unsupported address family "                    // LCOV_EXCL_LINE
                         + std::to_string(addrlist->ai_family)              // LCOV_EXCL_LINE
                         + ".");                                            // LCOV_EXCL_LINE
            }

            addrlist = addrlist->ai_next;
        }
    }
    else
    {
        std::int64_t port(0);
        if(get_allow(allow_t::ALLOW_REQUIRED_PORT)
        || !port_str.empty())
        {
            bool const valid_port(advgetopt::validator_integer::convert_string(port_str, port));
            if(!valid_port
            || port < 0
            || port > 65535)
            {
                emit_error("invalid port in \""
                         + port_str
                         + "\" (no service name lookup allowed).");
                return;
            }

            if(!get_allow(allow_t::ALLOW_PORT)
            && !get_allow(allow_t::ALLOW_REQUIRED_PORT))
            {
                // TBD: this is probably a logic error because as far as I
                //      know it can only happen if the programmer defined
                //      a default port and also told the parser that no
                //      port is allowed
                //
                emit_error("Found a port (\""
                         + port_str
                         + "\") when it is not allowed.");
                return;
            }
        }
        sockaddr_in in;
        if(inet_pton(AF_INET, address.c_str(), &in.sin_addr) == 1)
        {
            in.sin_family = AF_INET;
            in.sin_port = htons(port);
            memset(in.sin_zero, 0, sizeof(in.sin_zero)); // probably useless

            addr a(in);
            a.set_hostname(address);
            if(f_protocol != -1)
            {
                a.set_protocol(f_protocol);
            }
            a.set_port_defined(defined_port);
            addr_range r;
            r.set_from(a);
            result.push_back(r);
        }
        else
        {
            sockaddr_in6 in6;
            if(inet_pton(AF_INET6, address.c_str(), &in6.sin6_addr) == 1)
            {
                in6.sin6_family = AF_INET6;
                in6.sin6_port = htons(port);
                in6.sin6_flowinfo = 0;
                in6.sin6_scope_id = 0;

                addr a(in6);
                a.set_hostname(address);
                if(f_protocol != -1)
                {
                    a.set_protocol(f_protocol);
                }
                a.set_port_defined(defined_port);
                addr_range r;
                r.set_from(a);
                result.push_back(r);
            }
            else
            {
                emit_error("Unknown address in \""
                         + address
                         + "\" (no DNS lookup was allowed).");
            }
        }
    }
}


/** \brief Parse the address and port and ignore duplicates.
 *
 * The default system search finds one address per protocol. So if the
 * address matches TCP, UDP, and other protocols, then we get one
 * address for each protocol.
 *
 * This function removes those duplicates which are an issue in a list
 * of IPs when working with ranges (i.e. 192.168.1.1-192.168.1.254 would
 * fail because we find at least a TCP and a UDP protocol for those
 * two addresses).
 *
 * \param[in] address  The address to convert to binary.
 * \param[in] port_str  The port as a string.
 * \param[out] result  The range where we save the results.
 * \param[in] ipv6  Use the default IPv6 address if the address is empty.
 */
void addr_parser::parse_address_port_ignore_duplicates(
      std::string address
    , std::string port_str
    , addr_range::vector_t & result
    , bool ipv6)
{
    parse_address_port(address, port_str, result, ipv6);
    if(result.size() > 1)
    {
        // the following works only on a single set of addresses
        // (i.e. if you called parse_address_port() multiple times,
        // then it will not properly reduce all the equivalent IPs)
        //
        // note that since this is used for ranges, if we return multiple
        // addresses (as in 192.168.1.1 and 192.168.3.1) then the range
        // definition fails anyway, so the fact that we do not properly
        // reduce the second set of addresses is not relevant here
        //
        addr first(result[0].get_from());
        while(result.size() > 1)
        {
            addr next(result[1].get_from());
            next.set_protocol(first.get_protocol());
            if(first != next)
            {
                       // I'm not sure of how to get two IPs in a locally running unit test...
                break; // LCOV_EXCL_LINE
            }
            result.erase(result.begin() + 1);
        }
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
 * The \p is_ipv4 flag is used to know whether the size of the CIDR is
 * 0 to 128 or 0 to 32 and fix the mask accordingly.
 *
 * \bug
 * Note that this flag* is bogus when the input is a domain name and lookup
 * are allowed. This is because in this case the address may be IPv4 or
 * IPv6, which means the mask would be bogus anyway. So I think we are fine.
 * The bug comes from the user in this case.
 *
 * \param[in] mask  The mask to transform to binary.
 * \param[in,out] cidr  The address to which the mask will be added.
 * \param[in] is_ipv4  Whether the address was parsed as an IPv4 (true) or
 * an IPv6 (false).
 */
void addr_parser::parse_mask(
      std::string const & mask
    , addr & cidr
    , bool const is_ipv4)
{
    // no mask?
    //
    if(mask.empty())
    {
        // however, the algorithm below expects that 'mask' is not
        // empty (otherwise we get the case of 0 even though it
        // may not be correct.)
        //
        return;
    }

    // the mask may be a decimal number or an address, if just one number
    // then it's not an address, so test that first
    //
    std::uint8_t mask_bits[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

    // convert the mask to an integer, if possible
    //
    int mask_count(0);
    {
        std::string m(mask);
        for(char const * s(m.c_str()); *s != '\0'; ++s)
        {
            if(*s >= '0' && *s <= '9')
            {
                mask_count = mask_count * 10 + *s - '0';
                if(mask_count > 10000)
                {
                    emit_error("Mask size too large ("
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
        if(is_ipv4)
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
        if(!get_allow(allow_t::ALLOW_ADDRESS_MASK))
        {
            emit_error("Address like mask not allowed (/"
                     + mask
                     + "), try with a simple number instead.");
            return;
        }

        // prepare hints for the the getaddrinfo() function
        //
        addrinfo hints = {};
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
        if(is_ipv4)
        {
            if(mask[0] == '[')
            {
                emit_error("The address uses the IPv4 syntax, the mask cannot use IPv6.");
                return;
            }
        }
        else //if(!is_ipv4)
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
        addrinfo * masklist(nullptr);
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
        std::shared_ptr<addrinfo> mask_ai(masklist, addrinfo_deleter);

        if(is_ipv4)
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
            if(masklist->ai_addrlen != sizeof(sockaddr_in))
            {
                emit_error("Unsupported address size ("                 // LCOV_EXCL_LINE
                        + std::to_string(masklist->ai_addrlen)          // LCOV_EXCL_LINE
                        + ", expected"                                  // LCOV_EXCL_LINE
                        + std::to_string(sizeof(sockaddr_in))           // LCOV_EXCL_LINE
                        + ").");                                        // LCOV_EXCL_LINE
                return;                                                 // LCOV_EXCL_LINE
            }
            memcpy(mask_bits + 12, &reinterpret_cast<sockaddr_in *>(masklist->ai_addr)->sin_addr.s_addr, 4); // last 4 bytes are the IPv4 address, keep the rest as 1s
        }
        else //if(!is_ipv4)
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
            if(masklist->ai_addrlen != sizeof(sockaddr_in6))
            {
                emit_error("Unsupported address size ("                 // LCOV_EXCL_LINE
                         + std::to_string(masklist->ai_addrlen)         // LCOV_EXCL_LINE
                         + ", expected "                                // LCOV_EXCL_LINE
                         + std::to_string(sizeof(sockaddr_in6))         // LCOV_EXCL_LINE
                         + ").");                                       // LCOV_EXCL_LINE
                return;                                                 // LCOV_EXCL_LINE
            }
            memcpy(mask_bits, &reinterpret_cast<sockaddr_in6 *>(masklist->ai_addr)->sin6_addr.s6_addr, 16);
        }
    }

    cidr.set_mask(mask_bits);
}


/** \brief Transform a string into an `addr` object.
 *
 * This function converts the string \p a in an IP address saved in
 * the returned addr object or throws an error if the conversion
 * fails.
 *
 * The \p default_address parameter string can be set to an address
 * which is returned if the input in \p a does not include an
 * address such as in ":123".
 *
 * The \p port parameter can be specified or set to -1. If -1, then
 * there is no default port. Either way, the port can be defined in
 * \p a.
 *
 * The protocol can be specified, as a string. For example, you can
 * use "tcp". The default is no specific protocol which means any
 * type of IP address can be returned. Note that if more than one
 * result is returned when the protocol was not specified, the
 * results will be filtered to only keep the address that uses the
 * TCP protocol. If as a result we have a single address, then that
 * result gets returned.
 *
 * \note
 * This function does not allow for address or port ranges. It is
 * expected to return exactly one address. You can allow a \p mask
 * by setting that parameter to true.
 *
 * \exception addr_invalid_argument
 * If the parsed address is not returning a valid `addr` object, then
 * this function fails by throwing an error. If you would prefer to
 * handle the error mechanism, you want to create your own addr_parser
 * and then call the addr_parser::parse() function. This will allow
 * you to get error messages instead of an exception.
 *
 * \param[in] a  The address string to be converted.
 * \param[in] default_address  The default address or an empty string.
 * \param[in] default_port  The default port or -1
 * \param[in] protocol  The protocol the address has to be of, or the
 *                      empty string to allow any protocol.
 * \param[in] mask  Whether to allow a mask (true) or not (false).
 *
 * \return The address converted in an `addr` object.
 *
 * \sa addr_parser::parse()
 */
addr string_to_addr(
          std::string const & a
        , std::string const & default_address
        , int default_port
        , std::string const & protocol
        , bool mask)
{
    addr_parser p;

    if(!default_address.empty())
    {
        p.set_default_address(default_address);
    }

    if(default_port != -1)
    {
        p.set_default_port(default_port);
    }

    if(!protocol.empty())
    {
        p.set_protocol(protocol);
    }

    p.set_allow(allow_t::ALLOW_MASK, mask);

    addr_range::vector_t result(p.parse(a));

    if(result.size() != 1)
    {
        // when the protocol is not specified, this happens like all the
        // time, we search for an entry with protocol TCP by default
        // because in most cases that's what people want
        //
        if(protocol.empty())
        {
            result.erase(
                      std::remove_if(
                          result.begin()
                        , result.end()
                        , [](auto const it)
                        {
                            return it.has_from() && it.get_from().get_protocol() != IPPROTO_TCP;
                        })
                    , result.end());
        }
        if(result.size() != 1)
        {
            // an invalid protocol is caught by the set_protocol()
            // function, but a totally invalid address or domain name
            // will get us here with an empty list (result.empty() == true)
            //
            throw addr_invalid_argument(
                      "the address \""
                    + a
                    + "\" could not be converted to a single address in string_to_addr(), found "
                    + std::to_string(result.size())
                    + " entries instead.");
        }
    }

    // at the moment, we can only get a "from" so the following exceptions
    // just cannot happen, which is why we have an LCOV_EXCL_LINE
    //
    if(result[0].has_to()
    || result[0].is_range())
    {
        throw addr_invalid_argument("string_to_addr() does not support ranges.");     // LCOV_EXCL_LINE
    }

    if(!result[0].has_from())
    {
        throw addr_invalid_argument("string_to_addr() has no 'from' address.");       // LCOV_EXCL_LINE
    }

    return result[0].get_from();
}



}
// namespace addr
// vim: ts=4 sw=4 et
