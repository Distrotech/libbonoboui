GPRINT_LIBS = $(top_builddir)/bonobo/libbonobo-print.la

if OAF
OBJECT_DIRECTORY_LIBS=$(OAF_LIBS) $(GNOMEUI_LIBS)
else
OBJECT_DIRECTORY_LIBS=$(GNOMEGNORBA_LIBS)
endif

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)				\
	$(OAF_CFLAGS)					\
	$(GTK_CFLAGS)

bin_PROGRAMS = bonobo-hello

bonobo_hello_SOURCES =		\
	bonobo-hello.c		\
	hello-embeddable.c	\
	hello-embeddable.h	\
	hello-object-io.h	\
	hello-object-io.c	\
	hello-view.c		\
	hello-view.h		\
	hello-object-print.h	\
	hello-object-print.c

bonobo_hello_LDADD = 				\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(top_builddir)/bonobo/libbonobox.la	\
	$(GPRINT_LIBS)				\
	$(GNOME_PRINT_LIBS)			\
	$(INTLLIBS)                             \
	$(OBJECT_DIRECTORY_LIBS)

gnorbadir = $(sysconfdir)/CORBA/servers
oafdir = $(datadir)/oaf

gnorbafile = hello.gnorba
oaffile = hello.oafinfo

if OAF
oaf_DATA = $(oaffile)
else
gnorba_DATA = $(gnorbafile)
endif

EXTRA_DIST = $(gnorbafile) $(oaffile)
