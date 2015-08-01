
bin_PROGRAMS = wldbg
lib_LTLIBRARIES = libwldbg.la

wayland_files =				\
	../wayland/connection.c		\
	../wayland/wayland-os.c		\
	../wayland/wayland-util.c	\
	../wayland/wayland-os.h		\
	../wayland/wayland-util.h	\
	../wayland/wayland-private.h

hardcoded_passes =			\
	list.c				\
	resolve-pass.c

interactive_sources =				\
	interactive/interactive.c		\
	interactive/interactive.h		\
	interactive/interactive-commands.c	\
	interactive/interactive-commands.h	\
	interactive/breakpoints.c		\
	interactive/filters.c			\
	interactive/info.c			\
	interactive/pass.c			\
	interactive/edit.c			\
	interactive/send.c

# XXX do it conditional
hardcoded_interfaces =				\
	../protocols/xdg-shell-protocol.c	\
	../protocols/wayland-drm-protocol.c

# libwldbg contains functions that we provide to
# passses, like wldbg_parse_message etc.
libwldbg_la_SOURCES = 		\
	wldbg-ids-map.c		\
	wldbg-ids-map.h		\
	resolve.h		\
	resolve.c		\
	parse-message.c

include_HEADERS = 		\
	wldbg.h			\
	wldbg-pass.h		\
	wldbg-parse-message.h

AM_CPPFLAGS = -I..
AM_CFLAGS =				\
	$(CFLAGS)			\
	$(WAYLAND_SERVER_CFLAGS)	\
	$(WAYLAND_CLIENT_CFLAGS)

wldbg_LDFLAGS = -ldl -lwayland-client
wldbg_LDADD = libwldbg.la
wldbg_SOURCES =			\
	wldbg.c			\
	wldbg-private.h		\
	passes.c		\
	sockets.c		\
	sockets.h		\
	print.c			\
	print.h			\
	getopt.c		\
	getopt.h		\
	util.c			\
	util.h			\
	$(wayland_files)	\
	$(hardcoded_passes)	\
	$(hardcoded_interfaces)	\
	$(interactive_sources)

if ENABLE_DEBUG
# enable debugging fetures. That include debugging messages
# backtraces and syscall checks
wldbg_SOURCES += debug.c
endif

