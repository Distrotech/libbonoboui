noinst_PROGRAMS = bonoboui-hello

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	-DBONOBO_UIDIR=\""$(datadir)/gnome/ui/"\"	\
	-DG_LOG_DOMAIN=\"Bonobo\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(WARN_CFLAGS)					\
	$(BONOBOUI_CFLAGS)				\
	-DHELLO_DATADIR=\""$(datadir)"\"

# Notice that we #define HELLO_DATADIR here.
# This would normally be done in your app's config.h

LDADD =	$(top_builddir)/bonobo/libbonoboui-2.la

bonoboui_hello_SOURCES =	\
	helloapp.c		\
	helloapp.h	\
	main.c

EXTRA_DIST = bonoboui-hello-ui.xml
