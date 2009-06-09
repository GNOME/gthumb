SUBDIRS = data

extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libsearch.la

libsearch_la_SOURCES = 			\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-search.c			\
	gth-search.h			\
	gth-search-editor-dialog.c	\
	gth-search-editor-dialog.h	\
	gth-search-task.c		\
	gth-search-task.h		\
	main.c

libsearch_la_CFLAGS = $(GTHUMB_CFLAGS) $(DISABLE_DEPRECATED) $(WARNINGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libsearch_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libsearch_la_LIBADD = $(GTHUMB_LIBS)
libsearch_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = search.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

CLEANFILES = $(extensionini_DATA)

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
