# rule to build the resources .c file from the xml file

RESOURCES_DEP = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir) $(srcdir)/resources.xml)
resources.c: resources.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gth $(srcdir)/resources.xml
resources.h: resources.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gth $(srcdir)/resources.xml