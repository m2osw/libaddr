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
#pragma once

/** \file
 * \brief The Unix libaddr class.
 *
 * This header defines the Unix address class. This is used to connect to
 * Unix sockets.
 *
 * The library supports all three types of Unix addresses supported by
 * Linux:
 *
 * * File based: a path to a file on disk.
 * * Abstract: a abstract path.
 * * Unnamed: a completely unnamed socket.
 *
 * The first one (File) is what most users are expected to use.
 *
 * The second one (Abstract) is what many X11 tools use to communicate.
 * Especially, it is called the "bus" in Gnome.
 *
 * The third one (Unnamed) is only useful for parent/child communication.
 * The child can be a process created by fork() and fork() + exec().
 */

// C++ library
//
#include    <memory>
#include    <string>
#include    <vector>


// C library
//
#include    <sys/un.h>
#include    <sys/socket.h>



namespace addr
{




/** \brief Initialize a Unix address as such.
 *
 * This function initializes a sockaddr_un with all zeroes except
 * for the sun_family which is set to AF_UNIX.
 *
 * return The initialized Unix address.
 */
constexpr struct sockaddr_un init_un()
{
    struct sockaddr_un un = sockaddr_un();
    un.sun_family = AF_UNIX;
    return un;
}


class unix
{
public:
    typedef std::shared_ptr<unix>   pointer_t;
    typedef std::vector<unix>       vector_t;
    typedef int                     socket_flag_t;

                                    unix();
                                    unix(sockaddr_un const & un);
                                    unix(std::string const & address, bool abstract = false);

    void                            set_un(sockaddr_un const & un);
    void                            make_unnamed();
    void                            set_file(std::string const & address);
    void                            set_abstract(std::string const & address);
    void                            set_uri(std::string const & address);

    bool                            is_file() const;
    bool                            is_abstract() const;
    bool                            is_unnamed() const;
    void                            get_un(sockaddr_un & un) const;
    std::string                     to_string() const;
    std::string                     to_uri() const;

    bool                            operator == (unix const & rhs) const;
    bool                            operator != (unix const & rhs) const;
    bool                            operator <  (unix const & rhs) const;
    bool                            operator <= (unix const & rhs) const;
    bool                            operator >  (unix const & rhs) const;
    bool                            operator >= (unix const & rhs) const;

private:
    std::string                     verify_path(std::string const & path, bool abstract);

    sockaddr_un                     f_address = init_un();
};





}
// namespace addr


inline bool operator == (sockaddr_un const & a, sockaddr_un const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_un)) == 0;
}


inline bool operator != (sockaddr_un const & a, sockaddr_un const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_un)) != 0;
}


inline bool operator < (sockaddr_un const & a, sockaddr_un const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_un)) < 0;
}


inline bool operator <= (sockaddr_un const & a, sockaddr_un const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_un)) <= 0;
}


inline bool operator > (sockaddr_un const & a, sockaddr_un const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_un)) > 0;
}


inline bool operator >= (sockaddr_un const & a, sockaddr_un const & b)
{
    return memcmp(&a, &b, sizeof(sockaddr_un)) >= 0;
}



// vim: ts=4 sw=4 et
