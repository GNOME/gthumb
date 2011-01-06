noinst_LTLIBRARIES = libeggsmclient.la

libeggsmclient_la_LIBADD = $(SMCLIENT_LIBS) \
			   $(GTK_LIBS)

libeggsmclient_la_CFLAGS = -DG_LOG_DOMAIN=\""EggSMClient"\" \
			   -DEGG_SM_CLIENT_BACKEND_XSMP     \
			   $(SMCLIENT_CFLAGS)               \
			   $(GTK_CFLAGS)

libeggsmclient_la_SOURCES = eggdesktopfile.h \
			    eggdesktopfile.c \
			    eggsmclient.h \
			    eggsmclient.c \
			    eggsmclient-private.h \
			    eggsmclient-xsmp.c

-include $(top_srcdir)/git.mk
