SUBDIRS = cursors icons

bin_PROGRAMS = gthumb 

ENUM_TYPES =						\
	gth-enum-types.h				\
	gth-enum-types.c

MARSHALLERS =						\
	gth-marshal.c					\
	gth-marshal.h

EXTERNAL = 						\
	egg-macros.h					\
	eggfileformatchooser.c				\
	eggfileformatchooser.h				\
	gedit-message-area.c				\
	gedit-message-area.h				\
	gnome-desktop-thumbnail.c			\
	gnome-desktop-thumbnail.h			\
	gnome-thumbnail-pixbuf-utils.c

PUBLIC_HEADER_FILES = 					\
	dom.h						\
	egg-macros.h					\
	eggfileformatchooser.h				\
	gconf-utils.h					\
	gedit-message-area.h				\
	gio-utils.h					\
	glib-utils.h					\
	gnome-desktop-thumbnail.h			\
	gth-browser.h					\
	gth-cell-renderer-thumbnail.h			\
	gth-cursors.h					\
	gth-dumb-notebook.h				\
	gth-duplicable.h				\
	gth-edit-metadata-dialog.h			\
	gth-embedded-dialog.h				\
	gth-empty-list.h				\
	gth-enum-types.h                                \
	gth-extensions.h				\
	gth-file-data.h					\
	gth-file-list.h					\
	gth-file-properties.h				\
	gth-file-selection.h				\
	gth-file-source.h				\
	gth-file-source-vfs.h				\
	gth-file-store.h				\
	gth-file-tool.h					\
	gth-file-view.h					\
	gth-filter.h					\
	gth-filterbar.h					\
	gth-filter-editor-dialog.h			\
	gth-filter-file.h				\
	gth-folder-tree.h				\
	gth-histogram.h					\
	gth-hook.h					\
	gth-icon-cache.h				\
	gth-icon-view.h					\
	gth-image-dragger.h				\
	gth-image-history.h				\
	gth-image-loader.h				\
	gth-image-preloader.h				\
	gth-image-selector.h				\
	gth-image-viewer.h				\
	gth-image-viewer-tool.h				\
	gth-location-chooser.h				\
	gth-main.h					\
	gth-metadata.h					\
	gth-metadata-provider.h				\
	gth-monitor.h					\
	gth-multipage.h					\
	gth-nav-window.h				\
	gth-overwrite-dialog.h				\
	gth-pixbuf-task.h				\
	gth-preferences.h				\
	gth-progress-dialog.h				\
	gth-sidebar.h					\
	gth-statusbar.h					\
	gth-source-tree.h				\
	gth-stock.h					\
	gth-string-list.h				\
	gth-tags-entry.h				\
	gth-tags-file.h					\
	gth-task.h					\
	gth-test.h					\
	gth-test-chain.h				\
	gth-test-selector.h				\
	gth-test-simple.h				\
	gth-thumb-loader.h				\
	gth-time.h					\
	gth-toggle-menu-tool-button.h			\
	gth-toolbox.h					\
	gth-uri-list.h					\
	gth-user-dir.h					\
	gth-viewer-page.h				\
	gth-window.h					\
	gthumb-error.h					\
	gtk-utils.h					\
	main.h						\
	pixbuf-cache.h					\
	pixbuf-io.h					\
	pixbuf-utils.h					\
	typedefs.h					\
	zlib-utils.h					\
	$(NULL)
	
PRIVATE_HEADER_FILES = 					\
	dlg-bookmarks.h					\
	dlg-edit-metadata.h				\
	dlg-extensions.h				\
	gth-browser-actions-callbacks.h			\
	gth-browser-actions-entries.h			\
	gth-browser-ui.h				\
	gth-metadata-provider-file.h			\
	dlg-personalize-filters.h			\
	dlg-preferences.h				\
	dlg-sort-order.h				\
	gth-window-actions-callbacks.h			\
	gth-window-actions-entries.h			\
	$(NULL)

gthumb_SOURCES = 					\
	$(ENUM_TYPES)					\
	$(MARSHALLERS)					\
	$(EXTERNAL)					\
	$(PUBLIC_HEADER_FILES)				\
	$(PRIVATE_HEADER_FILES)				\
	dlg-bookmarks.c					\
	dlg-edit-metadata.c				\
	dlg-extensions.c				\
	dlg-personalize-filters.c			\
	dlg-preferences.c				\
	dlg-sort-order.c				\
	dom.c						\
	gconf-utils.c					\
	gio-utils.c					\
	glib-utils.c					\
	gth-browser.c					\
	gth-browser-actions-callbacks.c			\
	gth-cell-renderer-thumbnail.c			\
	gth-cursors.c					\
	gth-dumb-notebook.c				\
	gth-duplicable.c				\
	gth-edit-metadata-dialog.c			\
	gth-embedded-dialog.c				\
	gth-empty-list.c				\
	gth-extensions.c				\
	gth-file-data.c					\
	gth-file-list.c					\
	gth-file-properties.c				\
	gth-file-selection.c				\
	gth-file-source.c				\
	gth-file-source-vfs.c				\
	gth-file-store.c				\
	gth-file-tool.c					\
	gth-file-view.c					\
	gth-filter.c					\
	gth-filterbar.c					\
	gth-filter-editor-dialog.c			\
	gth-filter-file.c				\
	gth-folder-tree.c				\
	gth-histogram.c					\
	gth-hook.c					\
	gth-icon-cache.c				\
	gth-icon-view.c					\
	gth-image-dragger.c				\
	gth-image-history.c				\
	gth-image-loader.c				\
	gth-image-preloader.c				\
	gth-image-selector.c				\
	gth-image-viewer.c				\
	gth-image-viewer-tool.c				\
	gth-location-chooser.c				\
	gth-main.c					\
	gth-main-default-hooks.c			\
	gth-main-default-metadata.c			\
	gth-main-default-sort-types.c			\
	gth-main-default-tests.c			\
	gth-main-default-types.c			\
	gth-metadata.c					\
	gth-metadata-provider.c				\
	gth-metadata-provider-file.c			\
	gth-monitor.c					\
	gth-multipage.c					\
	gth-nav-window.c				\
	gth-overwrite-dialog.c				\
	gth-pixbuf-task.c				\
	gth-preferences.c				\
	gth-progress-dialog.c				\
	gth-sidebar.c					\
	gth-source-tree.c				\
	gth-statusbar.c					\
	gth-string-list.c				\
	gth-tags-entry.c				\
	gth-tags-file.c					\
	gth-task.c					\
	gth-test.c					\
	gth-test-chain.c				\
	gth-test-selector.c				\
	gth-test-simple.c				\
	gth-thumb-loader.c				\
	gth-time.c					\
	gth-toggle-menu-tool-button.c			\
	gth-toolbox.c					\
	gth-uri-list.c					\
	gth-user-dir.c					\
	gth-viewer-page.c				\
	gth-window.c					\
	gth-window-actions-callbacks.c			\
	gthumb-error.c					\
	gtk-utils.c					\
	main.c						\
	pixbuf-cache.c					\
	pixbuf-io.c					\
	pixbuf-utils.c					\
	zlib-utils.c					\
	$(NULL)

if PLATFORM_WIN32
gthumb_LDFLAGS = -Wl,--export-all-symbols,--out-implib,.libs/gthumb.exe.a
endif

gthumb_LDADD =						\
	$(top_builddir)/copy-n-paste/libeggsmclient.la	\
	$(GTHUMB_LIBS)					\
	$(EXIV2_LIBS)					\
	$(JPEG_LIBS)					\
	$(CLUTTER_LIBS)					\
	$(NULL)	

if RUN_IN_PLACE
ui_dir = $(abs_top_srcdir)/data/ui
extensions_ui_dir = $(abs_top_srcdir)/extensions
extensions_dir = $(abs_top_builddir)/extensions
else
ui_dir = $(datadir)/gthumb-2.0/ui
extensions_ui_dir = $(datadir)/gthumb-2.0/ui
extensions_dir = $(libdir)/gthumb-2.0/extensions
endif

gthumb_CFLAGS =							\
	$(GTHUMB_CFLAGS)					\
	$(EXIV2_CFLAGS)						\
	$(CLUTTER_CFLAGS)					\
	$(DISABLE_DEPRECATED)                   		\
        $(WARNINGS)                             		\
	-I$(top_srcdir)/copy-n-paste/				\
	-DGTHUMB_LOCALEDIR=\"$(datadir)/locale\"		\
	-DGTHUMB_PREFIX=\"$(prefix)\"           		\
	-DGTHUMB_SYSCONFDIR=\"$(sysconfdir)\"   		\
	-DGTHUMB_DATADIR=\"$(datadir)\"         		\
	-DGTHUMB_LIBDIR=\"$(libdir)\" 				\
	-DGTHUMB_PKGDATADIR=\"$(pkgdatadir)\"			\
	-DGTHUMB_UI_DIR=\"$(ui_dir)\"				\
	-DGTHUMB_EXTENSIONS_UI_DIR=\"$(extensions_ui_dir)\"	\
	-DGTHUMB_EXTENSIONS_DIR=\"$(extensions_dir)\"		\
	$(NULL)

gth-enum-types.h: $(PUBLIC_HEADER_FILES) $(GLIB_MKENUMS)
	$(GLIB_MKENUMS) \
		--fhead "#ifndef GTH_ENUM_TYPES_H\n#define GTH_ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* GTH_ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) gth-enum-types.h || cp xgen-$(@F) gth-enum-types.h ) \
	&& rm -f xgen-$(@F)

gth-enum-types.c: $(PUBLIC_HEADER_FILES) gth-enum-types.h
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) gth-enum-types.c || cp xgen-$(@F) gth-enum-types.c ) \
	&& rm -f xgen-$(@F)

gth-marshal.h: gth-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $(srcdir)/gth-marshal.list --header --prefix=gth_marshal > $@

gth-marshal.c: gth-marshal.h gth-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gth-marshal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) $(srcdir)/gth-marshal.list --body --prefix=gth_marshal >> $@ 

gthumb.h: make-header.sh gthumb.h.template Makefile.am
	$(srcdir)/make-header.sh $(srcdir)/gthumb.h.template  $(PUBLIC_HEADER_FILES) > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) gthumb.h || cp xgen-$(@F) gthumb.h ) \
	&& rm -f xgen-$(@F)

AUTHORS.tab : $(top_srcdir)/AUTHORS
	sed -e 's/^/"/' -e 's/$$/",/' < $(top_srcdir)/AUTHORS > $@.tmp
	mv $@.tmp $@

gthumbincludedir = $(includedir)/gthumb-2.0
gthumbinclude_HEADERS = gthumb.h

gthumbsubincludedir = $(includedir)/gthumb-2.0/gthumb
gthumbsubinclude_HEADERS = $(PUBLIC_HEADER_FILES)

BUILT_SOURCES = AUTHORS.tab gthumb.h $(MARSHALLERS) $(ENUM_TYPES)
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = 			\
	gth-marshal.list	\
	gthumb.h.template	\
	make-header.sh

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
