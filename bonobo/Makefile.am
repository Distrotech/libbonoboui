corbadir = $(sysconfdir)/CORBA/servers

corba_DATA = sample.gnorba item.gnorba

if OAF
OBJECT_DIRECTORY_LIBS=$(OAF_LIBS)
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
        $(GNOME_INCLUDEDIR)

gnome_libs =			\
	$(GNOME_LIBDIR)		\
        $(GNOMEUI_LIBS)		\
	$(OBJECT_DIRECTORY_LIBS) \
	$(INTLLIBS)

bonobo_libs = 			\
	libbonobo.la 		\
	$(gnome_libs)


lib_LTLIBRARIES = libbonobo.la libbonobo-xml.la

if OAF
test_programs =
else
test_programs = \
	test-container 		\
	sample-server 		\
	sample-item 		\
	test-container-autoload	\
	selector_test		\
	test-storage		\
	test-bw
endif

noinst_PROGRAMS = 		\
	$(test_programs)

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
	$(top_srcdir)/idl/bonobo-storage.idl		\
	$(top_srcdir)/idl/bonobo-progressive.idl	\
	$(top_srcdir)/idl/bonobo-ui-handler.idl		\
	$(top_srcdir)/idl/bonobo-property.idl

idl_flags = `$(GNOME_CONFIG) --cflags idl` -D__BONOBO_COMPILATION

$(CORBA_SOURCE): $(idls)
	$(ORBIT_IDL) $(top_srcdir)/idl/Bonobo.idl -I$(top_srcdir)/idl $(idl_flags)

libbonobo_la_LDFLAGS = -version-info 1:0:0 #-rpath $(libdir)

libbonobo_la_SOURCES =			\
	$(CORBA_SOURCE)			\
	bonobo-object-directory.h	\
	bonobo-object-directory.c	\
	$(OBJECT_DIRECTORY_IMPL)	\
	bonobo-ui-handler.c		\
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
	bonobo-view.c			\
	bonobo-view-frame.c		\
	bonobo-wrapper.c		\
	bonobo-property.c		\
	bonobo-property-types.c		\
	bonobo-property-bag.c		\
	bonobo-property-bag-client.c

libbonoboincludedir = $(includedir)/bonobo

libbonobodir = $(includedir)
libbonobo_HEADERS = bonobo.h

libbonoboinclude_HEADERS =		\
	Bonobo.h			\
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
	bonobo-object-io.h		\
	bonobo-persist-file.h		\
	bonobo-persist-stream.h		\
	bonobo-persist.h		\
	bonobo-progressive.h		\
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
	bonobo-property-types.h		\
	bonobo-property-bag.h		\
	bonobo-property-bag-client.h

libbonobo_xml_la_SOURCES =		\
	bonobo-property-bag-xml.c

libbonobo_xmlincludedir = $(includedir)/bonobo

libbonobo_xmlinclude_HEADERS =		\
	Bonobo.h			\
	bonobo-property-bag.h		\
	bonobo-property-bag-xml.h

test_bw_SOURCES =			\
	test-bw.c

test_bw_LDADD =	$(bonobo_libs)

test_container_SOURCES =		\
	test-container.c

test_container_LDADD = $(bonobo_libs)

test_container_autoload_SOURCES =	\
	test-container-autoload.c

test_container_autoload_LDADD = $(bonobo_libs)

sample_server_SOURCES =			\
	sample-server.c

sample_server_LDADD = $(bonobo_libs)

sample_item_SOURCES = 			\
	sample-item.c

sample_item_LDADD = $(bonobo_libs)

test_storage_SOURCES = test-storage.c

test_storage_LDADD = $(bonobo_libs)

selector_test_SOURCES =			\
	selector_test.c

selector_test_LDADD = $(bonobo_libs)

BUILT_SOURCES=$(CORBA_SOURCE)

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = $(corba_DATA)
