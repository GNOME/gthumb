libexec_PROGRAMS = gthumb-catalog-view

if ENABLE_DEPRECATIONS
DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DBONOBO_DISABLE_DEPRECATED
else
DISABLE_DEPRECATED =
endif

INCLUDES =					\
	-DGNOMEDATADIR=\""$(datadir)"\" 	\
	$(COMPONENT_CFLAGS)			\
	$(DISABLE_DEPRECATED)			\
	-I$(top_srcdir)/libgthumb

nautilus_view_component_idl_sources =           \
	nautilus-view-component-stubs.c         \
	nautilus-view-component-skels.c         \
	nautilus-view-component.h               \
	nautilus-view-component-common.c

gthumb_catalog_view_SOURCES = 			\
	$(nautilus_view_component_idl_sources)	\
	iids.h					\
	main.c					\
	catalog-control.c			\
	catalog-control.h			\
	catalog-nautilus-view.c			\
	catalog-nautilus-view.h

gthumb_catalog_view_LDADD = 			\
	$(COMPONENT_LIBS)			\
	$(JPEG_LIBS)				\
	$(PNG_LIBS)				\
	$(TIFF_LIBS)				\
	$(RENDER_LIBS)				\
	$(top_builddir)/libgthumb/libgthumb.la

$(nautilus_view_component_idl_sources): nautilus_view_component_idl_stamp
nautilus_view_component_idl_stamp: nautilus-view-component.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $(VIEWER_IDL_INCLUDES) $<
	touch $@

server_in_files = GNOME_GThumb_CatalogView.server.in.in

serverdir = $(libdir)/bonobo/servers
server_DATA = GNOME_GThumb_CatalogView.server
GNOME_GThumb_CatalogView.server.in: GNOME_GThumb_CatalogView.server.in.in Makefile
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|g" \
	< $(srcdir)/GNOME_GThumb_CatalogView.server.in.in > temp \
	&& mv temp GNOME_GThumb_CatalogView.server.in

@INTLTOOL_SERVER_RULE@

GNOME_GThumb_CatalogView.xml: GNOME_GThumb_CatalogView.xml.in Makefile
	sed -e 's?\@ICONSDIR\@?$(datadir)/gthumb/icons?g' \
	< $(srcdir)/GNOME_GThumb_CatalogView.xml.in > temp \
	&& mv temp GNOME_GThumb_CatalogView.xml

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = GNOME_GThumb_CatalogView.xml

EXTRA_DIST =				\
	$(server_in_files) 		\
        $(server_DATA)     		\
        GNOME_GThumb_CatalogView.xml.in	\
	nautilus-view-component.idl
