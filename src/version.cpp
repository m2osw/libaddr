// Network Address -- classes functions to ease handling IP addresses
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "libaddr/addr.h"
#include    "libaddr/version.h"


// last include
//
#include    <snapdev/poison.h>



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
