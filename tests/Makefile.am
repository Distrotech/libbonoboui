noinst_PROGRAMS =		\
	test-ui			\
	test-toolbar		\
	selector_test		\
	test-focus

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(WARN_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(ORBIT_CFLAGS)					\
	$(OAF_CFLAGS)					\
	$(LIBGNOMEBASE_CFLAGS)

LDADD =	$(top_builddir)/bonobo/libbonoboui-2.la

test_ui_SOURCES = test-ui.c

test_focus_SOURCES = test-focus.c

test_toolbar_SOURCES = test-toolbar.c

selector_test_SOURCES =					\
	selector_test.c
