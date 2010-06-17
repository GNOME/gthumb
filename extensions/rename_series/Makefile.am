SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = librename_series.la

librename_series_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-rename-series.c		\
	dlg-rename-series.h		\
	gth-rename-task.c		\
	gth-rename-task.h		\
	main.c				\
	preferences.h

librename_series_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
librename_series_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
librename_series_la_LIBADD = $(GTHUMB_LIBS)
librename_series_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = rename_series.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
