## Process this file with automake to produce Makefile.in.
SUBDIRS =		\
	libgthumb	\
	src		\
	components	\
	doc		\
	omf-install	\
	po

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files=gthumb.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

applicationsdir = $(datadir)/application-registry
applications_DATA = gthumb.applications

icondir = $(datadir)/pixmaps
icon_DATA = gthumb.png

man_MANS = gthumb.1

EXTRA_DIST = 			\
        AUTHORS                 \
        NEWS                    \
        README                  \
	acinclude.m4		\
	add-include-prefix	\
	gthumb.spec.in          \
	gthumb.spec		\
	intltool-merge.in 	\
        intltool-update.in 	\
        intltool-extract.in	\
	omf.make		\
	xmldocs.make		\
	$(applications_DATA)	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	$(icon_DATA)		\
	$(man_MANS)
