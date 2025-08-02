// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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


/** \file
 * \brief The implementation of the addr_unix class.
 *
 * This file includes the implementation of the addr_unix class. The one that
 * deals with low level Unix addresses.
 */

// self
//
#include    "libaddr/addr_unix.h"
#include    "libaddr/exception.h"


// snapdev
//
#include    <snapdev/join_strings.h>
#include    <snapdev/raii_generic_deleter.h>
#include    <snapdev/tokenize_string.h>


// libutf8
//
#include    <libutf8/libutf8.h>


// C++
//
#include    <iostream>


// C
//
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>





namespace addr
{






/** \brief Create a addr_unix object that represents an unnamed socket.
 *
 * The default is to create an unnamed address. Note that you can later
 * change an address with functions such as the set_abstract() function.
 */
addr_unix::addr_unix()
{
}


/** \brief Create a addr_unix object from a binary Unix address structure.
 *
 * This function initializes this Unix object with the specified sockaddr_un
 * address.
 *
 * \param[in] un  The Unix address.
 */
addr_unix::addr_unix(sockaddr_un const & un)
{
    set_un(un);
}


/** \brief Create an addr object from a binary IPv6 address.
 *
 * This function initializes this addr object with the specified IPv6
 * address. The is_ipv4() function will return false.
 *
 * \note
 * In this case, the \p address parameter is expected to be a bare
 * address. If you have a URI address, make sure to create the
 * addr_unix address and then call the set_uri() function.
 *
 * \param[in] address  A Unix address in a string.
 * \param[in] abstract  Whether this address is considered abstract.
 */
addr_unix::addr_unix(std::string const & address, bool abstract)
{
    if(abstract)
    {
        set_abstract(address);
    }
    else if(!address.empty())
    {
        set_file(address);
    }
    //else -- this is the default so we don't need to call that function
    //{
    //    make_unnamed();
    //}
}


/** \brief Define a scheme name.
 *
 * This parameter is used when you convert the Unix address back to a string
 * using to_uri(). It is also set when you use the set_uri().
 *
 * The default is set to "unix", which is not really a scheme per se, but
 * for the purpose it works.
 *
 * If you pass an empty string, the scheme is reset to the default ("unix").
 *
 * \param[in] scheme  The name of the scheme.
 */
void addr_unix::set_scheme(std::string const & scheme)
{
    f_scheme = scheme == "unix" ? std::string() : scheme;
}


/** \brief Save a Unix address in this addr_unix object.
 *
 * This function saves the specified Unix address in this addr_unix object.
 *
 * Any type of address can be saved. The function determines what type
 * this address represents (i.e. file, abstract, or unnamed).
 *
 * \warning
 * This function does not just copy the input address in this addr_unix
 * object. It applies our detection mechanism to detect which type
 * of address you are supplying and then call one of the set_file(),
 * set_abstract(), or make_unnamed() functions. Therefore, the function
 * may throw because the address is not considered compatible (invalid
 * UTF-8, includes control characters, etc.) Also, in the case of an
 * abstract name, if it includes '\0' characters, then it won't work
 * as expected. These will not be detected properly.
 *
 * \todo
 * I think that to properly support the abstract name we should offer
 * a size input parameter so we know of the exact size of the address.
 *
 * \exception addr_invalid_argument
 * The input address must be of type AF_UNIX or AF_LOCAL.
 *
 * \param[in] un  The Unix address to save in this addr_unix object.
 */
void addr_unix::set_un(struct sockaddr_un const & un)
{
    if(un.sun_family != AF_UNIX)
    {
        throw addr_invalid_structure("addr_unix::set_un(): the input address does not represent a Unix address (family is not AF_UNIX).");
    }

    if(un.sun_path[0] != '\0')
    {
        std::size_t const len(strnlen(un.sun_path, sizeof(un.sun_path)));
        if(len == sizeof(un.sun_path))
        {
            throw addr_invalid_argument("addr_unix::set_un(): the input address filename is too long.");
        }

        // this is considered a file address
        // (we do not support a missing '\0' at the end of the name, but
        // the input could be missing it so we have to make sure by creating
        // a temp version of the name)
        //
        set_file(std::string(un.sun_path, len));
    }
    else if(un.sun_path[1] != '\0')
    {
        std::size_t const len(strnlen(un.sun_path + 1, sizeof(un.sun_path) - 1));
        if(len == sizeof(un.sun_path) - 1)
        {
            throw addr_invalid_argument("addr_unix::set_un(): the input abstract name is too long.");
        }
        set_abstract(std::string(un.sun_path + 1, len));
    }
    else
    {
        make_unnamed();
    }
}


/** \brief Change this Unix address in an unnamed Unix address.
 *
 * Transform this Unix address in an unnamed Unix address.
 *
 * \note
 * Linux accepts an abstract address as having the first sun_path
 * character set to '\0' and all the other characters set to anything
 * including all '\0'. We handle our abstract addresses as not supporting
 * zeroes. Therefore, we do not 100% match Linux.
 */
void addr_unix::make_unnamed()
{
    memset(f_address.sun_path, 0, sizeof(f_address.sun_path));
}


/** \brief Set the this Unix address to the specified filename.
 *
 * This function changes address to the specified filename.
 *
 * The path does not need to be a full path. However, relative paths are
 * often considered dangerous.
 *
 * \exception addr_invalid_argument
 * If the input doesn't look like a valid filename, then the function
 * raises this exception.
 *
 * \param[in] file  The path to a file.
 */
void addr_unix::set_file(std::string const & file)
{
    std::string const address(verify_path(file, false));

    memcpy(f_address.sun_path, address.c_str(), address.length());
    if(address.length() < sizeof(f_address.sun_path))
    {
        // clear the rest
        //
        memset(
              f_address.sun_path + address.length()
            , 0
            , sizeof(f_address.sun_path) - address.length());
    }
}


/** \brief Define the permissions of the socket file.
 *
 * This is used to set the permissions of the socket when creating a socket
 * file. Under Linux we can set that on the socket with:
 *
 * \code
 *     int s = socket(...);
 *     fchmod(s, mode);
 * \endcode
 *
 * Other systems may require to set the mode after the `bind()` using a
 * standard `chmod()` on the file by name. Also some system ignore the
 * permissions so it would not matter at all.
 *
 * The default value is DEFAULT_MODE (0600 at time of writing) which means
 * only programs running as the same user can access the socket. To make
 * the socket accessible by others in the same group, use 0660. To let
 * anyone running on the same server access the socket, use 0666.
 *
 * Note that the parent folder permissions are also checked. It needs to
 * have the 'x' flag set for the type of user that you authorize. The folder
 * should not have the 'w' access or those users could temper with the file
 * (i.e. delete it).
 *
 * \param[in] mode  The mode to set the socket to once created.
 */
void addr_unix::set_mode(int mode)
{
    f_mode = mode;
}


/** \brief Name of a group used to change the socket ownership.
 *
 * This function saves the name of a group that will be used to change the
 * ownership of the socket after the bind() and before the chmod().
 *
 * This way you can better control which users have access to your services.
 *
 * If \p group is set to the empty string, then the socket file group
 * ownerhship will not be changed.
 *
 * \param[in] group  The name of the group to use on the socket file.
 */
void addr_unix::set_group(std::string const & group)
{
    f_group = group;
}


/** \brief Set the address to an Abstract Unix Address.
 *
 * This function saves the specified \p abstract path as an Abstract Unix
 * Address in this object.
 *
 * Note that we check the \p abstract path as if it were a file path. We
 * forbid control characters, and it has to be at least one character and
 * at most 106 characters.
 *
 * The function generates an abstract path. That means the file won't be
 * created on disk. You should choose a path which is not going to conflict
 * with any other system. It is conventional, in the dbus area, to make use
 * of your domain name in reverse order (Java-like). For example:
 *
 * \code
 *     u.set_abstract("/net/snapwebsite/settings");
 * \endcode
 *
 * \note
 * The Linux OS supports abstract paths which include any byte. However,
 * there are tools to look at those paths as strings, so limiting these
 * path to valid strings is something we think makes sense. It also
 * makes it possible for us to distinguish between unnamed and abstract
 * addresses without the need for another parameter such as an address
 * size.
 *
 * \exception addr_invalid_argument
 * If the input \p abstract path is not considered valid, then this
 * exception is raised.
 *
 * \param[in] abstract  The abstract path to save in this Unix address.
 */
void addr_unix::set_abstract(std::string const & abstract)
{
    std::string const address(verify_path(abstract, true));

    f_address.sun_path[0] = '\0';
    strncpy(f_address.sun_path + 1, address.c_str(), sizeof(f_address.sun_path) - 1);
    if(address.length() < sizeof(f_address.sun_path) - 2)
    {
        // clear the rest
        //
        memset(
              f_address.sun_path + address.length() + 2
            , 0
            , sizeof(f_address.sun_path) - address.length() - 2);
    }
}


/** \brief Allow for URI like notation to set a Unix address.
 *
 * Since we often use a preference setting which is a string to define our
 * addresses, having a notation that clearly distinguishes between the
 * Unix address types seems to make sense.
 *
 * We support the following basic URI syntax:
 *
 * \code
 *     <scheme>:[<path>][?<mode>]
 * \endcode
 *
 * Where `\<scheme>` has to be a valid URI scheme. RFC 3986 defines the
 * valid characters in a scheme:
 *
 * \code
 *     scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
 * \endcode
 *
 * You can later retrieve the scheme with the get_scheme() function.
 *
 * Where `\<path>` is the path to the file or abstract filename. For unnamed
 * sockets, it must be an empty string. If it starts with more than one
 * slash, extraneous slashes are removed (actually, any extraneous slashes
 * within the name are removed).
 *
 * Where `\<mode>` must be one of:
 *
 * * `?unnamed` -- create an unnamed address, the `\<path>` must be empty
 * * `?file` -- create a file based address, the `\<path>` cannot be empty
 * * `?abstract` -- create an abstract address
 *
 * By default, when the `\<mode>` is not specified, the function creates
 * a file based address unless the address is empty in which case it
 * creates an unnamed address. The only way to create an abstract name
 * is by specifying the `?abstract` query string.
 *
 * \note
 * We do not use/support the `file:///` scheme because this is a reference
 * to a specific file, not a socket. We think that `unix:` is better adapted
 * to our situation.
 *
 * \warning
 * At the moment the parsing of the string is very basic. You must make sure
 * not to include anything more than what is allowed as presented above.
 *
 * \todo
 * Use the snap_uri once we extracted that in its own library.
 *
 * \exception addr_invalid_argument
 * If the URI can't be properly parsed, this function raises this exception.
 *
 * \param[in] uri  The URI to save in this address.
 */
void addr_unix::set_uri(std::string const & uri)
{
    enum class uri_force_t
    {
        URI_FORCE_NONE,
        URI_FORCE_UNNAMED,
        URI_FORCE_FILE,
        URI_FORCE_ABSTRACT,
    };

    std::string::size_type const scheme_pos(uri.find(':'));
    if(scheme_pos == std::string::npos)
    {
        throw addr_invalid_argument("invalid URI for a Unix address, scheme not found (':' missing).");
    }

    std::string const scheme(uri.substr(0, scheme_pos));
    if(scheme.empty())
    {
        throw addr_invalid_argument("the scheme of a URI cannot be an empty string.");
    }
    for(auto it(scheme.begin()); it != scheme.end(); ++it)
    {
        std::string::value_type const c(*it);
        if((c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && (it == scheme.begin()
            || ((c < '0' || c > '9') && c != '+' && c != '-' && c != '.')))
        {
            // as per RFC 3986
            // https://datatracker.ietf.org/doc/html/rfc3986
            //
            throw addr_invalid_argument(
                  "\""
                + scheme
                + "\" is not a supported URI scheme for a Unix address;"
                  " supported scheme are limited to `[a-zA-Z][-+.a-zA-Z0-9]*`.");
        }
    }

    std::string address;
    std::string query;

    std::string::size_type query_pos(uri.find('?', scheme_pos + 1));
    if(query_pos == std::string::npos)
    {
        address = uri.substr(scheme_pos + 1);
    }
    else
    {
        address = uri.substr(scheme_pos + 1, query_pos - scheme_pos - 1);
        query = uri.substr(query_pos + 1);
    }

    uri_force_t force(uri_force_t::URI_FORCE_NONE);
    if(!query.empty())
    {
        if(query == "unnamed")
        {
            force = uri_force_t::URI_FORCE_UNNAMED;
        }
        else if(query == "file")
        {
            force = uri_force_t::URI_FORCE_FILE;
        }
        else if(query == "abstract")
        {
            force = uri_force_t::URI_FORCE_ABSTRACT;
        }
        else
        {
            throw addr_invalid_argument(
                  "\""
                + query
                + "\" is not a supported URI query string for a Unix address;"
                  " supported query strings are one of: \"unnamed\", \"file\" and \"abstract\".");
        }
    }

    switch(force)
    {
    case uri_force_t::URI_FORCE_NONE:
        if(address.empty())
        {
            make_unnamed();
        }
        else
        {
            set_file(address);
        }
        break;

    case uri_force_t::URI_FORCE_UNNAMED:
        if(!address.empty())
        {
            throw addr_invalid_argument(
                  "address \""
                + address
                + "\" must be empty to represent an unnamed Unix address.");
        }
        make_unnamed();
        break;

    case uri_force_t::URI_FORCE_FILE:
        set_file(address);
        break;

    case uri_force_t::URI_FORCE_ABSTRACT:
        set_abstract(address);
        break;

    }

    // unix is the default so we don't have to save it
    //
    set_scheme(scheme);
}


/** \brief Define this address given an open socket.
 *
 * This function tries to retrieve the address of a socket. If the function
 * succeeds, then it returns true meaning that the address is considered
 * valid.
 *
 * The function verifies that the address is indeed a Unix address. If not,
 * then it fails with EADDRNOTAVAIL and this object is not modified.
 *
 * \warning
 * The function makes sure that the name of the socket fits a
 * sockaddr_un.sun_path as per our rules (i.e. we force the presence
 * of the '\0' terminator). So this function may throw an exception
 * on Linux which supports socket names without the '\0'. Further,
 * if the name is not considered compatible with our verify_path()
 * function.
 *
 * \return true if the address was successfully retrieved.
 */
bool addr_unix::set_from_socket(int s)
{
    // WARNING: the sockaddr (or sockaddr_storage) structure that the
    //          getsockname() expects is not large enough for a Unix
    //          socket so we use a sockaddr_un instead
    //
    sockaddr_un address;
    socklen_t length(sizeof(address));
    if(getsockname(s, reinterpret_cast<sockaddr *>(&address), &length) != 0)
    {
        return false;
    }

    if(address.sun_family != AF_UNIX)
    {
        errno = EADDRNOTAVAIL;
        return false;
    }

    if(length < sizeof(address))
    {
        memset(
                  reinterpret_cast<char *>(&address) + length
                , 0
                , sizeof(address) - length);
    }

    set_un(address);

    return true;
}


/** \brief Check whether this address represents file based Unix address.
 *
 * The function checks whether the path starts with a character other
 * than '\0'.
 *
 * \return true if this addr represents a file based Unix address.
 */
bool addr_unix::is_file() const
{
    return f_address.sun_path[0] != '\0';
}


/** \brief Check whether this address represents an abstract Unix address.
 *
 * This function checks whether this Unix address represet an abstract
 * Unix address.
 *
 * An abstract Unix address has the first byte of the path set to '\0'
 * and the second not set to '\0'.
 *
 * \return true if this address represents an abstract Unix address.
 */
bool addr_unix::is_abstract() const
{
    return f_address.sun_path[0] == '\0'
        && f_address.sun_path[1] != '\0';
}


/** \brief Check whether this address represents an unnamed Unix address.
 *
 * This function checks whether the path represents and unnamed Unix address.
 *
 * An unnamed Unix address has all the bytes of the `sun_path` string set
 * to '\0', but we really only need to test the first two.
 *
 * \return true if the address represents an unnamed Unix address.
 */
bool addr_unix::is_unnamed() const
{
    return f_address.sun_path[0] == '\0'
        && f_address.sun_path[1] == '\0';
}


/** \brief Retrieve the protocol.
 *
 * This function retrive the protocol as defined by the set_scheme().
 *
 * The default value is "unix", which is not really a scheme, but a
 * default is required for the to_uri() function to work properly.
 *
 * \return The current scheme.
 */
std::string addr_unix::get_scheme() const
{
    return f_scheme.empty() ? "unix" : f_scheme;
}


/** \brief Retrieve a copy of this Unix address.
 *
 * This function returns the Unix address as currently defined in this
 * addr_unix object.
 *
 * The address is distinguished between an unnamed address and an
 * abstract name by the fact that `sun_path[1] != '\0'` for the
 * abstract path.
 *
 * \param[out] un  The structure where the address gets saved.
 */
void addr_unix::get_un(sockaddr_un & un) const
{
    memcpy(&un, &f_address, sizeof(un));
}


/** \brief Retrieve the mode as set by set_mode().
 *
 * This function returns the mode expected to be used if creating a file
 * socket.
 *
 * If the set_mode() function doesn't get called, the default permissions
 * are to be set to 0600.
 *
 * \return The mode to use as in: `fchmod(s, mode)`.
 *
 * \sa set_mode()
 */
int addr_unix::get_mode() const
{
    return f_mode;
}


/** \brief Get the name of the group.
 *
 * This function returns the name of the group to be used on the socket file.
 * If the string is empty, then the group does not get changed. Otherwise,
 * you are expected to use chgrp() in order to change the ownership to this
 * named group.
 *
 * \return The name of the expected socket file group ownership.
 */
std::string addr_unix::get_group() const
{
    return f_group;
}


/** \brief Return the path of this Unix address.
 *
 * This function returns the path of the Unix address. Note that you can know
 * if it was an unnamed address, the string will be empty. However, you can't
 * distinguish between a file or abstract name with this function. You can
 * always use the is_file() and is_abstract() to know.
 *
 * \return the path of this Unix address.
 */
std::string addr_unix::to_string() const
{
    if(is_abstract())
    {
        return f_address.sun_path + 1;
    }

    return f_address.sun_path;
}


/** \brief Get the network type string
 *
 * Translate the network type into a string, which can be really useful
 * to log that information.
 *
 * Note that PUBLIC is the same as UNKNOWN, this function returns
 * "Unknown" in that case, though.
 *
 * \return The string representing the type of network.
 */
std::string addr_unix::to_uri() const
{
    std::string result;
    result.reserve(125);

    result = get_scheme();
    result += ':';

    if(!is_unnamed())
    {
        char const * path(nullptr);
        std::string query;
        if(is_file())
        {
            path = f_address.sun_path;
        }
        else
        {
            path = f_address.sun_path + 1;
            query = "?abstract";
        }

        if(path[0] == '/')
        {
            result += "//";
        }
        result += path;
        result += query;
    }

    return result;
} // LCOV_EXCL_LINE


/** \brief Delete the socket file.
 *
 * This function will delete the socket file if it exists.
 *
 * The function does nothing if the address is not representing a file.
 * In that case, the function returns 0 and does not modify the errno
 * variable.
 *
 * \note
 * This function does not verify whether the file is in use. That means
 * you may delete a file that should not be deleted. It is your
 * responsibility to verify the current state of the socket. The
 * ed::local_stream_server_connection implementation takes care of
 * that for you if you want to have it easy.
 *
 * \return 0 if the unlink worked, -1 on error and errno is set.
 */
int addr_unix::unlink()
{
    if(is_file())
    {
        return ::unlink(f_address.sun_path);
    }

    return 0;
}


/** \brief Check whether two addresses are equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) for equality. If both represent the same IP
 * address, then the function returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, scheme are all ignored. You can
 * compare the scheme using the get_scheme() of lhs and rhs and
 * compare. The family is expected to by AF_UNIX in both so there
 * should be no need to compare.
 *
 * \param[in] rhs  The other Unix address to compare against.
 *
 * \return true if \p this is equal to \p rhs.
 */
bool addr_unix::operator == (addr_unix const & rhs) const
{
    return f_address == rhs.f_address;
}


/** \brief Check whether two addresses are not equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) for inequality. If both represent the same IP
 * address, then the function returns false.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \param[in] rhs  The other Unix address to compare against.
 *
 * \return true if \p this is not equal to \p rhs.
 */
bool addr_unix::operator != (addr_unix const & rhs) const
{
    return f_address != rhs.f_address;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \param[in] rhs  The other Unix address to compare against.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr_unix::operator < (addr_unix const & rhs) const
{
    return f_address < rhs.f_address;
}


/** \brief Compare two addresses to know which one is smaller or equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know whether the left hand side is smaller or
 * equal to thr right handside.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \param[in] rhs  The other Unix address to compare against.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr_unix::operator <= (addr_unix const & rhs) const
{
    return f_address <= rhs.f_address;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \param[in] rhs  The other Unix address to compare against.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr_unix::operator > (addr_unix const & rhs) const
{
    return f_address > rhs.f_address;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \param[in] rhs  The other Unix address to compare against.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr_unix::operator >= (addr_unix const & rhs) const
{
    return f_address >= rhs.f_address;
}


/** \brief Check whether the specified \p path is a valid path.
 *
 * Although there are even less restrictions on the path of an abstract
 * socket, we force you to have a valid UTF-8 string without any control
 * characters (especially not a '\0' character).
 *
 * The function makes sure to remove double slashes from the path. So a
 * path such as:
 *
 * \code
 *     /run/communicatord//communicator.sock
 * \endcode
 *
 * is canonicalized to:
 *
 * \code
 *     /run/communicatord/communicator.sock
 * \endcode
 *
 * The length of the string is also checked. If it cannot fit in the `sun_path`
 * buffer with a terminal '\0', then an exception is raised.
 *
 * \exception addr_invalid_argument
 * If \p path is empty, too long, or any invalid character is found within
 * the string, then this exception is raised.
 *
 * \exception libutf8::libutf8_exception_decoding
 * If an invalid UTF-8 character is found, then this exception is raised.
 *
 * \param[in] path  The path to be validated.
 * \param[in] abstract  Whether this is an abstract path (true) or not.
 *
 * \return A canonicalized version of \p path.
 */
std::string addr_unix::verify_path(std::string const & path, bool abstract)
{
    if(path.empty())
    {
        throw addr_invalid_argument(
              std::string(abstract ? "an abstract" : "a Unix")
            + " filename can't be empty;"
              " use make_empty() if you want to use an unnamed socket.");
    }

    std::size_t const max_length(abstract
                ? sizeof(f_address.sun_path) - 1
                : sizeof(f_address.sun_path));

    std::vector<std::string> segments;
    snapdev::tokenize_string(segments, path, "/", true);
    std::string p(snapdev::join_strings(segments, "/"));
    if(path[0] == '/')
    {
        p.insert(0, 1, '/');
    }

    // this forces a '\0' at the end of the sun_path buffer
    //
    if(p.length() >= max_length)
    {
        throw addr_invalid_argument(
              std::string(abstract ? "an abstract" : "a Unix")
            + " filename is limited to "
            + std::to_string(max_length)
            + " characters.");
    }

    std::u32string u32(libutf8::to_u32string(p));
    for(auto const & c : u32)
    {
        if(c <= 0x1F                    // controls
        || (c >= 0x7F && c <= 0x9F))    // graphic controls
        {
            throw addr_invalid_argument(
                  "path \""
                + path
                + "\" is not a valid UTF-8 string (it includes controls).");
        }
    }

    if(p == "/")
    {
            throw addr_invalid_argument(
                "the root path (\"/\") is not a valid socket filename.");
    }

    return p;
}



}
// namespace addr
// vim: ts=4 sw=4 et
