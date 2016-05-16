SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfile_tools.la

ENUM_TYPES =		\
	enum-types.h	\
	enum-types.c

HEADER_FILES = 				\
	cairo-blur.h			\
	cairo-rotate.h			\
	gth-file-tool-adjust-colors.h	\
	gth-file-tool-adjust-contrast.h	\
	gth-file-tool-crop.h		\
	gth-file-tool-equalize.h	\
	gth-file-tool-flip.h		\
	gth-file-tool-grayscale.h	\
	gth-file-tool-mirror.h		\
	gth-file-tool-negative.h	\
	gth-file-tool-redo.h		\
	gth-file-tool-resize.h		\
	gth-file-tool-rotate.h		\
	gth-file-tool-rotate-left.h	\
	gth-file-tool-rotate-right.h	\
	gth-file-tool-rotate.h		\
	gth-file-tool-save.h		\
	gth-file-tool-save-as.h		\
	gth-file-tool-sharpen.h		\
	gth-file-tool-undo.h		\
	gth-image-line-tool.h		\
	gth-image-rotator.h		\
	gth-preview-tool.h		\
	preferences.h

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

libfile_tools_la_SOURCES = 		\
	$(ENUM_TYPES)			\
	$(HEADER_FILES)			\
	callbacks.c			\
	callbacks.h			\
	cairo-blur.c			\
	cairo-rotate.c			\
	gth-file-tool-adjust-colors.c	\
	gth-file-tool-adjust-contrast.c	\
	gth-file-tool-crop.c		\
	gth-file-tool-equalize.c	\
	gth-file-tool-flip.c		\
	gth-file-tool-grayscale.c	\
	gth-file-tool-mirror.c		\
	gth-file-tool-negative.c	\
	gth-file-tool-redo.c		\
	gth-file-tool-resize.c		\
	gth-file-tool-rotate.c		\
	gth-file-tool-rotate-left.c	\
	gth-file-tool-rotate-right.c	\
	gth-file-tool-save.c		\
	gth-file-tool-save-as.c		\
	gth-file-tool-sharpen.c		\
	gth-file-tool-undo.c		\
	gth-image-line-tool.c		\
	gth-image-rotator.c		\
	gth-preview-tool.c		\
	main.c

libfile_tools_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfile_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfile_tools_la_LIBADD = $(GTHUMB_LIBS) ../image_viewer/libimage_viewer.la
libfile_tools_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = file_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 
DISTCLEANFILES = $(extensionini_DATA)
BUILT_SOURCES = $(ENUM_TYPES)
CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
