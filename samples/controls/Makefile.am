
bin_PROGRAMS = sample-control-factory sample-control-container

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)				\
	$(GTK_CFLAGS)

sample_control_factory_SOURCES =			\
	bonobo-clock-control.c				\
	bonobo-clock-control.h				\
	bonobo-calculator-control.c			\
	bonobo-calculator-control.h			\
	sample-control-factory.c

sample_control_factory_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(GNOMEGNORBA_LIBS)				\
	$(INTLLIBS)

sample_control_container_SOURCES =			\
	sample-control-container.c

sample_control_container_LDADD =			\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(GNOMEGNORBA_LIBS)				\
	$(INTLLIBS)

gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA =	bonobo-clock-control.gnorba		\
		bonobo-calculator-control.gnorba

EXTRA_DIST = $(gnorba_DATA)

