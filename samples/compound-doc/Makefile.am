
SUBDIRS = container bonobo-hello

bin_PROGRAMS = paint-component-simple sample-canvas-item

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

paint_component_simple_SOURCES =	\
	paint-component-simple.c

paint_component_simple_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(top_builddir)/bonobo/libbonobox.la	\
	$(GPRINT_LIBS)				\
	$(GNOME_PRINT_LIBS)			\
	$(INTLLIBS)                             \
	$(OBJECT_DIRECTORY_LIBS)

sample_canvas_item_SOURCES =	\
	sample-canvas-item.c

sample_canvas_item_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(top_builddir)/bonobo/libbonobox.la	\
	$(GPRINT_LIBS)				\
	$(GNOME_PRINT_LIBS)			\
	$(INTLLIBS)                             \
	$(OBJECT_DIRECTORY_LIBS)

goaddir = $(sysconfdir)/CORBA/servers
oafdir = $(datadir)/oaf

GOAD_FILES =    paint-component-simple.gnorba  item.gnorba
OAF_FILES =	paint-component-simple.oafinfo item.oafinfo

#if OAF
goad_DATA = $(GOAD_FILES)
#else
oaf_DATA = $(OAF_FILES)
#endif

oafdir = $(datadir)/oaf
oaf_DATA = paint-component-simple.oafinfo

EXTRA_DIST = $(GOAD_FILES) $(OAF_FILES)

