noinst_PROGRAMS = bonobo-sample-container

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(BONOBOUI_CFLAGS)

bonobo_sample_container_SOURCES =			\
	bonobo-sample-container.c

bonobo_sample_container_LDADD =			\
	$(top_builddir)/bonobo/libbonoboui-2.la \
	$(BONOBOUI_LIBS)			\
	$(INTLLIBS)
