/*
 * test-ui-auto.c: An automatic Bonobo UI api regression test
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2002 Ximian, Inc.
 */
#include <config.h>
#include <string.h>

#include <libbonoboui.h>
#include <bonobo/bonobo-ui-private.h>
#include <bonobo/bonobo-ui-node-private.h>

static void
test_ui_node_attrs (void)
{
	GQuark baa_id;
	BonoboUINode *node;

	fprintf (stderr, "  attrs ...\n");

	node = bonobo_ui_node_new ("foo");
	g_assert (node != NULL);
	g_assert (bonobo_ui_node_has_name (node, "foo"));

	baa_id = g_quark_from_static_string ("baa");
	g_assert ( bonobo_ui_node_try_set_attr (node, baa_id, "baz"));
	g_assert (!bonobo_ui_node_try_set_attr (node, baa_id, "baz"));

	g_assert (!strcmp (bonobo_ui_node_get_attr_by_id (node, baa_id), "baz"));

	bonobo_ui_node_set_attr (node, "A", "A");
	bonobo_ui_node_set_attr (node, "A", "B");
	g_assert (!strcmp (bonobo_ui_node_peek_attr (node, "A"), "B"));

	bonobo_ui_node_free (node);
}

static void
test_ui_node_inserts (void)
{
	BonoboUINode *parent, *a, *b;

	fprintf (stderr, "  inserts ...\n");

	parent = bonobo_ui_node_new ("parent");

	a = bonobo_ui_node_new_child (parent, "a");
	g_assert (a->prev == NULL);
	g_assert (a->next == NULL);
	g_assert (a->parent == parent);

	b = bonobo_ui_node_new ("b");
	g_assert (b->prev == NULL);
	g_assert (b->next == NULL);

	bonobo_ui_node_insert_before (a, b);

	g_assert (b->prev == NULL);
	g_assert (b->next == a);
	g_assert (b->parent == parent);
	g_assert (a->prev == b);
	g_assert (a->next == NULL);
	g_assert (a->parent == parent);

	bonobo_ui_node_free (parent);
}

static void
test_ui_node (void)
{
	fprintf (stderr, "testing BonoboUINode ...\n");

	test_ui_node_attrs ();
	test_ui_node_inserts ();
}

static void
check_prop (BonoboUIEngine *engine,
	    const char     *path,
	    const char     *value,
	    const char     *intended)
{
	CORBA_char *str;

	str = bonobo_ui_engine_xml_get_prop (engine, path, value, NULL);

	if (intended) {
		g_assert (str != NULL);
		g_assert (!strcmp (str, intended));
		CORBA_free (str);
	} else
		g_assert (str == NULL);
}

static void
test_engine_misc (CORBA_Environment *ev)
{
	BonoboUINode *node;
	BonoboUIEngine *engine;

	fprintf (stderr, "   misc ...\n");

	engine = bonobo_ui_engine_new (NULL);

	node = bonobo_ui_node_from_string (
		"<testnode name=\"Foo\" prop=\"A\"/>");

	bonobo_ui_engine_xml_merge_tree (engine, "/", node, "A");

	bonobo_ui_engine_xml_set_prop (engine, "/Foo", "prop", "B", "B");

	check_prop (engine, "/Foo", "prop", "B");

	bonobo_ui_engine_xml_rm (engine, "/", "B");

	check_prop (engine, "/Foo", "prop", "A");

	g_assert (bonobo_ui_engine_node_is_dirty (
		engine, bonobo_ui_engine_get_path (engine, "/Foo")));

	g_object_unref (G_OBJECT (engine));
}

static void
test_engine_default_placeholder (CORBA_Environment *ev)
{
	BonoboUIEngine *engine;
	CORBA_char *str;
	BonoboUINode *node;

	fprintf (stderr, "  default placeholders ...\n");

	engine = bonobo_ui_engine_new (NULL);

	node = bonobo_ui_node_from_string (
		"<Root>"
		"  <nodea name=\"fooa\" attr=\"baa\"/>"
		"  <placeholder/>"
		"  <nodec name=\"fooc\" attr=\"baa\"/>"
		"</Root>");

	bonobo_ui_engine_xml_merge_tree (engine, "/", node, "A");

	node = bonobo_ui_node_from_string ("<nodeb name=\"foob\" attr=\"baa\"/>");
	bonobo_ui_engine_xml_merge_tree (engine, "/", node, "A");

	str = bonobo_ui_engine_xml_get (engine, "/", FALSE);
/*	g_warning ("foo '%s'", str); */
	CORBA_free (str);

	node = bonobo_ui_engine_get_path (engine, "/fooa");
	g_assert (node != NULL);
	g_assert (node->name_id == g_quark_from_string ("nodea"));
	g_assert (node->next != NULL);
	node = node->next;
	g_assert (node->name_id == g_quark_from_string ("nodeb"));
	g_assert (node->next != NULL);
	node = node->next;
	g_assert (node->name_id == g_quark_from_string ("placeholder"));
	g_assert (node->next != NULL);
	node = node->next;
	g_assert (node->name_id == g_quark_from_string ("nodec"));
	g_assert (node->next == NULL);
	
	g_object_unref (engine);
}

static void
test_ui_engine (CORBA_Environment *ev)
{
	fprintf (stderr, "testing BonoboUIEngine ...\n");

	test_engine_misc (ev);
	test_engine_default_placeholder (ev);
}

static void
test_ui_performance (CORBA_Environment *ev)
{
	int i;
	GTimer *timer;
	BonoboUINode *node;
	BonoboUIEngine *engine;

	fprintf (stderr, "performance tests ...\n");

	timer = g_timer_new ();
	g_timer_start (timer);

	engine = bonobo_ui_engine_new (NULL);

        node = bonobo_ui_node_from_file ("../doc/std-ui.xml");
	if (!node)
		g_error ("Can't find std-ui.xml");

	bonobo_ui_engine_xml_merge_tree (engine, "/", node, "A");

	g_timer_reset (timer);
	for (i = 0; i < 10000; i++)
		bonobo_ui_engine_xml_set_prop (
			engine, "/menu/File/FileOpen",
			"hidden", (i / 3) % 2 ? "1" : "0", "A");

	fprintf (stderr, "  set prop item: %g(ns)\n",
		 g_timer_elapsed (timer, NULL) * 100);


	g_timer_reset (timer);
	for (i = 0; i < 10000; i++)
		bonobo_ui_engine_xml_set_prop (
			engine, "/menu/File/FileOpen",
			"hidden", (i / 3) % 2 ? "1" : "0", (i/6) % 2 ? "A" : "B");
	fprintf (stderr, "  set prop cmd override: %g(ns)\n",
		 g_timer_elapsed (timer, NULL) * 100);


	g_object_unref (engine);
}

int
main (int argc, char **argv)
{
	CORBA_Environment *ev, real_ev;

	ev = &real_ev;
	CORBA_exception_init (ev);

	free (malloc (8)); /* -lefence */

	if (!bonobo_ui_init_full ("test-ui-auto", VERSION,
				  &argc, argv,
				  CORBA_OBJECT_NIL,
				  CORBA_OBJECT_NIL,
				  CORBA_OBJECT_NIL,
				  FALSE)) /* avoid gconf init */
		g_error (_("Cannot init libbonoboui code"));

	bonobo_activate ();

	test_ui_node ();
	test_ui_engine (ev);
	test_ui_performance (ev);

	CORBA_exception_free (ev);

	fprintf (stderr, "All tests passed successfully\n");

	return bonobo_ui_debug_shutdown ();
}
