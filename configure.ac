#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(u3-tool, 0.3, daviedev@users.sourceforge.net)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Parse arguments
AC_ARG_ENABLE([libusb],
     [  --enable-libusb    Force using libUSB subsystem instead of platform specific],
     [case "${enableval}" in
       yes) force_libusb=true ;;
       no)  force_libusb=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-libusb]) ;;
     esac],[force_libusb=false])

# Determine subsystem
AS_IF([test x$force_libusb = xtrue],
	[ subsystem=libusb ],
	[ AS_CASE([$host_os],
		[linux*], [subsystem=sg],
		[mingw32], [subsystem=spt],
		[subsystem=libusb])]
	)

AS_IF([test x$subsystem = xlibusb ], [ AC_DEFINE([SUBSYS_LIBUSB], [1], [Use libusb subsystem]) ])
AS_IF([test x$subsystem = xsg ], [ AC_DEFINE([SUBSYS_SG], [1], [Use sg subsystem]) ])
AS_IF([test x$subsystem = xspt ], [ AC_DEFINE([SUBSYS_SPT], [1], [Use spt subsystem]) ])

# Checks for libraries.
#FIXME: PKG_CHECK_MODULES not provided on MinGW
#AS_IF([ test x$subsystem = xlibusb ],
#	[ PKG_CHECK_MODULES([LIBUSB], [libusb],
#		[  ],
#		[ AC_MSG_FAILURE([libusb is required but not found.]) ])
#	])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdint.h stdlib.h string.h sys/ioctl.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_CHECK_FUNCS([memset regcomp strdup strerror strtoul])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile])
AC_OUTPUT

echo ""
echo "Subsystem:     ${subsystem}"
echo ""
