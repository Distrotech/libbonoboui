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

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-config.h>

#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-sync-menu.h>
#include <bonobo/bonobo-ui-engine-config.h>
#include <bonobo/bonobo-ui-engine-private.h>

#define PARENT_TYPE gtk_object_get_type ()

static GtkObjectClass *parent_class = NULL;

struct _BonoboUIEngineConfigPrivate {
	char           *path; 

	BonoboUIEngine *engine;
	BonoboUIXml    *tree;

	GSList         *clobbers;
};

typedef struct {
	char *path;
	char *attr;
	char *value;
} clobber_t;

static void
clobber_destroy (BonoboUIXml *tree, clobber_t *cl)
{
	if (cl) {
		bonobo_ui_xml_remove_watch_by_data (tree, cl);

		g_free (cl->path);
		cl->path= NULL;

		g_free (cl->attr);
		cl->attr= NULL;

		g_free (cl->value);
		cl->value = NULL;

		g_free (cl);
	}
}

static void
clobbers_free (BonoboUIEngineConfig *config)
{
	GSList *l;

	for (l = config->priv->clobbers; l; l = l->next)
		clobber_destroy (config->priv->tree, l->data);

	g_slist_free (config->priv->clobbers);
	config->priv->clobbers = NULL;
}

void
bonobo_ui_engine_config_serialize (BonoboUIEngineConfig *config)
{
	GPtrArray *array;
	GSList    *l;
	int        i;

	g_return_if_fail (config->priv->path != NULL);

	array = g_ptr_array_new ();

	for (l = config->priv->clobbers; l; l = l->next) {
		clobber_t *cl = l->data;

		g_ptr_array_add (
			array,
			g_strconcat (cl->path, ":",
				     cl->attr, ":",
				     cl->value, NULL));
	}
	
	gnome_config_set_vector (config->priv->path,
				 array->len, (const char * const *) array->pdata);

	for (i = 0; i < array->len; i++)
		g_free (g_ptr_array_index (array, i));

	g_ptr_array_free (array, TRUE);

	gnome_config_sync ();
}

static void
clobber_add (BonoboUIEngineConfig *config,
	     const char           *path,
	     const char           *attr,
	     const char           *value)
{
	clobber_t *cl = g_new0 (clobber_t, 1);

	cl->path  = g_strdup (path);
	cl->attr  = g_strdup (attr);
	cl->value = g_strdup (value);

	config->priv->clobbers = g_slist_prepend (
		config->priv->clobbers, cl);

	bonobo_ui_xml_add_watch (config->priv->tree, path, cl);
}

void
bonobo_ui_engine_config_add (BonoboUIEngineConfig *config,
			     const char           *path,
			     const char           *attr,
			     const char           *value)
{
	BonoboUINode *node;

	bonobo_ui_engine_config_remove (config, path, attr);

	clobber_add (config, path, attr, value);

	if ((node = bonobo_ui_xml_get_path (config->priv->tree, path))) {
		char *existing;
		gboolean set = TRUE;

		if ((existing = bonobo_ui_node_get_attr (node, attr))) {
			if (!strcmp (existing, value))
				set = FALSE;
			bonobo_ui_node_free_string (existing);
		}

		if (set) {
			bonobo_ui_node_set_attr (node, attr, value);
			bonobo_ui_xml_set_dirty (config->priv->tree, node);
			bonobo_ui_engine_update (config->priv->engine);
		}
	}
}

void
bonobo_ui_engine_config_remove (BonoboUIEngineConfig *config,
				const char           *path,
				const char           *attr)
{
	GSList *l, *next;
	BonoboUINode *node;

	for (l = config->priv->clobbers; l; l = next) {
		clobber_t *cl = l->data;

		next = l->next;

		if (!strcmp (cl->path, path) &&
		    !strcmp (cl->attr, attr)) {
			config->priv->clobbers = g_slist_remove (
				config->priv->clobbers, cl);
			clobber_destroy (config->priv->tree, cl);
		}
	}

	if ((node = bonobo_ui_xml_get_path (config->priv->tree, path))) {

		if (bonobo_ui_node_has_attr (node, attr)) {
			bonobo_ui_node_remove_attr (node, attr);
			bonobo_ui_xml_set_dirty (config->priv->tree, node);
			bonobo_ui_engine_update (config->priv->engine);
		}
	}
}

static void
config_hydrate (BonoboUIEngineConfig *config)
{
	char **argv;
	int    argc, i;

	g_return_if_fail (config->priv->path != NULL);

	clobbers_free (config);

	gnome_config_get_vector (config->priv->path,
				 &argc, &argv);

	for (i = 0; i < argc; i++) {
		char **strs = g_strsplit (argv [i], ":", -1);

		if (!strs || !strs [0] || !strs [1] || !strs [2] || strs [3])
			g_warning ("Syntax error in '%s'", argv [i]);
		else
			clobber_add (config, strs [0], strs [1], strs [2]);

		g_strfreev (strs);
	}
}

typedef struct {
	BonoboUIEngine *engine;
	char           *path;
	BonoboUIEngineConfigFn     config_fn;
	BonoboUIEngineConfigVerbFn verb_fn;
} closure_t;

static void
closure_destroy (closure_t *c)
{
	g_free (c->path);
	g_free (c);
}

static char **
parse_dep_attr (const char *str,
		char      **attr,
		char      **type,
		char      **show_if_value,
		char      **set_to_value)
{
	char **strs;

	strs = g_strsplit (str, ":", -1);

	if (!strs || !strs [0] || !strs [1] ||
	    !strs [2] || !strs [3] || strs [4]) {
		g_strfreev (strs);
		g_warning ("dep_attr format error '%s'", str);
		return NULL;
	}

	if (attr)
		*attr = strs [0];

	if (type)
		*type = strs [1];

	if (show_if_value)
		*show_if_value = strs [2];

	if (set_to_value)
		*set_to_value = strs [3];

	return strs;
}

static void
emit_verb_on_cb (BonoboUIEngine *engine,
		 BonoboUINode   *popup_node,
		 closure_t      *c)
{
	if (c->verb_fn)
		c->verb_fn (bonobo_ui_engine_get_config (c->engine),
			    c->path, NULL, engine, popup_node);
}


static void
emit_event_on_cb (BonoboUIEngine *engine,
		  BonoboUINode   *popup_node,
		  const char     *state,
		  closure_t      *c)
{
	if (c->verb_fn)
		c->verb_fn (bonobo_ui_engine_get_config (c->engine),
			    c->path, state, engine, popup_node);
}

static BonoboUIEngine *
create_popup_engine (closure_t *c,
		     GtkMenu   *menu)
{
	BonoboUIEngine *engine;
	BonoboUISync   *smenu;
	BonoboUINode   *node;
	char           *str;

	engine = bonobo_ui_engine_new ();
	smenu  = bonobo_ui_sync_menu_new (engine, NULL, NULL, NULL);

	bonobo_ui_engine_add_sync (engine, smenu);

	node = bonobo_ui_engine_get_path (c->engine, c->path);
	if (c->config_fn)
		str = c->config_fn (
			bonobo_ui_engine_get_config (c->engine),
			node, engine);
	else
		str = NULL;

	g_return_val_if_fail (str != NULL, NULL);

	node = bonobo_ui_node_from_string (str);
	bonobo_ui_util_translate_ui (node);
	bonobo_ui_engine_xml_merge_tree (
		engine, "/", node, "popup");

	bonobo_ui_sync_menu_add_popup (
		BONOBO_UI_SYNC_MENU (smenu),
		menu, "/popups/popup");

	gtk_signal_connect (GTK_OBJECT (engine),
			    "emit_verb_on",
			    (GtkSignalFunc) emit_verb_on_cb, c);

	gtk_signal_connect (GTK_OBJECT (engine),
			    "emit_event_on",
			    (GtkSignalFunc) emit_event_on_cb, c);

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
				 BonoboUIEngineConfigFn     config_fn,
				 BonoboUIEngineConfigVerbFn verb_fn)
{
	BonoboUIEngineConfig *config;
	closure_t *c = g_new0 (closure_t, 1);

	c->engine    = engine;
	c->path      = g_strdup (path);
	c->config_fn = config_fn;
	c->verb_fn   = verb_fn;

	config = bonobo_ui_engine_get_config (engine);
	if (!config || !config->priv->path)
		return;

	gtk_signal_connect_full (
		GTK_OBJECT (widget), "button_press_event",
		(GtkSignalFunc) config_button_pressed, NULL, c,
		(GtkDestroyNotify) closure_destroy,
		FALSE, FALSE);
}

static void
bonobo_ui_engine_config_watch (BonoboUIXml    *xml,
			       const char     *path,
			       BonoboUINode   *opt_node,
			       gpointer        user_data)
{
	clobber_t *cl = user_data;

	if (opt_node) {
/*		g_warning ("Setting attr '%s' to '%s' on '%s",
		cl->attr, cl->value, path);*/
		bonobo_ui_node_set_attr (opt_node, cl->attr, cl->value);
	} else
		g_warning ("Stamp new config data onto NULL @ '%s'", path);
}

static void
impl_finalize (GtkObject *object)
{
	BonoboUIEngineConfig *config;
	BonoboUIEngineConfigPrivate *priv;

	config = BONOBO_UI_ENGINE_CONFIG (object);
	priv = config->priv;

	g_free (priv->path);

	clobbers_free (config);

	g_free (priv);

	parent_class->finalize (object);
}

static void
class_init (BonoboUIEngineClass *engine_class)
{
	GtkObjectClass *object_class;

	parent_class = gtk_type_class (PARENT_TYPE);

	object_class = GTK_OBJECT_CLASS (engine_class);

	object_class->finalize = impl_finalize;
}

static void
init (BonoboUIEngineConfig *config)
{
	BonoboUIEngineConfigPrivate *priv;

	priv = g_new0 (BonoboUIEngineConfigPrivate, 1);

	config->priv = priv;
}

GtkType
bonobo_ui_engine_config_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"BonoboUIEngineConfig",
			sizeof (BonoboUIEngineConfig),
			sizeof (BonoboUIEngineConfigClass),
			(GtkClassInitFunc)  class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		type = gtk_type_unique (PARENT_TYPE, &info);
	}

	return type;
}

BonoboUIEngineConfig *
bonobo_ui_engine_config_construct (BonoboUIEngineConfig *config,
				   BonoboUIEngine       *engine)
{
	config->priv->engine = engine;
	config->priv->tree   = bonobo_ui_engine_get_xml (engine);

	bonobo_ui_xml_set_watch_fn (
		bonobo_ui_engine_get_xml (engine),
		bonobo_ui_engine_config_watch);

	return config;
}

BonoboUIEngineConfig *
bonobo_ui_engine_config_new (BonoboUIEngine *engine)
{
	BonoboUIEngineConfig *config;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	config = gtk_type_new (bonobo_ui_engine_config_get_type ());

	return bonobo_ui_engine_config_construct (config, engine);
}

void
bonobo_ui_engine_config_set_path (BonoboUIEngine *engine,
				  const char     *path)
{
	BonoboUIEngineConfig *config;

	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	config = bonobo_ui_engine_get_config (engine);

	g_free (config->priv->path);
	config->priv->path = g_strdup (path);

	config_hydrate (config);
}

const char *
bonobo_ui_engine_config_get_path (BonoboUIEngine *engine)
{
	BonoboUIEngineConfig *config;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	config = bonobo_ui_engine_get_config (engine);
	
	return config->priv->path;
}
