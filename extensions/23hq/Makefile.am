if ENABLE_WEB_SERVICES

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = lib23hq.la

lib23hq_la_SOURCES = 		\
	actions.c		\
	actions.h		\
	callbacks.c		\
	callbacks.h		\
	main.c

lib23hq_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(LIBSECRET_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
lib23hq_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
lib23hq_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(LIBSECRET_LIBS) ../importer/libimporter.la ../export_tools/libexport_tools.la ../flicker_utils/libflicker_utils.la
lib23hq_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = 23hq.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
