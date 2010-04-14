if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libphotobucket.la

libphotobucket_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-export-to-photobucket.c		\
	dlg-export-to-photobucket.h		\
	photobucket-account.c			\
	photobucket-account.h			\
	photobucket-account-chooser-dialog.c	\
	photobucket-account-chooser-dialog.h	\
	photobucket-account-manager-dialog.c	\
	photobucket-account-manager-dialog.h	\
	photobucket-authentication.c		\
	photobucket-authentication.h		\
	photobucket-connection.c		\
	photobucket-connection.h		\
	photobucket-photo.c			\
	photobucket-photo.h			\
	photobucket-photoset.c			\
	photobucket-photoset.h			\
	photobucket-service.c			\
	photobucket-service.h			\
	photobucket-types.h			\
	photobucket-user.c			\
	photobucket-user.h			\
	main.c

libphotobucket_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(GNOME_KEYRING_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libphotobucket_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libphotobucket_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(GNOME_KEYRING_LIBS) ../oauth/liboauth.la
libphotobucket_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = photobucket.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
