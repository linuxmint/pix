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

libfind_duplicates_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfind_duplicates_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfind_duplicates_la_LIBADD = $(GTHUMB_LIBS) ../catalogs/libcatalogs.la
libfind_duplicates_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = find_duplicates.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
