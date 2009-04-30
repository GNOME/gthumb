if ENABLE_DEPRECATIONS
DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED
else
DISABLE_DEPRECATED =
endif

INCLUDES = 				\
	-I$(top_srcdir)/libgthumb	\
	$(DISABLE_DEPRECATED)		\
	$(EXIF_CFLAGS)			\
	$(GTHUMB_CFLAGS)


noinst_LTLIBRARIES = libgthumb-jpegutils.la

libgthumb_jpegutils_la_SOURCES =	\
	jpeg-memory-mgr.c			\
	jpeg-memory-mgr.h			\
	jpeg-data.c			\
	jpeg-data.h			\
	jpeg-marker.c			\
	jpeg-marker.h			\
	jpegtran.c			\
	jpegtran.h			\
	transupp.c			\
	transupp.h

EXTRA_DIST = README

-include $(top_srcdir)/git.mk
