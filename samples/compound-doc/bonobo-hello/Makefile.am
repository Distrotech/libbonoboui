GPRINT_LIBS = $(top_builddir)/bonobo/libbonobo-print.la

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

oafdir = $(datadir)/oaf
oaffile = hello.oafinfo

oaf_DATA = $(oaffile)

EXTRA_DIST = $(oaffile)
