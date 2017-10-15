#
# File:         src/CMakeLists.txt
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

##
## TLD parser
##
project(libaddr)

configure_file(
    libaddr/version.h.in
    ${PROJECT_BINARY_DIR}/libaddr/version.h
)

include_directories(
    ${addr_library_BINARY_DIR}/src
    ${addr_library_SOURCE_DIR}/src
    ${LIBEXCEPT_INCLUDE_DIRS}
)

add_library(addr SHARED
    addr.cpp
    addr_parser.cpp
    addr_range.cpp
    version.cpp
)

set_target_properties(addr PROPERTIES
    VERSION
        ${LIBADDR_VERSION_MAJOR}.${LIBADDR_VERSION_MINOR}
    SOVERSION
        ${LIBADDR_VERSION_MAJOR}
)

target_link_libraries(addr
    ${LIBEXCEPT_LIBRARIES}
)

install(
    TARGETS
        addr
    LIBRARY DESTINATION
        lib
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/libaddr
    DESTINATION
        include
    FILES_MATCHING PATTERN
        "libaddr/*.h"
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libaddr/version.h
    DESTINATION
        include/libaddr
)

##
## Addr command line tool
##
project(validate_ip)

include_directories(
    ${addr_library_BINARY_DIR}/src
    ${addr_library_SOURCE_DIR}/src
    ${LIBEXCEPT_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    validate_ip.cpp
)

target_link_libraries(${PROJECT_NAME}
    addr
)

install(
    TARGETS
        ${PROJECT_NAME}
    RUNTIME DESTINATION
        bin
)

# vim: ts=4 sw=4 et
