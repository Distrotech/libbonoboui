
if OAF
bin_PROGRAMS =
else
bin_PROGRAMS = paint-component-simple sample-container
endif

if GPRINT
GPRINT_LIBS = $(top_builddir)/bonobo/libbonobo-print.la
else
GPRINT_LIBS =
endif

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)				\
	$(GTK_CFLAGS)

paint_component_simple_SOURCES =	\
	paint-component-simple.c

paint_component_simple_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(BONOBO_LIBS)				\
	$(GPRINT_LIBS)				\
	$(INTLLIBS)

sample_container_SOURCES =		\
	sample-container.c

sample_container_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(BONOBO_LIBS)				\
	$(GPRINT_LIBS)				\
	$(INTLLIBS)

gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = paint-component-simple.gnorba

EXTRA_DIST = $(gnorba_DATA)

