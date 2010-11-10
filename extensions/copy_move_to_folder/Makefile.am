SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcopy_move_to_folder.la

libcopy_move_to_folder_la_SOURCES = \
	actions.c \
	actions.h \
	callbacks.c \
	callbacks.h \
	preferences.h \
	dlg-copy-complete.c \
	dlg-copy-complete.h \
	dlg-copy-move-to-folder-preferences.c \
	dlg-copy-move-to-folder-preferences.h \
	main.c

libcopy_move_to_folder_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb
libcopy_move_to_folder_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcopy_move_to_folder_la_LIBADD = $(GTHUMB_LIBS)  ../file_manager/libfile_manager.la
libcopy_move_to_folder_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_in_files = copy_move_to_folder.extension.in.in
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

DISTCLEANFILES = $(extensionini_DATA)

-include $(top_srcdir)/git.mk
