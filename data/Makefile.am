SUBDIRS = albumthemes glade icons

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files=gthumb.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

man_MANS = gthumb.1

schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gthumb.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

serverdir = $(libdir)/bonobo/servers
server_DATA = GNOME_GThumb.server

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$(schema_DATA)
endif

EXTRA_DIST = 			\
	gthumb.schemas		\
	gthumb.schemas.in	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	$(man_MANS)		\
	$(server_DATA)

DISTCLEANFILES =                \
	$(schema_DATA)		\
	$(desktop_DATA)
