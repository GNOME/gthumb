themename  = Classic
themedir   = $(datadir)/gthumb/albumthemes/$(themename)
theme_DATA =			\
	1.gif			\
	background.gif		\
	back.png		\
	bot.png			\
	left.png		\
	next.png		\
	prev.png		\
	right.png		\
	top.png

%.gthtml : %.xml %.xml.in
	-mv -f $< $@

xmldir = $(datadir)/gthumb/albumthemes/$(themename)
xml_in_files = index.xml.in image.xml.in thumbnail.xml.in
xml_DATA = $(xml_in_files:.xml.in=.gthtml)

@INTLTOOL_XML_RULE@

EXTRA_DIST = $(theme_DATA) $(xml_in_files) $(xml_DATA)
CLEANFILES = $(xml_DATA)
