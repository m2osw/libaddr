
* Optimizer of "List of Addresses"

  When loading a list of addresses, especially with masks, it is often
  possible to optimize the resulting list by removing entries that are
  there twice or a network that is fully included in another network.

  For example, in the following three network definitions, only the last
  one can be kept:

      192.168.5.0/24
      192.168.192.0/20
      192.168.0.0/16

  This would be useful to optimize the lists we add to an ipset. We really
  only need to add 192.168.0.0/16 if we expected an address to match any one
  of those three networks.

  The optimizer would go through the list of IP addresses and testing whether
  A is equal or included in B, in which case A gets removed. Then it tests
  whether B is included in A and if so, remove B.

* Add support for adding our own set of "service to port" or
  "port to service"--i.e. `getservbyname()` or `getservbyport()`

  When nmap is installed, there is a much bigger services file here:

      /usr/share/nmap/nmap-services

  I think nmap comes with a library. Either way, reading their file is not
  complicated. However, we probably do not want to force the installtion of
  that package. The nmap package can be installed by the build system and we
  can change the format to our own at build time.

  There is a list from IANA here:

  http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.txt

  which is pretty complete. It includes deprecated numbers.

  The default system feature does not allow for growth without editing the
  system supplied `/etc/services` file. We would like to have our own
  protocols added to a directory which our functions can parse.

  The function can first check with the default system functions and then
  search our files if the data was not found in the system file.

  Note 1: This is probably not very important in the Snap! environment because
          nearly each system connects to the communicator daemon and the
          supported protocols are all handled under the hood in this case.

  Note 2: The current implementation of the `addr_parser` is an all or nothing
          scheme, but if the port is a string (such as ":http"), then the
          system will succeed; however, if it is set to ":cd" (from our
          communicator daemon protocol), then it fails.

* Think about re-writing the parser using a lexer and a yacc. I think that
  would give us much more room for easier expansion of the parser.

  We have several issues at the moment with distinguishing between commas
  separating addresses or ports, etc. which I think would be easier to
  resolve with a full fledge parser.

