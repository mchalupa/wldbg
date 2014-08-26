
bin_PROGRAMS = wldbg

wayland_files =				\
	../wayland/connection.c		\
	../wayland/wayland-os.c		\
	../wayland/wayland-util.c	\
	../wayland/wayland-os.h		\
	../wayland/wayland-util.h	\
	../wayland/wayland-private.h

hardcoded_passes =				\
	$(top_builddir)/passes/list.c		\
	$(top_builddir)/passes/resolve.c	\
	$(top_builddir)/passes/resolve.h


AM_CPPFLAGS = -I.. -I$(top_builddir)/passes
AM_CFLAGS = $(CFLAGS)

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
