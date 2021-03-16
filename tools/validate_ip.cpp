// Network Address -- get addresses on the command line and validate them
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
//

/** \file
 * \brief A tool to check the validity of an IP address.
 *
 * This tool is used to verify or canonicalize an IP address or a list of
 * IP addresses.
 *
 * The default is to verify (`--verify`). In this case, the tool generates
 * no output. It exits with 0 when no error were found. It prints out
 * errors to stderr and exit with 1 when errors there are.
 *
 * The `--canonicalize` command line option requires the tool to verify
 * each address and then print them, canonicalized, to stdout. At this
 * time, the CIDR are printed in full (i.e. `255.255.255.255`).
 *
 * The tool supports a few options as follow:
 *
 * \li `--address` -- require that an address be specified otherwise we
 * generate an error.
 * \li `--port` -- require that a port be specified otherwise we generate
 * an error.
 * \li `--list` -- allow list of IP address (command & space separated).
 * \li `--mask` -- allow a mask.
 * \li `--protocol \<name>` -- specify the protocol (default "tcp").
 * \li `--default-address` -- define the default address, used when not
 * specified in an input string (i.e. `:123`).
 * \li `--default-port` -- define the default port, used when not
 * specified in an input string (i.e. `[::]`).
 *
 * To avoid receiving the error messages in stderr, use the `--quiet` flag.
 */


// libaddr lib
//
#include    "libaddr/addr.h"
#include    "libaddr/addr_exception.h"
#include    "libaddr/addr_parser.h"
#include    "libaddr/version.h"


// advgetopt lib
//
#include    <advgetopt/exception.h>
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/options.h>
#include    <advgetopt/utils.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// boost lib
//
#include    <boost/algorithm/string/join.hpp>
#include    <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace
{


advgetopt::option const g_options[] =
{
    // COMMANDS
    //
    advgetopt::define_option(
          advgetopt::Name("canonicalize")
        , advgetopt::ShortName('c')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("canonicalize the addresses and print the result in stdout.")
    ),
    advgetopt::define_option(
          advgetopt::Name("verify")
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("verify the specify addresses.")
    ),

    // OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("address")
        , advgetopt::ShortName('a')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("address is required.")
    ),
    advgetopt::define_option(
          advgetopt::Name("default-address")
        , advgetopt::Flags(advgetopt::command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1,::")
        , advgetopt::Help("default IPv4 and IPv6 addresses.")
    ),
    advgetopt::define_option(
          advgetopt::Name("default-port")
        , advgetopt::Flags(advgetopt::command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("60000")
        , advgetopt::Help("default port.")
    ),
    advgetopt::define_option(
          advgetopt::Name("list")
        , advgetopt::ShortName('l')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("specify protocol.")
    ),
    advgetopt::define_option(
          advgetopt::Name("mask")
        , advgetopt::ShortName('m')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("mask is allowed.")
    ),
    advgetopt::define_option(
          advgetopt::Name("port")
        , advgetopt::ShortName('p')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("port is required.")
    ),
    advgetopt::define_option(
          advgetopt::Name("protocol")
        , advgetopt::Flags(advgetopt::command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("tcp")
        , advgetopt::Help("specify protocol.")
    ),
    advgetopt::define_option(
          advgetopt::Name("quiet")
        , advgetopt::ShortName('q')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("do not show error messages.")
    ),

    // FILENAMES/PATHS
    //
    advgetopt::define_option(
          advgetopt::Name("--")
        , advgetopt::Flags(advgetopt::command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_NONE
                    , advgetopt::GETOPT_FLAG_MULTIPLE
                    , advgetopt::GETOPT_FLAG_DEFAULT_OPTION>())
    ),

    // END
    //
    advgetopt::end_options()
};


advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};


// until we have C++20, remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "libaddr",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [--<opt>] <config-name> ...\n"
                     "where --<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = LIBADDR_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop



class validate_ip
{
public:
                            validate_ip(int argc, char * argv[]);

    void                    run();
    int                     error_count() const;

private:
    advgetopt::getopt       f_opts;
    int                     f_error_count = 0;
};



validate_ip::validate_ip(int argc, char * argv[])
    : f_opts(g_options_environment, argc, argv)
{
}


void validate_ip::run()
{
    size_t const max(f_opts.size("--"));
    for(size_t idx(0); idx < max; ++idx)
    {
        std::string addr(f_opts.get_string("--", idx));

        addr::addr_parser p;

        p.set_default_address("127.0.0.1");
        p.set_default_address("::");
        std::string const default_address(f_opts.get_string("default-address"));
        std::string::size_type const pos(default_address.find(','));
        if(pos == std::string::npos)
        {
            p.set_default_address(default_address);
        }
        else
        {
            p.set_default_address(default_address.substr(0, pos));
            p.set_default_address(default_address.substr(pos + 1));
        }

        p.set_default_port(f_opts.get_long("default-port"));

        p.set_default_mask("255.255.255.255");
        p.set_default_mask("FF:FF:FF:FF:FF:FF:FF:FF");

        p.set_protocol(f_opts.get_string("protocol"));

        p.set_allow(addr::addr_parser::flag_t::ADDRESS, true);
        p.set_allow(addr::addr_parser::flag_t::PORT, true);

        if(f_opts.is_defined("address"))
        {
            p.set_allow(addr::addr_parser::flag_t::REQUIRED_ADDRESS, true);
        }
        if(f_opts.is_defined("port"))
        {
            p.set_allow(addr::addr_parser::flag_t::REQUIRED_PORT, true);
        }
        if(f_opts.is_defined("mask"))
        {
            p.set_allow(addr::addr_parser::flag_t::MASK, true);
        }
        if(f_opts.is_defined("list"))
        {
            p.set_allow(addr::addr_parser::flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES, true);
        }

        addr::addr_range::vector_t const range(p.parse(addr));

        if(p.has_errors())
        {
            f_error_count += p.error_count();
            if(!f_opts.is_defined("quiet"))
            {
                std::cerr << "address \""
                          << addr
                          << "\" generated "
                          << p.error_count()
                          << " error"
                          << (p.error_count() == 1 ? "" : "s")
                          << ": "
                          << p.error_messages()
                          << std::endl;
            }
        }
        else if(f_opts.is_defined("canonicalize"))
        {
            for(auto r : range)
            {
                if(r.is_range())
                {
                    std::cout << r.get_from().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL)
                              << " .. "
                              << r.get_to().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL)
                              << std::endl;
                }
                else if(r.has_from())
                {
                    std::cout << r.get_from().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL)
                              << std::endl;
                }
                else if(r.has_to())
                {
                    std::cout << r.get_to().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL)
                              << std::endl;
                }
                else
                {
                    throw addr::addr_invalid_structure("unexpected range, no from and no to defined.");
                }
            }
        }
    }
}



int validate_ip::error_count() const
{
    return f_error_count;
}




}
// no name namespace


int main(int argc, char * argv[])
{
    try
    {
        validate_ip v(argc, argv);
        v.run();
        return v.error_count() != 0 ? 1 : 0;
    }
    catch(advgetopt::getopt_exit const & e)
    {
        snap::NOTUSED(e);
        return 0;
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an error occurred: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
