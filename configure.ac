m4_define([gthumb_major_version], [3])
m4_define([gthumb_minor_version], [2])
m4_define([gthumb_micro_version], [8])
m4_define([gthumb_version],
          [gthumb_major_version.gthumb_minor_version.gthumb_micro_version])
m4_define([gthumb_api_version],
          [gthumb_major_version.gthumb_minor_version])

AC_PREREQ(2.63)
AC_INIT([gthumb], [gthumb_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gthumb],
        [gthumb])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz tar-ustar])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([configure.ac])

GTHUMB_API_VERSION=gthumb_api_version
AC_SUBST([GTHUMB_API_VERSION])
AC_DEFINE_UNQUOTED([GTHUMB_API_VERSION],["$GTHUMB_API_VERSION"],[gThumb API version])

if test -z "$enable_maintainer_mode"; then
  enable_maintainer_mode=yes
fi
AM_MAINTAINER_MODE([enable])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_INIT

AC_SEARCH_LIBS([strerror],[cposix], [])

AC_PROG_CC
AC_DIAGNOSE([obsolete],[AM_PROG_CC_STDC:
	your code should no longer depend upon `am_cv_prog_cc_stdc', but upon
	`ac_cv_prog_cc_stdc'.  Remove this warning and the assignment when
	you adjust the code.  You can also remove the above call to
	AC_PROG_CC if you already called it elsewhere.])
am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc

AM_PROG_CC_C_O
AC_HEADER_STDC
AC_C_BIGENDIAN
AC_PROG_CPP
AC_PROG_CXX

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums, glib-mkenums)
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources, glib-compile-resources)

GNOME_COMMON_INIT
YELP_HELP_INIT
GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS
GLIB_GSETTINGS
GTHUMB_EXTENSION_RULES

GLIB_REQUIRED=2.34.0
GTK_REQUIRED=3.4.0
EXIV2_REQUIRED=0.21
CLUTTER_REQUIRED=1.0.0
CLUTTER_GTK_REQUIRED=1.0.0
GSTREAMER_REQUIRED=1.0.0
OPENRAW_REQUIRED=0.0.8
LIBSOUP_REQUIRED=2.36
LIBSECRET_REQUIRED=0.11
LIBBRASERO_REQUIRED=3.2.0
LIBCHAMPLAIN_REQUIRED=0.12.0
LIBRSVG_REQUIRED=2.34.0
LIBWEBP_REQUIRED=0.2.0
JSON_GLIB_REQUIRED=0.15.0
WEBKIT2_REQUIRED=1.10.0

dnl ===========================================================================

CFLAGS=${CFLAGS-:""}
CFLAGS="$CFLAGS -fPIC -DPIC -Wl,--as-needed"

AC_ARG_ENABLE(deprecated,
	      AS_HELP_STRING([--disable-deprecated], [do not use deprecated API]),
	      disable_deprecated=yes,
	      disable_deprecated=no)
if test x$disable_deprecated = xyes; then
	CFLAGS="$CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
	CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED"
fi

dnl ===========================================================================

AC_ARG_ENABLE(debug,
	      AS_HELP_STRING([--enable-debug], [enable compilation of debugging messages]),
	      [case "${enableval}" in
	      yes) ENABLE_DEBUG=yes ;;
	      no) ENABLE_DEBUG=no ;;
	      *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	      esac],
	      [ENABLE_DEBUG=no])
if test x$ENABLE_DEBUG = xyes; then
	AC_DEFINE(DEBUG, 1, [enable compilation of debugging messages])
	CFLAGS="$CFLAGS -g -O0 -DDEBUG"
fi

AC_ARG_ENABLE(gseal,
	      AS_HELP_STRING([--enable-gseal], [check use of direct access to object fields]),
	      [case "${enableval}" in
	      yes) ENABLE_GSEAL=yes ;;
	      no) ENABLE_GSEAL=no ;;
	      *) AC_MSG_ERROR([bad value ${enableval} for --enable-gseal]) ;;
	      esac],
	      [ENABLE_GSEAL=no])
if test x$ENABLE_GSEAL = xyes; then
	CFLAGS="$CFLAGS -DGSEAL_ENABLE"
fi

AC_ARG_ENABLE(run_in_place,
	      AS_HELP_STRING([--enable-run-in-place],[load ui data and extensions from the source tree]),
	      [case "${enableval}" in
	      yes) enable_run_in_place=yes ;;
	      no) enable_run_in_place=no ;;
	      *) AC_MSG_ERROR([bad value ${enableval} for --enable-run-in-place]) ;;
	      esac],
	      [enable_run_in_place=no])
AM_CONDITIONAL(RUN_IN_PLACE, test "x$enable_run_in_place" != xno)

if test x$enable_run_in_place = xyes; then
	AC_DEFINE(RUN_IN_PLACE, 1, [load ui data and extensions from the source tree])
fi

AC_ARG_ENABLE(test-suite,
	      AS_HELP_STRING([--enable-test-suite], [enable the compilation of the test suite]),
	      [case "${enableval}" in
	      yes) ENABLE_TEST_SUITE=yes ;;
	      no) ENABLE_TEST_SUITE=no ;;
	      *) AC_MSG_ERROR([bad value ${enableval} for --enable-test-suite]) ;;
	      esac],
	      [ENABLE_TEST_SUITE=no])
AM_CONDITIONAL(BUILD_TEST_SUITE, test "x$ENABLE_TEST_SUITE" = xyes)

dnl ===========================================================================

PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= $GTK_REQUIRED])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

PKG_CHECK_MODULES(GTHUMB, [
	glib-2.0 		>= $GLIB_REQUIRED
	gthread-2.0
	gmodule-2.0
	gio-unix-2.0
	gtk+-3.0 		>= $GTK_REQUIRED
	libpng
	zlib
	gsettings-desktop-schemas
])
AC_SUBST(GTHUMB_LIBS)
AC_SUBST(GTHUMB_CFLAGS)

dnl ===========================================================================

SAVE_CFLAGS="$CFLAGS"
CFLAGS=""
AC_CHECK_LIB(m, floor,, AC_MSG_ERROR([math library not found]))
M_LIBS="-lm"
CFLAGS="$SAVE_CFLAGS"
unset SAVE_CFLAGS
AC_SUBST(M_LIBS)

dnl ===========================================================================

AC_ARG_ENABLE([exiv2],
	      [AS_HELP_STRING([--disable-exiv2],[do not compile code that uses the exiv2 library])],,
	      [enable_exiv2=yes])

if test x$enable_exiv2 = xyes ; then
	PKG_CHECK_MODULES(EXIV2,
			  exiv2 >= $EXIV2_REQUIRED,
			  [enable_exiv2=yes],
			  [enable_exiv2=no])
	if pkg-config --atleast-version=0.20 exiv2; then
		AC_DEFINE(HAVE_EXIV2_020, 1, [Have exiv2 0.20 or greater])
	fi
fi
AC_SUBST(EXIV2_LIBS)
AC_SUBST(EXIV2_CFLAGS)
AM_CONDITIONAL(ENABLE_EXIV2, test "x$enable_exiv2" = xyes)

dnl ===========================================================================

AC_ARG_ENABLE([clutter],
	      [AS_HELP_STRING([--disable-clutter],[do not compile code that uses the clutter library])],,
	      [enable_clutter=yes])

if test x$enable_clutter = xyes ; then
	PKG_CHECK_MODULES(CLUTTER,
			  [clutter-1.0 >= $CLUTTER_REQUIRED
			   clutter-gtk-1.0 >= $CLUTTER_GTK_REQUIRED],
			  [enable_clutter=yes],
			  [enable_clutter=no])
fi
if test x$enable_clutter = xyes ; then
	AC_DEFINE(HAVE_CLUTTER, 1, [Define to 1 if clutter support is included])
fi

AC_SUBST(CLUTTER_LIBS)
AC_SUBST(CLUTTER_CFLAGS)
AM_CONDITIONAL(ENABLE_CLUTTER, test "x$enable_clutter" = xyes)

dnl ===========================================================================

AC_ARG_ENABLE([gstreamer],
	      [AS_HELP_STRING([--disable-gstreamer],[do not compile code that uses the gstreamer library])],,
	      [enable_gstreamer=yes])

if test x$enable_gstreamer = xyes ; then
	PKG_CHECK_MODULES(GSTREAMER,
			  [gstreamer-1.0 >= $GSTREAMER_REQUIRED
			   gstreamer-plugins-base-1.0 >= $GSTREAMER_REQUIRED
			   gstreamer-video-1.0 >= $GSTREAMER_REQUIRED],
			  [AC_DEFINE(HAVE_GSTREAMER, 1, [Have gstreamer])
			   enable_gstreamer=yes],
			  [enable_gstreamer=no])
fi
AC_SUBST(GSTREAMER_LIBS)
AC_SUBST(GSTREAMER_CFLAGS)
AM_CONDITIONAL(ENABLE_GSTREAMER, test "x$enable_gstreamer" = xyes)

dnl ===========================================================================

AC_ARG_ENABLE([libchamplain],
	      [AS_HELP_STRING([--enable-libchamplain],[compile code that uses the libchamplain library (crashes frequently)])],,
	      [enable_libchamplain=no])

if test x$enable_libchamplain = xyes ; then
	PKG_CHECK_MODULES(LIBCHAMPLAIN,
			  [champlain-0.12 >= $LIBCHAMPLAIN_REQUIRED
			   champlain-gtk-0.12 >= $LIBCHAMPLAIN_REQUIRED],
			  [enable_libchamplain=yes],
			  [enable_libchamplain=no])
fi
if test x$enable_libchamplain = xyes ; then
	AC_DEFINE(HAVE_LIBCHAMPLAIN, 1, [Define to 1 if libchamplain support is included])
fi

AC_SUBST(LIBCHAMPLAIN_LIBS)
AC_SUBST(LIBCHAMPLAIN_CFLAGS)
AM_CONDITIONAL(ENABLE_LIBCHAMPLAIN, test "x$enable_libchamplain" = xyes)

dnl ===========================================================================

IT_PROG_INTLTOOL([0.35.0])
GETTEXT_PACKAGE=gthumb
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[Gettext package])
AM_GLIB_GNU_GETTEXT

if test "x${prefix}" = "xNONE"; then
	AC_DEFINE_UNQUOTED(LOCALEDIR, "${ac_default_prefix}/${DATADIRNAME}/locale", [Locale directory])
else
	AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Locale directory])
fi

dnl ===========================================================================

AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-cygwin*|*-*-mingw*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

if test "$platform_win32" = "yes" ; then
	EXTENSION_LIBTOOL_FLAGS='-module -avoid-version -no-undefined -Wl,$(top_builddir)/gthumb/.libs/libgthumb.a'
	GTHUMB_IMPLIB="-lgthumb"
else
	EXTENSION_LIBTOOL_FLAGS='-module -avoid-version -disable-static'
	GTHUMB_IMPLIB=""
fi
AC_SUBST(EXTENSION_LIBTOOL_FLAGS)
AC_SUBST(GTHUMB_IMPLIB)

dnl ===========================================================================

AC_CACHE_CHECK(for tm_gmtoff in struct tm, [ac_cv_struct_tm_gmtoff],
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
	[[	#include <time.h>	]],
	[[
		struct tm tm;
		tm.tm_gmtoff = 1;
	]]
	)],[ac_cv_struct_tm_gmtoff=yes],[ac_cv_struct_tm_gmtoff=no]))
if test "x$ac_cv_struct_tm_gmtoff" = "xyes"; then
	AC_DEFINE(HAVE_TM_GMTOFF, 1, [Define if struct tm has a tm_gmtoff member])
else
	AC_CACHE_CHECK(for timezone variable, [ac_cv_var_timezone],
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[	#include <time.h> ]],
		[[	timezone = 1;	]]
		)],[ac_cv_var_timezone=yes],[ac_cv_var_timezone=no]))
	if test "x$ac_cv_var_timezone" = "xyes"; then
		AC_DEFINE(HAVE_TIMEZONE, 1, [Define if libc defines a timezone variable])
		AC_CACHE_CHECK(for altzone variable, [ac_cv_var_altzone],
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
			[[	#include <time.h>	]],
			[[	altzone = 1; ]]
			)],[ac_cv_var_altzone=yes],[ac_cv_var_altzone=no]))
		if test "x$ac_cv_var_altzone" = "xyes"; then
			AC_DEFINE(HAVE_ALTZONE, 1, [Define if libc defines an altzone variable])
		fi
	else
		AC_MSG_ERROR([unable to find a way to determine timezone])
	fi
fi

dnl ===========================================================================

WARN_CFLAGS="-Wall -Wcast-align -Wtype-limits -Wclobbered -Wempty-body -Wignored-qualifiers"

for option in $WARN_CFLAGS; do
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $option"
	AC_MSG_CHECKING([whether gcc understands $option])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[has_option=yes],[has_option=no])
	if test x$has_option = xyes; then
		WARNINGS="$WARNINGS $option"
	fi
	AC_MSG_RESULT($has_option)
	CFLAGS="$SAVE_CFLAGS"
	unset has_option
	unset SAVE_CFLAGS
done
unset option
CFLAGS="$CFLAGS $WARNINGS"

dnl ===========================================================================

AC_ARG_ENABLE([jpeg],
	      [AS_HELP_STRING([--disable-jpeg],[do not compile code that uses the libjpeg library])],,
	      [enable_jpeg=yes])
if test "x$enable_jpeg" = "xyes"; then
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS=""
	AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
        	     [enable_jpeg=yes],
	     	     [enable_jpeg=no])
	if test "x$enable_jpeg" = "xyes"; then
		AC_DEFINE(HAVE_LIBJPEG, 1, [Define to 1 if libjpeg support is included])
		JPEG_LIBS='-ljpeg -lm -lz'
	fi
	CFLAGS="$SAVE_CFLAGS"
	unset SAVE_CFLAGS
fi
AC_SUBST(JPEG_LIBS)
AM_CONDITIONAL(ENABLE_JPEG_TOOLS, test "x$enable_jpeg" = xyes)

have_libjpeg_80=no
if test "x$enable_jpeg" = "xyes"; then
	AC_MSG_CHECKING([[libjpeg version is 8 or greater]])
	AC_COMPILE_IFELSE([ AC_LANG_SOURCE(
			  [[
#include <stdio.h>
#include <jpeglib.h>
#if JPEG_LIB_VERSION < 80
#error "wrong version"
#endif
int main(int c, char**v) { return 0; }
			  ]]) ],
			  [AC_MSG_RESULT(yes)
			   have_libjpeg_80=yes],
		          [AC_MSG_RESULT(no)
		           have_libjpeg_80=no])
fi
AM_CONDITIONAL(HAVE_LIBJPEG_80, test "x$have_libjpeg_80" = xyes)

AC_CHECK_LIB(jpeg, jpeg_simple_progression,
             [AC_DEFINE(HAVE_PROGRESSIVE_JPEG, 1, [Define to 1 if libjpeg supports progressive JPEG.])],
             [AC_MSG_WARN(JPEG library does not support progressive saving.)])

dnl ===========================================================================

AC_ARG_ENABLE([tiff],
	      [AS_HELP_STRING([--disable-tiff],[do not compile code that uses the libtiff library])],,
	      [enable_tiff=yes])
if test x$enable_tiff = xyes ; then
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS=""
	AC_CHECK_LIB(tiff, TIFFWriteScanline,
		     [AC_CHECK_HEADER(tiffio.h,
				      [enable_tiff=yes],
				      [enable_tiff=no])],
		     [enable_tiff=no])
	if test "x$enable_tiff" = "xyes"; then
		AC_DEFINE(HAVE_LIBTIFF, 1, [Define to 1 if libtiff support is included])
		TIFF_LIBS='-ltiff -lm -lz'
	fi
	CFLAGS="$SAVE_CFLAGS"
	unset SAVE_CFLAGS
fi
AC_SUBST(TIFF_LIBS)

dnl ===========================================================================

AC_ARG_ENABLE([libwebp],
	      [AS_HELP_STRING([--disable-libwebp],[do not compile code that uses the libwebp library])],,
	      [enable_libwebp=yes])

if test x$enable_libwebp = xyes ; then
	PKG_CHECK_MODULES(LIBWEBP,
			  [libwebp >= $LIBWEBP_REQUIRED],
			  [enable_libwebp=yes],
			  [enable_libwebp=no])
fi
if test x$enable_libwebp = xyes ; then
	AC_DEFINE(HAVE_LIBWEBP, 1, [Define to 1 if libwebp support is included])
fi

AC_SUBST(LIBWEBP_LIBS)
AC_SUBST(LIBWEBP_CFLAGS)
AM_CONDITIONAL(ENABLE_LIBWEBP, test "x$enable_libwebp" = xyes)

AC_DEFINE(WEBP_IS_UNKNOWN_TO_GLIB, 1, [Define to 1 if webp images area not recognized by the glib functions])

dnl ===========================================================================

AC_MSG_CHECKING(liboperaw)
AC_ARG_ENABLE([libopenraw],
	      [AS_HELP_STRING([--enable-libopenraw],[use the libopenraw library to read raw files [default=no]])],,
	      [enable_libopenraw=no])

if test x$enable_libopenraw = xyes ; then
	PKG_CHECK_MODULES(LIBOPENRAW,
			  libopenraw-1.0 >= $OPENRAW_REQUIRED,
			  [enable_libopenraw=yes],
			  [enable_libopenraw=no])
	if test "x$enable_libopenraw" = "xyes"; then
		AC_DEFINE(ENABLE_LIBOPENRAW, 1, [Define to 1 if libopenraw must be used to read raw files instead of dcraw])
	fi
fi
if test x$enable_libopenraw = xno ; then
	enable_libopenraw='no (uses dcraw)'
fi
AC_SUBST(LIBOPENRAW_LIBS)
AC_SUBST(LIBOPENRAW_CFLAGS)
AC_MSG_RESULT($enable_libopenraw)

dnl ===========================================================================

AC_ARG_ENABLE([librsvg],
	      [AS_HELP_STRING([--disable-librsvg],[do not compile code that uses the librsvg library [default=yes]])],,
	      [enable_librsvg=yes])

if test x$enable_librsvg = xyes ; then
	PKG_CHECK_MODULES(LIBRSVG,
			  librsvg-2.0 >= $LIBRSVG_REQUIRED,
			  [enable_librsvg=yes],
			  [enable_librsvg=no])
	if test "x$enable_librsvg" = "xyes"; then
		AC_DEFINE(HAVE_LIBRSVG, 1, [Define to 1 if librsvg support is included])
	fi
fi
AC_SUBST(LIBRSVG_LIBS)
AC_SUBST(LIBRSVG_CFLAGS)
AM_CONDITIONAL(ENABLE_LIBRSVG, test "x$enable_librsvg" = xyes)

dnl ===========================================================================

AC_ARG_ENABLE([libsecret],
	      [AS_HELP_STRING([--disable-libsecret],[do not compile code that uses the libsecret library])],,
	      [enable_libsecret=yes])

if test x$enable_libsecret = xyes ; then
	PKG_CHECK_MODULES(LIBSECRET,
			  [libsecret-1 >= $LIBSECRET_REQUIRED],
			  [AC_DEFINE(HAVE_LIBSECRET, 1, [Have libsecret])
			   enable_libsecret=yes],
			  [enable_libsecret=no])
fi
AC_SUBST(LIBSECRET_CFLAGS)
AC_SUBST(LIBSECRET_LIBS)

dnl ===========================================================================

AC_ARG_ENABLE([libsoup],
	      [AS_HELP_STRING([--disable-libsoup],[do not compile code that uses the libsoup library])],,
	      [enable_libsoup=yes])

if test x$enable_libsoup = xyes ; then
	PKG_CHECK_MODULES(LIBSOUP_GNOME,
			  [libsoup-gnome-2.4 >= $LIBSOUP_REQUIRED],
			  [LIBSOUP_CFLAGS="$LIBSOUP_GNOME_CFLAGS"
			   LIBSOUP_LIBS="$LIBSOUP_GNOME_LIBS"
			   AC_DEFINE(HAVE_LIBSOUP_GNOME, 1, [Have libsoup-gnome])
			   enable_libsoup=yes],
			  [PKG_CHECK_MODULES(LIBSOUP,
					     [libsoup-2.4 >= $LIBSOUP_REQUIRED],
					     [enable_libsoup=yes],
					     [enable_libsoup=no])])
fi
AC_SUBST(LIBSOUP_CFLAGS)
AC_SUBST(LIBSOUP_LIBS)

dnl ===========================================================================

AC_ARG_ENABLE([libjson_glib],
	      [AS_HELP_STRING([--disable-libjson-glib],[do not compile code that uses the libjson-glib library])],,
	      [enable_libjson_glib=yes])

if test x$enable_libjson_glib = xyes ; then
	PKG_CHECK_MODULES(JSON_GLIB,
			  [json-glib-1.0 >= $JSON_GLIB_REQUIRED],
			  [AC_DEFINE(HAVE_JSON_GLIB, 1, [Have libjson-glib])
			   enable_libjson_glib=yes],
			  [enable_libjson_glib=no])
fi
AC_SUBST(JSON_GLIB_CFLAGS)
AC_SUBST(JSON_GLIB_LIBS)

dnl ===========================================================================

AC_ARG_ENABLE([webkit2],
	      [AS_HELP_STRING([--disable-webkit2],[do not compile code that uses webkit2gtk])],,
	      [enable_webkit2=yes])

if test x$enable_webkit2 = xyes ; then
	PKG_CHECK_MODULES(WEBKIT2,
			  [webkit2gtk-3.0 >= $WEBKIT2_REQUIRED],
			  [AC_DEFINE(HAVE_WEBKIT2, 1, [Have webkit2gtk])
			   enable_webkit2=yes],
			  [enable_webkit2=no])
fi
AC_SUBST(WEBKIT2_CFLAGS)
AC_SUBST(WEBKIT2_LIBS)

if [ test "x$enable_libsoup" = xyes && test "x$enable_libjson_glib" = xyes && test "x$enable_webkit2" = xyes ] ; then
  enable_web_services=yes
else
  enable_web_services=no
fi

AM_CONDITIONAL(ENABLE_WEB_SERVICES, test "x$enable_web_services" = xyes)

dnl ===========================================================================

AC_ARG_ENABLE([libbrasero],
	      [AS_HELP_STRING([--disable-libbrasero],[do not compile code that uses the libbrasero library])],,
	      [enable_libbrasero=yes])

if test x$enable_libbrasero = xyes ; then
	PKG_CHECK_MODULES(LIBBRASERO,
			  [libbrasero-burn3 >= $LIBBRASERO_REQUIRED],
			  [AC_DEFINE(HAVE_LIBBRASERO, 1, [Have libbrasero])
			   enable_libbrasero=yes],
			  [enable_libbrasero=no])
fi
AC_SUBST(LIBBRASERO_CFLAGS)
AC_SUBST(LIBBRASERO_LIBS)
AM_CONDITIONAL(ENABLE_BRASERO_TOOLS, test "x$enable_libbrasero" = xyes)

dnl ===========================================================================

enable_web_albums=yes
if test x$enable_web_albums = xyes; then
	AC_CHECK_PROGS(BISON, bison, no)
	if test "$BISON" = "no"; then
		enable_web_albums=no
	fi
fi
if test x$enable_web_albums = xyes; then
	AC_CHECK_PROGS(FLEX, flex lex, no)
	if test "$FLEX" = "no"; then
		enable_web_albums=no
	else
		AC_PROG_LEX
	fi
fi
AC_SUBST(BISON)
AC_SUBST(FLEX)
AM_CONDITIONAL(ENABLE_WEB_ALBUMS, test "x$enable_web_albums" = xyes)

dnl ===========================================================================

GDK_TARGET="$($PKG_CONFIG --variable targets gdk-3.0)"

AC_MSG_CHECKING([which smclient backend to use])
AC_ARG_WITH([smclient],
  [AS_HELP_STRING([--with-smclient=no|auto|xsmp],
		  [Setting smclient backend (default:no)])],,
		  [with_smclient=no])

if test "$with_smclient" == "auto"; then
  case "$GDK_TARGET" in
    *x11*) with_smclient=xsmp ;;
    *) with_smclient=no ;;
  esac
fi

AC_MSG_RESULT([$with_smclient])

if test "$with_smclient" != "no"; then
  AC_DEFINE([WITH_SMCLIENT],[1],[Define if smclient is enabled])

  case "$with_smclient" in
    xsmp) SMCLIENT_PKGS="sm >= 1.0.0 ice" ;;
    *) SMCLIENT_PKGS="" ;;
  esac

  PKG_CHECK_MODULES([SMCLIENT],[$SMCLIENT_PKGS])
  AC_SUBST([SMCLIENT_CFLAGS])
  AC_SUBST([SMCLIENT_LIBS])
  AC_DEFINE(USE_SMCLIENT, 1, [Use a session menager])
fi

AM_CONDITIONAL([WITH_SMCLIENT],[test "$with_smclient" != "no"])
AM_CONDITIONAL([WITH_SMCLIENT_XSMP],[test "$with_smclient" = "xsmp"])
AM_CONDITIONAL([WITH_SMCLIENT_WIN32],[test "$with_smclient" = "win32"])

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
copy-n-paste/Makefile
data/Makefile
data/gthumb.desktop.in
data/gthumb-${GTHUMB_API_VERSION}.pc:data/gthumb.pc.in
data/gthumb-import.desktop.in
data/icons/Makefile
data/icons/hicolor/Makefile
data/icons/hicolor/16x16/Makefile
data/icons/hicolor/16x16/actions/Makefile
data/icons/hicolor/16x16/apps/Makefile
data/icons/hicolor/22x22/Makefile
data/icons/hicolor/22x22/actions/Makefile
data/icons/hicolor/22x22/apps/Makefile
data/icons/hicolor/24x24/Makefile
data/icons/hicolor/24x24/actions/Makefile
data/icons/hicolor/32x32/Makefile
data/icons/hicolor/32x32/apps/Makefile
data/icons/hicolor/48x48/Makefile
data/icons/hicolor/48x48/actions/Makefile
data/icons/hicolor/48x48/apps/Makefile
data/icons/hicolor/256x256/Makefile
data/icons/hicolor/256x256/apps/Makefile
data/icons/hicolor/scalable/Makefile
data/icons/hicolor/scalable/actions/Makefile
data/icons/hicolor/scalable/apps/Makefile
data/ui/Makefile
help/Makefile
m4/Makefile
extensions/Makefile
extensions/23hq/Makefile
extensions/bookmarks/Makefile
extensions/bookmarks/data/Makefile
extensions/bookmarks/data/ui/Makefile
extensions/burn_disc/Makefile
extensions/burn_disc/data/Makefile
extensions/burn_disc/data/ui/Makefile
extensions/cairo_io/Makefile
extensions/cairo_io/data/Makefile
extensions/cairo_io/data/ui/Makefile
extensions/catalogs/Makefile
extensions/catalogs/data/Makefile
extensions/catalogs/data/ui/Makefile
extensions/change_date/Makefile
extensions/change_date/data/Makefile
extensions/change_date/data/ui/Makefile
extensions/comments/Makefile
extensions/comments/data/Makefile
extensions/comments/data/ui/Makefile
extensions/contact_sheet/Makefile
extensions/contact_sheet/data/Makefile
extensions/contact_sheet/data/contact_sheet_themes/Makefile
extensions/contact_sheet/data/ui/Makefile
extensions/convert_format/Makefile
extensions/convert_format/data/Makefile
extensions/convert_format/data/ui/Makefile
extensions/desktop_background/Makefile
extensions/edit_metadata/Makefile
extensions/edit_metadata/data/Makefile
extensions/edit_metadata/data/ui/Makefile
extensions/exiv2_tools/Makefile
extensions/exiv2_tools/data/Makefile
extensions/exiv2_tools/data/ui/Makefile
extensions/export_tools/Makefile
extensions/facebook/Makefile
extensions/facebook/data/Makefile
extensions/facebook/data/ui/Makefile
extensions/file_manager/Makefile
extensions/file_tools/Makefile
extensions/file_tools/data/Makefile
extensions/file_tools/data/ui/Makefile
extensions/file_viewer/Makefile
extensions/find_duplicates/Makefile
extensions/find_duplicates/data/Makefile
extensions/find_duplicates/data/ui/Makefile
extensions/flicker_utils/Makefile
extensions/flicker_utils/data/Makefile
extensions/flicker_utils/data/ui/Makefile
extensions/flicker/Makefile
extensions/gstreamer_tools/Makefile
extensions/gstreamer_tools/data/Makefile
extensions/gstreamer_tools/data/ui/Makefile
extensions/gstreamer_utils/Makefile
extensions/image_print/Makefile
extensions/image_print/data/Makefile
extensions/image_print/data/ui/Makefile
extensions/image_rotation/Makefile
extensions/image_viewer/Makefile
extensions/image_viewer/data/Makefile
extensions/image_viewer/data/ui/Makefile
extensions/importer/Makefile
extensions/importer/data/Makefile
extensions/importer/data/ui/Makefile
extensions/jpeg_utils/Makefile
extensions/list_tools/Makefile
extensions/list_tools/data/Makefile
extensions/list_tools/data/ui/Makefile
extensions/map_view/Makefile
extensions/oauth/Makefile
extensions/oauth/data/Makefile
extensions/oauth/data/ui/Makefile
extensions/photo_importer/Makefile
extensions/photo_importer/data/Makefile
extensions/photo_importer/data/ui/Makefile
extensions/photobucket/Makefile
extensions/photobucket/data/Makefile
extensions/photobucket/data/ui/Makefile
extensions/picasaweb/Makefile
extensions/picasaweb/data/Makefile
extensions/picasaweb/data/ui/Makefile
extensions/raw_files/Makefile
extensions/red_eye_removal/Makefile
extensions/red_eye_removal/data/Makefile
extensions/red_eye_removal/data/ui/Makefile
extensions/rename_series/Makefile
extensions/rename_series/data/Makefile
extensions/rename_series/data/ui/Makefile
extensions/resize_images/Makefile
extensions/resize_images/data/Makefile
extensions/resize_images/data/ui/Makefile
extensions/search/Makefile
extensions/search/data/Makefile
extensions/search/data/ui/Makefile
extensions/selections/Makefile
extensions/slideshow/Makefile
extensions/slideshow/data/Makefile
extensions/slideshow/data/ui/Makefile
extensions/webalbums/Makefile
extensions/webalbums/data/Makefile
extensions/webalbums/data/albumthemes/Makefile
extensions/webalbums/data/albumthemes/Classic/Makefile
extensions/webalbums/data/albumthemes/ClassicClips/Makefile
extensions/webalbums/data/albumthemes/NeatRound/Makefile
extensions/webalbums/data/albumthemes/Wiki/Makefile
extensions/webalbums/data/ui/Makefile
gthumb/Makefile
gthumb/resources/Makefile
po/Makefile.in
tests/Makefile
])

AC_OUTPUT

echo "
Configuration:

	Source code location : $srcdir
	Compiler             : $CC
	Prefix               : $prefix
	CFLAGS		     : $CFLAGS
	Debug                : $ENABLE_DEBUG
	Run in place         : ${enable_run_in_place}
	Build tests          : $ENABLE_TEST_SUITE
	Exiv2 support        : ${enable_exiv2}
	JPEG tools           : ${enable_jpeg}
	TIFF tools           : ${enable_tiff}
	Clutter support      : ${enable_clutter}
	GStreamer support    : ${enable_gstreamer}
	Use libopenraw       : ${enable_libopenraw}
	Web services support : ${enable_web_services}
	Use libsecret        : ${enable_libsecret}
	Burn disc support    : ${enable_libbrasero}
	Web albums           : ${enable_web_albums}
	SM client support    : ${with_smclient}
	Map support          : ${enable_libchamplain}
	SVG support          : ${enable_librsvg}
	WebP support         : ${enable_libwebp}
"
