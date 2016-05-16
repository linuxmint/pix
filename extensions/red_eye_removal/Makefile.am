SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libred_eye_removal.la

libred_eye_removal_la_SOURCES = 	\
	gth-file-tool-red-eye.c		\
	gth-file-tool-red-eye.h		\
	main.c

libred_eye_removal_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libred_eye_removal_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libred_eye_removal_la_LIBADD = $(PIX_LIBS) ../image_viewer/libimage_viewer.la
libred_eye_removal_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = red_eye_removal.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
