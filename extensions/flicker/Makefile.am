if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libflicker.la

libflicker_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-export-to-flickr.c			\
	dlg-export-to-flickr.h			\
	dlg-import-from-flickr.c		\
	dlg-import-from-flickr.h		\
	flickr-account.c			\
	flickr-account.h			\
	flickr-account-chooser-dialog.c		\
	flickr-account-chooser-dialog.h		\
	flickr-account-manager-dialog.c		\
	flickr-account-manager-dialog.h		\
	flickr-connection.c			\
	flickr-connection.h			\
	flickr-photo.c				\
	flickr-photo.h				\
	flickr-photoset.c			\
	flickr-photoset.h			\
	flickr-service.c			\
	flickr-service.h			\
	flickr-types.h				\
	flickr-user.c				\
	flickr-user.h				\
	main.c

libflicker_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libflicker_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libflicker_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) 
libflicker_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = flicker.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
