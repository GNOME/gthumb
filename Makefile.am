## Process this file with automake to produce Makefile.in.
SUBDIRS =		\
	po		\
	libgthumb	\
	src		\
	components	\
	doc		\
	data

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files=gthumb.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

applicationsdir = $(datadir)/application-registry
applications_DATA = gthumb.applications

icondir = $(datadir)/pixmaps
icon_DATA = gthumb.png

man_MANS = gthumb.1

distcleancheck_listfiles = find . -type f -print | grep -v 'omf\.out'

distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper' | grep -v 'omf' | grep -v 'figures'


EXTRA_DIST = 			\
        AUTHORS                 \
        NEWS                    \
        README                  \
	add-include-prefix	\
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

DISTCLEANFILES =                	\
	po/.intltool-merge-cache	\
	$(desktop_DATA)			\
	intltool-extract        	\
	intltool-merge          	\
	intltool-update
