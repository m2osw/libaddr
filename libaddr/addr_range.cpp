// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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


// C++ library
//
#include    <algorithm>


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
 * This happens if the mask is not just ending with 0s but 0s are found
 * earlier.
 *
 * \param[in] a  The address to convert to a range.
 */
void addr_range::from_cidr(addr const & a)
{
    std::uint8_t mask[16];
    a.get_mask(mask);
    bool found(false);
    sockaddr_in6 from = {};
    a.get_ipv6(from);
    sockaddr_in6 to(from);
    for(std::size_t i(0); i < sizeof(mask); ++i)
    {
        std::uint8_t inverse = ~mask[i];

        from.sin6_addr.s6_addr[i] &= mask[i];
        to.sin6_addr.s6_addr[i] |= inverse;

        if(found)
        {
            if(inverse != 0xFF)
            {
                throw addr_unsupported_as_range("unsupported mask for a range");
            }
        }
        else
        {
            if(inverse != 0)
            {
                found = true;
                switch(inverse)
                {
                case 0x01:
                case 0x03:
                case 0x07:
                case 0x0F:
                case 0x1F:
                case 0x3F:
                case 0x7F:
                case 0xFF:
                    break;

                default:
                    throw addr_unsupported_as_range("unsupported mask for a range");

                }
            }
        }
    }

    set_from(from);
    set_to(to);
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
}


/** \brief Compute a new range with the union of two address ranges.
 *
 * This function checks whether the two ranges have any addresses in
 * common or if they are just one after the other. If so, then it
 * computes the union of both ranges. Otherwise it returns an empty
 * range.
 *
 * \note
 * If the `from` and `to` addresses of the range have a mask, it is
 * ignored.
 *
 * \param[in] rhs  The other range to compute the intersection with.
 *
 * \return The resulting intersection range.
 *
 * \sa is_empty()
 */
addr_range addr_range::union_if_possible(addr_range const & rhs) const
{
    addr_range result;

    if((f_from <= rhs.f_to || f_from.is_previous(rhs.f_to)
    && (f_to >= rhs.f_from || f_to.is_next(rhs.f_from))))
    {
        result.set_from(f_from < rhs.f_from ? f_from : rhs.f_from);
        result.set_from(f_to   > rhs.f_to   ? f_to   : rhs.f_to);
    }

    return result;
}


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
 * Comparing two address ranges against each other can return one of many
 * possible results (See the compare_t enumeration).
 *
 * \param[in] rhs  The right hand side to compare against this range.
 *
 * \return One of the compare_t values.
 */
compare_t addr_range::compare(addr_range const & rhs) const
{
    // check for the empty case first
    //
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

    // no overlap (lhs < rhs)
    //
    if(f_to < rhs.f_from)
    {
        if(f_to.is_next(rhs.f_from))
        {
            return compare_t::COMPARE_FOLLOWS;
        }
        return compare_t::COMPARE_SMALL_VS_LARGE;
    }

    // no overlap (lhs > rhs)
    //
    if(f_from > rhs.f_to)
    {
        if(f_to.is_previous(rhs.f_from))
        {
            return compare_t::COMPARE_FOLLOWING;
        }
        return compare_t::COMPARE_LARGE_VS_SMALL;
    }

    // overlap (lhs <= rhs)
    //
    if(f_from <= rhs.f_from
    && f_to >= rhs.f_from)
    {
        if(f_to >= rhs.f_to)
        {
            if(f_from == rhs.f_from
            && f_to == rhs.f_to)
            {
                return compare_t::COMPARE_EQUAL;
            }
            return compare_t::COMPARE_INCLUDED;
        }
        if(f_from == rhs.f_from)
        {
            return compare_t::COMPARE_INCLUDES;
        }
        return compare_t::COMPARE_OVERLAP_SMALL_VS_LARGE;
    }

    // overlap (lhs >= rhs)
    //
    if(f_to >= rhs.f_from
    && f_to <= rhs.f_to)
    {
        if(f_from >= rhs.f_from)
        {
            // the previous block already captured this case
            //
            //if(f_from == rhs.f_from
            //&& f_to == rhs.f_to)
            //{
            //    return compare_t::COMPARE_EQUAL;
            //}

            return compare_t::COMPARE_INCLUDES;
        }
        if(f_to == rhs.f_to)
        {
            return compare_t::COMPARE_INCLUDED;
        }
        return compare_t::COMPARE_OVERLAP_LARGE_VS_SMALL;
    }

    // no overlap
    //
    return f_to < rhs.f_from
            ? compare_t::COMPARE_SMALLER
            : compare_t::COMPARE_LARGER;
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




}
// namespace addr
// vim: ts=4 sw=4 et
