SUBDIRS = data

extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libimage_tools.la

libimage_tools_la_SOURCES = 		\
	gth-image-tool-crop.c		\
	gth-image-tool-crop.h		\
	gth-image-tool-desaturate.c	\
	gth-image-tool-desaturate.h	\
	gth-image-tool-redo.c		\
	gth-image-tool-redo.h		\
	gth-image-tool-save.c		\
	gth-image-tool-save.h		\
	gth-image-tool-undo.c		\
	gth-image-tool-undo.h		\
	main.c

libimage_tools_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libimage_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libimage_tools_la_LIBADD = $(GTHUMB_LIBS)
libimage_tools_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = image_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

CLEANFILES = $(extensionini_DATA)

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
