libexec_PROGRAMS = gthumb-image-viewer
noinst_PROGRAMS = test-component

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

gthumb_image_viewer_SOURCES = 			\
	$(nautilus_view_component_idl_sources)	\
	iids.h					\
	main.c					\
	ui.c					\
	ui.h					\
	viewer-control.c			\
	viewer-control.h			\
	viewer-nautilus-view.c			\
	viewer-nautilus-view.h			\
	viewer-stream.c				\
	viewer-stream.h

gthumb_image_viewer_LDADD = 			\
	$(COMPONENT_LIBS)			\
	$(EXIF_LIBS)				\
	$(JPEG_LIBS)				\
	$(IPTCDATA_LIBS)			\
	$(TIFF_LIBS)				\
	$(RENDER_LIBS)				\
	$(top_builddir)/libgthumb/libgthumb.la

$(nautilus_view_component_idl_sources): nautilus_view_component_idl_stamp
nautilus_view_component_idl_stamp: nautilus-view-component.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $(IDL_INCLUDES) $<
	touch $@

test_component_SOURCES =		\
	test-component.c

test_component_LDADD =			\
	$(COMPONENT_LIBS)

server_in_files = GNOME_GThumb_Viewer.server.in.in

serverdir = $(libdir)/bonobo/servers
server_DATA = GNOME_GThumb_Viewer.server
GNOME_GThumb_Viewer.server.in: GNOME_GThumb_Viewer.server.in.in Makefile
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|g" \
	< $(srcdir)/GNOME_GThumb_Viewer.server.in.in > GNOME_GThumb_Viewer.server.temp \
	&& mv -f GNOME_GThumb_Viewer.server.temp GNOME_GThumb_Viewer.server.in

@INTLTOOL_SERVER_RULE@

GNOME_GThumb_Viewer.xml: GNOME_GThumb_Viewer.xml.in Makefile
	sed -e 's?\@ICONSDIR\@?$(datadir)/gthumb/icons?g' \
	< $(srcdir)/GNOME_GThumb_Viewer.xml.in > GNOME_GThumb_Viewer.xml.temp \
	&& mv -f GNOME_GThumb_Viewer.xml.temp GNOME_GThumb_Viewer.xml

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = GNOME_GThumb_Viewer.xml

iconsdir = $(datadir)/gthumb/icons
icons_DATA =				\
	black-white.png			\
	rotate.png			\
	rotate.xpm			\
	flip.xpm			\
	flip.png			\
	mirror.xpm			\
	mirror.png

EXTRA_DIST =				\
	$(icons_DATA)			\
	$(server_in_files) 		\
        $(server_DATA)     		\
        GNOME_GThumb_Viewer.xml.in	\
	nav_button.xpm  		\
	nautilus-view-component.idl

DISTCLEANFILES =                        	\
	$(nautilus_view_component_idl_sources)	\
	nautilus_view_component_idl_stamp	\
	$(ui_DATA)                 		\
	GNOME_GThumb_Viewer.server.in		\
	$(server_DATA)

