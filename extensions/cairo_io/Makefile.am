extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcairo_io.la

libcairo_io_la_SOURCES = 	\
	main.c

if ENABLE_JPEG_TOOLS
libcairo_io_la_SOURCES +=	\
	cairo-io-jpeg.c		\
	cairo-io-jpeg.h		\
	cairo-io-png.c		\
	cairo-io-png.h
endif

libcairo_io_la_CFLAGS = $(GTHUMB_CFLAGS) $(JPEG_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb
libcairo_io_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcairo_io_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS)
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
