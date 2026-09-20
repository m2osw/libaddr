
* Use the advgetopt in all the tools.

* Add support for adding our own set of "service to port" or
  "port to service"--i.e. `getservbyname()` or `getservbyport()`

  When nmap is installed, there is a much bigger services file here:

      /usr/share/nmap/nmap-services

  I think nmap comes with a library. Either way, reading their file is not
  complicated. However, we probably do not want to force the installation of
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
  resolve with a full-fledged parser.

* Look into having our own cache; it should use a globally allocated class.
  More or less, Linux does not cache anything by default and yet, the correct
  way to use a domain name is to re-query the IP each time we want to
  connect. In most cases, at the moment I just cache the IP forever, which
  is really bad too (i.e. you have to restart the process to update that IP).
  Note that when systemd-resolved is running on a machine, it does all the
  caching for us and the simple `getaddrinfo()` works fine. Otherwise, we
  need to use a library such as libresolv or c-ares so we can get the TTL
  along the IP address (and other info).

