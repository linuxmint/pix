if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfacebook.la

libfacebook_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-export-to-facebook.c		\
	dlg-export-to-facebook.h		\
	dlg-import-from-facebook.c		\
	dlg-import-from-facebook.h		\
	facebook-album.c			\
	facebook-album.h			\
	facebook-album-properties-dialog.c	\
	facebook-album-properties-dialog.h	\
	facebook-photo.c			\
	facebook-photo.h			\
	facebook-service.c			\
	facebook-service.h			\
	facebook-types.h			\
	main.c					\
	preferences.h

libfacebook_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(LIBSECRET_CFLAGS) $(WEBKIT2_CFLAGS) $(JSON_GLIB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfacebook_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfacebook_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(LIBSECRET_LIBS) $(WEBKIT2_LIBS) $(JSON_GLIB_LIBS) ../export_tools/libexport_tools.la ../oauth/liboauth.la ../importer/libimporter.la
libfacebook_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = facebook.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
