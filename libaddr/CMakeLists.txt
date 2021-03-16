# Copyright (c) 2011-2021  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/libaddr
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


##
## addr library
##
project(addr)

configure_file(
    version.h.in
    ${PROJECT_BINARY_DIR}/version.h
)

include_directories(
    ${LIBEXCEPT_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} SHARED
    addr.cpp
    addr_parser.cpp
    addr_range.cpp
    iface.cpp
    route.cpp
    version.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${LIBADDR_VERSION_MAJOR}.${LIBADDR_VERSION_MINOR}

    SOVERSION
        ${LIBADDR_VERSION_MAJOR}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBEXCEPT_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib
)

install(
    FILES
        addr.h
        addr_exception.h
        addr_parser.h
        addr_range.h
        iface.h
        route.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/libaddr
)


# vim: ts=4 sw=4 et
