SUBDIRS = data

extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libimage_print.la

libimage_print_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-image-info.c		\
	gth-image-info.h		\
	gth-image-print-job.c		\
	gth-image-print-job.h		\
	gth-load-image-info-task.c	\
	gth-load-image-info-task.h	\
	main.c

libimage_print_la_CPPFLAGS = $(GTHUMB_CFLAGS) $(DISABLE_DEPRECATED) $(WARNINGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libimage_print_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libimage_print_la_LIBADD = $(GTHUMB_LIBS) 
libimage_print_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = image_print.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
