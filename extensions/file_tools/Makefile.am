SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfile_tools.la

libfile_tools_la_SOURCES = 		\
	gth-file-tool-adjust-colors.c	\
	gth-file-tool-adjust-colors.h	\
	gth-file-tool-crop.c		\
	gth-file-tool-crop.h		\
	gth-file-tool-desaturate.c	\
	gth-file-tool-desaturate.h	\
	gth-file-tool-enhance.c		\
	gth-file-tool-enhance.h		\
	gth-file-tool-equalize.c	\
	gth-file-tool-equalize.h	\
	gth-file-tool-flip.c		\
	gth-file-tool-flip.h		\
	gth-file-tool-mirror.c		\
	gth-file-tool-mirror.h		\
	gth-file-tool-negative.c	\
	gth-file-tool-negative.h	\
	gth-file-tool-redo.c		\
	gth-file-tool-redo.h		\
	gth-file-tool-resize.c		\
	gth-file-tool-resize.h		\
	gth-file-tool-rotate-left.c	\
	gth-file-tool-rotate-left.h	\
	gth-file-tool-rotate-right.c	\
	gth-file-tool-rotate-right.h	\
	gth-file-tool-save.c		\
	gth-file-tool-save.h		\
	gth-file-tool-save-as.c		\
	gth-file-tool-save-as.h		\
	gth-file-tool-undo.c		\
	gth-file-tool-undo.h		\
	main.c

libfile_tools_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfile_tools_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfile_tools_la_LIBADD = $(GTHUMB_LIBS)
libfile_tools_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = file_tools.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
