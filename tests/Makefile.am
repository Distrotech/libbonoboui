noinst_PROGRAMS =		\
	test-any		\
	test-properties-server	\
	test-properties-client	\
	test-storage		\
	test-container 		\
	test-container-autoload	\
	selector_test

bonobo_libs =						\
	$(top_builddir)/bonobo/libbonobo.la		\
	$(BONOBOX_TEST_LIBS)				\
	$(INTLLIBS)

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(BONOBOX_TEST_CFLAGS)

test_properties_server_SOURCES =			\
	test-properties-server.c

test_properties_server_LDADD =				\
	$(bonobo_libs)

test_properties_client_SOURCES =			\
	test-properties-client.c

test_properties_client_LDADD =				\
	$(bonobo_libs)

test_storage_SOURCES = test-storage.c

test_storage_LDADD = $(bonobo_libs)

test_any_SOURCES = test-any.c

test_any_LDADD =					\
	$(bonobo_libs)					\
	$(top_builddir)/bonobo/libbonobox.la

test_container_SOURCES =				\
	test-container.c

test_container_LDADD =					\
	$(bonobo_libs)					\
	$(top_builddir)/bonobo/libbonobox.la

test_container_autoload_SOURCES =			\
	test-container-autoload.c

test_container_autoload_LDADD =				\
	$(bonobo_libs)					\
	$(top_builddir)/bonobo/libbonobox.la

selector_test_SOURCES =					\
	selector_test.c

selector_test_LDADD =					\
	$(bonobo_libs)					\
	$(top_builddir)/bonobo/libbonobox.la


TESTS_ENVIRONMENT =					\
	LD_LIBRARY_PATH=$(top_builddir)/bonobo/.libs/	\
	BONOBO_PLUGIN_PATH=$(top_builddir)/storage-modules/.libs

TESTS = test-storage
