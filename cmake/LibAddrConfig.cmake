# - Find LibAddr
#
# LIBADDR_FOUND        - System has LibAddr
# LIBADDR_INCLUDE_DIRS - The LibAddr include directories
# LIBADDR_LIBRARIES    - The libraries needed to use LibAddr
# LIBADDR_DEFINITIONS  - Compiler switches required for using LibAddr
#
# License:
#
# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/libaddr
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_path(
    LIBADDR_INCLUDE_DIR
        libaddr/addr.h

    PATHS
        ENV LIBADDR_INCLUDE_DIR
)
find_library(
    LIBADDR_LIBRARY
        addr

    PATHS
        ${LIBADDR_LIBRARY_DIR}
        ENV LIBADDR_LIBRARY
)

mark_as_advanced(
    LIBADDR_INCLUDE_DIR
    LIBADDR_LIBRARY
)

set(LIBADDR_INCLUDE_DIRS ${LIBADDR_INCLUDE_DIR})
set(LIBADDR_LIBRARIES    ${LIBADDR_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LibAddr
    REQUIRED_VARS
        LIBADDR_INCLUDE_DIR
        LIBADDR_LIBRARY
)

# vim: ts=4 sw=4 et
