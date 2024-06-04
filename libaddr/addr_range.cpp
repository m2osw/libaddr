// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The implementation of the addr_range class.
 *
 * This file includes the implementation of the addr_range class
 * and the address_match_ranges() global function.
 */

// self
//
#include    "libaddr/addr_range.h"
#include    "libaddr/exception.h"


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/ostream_int128.h>


// C++
//
#include    <algorithm>
#include    <cassert>
#include    <limits>


// last include
//
#include    <snapdev/poison.h>



namespace addr
{


/** \brief Return true if the range has a 'from' address defined.
 *
 * By default the 'from' and 'to' addresses of an addr_range are legal
 * but considered undefined. After you called the set_from() function
 * once, this function will always return true.
 *
 * \return false until 'set_from()' is called at least once.
 */
bool addr_range::has_from() const
{
    return f_has_from;
}


/** \brief Return true if the range has a 'to' address defined.
 *
 * By default the 'from' and 'to' addresses of an addr_range are legal
 * but considered undefined. After you called the set_to() function
 * once, this function will always return true.
 *
 * \return false until 'set_to()' is called at least once.
 */
bool addr_range::has_to() const
{
    return f_has_to;
}


/** \brief Check whether the range has at least a "from" or a "to".
 *
 * We have several functions to check ranges. This one returns true
 * if at least one of has_from() or has_to() would return true.
 *
 * A defined range can be ordered (see the compare() function).
 *
 * For an addr_range object to be a defined range, both, the
 * has_from() and has_to() have to return true, which the
 * compare() function doesn't require (i.e. the compare can
 * compare two "from" addresses).
 *
 * \return true if at least one of "from" or "to" is defined.
 */
bool addr_range::is_defined() const
{
    return f_has_from || f_has_to;
}


/** \brief Determine whether an addr_range object is considered a range.
 *
 * This function returns false until both, set_from() and set_to(),
 * were called.
 *
 * Note that the order in which the two functions get called is not
 * important, although we generally expect set_from() to be called
 * first, it does not matter.
 *
 * \return true if both, 'from' and 'to', were set.
 */
bool addr_range::is_range() const
{
    return f_has_from && f_has_to;
}


/** \brief Check whether this range is empty.
 *
 * If you defined the 'from' and 'to' addresses of the range, then you
 * can check whether the range is empty or not.
 *
 * A range is considered empty if 'from' is larger than 'to' because
 * in that case nothing can appear in between (no IP can at the same
 * time be larger than 'from' and smaller than 'to' if 'from > to'
 * is true.)
 *
 * \return true if 'from > to' and is_range() returns true.
 *
 * \sa is_range()
 * \sa has_from()
 * \sa has_to()
 */
bool addr_range::is_empty() const
{
    if(!is_range())
    {
        return false;
    }
    return f_from > f_to;
}


/** \brief Check whether \p rhs is part of this range.
 *
 * If the address specified in rhs is part of this range, then the function
 * returns true. The 'from' and 'to' addresses are considered inclusive,
 * so if rhs is equal to 'from' or 'to', then the function returns true.
 *
 * If 'from' is larger than 'to' then the function already returns false
 * since the range represents an empty range.
 *
 * \exception addr_invalid_state_exception
 * The addr_range object must be a range or this function throws this
 * exception. To test whether you can call this function, first call
 * the is_range() function. If it returns true, then is_in() is available.
 *
 * \param[in] rhs  The address to check for inclusion.
 *
 * \return true if rhs is considered part of this range.
 */
bool addr_range::is_in(addr const & rhs) const
{
    if(!is_range())
    {
        throw addr_invalid_state("addr_range::is_in(): range is not complete (from or to missing.)");
    }

    if(f_from <= f_to)
    {
        //
        return rhs >= f_from && rhs <= f_to;
    }
    //else -- from/to are swapped... this represents an empty range

    return false;
}


/** \brief Check whether this range is an IPv4 range.
 *
 * This function verifies whether this range represents an IPv6 range or
 * an IPv4 range.
 *
 * If the range is not defined (no from and no to) then the function
 * always returns false.
 *
 * \return true if the range represents an IPv4 address.
 */
bool addr_range::is_ipv4() const
{
    if(f_has_from && f_has_to)
    {
        return f_from.is_ipv4() && f_to.is_ipv4();
    }

    if(f_has_from)
    {
        return f_from.is_ipv4();
    }

    if(f_has_to)
    {
        return f_to.is_ipv4();
    }

    return false;
}


/** \brief Set 'from' address.
 *
 * This function saves the 'from' address in this range object.
 *
 * Once this function was called at least once, the has_from() returns true.
 *
 * \param[in] from  The address to save as the 'from' address.
 */
void addr_range::set_from(addr const & from)
{
    f_has_from = true;
    f_from = from;
}


/** \brief Get 'from' address.
 *
 * This function return the 'from' address as set by the set_from()
 * functions.
 *
 * The get_from() function can be called even if the has_from()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'from' address.
 */
addr & addr_range::get_from()
{
    return f_from;
}


/** \brief Get the 'from' address when addr_range is constant.
 *
 * This function return the 'from' address as set by the set_from()
 * functions.
 *
 * The get_from() function can be called even if the has_from()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'from' address.
 */
addr const & addr_range::get_from() const
{
    return f_from;
}


/** \brief Set 'to' address.
 *
 * This function saves the 'to' address in this range object.
 *
 * Once this function was called at least once, the has_to() returns true.
 *
 * \param[in] to  The address to save as the 'to' address.
 */
void addr_range::set_to(addr const & to)
{
    f_has_to = true;
    f_to = to;
}


/** \brief Get the 'to' address.
 *
 * This function return the 'to' address as set by the set_to()
 * function.
 *
 * The get_from() function can be called even if the has_from()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'to' address.
 */
addr & addr_range::get_to()
{
    return f_to;
}


/** \brief Get the 'to' address when addr_range is constant.
 *
 * This function return the 'to' address as set by the set_to()
 * function.
 *
 * The get_to() function can be called even if the has_to()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'to' address.
 */
addr const & addr_range::get_to() const
{
    return f_to;
}


/** \brief Swap the "from" and "to" addresses.
 *
 * If you want to cut some slack to your users and detect that "from > to"
 * then you can call this function to fix the range.
 *
 * It can also be used to make a "from" only range a "to" only range
 * and vice versa.
 */
void addr_range::swap_from_to()
{
    std::swap(f_from, f_to);
    std::swap(f_has_from, f_has_to);
}


/** \brief Transform an address to a range.
 *
 * This function transforms an address (\p a) in a range.
 *
 * This is useful if you have a CIDR type of address (an address with a
 * mask length defined along with it) and want to generate a range with
 * it.
 *
 * The range is defined as (a & mask) for the "from" address and
 * (a | ~mask) for the "to" address. If the mask is all 1s, then the
 * resulting range is just (a).
 *
 * \exception addr_unsupported_as_range
 * If the address cannot be transform into a range, this exception is raised.
 * This happens if the mask is not just ending with 0s (i.e. the mask cannot
 * be represented by just a number).
 *
 * \param[in] a  The address to convert to a range.
 */
void addr_range::from_cidr(addr const & a)
{
    if(a.get_mask_size() == -1)
    {
        throw addr_unsupported_as_range("unsupported mask for a range");
    }

    f_has_from = true;
    f_has_to = true;

    f_from = a;
    f_to = a;

    f_from.apply_mask();
    f_to.apply_mask(true);
}


/** \brief Try to transform a range in a CIDR address.
 *
 * This function checks whether the range represents a network address as
 * in 192.168.0.0/24. For that example, it is the case if the "from"
 * address is 192.168.0.0 and the "to" address is 192.168.255.255.
 *
 * \param[out] a  The resulting address. Changed only if the function returns
 * true.
 *
 * \return true and \p a set to the from address and the mask to the
 * corresponding CIDR if it was possible.
 */
bool addr_range::to_cidr(addr & a) const
{
    if(!is_range()
    || is_empty())
    {
        return false;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    unsigned __int128 const from(f_from.ip_to_uint128());
    unsigned __int128 const to(f_to.ip_to_uint128());
    int cidr(128);
    unsigned __int128 bit(1);
    for(int count(0); count < 128; ++count, bit <<= 1)
    {
        if((from & bit) != 0
        || (to & bit) == 0)
        {
            do
            {
                if((from & bit) != (to & bit))
                {
                    return false;
                }
                ++count;
                bit <<= 1;
            }
            while(count < 128);
            break;
        }
        --cidr;
    }
#pragma GCC diagnostic pop

    a.ip_from_uint128(from);
    a.set_mask_count(cidr);

    return true;
}


/** \brief Transform a range in a vector of separate addresses.
 *
 * This function goes through a range and transforms it in a set of addresses.
 * This a range can be really large, this function has a limit which can
 * be used to very much limit the results (i.e. to 1000, for example).
 *
 * This is useful to determine all the addresses and attempt connecting
 * to all of them separately.
 *
 * \exception out_of_range
 * If the range represents more than \p limit addresses, then this
 * exception is raised. Note that with IPv6, the number of addresses
 * can be really large.
 *
 * \param[in] limit  The maximum number of addresses accepted.
 *
 * \return The vector of addresses representing this range.
 */
addr::vector_t addr_range::to_addresses(std::size_t limit) const
{
    addr::vector_t result;

    if(size() > limit)
    {
        throw out_of_range("too many addresses in this range: "
            + std::to_string(size())
            + " > "
            + std::to_string(limit));
    }

    if(has_from()
    && has_to())
    {
        addr a(f_from);
        do
        {
            result.push_back(a);
            ++a;
        }
        while(a <= f_to);
    }
    else if(has_from())
    {
        result.push_back(f_from);
    }
    else if(has_to())
    {
        result.push_back(f_to);
    }

    return result;
}


addr::vector_t addr_range::to_addresses(vector_t ranges, std::size_t limit)
{
    std::size_t total_size(0);
    for(auto const & r : ranges)
    {
        total_size += r.size();
    }
    if(total_size > limit)
    {
        throw out_of_range("too many addresses in this range: "
            + std::to_string(total_size)
            + " > "
            + std::to_string(limit));
    }

    addr::vector_t result;
    for(auto const & r : ranges)
    {
        addr::vector_t const v(r.to_addresses(limit));
        result.insert(result.end(), v.begin(), v.end());
    }

    return result;
} // LCOV_EXCL_LINE


/** \brief Transform the range into a string.
 *
 * This function converts this range in a string.
 *
 * If the range is not defined (is_range() returns false) or if it is
 * empty (from > to), then the function returns the string:
 *
 * \code
 *     "<empty address range>"
 * \endcode
 *
 * Otherwise, it returns the range as two addresses separated by a dash
 * character.
 *
 * The mode works the same way here as it does in the
 * addr::to_ipv4or6_string() function.
 *
 * If the range is only composed of a "from" address, then only that one
 * address is output.
 *
 * If the range is only composed of a "to" address, then the output starts
 * with a dash (-) and then the "to" address.
 *
 * \param[in] mode  The mode used to generate the address.
 *
 * \return The range or the "<empty address range>" string.
 */
std::string addr_range::to_string(string_ip_t const mode) const
{
    if(is_empty()
    || (!has_from() && !has_to()))
    {
        return std::string("<empty address range>");
    }

    std::string result;

    if(has_from() && has_to())
    {
        // we do not want the port nor mask in the from address
        //
        string_ip_t const from_mode(mode & (STRING_IP_ADDRESS | STRING_IP_BRACKET_ADDRESS));

        // WARNING: we are assuming that the from & to data has the same
        //          port information (which should be the case if you used
        //          the parser)
        //
        std::string const from(f_from.to_ipv4or6_string(from_mode));
        std::string const to(f_to.to_ipv4or6_string(mode));

        result.reserve(from.length() + 1 + to.length());
        result = from;
        result += '-';
        result += to;
    }
    else if(has_from())
    {
        result = f_from.to_ipv4or6_string(mode);
    }
    else //if(has_to()) -- this is the only other possible case, no need to test
    {
        std::string const to(f_to.to_ipv4or6_string(mode));

        result.reserve(1 + to.length());
        result += '-';
        result += f_to.to_ipv4or6_string(mode);
    }

    return result;
}


/** \brief Concatenate all the ranges of a vector in a string.
 *
 * This function concatenate all the ranges of a vector in a string
 * using \p separator as the character to separate each item.
 *
 * By default, the \p separator is set to the comma (,). The separator
 * needs tobe set to a comma (,), a space ( ), or a newline (\\n) if
 * you want to generate a string which the addr_parser() can handle
 * innately.
 *
 * The \p separator parameter is a string allowing you to pass any
 * Unicode character as the separator. It is still expected to be a
 * single character although you could pass multiple.
 *
 * \param[in] ranges  The ranges to convert into a string.
 * \param[in] mode  The mode used to output each range.
 * \param[in] separator  The separator character.
 */
std::string addr_range::to_string(
      vector_t const & ranges
    , string_ip_t const mode
    , std::string const & separator)
{
    std::string result;

    // 15 is somewhat arbitrary, it is a full IPv4 address; a full IPv6
    // address can be 39 characters (8 x 4 hex digits + 7 x 1 colons)
    // but those are rarely full either
    //
    result.reserve(ranges.size() * (15 + separator.length()));

    bool first(true);
    for(auto const & r : ranges)
    {
        if(first)
        {
            first = false;
        }
        else
        {
            result += separator;
        }
        result += r.to_string(mode);
    }

    return result;
} // LCOV_EXCL_LINE


/** \brief Compute the number of addresses this range represents.
 *
 * The size() function calculates the size of the range. We currently
 * support three possibilities:
 *
 * 1. from & to are defined, the result is "to - from + 1"
 * 2. the range is empty, the result is 0
 * 3. the range only has a from or a to, the result is 1
 *
 * \note
 * The function returns an std::size_t, the maximum possible range
 * would require 128 bits. It is expected that the function won't
 * be used with such large ranges which are anyway not going to be
 * useful in the real world.
 *
 * \return The size of this range.
 */
std::size_t addr_range::size() const
{
    if(has_from()
    && has_to())
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        return std::min(static_cast<unsigned __int128>(f_to - f_from + 1)
                    , static_cast<unsigned __int128>(std::numeric_limits<std::size_t>::max()));
#pragma GCC diagnostic pop
    }

    if(!has_from()
    && !has_to())
    {
        return 0;
    }

    return 1;
}


/** \brief Compute a new range with the part that is shared between both inputs.
 *
 * This function computers a range which encompasses all the addresses found
 * in \p this range and \p rhs range.
 *
 * If the two range do not intersect, then the resulting range will be an
 * empty range (see is_empty() for details).
 *
 * The new range receives the largest 'from' address from both inputs and
 * the smallest 'to' address from both inputs.
 *
 * \param[in] rhs  The other range to compute the intersection with.
 *
 * \return The resulting intersection range.
 *
 * \sa is_empty()
 */
addr_range addr_range::intersection(addr_range const & rhs) const
{
    addr_range result;

    result.set_from(f_from > rhs.f_from ? f_from : rhs.f_from);
    result.set_to  (f_to   < rhs.f_to   ? f_to   : rhs.f_to);

    return result;
} // LCOV_EXCL_LINE


/** \brief Compute a new range representing the union of two address ranges.
 *
 * This function checks whether this range and the \p rhs range have any
 * addresses in common or if they are just one after the other. If so,
 * it computes the union of both ranges. Otherwise it returns a non-range
 * object (i.e. an addr_range which is_range() predicate returns false).
 *
 * Note that if both inputs are the same IP address, then the result is just
 * one address (i.e. only "from" will be defined in the resulting range).
 *
 * \note
 * The masks are ignored.
 *
 * \note
 * To test two network addresses (i.e. an address with a CIDR), create two
 * ranges using the from_cidr() function.
 * \note
 * \code
 *     addr_range ra, rb;
 *     ra.from_cidr(a);
 *     rb.from_cidr(b);
 *     addr_range u(ra.union_if_possible(rb));
 *     if(u.is_defined())
 *     {
 *         // 'a' is equal or included in 'b'
 *         // or 'b' is equal or included in 'a'
 *         //
 *         ...
 *     }
 * \endcode
 *
 * \param[in] rhs  The other range to compute the intersection with.
 *
 * \return The resulting union range or no range (use is_defined() on result).
 *
 * \sa is_empty()
 */
addr_range addr_range::union_if_possible(addr_range const & rhs) const
{
    addr_range result;

    if(!is_defined()
    && !rhs.is_defined())
    {
        return result;
    }

    addr const & lhs_from(f_has_from     ? f_from     : f_to);
    addr const & lhs_to  (f_has_to       ? f_to       : f_from);
    addr const & rhs_from(rhs.f_has_from ? rhs.f_from : rhs.f_to);
    addr const & rhs_to  (rhs.f_has_to   ? rhs.f_to   : rhs.f_from);

    if((lhs_from <= rhs_to && lhs_to >= rhs_from)
    || (lhs_to >= rhs_from && lhs_from <= rhs_to)
    || lhs_from.is_previous(rhs_to)
    || lhs_to.is_next(rhs_from))
    {
        addr from(lhs_from < rhs_from ? lhs_from : rhs_from);
        addr to  (lhs_to   > rhs_to   ? lhs_to   : rhs_to);

        // keep the smallest mask if defined
        //
        from.set_mask_count(std::min(f_from.get_mask_size(), rhs.f_from.get_mask_size()));
        to.set_mask_count(std::min(f_to.get_mask_size(), rhs.f_to.get_mask_size()));

        if(is_range()
        || rhs.is_range()
        || from != to)
        {
            result.set_from(from);
            result.set_to(to);
        }
        else
        {
            result.set_from(from);
        }
    }

    return result;
} // LCOV_EXCL_LINE


/** \brief Check whether an address matches a range.
 *
 * This function checks whether an address matches a range of addresses.
 *
 * The range may be empty, in which case the result is always false.
 *
 * If the range is a range (i.e. 'from' and 'to' are both defined,)
 * then the is_in() function is used to determine whether the address
 * is a match.
 *
 * If only one of the 'from' or 'to' addresses is defined, then that
 * one address addr::match() function is used to determine whether the
 * input \p address is a match.
 *
 * \param[in] address  The address to match against a range of addresses.
 *
 * \return true if address matches this range.
 */
bool addr_range::match(addr const & address) const
{
    // if neith 'from' nor 'to' were defined, return
    //
    if(is_empty())
    {
        return false;
    }

    if(is_range())
    {
        return is_in(address);
    }

    if(has_from())
    {
        return f_from.match(address);
    }
    else
    {
        // if not empty and it does not have 'from', it has to be 'to'
        //
        return f_to.match(address);
    }
}


/** \brief Compare an address range against another.
 *
 * This function compares two address ranges against each other and return
 * a compare_t value representing the result.
 *
 * * COMPARE_EQUAL -- lhs == rhs
 * * COMPARE_SMALLER -- lhs < rhs
 * * COMPARE_LARGER -- lhs > rhs
 * * COMPARE_OVERLAP_SMALL_VS_LARGE -- lhs is before rhs with an overlap
 * * COMPARE_OVERLAP_LARGE_VS_SMALL -- rhs is before lhs with an overlap
 * * COMPARE_INCLUDED -- rhs is included in lhs
 * * COMPARE_INCLUDES -- lhs is included in rhs
 * * COMPARE_PRECEDES -- lhs is just before rhs
 * * COMPARE_FOLLOWS -- lhs is just after rhs
 * * COMPARE_FIRST -- lhs is defined, rhs is empty
 * * COMPARE_LAST -- lhs is empty, rhs is defined
 * * COMPARE_IPV4_VS_IPV6 -- lhs is an IPv4, rhs an IPv6, not used if mixed is true
 * * COMPARE_IPV6_VS_IPV4 -- lhs is an IPv6, rhs an IPv4, not used if mixed is true
 * * COMPARE_UNORDERED -- lhs and rhs are both empty or are undefined
 *
 * Note that if you have one of the inputs which is not a valid range
 * (i.e. the is_range() function return false) then the function
 * always returns COMPARE_UNORDERED. This is different from the
 * is_empty() predicate where if one is empty and not the other,
 * then you can get COMPARE_FIRST or COMPARE_LAST.
 *
 * By default the function separates IPv6 and IPv4 addresses. If you want
 * to group IPv6 and IPv4 separately, set the \p mixed flag to true. That
 * means you get the COMPARE_IPV4_VS_IPV6 and COMPARE_IPV6_VS_IPV4 results
 * if the input IPs are mixed and that allows you to group IPv6 first and
 * then IPv4 or vice versa (the addr_parser does that for you with the
 * SORT_IPV6_FIRST and the SORT_IPV4_FIRST).
 *
 * Here is a graphical representation of the first 9 cases:
 *
 * \code
 *                        Ranges                          Results
 *   LHS:
 *                  |----------------|
 *
 *   RHS:
 *                  |----------------|                   Equal (LHS == RHS)
 *     |----|                                            Larger (LHS > RHS)
 *                                       |-----|         Smaller (LHS < RHS)
 *     |-----------|                                     Follows (LHS.from == RHS.to + 1)
 *                                    |-----------|      Precedes (LHS.to + 1 == RHS.from)
 *
 *                  |----------|                         Included (LHS.from <= RHS.from && LHS.to >= RHS.to && LHS != RHS)
 *                      |----------|
 *                        |----------|
 *
 *         |---------------------------------|           Includes (LHS.from > RHS.from && LHS.to < RHS.to)
 *                  |------------------------|
 *         |-------------------------|
 *
 *                         |----------------------|      Small vs Large (LHS.from <= RHS.from && LHS.to < RHS.to)
 *                  |-----------------------------|
 *
 *   |-----------------------|                           Large vs Small (LHS.from > RHS.from && LHS.to >= RHS.to)
 *   |-------------------------------|
 * \endcode
 *
 * \param[in] rhs  The right hand side to compare against this range.
 * \param[in] mixed  Whether IPv4 and IPv6 are compared as is (false) or not.
 *
 * \return One of the compare_t values.
 */
compare_t addr_range::compare(addr_range const & rhs, bool mixed) const
{
    // check for the empty case first
    //
    if(!is_defined()
    || !rhs.is_defined())
    {
        return compare_t::COMPARE_UNORDERED;
    }

    if(is_empty())
    {
        if(rhs.is_empty())
        {
            return compare_t::COMPARE_UNORDERED;
        }

        return compare_t::COMPARE_LAST;
    }
    else if(rhs.is_empty())
    {
        return compare_t::COMPARE_FIRST;
    }

    // IPv4 versus IPv6
    //
    if(!mixed)
    {
        if(is_ipv4())
        {
            if(!rhs.is_ipv4())
            {
                return compare_t::COMPARE_IPV4_VS_IPV6;
            }
        }
        else if(rhs.is_ipv4())
        {
            return compare_t::COMPARE_IPV6_VS_IPV4;
        }
    }

    // the f_to is often undefined, although the f_from may be undefined
    // too, so here we copy the values in order for our compares below to
    // work as expected
    //
    // note: the is_defined() already verified that at least one of the
    //       two (f_from or f_to) is defined
    //
    addr const & lhs_from(f_has_from     ? f_from     : f_to);
    addr const & lhs_to  (f_has_to       ? f_to       : f_from);
    addr const & rhs_from(rhs.f_has_from ? rhs.f_from : rhs.f_to);
    addr const & rhs_to  (rhs.f_has_to   ? rhs.f_to   : rhs.f_from);

    // no overlap (lhs < rhs)
    //
    if(lhs_to < rhs_from)
    {
        if(lhs_to.is_next(rhs_from))
        {
            return compare_t::COMPARE_PRECEDES;
        }
        return compare_t::COMPARE_SMALLER;
    }

    // no overlap (lhs > rhs)
    //
    if(lhs_from > rhs_to)
    {
        if(lhs_from.is_previous(rhs_to))
        {
            return compare_t::COMPARE_FOLLOWS;
        }
        return compare_t::COMPARE_LARGER;
    }

    // overlap (lhs <= rhs)
    //
    if(lhs_from <= rhs_from)
    {
        if(lhs_to >= rhs_to)
        {
            if(lhs_from == rhs_from
            && lhs_to == rhs_to)
            {
                return compare_t::COMPARE_EQUAL;
            }
            return compare_t::COMPARE_INCLUDED;
        }
        if(lhs_from == rhs_from)
        {
            return compare_t::COMPARE_INCLUDES;
        }
        return compare_t::COMPARE_OVERLAP_SMALL_VS_LARGE;
    }

    assert(lhs_to >= rhs_from);

    // overlap (lhs >= rhs)
    //
    if(lhs_to <= rhs_to)
    {
        return compare_t::COMPARE_INCLUDES;
    }

    return compare_t::COMPARE_OVERLAP_LARGE_VS_SMALL;
}


/** \brief Compare for smaller or larger.
 *
 * The `<` operator is used to sort a vector of address range.
 *
 * Note that it is not a solid order operator in the event some of your
 * ranges are not properly defined (i.e. has_from() and has_to() both
 * return false).
 *
 * \warning
 * This compare sorts IPv4 and IPv6 addresses in a mixed manner.
 *
 * \param[in] rhs  The right hand side range to compare against this.
 *
 * \return true if this range is considered smaller than \p rhs
 */
bool addr_range::operator < (addr_range const & rhs) const
{
    switch(compare(rhs, true))
    {
    case compare_t::COMPARE_SMALLER:
    case compare_t::COMPARE_OVERLAP_SMALL_VS_LARGE:
    case compare_t::COMPARE_INCLUDED:
    //case compare_t::COMPARE_IPV6_VS_IPV4: -- not necessary, v4/v6 are sorted against each other
    case compare_t::COMPARE_PRECEDES:
    case compare_t::COMPARE_FIRST:
        return true;

    default:
        return false;

    }
    snapdev::NOT_REACHED();
}


/** \brief Check whether an address matches a range.
 *
 * When you call the addr_parser::parse() function, you get a vector of
 * ranges as a result. This function allows you to check whether an
 * address matches any one of those ranges.
 *
 * \param[in] ranges  The vector of ranges to search for \p address.
 * \param[in] address  The address to search in \p ranges.
 *
 * \return true if \p address matches any one of the \p ranges.
 */
bool address_match_ranges(addr_range::vector_t ranges, addr const & address)
{
    auto const it(std::find_if
            ( ranges.begin()
            , ranges.end()
            , [&address](auto const & range)
                {
                    return range.match(address);
                }
            ));

    return it != ranges.end();
}


/** \brief Optimize a vector of addresses.
 *
 * This function checks whether an address is equal or included in another.
 * If so, it gets removed from the \p v vector.
 *
 * \note
 * This function makes use of the mask to know whether an address is include
 * in address. More precisely, it checks whether a network is included in
 * another. For example, 10.0.0.0/24 is included in 10.0.0.0/16 so it can
 * be optimized out.
 *
 * \warning
 * The function ignores the port information. So two networks that match,
 * ignoring the port, will be optimized.
 *
 * \param[in,out] v  The vector of addresses to optimize.
 *
 * \return true if the input vector was updated.
 */
bool optimize_vector(addr::vector_t & v)
{
    bool result(false);

    std::size_t max(v.size());
    for(std::size_t i(0); i < max; ++i)
    {
        addr_range ra;
        ra.from_cidr(v[i]);
        for(std::size_t j(i + 1); j < max; ++j)
        {
            addr_range rb;
            rb.from_cidr(v[j]);

            addr_range const u(ra.union_if_possible(rb));
            if(u.is_defined())
            {
                // union was possible -- check whether result has valid CIDR
                //
                addr p(u.get_from());
                addr q(p);
                p.apply_mask();
                q.apply_mask(true);
                if(p == u.get_from()
                && q == u.get_to())
                {
                    ra = u;
                    v[i] = p;
                    v.erase(v.begin() + j);
                    --max;
                    --j; // cancel the ++j in the for() loop
                    result = true;
                }
            }
        }
    }

    return result;
}


}
// namespace addr
// vim: ts=4 sw=4 et
