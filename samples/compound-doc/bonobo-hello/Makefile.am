GPRINT_LIBS = $(top_builddir)/bonobo/libbonobo-print.la

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(BONOBOX_TEST_CFLAGS)

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

bonobo_hello_LDADD =					\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(top_builddir)/bonobo/libbonobox.la		\
	$(top_builddir)/bonobo/libbonobo-print.la	\
	$(BONOBOX_TEST_LIBS)				\
	$(INTLLIBS)

oafdir = $(datadir)/oaf
oaffile = hello.oafinfo

oaf_DATA = $(oaffile)

EXTRA_DIST = $(oaffile)
