if ENABLE_GSTREAMER

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libgstreamer_utils.la

libgstreamer_utils_la_SOURCES = 		\
	gstreamer-utils.c			\
	gstreamer-utils.h			\
	main.c

libgstreamer_utils_la_CPPFLAGS = $(GTHUMB_CFLAGS) $(GSTREAMER_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libgstreamer_utils_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libgstreamer_utils_la_LIBADD = $(GTHUMB_LIBS) $(GSTREAMER_LIBS)
libgstreamer_utils_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = gstreamer_utils.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
