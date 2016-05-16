if ENABLE_JPEG_TOOLS

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libjpeg_utils.la

libjpeg_utils_la_SOURCES = 		\
	jmemorydest.c			\
	jmemorydest.h			\
	jmemorysrc.c			\
	jmemorysrc.h			\
	jpeg-info.c			\
	jpeg-info.h			\
	jpegtran.c			\
	jpegtran.h			\
	transupp.h			\
	main.c
	
if HAVE_LIBJPEG_80
libjpeg_utils_la_SOURCES +=		\
	jpegint-80.h			\
	transupp-80.c			\
	transupp-80.h
else
libjpeg_utils_la_SOURCES +=		\
	transupp-62.c			\
	transupp-62.h
endif

libjpeg_utils_la_CFLAGS = $(GTHUMB_CFLAGS) $(JPEG_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libjpeg_utils_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libjpeg_utils_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS)
libjpeg_utils_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = jpeg_utils.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) README.6b README.8a

-include $(top_srcdir)/git.mk
