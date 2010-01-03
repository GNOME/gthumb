extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libjpeg_utils.la

if ENABLE_JPEG_TOOLS
libjpeg_utils_la_SOURCES = 		\
	jmemorydest.c			\
	jmemorydest.h			\
	jmemorysrc.c			\
	jmemorysrc.h			\
	jpegtran.c			\
	jpegtran.h			\
	main.c				\
	transupp.c			\
	transupp.h
else
libjpeg_utils_la_SOURCES =
endif

libjpeg_utils_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libjpeg_utils_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libjpeg_utils_la_LIBADD = $(GTHUMB_LIBS) $(JPEG_LIBS)
libjpeg_utils_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = jpeg_utils.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
