// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
//
// http://snapwebsites.org/project/libaddr
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the few global functions used to define the version.
 *
 * This file includes the few functions one can use to dynamically check
 * the version of the libaddr library. If you compiled with a different
 * version, then you very certainly could have an incompatible version
 * of the interface (i.e. C++ classes cannot always work right when
 * created through modified versions of such.)
 */

// self
//
#include "libaddr/addr.h"
#include "libaddr/version.h"


namespace addr
{

/** \brief Major version of the libaddr library.
 *
 * This function returns the major version of the library at the
 * time it was compiled.
 *
 * The C++ classes are very likely different when compiled against
 * a different major version of the library. This means it is likely
 * to crash if used.
 *
 * \return The major version of the libaddr library.
 */
int get_version_major()
{
    return LIBADDR_VERSION_MAJOR;
}


/** \brief Minor version of the libaddr library.
 *
 * This function returns the minor version of the library at the
 * time it was compiled.
 *
 * The C++ classes are likely different when compiled against
 * a different minor version of the library. This means it is likely
 * to crash if used.
 *
 * \return The minor version of the libaddr library.
 */
int get_version_minor()
{
    return LIBADDR_VERSION_MINOR;
}


/** \brief Patch version of the libaddr library.
 *
 * This function returns the patch version of the library at the
 * time it was compiled.
 *
 * The C++ classes should not have changed in such a way that it
 * will crash your application when compiled against a version
 * that has a different patching version.
 *
 * \return The patch version of the libaddr library.
 */
int get_version_patch()
{
    return LIBADDR_VERSION_PATCH;
}


/** \brief The full version of the libaddr library as a string.
 *
 * This function returns a string with the major, minor, and
 * path versions of the library. The build number is not included.
 *
 * If you want to compare the version, we suggest that you use
 * the other functions: get_version_major(), get_version_minor(),
 * and get_version_patch(). This function should be used for
 * display only.
 *
 * \return The full version of the libaddr library as a string.
 */
char const * get_version_string()
{
    return LIBADDR_VERSION_STRING;
}

}
// snap_addr namespace
// vim: ts=4 sw=4 et
