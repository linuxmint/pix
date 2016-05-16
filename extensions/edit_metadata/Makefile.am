SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libedit_metadata.la

libedit_metadata_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-edit-metadata.c		\
	dlg-edit-metadata.h		\
	gth-delete-metadata-task.c	\
	gth-delete-metadata-task.h	\
	gth-edit-comment-dialog.c	\
	gth-edit-comment-dialog.h	\
	gth-edit-general-page.c		\
	gth-edit-general-page.h		\
	gth-edit-metadata-dialog.c	\
	gth-edit-metadata-dialog.h	\
	gth-edit-tags-dialog.c		\
	gth-edit-tags-dialog.h		\
	gth-tag-task.c			\
	gth-tag-task.h			\
	main.c				\
	utils.c				\
	utils.h

libedit_metadata_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libedit_metadata_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libedit_metadata_la_LIBADD = $(GTHUMB_LIBS)
libedit_metadata_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = edit_metadata.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
