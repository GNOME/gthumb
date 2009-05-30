INCLUDES = 				\
	-I$(top_srcdir)/libgthumb	\
	$(DISABLE_DEPRECATED)		\
	$(WARNINGS)			\
	$(GTHUMB_CFLAGS)

noinst_LTLIBRARIES = libgthumb-jpegutils.la

libgthumb_jpegutils_la_SOURCES =	\
	jpegtran.c			\
	jpegtran.h			\
	transupp.c			\
	transupp.h

EXTRA_DIST = README

-include $(top_srcdir)/git.mk
