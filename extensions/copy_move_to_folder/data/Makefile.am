schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gthumb-copy-move-to-folder.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/extensions/copy_move_to_folder/data/$(schema_DATA)
endif

EXTRA_DIST = $(schema_in_files)

CLEANFILES = $(schema_DATA)

-include $(top_srcdir)/git.mk
