if ENABLE_WEB_ALBUMS

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libwebalbums.la

libwebalbums_la_SOURCES = 			\
	albumtheme.c				\
	albumtheme-private.c			\
	albumtheme-private.h			\
	catalog-web-exporter.c			\
	catalog-web-exporter.h			\
	dlg-web-exporter.c			\
	dlg-web-exporter.h

albumtheme-lex.c: albumtheme.l 
	$(AM_V_GEN)( $(FLEX) $< && mv $(LEX_OUTPUT_ROOT).c albumtheme-lex.c ) 
	
albumtheme.c: albumtheme.y albumtheme-lex.c
	$(AM_V_GEN)( $(BISON) $< -o albumtheme.c ) 
	
libwebalbums_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libwebalbums_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libwebalbums_la_LIBADD = $(GTHUMB_LIBS)
libwebalbums_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = webalbums.extension.in.in

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files)	\
	albumtheme.c			\
	albumtheme.y			\
	albumtheme.l			\
	lex.albumtheme.c

CLEANFILES = $(BUILT_SOURCES) 

-include $(top_srcdir)/git.mk
