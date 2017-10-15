# - Try to find LibAddr (libaddr)
#
# Once done this will define
#
# LIBADDR_FOUND        - System has LibAddr
# LIBADDR_INCLUDE_DIR  - The LibAddr include directories
# LIBADDR_LIBRARY      - The libraries needed to use LibAddr (none)
# LIBADDR_DEFINITIONS  - Compiler switches required for using LibAddr (none)

find_path( LIBADDR_INCLUDE_DIR libaddr/addr.h
			PATHS $ENV{LIBADDR_INCLUDE_DIR}
		 )
find_library( LIBADDR_LIBRARY addr
			PATHS $ENV{LIBADDR_LIBRARY}
		 )
mark_as_advanced( LIBADDR_INCLUDE_DIR LIBADDR_LIBRARY )

set( LIBADDR_INCLUDE_DIRS ${LIBADDR_INCLUDE_DIR} )
set( LIBADDR_LIBRARIES    ${LIBADDR_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBADDR_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibAddr DEFAULT_MSG LIBADDR_INCLUDE_DIR LIBADDR_LIBRARY )
