
SUBDIRS = container bonobo-hello

bin_PROGRAMS = paint-component-simple sample-canvas-item

GPRINT_LIBS = $(top_builddir)/bonobo/libbonobo-print.la

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
	$(OBJECT_DIRECTORY_LIBS)		\
	$(INTLLIBS)

sample_canvas_item_SOURCES =	\
	sample-canvas-item.c

sample_canvas_item_LDADD = 			\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(top_builddir)/bonobo/libbonobox.la	\
	$(GPRINT_LIBS)				\
	$(GNOME_PRINT_LIBS)			\
	$(OBJECT_DIRECTORY_LIBS)		\
	$(INTLLIBS)

oafdir = $(datadir)/oaf
OAF_FILES =	paint-component-simple.oafinfo item.oafinfo
oaf_DATA = $(OAF_FILES)

oafdir = $(datadir)/oaf
oaf_DATA = paint-component-simple.oafinfo

EXTRA_DIST = $(OAF_FILES)

