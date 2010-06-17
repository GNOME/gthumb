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
	flickr-authentication.c			\
	flickr-authentication.h			\
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

libflicker_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBSOUP_CFLAGS) $(GNOME_KEYRING_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libflicker_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libflicker_la_LIBADD = $(GTHUMB_LIBS) $(LIBSOUP_LIBS) $(GNOME_KEYRING_LIBS)
libflicker_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

endif

extensionini_in_files = flicker.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

-include $(top_srcdir)/git.mk
