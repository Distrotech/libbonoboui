INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(LIBGNOME_CFLAGS)				\
	$(LIBGNOMECANVAS_CFLAGS)			\
	$(LIBBONOBO_CFLAGS)

bin_PROGRAMS = sample-container

sample_container_SOURCES =	\
	container.c		\
	container.h		\
	component.c		\
	component.h		\
	container-filesel.h	\
	container-filesel.c	\
	container-menu.h	\
	container-menu.c	\
	document.c		\
	document.h		\
	doc-view.c		\
	doc-view.h

sample_container_LDADD =				\
	$(top_builddir)/bonobo/libbonoboui-2.la		\
	$(LIBGNOME_LIBS)				\
	$(LIBGNOMECANVAS_LIBS)				\
	$(LIBBONOBO_LIBS)				\
	$(INTLLIBS)
