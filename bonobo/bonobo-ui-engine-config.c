/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-ui-engine-config.c: The Bonobo UI/XML Sync engine user config code
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Helix Code, Inc.
 */

#include <stdlib.h>
#include <gtk/gtk.h>

#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-sync-menu.h>
#include <bonobo/bonobo-ui-engine-config.h>

typedef struct {
	BonoboUIEngine *engine;
	char           *path;
	char           *popup_xml;
} closure_t;

static closure_t *
closure_new (BonoboUIEngine *engine,
	     const char     *path,
	     const char     *popup_xml)
{
	closure_t *c = g_new0 (closure_t, 1);

	c->engine    = engine;
	c->path      = g_strdup (path);
	c->popup_xml = g_strdup (popup_xml);

	return c;
}

static void
closure_destroy (closure_t *c)
{
	g_free (c->path);
	g_free (c->popup_xml);
	g_free (c);
}

static void
toolbar_configure (closure_t *c)
{
	g_warning ("Throw up some glade");
}

static void
emit_verb_on_cb (BonoboUIEngine *engine,
		 BonoboUINode   *node,
		 closure_t      *c)
{
	char *verb;

	if ((verb = bonobo_ui_node_get_attr (node, "verb"))) {

		if (!strcmp (verb, "Configure"))
			toolbar_configure (c);
		else
			g_warning ("Unknown verb '%s'", verb);

		bonobo_ui_node_free_string (verb);
	}

	g_warning ("Verb on '%s'",
		   bonobo_ui_xml_make_path (node));
}

/*
 *  Hide any items that are flagged as needing it by
 * dep_attr="hidden:1" type nodes in the popup descr.
 */
static void
prep_inline_toggles (closure_t *c, BonoboUIEngine *view_engine)
{
	BonoboUINode *popup_node, *main_node, *l;

	main_node = bonobo_ui_engine_get_path (c->engine, c->path);
	if (!main_node) {
		g_warning ("No node to configure at '%s'", c->path);
		return;
	}

	popup_node = bonobo_ui_engine_get_path (view_engine, "/popups/popup");
	g_return_if_fail (popup_node != NULL);

	for (l = bonobo_ui_node_children (popup_node); l;
	     l = bonobo_ui_node_next (l)) {
		char *dep;

		if ((dep = bonobo_ui_node_get_attr (l, "dep_attr"))) {
			char *attr, **strs;
			gboolean hide = FALSE;

			strs = g_strsplit (dep, ":", -1);

			if (!strs || !strs [0] || !strs [1] || !strs [2] || strs [3]) {
				g_warning ("dep_attr format error '%s'", dep);
				return;
			}
				
			attr = bonobo_ui_node_get_attr (main_node, strs [0]);

			if (strs [1] [0] == 'b') { /* Boolean */
				if (attr)
					hide = atoi (attr) == atoi (strs [2]);
				else /* FIXME: This may need to know things about
				      * types and their defaults */
					hide = atoi (strs [2]);
			} else
				g_warning ("Unknown dep type '%s'", strs [1]);

			if (hide)
				bonobo_ui_node_set_attr (l, "hidden", "1");

			g_strfreev (strs);
			bonobo_ui_node_free_string (attr);
			bonobo_ui_node_free_string (dep);
		}
	}
}

static BonoboUIEngine *
create_popup_engine (closure_t *c,
		     GtkMenu   *menu)
{
	BonoboUIEngine *engine;
	BonoboUISync   *smenu;
	BonoboUINode   *node;

	engine = bonobo_ui_engine_new ();
	smenu  = bonobo_ui_sync_menu_new (engine, NULL, NULL, NULL);

	bonobo_ui_engine_add_sync (engine, smenu);

	node = bonobo_ui_node_from_string (c->popup_xml);

	bonobo_ui_util_translate_ui (node);

	bonobo_ui_engine_xml_merge_tree (
		engine, "/", node, "popup");

	prep_inline_toggles (c, engine);

	bonobo_ui_sync_menu_add_popup (
		BONOBO_UI_SYNC_MENU (smenu),
		menu, "/popups/popup");

	gtk_signal_connect (GTK_OBJECT (engine),
			    "emit_verb_on",
			    (GtkSignalFunc) emit_verb_on_cb, c);

	bonobo_ui_engine_update (engine);

	return engine;
}

static int
config_button_pressed (GtkWidget      *widget,
		       GdkEventButton *event,
		       closure_t      *c)
{
	if (event->button == 3) {
		GtkWidget *menu;

		menu = gtk_menu_new ();

		create_popup_engine (c, GTK_MENU (menu));

		gtk_widget_show (GTK_WIDGET (menu));

		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
				NULL, NULL, 3, 0);

		return TRUE;
	} else
		return FALSE;
}

void
bonobo_ui_engine_config_connect (GtkWidget      *widget,
				 BonoboUIEngine *engine,
				 const char     *path,
				 const char     *popup_xml)
{
	gtk_signal_connect_full (
		GTK_OBJECT (widget), "button_press_event",
		(GtkSignalFunc) config_button_pressed, NULL, 
		closure_new (engine, path, popup_xml),
		(GtkDestroyNotify) closure_destroy,
		FALSE, FALSE);
}
