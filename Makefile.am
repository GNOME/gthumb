## Process this file with automake to produce Makefile.in.
SUBDIRS =		\
	po		\
	libgthumb	\
	src		\
	components	\
	doc		\
	data

distcleancheck_listfiles = find . -type f -print | grep -v 'omf\.out'

distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper' | grep -v 'omf' | grep -v 'figures'


EXTRA_DIST = 			\
        AUTHORS                 \
	MAINTAINERS		\
        NEWS                    \
        README                  \
	add-include-prefix	\
	intltool-merge.in 	\
        intltool-update.in 	\
        intltool-extract.in	\
	omf.make		\
	xmldocs.make		\
	gnome-doc-utils.make

DISTCLEANFILES =                	\
	po/.intltool-merge-cache	\
	intltool-extract        	\
	intltool-merge          	\
	intltool-update			\
	gnome-doc-utils.make

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper
