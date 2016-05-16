extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfile_viewer.la

libfile_viewer_la_SOURCES = 		\
	gth-file-viewer-page.c		\
	gth-file-viewer-page.h		\
	main.c

libfile_viewer_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libfile_viewer_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfile_viewer_la_LIBADD = $(PIX_LIBS)
libfile_viewer_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = file_viewer.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
