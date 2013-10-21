SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libbookmarks.la

libbookmarks_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-bookmarks.c			\
	dlg-bookmarks.h			\
	main.c				\
	resources.c			\
	resources.h

libbookmarks_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libbookmarks_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libbookmarks_la_LIBADD = $(GTHUMB_LIBS)
libbookmarks_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = bookmarks.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST =				\
	$(extensionini_in_files)	\
	resources.xml 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
-include $(top_srcdir)/extension-resources.mk
