SUBDIRS = icons
DIST_SUBDIRS = icons 

gladedir = $(datadir)/gthumb/glade

INCLUDES =					\
	-I$(top_srcdir)/libgthumb		\
	$(GTHUMB_CFLAGS)			\
	$(GPHOTO_CFLAGS)			\
	$(IGE_MAC_INTEGRATION_CFLAGS)		\
	$(GTKUNIQUE_CFLAGS)			\
	$(DISABLE_DEPRECATED)			\
	$(WARNINGS)       			\
	-DGTHUMB_PREFIX=\"$(prefix)\"		\
	-DGTHUMB_SYSCONFDIR=\"$(sysconfdir)\"	\
	-DGTHUMB_DATADIR=\"$(datadir)\"		\
	-DGTHUMB_LIBDIR=\"$(libdir)\"		\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps/"\"	\
	-DBONOBO_UIDIR=\""$(uidir)/"\"		\
	-DGTHUMB_GLADEDIR=\""$(gladedir)"\"

bin_PROGRAMS = gthumb

gnome_gthumb_idl_sources =              \
	GNOME_GThumb-stubs.c            \
	GNOME_GThumb-skels.c            \
	GNOME_GThumb.h                  \
	GNOME_GThumb-common.c

$(gnome_gthumb_idl_sources): gnome_gthumb_idl_stamp
gnome_gthumb_idl_stamp: GNOME_GThumb.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $(IDL_INCLUDES) $<
	touch $@
	
gthumb_SOURCES = 				\
	$(gnome_gthumb_idl_sources)		\
	bookmark-list.c				\
	bookmark-list.h				\
	catalog-list.c				\
	catalog-list.h				\
	dlg-bookmarks.c				\
	dlg-bookmarks.h				\
	dlg-brightness-contrast.c		\
	dlg-brightness-contrast.h		\
	dlg-catalog.c				\
	dlg-catalog.h				\
	dlg-tags.c				\
	dlg-tags.h				\
	dlg-change-date.c			\
	dlg-change-date.h			\
	dlg-color-balance.c			\
	dlg-color-balance.h			\
	dlg-comment.c				\
	dlg-comment.h				\
	dlg-convert.c				\
	dlg-convert.h				\
	dlg-crop.c				\
	dlg-crop.h				\
	dlg-file-utils.c			\
	dlg-file-utils.h			\
	dlg-hue-saturation.c			\
	dlg-hue-saturation.h			\
	dlg-image-prop.c			\
	dlg-image-prop.h			\
	dlg-open-with.c				\
	dlg-open-with.h				\
	dlg-posterize.c				\
	dlg-posterize.h				\
	dlg-preferences.c			\
	dlg-preferences.h			\
	dlg-redeye-removal.c			\
	dlg-redeye-removal.h			\
	dlg-rename-series.c			\
	dlg-rename-series.h			\
        dlg-reset-exif.c                        \
        dlg-reset-exif.h                        \
	dlg-scale-image.c			\
	dlg-scale-image.h			\
	dlg-scale-series.c			\
	dlg-scale-series.h			\
	dlg-scripts.c				\
	dlg-scripts.h				\
	dlg-write-to-cd.c			\
	dlg-write-to-cd.h			\
	gth-application.c			\
	gth-application.h			\
	gth-batch-op.c				\
	gth-batch-op.h				\
	gth-browser.c				\
	gth-browser.h				\
	gth-browser-actions-entries.h		\
	gth-browser-actions-callbacks.h		\
	gth-browser-actions-callbacks.c		\
	gth-browser-ui.h			\
	gth-tag-selection-dialog.c		\
	gth-tag-selection-dialog.h		\
	gth-dir-list.c				\
	gth-dir-list.h				\
	gth-exif-data-viewer.c			\
	gth-exif-data-viewer.h			\
	gth-filter-bar.c			\
	gth-filter-bar.h			\
	gth-folder-selection-dialog.c		\
	gth-folder-selection-dialog.h		\
	gth-fullscreen.c			\
	gth-fullscreen.h			\
	gth-fullscreen-actions-entries.h	\
	gth-fullscreen-actions-callbacks.h	\
	gth-fullscreen-actions-callbacks.c	\
	gth-fullscreen-ui.h			\
	gth-image-history.c			\
	gth-image-history.h			\
	gth-image-selector.c			\
	gth-image-selector.h			\
	gth-location.c				\
	gth-location.h				\
	gth-viewer.c				\
	gth-viewer.h				\
	gth-viewer-actions-entries.h		\
	gth-viewer-actions-callbacks.h		\
	gth-viewer-actions-callbacks.c		\
	gth-viewer-ui.h				\
	gth-window.c				\
	gth-window.h				\
	gth-window-actions-entries.h		\
	gth-window-actions-callbacks.h		\
	gth-window-actions-callbacks.c		\
	gth-window-utils.c			\
	gth-window-utils.h			\
	gthumb-preloader.c			\
	gthumb-preloader.h			\
	gtkcellrendererthreestates.h		\
	gtkcellrendererthreestates.c		\
	main.c					\
	main.h					\
	totem-scrsaver.c			\
	totem-scrsaver.h			\
	rotation-utils.c			\
	rotation-utils.h 			\
	dlg-duplicates.c 			\
	dlg-duplicates.h 			\
	dlg-jpegtran.c 				\
	dlg-jpegtran.h 				\
	catalog-png-exporter.c			\
	catalog-png-exporter.h			\
	dlg-png-exporter.c			\
	dlg-png-exporter.h 			\
	dlg-search.c 				\
	dlg-search.h 				\
	albumtheme.c				\
	albumtheme-private.c			\
	albumtheme-private.h			\
	catalog-web-exporter.c			\
	catalog-web-exporter.h			\
	dlg-web-exporter.c			\
	dlg-web-exporter.h      		\
	dlg-photo-importer.c    		\
	dlg-photo-importer.h

if PLATFORM_WIN32
gthumb_LDFLAGS = -Wl,--export-all-symbols,--out-implib,.libs/gthumb.exe.a
endif

gthumb_LDADD = 					\
	$(top_builddir)/libgthumb/jpegutils/libgthumb-jpegutils.la	\
	$(top_builddir)/libgthumb/libgthumb.la	\
	$(GTHUMB_LIBS)				\
	$(JPEG_LIBS)				\
	$(TIFF_LIBS)				\
	$(RENDER_LIBS)				\
	$(GPHOTO_LIBS)				\
	$(IGE_MAC_INTEGRATION_LIBS)		\
	$(XTEST_LIBS)


AUTHORS.tab : $(top_srcdir)/AUTHORS
	sed -e 's/^/"/' -e 's/$$/",/' < $(top_srcdir)/AUTHORS > $@.tmp
	mv $@.tmp $@

BUILT_SOURCES = AUTHORS.tab

EXTRA_DIST = 			\
	GNOME_GThumb.idl	\
	albumtheme.c		\
	albumtheme.y		\
	albumtheme.l		\
	lex.albumtheme.c

CLEANFILES =				\
	$(BUILT_SOURCES)                \
	$(gnome_gthumb_idl_sources)	\
	gnome_gthumb_idl_stamp

-include $(top_srcdir)/git.mk
