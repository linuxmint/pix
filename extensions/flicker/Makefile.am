if ENABLE_WEB_SERVICES

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libflicker.la

libflicker_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	main.c

libflicker_la_CFLAGS = $(PIX_CFLAGS) $(LIBSOUP_CFLAGS) $(LIBSECRET_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/pix 
libflicker_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libflicker_la_LIBADD = $(PIX_LIBS) $(LIBSOUP_LIBS) $(LIBSECRET_LIBS) ../importer/libimporter.la ../export_tools/libexport_tools.la ../flicker_utils/libflicker_utils.la
libflicker_la_DEPENDENCIES = $(top_builddir)/pix/pix$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = flicker.extension.in.in

@PIX_EXTENSION_IN_RULE@
@PIX_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
