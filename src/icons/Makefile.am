IMAGES=				\
	add_comment.png		\
	add_to_catalog.png	\
	catalog.png		\
	catalog_search.png	\
	dir.png			\
	image-info-24.png	\
	image-info-16.png	\
	library.png		\
	toolbar_catalog.png	\
	toolbar_dir.png		\
	toolbar_fullscreen.png	\
	toolbar_prev_image.png	\
	toolbar_next_image.png	\
	flip.png		\
	mirror.png		\
	rotate90.png		\
	rotate270.png

noinst_DATA = pixbufs.h
CLEANFILES = $(noinst_DATA)

pixbufs.h: $(IMAGES)
	gdk-pixbuf-csource --raw --build-list add_comment_rgba $(srcdir)/add_comment.png > $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list add_to_catalog_rgba $(srcdir)/add_to_catalog.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list catalog_rgba $(srcdir)/catalog.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list catalog_search_rgba $(srcdir)/catalog_search.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list dir_rgba $(srcdir)/dir.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list image_info_24_rgba $(srcdir)/image-info-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list image_info_16_rgba $(srcdir)/image-info-16.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list library_rgba $(srcdir)/library.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list toolbar_fullscreen_rgba $(srcdir)/toolbar_fullscreen.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list toolbar_catalog_rgba $(srcdir)/toolbar_catalog.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list toolbar_dir_rgba $(srcdir)/toolbar_dir.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list toolbar_prev_image_rgba $(srcdir)/toolbar_prev_image.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list toolbar_next_image_rgba $(srcdir)/toolbar_next_image.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list flip_rgba $(srcdir)/flip.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list mirror_rgba $(srcdir)/mirror.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list rotate90_rgba $(srcdir)/rotate90.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list rotate270_rgba $(srcdir)/rotate270.png >> $(srcdir)/pixbufs.h

ICONS=				\
	flip.xpm		\
	layout1.xpm		\
	layout2.xpm		\
	layout3.xpm		\
	layout4.xpm		\
	mirror.xpm		\
	nav_button.xpm		\
	rotate.xpm

EXTRA_DIST = \
	$(IMAGES)		\
	$(ICONS)		\
	pixbufs.h
