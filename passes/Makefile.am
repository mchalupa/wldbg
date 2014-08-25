
lib_LTLIBRARIES = help.la dump.la

AM_CPPFLAGS = -I$(top_builddir)/src

help_la_LDFLAGS = -module -avoid-version
help_la_SOURCES = help.c ../wayland/wayland-util.c

dump_la_LDFLAGS = -module -avoid-version
dump_la_SOURCES = dump.c
