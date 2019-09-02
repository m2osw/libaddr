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
 * \brief The implementation of the addr class.
 *
 * This file includes the implementation of the addr class. The one that
 * deals with low level classes.
 */

// self
//
#include    "libaddr/iface.h"


// addr library
//
#include    "libaddr/route.h"


// C++ library
//
#include    <algorithm>
#include    <iostream>


// C library
//
#include    <ifaddrs.h>
#include    <net/if.h>


// last include
//
#include    <snapdev/poison.h>



namespace addr
{


/** \brief Details used by the addr class implementation.
 *
 * We have a function to check whether an address is part of
 * the interfaces of your computer. This check requires the
 * use of a `struct ifaddrs` and as such it requires to
 * delete that structure. We define a deleter for that
 * strucure here.
 */
namespace
{

/** \brief Delete an ifaddrs structure.
 *
 * This deleter is used to make sure all the ifaddrs get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ia  The ifaddrs structure to free.
 */
void ifaddrs_deleter(struct ifaddrs * ia)
{
    freeifaddrs(ia);
}


}
// no name namespace





/** \brief Initializes an interface name/index pair.
 *
 * This function creates an interface name/index object.
 *
 * In some circumstances (NETLINK) you need to specify the index of an
 * interface. The kernel keeps a list of interface by index starting at 1
 * and each have a name such as "eth0". This function initializes
 * one of those pairs.
 *
 * \param[in] index  The index of the interface.
 * \param[in] name  The name of the interface.
 */
iface_index_name::iface_index_name(int index, std::string const & name)
    : f_index(index)
    , f_name(name)
{
}


/** \brief Get the index of this interface.
 *
 * This function is used to retrieve the index of a name/index pair.
 *
 * \return The index of the name/index pair.
 */
int iface_index_name::get_index() const
{
    return f_index;
}


/** \brief Get the name of this interface.
 *
 * This function is used to retrieve the name of a name/index pair.
 *
 * \return The name of the name/index pair.
 */
std::string const & iface_index_name::get_name() const
{
    return f_name;
}



/** \brief Get the list of existing interfaces.
 *
 * This function gathers the complete list of interfaces by index and
 * name pairs. The result is a vector of iface_index_name objects.
 *
 * Note that over time the index of an interface can change since interfaces
 * can be added and removed from your network configuration. It is a good
 * idea to not cache that information.
 *
 * \return A vector of index/name pair objects.
 */
iface_index_name::vector_t get_interface_name_index()
{
    iface_index_name::vector_t result;

    // the index starts at 1
    //
    for(int index(1);; ++index)
    {
        // get the next interface name by index
        //
        char name[IF_NAMESIZE + 1];
        if(if_indextoname(index, name) == nullptr)
        {
            return result;
        }

        // make sure the name is null terminated
        //
        name[IF_NAMESIZE] = '\0';

        iface_index_name const in(index, name);
        result.push_back(in);
    }
    // NOT REACHED, the loop is broken by a "return"
}


/** \brief Get the index of an interface from its name.
 *
 * If you are given the name of an interface, you can retrieve its index
 * by calling this function. The resulting value is the index from 1 to n.
 *
 * If the named interface is not found, then the function returns 0.
 *
 * \return The interface index or 0 on error.
 */
unsigned int get_interface_index_by_name(std::string const & name)
{
    return if_nametoindex(name.c_str());
}






/** \brief Return a list of local addresses on this machine.
 *
 * Peruse the list of available interfaces, and return any detected
 * ip addresses in a vector.
 *
 * These addresses include:
 *
 * \li A mask whenever available (very likely if the interface is up).
 * \li A name you can retrieve with get_iface_name()
 * \li A set of flags defining the current status of the network interface
 *     (i.e. IFF_UP, IFF_BROADCAST, IFF_NOARP, etc.)
 *
 * \return A vector of all the local interface IP addresses.
 */
iface::vector_t iface::get_local_addresses()
{
    // get the list of interface addresses
    //
    struct ifaddrs * ifa_start(nullptr);
    if(getifaddrs(&ifa_start) != 0)
    {
        // TODO: Should this throw, or just return an empty list quietly?
        //
        return iface::vector_t(); // LCOV_EXCL_LINE
    }

    std::shared_ptr<struct ifaddrs> auto_free(ifa_start, ifaddrs_deleter);

    uint8_t mask[16];
    iface::vector_t iface_list;
    for(struct ifaddrs * ifa(ifa_start); ifa != nullptr; ifa = ifa->ifa_next)
    {
        // the documentation says there may be no address at all
        // skip such entries at the moment
        //
        if(ifa->ifa_addr == nullptr)
        {
            continue;
        }

        // initialize an interface
        iface the_interface;

        // copy the name and flags as is
        //
        // TBD: can the ifa_name even be a null pointer?
        //
        the_interface.f_name = ifa->ifa_name;
        the_interface.f_flags = ifa->ifa_flags; // IFF_... flags (see `man 7 netdevice` search for SIOCGIFFLAGS)

        // get the family to know how to treat the address
        //
        // when an interface has an IPv4 and an IPv6, there are two entries in
        // the list, both with the same name
        //
        uint16_t const family(ifa->ifa_addr->sa_family);

        switch(family)
        {
        case AF_INET:
            the_interface.f_address.set_ipv4(*(reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)));

            if((ifa->ifa_flags & IFF_BROADCAST) != 0
            && ifa->ifa_broadaddr != nullptr)
            {
                the_interface.f_broadcast_address.set_ipv4(*(reinterpret_cast<struct sockaddr_in *>(ifa->ifa_broadaddr)));
            }
            if((ifa->ifa_flags & IFF_POINTOPOINT) != 0
            && ifa->ifa_dstaddr != nullptr)  // LCOV_EXCL_LINE
            {
                the_interface.f_destination_address.set_ipv4(*(reinterpret_cast<struct sockaddr_in *>(ifa->ifa_dstaddr)));  // LCOV_EXCL_LINE
            }

            // if present, add the mask as well
            //
            if(ifa->ifa_netmask != nullptr)
            {
                // for the IPv4 mask, we have to break it down in such a
                // way as to make it IPv6 compatible
                //
                memset(mask, 255, 12);
                mask[12] = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_netmask)->sin_addr.s_addr >>  0;
                mask[13] = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_netmask)->sin_addr.s_addr >>  8;
                mask[14] = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_netmask)->sin_addr.s_addr >> 16;
                mask[15] = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_netmask)->sin_addr.s_addr >> 24;
                the_interface.f_address.set_mask(mask);
            }
            break;

        case AF_INET6:
            the_interface.f_address.set_ipv6(*(reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)));

            if((ifa->ifa_flags & IFF_BROADCAST) != 0
            && ifa->ifa_broadaddr != nullptr)
            {
                the_interface.f_broadcast_address.set_ipv6(*(reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_broadaddr)));  // LCOV_EXCL_LINE
            }
            if((ifa->ifa_flags & IFF_POINTOPOINT) != 0
            && ifa->ifa_dstaddr != nullptr)  // LCOV_EXCL_LINE
            {
                the_interface.f_destination_address.set_ipv6(*(reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_dstaddr)));  // LCOV_EXCL_LINE
            }

            // if present, add the mask as well
            //
            if(ifa->ifa_netmask != nullptr)
            {
                the_interface.f_address.set_mask(reinterpret_cast<uint8_t *>(&reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_netmask)->sin6_addr));
            }
            break;

        default:
            // TODO: can we just ignore unexpected address families?
            //throw addr_invalid_structure("Unknown address family!");
            continue;

        }

        iface_list.push_back(the_interface);
    }

    return iface_list;
}


/** \brief Get the interface name.
 *
 * This function returns the name of the interface such as 'eth0' or 'p4p1'.
 *
 * The name is used in a few places such as the ioctl() function with the
 * SIOCGIFMTU command. Otherwise, it's mainly for display and easing use
 * (i.e. to let users select which interface to connect to.)
 *
 * \return The interface name.
 */
std::string iface::get_name() const
{
    return f_name;
}


/** \brief Get the interface setup flags.
 *
 * This function returns a set of flags defined on that interface. The flags
 * are defined in the `man 7 netdevice` as the IFF_... flags. The flags are
 * defined under the SIOCGIFFLAGS and SIOCSIFFLAGS entries.
 *
 * One flag of interest is the IFF_UP flag. This means the interface is
 * active (even if not actually in use.)
 *
 * \return The interface flags.
 */
unsigned int iface::get_flags() const
{   
    return f_flags;
}


/** \brief Get this interface address.
 *
 * This function returns the address of the interface. This address is very
 * likely to have a mask (i.e. 192.168.0.0/255.255.0.0).
 *
 * The address may be an IPv4 or an IPv6 address.
 *
 * \return The address of the interface.
 */
addr const & iface::get_address() const
{
    return f_address;
}


/** \brief Get the broadcast address.
 *
 * This function returns a constant reference to the broadcast address
 * of this interface. The address is always available in this class. It
 * will be set to the ANY address if it was not defined. Note, however,
 * that even though the ANY address is not a valid broadcast address,
 * you should call the has_broadcast_address() function to know whether
 * this address is indeed defined.
 *
 * \return The broadcast address of this interface.
 */
addr const & iface::get_broadcast_address() const
{
    return f_broadcast_address;
}


/** \brief Get the destination address.
 *
 * This function returns a constant reference to the destination address
 * of this interface. The address is always available in this class. It
 * will be set to the ANY address if it was not defined. Note, however,
 * that the ANY address is a valid destination address (i.e. default
 * route).
 *
 * To know whether the destination address is defined in that interface,
 * make sure to call the has_destination_address() function first.
 *
 * \return The destination address of this interface.
 */
addr const & iface::get_destination_address() const
{
    return f_destination_address;
}


/** \brief Check whether a broadcast address.
 *
 * The broadcast address is not present on all interfaces. When it is, this
 * function returns true.
 *
 * Note that you can always call the get_broadcast_address(), but if
 * undefined it will appear as a default address (NETWORK_TYPE_ANY)
 * which you can't distinguish from a valid address although a
 * the NETWORK_TYPE_ANY is not a valid address for a broacast
 * address.
 *
 * \note
 * When a broadcast address is defined on an interface, then there can't
 * be a destination address.
 *
 * \return true if a broadcast address is defined.
 */
bool iface::has_broadcast_address() const
{
    return (f_flags & IFF_BROADCAST) != 0;
}


/** \brief Check whether this interface defines a destination address.
 *
 * This function returns true if this interface defined a destination
 * address. Either way you can call the get_destination_address()
 * function, however, the address will be of type NETWORK_TYPE_ANY
 * which does not tell you whether it was defined or not.
 *
 * Ethernet and the local interfaces all define a destination address.
 *
 * \note
 * The destination address is not assigned any specific mask (all
 * are ff or 255).
 *
 * \note
 * When there is a destination address defined on an interface, then
 * there can't be a broadcast address.
 *
 * \return true when that interface defined a destination address.
 */
bool iface::has_destination_address() const
{
    return (f_flags & IFF_POINTOPOINT) != 0;
}


/** \brief Search for the interface corresponding to this address.
 *
 * Peruse the list of available interfaces and return the one that matches
 * this address if any, otherwise return a null pointer.
 *
 * Say you create an addr object with the IP address "127.0.0.1" and then
 * call this function. You will get a pointer to the "lo" interface and
 * can check the validity of the flags (i.e. is the interface UP, can it
 * BROADCAST or MULTICAST your UDP packets, etc.)
 *
 * If the address is a remote address, then this function returns a null
 * pointer.
 *
 * \note
 * This function replaces the addr::is_computer_interface_address() function.
 * If this function returns a non-null pointer when allow_default_destination
 * set to false, then you've got the same result plus you have access to all
 * the available information from that interface.
 *
 * \warning
 * If you allow for the default destination, this function calls the
 * route::get_ipv4_routes() function which can be costly. Try to avoid
 * doing that in a loop.
 *
 * \param[in] a  The address used to search for an interface.
 * \param[in] allow_default_destination  If true and \p a doesn't match
 *            any of the interfaces, use the one interface with its
 *            destination set to 0.0.0.0 or equivalent.
 *
 * \return A pointer to an interface IP address.
 */
iface::pointer_t find_addr_interface(addr const & a, bool allow_default_destination)
{
    iface::vector_t interfaces(iface::get_local_addresses());

    for(auto i : interfaces)
    {
        if(i.get_address().match(a))
        {
            return iface::pointer_t(new iface(i));
        }
    }

    // if there is a default, keep a copy in case we do not find a
    // local address while looking (and only if the user requested
    // such, which is the default)
    //
    if(!allow_default_destination)
    {
        return iface::pointer_t();
    }

    // to determine the default interface, we need the list of routes
    // so we first gather that information and then search for the
    // interface that has that name
    //
    route::vector_t routes(route::get_ipv4_routes());
    route::pointer_t default_route(find_default_route(routes));
    if(default_route == nullptr)
    {
        return iface::pointer_t(); // LCOV_EXCL_LINE
    }

    std::string const & default_iface(default_route->get_interface_name());
    auto it(std::find_if(
              interfaces.cbegin()
            , interfaces.cend()
            , [default_iface](auto & i)
            {
                return i.get_name() == default_iface;
            }));
    if(it == interfaces.cend())
    {
        return iface::pointer_t(); // LCOV_EXCL_LINE
    }

    return iface::pointer_t(new iface(*it));
}


#if 0
/** \brief Check whether this address represents this computer.
 *
 * This function reads the addresses as given to us by the getifaddrs()
 * function. This is a system function that returns a complete list of
 * all the addresses this computer is managing / represents. In other
 * words, a list of address that other computers can use to connect
 * to this computer (assuming proper firewall, of course.)
 *
 * \warning
 * The list of addresses from getifaddrs() is not being cached. So you
 * probably do not want to call this function in a loop. That being
 * said, I still would imagine that retrieving that list is fast.
 *
 * \todo
 * We need to apply the mask to make this work properly. This is why
 * the current implementation fails big time (used by snapcommunicator.cpp).
 *
 * \return a computer_interface_address_t enumeration: error, true, or
 *         false at this time; on error errno should be set to represent
 *         what the error was.
 */

// replaced by with a pointer_t == nullptr if there was no match,
// although make sure to set allow_default_destination to false
//
iface::pointer_t find_addr_interface(addr const & a, bool allow_default_destination)

bool is_computer_interface_address(addr const & a)
{
    iface::vector_t interfaces(iface::get_local_addresses());

    for(auto i : interfaces)
    {
    }


    // TBD: maybe we could cache the ifaddrs for a small amount of time
    //      (i.e. 1 minute) so additional calls within that time
    //      can go even faster?
    //

    // get the list of interface addresses
    //
    struct ifaddrs * ifa_start(nullptr);
    if(getifaddrs(&ifa_start) != 0)
    {
        return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_ERROR; // LCOV_EXCL_LINE
    }
    std::shared_ptr<struct ifaddrs> auto_free(ifa_start, ifaddrs_deleter);

    bool const ipv4(a.is_ipv4());
    uint16_t const family(ipv4 ? AF_INET : AF_INET6);
    for(struct ifaddrs * ifa(ifa_start); ifa != nullptr; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr != nullptr
        && ifa->ifa_addr->sa_family == family)
        {
            if(ipv4)
            {
                // the interface address structure is a 'struct sockaddr_in'
                //
                if(memcmp(&reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)->sin_addr,
                            f_address.sin6_addr.s6_addr32 + 3, //&reinterpret_cast<struct sockaddr_in const *>(&f_address)->sin_addr,
                            sizeof(struct in_addr)) == 0)
                {
                    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE;
                }
            }
            else
            {
                // the interface address structure is a 'struct sockaddr_in6'
                //
                if(memcmp(&reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)->sin6_addr,
                            &f_address.sin6_addr,
                            sizeof(f_address.sin6_addr)) == 0)
                {
                    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE;
                }
            }
        }
    }

    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_FALSE;
}
#endif


}
// addr namespace
// vim: ts=4 sw=4 et
