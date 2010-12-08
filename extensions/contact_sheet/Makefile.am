SUBDIRS = data

extensiondir = $(pkglibdir)/extensions
extension_LTLIBRARIES = libcontact_sheet.la

libcontact_sheet_la_SOURCES = 		\
	actions.c			\
	actions.h			\
	callbacks.c			\
	callbacks.h			\
	dlg-contact-sheet.c		\
	dlg-contact-sheet.h		\
	gth-contact-sheet-creator.c	\
	gth-contact-sheet-creator.h	\
	main.c				\
	preferences.h

if RUN_IN_PLACE
contact_sheet_datadir = $(abs_top_srcdir)/extensions/contact_sheet/data
else
contact_sheet_datadir = $(pkgdatadir)
endif

libcontact_sheet_la_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir) -I$(top_builddir)/gthumb -DCONTACT_SHEET_DATADIR=\"$(contact_sheet_datadir)\"
libcontact_sheet_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)
libcontact_sheet_la_LIBADD = $(GTHUMB_LIBS) ../image_rotation/libimage_rotation.la ../export_tools/libexport_tools.la
libcontact_sheet_la_DEPENDENCIES = $(top_builddir)/gthumb/gthumb$(EXEEXT)

extensioninidir = $(extensiondir)
extensionini_DATA = $(extensionini_in_files:.extension.in.in=.extension)
DISTCLEANFILES = $(extensionini_DATA)

extensionini_in_files = contact_sheet.extension.in.in

@GTHUMB_EXTENSION_IN_RULE@
@GTHUMB_EXTENSION_RULE@

EXTRA_DIST = $(extensionini_in_files)

CLEANFILES = $(BUILT_SOURCES) 

-include $(top_srcdir)/git.mk
