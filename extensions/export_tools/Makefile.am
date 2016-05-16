extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libexport_tools.la

libexport_tools_la_SOURCES = 		\
	callbacks.c			\
	callbacks.h			\
	main.c

libexport_tools_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libexport_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libexport_tools_la_LIBADD = $(PIX_LIBS)
libexport_tools_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = export_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
