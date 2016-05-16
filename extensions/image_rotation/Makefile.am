if ENABLE_JPEG_TOOLS

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libimage_rotation.la

libimage_rotation_la_SOURCES = 		\
	$(JPEG_FILES)			\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-reset-orientation-task.c	\
	gth-reset-orientation-task.h	\
	gth-transform-task.c		\
	gth-transform-task.h		\
	main.c				\
	rotation-utils.c		\
	rotation-utils.h

libimage_rotation_la_CFLAGS = $(GTHUMB_CFLAGS) $(JPEG_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb
libimage_rotation_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libimage_rotation_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS) ../jpeg_utils/libjpeg_utils.la
libimage_rotation_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = image_rotation.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

-include $(top_srcdir)/git.mk
