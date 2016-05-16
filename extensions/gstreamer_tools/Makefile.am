if ENABLE_GSTREAMER

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libgstreamer_tools.la

libgstreamer_tools_la_SOURCES = 		\
	actions.c				\
	actions.h				\
	gth-media-viewer-page.c			\
	gth-media-viewer-page.h			\
	gth-metadata-provider-gstreamer.c	\
	gth-metadata-provider-gstreamer.h	\
	main.c					\
	preferences.h

libgstreamer_tools_la_CPPFLAGS = $(PIX_CFLAGS) $(GSTREAMER_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libgstreamer_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libgstreamer_tools_la_LIBADD = $(PIX_LIBS) $(GSTREAMER_LIBS) ../gstreamer_utils/libgstreamer_utils.la
libgstreamer_tools_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = gstreamer_tools.extension.in.in

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
