INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(BONOBOUI_CFLAGS)

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
	$(BONOBOUI_LIBS)				\
	$(INTLLIBS)
