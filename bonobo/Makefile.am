PLUGIN_DIR=$(libdir)/bonobo/plugin

INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
	-DG_LOG_DOMAIN=\"Bonobo\"			\
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
        -I$(includedir)                                 \
	$(GNOME_INCLUDEDIR)				\
	$(OAF_CFLAGS)					\
	$(GDK_PIXBUF_CFLAGS)

lib_LTLIBRARIES = libbonobo.la libbonobox.la libbonobo-print.la

CORBA_SOURCE =			\
	Bonobo.h		\
	Bonobo-common.c		\
	Bonobo-stubs.c		\
	Bonobo-skels.c

idls =							\
	$(top_srcdir)/idl/Bonobo.idl			\
	$(top_srcdir)/idl/bonobo-advise.idl		\
	$(top_srcdir)/idl/bonobo-listener.idl		\
	$(top_srcdir)/idl/bonobo-ui.idl			\
	$(top_srcdir)/idl/bonobo-zoomable.idl		\
	$(top_srcdir)/idl/bonobo-canvas.idl 		\
	$(top_srcdir)/idl/bonobo-item-container.idl	\
	$(top_srcdir)/idl/bonobo-control.idl		\
	$(top_srcdir)/idl/bonobo-client-site.idl	\
	$(top_srcdir)/idl/bonobo-embeddable.idl		\
	$(top_srcdir)/idl/bonobo-moniker.idl		\
	$(top_srcdir)/idl/bonobo-persist.idl		\
	$(top_srcdir)/idl/bonobo-progressive.idl	\
	$(top_srcdir)/idl/bonobo-property.idl		\
	$(top_srcdir)/idl/bonobo-storage.idl

idl_flags = `$(GNOME_CONFIG) --cflags idl` -D__BONOBO_COMPILATION

$(CORBA_SOURCE): $(idls)
	$(ORBIT_IDL) $(top_srcdir)/idl/Bonobo.idl -I$(top_srcdir)/idl $(idl_flags)

Bonobo.h: $(idls)

libbonobo_la_LDFLAGS = -version-info 2:0:1 $(gnome_libs)

libbonobo_la_SOURCES =			\
	$(CORBA_SOURCE)			\
	bonobo-arg.h			\
	bonobo-arg.c			\
	bonobo-exception.c		\
	bonobo-object-directory.c	\
	bonobo-generic-factory.c	\
	bonobo-main.c			\
	bonobo-moniker.c		\
	bonobo-moniker-util.c		\
	bonobo-object.c			\
	bonobo-object-client.c		\
	bonobo-object-io.c		\
	bonobo-persist-file.c		\
	bonobo-persist-stream.c		\
	bonobo-persist.c		\
	bonobo-progressive.c		\
	bonobo-property-listener.c	\
	bonobo-storage-plugin.c		\
	bonobo-storage.c		\
	bonobo-stream.c			\
	bonobo-stream-client.c		\
	bonobo-stream-memory.c		\
	bonobo-transient.c		\
	bonobo-property.c		\
	bonobo-property-bag.c		\
	bonobo-property-bag-client.c

libbonoboincludedir = $(includedir)/bonobo

libbonobodir = $(includedir)
libbonobo_HEADERS = bonobo.h

libbonoboinclude_HEADERS =			\
	Bonobo.h				\
	bonobo-arg.h				\
	bonobo-canvas-component.h		\
	bonobo-canvas-item.h			\
	bonobo-client-site.h			\
	bonobo-component-io.h			\
	bonobo-control-frame.h			\
	bonobo-control.h			\
	bonobo-desktop-window.h			\
	bonobo-embeddable-factory.h		\
	bonobo-embeddable.h			\
	bonobo-exception.h			\
	bonobo-generic-factory.h		\
	bonobo-item-container.h			\
	bonobo-main.h				\
	bonobo-moniker-util.h			\
	bonobo-moniker.h			\
	bonobo-object-client.h			\
	bonobo-object-directory.h		\
	bonobo-object-io.h			\
	bonobo-object.h				\
	bonobo-persist-file.h			\
	bonobo-persist-stream.h			\
	bonobo-persist.h			\
	bonobo-plug.h				\
	bonobo-progressive.h			\
	bonobo-property-bag-client.h		\
	bonobo-property-bag.h			\
	bonobo-property-listener.h		\
	bonobo-property-types.h			\
	bonobo-property.h			\
	bonobo-selector.h			\
	bonobo-socket.h				\
	bonobo-storage-plugin.h			\
	bonobo-storage.h			\
	bonobo-stream-client.h			\
	bonobo-stream-memory.h			\
	bonobo-stream.h				\
	bonobo-transient.h			\
	bonobo-ui-component.h            	\
	bonobo-ui-container.h            	\
	bonobo-ui-node.h                 	\
	bonobo-ui-toolbar-button-item.h  	\
	bonobo-ui-toolbar-icon.h         	\
	bonobo-ui-toolbar-item.h		\
	bonobo-ui-toolbar-toggle-button-item.h	\
	bonobo-ui-util.h			\
	bonobo-ui-xml.h				\
	bonobo-view-frame.h			\
	bonobo-view.h				\
	bonobo-widget.h 			\
	bonobo-win.h				\
	bonobo-wrapper.h			\
	bonobo-zoomable.h			\
	bonobo-zoomable-frame.h

noinst_HEADERS =				\
	bonobo-ui-toolbar.h			\
	bonobo-ui-toolbar-control-item.h  	\
	bonobo-ui-toolbar-popup-item.h		\
	bonobo-ui-toolbar-separator-item.h

libbonobox_la_SOURCES =				\
	bonobo-canvas-component.c		\
	bonobo-canvas-item.c			\
	bonobo-client-site.c			\
	bonobo-control-frame.c			\
	bonobo-control.c			\
	bonobo-desktop-window.c			\
	bonobo-embeddable.c			\
	bonobo-item-container.c			\
	bonobo-plug.c				\
	bonobo-selector.c			\
	bonobo-socket.c				\
	bonobo-ui-component.c            	\
	bonobo-ui-container.c            	\
	bonobo-ui-node.c                 	\
	bonobo-ui-toolbar-button-item.c  	\
	bonobo-ui-toolbar-control-item.c  	\
	bonobo-ui-toolbar-icon.c         	\
	bonobo-ui-toolbar-item.c		\
	bonobo-ui-toolbar-popup-item.c		\
	bonobo-ui-toolbar-separator-item.c	\
	bonobo-ui-toolbar-toggle-button-item.c	\
	bonobo-ui-toolbar.c			\
	bonobo-ui-util.c			\
	bonobo-ui-xml.c				\
	bonobo-ui-xml.h				\
	bonobo-view-frame.c			\
	bonobo-view.c				\
	bonobo-widget.c				\
	bonobo-win.c				\
	bonobo-wrapper.c			\
	bonobo-zoomable.c			\
	bonobo-zoomable-frame.c

libbonobo_print_la_SOURCES =		\
	bonobo-print.c			\
	bonobo-print-client.c

libbonobo_printincludedir = $(includedir)/bonobo

libbonobo_printinclude_HEADERS =		\
	Bonobo.h			\
	bonobo-print.h		\
	bonobo-print-client.h

noinst_PROGRAMS = test-app test-toolbar

bin_PROGRAMS = bonobo-ui-extract

test_app_SOURCES = app.c
test_app_LDADD =					\
	libbonobo.la					\
	libbonobox.la					\
	libbonobo-print.la				\
	$(BONOBOX_TEST_LIBS)

test_toolbar_SOURCES = test-toolbar.c
test_toolbar_LDADD =					\
	libbonobo.la					\
	libbonobox.la					\
	libbonobo-print.la				\
	$(BONOBOX_TEST_LIBS)

bonobo_ui_extract_SOURCES = extract.c
bonobo_ui_extract_LDADD =				\
	$(GNOME_LIBS)					\
	$(XML_LIBS)

BUILT_SOURCES=$(CORBA_SOURCE)

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = bonobo-insert-component.xpm
