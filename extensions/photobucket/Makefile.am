if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libphotobucket.la

libphotobucket_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-export-to-photobucket.c		\
	dlg-export-to-photobucket.h		\
	main.c					\
	photobucket-account.c			\
	photobucket-account.h			\
	photobucket-album.c			\
	photobucket-album.h			\
	photobucket-album-properties-dialog.c	\
	photobucket-album-properties-dialog.h	\
	photobucket-consumer.c			\
	photobucket-consumer.h			\
	photobucket-photo.c			\
	photobucket-photo.h			\
	photobucket-service.c			\
	photobucket-service.h

libphotobucket_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(LIBSECRET_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libphotobucket_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libphotobucket_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(LIBSECRET_LIBS) ../oauth/liboauth.la ../export_tools/libexport_tools.la
libphotobucket_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = photobucket.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
