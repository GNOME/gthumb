if BUILD_TEST_SUITE
bin_PROGRAMS = dom-test glib-utils-test
endif

dom_test_SOURCES = dom-test.c $(top_srcdir)/gthumb/dom.c
dom_test_LDADD = $(GTHUMB_LIBS) 
dom_test_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir)/gthumb

glib_utils_test_SOURCES = glib-utils-test.c $(top_srcdir)/gthumb/glib-utils.c
glib_utils_test_LDADD = $(GTHUMB_LIBS) 
glib_utils_test_CFLAGS = $(GTHUMB_CFLAGS) $(DISABLE_DEPRECATED) $(WARNINGS) -I$(top_srcdir)/gthumb
-include $(top_srcdir)/git.mk
