
bin_PROGRAMS = wldbg

wayland_files =				\
	../wayland/connection.c		\
	../wayland/wayland-os.c		\
	../wayland/wayland-util.c	\
	../wayland/wayland-os.h		\
	../wayland/wayland-util.h	\
	../wayland/wayland-private.h

hardcoded_passes =			\
	list.c				\
	resolve.c			\
	resolve.h

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

AM_CPPFLAGS = -I..
AM_CFLAGS =				\
	$(CFLAGS)			\
	$(WAYLAND_SERVER_CFLAGS)	\
	$(WAYLAND_CLIENT_CFLAGS)

wldbg_SOURCES =			\
	wldbg.c			\
	wldbg.h			\
	wldbg-private.c		\
	wldbg-private.h		\
	wldbg-parse-message.h	\
	parse-message.c		\
	passes.c		\
	sockets.c		\
	sockets.h		\
	util.c			\
	util.h			\
	print.c			\
	print.h			\
	getopt.c		\
	getopt.h		\
	$(wayland_files)	\
	$(hardcoded_passes)	\
	$(interactive_sources)	\
	$(hardcoded_interfaces)

if ENABLE_DEBUG
# enable debugging fetures. That include debugging messages
# backtraces and syscall checks
wldbg_SOURCES += debug.c
endif

wldbg_LDFLAGS = -ldl -lwayland-client
