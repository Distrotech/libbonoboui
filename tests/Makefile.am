noinst_PROGRAMS =		\
	test-ui			\
	test-toolbar		\
	selector_test		\
	test-focus		\
	test-sensitivity

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	-DBONOBO_UIDIR=\""$(datadir)/gnome/ui/"\"	\
	-DG_LOG_DOMAIN=\"Bonobo\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(WARN_CFLAGS)					\
	$(LIBBONOBO_CFLAGS)				\
	$(LIBGNOMECANVAS_CFLAGS)

LDADD =	$(top_builddir)/bonobo/libbonoboui-2.la

test_ui_SOURCES = test-ui.c

test_focus_SOURCES = test-focus.c

test_toolbar_SOURCES = test-toolbar.c

selector_test_SOURCES =	selector_test.c

test_sensitivity_SOURCES = test-sensitivity.c
