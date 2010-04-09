if ENABLE_WEB_SERVICES

SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libfacebook.la

libfacebook_la_SOURCES = 			\
	actions.c				\
	actions.h				\
	callbacks.c				\
	callbacks.h				\
	dlg-export-to-facebook.c		\
	dlg-export-to-facebook.h		\
	facebook-account.c			\
	facebook-account.h			\
	facebook-account-chooser-dialog.c	\
	facebook-account-chooser-dialog.h	\
	facebook-account-manager-dialog.c	\
	facebook-account-manager-dialog.h	\
	facebook-album.c			\
	facebook-album.h			\
	facebook-album-properties-dialog.c	\
	facebook-album-properties-dialog.h	\
	facebook-authentication.c		\
	facebook-authentication.h		\
	facebook-connection.c			\
	facebook-connection.h			\
	facebook-photo.c			\
	facebook-photo.h			\
	facebook-service.c			\
	facebook-service.h			\
	facebook-types.h			\
	facebook-user.c				\
	facebook-user.h				\
	main.c

libfacebook_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(GNOME_KEYRING_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libfacebook_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libfacebook_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(GNOME_KEYRING_LIBS)
libfacebook_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = facebook.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	$(AM_V_GEN)( sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@ )

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

endif

-include $(top_srcdir)/git.mk
