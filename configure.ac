AC_INIT([gthumb], [2.11.0], [http://bugzilla.gnome.org/enter_bug.cgi?product=gthumb])

GNOME_COMMON_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])

AM_INIT_AUTOMAKE([1.9 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_C_BIGENDIAN
AC_PROG_CPP
AC_PROG_CXX

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

AM_PROG_LIBTOOL
GNOME_DOC_INIT
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

GLIB_REQUIRED=2.16.0
GTK_REQUIRED=2.16.0
GCONF_REQUIRED=2.6.0
EXIV2_REQUIRED=0.18
CLUTTER_REQUIRED=1.0.0
CLUTTER_GTK_REQUIRED=0.10.0
GSTREAMER_REQUIRED=0.10

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
	CFLAGS="-g -O0"
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

PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= $GTK_REQUIRED])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

PKG_CHECK_MODULES(GTHUMB, [
	glib-2.0 		>= $GLIB_REQUIRED
	gthread-2.0
	gmodule-2.0
	gio-unix-2.0
	gtk+-2.0 		>= $GTK_REQUIRED
	gconf-2.0 		>= $GCONF_REQUIRED
	unique-1.0
])
AC_SUBST(GTHUMB_LIBS)
AC_SUBST(GTHUMB_CFLAGS)

dnl ===========================================================================

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

dnl ===========================================================================

AC_ARG_ENABLE([exiv2],
	      [AS_HELP_STRING([--disable-exiv2],[do not compile code that uses the exiv2 library])],,
	      [enable_exiv2=yes])

if test x$enable_exiv2 = xyes ; then
	PKG_CHECK_MODULES(EXIV2,
			  exiv2 >= $EXIV2_REQUIRED,
			  [enable_exiv2=yes],
			  [enable_exiv2=no])
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
			   clutter-gtk-0.10 >= $CLUTTER_GTK_REQUIRED],
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
			  [gstreamer-0.10 >= $GSTREAMER_REQUIRED],
			  [enable_gstreamer=yes],
			  [enable_gstreamer=no])
fi
AC_SUBST(GSTREAMER_LIBS)
AC_SUBST(GSTREAMER_CFLAGS)
AM_CONDITIONAL(ENABLE_GSTREAMER, test "x$enable_gstreamer" = xyes)

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

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
	AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi
AM_GCONF_SOURCE_2

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
	EXTENSION_LIBTOOL_FLAGS='-module -avoid-version -no-undefined -Wl,$(top_builddir)/src/.libs/gthumb.exe.a'
else
	EXTENSION_LIBTOOL_FLAGS='-module -avoid-version -disable-static'
fi
AC_SUBST(EXTENSION_LIBTOOL_FLAGS)


dnl ===========================================================================

AC_ARG_ENABLE(deprecations,
	      [AS_HELP_STRING([--enable-deprecations],[warn about deprecated usages])])
if test "x$enable_deprecations" = "xyes"; then
	DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi
AC_SUBST(DISABLE_DEPRECATED)

dnl ===========================================================================

WARN_CFLAGS="-Wall -Wcast-align -Wtype-limits -Wclobbered -Wempty-body -Wignored-qualifiers"

for option in $WARN_CFLAGS; do
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $option"
	AC_MSG_CHECKING([whether gcc understands $option])
	AC_TRY_COMPILE([], [], [has_option=yes], [has_option=no])
	if test x$has_option = xyes; then
		WARNINGS="$WARNINGS $option"
	fi
	AC_MSG_RESULT($has_option)
	CFLAGS="$SAVE_CFLAGS"
	unset has_option
	unset SAVE_CFLAGS
done
unset option
AC_SUBST(WARNINGS)


dnl ===========================================================================

AC_MSG_CHECKING(JPEG Support)
AC_ARG_ENABLE([jpeg],
	      [AS_HELP_STRING([--disable-jpeg],[do not compile code that uses the libjpeg library])],,
	      [enable_jpeg=yes])
if test "x$enable_jpeg" = "xyes"; then
	AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
        	     [enable_jpeg=yes],
	     	     [enable_jpeg=no])
	if test "x$enable_jpeg" = "xyes"; then
		AC_DEFINE(HAVE_LIBJPEG, 1, [Define to 1 if libjpeg support is included])
		JPEG_LIBS='-ljpeg -lm -lz'
	fi
fi
AC_SUBST(JPEG_LIBS)
AM_CONDITIONAL(ENABLE_JPEG_TOOLS, test "x$enable_jpeg" = xyes)
AC_MSG_RESULT($enable_jpeg)


dnl ===========================================================================

AC_MSG_CHECKING(TIFF Support)
AC_ARG_ENABLE([tiff],
	      [AC_HELP_STRING([--disable-tiff],[do not compile code that uses the libtiff library])],,
	      [enable_tiff=yes])
if test x$enable_tiff = xyes ; then
	AC_CHECK_LIB(tiff, TIFFWriteScanline,
		     [AC_CHECK_HEADER(tiffio.h,				    
				      [enable_tiff=yes],
				      [enable_tiff=no])],
		     [enable_tiff=no])
	if test "x$enable_tiff" = "xyes"; then
		AC_DEFINE(HAVE_LIBTIFF, 1, [Define to 1 if libtiff support is included])
		TIFF_LIBS='-ltiff -lm -lz'
	fi
fi
AC_SUBST(TIFF_LIBS)
AC_MSG_RESULT($enable_tiff)

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
copy-n-paste/Makefile
data/Makefile
data/gthumb.desktop.in
data/gthumb-import.desktop.in
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/apps/Makefile
data/icons/22x22/Makefile
data/icons/22x22/apps/Makefile
data/icons/32x32/Makefile
data/icons/32x32/apps/Makefile
data/icons/48x48/Makefile
data/icons/48x48/apps/Makefile
data/icons/scalable/Makefile
data/icons/scalable/apps/Makefile
data/ui/Makefile
help/Makefile
extensions/Makefile
extensions/bookmarks/Makefile
extensions/bookmarks/data/Makefile
extensions/bookmarks/data/ui/Makefile
extensions/catalogs/Makefile
extensions/catalogs/data/Makefile
extensions/catalogs/data/ui/Makefile
extensions/comments/Makefile
extensions/comments/data/Makefile
extensions/comments/data/ui/Makefile
extensions/desktop_background/Makefile
extensions/exiv2/Makefile
extensions/file_manager/Makefile
extensions/file_tools/Makefile
extensions/file_tools/data/Makefile
extensions/file_tools/data/ui/Makefile
extensions/file_viewer/Makefile
extensions/gstreamer/Makefile
extensions/image_rotation/Makefile
extensions/image_viewer/Makefile
extensions/image_viewer/data/Makefile
extensions/image_viewer/data/ui/Makefile
extensions/jpeg_utils/Makefile
extensions/list_tools/Makefile
extensions/list_tools/data/Makefile
extensions/list_tools/data/ui/Makefile
extensions/photo_importer/Makefile
extensions/photo_importer/data/Makefile
extensions/photo_importer/data/ui/Makefile
extensions/pixbuf_savers/Makefile
extensions/pixbuf_savers/data/Makefile
extensions/pixbuf_savers/data/ui/Makefile
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
extensions/slideshow/Makefile
extensions/slideshow/data/Makefile
extensions/slideshow/data/ui/Makefile
gthumb/Makefile
gthumb/cursors/Makefile
gthumb/icons/Makefile
po/Makefile.in
tests/Makefile
])

AC_OUTPUT

echo "
Configuration:

	Source code location : $srcdir
	Compiler             : $CC
	Prefix               : $prefix
	Debug                : $ENABLE_DEBUG
	Run in place         : ${enable_run_in_place}
	Build tests          : $ENABLE_TEST_SUITE
	Exiv2 support        : ${enable_exiv2}
	JPEG tools           : ${enable_jpeg}
	TIFF tools           : ${enable_tiff}
	Clutter support      : ${enable_clutter}
	GStreamer support    : ${enable_gstreamer}
"
