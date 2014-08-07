
bin_PROGRAMS = wldbg

wayland_files =				\
	../wayland/connection.c		\
	../wayland/wayland-os.c		\
	../wayland/wayland-util.c	\
	../wayland/wayland-os.h		\
	../wayland/wayland-util.h	\
	../wayland/wayland-private.h

AM_CPPFLAGS = -I..
AM_CFLAGS = $(CFLAGS)

wldbg_SOURCES =			\
	wldbg.c			\
	wldbg.h			\
	interactive.c		\
	passes.c		\
	$(wayland_files)
