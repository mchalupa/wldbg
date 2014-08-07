AC_PREREQ([2.64])

m4_define([wldbg_major_version], [0])
m4_define([wldbg_minor_version], [0])
m4_define([wldbg_micro_version], [0])
m4_define([wldbg_version],
          [wldbg_major_version.wldbg_minor_version.wldbg_micro_version])

AC_INIT([wldbg],
        [wldbg_version],
        [mchqwerty@gmail.com],
        [wldbg],
        [https://github.com/mchalupa/wldbg])

AC_SUBST([WLDBG_VERSION_MAJOR], [wldbg_major_version])
AC_SUBST([WLDBG_VERSION_MINOR], [wldbg_minor_version])
AC_SUBST([WLDBG_VERSION_MICRO], [wldbg_micro_version])
AC_SUBST([WLDBG_VERSION], [wldbg_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

PKG_PROG_PKG_CONFIG()

# future use
TESTS_CFLAGS=
TESTS_LIBS=
COMPILER_CFLAGS=

# check wayland's presence
PKG_CHECK_MODULES([WAYLAND_SERVER], [wayland-server],\
			[HAVE_WAYLAND_SERVER=yes], [HAVE_WAYLAND_SERVER=no])
PKG_CHECK_MODULES([WAYLAND_CLIENT], [wayland-client],\
			[HAVE_WAYLAND_CLIENT=yes], [HAVE_WAYLAND_CLIENT=no])

if test "x$HAVE_WAYLAND_SERVER" != "xyes"; then
	AC_MSG_ERROR([Need wayland-server libraries to compile])
fi

if test "x$HAVE_WAYLAND_CLIENT" != "xyes"; then
	AC_MSG_ERROR([Need wayland-client libraries to compile])
fi

#export options to makefiles
AC_SUBST([TESTS_LIBS])
AC_SUBST([TESTS_CFLAGS])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 passes/Makefile
		 tests/Makefile])

AC_OUTPUT
