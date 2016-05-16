SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libbookmarks.la

libbookmarks_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-bookmarks.c			\
	dlg-bookmarks.h			\
	main.c

libbookmarks_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libbookmarks_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libbookmarks_la_LIBADD = $(PIX_LIBS)
libbookmarks_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = bookmarks.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
