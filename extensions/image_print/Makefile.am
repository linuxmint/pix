SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libimage_print.la

libimage_print_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-image-info.c		\
	gth-image-info.h		\
	gth-image-print-job.c		\
	gth-image-print-job.h		\
	gth-load-image-info-task.c	\
	gth-load-image-info-task.h	\
	preferences.c			\
	preferences.h			\
	main.c

libimage_print_la_CPPFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libimage_print_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libimage_print_la_LIBADD = $(PIX_LIBS) 
libimage_print_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = image_print.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
