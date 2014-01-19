SUBDIRS = m4 data po gthumb extensions help

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =                    \
	AUTHORS                 \
	ChangeLog.pre-git	\
        MAINTAINERS             \
        NEWS                    \
        README                  \
	config.rpath		\
	intltool-merge.in       \
	intltool-update.in      \
	intltool-extract.in	\
	extension-resources.mk

DISTCLEANFILES =                        \
	po/.intltool-merge-cache        \
	intltool-extract                \
	intltool-merge                  \
	intltool-update

MAINTAINERCLEANFILES = 			\
	$(srcdir)/INSTALL		\
	$(srcdir)/aclocal.m4 		\
	$(srcdir)/autoscan.log 		\
	$(srcdir)/compile 		\
	$(srcdir)/config.guess 		\
	$(srcdir)/config.h.in 		\
	$(srcdir)/config.sub 		\
	$(srcdir)/configure.scan 	\
	$(srcdir)/depcomp 		\
	$(srcdir)/gnome-doc-utils.make	\
	$(srcdir)/install-sh 		\
	$(srcdir)/ltmain.sh 		\
	$(srcdir)/omf.make 		\
	$(srcdir)/m4/intltool.m4	\
	$(srcdir)/m4/libtool.m4		\
	$(srcdir)/m4/lt~obsolete.m4	\
	$(srcdir)/m4/ltoptions.m4	\
	$(srcdir)/m4/ltsugar.m4		\
	$(srcdir)/m4/ltversion.m4	\
	$(srcdir)/m4/gnome-doc-utils.m4	\
	$(srcdir)/missing 		\
	$(srcdir)/mkinstalldirs 	\
	$(srcdir)/xmldocs.make 		\
	$(srcdir)/gtk-doc.make 		\
	$(srcdir)/ChangeLog 		\
	`find "$(srcdir)" -type f -name Makefile.in -print`

GITIGNOREFILES = build .autotools .cdt* .cproject .project .settings/ *.patch

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

CLEANFILES = ChangeLog

# Build ChangeLog from GIT history

CHANGELOG_START = GTHUMB_2_10_11
ChangeLog:
	@echo Creating $@
	@if test -d $(top_srcdir)/.git; then \
		(GIT_DIR=$(top_srcdir)/.git $(top_srcdir)/missing --run git log $(CHANGELOG_START).. --stat -M -C --name-status  --date=short --no-color) | fmt --split-only > $@.tmp \
		 && mv -f $@.tmp $@ \
		 || ($(RM) $@.tmp; \
		     echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
		     (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
		test -f $@ || \
		(echo A git checkout and git-log is required to generate ChangeLog >&2 && \
		echo A git checkout and git-log is required to generate this file >> $@); \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
