SUBDIRS = albumthemes glade

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files=gthumb.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

applicationsdir = $(datadir)/application-registry
applications_DATA = gthumb.applications

icondir = $(datadir)/pixmaps
icon_DATA = gthumb.png

man_MANS = gthumb.1

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = gthumb-ui.xml 

schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gthumb.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
endif

EXTRA_DIST = 			\
	$(ui_DATA)		\
	gthumb.schemas		\
	gthumb.schemas.in	\
	$(applications_DATA)	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	$(icon_DATA)		\
	$(man_MANS)


DISTCLEANFILES =                \
	$(schema_DATA)		\
	$(desktop_DATA)	
