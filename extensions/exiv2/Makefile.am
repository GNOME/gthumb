if ENABLE_EXIV2

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libexiv2.la

libexiv2_la_SOURCES = 			\
	exiv2-utils.h			\
	exiv2-utils.cpp			\
	gth-edit-exiv2-page.c		\
	gth-edit-exiv2-page.h		\
	gth-metadata-provider-exiv2.c	\
	gth-metadata-provider-exiv2.h	\
	main.c

libexiv2_la_CPPFLAGS = $(GTHUMB_CFLAGS) $(EXIV2_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libexiv2_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libexiv2_la_LIBADD = $(GTHUMB_LIBS) $(EXIV2_LIBS)
libexiv2_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = exiv2.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
