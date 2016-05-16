extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfile_manager.la

libfile_manager_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-copy-task.c			\
	gth-copy-task.h			\
	gth-delete-task.c		\
	gth-delete-task.h		\
	gth-duplicate-task.c		\
	gth-duplicate-task.h		\
	gth-reorder-task.c		\
	gth-reorder-task.h		\
	main.c				\
	preferences.h

libfile_manager_la_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libfile_manager_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfile_manager_la_LIBADD = $(PIX_LIBS)
libfile_manager_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = file_manager.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
