/* test_addr_main.cpp
 * Copyright (C) 2011-2017  Made to Order Software Corporation
 *
 * Project: http://snapwebsites.org/project/libaddr
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and
 * associated documentation files (the "Software"), to
 * deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 * \brief The main() function of the addr library tests.
 *
 * This file implements the main() function of the unit tests used to
 * verify the addr library.
 *
 * It defines any globals (none at this point) and a few basic command
 * line options such as --help and --version.
 */

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

// self
//
#include "test_addr_main.h"




namespace unittest
{

int         g_tcp_port = -1;

}


namespace
{
    struct UnitTestCLData
    {
        bool        help = false;
        bool        version = false;
        int         seed = 0;
        int         tcp_port = -1;
    };

    void remove_from_args( std::vector<std::string> & vect, std::string const & long_opt, std::string const & short_opt )
    {
        auto iter = std::find_if( vect.begin(), vect.end(), [long_opt, short_opt]( std::string const & arg )
        {
            return arg == long_opt || arg == short_opt;
        });

        if( iter != vect.end() )
        {
            auto next_iter = iter;
            vect.erase( ++next_iter );
            vect.erase( iter );
        }
    }
}
// namespace


int test_addr_main(int argc, char * argv[])
{
    UnitTestCLData configData;
    Catch::Clara::CommandLine<UnitTestCLData> cli;

    cli["-?"]["-h"]["--help"]
        .describe("display usage information")
        .bind(&UnitTestCLData::help);

    cli["-S"]["--seed"]
        .describe("value to seed the randomizer, if not specified, randomize")
        .bind(&UnitTestCLData::seed, "seed");

    cli["--tcp-port"]
        .describe("define a TCP port we can connect to to test the get_from_socket() function")
        .bind(&UnitTestCLData::tcp_port, "port");

    cli["-V"]["--version"]
        .describe("print out the advgetopt library version these unit tests pertain to")
        .bind(&UnitTestCLData::version);

    cli.parseInto( argc, argv, configData );

    if( configData.help )
    {
        cli.usage( std::cout, argv[0] );
        Catch::Session().run(argc, argv);
        exit(1);
    }

    if( configData.version )
    {
        std::cout << LIBADDR_VERSION_STRING << std::endl;
        exit(0);
    }

    std::vector<std::string> arg_list;
    for( int i = 0; i < argc; ++i )
    {
        arg_list.push_back( argv[i] );
    }

    // by default we get a different seed each time; that really helps
    // in detecting errors! (I know, I wrote loads of tests before)
    //
    unsigned int seed(static_cast<unsigned int>(time(NULL)));
    if( configData.seed != 0 )
    {
        seed = static_cast<unsigned int>(configData.seed);
        remove_from_args( arg_list, "--seed", "-S" );
    }
    srand(seed);
    std::cout << "libaddr[" << getpid() << "]:test: seed is " << seed << std::endl;

    // make a copy of the port specified on the command line
    //
    if( configData.tcp_port != -1 )
    {
        unittest::g_tcp_port = configData.tcp_port;
        remove_from_args( arg_list, "--tcp-port", "" );
    }

    std::vector<char *> new_argv;
    std::for_each( arg_list.begin(), arg_list.end(), [&new_argv]( const std::string& arg )
    {
        new_argv.push_back( const_cast<char *>(arg.c_str()) );
    });

    return Catch::Session().run( static_cast<int>(new_argv.size()), &new_argv[0] );
}


int main(int argc, char * argv[])
{
    int r(1);

    try
    {
        r = test_addr_main(argc, argv);
    }
    catch(std::logic_error const & e)
    {
        std::cerr << "fatal error: caught a logic error in libaddr unit tests: " << e.what() << "\n";
    }

    return r;
}

// vim: ts=4 sw=4 et
