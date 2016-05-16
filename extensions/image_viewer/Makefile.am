SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libimage_viewer.la

libimage_viewer_la_SOURCES = 		\
	gth-image-histogram.c		\
	gth-image-histogram.h		\
	gth-image-viewer-page.c		\
	gth-image-viewer-page.h		\
	gth-metadata-provider-image.c	\
	gth-metadata-provider-image.h	\
	main.c				\
	preferences.c			\
	preferences.h

libimage_viewer_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libimage_viewer_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libimage_viewer_la_LIBADD = $(GTHUMB_LIBS)
if ENABLE_JPEG_TOOLS
libimage_viewer_la_LIBADD += ../jpeg_utils/libjpeg_utils.la
endif
libimage_viewer_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = image_viewer.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
