extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libraw_files.la

libraw_files_la_SOURCES = \
	main.c

libraw_files_la_CFLAGS = $(PIX_CFLAGS) $(LIBOPENRAW_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libraw_files_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libraw_files_la_LIBADD = $(PIX_LIBS) $(LIBOPENRAW_LIBS)
libraw_files_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = raw_files.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
