INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)

gnome_libs =			\
	$(GNOME_LIBDIR)		\
        $(GNOMEUI_LIBS)		\
	$(GNOMEGNORBA_LIBS)	\
	$(INTLLIBS)

lib_LTLIBRARIES = libbonobo.la
noinst_PROGRAMS = object_test

libbonobo_la_SOURCES =		\
	gtk-interfaces.c	\
	gnome-object.c		\
	GnomeObject-common.c	\
	GnomeObject-skels.c

bonobo_headers =		\
	gtk-interfaaces.h	\
	gnome-object.h


object_test_SOURCES =	\
	object_test.c

object_test_LDADD =	\
	$(gnome_libs) libbonobo.la 


