SUBDIRS = appdata icons ui

desktopdir = $(datadir)/applications
desktop_in_in_files = gthumb.desktop.in.in gthumb-import.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

@INTLTOOL_DESKTOP_RULE@

gsettings_ENUM_NAMESPACE = org.gnome.gthumb
gsettings_ENUM_FILES = $(top_srcdir)/gthumb/*.h $(top_srcdir)/extensions/*/*.h
gsettings_SCHEMAS = 					\
	org.gnome.gthumb.gschema.xml			\
	org.gnome.gthumb.change-date.gschema.xml	\
	org.gnome.gthumb.comments.gschema.xml		\
	org.gnome.gthumb.contact-sheet.gschema.xml	\
	org.gnome.gthumb.convert-format.gschema.xml	\
	org.gnome.gthumb.crop.gschema.xml		\
	org.gnome.gthumb.facebook.gschema.xml		\
	org.gnome.gthumb.file-manager.gschema.xml	\
	org.gnome.gthumb.flickr.gschema.xml		\
	org.gnome.gthumb.gstreamer-tools.gschema.xml	\
	org.gnome.gthumb.image-print.gschema.xml	\
	org.gnome.gthumb.image-viewer.gschema.xml	\
	org.gnome.gthumb.importer.gschema.xml		\
	org.gnome.gthumb.photo-importer.gschema.xml	\
	org.gnome.gthumb.picasaweb.gschema.xml		\
	org.gnome.gthumb.pixbuf-savers.gschema.xml	\
	org.gnome.gthumb.rename-series.gschema.xml	\
	org.gnome.gthumb.resize.gschema.xml		\
	org.gnome.gthumb.resize-images.gschema.xml	\
	org.gnome.gthumb.rotate.gschema.xml		\
	org.gnome.gthumb.slideshow.gschema.xml		\
	org.gnome.gthumb.webalbums.gschema.xml

@GSETTINGS_RULES@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = gthumb.convert

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gthumb-$(GTHUMB_API_VERSION).pc

man_MANS=gthumb.1

EXTRA_DIST =                    	\
	$(desktop_in_in_files)  	\
	$(gsettings_SCHEMAS)  		\
	$(pkgconfig_DATA)		\
	gthumb.convert			\
	$(man_MANS)

DISTCLEANFILES =                	\
	$(desktop_in_files)		\
	$(pkgconfig_DATA)

CLEANFILES =				\
	$(desktop_DATA)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
