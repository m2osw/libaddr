
* Add support for adding our own set of "protocol to port" or
  "port to protocol"--i.e. `getservbyname()` or `getservbyport()`

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

