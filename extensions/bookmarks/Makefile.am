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
	resources.c			\
	resources.h			\
	main.c

RESOURCES_DEP = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir) $(srcdir)/gresource.xml)
resources.c: gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gth $(srcdir)/gresource.xml
resources.h: gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gth $(srcdir)/gresource.xml

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
	 gresource.xml 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
