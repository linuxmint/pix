if ENABLE_BRASERO_TOOLS

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libburn_disc.la

libburn_disc_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-burn-task.c			\
	gth-burn-task.h			\
	main.c

libburn_disc_la_CFLAGS = $(PIX_CFLAGS) $(LIBBRASERO_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libburn_disc_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libburn_disc_la_LIBADD = $(PIX_LIBS) $(LIBBRASERO_LIBS) ../export_tools/libexport_tools.la
libburn_disc_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = burn_disc.extension.in.in

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
