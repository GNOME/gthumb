SUBDIRS = albumthemes glade

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files=gthumb.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

icondir = $(datadir)/icons/hicolor/48x48/apps
icon_DATA = gthumb.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi


man_MANS = gthumb.1

schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gthumb.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$(schema_DATA)
endif

EXTRA_DIST = 			\
	gthumb.schemas		\
	gthumb.schemas.in	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	$(icon_DATA)		\
	$(man_MANS)

DISTCLEANFILES =                \
	$(schema_DATA)		\
	$(desktop_DATA)
