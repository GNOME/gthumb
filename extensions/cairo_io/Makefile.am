extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcairo_io.la

libcairo_io_la_SOURCES = 	\
	cairo-io-jpeg.c		\
	cairo-io-jpeg.h		\
	main.c

libcairo_io_la_CFLAGS = $(GTHUMB_CFLAGS) $(JPEG_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libcairo_io_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcairo_io_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS) ../jpeg_utils/libjpeg_utils.la
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
