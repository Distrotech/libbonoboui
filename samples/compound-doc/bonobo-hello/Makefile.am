Bonobo_Sample_Hello.server.in: Bonobo_Sample_Hello.server.in.in
	sed -e "s|\@prefix\@|$(prefix)|" $< > $@

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(LIBBONOBOUI_CFLAGS)

samplesdir = $(prefix)/lib/bonobo-2.0/samples
samples_PROGRAMS = bonobo-sample-hello

bonobo_sample_hello_SOURCES =	\
	bonobo-hello.c		\
	hello-embeddable.c	\
	hello-embeddable.h	\
	hello-object-io.h	\
	hello-object-io.c	\
	hello-view.c		\
	hello-view.h		\
	hello-object-print.h	\
	hello-object-print.c

bonobo_sample_hello_LDADD =				\
	$(top_builddir)/bonobo/libbonoboui-2.la		\
	$(LIBBONOBOUI_LIBS)				\
	$(INTLLIBS)

serverdir = $(libdir)/bonobo/servers
server_in_files = Bonobo_Sample_Hello.server.in
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = $(server_in_files) $(server_DATA)
