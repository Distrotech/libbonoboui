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
test_ui_node (void)
{
	GQuark baa_id;
	BonoboUINode *node;

	fprintf (stderr, "testing BonoboUINode ...\n");

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
test_ui_engine (BonoboUIEngine *engine, CORBA_Environment *ev)
{
	BonoboUINode *node;

	fprintf (stderr, "testing the UI engine ...\n");

	node = bonobo_ui_node_from_string (
		"<testnode name=\"Foo\" prop=\"A\"/>");

	bonobo_ui_engine_xml_merge_tree (engine, "/", node, "A");

	bonobo_ui_engine_xml_set_prop (engine, "/Foo", "prop", "B", "B");

	check_prop (engine, "/Foo", "prop", "B");

	bonobo_ui_engine_xml_rm (engine, "/", "B");

	check_prop (engine, "/Foo", "prop", "A");

	g_assert (bonobo_ui_engine_node_is_dirty (
		engine, bonobo_ui_engine_get_path (engine, "/Foo")));
}

int
main (int argc, char **argv)
{
	BonoboUIEngine *engine;
	CORBA_Environment *ev, real_ev;

	ev = &real_ev;
	CORBA_exception_init (ev);

	free (malloc (8)); /* -lefence */

	bonobo_ui_gconf_leaks_refs ();

	if (!bonobo_ui_init_full ("test-ui-auto", VERSION,
				  &argc, argv,
				  CORBA_OBJECT_NIL,
				  CORBA_OBJECT_NIL,
				  CORBA_OBJECT_NIL,
				  FALSE)) /* avoid gconf init */
		g_error (_("Cannot init libbonoboui code"));

	bonobo_activate ();

	engine = bonobo_ui_engine_new (NULL);

	test_ui_node ();
	test_ui_engine (engine, ev);

	g_object_unref (G_OBJECT (engine));

	CORBA_exception_free (ev);

	fprintf (stderr, "All tests passed successfully\n");

	return bonobo_ui_debug_shutdown ();
}

