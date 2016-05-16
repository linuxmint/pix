SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libsearch.la

libsearch_la_SOURCES = 			\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-search.c			\
	gth-search.h			\
	gth-search-editor.c		\
	gth-search-editor.h		\
	gth-search-editor-dialog.c	\
	gth-search-editor-dialog.h	\
	gth-search-task.c		\
	gth-search-task.h		\
	main.c

libsearch_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libsearch_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libsearch_la_LIBADD = $(PIX_LIBS) ../catalogs/libcatalogs.la
libsearch_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = search.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
