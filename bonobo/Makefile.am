corbadir = $(sysconfdir)/CORBA/servers

corba_DATA = sample.gnorba

INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
        -I$(includedir)                                 \
	-I$(..)						\
        $(GNOME_INCLUDEDIR)

gnome_libs =			\
	$(GNOME_LIBDIR)		\
        $(GNOMEUI_LIBS)		\
	$(GNOMEGNORBA_LIBS)	\
	$(INTLLIBS)

bonobo_libs = 			\
	$(gnome_libs)		\
	libbonobo.la 

lib_LTLIBRARIES = libbonobo.la
noinst_PROGRAMS = object_test test-container sample-server

CORBA_SOURCE =			\
	bonobo.h		\
	bonobo-common.c		\
	bonobo-stubs.c		\
	bonobo-skels.c

idls =						\
	$(top_srcdir)/idl/bonobo.idl		\
	$(top_srcdir)/idl/advise.idl		\
	$(top_srcdir)/idl/base.idl		\
	$(top_srcdir)/idl/bobject.idl		\
	$(top_srcdir)/idl/clientsite.idl	\
	$(top_srcdir)/idl/container.idl		\
	$(top_srcdir)/idl/inplace.idl		\
	$(top_srcdir)/idl/moniker.idl		\
	$(top_srcdir)/idl/persist.idl		\
	$(top_srcdir)/idl/storage.idl

idl_flags = `gnome-config --cflags idl`

$(CORBA_SOURCE): $(idls)
	orbit-idl $(top_srcdir)/idl/bonobo.idl $(idl_flags)

libbonobo_la_SOURCES =			\
	$(CORBA_SOURCE)			\
	gtk-interfaces.c		\
	gnome-client-site.c		\
	gnome-component.c		\
	gnome-component-factory.c 	\
	gnome-container.c		\
	gnome-inplace-component.c	\
	gnome-inplace-site.c		\
	gnome-main.c			\
	gnome-object.c			\
	gnome-view.c


libbonoboincludedir = $(includedir)/bonobo

libbonoboinclude_HEADERS =		\
	gtk-interfaces.h		\
	gnome-client-site.h		\
	gnome-inplace-site.h		\
	gnome-inplace-component.h	\
	gnome-component.h		\
	gnome-component-factory.h	\
	gnome-container.h		\
	gnome-main.h			\
	gnome-object.h			\
	gnome-view.h

test_container_SOURCES =		\
	test-container.c

test_container_LDADD = $(bonobo_libs)

object_test_SOURCES =		\
	object_test.c

object_test_LDADD = $(bonobo_libs)

sample_server_SOURCES =		\
	sample-server.c

sample_server_LDADD = $(bonobo_libs)
