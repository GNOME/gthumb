SUBDIRS = data

extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libcatalogs.la

libcatalogs_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-add-to-catalog.c		\
	dlg-add-to-catalog.h		\
	dlg-catalog-properties.c	\
	dlg-catalog-properties.h	\
	gth-catalog.c			\
	gth-catalog.h			\
	gth-file-source-catalogs.c 	\
	gth-file-source-catalogs.h	\
	main.c

libcatalogs_la_CFLAGS = $(GTHUMB_CFLAGS) $(WARNINGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libcatalogs_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcatalogs_la_LIBADD = $(GTHUMB_LIBS)
libcatalogs_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = catalogs.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
