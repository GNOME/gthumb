SUBDIRS = icons cursors
DIST_SUBDIRS = icons cursors

if ENABLE_DEPRECATIONS
DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DBONOBO_DISABLE_DEPRECATED
else
DISABLE_DEPRECATED =
endif

gladedir = $(datadir)/gthumb/glade

INCLUDES =							\
	$(GTHUMB_CFLAGS)					\
	-DGTHUMB_MODULEDIR=\""$(libdir)/gthumb/modules"\"	\
	-DGTHUMB_GLADEDIR=\""$(gladedir)"\"			\
	$(DISABLE_DEPRECATED)

libgthumbdir = $(libdir)/gthumb
libgthumb_LTLIBRARIES = libgthumb.la
libgthumb_la_LDFLAGS = -avoid-version

EEL_CANVAS_RECT =		\
	eel-canvas-rect.c	\
	eel-canvas-rect.h

ENUMS =				\
	gthumb-enum-types.h	\
	gthumb-enum-types.c

EOG_CURSORS =			\
	cursors.c		\
	cursors.h

HEADER_FILES =				\
	async-pixbuf-ops.h		\
	bookmarks.h			\
	catalog.h			\
	comments.h			\
	dlg-save-image.h		\
	file-data.h			\
	file-utils.h			\
	gconf-utils.h			\
	glib-utils.h			\
	gnome-print-font-picker.h	\
	gnome-print-font-dialog.h	\
	gth-image-list.h		\
	gth-file-list.h			\
	gth-file-view.h			\
	gth-file-view-list.h		\
	gth-file-view-thumbs.h		\
	gth-pixbuf-op.h			\
	gthumb-error.h			\
	gthumb-histogram.h		\
	gthumb-init.h			\
	gthumb-info-bar.h		\
	gthumb-module.h			\
	gthumb-slide.h			\
	gthumb-stock.h			\
	gtk-utils.h			\
	image-loader.h			\
	image-viewer.h			\
	nav-window.h			\
	pixbuf-utils.h			\
	preferences.h			\
	print-callbacks.h		\
	progress-dialog.h		\
	search.h			\
	thumb-cache.h			\
	thumb-loader.h			\
	typedefs.h			\
	utf8-fnmatch.h

MARSHALLERS =			\
	gthumb-marshal.c	\
	gthumb-marshal.h

MD5 = 				\
	md5.c			\
	md5.h

libgthumb_la_SOURCES =			\
	$(EEL_CANVAS_RECT)		\
	$(ENUMS)			\
	$(EOG_CURSORS)			\
	$(HEADER_FILES)			\
	$(MARSHALLERS)			\
	$(MD5)				\
	async-pixbuf-ops.c		\
	bookmarks.c			\
	catalog.c			\
	comments.c			\
	dlg-save-image.c		\
	file-data.c			\
	file-utils.c			\
	gconf-utils.c			\
	glib-utils.c			\
	gnome-print-font-picker.c	\
	gnome-print-font-dialog.c	\
	gth-pixbuf-op.c			\
	gth-image-list.c		\
	gth-file-list.c			\
	gth-file-view.c			\
	gth-file-view-list.c		\
	gth-file-view-thumbs.c		\
	gthumb-error.c			\
	gthumb-histogram.c		\
	gthumb-init.c			\
	gthumb-info-bar.c		\
	gthumb-module.c			\
	gthumb-slide.c			\
	gthumb-stock.c			\
	gtk-utils.c			\
	image-loader.c			\
	image-viewer.c			\
	nav-window.c			\
	pixbuf-utils.c			\
	preferences.c			\
	print-callbacks.c		\
	progress-dialog.c		\
	search.c			\
	thumb-cache.c			\
	thumb-loader.c			\
	utf8-fnmatch.c


gthumb-marshal.h: gthumb-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) gthumb-marshal.list --header --prefix=gthumb_marshal > $@

gthumb-marshal.c: gthumb-marshal.h gthumb-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gthumb-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) gthumb-marshal.list --body --prefix=gthumb_marshal >> $@

gthumb-enum-types.h: $(HEADER_FILES) $(GLIB_MKENUMS)
	$(GLIB_MKENUMS) \
		--fhead "#ifndef GTHUMB_ENUM__TYPES_H\n#define GTHUMB_ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* GTHUMB_ENUM_TYPES_H */" \
                $(HEADER_FILES) > xgen-geth \
        && (cmp -s xgen-geth $(srcdir)/gthumb-enum-types.h || cp xgen-geth $(srcdir)/gthumb-enum-types.h ) \
        && rm -f xgen-geth

gthumb-enum-types.c: $(HEADER_FILES) $(GLIB_MKENUMS) $(srcdir)/gthumb-enum-types.h
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
                $(HEADER_FILES) > xgen-getc \
	&& cp xgen-getc $(srcdir)/gthumb-enum-types.c  \
        && rm -f xgen-getc

CLEANFILES = $(MARSHALLERS) $(ENUMS)
EXTRA_DIST = gthumb-marshal.list
