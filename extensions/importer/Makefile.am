SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libimporter.la

ENUM_TYPES =					\
	gth-import-enum-types.h			\
	gth-import-enum-types.c

HEADER_FILES = 					\
	gth-import-destination-button.h		\
	gth-import-preferences-dialog.h		\
	gth-import-task.h			\
	preferences.h				\
	utils.h

gth-import-enum-types.h: $(HEADER_FILES)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#ifndef ENUM_TYPES_H\n#define ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& mv -f xgen-$(@F) gth-import-enum-types.h )

gth-import-enum-types.c: $(HEADER_FILES)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$^> xgen-$(@F) \
	&& mv -f xgen-$(@F) gth-import-enum-types.c )

libimporter_la_SOURCES = 			\
	$(ENUM_TYPES)				\
	$(HEADER_FILES)				\
	gth-import-destination-button.c		\
	gth-import-preferences-dialog.c		\
	gth-import-task.c			\
	importer.h				\
	main.c					\
	utils.c

libimporter_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb
libimporter_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libimporter_la_LIBADD = $(GTHUMB_LIBS) ../catalogs/libcatalogs.la
if ENABLE_JPEG_TOOLS
libimporter_la_LIBADD += ../image_rotation/libimage_rotation.la
endif
if ENABLE_EXIV2
libimporter_la_LIBADD += ../exiv2_tools/libexiv2_tools.la
endif
libimporter_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = importer.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 
BUILT_SOURCES = $(ENUM_TYPES)
CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(extensionini_DATA)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
