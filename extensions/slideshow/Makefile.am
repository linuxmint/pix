SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libslideshow.la

libslideshow_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-slideshow.c			\
	gth-slideshow.h			\
	gth-slideshow-preferences.c	\
	gth-slideshow-preferences.h	\
	gth-transition.c		\
	gth-transition.h		\
	main.c				\
	preferences.c			\
	preferences.h

libslideshow_la_CFLAGS = $(GTHUMB_CFLAGS) $(CLUTTER_CFLAGS) $(GSTREAMER_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libslideshow_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libslideshow_la_LIBADD = $(GTHUMB_LIBS) $(CLUTTER_LIBS) $(GSTREAMER_LIBS) ../catalogs/libcatalogs.la
if ENABLE_GSTREAMER
libslideshow_la_LIBADD += ../gstreamer_utils/libgstreamer_utils.la
endif
libslideshow_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

extensionini_in_files = slideshow.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
