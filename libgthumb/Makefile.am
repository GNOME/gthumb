SUBDIRS = icons cursors jpegutils
DIST_SUBDIRS = icons cursors jpegutils

if ENABLE_DEPRECATIONS
DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED
else
DISABLE_DEPRECATED =
endif

gladedir = $(datadir)/gthumb/glade

INCLUDES =							\
	$(EXIF_CFLAGS)						\
	$(GTHUMB_CFLAGS)					\
	-DGTHUMB_MODULEDIR=\""$(libdir)/gthumb/modules"\"	\
	-DGTHUMB_GLADEDIR=\""$(gladedir)"\"			\
	$(DISABLE_DEPRECATED)

libgthumbdir = $(libdir)
libgthumb_LTLIBRARIES = libgthumb.la
libgthumb_la_LDFLAGS = -avoid-version -no-undefined

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
	gstringlist.h			\
	gth-exif-utils.h		\
	gth-filter.h			\
	gth-image-list.h		\
	gth-iviewer.h			\
	gth-file-list.h			\
	gth-file-view.h			\
	gth-file-view-list.h		\
	gth-file-view-thumbs.h		\
	gth-nav-window.h		\
	gth-pixbuf-op.h			\
	gth-sort-utils.h		\
	gth-utils.h			\
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
	jpeg-utils.h			\
	nav-window.h			\
	pixbuf-utils.h			\
	preferences.h			\
	print-callbacks.h		\
	progress-dialog.h		\
	search.h			\
	thumb-cache.h			\
	thumb-loader.h			\
	typedefs.h

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
	gstringlist.c			\
	gth-exif-utils.c		\
	gth-filter.c			\
	gth-pixbuf-op.c			\
	gth-image-list.c		\
	gth-iviewer.c			\
	gth-file-list.c			\
	gth-file-view.c			\
	gth-file-view-list.c		\
	gth-file-view-thumbs.c		\
	gth-nav-window.c		\
	gth-sort-utils.c		\
	gth-utils.c			\
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
	jpeg-utils.c			\
	nav-window.c			\
	pixbuf-utils.c			\
	preferences.c			\
	print-callbacks.c		\
	progress-dialog.c		\
	search.c			\
	thumb-cache.c			\
	thumb-loader.c

libgthumb_la_LIBADD =		\
	$(EXIF_LIBS)		\
	$(JPEG_LIBS)		\
	$(TIFF_LIBS)		\
	$(GTHUMB_LIBS)		\
	$(IPTCDATA_LIBS)	\
	$(top_builddir)/libgthumb/jpegutils/libgthumb-jpegutils.la

gthumb-marshal.h: gthumb-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $(srcdir)/gthumb-marshal.list --header --prefix=gthumb_marshal > $@

gthumb-marshal.c: gthumb-marshal.h gthumb-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gthumb-marshal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) $(srcdir)/gthumb-marshal.list --body --prefix=gthumb_marshal >> $@

gthumb-enum-types.h: $(HEADER_FILES) $(GLIB_MKENUMS)
	$(GLIB_MKENUMS) \
		--fhead "#ifndef GTHUMB_ENUM__TYPES_H\n#define GTHUMB_ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* GTHUMB_ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) gthumb-enum-types.h || cp xgen-$(@F) gthumb-enum-types.h ) \
	&& rm -f xgen-$(@F)

gthumb-enum-types.c: $(HEADER_FILES) gthumb-enum-types.h
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
                $^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) gthumb-enum-types.c || cp xgen-$(@F) gthumb-enum-types.c ) \
        && rm -f xgen-$(@F)

CLEANFILES = $(MARSHALLERS) $(ENUMS)
EXTRA_DIST = gthumb-marshal.list
