SUBDIRS = icons jpegutils
DIST_SUBDIRS = icons jpegutils 

gladedir = $(datadir)/gthumb/glade

if ENABLE_DEPRECATIONS
DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DBONOBO_DISABLE_DEPRECATED
else
DISABLE_DEPRECATED =
endif

INCLUDES =					\
	$(GTHUMB_CFLAGS)			\
	$(EXIF_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(LIBGPHOTO_CFLAGS)			\
	-DGTHUMB_PREFIX=\"$(prefix)\"		\
	-DGTHUMB_SYSCONFDIR=\"$(sysconfdir)\"	\
	-DGTHUMB_DATADIR=\"$(datadir)\"		\
	-DGTHUMB_LIBDIR=\"$(libdir)\"		\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps/"\"	\
	-DBONOBO_UIDIR=\""$(uidir)/"\"		\
	-DGTHUMB_GLADEDIR=\""$(gladedir)"\"	\
	-I$(top_srcdir)/libgthumb		

bin_PROGRAMS = gthumb

gthumb_SOURCES = 			\
	actions.c			\
	actions.h			\
	auto-completion.c		\
	auto-completion.h		\
	bookmark-list.c			\
	bookmark-list.h			\
	catalog-list.c			\
	catalog-list.h			\
	dir-list.c			\
	dir-list.h			\
	dlg-bookmarks.c			\
	dlg-bookmarks.h			\
	dlg-brightness-contrast.c	\
	dlg-brightness-contrast.h	\
	dlg-catalog.c			\
	dlg-catalog.h			\
	dlg-categories.c		\
	dlg-categories.h		\
	dlg-change-date.c		\
	dlg-change-date.h		\
	dlg-color-balance.c		\
	dlg-color-balance.h		\
	dlg-comment.c			\
	dlg-comment.h			\
	dlg-convert.c			\
	dlg-convert.h			\
	dlg-crop.c			\
	dlg-crop.h			\
	dlg-file-utils.c		\
	dlg-file-utils.h		\
	dlg-hue-saturation.c		\
	dlg-hue-saturation.h		\
	dlg-image-prop.c		\
	dlg-image-prop.h		\
	dlg-open-with.c			\
	dlg-open-with.h			\
	dlg-posterize.c			\
	dlg-posterize.h			\
	dlg-preferences.c		\
	dlg-preferences.h		\
	dlg-rename-series.c		\
	dlg-rename-series.h		\
	dlg-scale-image.c		\
	dlg-scale-image.h		\
	dlg-write-to-cd.c		\
	dlg-write-to-cd.h		\
	e-combo-button.c		\
	e-combo-button.h		\
	fullscreen.c			\
	fullscreen.h			\
	gth-image-selector.c		\
	gth-image-selector.h		\
	gth-exif-data-viewer.c		\
	gth-exif-data-viewer.h		\
	gth-exif-utils.c		\
	gth-exif-utils.h		\
	gth-folder-selection-dialog.c	\
	gth-folder-selection-dialog.h	\
	gth-toggle-button.c		\
	gth-toggle-button.h		\
	gtkorientationbox.c		\
	gtkorientationbox.h		\
	gthumb-preloader.c		\
	gthumb-preloader.h		\
	gthumb-window.c			\
	gthumb-window.h			\
	gtkcellrendererthreestates.h	\
	gtkcellrendererthreestates.c	\
	main.c				\
	main.h				\
	ui.h

gthumb_LDADD = 					\
	$(EXIF_LIBS)				\
	$(GTHUMB_LIBS)				\
	$(JPEG_LIBS)				\
	$(PNG_LIBS)				\
	$(TIFF_LIBS)				\
	$(RENDER_LIBS)				\
	$(LIBGPHOTO_LIBS)			\
	$(top_builddir)/libgthumb/libgthumb.la	\
	$(top_builddir)/src/jpegutils/libgthumb-jpegutils.la

moduledir = $(libdir)/gthumb/modules
module_LTLIBRARIES = 		\
	libduplicates.la	\
	libjpegtran.la		\
	libpngexporter.la	\
	libsearch.la		\
	libwebexporter.la	\
	libphotoimporter.la

libduplicates_la_SOURCES = dlg-duplicates.c dlg-duplicates.h
libduplicates_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libjpegtran_la_SOURCES = dlg-jpegtran.c dlg-jpegtran.h
libjpegtran_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)
libjpegtran_la_LIBADD  = $(top_builddir)/src/jpegutils/libgthumb-jpegutils.la

libpngexporter_la_SOURCES = 	\
	catalog-png-exporter.c	\
	catalog-png-exporter.h	\
	dlg-png-exporter.c	\
	dlg-png-exporter.h
libpngexporter_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libsearch_la_SOURCES = dlg-search.c dlg-search.h
libsearch_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libwebexporter_la_SOURCES = 	\
	albumtheme.c		\
	albumtheme-private.c	\
	albumtheme-private.h	\
	catalog-web-exporter.c	\
	catalog-web-exporter.h	\
	dlg-web-exporter.c	\
	dlg-web-exporter.h
libwebexporter_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libphotoimporter_la_SOURCES = dlg-photo-importer.c dlg-photo-importer.h
libphotoimporter_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)


EXTRA_DIST = 			\
	albumtheme.c		\
	albumtheme.y		\
	albumtheme.l		\
	lex.albumtheme.c
