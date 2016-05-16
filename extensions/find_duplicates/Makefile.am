SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfind_duplicates.la

libfind_duplicates_la_SOURCES = 	\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-find-duplicates.c		\
	dlg-find-duplicates.h		\
	gth-find-duplicates.c		\
	gth-find-duplicates.h		\
	gth-folder-chooser-dialog.c	\
	gth-folder-chooser-dialog.h	\
	main.c

libfind_duplicates_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libfind_duplicates_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfind_duplicates_la_LIBADD = $(PIX_LIBS) ../catalogs/libcatalogs.la
libfind_duplicates_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = find_duplicates.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
