SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcairo_io.la

ENUM_TYPES =                            \
        enum-types.h                    \
        enum-types.c

HEADER_FILES =                          \
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

libcairo_io_la_SOURCES = 		\
	cairo-image-surface-png.c	\
	cairo-image-surface-png.h	\
	cairo-image-surface-xcf.c	\
	cairo-image-surface-xcf.h	\
	gth-image-saver-png.c         	\
        gth-image-saver-png.h         	\
        gth-image-saver-tga.c         	\
        gth-image-saver-tga.h         	\
        gth-image-saver-tiff.c        	\
        gth-image-saver-tiff.h        	\
        preferences.c			\
        preferences.h			\
	main.c

if ENABLE_JPEG_TOOLS
libcairo_io_la_SOURCES +=		\
	cairo-image-surface-jpeg.c	\
	cairo-image-surface-jpeg.h	\
        gth-image-saver-jpeg.c        	\
        gth-image-saver-jpeg.h
endif

if ENABLE_LIBRSVG
libcairo_io_la_SOURCES +=		\
	cairo-image-surface-svg.c	\
	cairo-image-surface-svg.h
endif

if ENABLE_LIBWEBP
libcairo_io_la_SOURCES +=		\
	cairo-image-surface-webp.c	\
	cairo-image-surface-webp.h	\
	gth-image-saver-webp.c		\
	gth-image-saver-webp.h
endif

libcairo_io_la_CFLAGS = $(GTHUMB_CFLAGS) $(JPEG_CFLAGS) $(LIBRSVG_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb
libcairo_io_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcairo_io_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS) $(TIFF_LIBS) $(LIBRSVG_LIBS) $(LIBWEBP_LIBS)
if ENABLE_JPEG_TOOLS
libcairo_io_la_LIBADD += ../jpeg_utils/libjpeg_utils.la
endif

libcairo_io_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = cairo_io.extension.in.in
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
