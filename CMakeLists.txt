# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/libaddr
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

cmake_minimum_required(VERSION 3.10.2)

project(addr_project)

enable_language(C)
enable_language(CXX)

#
# Include support modules
# (must be included in a project even though it is not made
# specific to that project)
#
find_package(SnapCMakeModules REQUIRED)
find_package(AdvGetOpt        REQUIRED)
find_package(CppThread        REQUIRED)
find_package(LibExcept        REQUIRED)
find_package(LibUtf8          REQUIRED)
find_package(SnapDev          REQUIRED)

SnapGetVersion( LIBADDR ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories(
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
)

add_subdirectory( cmake   )
add_subdirectory( libaddr )
add_subdirectory( tools   )
add_subdirectory( doc     )
add_subdirectory( tests   )

# vim: ts=4 sw=4 et
