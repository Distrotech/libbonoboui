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

bin_PROGRAMS = sample-container

sample_container_SOURCES = \
	container.c container.h \
	component.c component.h \
	container-io.h container-io.c \
	container-menu.h container-menu.c \
	container-print.h container-print.c

sample_container_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(GPRINT_LIBS)				        \
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(GNOME_PRINT_LIBS)				\
	$(OBJECT_DIRECTORY_LIBS)			\
	$(INTLLIBS)
