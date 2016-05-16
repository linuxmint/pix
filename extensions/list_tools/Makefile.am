SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = liblist_tools.la

liblist_tools_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-personalize-scripts.c	\
	dlg-personalize-scripts.h	\
	gth-script.c			\
	gth-script.h			\
	gth-script-editor-dialog.c	\
	gth-script-editor-dialog.h	\
	gth-script-file.c		\
	gth-script-file.h		\
	gth-script-task.h		\
	gth-script-task.c		\
	main.c

liblist_tools_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
liblist_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
liblist_tools_la_LIBADD = $(PIX_LIBS)
liblist_tools_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = list_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
