if ENABLE_GSTREAMER

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libgstreamer_tools.la

libgstreamer_tools_la_SOURCES = 		\
	actions.c				\
	actions.h				\
	gth-media-viewer-page.c			\
	gth-media-viewer-page.h			\
	gth-metadata-provider-gstreamer.c	\
	gth-metadata-provider-gstreamer.h	\
	main.c					\
	preferences.h

libgstreamer_tools_la_CPPFLAGS = $(GTHUMB_CFLAGS) $(GSTREAMER_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libgstreamer_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libgstreamer_tools_la_LIBADD = $(GTHUMB_LIBS) $(GSTREAMER_LIBS) ../gstreamer_utils/libgstreamer_utils.la
libgstreamer_tools_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = gstreamer_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
