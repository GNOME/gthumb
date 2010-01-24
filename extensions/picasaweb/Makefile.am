if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libpicasaweb.la

libpicasaweb_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-export-to-picasaweb.c	\
	dlg-export-to-picasaweb.h	\
	google-connection.c		\
	google-connection.h		\
	gth-account-chooser-dialog.c	\
	gth-account-chooser-dialog.h	\
	gth-account-properties-dialog.c	\
	gth-account-properties-dialog.h	\
	main.c				\
	picasa-web-album.c		\
	picasa-web-album.h		\
	picasa-web-service.c		\
	picasa-web-service.h		\
	preferences.h

libpicasaweb_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(GNOME_KEYRING_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libpicasaweb_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libpicasaweb_la_LIBADD = $(GTHUMB_LIBS) 
libpicasaweb_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = picasaweb.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
