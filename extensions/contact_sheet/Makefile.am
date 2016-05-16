SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcontact_sheet.la

ENUM_TYPES =		\
	enum-types.h	\
	enum-types.c
	
HEADER_FILES = 					\
	actions.h				\
	callbacks.h				\
	dlg-contact-sheet.h			\
	dlg-image-wall.h			\
	gth-contact-sheet-creator.h		\
	gth-contact-sheet-theme.h		\
	gth-contact-sheet-theme-dialog.h	\
	preferences.h				\
	$(NULL)

enum-types.h: $(HEADER_FILES)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#ifndef ENUM_TYPES_H\n#define ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& mv -f xgen-$(@F) enum-types.h )

enum-types.c: $(HEADER_FILES)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$^> xgen-$(@F) \
	&& mv -f xgen-$(@F) enum-types.c )

libcontact_sheet_la_SOURCES = 			\
	$(ENUM_TYPES)				\
	$(HEADER_FILES)				\
	actions.c				\
	callbacks.c				\
	dlg-contact-sheet.c			\
	dlg-image-wall.c			\
	gth-contact-sheet-creator.c		\
	gth-contact-sheet-theme.c		\
	gth-contact-sheet-theme-dialog.c	\
	main.c					\
	$(NULL)

if RUN_IN_PLACE
contact_sheet_datadir = $(abs_top_srcdir)/extensions/contact_sheet/data
else
contact_sheet_datadir = $(pkgdatadir)
endif

libcontact_sheet_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb -DCONTACT_SHEET_DATADIR=\"$(contact_sheet_datadir)\"
libcontact_sheet_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcontact_sheet_la_LIBADD = $(GTHUMB_LIBS) ../export_tools/libexport_tools.la
if ENABLE_JPEG_TOOLS
libcontact_sheet_la_LIBADD += ../image_rotation/libimage_rotation.la
endif
libcontact_sheet_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
extensionini_in_files = contact_sheet.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)
DISTCLEANFILES = $(extensionini_DATA)
BUILT_SOURCES = $(ENUM_TYPES)
CLEANFILES = $(BUILT_SOURCES) 

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
