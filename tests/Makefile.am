
test_runner =					\
	$(top_builddir)/wayland/test-runner.c	\
	$(top_builddir)/wayland/test-runner.h	\
	$(top_builddir)/wayland/test-helpers.c	\
	$(top_builddir)/wayland/test-helpers.h


check_PROGRAMS = 				\
	map-test

TESTS = $(check_PROGRAMS)

AM_LDFLAGS = -no-install -ldl
AM_CPPFLAGS =					\
	-I$(top_builddir)/src			\
	-I$(top_builddir)/wayland

map_test_SOURCES =				\
	$(test_runner)				\
	map-test.c				\
	$(top_builddir)/src/wldbg-ids-map.c	\
	$(top_builddir)/wayland/wayland-util.c
