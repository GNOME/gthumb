SUBDIRS = data

extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libresize_images.la

libresize_images_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-resize-images.c		\
	dlg-resize-images.h		\
	main.c				\
	preferences.h

libresize_images_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libresize_images_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libresize_images_la_LIBADD = $(GTHUMB_LIBS)
libresize_images_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = resize_images.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
