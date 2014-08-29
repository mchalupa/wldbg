
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


AM_CPPFLAGS = -I..
AM_CFLAGS =				\
	$(CFLAGS)			\
	$(WAYLAND_SERVER_CFLAGS)	\
	$(WAYLAND_CLIENT_CFLAGS)

wldbg_SOURCES =			\
	wldbg.c			\
	wldbg.h			\
	interactive.c		\
	interactive.h		\
	interactive-commands.c	\
	passes.c		\
	util.c			\
	util.h			\
	print.c			\
	print.h			\
	$(wayland_files)	\
	$(hardcoded_passes)
wldbg_LDFLAGS = -ldl
