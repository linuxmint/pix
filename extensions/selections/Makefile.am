extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libselections.la

libselections_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	gth-file-source-selections.c 		\
	gth-file-source-selections.h		\
	gth-metadata-provider-selections.c	\
	gth-metadata-provider-selections.h	\
	gth-selections-manager.c		\
	gth-selections-manager.h		\
	main.c

libselections_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libselections_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libselections_la_LIBADD = $(GTHUMB_LIBS)
libselections_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = selections.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
