SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcomments.la

libcomments_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-comments-preferences.c		\
	dlg-comments-preferences.h		\
	gth-comment.c				\
	gth-comment.h				\
	gth-import-metadata-task.c		\
	gth-import-metadata-task.h		\
	gth-metadata-provider-comment.c		\
	gth-metadata-provider-comment.h		\
	main.c					\
	preferences.h

libcomments_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libcomments_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcomments_la_LIBADD = $(GTHUMB_LIBS)
if ENABLE_EXIV2
libcomments_la_LIBADD += ../exiv2_tools/libexiv2_tools.la
endif 
libcomments_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = comments.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
