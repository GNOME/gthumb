extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libwork_queues.la

libwork_queues_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	gth-file-source-work-queues.c 	\
	gth-file-source-work-queues.h	\
	gth-queue-manager.c		\
	gth-queue-manager.h		\
	main.c

libwork_queues_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb 
libwork_queues_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libwork_queues_la_LIBADD = $(GTHUMB_LIBS)
libwork_queues_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = work_queues.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files) 

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
