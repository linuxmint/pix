extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libdesktop_background.la

libdesktop_background_la_SOURCES = 	\
	$(JPEG_FILES)			\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	main.c		

libdesktop_background_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libdesktop_background_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libdesktop_background_la_LIBADD = $(GTHUMB_LIBS)  ../image_viewer/libimage_viewer.la
libdesktop_background_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = desktop_background.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
