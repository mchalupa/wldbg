
test_runner =					\
	$(top_builddir)/wayland/test-runner.c	\
	$(top_builddir)/wayland/test-runner.h	\
	$(top_builddir)/wayland/test-helpers.c


check_PROGRAMS = 				\
	map-test				\
	parse-message-test			\
	util-test

TESTS = $(check_PROGRAMS)

AM_LDFLAGS = -no-install -ldl
AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/wayland

map_test_SOURCES =				\
	$(test_runner)				\
	map-test.c				\
	$(top_builddir)/src/wldbg-ids-map.h	\
	$(top_builddir)/src/wldbg-ids-map.c	\
	$(top_builddir)/wayland/wayland-util.h	\
	$(top_builddir)/wayland/wayland-util.c

parse_message_test_LDADD = 			\
	$(top_builddir)/src/libwldbg.la
parse_message_test_LDFLAGS =			\
	-lwayland-client			\
	$(AM_LDFLAGS)

parse_message_test_SOURCES =			\
	$(test_runner)				\
	parse-message-test.c

util_test_SOURCES =				\
	$(test_runner)				\
	util-test.c				\
	$(top_builddir)/src/util.c
