extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libdesktop_background.la

libdesktop_background_la_SOURCES = 	\
	$(JPEG_FILES)			\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	main.c		

libdesktop_background_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libdesktop_background_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libdesktop_background_la_LIBADD = $(GTHUMB_LIBS) 
libdesktop_background_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = desktop_background.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
