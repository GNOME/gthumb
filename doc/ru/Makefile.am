gthumb_helpdir = $(datadir)/gnome/help/gthumb/ru
gthumb_help_DATA =	\
        index.html	\
        topic.dat

SGML_FILES =		\
        gthumb.sgml

EXTRA_DIST =		\
	gthumb.1	\
        topic.dat	\
        $(SGML_FILES)


## The - before the command means to ignore it if it fails.  that way
## people can still build the software without the docbook tools

all: 

index.html: gthumb/index.html
	-cp -f gthumb/index.html index.html

gthumb/index.html: $(SGML_FILES)
	-db2html gthumb.sgml

## when we make dist, we include the generated HTML so people don't
## have to have the docbook tools
dist-hook:
	mkdir $(distdir)/gthumb
	-cp gthumb/*.html gthumb/*.css $(distdir)/gthumb
	-cp index.html $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(gthumb_helpdir)/images
	-for file in $(srcdir)/gthumb/*.html $(srcdir)/gthumb/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $(srcdir)/$$file $(gthumb_helpdir)/$$basefile; \
	done

gthumb.ps: gthumb.sgml
	-db2ps $<

gthumb.rtf: gthumb.sgml       
	-db2rtf $<

