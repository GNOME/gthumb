extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libfile_manager.la

libfile_manager_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-copy-task.c			\
	gth-copy-task.h			\
	gth-delete-task.c		\
	gth-delete-task.h		\
	gth-duplicate-task.c		\
	gth-duplicate-task.h		\
	gth-reorder-task.c		\
	gth-reorder-task.h		\
	main.c

libfile_manager_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfile_manager_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfile_manager_la_LIBADD = $(GTHUMB_LIBS)
libfile_manager_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = file_manager.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
