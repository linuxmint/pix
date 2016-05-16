extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libexport_tools.la

libexport_tools_la_SOURCES = 		\
	callbacks.c			\
	callbacks.h			\
	main.c

libexport_tools_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libexport_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libexport_tools_la_LIBADD = $(GTHUMB_LIBS)
libexport_tools_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = export_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
