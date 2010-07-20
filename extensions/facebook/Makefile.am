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
libfacebook_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(GNOME_KEYRING_LIBS) ../export_tools/libexport_tools.la
libfacebook_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = facebook.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
