## Process this file with automake to produce Makefile.in.
AMCFLAGS = -fPIC -DPIC

SUBDIRS =		\
	po		\
	libgthumb	\
	src		\
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
	gnome-doc-utils.make	\
	ChangeLog.pre-git

DISTCLEANFILES =                	\
	po/.intltool-merge-cache	\
	intltool-extract        	\
	intltool-merge          	\
	intltool-update			\
	gnome-doc-utils.make

CLEANFILES = \
	ChangeLog

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

-include $(top_srcdir)/git.mk

# Build ChangeLog from GIT history
ChangeLog:
	@if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog
