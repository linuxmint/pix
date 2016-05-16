if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libpicasaweb.la

libpicasaweb_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-export-to-picasaweb.c		\
	dlg-export-to-picasaweb.h		\
	dlg-import-from-picasaweb.c		\
	dlg-import-from-picasaweb.h		\
	main.c					\
	picasa-album-properties-dialog.c	\
	picasa-album-properties-dialog.h	\
	picasa-web-album.c			\
	picasa-web-album.h			\
	picasa-web-photo.c			\
	picasa-web-photo.h			\
	picasa-web-service.c			\
	picasa-web-service.h			\
	picasa-web-types.h			\
	preferences.h

libpicasaweb_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(LIBSECRET_CFLAGS) $(JSON_GLIB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libpicasaweb_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libpicasaweb_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(LIBSECRET_LIBS) $(JSON_GLIB_LIBS) ../importer/libimporter.la ../export_tools/libexport_tools.la ../oauth/liboauth.la
libpicasaweb_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = picasaweb.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
