SUBDIRS = ui

schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gthumb-image-viewer.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$(schema_DATA)
endif

EXTRA_DIST = 				\
	gthumb-image-viewer.schemas	\
	gthumb-image-viewer.schemas.in

DISTCLEANFILES = $(schema_DATA)
-include $(top_srcdir)/git.mk
