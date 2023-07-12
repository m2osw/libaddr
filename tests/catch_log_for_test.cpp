// Copyright (c) 2006-2023  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "catch_main.h"


// cppthread
//
#include    <cppthread/log.h>


// libexcept
//
#include    <libexcept/exception.h>


// last include
//
#include    <snapdev/poison.h>




namespace SNAP_CATCH2_NAMESPACE
{



namespace
{

std::vector<std::string>        g_expected_logs = std::vector<std::string>();

}
// no name namespace



void push_expected_log(std::string const & message)
{
    g_expected_logs.push_back(message);
}


void log_for_test(cppthread::log_level_t level, std::string const & message)
{
    if(SNAP_CATCH2_NAMESPACE::g_verbose()
    || g_expected_logs.empty())
    {
        std::cerr << "logger sent:\n"
                  << cppthread::to_string(level)
                  << ": "
                  << message
                  << std::endl;
    }

    // at this time it's impossible to debug the location of the empty
    // problem without a proper stack trace...
    //
    if(g_expected_logs.empty())
    {
        libexcept::stack_trace_t trace(libexcept::collect_stack_trace_with_line_numbers());
        std::cerr << "*** STACK TRACE ***" << std::endl;
        for(auto const & l : trace)
        {
            std::cerr << l << std::endl;
        }
        std::cerr << "***" << std::endl;
    }

    CATCH_REQUIRE_FALSE(g_expected_logs.empty());

    std::stringstream ss;
    ss << cppthread::to_string(level) << ": " << message;

    // again, the REQUIRE() is not going to be useful in terms of line number
    //
    if(g_expected_logs[0] != ss.str())
    {
        libexcept::stack_trace_t trace(libexcept::collect_stack_trace_with_line_numbers());
        std::cerr << "*** STACK TRACE ***" << std::endl;
        for(auto const & l : trace)
        {
            std::cerr << l << std::endl;
        }
        std::cerr << "***" << std::endl;
    }

    std::string expected_msg(g_expected_logs[0]);
    g_expected_logs.erase(g_expected_logs.begin());

    CATCH_REQUIRE(expected_msg == ss.str());
}


void expected_logs_stack_is_empty()
{
    if(!g_expected_logs.empty())
    {
        std::cerr << "List of expected error logs which did not occur:" << std::endl;
        for(auto l : g_expected_logs)
        {
            std::cerr << "  " << l << std::endl;
        }
        g_expected_logs.clear();
        throw std::logic_error("a test left an unexpected error message in the g_expected_logs vector.");
    }

    cppthread::log.reset_counters();
}


} // SNAP_CATCH2_NAMESPACE namespace
// vim: ts=4 sw=4 et
