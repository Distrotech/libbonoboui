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

libbonobo_la_SOURCES =		\
	$(CORBA_SOURCE)		\
	gtk-interfaces.c	\
	gnome-client-site.c	\
	gnome-component.c	\
	gnome-container.c	\
	gnome-main.c		\
	gnome-object.c		\
	gnome-view.c

bonobo_headers =		\
	gtk-interfaaces.h	\
	gnome-client-site.h	\
	gnome-component.h	\
	gnome-container.h	\
	gnome-main.h		\
	gnome-object.h		\
	gnome-view.h

test_container_SOURCES =	\
	test-container.c

test_container_LDADD = $(bonobo_libs)

object_test_SOURCES =		\
	object_test.c

object_test_LDADD = $(bonobo_libs)

sample_server_SOURCES =		\
	sample-server.c		\
	sample-skels.c		\
	sample-common.c

sample_server_LDADD = $(bonobo_libs)