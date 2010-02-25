if ENABLE_BRASERO_TOOLS

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libburn_disc.la

libburn_disc_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	main.c

libburn_disc_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBBRASERO_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libburn_disc_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libburn_disc_la_LIBADD = $(GTHUMB_LIBS) $(LIBBRASERO_LIBS)
libburn_disc_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = burn_disc.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
