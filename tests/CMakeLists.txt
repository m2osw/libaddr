#
# File:         tests/CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2011-2017 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/project/libaddr
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Setup Catch
#
find_package( Catch )
if( ${CATCH_INCLUDE_DIR} STREQUAL "CATCH_INCLUDE_DIR-NOTFOUND" )
    message( FATAL_ERROR "catch.hpp not found! Please run 'apt install catch'." )
endif()
include_directories(
    #SYSTEM
    ${CATCH_INCLUDE_DIR}
)

##
## libaddr tests
##
project(addr_test)

# The reference to the src folder is required by the internal test
include_directories(
    ${addr_library_BINARY_DIR}/src
    ${addr_library_SOURCE_DIR}/src
    ${LIBEXCEPT_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    test_addr_main.cpp
    test_addr_global.cpp
    test_addr_interfaces.cpp
    test_addr_ipv4.cpp
    test_addr_ipv6.cpp
    test_addr_range.cpp
)

target_link_libraries(${PROJECT_NAME}
    addr
)


##
## Run all the tests
##
project(run_libaddr_tests)

add_custom_target(run_libaddr_tests
    COMMAND addr_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
        addr_test
)

# vim: ts=4 sw=4 et
