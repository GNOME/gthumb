DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DBONOBO_DISABLE_DEPRECATED

INCLUDES = 				\
	$(DISABLE_DEPRECATED)		\
	$(EXIF_CFLAGS)			\
	$(GTHUMB_CFLAGS)		\
	-I$(top_srcdir)/libgthumb		


noinst_LIBRARIES = libgthumb-jpegutils.a

libgthumb_jpegutils_a_SOURCES =		\
	jpeg-data.c			\
	jpeg-data.h			\
	jpeg-marker.c			\
	jpeg-marker.h			\
	jpegtran.c			\
	jpegtran.h			\
	transupp.c			\
	transupp.h

EXTRA_DIST = README
