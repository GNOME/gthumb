SUBDIRS = icons jpegutils
DIST_SUBDIRS = icons jpegutils 

glade_DATA = 				\
	gthumb.glade 			\
	gthumb_comments.glade		\
	gthumb_convert.glade		\
	gthumb_edit.glade		\
	gthumb_png_exporter.glade 	\
	gthumb_preferences.glade	\
	gthumb_print.glade		\
	gthumb_search.glade		\
	gthumb_tools.glade		\
	gthumb_web_exporter.glade 

gladedir = $(datadir)/gthumb/glade

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = gthumb-ui.xml 

DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DBONOBO_DISABLE_DEPRECATED

INCLUDES =					\
	$(GTHUMB_CFLAGS)			\
	$(EXIF_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
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
	auto-completion.c		\
	auto-completion.h		\
	bookmark-list.c			\
	bookmark-list.h			\
	catalog-list.c			\
	catalog-list.h			\
	commands-impl.c			\
	commands-impl.h			\
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
	dlg-file-utils.c		\
	dlg-file-utils.h		\
	dlg-hue-saturation.c		\
	dlg-hue-saturation.h		\
	dlg-image-prop.c		\
	dlg-image-prop.h		\
	dlg-maintenance.c		\
	dlg-maintenance.h		\
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
	e-combo-button.c		\
	e-combo-button.h		\
	fullscreen.c			\
	fullscreen.h			\
	gth-folder-selection-dialog.c	\
	gth-folder-selection-dialog.h	\
	gtkorientationbox.c		\
	gtkorientationbox.h		\
	gthumb-preloader.c		\
	gthumb-preloader.h		\
	gthumb-window.c			\
	gthumb-window.h			\
	gtkcellrendererthreestates.h	\
	gtkcellrendererthreestates.c	\
	main.c				\
	main.h

gthumb_LDADD = 					\
	$(EXIF_LIBS)				\
	$(GTHUMB_LIBS)				\
	$(JPEG_LIBS)				\
	$(PNG_LIBS)				\
	$(TIFF_LIBS)				\
	$(RENDER_LIBS)				\
	$(top_builddir)/libgthumb/libgthumb.la

moduledir = $(libdir)/gthumb/modules
module_LTLIBRARIES = 		\
	libduplicates.la	\
	libjpegtran.la		\
	libpngexporter.la	\
	libsearch.la		\
	libwebexporter.la	

libduplicates_la_SOURCES = dlg-duplicates.c dlg-duplicates.h
libduplicates_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libjpegtran_la_SOURCES = dlg-jpegtran.c dlg-jpegtran.h
libjpegtran_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)
libjpegtran_la_LIBADD  = $(top_builddir)/src/jpegutils/libgthumb-jpegutils.a

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

schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gthumb.schemas

if SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
endif

EXTRA_DIST = 			\
	$(glade_DATA)		\
	$(ui_DATA)		\
	$(schema_DATA)		\
	albumtheme.c		\
	albumtheme.y		\
	albumtheme.l		\
	lex.albumtheme.c

