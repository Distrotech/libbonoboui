corbadir = $(sysconfdir)/CORBA/servers

corba_DATA = sample.gnorba

INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DSTORAGE_LIB=\""$(libdir)"\"			\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. 						\
	-I$(top_builddir)				\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)

gnome_libs =			\
	$(GNOME_LIBDIR)		\
        $(GNOMEUI_LIBS)		\
	$(GNOMEGNORBA_LIBS)	\
	$(INTLLIBS)

bonobo_libs = 			\
	libbonobo.la 		\
	$(gnome_libs)


lib_LTLIBRARIES = libbonobo.la
noinst_PROGRAMS = test-container sample-server \
			test-container-autoload selector_test test-storage 

CORBA_SOURCE =			\
	bonobo.h		\
	bonobo-common.c		\
	bonobo-stubs.c		\
	bonobo-skels.c

idls =						\
	$(top_srcdir)/idl/bonobo.idl		\
	$(top_srcdir)/idl/gnome-advise.idl	\
	$(top_srcdir)/idl/gnome-client-site.idl	\
	$(top_srcdir)/idl/gnome-embeddable.idl	\
	$(top_srcdir)/idl/gnome-container.idl	\
	$(top_srcdir)/idl/gnome-persist.idl	\
	$(top_srcdir)/idl/gnome-storage.idl	\
	$(top_srcdir)/idl/gnome-progressive.idl \
	$(top_srcdir)/idl/gnome-ui-handler.idl

idl_flags = `$(GNOME_CONFIG) --cflags idl` -D__BONOBO_COMPILATION

$(CORBA_SOURCE): $(idls)
	$(ORBIT_IDL) $(top_srcdir)/idl/bonobo.idl $(idl_flags)

libbonobo_la_SOURCES =			\
	$(CORBA_SOURCE)			\
	gnome-bonobo-selector.c		\
	gnome-client-site.c		\
	gnome-container.c		\
	gnome-embeddable-factory.c 	\
	gnome-embeddable.c		\
	gnome-main.c			\
	gnome-moniker.c			\
	gnome-moniker-client.c		\
	gnome-object.c			\
	gnome-object-client.c		\
	gnome-object-io.c		\
	gnome-persist-file.c		\
	gnome-persist-stream.c		\
	gnome-persist.c			\
	gnome-progressive.c		\
	gnome-storage.c			\
	gnome-stream.c			\
	gnome-stream-client.c		\
	gnome-stream-fs.c		\
	gnome-stream-memory.c		\
	gnome-ui-handler.c		\
	gnome-view.c			\
	gnome-view-frame.c		\
	gnome-wrapper.c


libbonoboincludedir = $(includedir)/bonobo

libbonoboinclude_HEADERS =		\
	bonobo.h			\
	gnome-bonobo.h			\
	gnome-bonobo-selector.h		\
	gnome-client-site.h		\
	gnome-component-io.h		\
	gnome-container.h		\
	gnome-embeddable.h		\
	gnome-embeddable-factory.h	\
	gnome-main.h			\
	gnome-moniker.h			\
	gnome-moniker-client.h		\
	gnome-object.h			\
	gnome-object-client.h		\
	gnome-object-io.h		\
	gnome-persist-file.h		\
	gnome-persist-stream.h		\
	gnome-persist.h			\
	gnome-progressive.h		\
	gnome-storage.h			\
	gnome-stream.h			\
	gnome-stream-client.h		\
	gnome-stream-fs.h		\
	gnome-stream-memory.h		\
	gnome-ui-handler.h		\
	gnome-view.h			\
	gnome-view-frame.h		\
	gnome-wrapper.h			\
	gnome-ui-handler.h

#test_ui_handler_SOURCES =		\
#	 test-ui-handler.c
#
#test_ui_handler_LDADD = $(bonobo_libs)

test_container_SOURCES =		\
	test-container.c

test_container_LDADD = $(bonobo_libs)

test_container_autoload_SOURCES =		\
	test-container-autoload.c

test_container_autoload_LDADD = $(bonobo_libs)

sample_server_SOURCES =		\
	sample-server.c

sample_server_LDADD = $(bonobo_libs)

test_storage_SOURCES = test-storage.c

test_storage_LDADD = $(bonobo_libs)

selector_test_SOURCES =		\
	selector_test.c

selector_test_LDADD = $(bonobo_libs)

BUILT_SOURCES=$(CORBA_SOURCE)

EXTRA_DIST = $(corba_DATA)

