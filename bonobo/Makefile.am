INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)

gnome_libs =			\
	  $(GNOME_LIBDIR)	\
        $(GNOMEUI_LIBS)		\
	$(INTLLIBS)

lib_LTLIBRARIES = libbonobo.la
noinst_PROGRAMS = object_test

libbonobo_la_SOURCES =	\
	gnome-object.c

bonobo_headers =	\
	gnome-object.h


object_test_SOURCES =	\
	object_test.c

object_test_LDADD =	\
	$(gnome_libs)