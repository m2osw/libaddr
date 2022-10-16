
Introduction
============

`libaddr` is an easy to use C++ library that parses IP addresses to objects.
This includes parsing IPv4 and IPv6 addresses, with a port or a CIDR mask.


IPv4 support
============

The IPv4 support is limited to the 4 number dot notation (A.B.C.D).


IPv6 support
============

IPv6 requires square brackets as we otherwise view the colon (:) as
the port separator. So something like [::1]:123 will work. Just ::1
will be an error.


Port support
============

We support basic port notation by separating the address and port with
a colon.

You may define a port range by separating two decimal numbers by a dash (-).
For example 1-1023 would define all reserved ports.

Multiports is also available. You can write multiple ports separated by
commas. This is useful if you want to handle a certain number of specific
ports that are not clearly defined in a range. For example 1.2.3.4:80,443
references IPv4 address 1.2.3.4 and ports 80 and 443.


Mask support (CIDR)
===================

Masks are written after a slash (/).

A mask can be one decimal number, in which case it represents the number
of bits from left to right that are set to 1.

The mask can also be an address. In case of an address, it has to use the
same format as the first part (IPv4/IPv4 or IPv6/IPv6). Such a mask can
be absolutely anything. (i.e. 85.85.85.85 would clear all even bits of
an IPv4 address).

More or less from the time IPv6 was designed, the use of an address as
a mask has been deprecated. The library still supports such, however, by
default that feature is now turned off. Make sure to set the
`addr::allow_t::ALLOW_ADDRESS_MASK` parameter to true before parsing an
address if you want to allow such a feature (not recommanded).


Known Bugs
==========

It is possible to write an IPv4 address using the IPv6 syntax:

    ::ffff:192.168.1.1

Even though you write this IP address using the IPv6 syntax, the `is_ipv4()`
function will return true, which is expected. This can cause problems if
you really needed an IPv6 address, though. The library does not really have
the means, at the moment, to tell you whether it parsed an IPv4 or an IPv6
address.

Bugs
====

Submit bug reports and patches on
[github](https://github.com/m2osw/libaddr/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
