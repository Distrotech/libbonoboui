
bin_PROGRAMS = sample-control-factory sample-control-container

if OAF
OBJECT_DIRECTORY_LIBS=$(OAF_LIBS) $(GNOMEUI_LIBS)
else
OBJECT_DIRECTORY_LIBS=$(GNOMEGNORBA_LIBS)
endif

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
	$(OBJECT_DIRECTORY_LIBS)			\
	$(INTLLIBS)

sample_control_container_SOURCES =			\
	sample-control-container.c

sample_control_container_LDADD =			\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(OBJECT_DIRECTORY_LIBS)			\
	$(INTLLIBS)

goaddir = $(sysconfdir)/CORBA/servers
oafdir = $(datadir)/oaf

GOAD_FILES =	bonobo-clock-control.gnorba		\
		bonobo-calculator-control.gnorba

OAF_FILES =	bonobo-clock-control.oafinfo		\
		bonobo-calculator-control.oafinfo

#if OAF
goad_DATA = $(GOAD_FILES)
#else
oaf_DATA = $(OAF_FILES)
#endif


EXTRA_DIST = $(GOAD_FILES) $(OAF_FILES)



