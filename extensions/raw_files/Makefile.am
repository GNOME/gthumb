extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libraw_files.la

libraw_files_la_SOURCES =	\
	main.c			\
	main.h

if ENABLE_LIBRAW
libraw_files_la_SOURCES += 		\
	gth-metadata-provider-raw.c	\
	gth-metadata-provider-raw.h	
endif

libraw_files_la_CFLAGS = $(GTHUMB_CFLAGS) $(LIBRAW_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libraw_files_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libraw_files_la_LIBADD = $(GTHUMB_LIBS) $(LIBRAW_LIBS)
libraw_files_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = raw_files.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
