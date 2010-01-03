SUBDIRS = data

extensiondir = $(libdir)/gthumb-2.0/extensions
extension_LTLIBRARIES = libpixbuf_savers.la

ENUM_TYPES =				\
	enum-types.h			\
	enum-types.c

HEADER_FILES = 				\
	preferences.h

enum-types.h: $(HEADER_FILES) $(GLIB_MKENUMS)
	$(GLIB_MKENUMS) \
		--fhead "#ifndef ENUM_TYPES_H\n#define ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) enum-types.h || cp xgen-$(@F) enum-types.h ) \
	&& rm -f xgen-$(@F)

enum-types.c: $(HEADER_FILES) enum-types.h
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) enum-types.c || cp xgen-$(@F) enum-types.c ) \
	&& rm -f xgen-$(@F)

libpixbuf_savers_la_SOURCES = 	\
	$(ENUM_TYPES)		\
	$(HEADER_FILES)		\
	gth-jpeg-saver.c	\
	gth-jpeg-saver.h	\
	gth-png-saver.c		\
	gth-png-saver.h		\
	gth-tga-saver.c		\
	gth-tga-saver.h		\
	gth-tiff-saver.c	\
	gth-tiff-saver.h	\
	main.c			\
	preferences.c

libpixbuf_savers_la_CFLAGS = $(GTHUMB_CFLAGS) $(CLUTTER_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libpixbuf_savers_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libpixbuf_savers_la_LIBADD = $(GTHUMB_LIBS) $(TIFF_LIBS)
libpixbuf_savers_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = pixbuf_savers.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

%.extension.in: %.extension.in.in $(extension_LTLIBRARIES)
	sed -e "s|%LIBRARY%|`. ./$(extension_LTLIBRARIES) && echo $$dlname`|" \
	$< > $@

%.extension: %.extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
