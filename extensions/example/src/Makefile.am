extensiondir = $(EXTENSION_DIR)
extension_LTLIBRARIES = libexample.la

libexample_la_SOURCES = \
	main.c

libexample_la_CFLAGS = $(EXTENSION_CFLAGS) -DEXTENSION_LOCALEDIR=\"$(datadir)/locale\" 
libexample_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libexample_la_LIBADD = $(EXTENSION_LIBS)

extensioninidir = $(EXTENSION_DIR)
extensionini_in_files = example.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
