
bin_PROGRAMS = paint-component-simple sample-container

GPRINT_LIBS = $(top_builddir)/bonobo/libbonobo-print.la

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

paint_component_simple_SOURCES =	\
	paint-component-simple.c

paint_component_simple_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(GPRINT_LIBS)				\
	$(GNOME_PRINT_LIBS)			\
	$(INTLLIBS)                             \
	$(OBJECT_DIRECTORY_LIBS)

sample_container_SOURCES =		\
	sample-container.c

sample_container_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(GPRINT_LIBS)				        \
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(GNOME_PRINT_LIBS)				\
	$(OBJECT_DIRECTORY_LIBS)			\
	$(INTLLIBS)

sample_container_LDFLAGS = -static


goaddir = $(sysconfdir)/CORBA/servers
oafdir = $(datadir)/oaf

GOAD_FILES =    paint-component-simple.gnorba
OAF_FILES =	paint-component-simple.oafinfo

#if OAF
goad_DATA = $(GOAD_FILES)
#else
oaf_DATA = $(OAF_FILES)
#endif

oafdir = $(datadir)/oaf
oaf_DATA = paint-component-simple.oafinfo

extra_DIST = $(GOAD_FILES) $(OAF_FILES)













