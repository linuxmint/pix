extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libdesktop_background.la

libdesktop_background_la_SOURCES = 	\
	$(JPEG_FILES)			\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	main.c		

libdesktop_background_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libdesktop_background_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libdesktop_background_la_LIBADD = $(PIX_LIBS)  ../image_viewer/libimage_viewer.la
libdesktop_background_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = desktop_background.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
