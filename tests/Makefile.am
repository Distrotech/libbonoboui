bin_PROGRAMS=test-properties-server test-properties-client

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)				\
	$(GTK_CFLAGS)

test_properties_server_SOURCES =			\
	test-properties-server.c

test_properties_server_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(GNOMEGNORBA_LIBS)				\
	$(INTLLIBS)


test_properties_client_SOURCES =			\
	test-properties-client.c

test_properties_client_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(GTK_LIBS)					\
	$(GNOME_LIBDIR)					\
	$(GNOMEGNORBA_LIBS)				\
	$(INTLLIBS)
