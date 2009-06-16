extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libfile_viewer.la

libfile_viewer_la_SOURCES = 		\
	gth-file-viewer-page.c		\
	gth-file-viewer-page.h		\
	main.c

libfile_viewer_la_CFLAGS = $(GTHUMB_CFLAGS) $(DISABLE_DEPRECATED) $(WARNINGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfile_viewer_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfile_viewer_la_LIBADD = $(GTHUMB_LIBS)
libfile_viewer_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = file_viewer.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
