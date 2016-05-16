if ENABLE_WEB_ALBUMS

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libwebalbums.la

libwebalbums_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	albumtheme.c			\
	albumtheme-private.c		\
	albumtheme-private.h		\
	callbacks.c			\
	callbacks.h			\
	dlg-web-exporter.c		\
	dlg-web-exporter.h		\
	gth-web-exporter.c		\
	gth-web-exporter.h		\
	main.c				\
	preferences.h

albumtheme-lex.c: albumtheme.l
	$(AM_V_GEN)( $(FLEX) -Pgth_albumtheme_yy -oalbumtheme-lex.c $< )
	
albumtheme.c: albumtheme.y albumtheme-lex.c
	$(AM_V_GEN)( $(BISON) $< -p gth_albumtheme_yy -o albumtheme.c )

if RUN_IN_PLACE
webalbum_datadir = $(abs_top_srcdir)/extensions/webalbums/data
else
webalbum_datadir = $(pkgdatadir)
endif

libwebalbums_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb -DWEBALBUM_DATADIR=\"$(webalbum_datadir)\"
libwebalbums_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libwebalbums_la_LIBADD = $(GTHUMB_LIBS) ../export_tools/libexport_tools.la
if ENABLE_JPEG_TOOLS
libwebalbums_la_LIBADD += ../image_rotation/libimage_rotation.la
endif 
libwebalbums_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

BUILT_SOURCES = 		\
	albumtheme-lex.c	\
	albumtheme.c

endif

extensionini_in_files = webalbums.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)	\
	albumtheme.c			\
	albumtheme.y			\
	albumtheme.l			\
	albumtheme-lex.c

CLEANFILES = $(BUILT_SOURCES) 

-include $(top_srcdir)/git.mk
