// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The addr_range class.
 *
 * This header defines the addr_range class used to handle one range of
 * addresses (with a 'from' and a 'to' pair of addresses.)
 *
 * Also we support CIDR, a CIDR is not a range. A range can define anything
 * that is not a perfect CIDR match. For example, you could have a start
 * address of 192.168.10.5 and an end address of 192.168.10.10.
 */

// self
//
#include    <libaddr/addr.h>



namespace addr
{


class addr_range
{
public:
    typedef std::shared_ptr<addr_range>     pointer_t;
    typedef std::vector<addr_range>         vector_t;

    bool                            has_from() const;
    bool                            has_to() const;
    bool                            is_defined() const;
    bool                            is_range() const;
    bool                            is_empty() const;
    bool                            is_in(addr const & rhs) const;
    bool                            is_ipv4() const;

    void                            set_from(addr const & from);
    addr &                          get_from();
    addr const &                    get_from() const;
    void                            set_to(addr const & to);
    addr &                          get_to();
    addr const &                    get_to() const;
    void                            swap_from_to();
    void                            from_cidr(addr const & range);
    addr::vector_t                  to_addresses(std::size_t limit = 1000) const;
    std::string                     to_string(string_ip_t const mode = STRING_IP_ALL) const;
    static std::string              to_string(
                                          vector_t const & ranges
                                        , string_ip_t const mode = STRING_IP_ALL
                                        , std::string const & separator = std::string(","));

    std::size_t                     size() const;
    addr_range                      intersection(addr_range const & rhs) const;
    addr_range                      union_if_possible(addr_range const & rhs) const;
    bool                            match(addr const & address) const;
    compare_t                       compare(addr_range const & rhs, bool mixed = false) const;
    bool                            operator < (addr_range const & rhs) const;

    static addr::vector_t           to_addresses(vector_t ranges, std::size_t limit = 1000);

private:
    bool                            f_has_from = false;
    bool                            f_has_to = false;
    addr                            f_from = addr();
    addr                            f_to = addr();
};


bool address_match_ranges(addr_range::vector_t ranges, addr const & address);



template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits> &
operator << (std::basic_ostream<_CharT, _Traits> & out, addr_range const & range)
{
    _ostream_info * info(static_cast<_ostream_info *>(out.pword(get_ostream_index())));
    if(info == nullptr)
    {
        out << range.to_string(STRING_IP_ALL);
    }
    else
    {
        out << range.to_string(info->f_mode);
    }
    return out;
}


template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits> &
operator << (std::basic_ostream<_CharT, _Traits> & out, addr_range::vector_t const & ranges)
{
    _ostream_info * info(static_cast<_ostream_info *>(out.pword(get_ostream_index())));
    if(info == nullptr)
    {
        out << addr_range::to_string(ranges);
    }
    else
    {
        out << addr_range::to_string(
                  ranges
                , info->f_mode
                , info->f_sep);
    }
    return out;
}






}
// namespace addr
// vim: ts=4 sw=4 et
