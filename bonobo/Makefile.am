INCLUDES = \
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-I$(builddir)					\
        -I$(includedir)                                 \
	$(WARN_CFLAGS)					\
	$(LIBGNOME_CFLAGS)				\
	$(LIBBONOBO_CFLAGS)				\
	$(LIBGNOMECANVAS_CFLAGS)			\
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
	-DBONOBO_UIDIR=\""$(datadir)/gnome/ui/"\"	\
        -DBONOBO_LIBDIR=\""$(libdir)"\" 		\
        -DBONOBO_DATADIR=\""$(datadir)"\" 		\
        -DBONOBO_PIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DBONOBO_BINDIR=\""$(bindir)"\" 		\
        -DBONOBO_LOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DBONOBO_LOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"Bonobo\"			\
	-DVERSION=\"$(VERSION)\"

lib_LTLIBRARIES = libbonoboui-2.la

noinst_HEADERS = \
	bonobo-ui-engine-config.h		\
	bonobo-ui-engine-private.h		\
	bonobo-ui-sync-menu.h			\
	bonobo-ui-sync-keys.h			\
	bonobo-ui-sync-status.h			\
	bonobo-ui-sync-toolbar.h		\
	bonobo-ui-toolbar.h			\
	bonobo-ui-toolbar-control-item.h  	\
	bonobo-ui-toolbar-popup-item.h		\
	bonobo-ui-toolbar-separator-item.h	\
	bonobo-ui-xml.h

libbonoboui_2_la_LIBADD = \
	$(LIBGNOMECANVAS_LIBS) $(LIBBONOBO_LIBS) \
	$(LIBGNOME_LIBS)

libbonoboui_2_la_SOURCES = \
	bonobo-canvas-component.c		\
	bonobo-canvas-item.c			\
	bonobo-client-site.c			\
	bonobo-control-frame.c			\
	bonobo-control.c			\
	bonobo-desktop-window.c			\
	bonobo-embeddable.c			\
	bonobo-plug.c				\
	bonobo-selector.c			\
	bonobo-selector-widget.c		\
	bonobo-socket.c				\
	bonobo-ui-component.c            	\
	bonobo-ui-config-widget.c		\
	bonobo-ui-container.c            	\
	bonobo-ui-engine.c			\
	bonobo-ui-engine-config.c		\
	bonobo-ui-node.c                 	\
	bonobo-ui-sync.c			\
	bonobo-ui-sync-menu.c			\
	bonobo-ui-sync-keys.c			\
	bonobo-ui-sync-status.c			\
	bonobo-ui-sync-toolbar.c		\
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
	bonobo-view-frame.c			\
	bonobo-view.c				\
	bonobo-widget.c				\
	bonobo-win.c				\
	bonobo-wrapper.c			\
	bonobo-zoomable.c			\
	bonobo-zoomable-frame.c

EXTRA_DIST = bonobo-insert-component.xpm libbonobo-2.0.pc.in
