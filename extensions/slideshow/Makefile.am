if ENABLE_CLUTTER

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libslideshow.la

libslideshow_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-slideshow.c			\
	gth-slideshow.h			\
	gth-slideshow-preferences.c	\
	gth-slideshow-preferences.h	\
	gth-transition.c		\
	gth-transition.h		\
	main.c				\
	preferences.c			\
	preferences.h

libslideshow_la_CFLAGS = $(GTHUMB_CFLAGS) $(CLUTTER_CFLAGS) $(GSTREAMER_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libslideshow_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libslideshow_la_LIBADD = $(GTHUMB_LIBS) $(CLUTTER_LIBS) $(GSTREAMER_LIBS) ../catalogs/libcatalogs.la
if ENABLE_GSTREAMER
libslideshow_la_LIBADD += ../gstreamer_utils/libgstreamer_utils.la
endif
libslideshow_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = slideshow.extension.in.in

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
