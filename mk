#!/bin/sh
#
# See the snapcmakemodules project for details about this script
#     https://github.com/m2osw/snapcmakemodules

if test -x ../../cmake/scripts/mk
then
	# We assume we have an Apache2 running on our server
	#
	export PROJECT_TEST_ARGS='--tcp-port 80'

	../../cmake/scripts/mk $*
else
	echo "error: could not locate the cmake mk script"
	exit 1
fi

