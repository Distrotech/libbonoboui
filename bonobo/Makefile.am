
if OAF
OBJECT_DIRECTORY_LIBS=$(OAF_LIBS) $(GNOME_LIBS)
OBJECT_DIRECTORY_IMPL=bonobo-object-directory-oaf.c
else
OBJECT_DIRECTORY_LIBS=$(GNOMEGNORBA_LIBS)
OBJECT_DIRECTORY_IMPL=bonobo-object-directory-goad.c
endif

INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DSTORAGE_LIB=\""$(libdir)"\"			\
	-DG_LOG_DOMAIN=\"Bonobo\"			\
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
        -I$(includedir)                                 \
	$(GNOME_INCLUDEDIR)				\
	$(OAF_CFLAGS)					\
	$(GDK_PIXBUF_CFLAGS)

gnome_libs =				\
	$(GNOME_LIBDIR)			\
	$(GDK_PIXBUF_LIBS) 		\
        $(GNOMEUI_LIBS)			\
	$(OBJECT_DIRECTORY_LIBS) 	\
	$(INTLLIBS)

bonobo_libs = 			\
	libbonobo.la 		\
	$(gnome_libs)


lib_LTLIBRARIES = libbonobo.la libbonobox.la libbonobo-print.la

CORBA_SOURCE =			\
	Bonobo.h		\
	Bonobo-common.c		\
	Bonobo-stubs.c		\
	Bonobo-skels.c

idls =							\
	$(top_srcdir)/idl/Bonobo.idl			\
	$(top_srcdir)/idl/bonobo-advise.idl		\
	$(top_srcdir)/idl/bonobo-canvas.idl 		\
	$(top_srcdir)/idl/bonobo-client-site.idl	\
	$(top_srcdir)/idl/bonobo-control.idl		\
	$(top_srcdir)/idl/bonobo-embeddable.idl		\
	$(top_srcdir)/idl/bonobo-container.idl		\
	$(top_srcdir)/idl/bonobo-persist.idl		\
	$(top_srcdir)/idl/bonobo-sizeable.idl		\
	$(top_srcdir)/idl/bonobo-storage.idl		\
	$(top_srcdir)/idl/bonobo-progressive.idl	\
	$(top_srcdir)/idl/bonobo-ui-handler.idl		\
	$(top_srcdir)/idl/bonobo-property.idl

idl_flags = `$(GNOME_CONFIG) --cflags idl` -D__BONOBO_COMPILATION

$(CORBA_SOURCE): $(idls)
	$(ORBIT_IDL) $(top_srcdir)/idl/Bonobo.idl -I$(top_srcdir)/idl $(idl_flags)

Bonobo.h: $(idls)

libbonobo_la_LDFLAGS = -version-info 2:0:1 $(gnome_libs)

libbonobo_la_SOURCES =			\
	$(CORBA_SOURCE)			\
	bonobo-arg.h			\
	bonobo-arg.c			\
	bonobo-object-directory.c	\
	$(OBJECT_DIRECTORY_IMPL)	\
	bonobo-generic-factory.c	\
	bonobo-main.c			\
	bonobo-moniker.c		\
	bonobo-moniker-client.c		\
	bonobo-object.c			\
	bonobo-object-client.c		\
	bonobo-object-io.c		\
	bonobo-persist-file.c		\
	bonobo-persist-stream.c		\
	bonobo-persist.c		\
	bonobo-progressive.c		\
	bonobo-storage.c		\
	bonobo-stream.c			\
	bonobo-stream-client.c		\
	bonobo-stream-fs.c		\
	bonobo-stream-memory.c		\
	bonobo-property.c		\
	bonobo-property-bag.c		\
	bonobo-property-bag-client.c

libbonoboincludedir = $(includedir)/bonobo

libbonobodir = $(includedir)
libbonobo_HEADERS = bonobo.h

libbonoboinclude_HEADERS =		\
	Bonobo.h			\
	bonobo-arg.h			\
	bonobo-canvas-item.h		\
	bonobo-selector.h		\
	bonobo-widget.h 		\
	bonobo-canvas-component.h	\
	bonobo-client-site.h		\
	bonobo-component-io.h		\
	bonobo-container.h		\
	bonobo-control.h		\
	bonobo-control-frame.h		\
	bonobo-desktop-window.h		\
	bonobo-embeddable.h		\
	bonobo-embeddable-factory.h	\
	bonobo-generic-factory.h	\
	bonobo-main.h			\
	bonobo-moniker.h		\
	bonobo-moniker-client.h		\
	bonobo-object.h			\
	bonobo-object-client.h		\
	bonobo-object-directory.h	\
	bonobo-object-io.h		\
	bonobo-persist-file.h		\
	bonobo-persist-stream.h		\
	bonobo-persist.h		\
	bonobo-plug.h			\
	bonobo-progressive.h		\
	bonobo-sizeable.h		\
	bonobo-socket.h			\
	bonobo-storage.h		\
	bonobo-stream.h			\
	bonobo-stream-client.h		\
	bonobo-stream-fs.h		\
	bonobo-stream-memory.h		\
	bonobo-ui-handler.h		\
	bonobo-view.h			\
	bonobo-view-frame.h		\
	bonobo-wrapper.h		\
	bonobo-property.h		\
	bonobo-property-bag.h		\
	bonobo-property-bag-client.h	\
	bonobo-property-types.h

libbonobox_la_SOURCES =			\
	bonobo-ui-handler.c		\
	bonobo-uih-menu.c		\
	bonobo-uih-toolbar.c		\
	bonobo-uih-dock.c		\
	bonobo-uih-private.h		\
	bonobo-canvas-item.c		\
	bonobo-selector.c		\
	bonobo-widget.c			\
	bonobo-canvas-component.c	\
	bonobo-client-site.c		\
	bonobo-container.c		\
	bonobo-control.c		\
	bonobo-control-frame.c		\
	bonobo-desktop-window.c		\
	bonobo-embeddable.c		\
	bonobo-plug.c			\
	bonobo-sizeable.c		\
	bonobo-socket.c			\
	bonobo-view.c			\
	bonobo-view-frame.c		\
	bonobo-wrapper.c


libbonobo_print_la_SOURCES =		\
	bonobo-print.c			\
	bonobo-print-client.c

libbonobo_printincludedir = $(includedir)/bonobo

libbonobo_printinclude_HEADERS =		\
	Bonobo.h			\
	bonobo-print.h		\
	bonobo-print-client.h

BUILT_SOURCES=$(CORBA_SOURCE)

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = bonobo-insert-component.xpm 	\
	     bonobo-object-directory-goad.c 	\
	     bonobo-object-directory-oaf.c
