
bin_PROGRAMS = paint-component-simple

paint_component_simple_SOURCES =	\
	paint-component-simple.c

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. 						\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)				\
	$(GTK_CFLAGS)

paint_component_simple_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(GTK_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(GNOMEGNORBA_LIBS)			\
	$(INTLLIBS)

gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = paint-component-simple.gnorba

EXTRA_DIST = $(gnorba_DATA)

