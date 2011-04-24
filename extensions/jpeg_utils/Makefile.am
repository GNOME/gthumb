extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libjpeg_utils.la

if ENABLE_JPEG_TOOLS
libjpeg_utils_la_SOURCES = 		\
	jmemorydest.c			\
	jmemorydest.h			\
	jmemorysrc.c			\
	jmemorysrc.h			\
	jpeg-exif-orientation.c		\
	jpeg-exif-orientation.h		\
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
	
else
libjpeg_utils_la_SOURCES =
endif

libjpeg_utils_la_CFLAGS = $(GTHUMB_CFLAGS) $(JPEG_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libjpeg_utils_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libjpeg_utils_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS)
libjpeg_utils_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = jpeg_utils.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) README.6b README.8a

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
