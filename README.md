Introduction
============

libaddr is an easy to use C++ library that parses IP addresses to objects.
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
an IPv4 address.)


