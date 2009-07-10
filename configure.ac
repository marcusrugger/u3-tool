#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(u3-tool, 0.3, daviedev@users.sourceforge.net)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Checks for libraries.
PKG_CHECK_MODULES([LIBUSB], [libusb],
	[ 
		LIBUSB_MSG=yes
	],
	[ LIBUSB_MSG=no ])
AM_CONDITIONAL(HAVE_LIBUSB, [test x$LIBUSB_MSG = xyes])

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
echo "Options:"
echo "LibUSB used:     ${LIBUSB_MSG}"
echo ""
