if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libflicker_utils.la

libflicker_utils_la_SOURCES = 			\
	dlg-export-to-flickr.c			\
	dlg-export-to-flickr.h			\
	dlg-import-from-flickr.c		\
	dlg-import-from-flickr.h		\
	flickr-account.c			\
	flickr-account.h			\
	flickr-consumer.c			\
	flickr-consumer.h			\
	flickr-photo.c				\
	flickr-photo.h				\
	flickr-photoset.c			\
	flickr-photoset.h			\
	flickr-service.c			\
	flickr-service.h			\
	flickr-types.h				\
	main.c					\
	preferences.h

libflicker_utils_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(LIBSECRET_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libflicker_utils_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libflicker_utils_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(LIBSECRET_LIBS) ../importer/libimporter.la ../export_tools/libexport_tools.la ../oauth/liboauth.la
libflicker_utils_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = flicker_utils.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
