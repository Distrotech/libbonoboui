corbadir = $(sysconfdir)/CORBA/servers

corba_DATA = sample.gnorba

INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. 						\
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
noinst_PROGRAMS = object_test test-container sample-server \
			test-container-autoload

CORBA_SOURCE =			\
	bonobo.h		\
	bonobo-common.c		\
	bonobo-stubs.c		\
	bonobo-skels.c

idls =						\
	$(top_srcdir)/idl/bonobo.idl		\
	$(top_srcdir)/idl/gnome-advise.idl	\
	$(top_srcdir)/idl/gnome-client-site.idl	\
	$(top_srcdir)/idl/gnome-component.idl	\
	$(top_srcdir)/idl/gnome-container.idl	\
	$(top_srcdir)/idl/gnome-inplace.idl	\
	$(top_srcdir)/idl/gnome-moniker.idl	\
	$(top_srcdir)/idl/gnome-persist.idl	\
	$(top_srcdir)/idl/gnome-storage.idl	\
	$(top_srcdir)/idl/gnome-progressive.idl

idl_flags = `$(GNOME_CONFIG) --cflags idl`

$(CORBA_SOURCE): $(idls)
	$(ORBIT_IDL) $(top_srcdir)/idl/bonobo.idl $(idl_flags)

libbonobo_la_SOURCES =			\
	$(CORBA_SOURCE)			\
	gtk-interfaces.c		\
	gnome-bind-context.c		\
	gnome-client-site.c		\
	gnome-component.c		\
	gnome-component-client.c	\
	gnome-component-factory.c 	\
	gnome-container.c		\
	gnome-file-moniker.c		\
	gnome-inplace-component.c	\
	gnome-inplace-site.c		\
	gnome-main.c			\
	gnome-moniker.c			\
	gnome-object.c			\
	gnome-object-client.c		\
	gnome-persist-stream.c		\
	gnome-persist.c			\
	gnome-progressive.c		\
	gnome-storage.c			\
	gnome-storage-fs.c		\
	gnome-stream.c			\
	gnome-stream-fs.c		\
	gnome-stream-memory.c		\
	gnome-view.c			\
	gnome-wrapper.c



libbonoboincludedir = $(includedir)/bonobo

libbonoboinclude_HEADERS =		\
	bonobo.h			\
	gtk-interfaces.h		\
	gnome-bind-context.h		\
	gnome-bonobo.h			\
	gnome-client-site.h		\
	gnome-component.h		\
	gnome-component-client.h	\
	gnome-component-factory.h	\
	gnome-component-io.h		\
	gnome-composite-moniker.h	\
	gnome-container.h		\
	gnome-file-moniker.h		\
	gnome-inplace-site.h		\
	gnome-inplace-component.h	\
	gnome-item-moniker.h		\
	gnome-main.h			\
	gnome-moniker.h			\
	gnome-object.h			\
	gnome-object-client.h		\
	gnome-persist-stream.h		\
	gnome-persist.h			\
	gnome-progressive.h		\
	gnome-storage.h			\
	gnome-storage-fs.h		\
	gnome-stream.h			\
	gnome-stream-fs.h		\
	gnome-stream-memory.h		\
	gnome-view.h			\
	gnome-wrapper.h

test_container_SOURCES =		\
	test-container.c

test_container_LDADD = $(bonobo_libs)

test_container_autoload_SOURCES =		\
	test-container-autoload.c

test_container_autoload_LDADD = $(bonobo_libs)

object_test_SOURCES =		\
	object_test.c

object_test_LDADD = gtk-interfaces.o $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

sample_server_SOURCES =		\
	sample-server.c

sample_server_LDADD = $(bonobo_libs)

BUILT_SOURCES=$(CORBA_SOURCE)

EXTRA_DIST = $(corba_DATA)

