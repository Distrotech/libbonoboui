bin_PROGRAMS = sample-control-factory sample-control-container

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(BONOBOX_TEST_CFLAGS)

sample_control_factory_SOURCES =			\
	bonobo-clock-control.c				\
	bonobo-clock-control.h				\
	bonobo-calculator-control.c			\
	bonobo-calculator-control.h			\
	sample-control-factory.c

sample_control_factory_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(top_builddir)/bonobo/libbonobox.la		\
	$(BONOBOX_TEST_LIBS)				\
	$(INTLLIBS)

sample_control_container_SOURCES =			\
	sample-control-container.c

sample_control_container_LDADD =			\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(top_builddir)/bonobo/libbonobox.la		\
	$(BONOBOX_TEST_LIBS)				\
	$(INTLLIBS)

oafdir = $(datadir)/oaf

OAF_FILES =	bonobo-clock-control.oafinfo		\
		bonobo-calculator-control.oafinfo

oaf_DATA = $(OAF_FILES)

EXTRA_DIST = $(OAF_FILES)



