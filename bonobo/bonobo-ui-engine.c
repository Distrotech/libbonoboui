/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * bonobo-ui-engine.c: The Bonobo UI/XML Sync engine.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include <config.h>
#include <stdlib.h>
#include <gtk/gtksignal.h>
#include <stdio.h>

#include <bonobo/bonobo-widget.h>
#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-engine.h>
#include <bonobo/bonobo-exception.h>

/* Various debugging output defines */
#undef STATE_SYNC_DEBUG
#undef WIDGET_SYNC_DEBUG
#undef XML_MERGE_DEBUG

#define PARENT_TYPE gtk_object_get_type ()

static GtkObjectClass *parent_class = NULL;

enum {
	ADD_HINT,
	REMOVE_HINT,
	EMIT_VERB_ON,
	EMIT_EVENT_ON,
	LAST_SIGNAL
};

guint signals [LAST_SIGNAL] = { 0 };

struct _BonoboUIEnginePrivate {
	BonoboUIXml  *tree;
	int           frozen;
	GSList       *syncs;
	GSList       *state_updates;
	GSList       *components;
	BonoboObject *container;

	char         *config_path;
};


/*
 *  Mapping from nodes to their synchronization
 * class information functions.
 */

static BonoboUISync *
find_sync_for_node (BonoboUIEngine *engine,
		    BonoboUINode   *node)
{
	GSList       *l;
	BonoboUISync *ret = NULL;

	if (!node) {
/*		fprintf (stderr, "Could not find sync for some path\n");*/
		return NULL;
	}

	for (l = engine->priv->syncs; l; l = l->next) {
		if (bonobo_ui_sync_can_handle (l->data, node)) {
			ret = l->data;
			break;
		}
	}

	if (ret) {
/*		fprintf (stderr, "Found sync '%s' for path '%s'\n",
			 gtk_type_name (GTK_OBJECT (ret)->klass->type),
			 bonobo_ui_xml_make_path (node));*/
		return ret;
	}

	return find_sync_for_node (
		engine, bonobo_ui_node_parent (node));
}

void
bonobo_ui_engine_add_sync (BonoboUIEngine *engine,
			   BonoboUISync   *sync)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if (g_slist_find (engine->priv->syncs, sync))
		g_warning ("Already added this Synchronizer %p", sync);
	else
		engine->priv->syncs = g_slist_append (
			engine->priv->syncs, sync);
}

void
bonobo_ui_engine_remove_sync (BonoboUIEngine *engine,
			      BonoboUISync   *sync)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	engine->priv->syncs = g_slist_remove (
		engine->priv->syncs, sync);
}

GSList *
bonobo_ui_engine_get_syncs (BonoboUIEngine *engine)
{
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	return g_slist_copy (engine->priv->syncs);
}

#define NODE_IS_ROOT_WIDGET(n)   ((n->type & ROOT_WIDGET) != 0)
#define NODE_IS_CUSTOM_WIDGET(n) ((n->type & CUSTOM_WIDGET) != 0)

typedef enum {
	ROOT_WIDGET   = 0x1,
	CUSTOM_WIDGET = 0x2
} NodeType;

typedef struct {
	BonoboUIXmlData parent;

	int             type;
	GtkWidget      *widget;
	Bonobo_Unknown  object;
} NodeInfo;

/*
 * BonoboUIXml impl functions
 */

static BonoboUIXmlData *
info_new_fn (void)
{
	NodeInfo *info = g_new0 (NodeInfo, 1);

	info->object = CORBA_OBJECT_NIL;

	return (BonoboUIXmlData *) info;
}

static void
info_free_fn (BonoboUIXmlData *data)
{
	NodeInfo *info = (NodeInfo *) data;

	if (info->object != CORBA_OBJECT_NIL) {
		bonobo_object_release_unref (info->object, NULL);
		info->object = CORBA_OBJECT_NIL;
	}
	info->widget = NULL;

	g_free (data);
}

static void
info_dump_fn (BonoboUIXml *tree, BonoboUINode *node)
{
	NodeInfo *info = bonobo_ui_xml_get_data (tree, node);

	if (info) {
		fprintf (stderr, " '%15s' object %8p type %d ",
			 (char *)info->parent.id, info->object, info->type);

		if (info->widget) {
			BonoboUINode *attached_node = 
				bonobo_ui_engine_widget_get_node (info->widget);

			fprintf (stderr, "widget '%8p' with node '%8p' attached ",
				 info->widget, attached_node);

			if (attached_node == NULL)
				fprintf (stderr, "is NULL\n");

			else if (attached_node != node)
				fprintf (stderr, "Serious mismatch attaches should be '%8p'\n",
					 node);
			else {
				if (info->widget->parent)
					fprintf (stderr, "and matching; parented\n");
				else
					fprintf (stderr, "and matching; BUT NO PARENT!\n");
			}
 		} else
			fprintf (stderr, " no associated widget\n");
	} else
		fprintf (stderr, " very weird no data on node '%p'\n", node);
}

/*
 * Insert slightly cleverly.
 *  NB. it is no use inserting into the default placeholder here
 *  since this will screw up path addressing and subsequent merging.
 */
static void
add_node_fn (BonoboUINode *parent, BonoboUINode *child)
{
	BonoboUINode *insert = parent;
	char    *pos;
/*	BonoboUINode *l;
	if (!bonobo_ui_node_get_attr (child, "noplace"))
		for (l = bonobo_ui_node_children (parent); l; l = l->next) {
			if (!strcmp (l->name, "placeholder") &&
			    !bonobo_ui_node_get_attr (l, "name")) {
			    insert = l;
				g_warning ("Found default placeholder");
			}
		}*/

	if (bonobo_ui_node_children (insert) &&
	    (pos = bonobo_ui_node_get_attr (child, "pos"))) {
		if (!strcmp (pos, "top")) {
			g_warning ("TESTME: unused code branch");
                        bonobo_ui_node_insert_before (bonobo_ui_node_children (insert),
                                                      child);
		} else
			bonobo_ui_node_add_child (insert, child);
		bonobo_ui_node_free_string (pos);
	} else /* just add to bottom */
		bonobo_ui_node_add_child (insert, child);
}

/*
 * BonoboUIXml signal functions
 */


static void
custom_widget_unparent (NodeInfo *info)
{
	GtkContainer *container;

	g_return_if_fail (info != NULL);

	if (!info->widget)
		return;

	g_return_if_fail (GTK_IS_WIDGET (info->widget));

	if (info->widget->parent) {
		container = GTK_CONTAINER (info->widget->parent);
		g_return_if_fail (container != NULL);

		gtk_widget_ref (info->widget);
		gtk_container_remove (container, info->widget);
	}
}

static void
sync_widget_set_node (BonoboUISync *sync,
		      GtkWidget    *widget,
		      BonoboUINode *node)
{
	GtkWidget *attached;

	if (!widget)
		return;

	g_return_if_fail (sync != NULL);

	bonobo_ui_engine_widget_attach_node (widget, node);

	attached = bonobo_ui_sync_get_attached (sync, widget, node);

	if (attached)
		bonobo_ui_engine_widget_attach_node (attached, node);
}

static void
replace_override_fn (GtkObject      *object,
		     BonoboUINode   *new,
		     BonoboUINode   *old,
		     BonoboUIEngine *engine)
{
	NodeInfo  *info = bonobo_ui_xml_get_data (engine->priv->tree, new);
	NodeInfo  *old_info = bonobo_ui_xml_get_data (engine->priv->tree, old);
	GtkWidget *old_widget;

	g_return_if_fail (info != NULL);
	g_return_if_fail (old_info != NULL);

/*	g_warning ("Replace override on '%s' '%s' widget '%p'",
		   old->name, bonobo_ui_node_get_attr (old, "name"), old_info->widget);
	info_dump_fn (old_info);
	info_dump_fn (info);*/

	/* Copy useful stuff across */
	old_widget = old_info->widget;
	old_info->widget = NULL;

	info->type = old_info->type;
	info->widget = old_widget;

	/* Re-stamp the widget - get sync from old node actually in tree */
	{
		BonoboUISync *sync = find_sync_for_node (engine, old);

		sync_widget_set_node (sync, info->widget, new);
	}

	/* Steal object reference */
	info->object = old_info->object;
	old_info->object = CORBA_OBJECT_NIL;
}

static void
prune_node (BonoboUIEngine *engine,
	    BonoboUINode   *node,
	    gboolean        save_custom)
{
	NodeInfo     *info;

	if (!node)
		return;

	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	if (info->widget) {
		gboolean save;
		
		save = NODE_IS_CUSTOM_WIDGET (info) && save_custom;

		if (!NODE_IS_ROOT_WIDGET (info) && !save) {
			BonoboUISync *sync;
			GtkWidget    *item;

			item = info->widget;

			if ((sync = find_sync_for_node (engine, node))) {
				GtkWidget *attached;

				attached = bonobo_ui_sync_get_attached (
					sync, item, node);

				if (attached) {
#ifdef XML_MERGE_DEBUG
					fprintf (stderr, "Got '%p' attached to '%p' for node '%s'\n",
						 attached, item,
						 bonobo_ui_xml_make_path (node));
#endif

					item = attached;
				}
			}

#ifdef XML_MERGE_DEBUG
			fprintf (stderr, "Destroy widget '%s' '%p'\n",
				 bonobo_ui_xml_make_path (node), item);
#endif

			gtk_widget_destroy (item);

		} else {
			if (save)
				custom_widget_unparent (info);
/*			printf ("leave widget '%s'\n",
			bonobo_ui_xml_make_path (node));*/
		}

		if (!save)
			info->widget = NULL;
	}
}

void
bonobo_ui_engine_prune_widget_info (BonoboUIEngine *engine,
				    BonoboUINode   *node,
				    gboolean        save_custom)
{
	BonoboUINode *l;

	if (!node)
		return;

	for (l = bonobo_ui_node_children (node); l;
             l = bonobo_ui_node_next (l))
		bonobo_ui_engine_prune_widget_info (
			engine, l, TRUE);

	prune_node (engine, node, save_custom);
}

static void
override_fn (GtkObject      *object,
	     BonoboUINode   *new,
	     BonoboUINode   *old,
	     BonoboUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Override '%s'\n", 
		 bonobo_ui_xml_make_path (old));
#endif

	bonobo_ui_engine_prune_widget_info (engine, old, TRUE);
}

static void
reinstate_fn (GtkObject      *object,
	      BonoboUINode   *node,
	      BonoboUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Reinstate '%s'\n", 
		 bonobo_ui_xml_make_path (node));
/*	bonobo_ui_engine_dump (engine, stderr, "pre reinstate_fn");*/
#endif

	bonobo_ui_engine_prune_widget_info (engine, node, TRUE);
}

static void
rename_fn (GtkObject      *object,
	   BonoboUINode   *node,
	   BonoboUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Rename '%s'\n", 
		 bonobo_ui_xml_make_path (node));
#endif
}

static void
remove_fn (GtkObject      *object,
	   BonoboUINode   *node,
	   BonoboUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Remove on '%s'\n",
		 bonobo_ui_xml_make_path (node));

/*	bonobo_ui_engine_dump (engine, stderr, "before remove_fn");*/
#endif

	bonobo_ui_engine_prune_widget_info (engine, node, FALSE);

	if (bonobo_ui_node_parent (node) == engine->priv->tree->root) {
		BonoboUISync *sync = find_sync_for_node (engine, node);

		if (sync)
			bonobo_ui_sync_remove_root (sync, node);
	}
}

/*
 * Sub Component management functions
 */

/*
 *  This indirection is needed so we can serialize user settings
 * on a per component basis in future.
 */
typedef struct {
	char          *name;
	Bonobo_Unknown object;
} SubComponent;


static SubComponent *
sub_component_get (BonoboUIEngine *engine, const char *name)
{
	SubComponent *component;
	GSList       *l;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	for (l = engine->priv->components; l; l = l->next) {
		component = l->data;
		
		if (!strcmp (component->name, name))
			return component;
	}
	
	component = g_new (SubComponent, 1);
	component->name = g_strdup (name);
	component->object = CORBA_OBJECT_NIL;

	engine->priv->components = g_slist_prepend (
		engine->priv->components, component);

	return component;
}

static SubComponent *
sub_component_get_by_ref (BonoboUIEngine *engine, CORBA_Object obj)
{
	GSList *l;
	SubComponent *component = NULL;
	CORBA_Environment ev;

	g_return_val_if_fail (obj != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	CORBA_exception_init (&ev);

	for (l = engine->priv->components; l; l = l->next) {
		gboolean equiv;
		component = l->data;

		equiv = CORBA_Object_is_equivalent (component->object, obj, &ev);

		if (BONOBO_EX (&ev)) { /* Something very badly wrong */
			component = NULL;
			break;
		} else if (equiv)
			break;
	}

	CORBA_exception_free (&ev);

	return component;
}

static Bonobo_Unknown
sub_component_objref (BonoboUIEngine *engine, const char *name)
{
	SubComponent *component = sub_component_get (engine, name);

	g_return_val_if_fail (component != NULL, CORBA_OBJECT_NIL);

	return component->object;
}

static void
sub_components_dump (BonoboUIEngine *engine, FILE *out)
{
	GSList *l;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (engine->priv != NULL);

	fprintf (out, "Component mappings:\n");

	for (l = engine->priv->components; l; l = l->next) {
		SubComponent *component = l->data;
		
		fprintf (out, "\t'%s' -> '%p'\n",
			 component->name, component->object);
	}
}

/* Use the pointer identity instead of a costly compare */
static char *
sub_component_cmp_name (BonoboUIEngine *engine, const char *name)
{
	SubComponent *component;

	/*
	 * NB. For overriding if we get a NULL we just update the
	 * node without altering the id.
	 */
	if (!name || name [0] == '\0') {
		g_warning ("This should never happen");
		return NULL;
	}

	component = sub_component_get (engine, name);
	g_return_val_if_fail (component != NULL, NULL);

	return component->name;
}

static void
sub_component_destroy (BonoboUIEngine *engine, SubComponent *component)
{
	if (engine->priv->container)
		gtk_signal_disconnect_by_data (
			GTK_OBJECT (engine->priv->container), engine);

	engine->priv->container = NULL;

	engine->priv->components = g_slist_remove (
		engine->priv->components, component);

	if (component) {
		g_free (component->name);
		if (component->object != CORBA_OBJECT_NIL)
			bonobo_object_release_unref (component->object, NULL);
		g_free (component);
	}
}

void
bonobo_ui_engine_deregister_dead_components (BonoboUIEngine *engine)
{
	SubComponent      *component;
	GSList            *l, *next;
	CORBA_Environment  ev;

	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	for (l = engine->priv->components; l; l = next) {
		next = l->next;

		component = l->data;
		CORBA_exception_init (&ev);

		if (CORBA_Object_non_existent (component->object, &ev))
			bonobo_ui_engine_deregister_component (
				engine, component->name);

		CORBA_exception_free (&ev);
	}
}

GList *
bonobo_ui_engine_get_component_names (BonoboUIEngine *engine)
{
	GSList *l;
	GList  *retval;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	retval = NULL;

	for (l = engine->priv->components; l; l = l->next) {
		SubComponent *component = l->data;
	
		retval = g_list_prepend (retval, component->name);
	}

	return retval;
}

Bonobo_Unknown
bonobo_ui_engine_get_component (BonoboUIEngine *engine,
				const char     *name)
{
	GSList *l;

	g_return_val_if_fail (name != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), CORBA_OBJECT_NIL);
		
	for (l = engine->priv->components; l; l = l->next) {
		SubComponent *component = l->data;
		
		if (!strcmp (component->name, name))
			return component->object;
	}

	return CORBA_OBJECT_NIL;
}

void
bonobo_ui_engine_register_component (BonoboUIEngine *engine,
				     const char     *name,
				     Bonobo_Unknown  component)
{
	SubComponent *subcomp;

	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if ((subcomp = sub_component_get (engine, name))) {
		if (subcomp->object != CORBA_OBJECT_NIL)
			bonobo_object_release_unref (subcomp->object, NULL);
	}

	if (component != CORBA_OBJECT_NIL)
		subcomp->object = bonobo_object_dup_ref (component, NULL);
	else
		subcomp->object = CORBA_OBJECT_NIL;
}

void
bonobo_ui_engine_deregister_component (BonoboUIEngine *engine,
				       const char     *name)
{
	SubComponent *component;

	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if ((component = sub_component_get (engine, name))) {
		bonobo_ui_engine_xml_rm (engine, "/", component->name);
		sub_component_destroy (engine, component);
	} else
		g_warning ("Attempting to deregister non-registered "
			   "component '%s'", name);
}

void
bonobo_ui_engine_deregister_component_by_ref (BonoboUIEngine *engine,
					      Bonobo_Unknown  ref)
{
	SubComponent *component;

	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if ((component = sub_component_get_by_ref (engine, ref))) {
		bonobo_ui_engine_xml_rm (engine, "/", component->name);
		sub_component_destroy (engine, component);
	} else
		g_warning ("Attempting to deregister non-registered "
			   "component");
}

/*
 * State update signal queueing functions
 */

typedef struct {
	BonoboUISync *sync;
	GtkWidget    *widget;
	char         *state;
} StateUpdate;

/*
 * Update the state later, but other aspects of the widget right now.
 * It's dangerous to update the state now because we can reenter if we
 * do that.
 */
static StateUpdate *
state_update_new (BonoboUISync *sync,
		  GtkWidget    *widget,
		  BonoboUINode *node)
{
	char *hidden, *sensitive, *state;
	StateUpdate   *su;

	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	hidden = bonobo_ui_node_get_attr (node, "hidden");
	if (hidden && atoi (hidden))
		gtk_widget_hide (widget);
	else
		gtk_widget_show (widget);
	bonobo_ui_node_free_string (hidden);

	sensitive = bonobo_ui_node_get_attr (node, "sensitive");
	if (sensitive)
		gtk_widget_set_sensitive (widget, atoi (sensitive));
	bonobo_ui_node_free_string (sensitive);

	if ((state = bonobo_ui_node_get_attr (node, "state"))) {
		su = g_new0 (StateUpdate, 1);
		
		su->sync   = sync;
		su->widget = widget;
		gtk_widget_ref (su->widget);
		su->state  = state;
	} else
		su = NULL;

	return su;
}

static void
state_update_destroy (StateUpdate *su)
{
	if (su) {
		gtk_widget_unref (su->widget);
		bonobo_ui_node_free_string (su->state);

		g_free (su);
	}
}

static void
state_update_now (BonoboUIEngine *engine,
		  BonoboUINode   *node,
		  GtkWidget      *widget)
{
	StateUpdate  *su;
	BonoboUISync *sync;

	if (!widget)
		return;

	sync = find_sync_for_node (engine, node);
	g_return_if_fail (sync != NULL);

	su = state_update_new (sync, widget, node);
	
	if (su) {
		bonobo_ui_sync_state_update (su->sync, su->widget, su->state);
		state_update_destroy (su);
	}
}

CORBA_char *
bonobo_ui_engine_xml_get (BonoboUIEngine *engine,
			  const char     *path,
			  gboolean        node_only)
{
 	char         *str;
 	BonoboUINode *node;
  	CORBA_char   *ret;
  
  	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);
  
  	node = bonobo_ui_xml_get_path (engine->priv->tree, path);
  	if (!node)
  		return NULL;
 	else {		
 		str = bonobo_ui_node_to_string (node, !node_only);
 		ret = CORBA_string_dup (str);
 		bonobo_ui_node_free_string (str);
 		return ret;
  	}
}

gboolean
bonobo_ui_engine_xml_node_exists (BonoboUIEngine   *engine,
				  const char       *path)
{
	BonoboUINode *node;
	gboolean      wildcard;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), FALSE);

	node = bonobo_ui_xml_get_path_wildcard (
		engine->priv->tree, path, &wildcard);

	if (!wildcard)
		return (node != NULL);
	else
		return (node != NULL &&
			bonobo_ui_node_children (node) != NULL);
}

BonoboUIError
bonobo_ui_engine_object_set (BonoboUIEngine   *engine,
			     const char       *path,
			     Bonobo_Unknown    object,
			     CORBA_Environment *ev)
{
	NodeInfo     *info;
	BonoboUINode *node;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), 
			      BONOBO_UI_ERROR_BAD_PARAM);

	node = bonobo_ui_xml_get_path (engine->priv->tree, path);
	if (!node)
		return BONOBO_UI_ERROR_INVALID_PATH;

	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	if (info->object != CORBA_OBJECT_NIL) {
		bonobo_object_release_unref (info->object, ev);
		if (info->widget)
			gtk_widget_destroy (info->widget);
		info->widget = NULL;
	}

	if (object != CORBA_OBJECT_NIL)
		info->object = bonobo_object_dup_ref (object, ev);
	else
		info->object = CORBA_OBJECT_NIL;

	bonobo_ui_xml_set_dirty (engine->priv->tree, node);

/*	fprintf (stderr, "Object set '%s'\n", path);
	bonobo_ui_engine_dump (win, "Before object set updatew");*/

	bonobo_ui_engine_update (engine);

/*	bonobo_ui_engine_dump (win, "After object set updatew");*/

	return BONOBO_UI_ERROR_OK;
	
}

BonoboUIError
bonobo_ui_engine_object_get (BonoboUIEngine    *engine,
			     const char        *path,
			     Bonobo_Unknown    *object,
			     CORBA_Environment *ev)
{
	NodeInfo     *info;
	BonoboUINode *node;

	g_return_val_if_fail (object != NULL,
			      BONOBO_UI_ERROR_BAD_PARAM);
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), 
			      BONOBO_UI_ERROR_BAD_PARAM);

	*object = CORBA_OBJECT_NIL;

	node = bonobo_ui_xml_get_path (engine->priv->tree, path);

	if (!node)
		return BONOBO_UI_ERROR_INVALID_PATH;

	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	if (info->object != CORBA_OBJECT_NIL)
		*object = bonobo_object_dup_ref (info->object, ev);

	return BONOBO_UI_ERROR_OK;
}

BonoboUIError
bonobo_ui_engine_xml_merge_tree (BonoboUIEngine    *engine,
				 const char        *path,
				 BonoboUINode      *tree,
				 const char        *component)
{
	BonoboUIError err;
	
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), 
			      BONOBO_UI_ERROR_BAD_PARAM);

	if (!tree || !bonobo_ui_node_get_name (tree))
		return BONOBO_UI_ERROR_OK;

	bonobo_ui_xml_strip (&tree);

	if (!tree) {
		g_warning ("Stripped tree to nothing");
		return BONOBO_UI_ERROR_OK;
	}

	/*
	 *  Because peer to peer merging makes the code hard, and
	 * paths non-inituitive and since we want to merge root
	 * elements as peers to save lots of redundant CORBA calls
	 * we special case root.
	 */
	if (bonobo_ui_node_has_name (tree, "Root")) {
		err = bonobo_ui_xml_merge (
			engine->priv->tree, path, bonobo_ui_node_children (tree),
			sub_component_cmp_name (engine, component));
		bonobo_ui_node_free (tree);
	} else
		err = bonobo_ui_xml_merge (
			engine->priv->tree, path, tree,
			sub_component_cmp_name (engine, component));

#ifdef XML_MERGE_DEBUG
/*	bonobo_ui_engine_dump (engine, stderr, "after merge");*/
#endif

	bonobo_ui_engine_update (engine);

	return err;
}

BonoboUIError
bonobo_ui_engine_xml_rm (BonoboUIEngine *engine,
			 const char     *path,
			 const char     *by_component)
{
	BonoboUIError err;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine),
			      BONOBO_UI_ERROR_BAD_PARAM);

	err = bonobo_ui_xml_rm (
		engine->priv->tree, path,
		sub_component_cmp_name (engine, by_component));

	bonobo_ui_engine_update (engine);

	return err;
}

static void
blank_container (BonoboUIContainer *container,
		 BonoboUIEngine    *engine)
{
	if (engine->priv)
		engine->priv->container = NULL;
}

void
bonobo_ui_engine_set_ui_container (BonoboUIEngine *engine,
				   BonoboObject   *ui_container)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));
	g_return_if_fail (ui_container == CORBA_OBJECT_NIL ||
			  BONOBO_IS_UI_CONTAINER (ui_container));

	engine->priv->container = ui_container;

	if (ui_container)
		gtk_signal_connect (GTK_OBJECT (ui_container), "destroy",
				    (GtkSignalFunc) blank_container, engine);
}

static char *
node_get_id (BonoboUINode *node)
{
	char *txt;
	char *ret;

	g_return_val_if_fail (node != NULL, NULL);

	if (!(txt = bonobo_ui_node_get_attr (node, "id"))) {
		txt = bonobo_ui_node_get_attr (node, "verb");
		if (txt && txt [0] == '\0') {
			bonobo_ui_node_free_string (txt);
			txt = bonobo_ui_node_get_attr (node, "name");
		}
	}

	if (txt) {
		ret = g_strdup (txt);
		bonobo_ui_node_free_string (txt);
	} else
		ret = NULL;

	return ret;
}

static void
real_exec_verb (BonoboUIEngine *engine,
		const char     *component_name,
		const char     *verb)
{
	Bonobo_UIComponent component;

	g_return_if_fail (verb != NULL);
	g_return_if_fail (component_name != NULL);
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	gtk_object_ref (GTK_OBJECT (engine));

	component = sub_component_objref (engine, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		Bonobo_UIComponent_execVerb (
			component, verb, &ev);

		if (engine->priv->container)
			bonobo_object_check_env (
				engine->priv->container, component, &ev);

		if (BONOBO_EX (&ev))
			g_warning ("Exception executing verb '%s' '%s'"
				   "major %d, %s",
				   verb, component_name, ev._major, ev._repo_id);
		
		CORBA_exception_free (&ev);
	}

	gtk_object_unref (GTK_OBJECT (engine));
}

static void
impl_emit_verb_on (BonoboUIEngine *engine,
		   BonoboUINode   *node)
{
	CORBA_char      *verb;
	BonoboUIXmlData *data;
	
	g_return_if_fail (node != NULL);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_if_fail (data != NULL);

	verb = node_get_id (node);
	if (!verb)
		return;

	if (!data->id) {
		g_warning ("Weird; no ID on verb '%s'", verb);
		bonobo_ui_node_free_string (verb);
		return;
	}

	real_exec_verb (engine, data->id, verb);

	g_free (verb);
}

static BonoboUINode *
cmd_get_node (BonoboUIEngine *engine,
	      BonoboUINode   *from_node)
{
	char         *path;
	BonoboUINode *ret;
	char         *cmd_name;

	g_return_val_if_fail (engine != NULL, NULL);

	if (!from_node)
		return NULL;

	if (!(cmd_name = node_get_id (from_node)))
		return NULL;

	path = g_strconcat ("/commands/", cmd_name, NULL);
	ret  = bonobo_ui_xml_get_path (engine->priv->tree, path);

	if (!ret) {
		BonoboUIXmlData *data_from;
		BonoboUINode *commands;
		BonoboUINode *node;

		commands = bonobo_ui_node_new ("commands");
		node     = bonobo_ui_node_new_child (commands, "cmd");

		bonobo_ui_node_set_attr (node, "name", cmd_name);

		data_from = bonobo_ui_xml_get_data (
			engine->priv->tree, from_node);

		bonobo_ui_xml_merge (
			engine->priv->tree, "/",
			commands, data_from->id);
		
		ret = bonobo_ui_xml_get_path (
			engine->priv->tree, path);
		g_assert (ret != NULL);
	}

	g_free (path);
	g_free (cmd_name);

	return ret;
}

static GSList *
make_updates_for_command (BonoboUIEngine *engine,
			  GSList         *list,
			  BonoboUINode   *search,
			  BonoboUINode   *state,
			  const char     *search_id)
{
	BonoboUINode *l;
	char *id = node_get_id (search);

/*	printf ("Update cmd state if %s == %s on node '%s'\n", search_id, id,
	bonobo_ui_xml_make_path (search));*/

	if (id && !strcmp (search_id, id)) { /* Sync its state */
		NodeInfo *info = bonobo_ui_xml_get_data (
			engine->priv->tree, search);

		if (info->widget) {
			BonoboUISync *sync;
			StateUpdate  *su;

			sync = find_sync_for_node (engine, search);
			g_return_val_if_fail (sync != NULL, list);

			su = state_update_new (sync, info->widget, state);

			if (su)
				list = g_slist_prepend (list, su);
		}
	}

	g_free (id);

	for (l = bonobo_ui_node_children (search); l;
             l = bonobo_ui_node_next (l))
		list = make_updates_for_command (
			engine, list, l, state, search_id);

	return list;
}

static void
update_cmd_state (BonoboUIEngine *engine, BonoboUINode *search,
		  BonoboUINode   *state, const char *search_id)
{
	GSList *updates, *l;

	g_return_if_fail (search_id != NULL);

/*	printf ("Update cmd state on %s from node '%s'\n", search_id,
	bonobo_ui_xml_make_path (state));*/

	updates = make_updates_for_command (
		engine, NULL, engine->priv->tree->root, state, search_id);

	for (l = updates; l; l = l->next) {
		StateUpdate *su = l->data;

		bonobo_ui_sync_state_update (su->sync, su->widget, su->state);
	}

	for (l = updates; l; l = l->next)
		state_update_destroy (l->data);

	g_slist_free (updates);
}

/*
 * set_cmd_attr:
 *   Syncs cmd / widgets on events [ event flag set ]
 *   or helps evil people who set state on menu /
 *      toolbar items instead of on the associated verb / id.
 **/
static void
set_cmd_attr (BonoboUIEngine *engine,
	      BonoboUINode   *node,
	      const char     *prop,
	      const char     *value,
	      gboolean        event)
{
	BonoboUINode *cmd_node;

	g_return_if_fail (prop != NULL);
	g_return_if_fail (node != NULL);
	g_return_if_fail (value != NULL);
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if (!(cmd_node = cmd_get_node (engine, node))) { /* A non cmd widget */
		NodeInfo *info = bonobo_ui_xml_get_data (
			engine->priv->tree, node);

		bonobo_ui_node_set_attr (node, prop, value);
		state_update_now (engine, node, info->widget);
		return;
	}

#ifdef STATE_SYNC_DEBUG
	fprintf (stderr, "Set '%s' : '%s' to '%s'",
		 bonobo_ui_node_get_attr (cmd_node, "name"),
		 prop, value);
#endif

	bonobo_ui_node_set_attr (cmd_node, prop, value);

	if (event) {
		char *cmd_name = bonobo_ui_node_get_attr (
			cmd_node, "name");

		update_cmd_state (
			engine, engine->priv->tree->root,
			cmd_node, cmd_name);

		bonobo_ui_node_free_string (cmd_name);
	} else {
		BonoboUIXmlData *data =
			bonobo_ui_xml_get_data (
				engine->priv->tree, cmd_node);

		data->dirty = TRUE;
	}
}

static void
real_emit_ui_event (BonoboUIEngine *engine,
		    const char     *component_name,
		    const char     *id,
		    int             type,
		    const char     *new_state)
{
	Bonobo_UIComponent component;

	g_return_if_fail (id != NULL);
	g_return_if_fail (new_state != NULL);

	if (!component_name) /* Auto-created entry, no-one can listen to it */
		return;

	gtk_object_ref (GTK_OBJECT (engine));

	component = sub_component_objref (engine, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		Bonobo_UIComponent_uiEvent (
			component, id, type, new_state, &ev);

		if (engine->priv->container)
			bonobo_object_check_env (
				engine->priv->container,
				component, &ev);

		if (BONOBO_EX (&ev))
			g_warning ("Exception emitting state change to %d '%s' '%s'"
				   "major %d, %s",
				   type, id, new_state, ev._major, ev._repo_id);
		
		CORBA_exception_free (&ev);
	}

	gtk_object_unref (GTK_OBJECT (engine));
}

static void
impl_emit_event_on (BonoboUIEngine *engine,
		    BonoboUINode   *node,
		    const char     *state)
{
	char            *id;
	BonoboUIXmlData *data;
	char            *component_id;

	g_return_if_fail (node != NULL);

	if (!(id = node_get_id (node)))
		return;

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_if_fail (data != NULL);

	component_id = g_strdup (data->id);

	/* This could invoke a CORBA method that might de-register the component */
	set_cmd_attr (engine, node, "state", state, TRUE);

	real_emit_ui_event (engine, component_id, id,
			    Bonobo_UIComponent_STATE_CHANGED,
			    state);

	g_free (component_id);
	g_free (id);
}

static void
impl_destroy (GtkObject *object)
{
	BonoboUIEngine *engine;
	BonoboUIEnginePrivate *priv;
	GSList *l;

	engine = BONOBO_UI_ENGINE (object);
	priv = engine->priv;

	while (priv->components)
		sub_component_destroy (
			engine, priv->components->data);

	gtk_object_unref (GTK_OBJECT (priv->tree));
	priv->tree = NULL;

	for (l = priv->syncs; l; l = l->next)
		gtk_object_unref (GTK_OBJECT (l->data));
	g_slist_free (priv->syncs);
	priv->syncs = NULL;

	parent_class->destroy (object);
}

static void
impl_finalize (GtkObject *object)
{
	BonoboUIEngine *engine;
	BonoboUIEnginePrivate *priv;

	engine = BONOBO_UI_ENGINE (object);
	priv = engine->priv;
	
	g_free (priv);

	parent_class->finalize (object);
}

static void
class_init (BonoboUIEngineClass *engine_class)
{
	GtkObjectClass *object_class;

	parent_class = gtk_type_class (PARENT_TYPE);

	object_class = GTK_OBJECT_CLASS (engine_class);
	object_class->destroy  = impl_destroy;
	object_class->finalize = impl_finalize;

	engine_class->emit_verb_on  = impl_emit_verb_on;
	engine_class->emit_event_on = impl_emit_event_on;

	signals [ADD_HINT]
		= gtk_signal_new ("add_hint",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (BonoboUIEngineClass, add_hint),
				  gtk_marshal_NONE__STRING,
				  GTK_TYPE_NONE, 1, GTK_TYPE_STRING);

	signals [REMOVE_HINT]
		= gtk_signal_new ("remove_hint",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (BonoboUIEngineClass, remove_hint),
				  gtk_marshal_NONE__NONE,
				  GTK_TYPE_NONE, 0);

	signals [EMIT_VERB_ON]
		= gtk_signal_new ("emit_verb_on",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (BonoboUIEngineClass, emit_verb_on),
				  gtk_marshal_NONE__POINTER,
				  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	signals [EMIT_EVENT_ON]
		= gtk_signal_new ("emit_event_on",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (BonoboUIEngineClass, emit_event_on),
				  gtk_marshal_NONE__POINTER_POINTER,
				  GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
init (BonoboUIEngine *engine)
{
	BonoboUIEnginePrivate *priv;

	priv = g_new0 (BonoboUIEnginePrivate, 1);

	engine->priv = priv;
}

GtkType
bonobo_ui_engine_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"BonoboUIEngine",
			sizeof (BonoboUIEngine),
			sizeof (BonoboUIEngineClass),
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

static void
add_node (BonoboUINode *parent, const char *name)
{
        BonoboUINode *node = bonobo_ui_node_new (name);

	bonobo_ui_node_add_child (parent, node);
}

static void
build_skeleton (BonoboUIXml *xml)
{
	g_return_if_fail (BONOBO_IS_UI_XML (xml));

	add_node (xml->root, "keybindings");
	add_node (xml->root, "commands");
}

BonoboUIEngine *
bonobo_ui_engine_construct (BonoboUIEngine *engine)
{
	BonoboUIEnginePrivate *priv;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	priv = engine->priv;

	priv->tree = bonobo_ui_xml_new (NULL,
					info_new_fn,
					info_free_fn,
					info_dump_fn,
					add_node_fn);

	build_skeleton (priv->tree);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "override",
			    (GtkSignalFunc) override_fn, engine);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "replace_override",
			    (GtkSignalFunc) replace_override_fn, engine);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "reinstate",
			    (GtkSignalFunc) reinstate_fn, engine);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "rename",
			    (GtkSignalFunc) rename_fn, engine);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "remove",
			    (GtkSignalFunc) remove_fn, engine);

	return engine;
}


BonoboUIEngine *
bonobo_ui_engine_new (void)
{
	BonoboUIEngine *engine = gtk_type_new (BONOBO_TYPE_UI_ENGINE);

	return bonobo_ui_engine_construct (engine);
}


static void
hide_all_widgets (BonoboUIEngine *engine,
		  BonoboUINode   *node)
{
	NodeInfo *info;
	BonoboUINode *child;
	
	info = bonobo_ui_xml_get_data (engine->priv->tree, node);
	if (info->widget)
		gtk_widget_hide (info->widget);
	
	for (child = bonobo_ui_node_children (node);
	     child != NULL;
	     child = bonobo_ui_node_next (child))
		hide_all_widgets (engine, child);
}

static gboolean
contains_visible_widget (BonoboUIEngine *engine,
			 BonoboUINode   *node)
{
	BonoboUINode *child;
	NodeInfo     *info;
	
	for (child = bonobo_ui_node_children (node);
	     child != NULL;
	     child = bonobo_ui_node_next (child)) {
		info = bonobo_ui_xml_get_data (engine->priv->tree, child);
		if (info->widget && GTK_WIDGET_VISIBLE (info->widget))
			return TRUE;
		if (contains_visible_widget (engine, child))
			return TRUE;
	}

	return FALSE;
}

static void
hide_placeholder_if_empty_or_hidden (BonoboUIEngine *engine,
				     BonoboUINode   *node)
{
	NodeInfo *info;
	char *txt;
	gboolean hide_placeholder_and_contents;
	gboolean has_visible_separator;

	txt = bonobo_ui_node_get_attr (node, "hidden");
	hide_placeholder_and_contents = txt && atoi (txt);
	bonobo_ui_node_free_string (txt);

	info = bonobo_ui_xml_get_data (engine->priv->tree, node);
	has_visible_separator = info && info->widget
		&& GTK_WIDGET_VISIBLE (info->widget);

	if (hide_placeholder_and_contents)
		hide_all_widgets (engine, node);

	else if (has_visible_separator
		 && !contains_visible_widget (engine, node))
		gtk_widget_hide (info->widget);
}

static gboolean
node_is_dirty (BonoboUIEngine *engine,
	       BonoboUINode   *node)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (
		engine->priv->tree, node);

	if (!data)
		return TRUE;
	else
		return data->dirty;
}

static void
bonobo_ui_engine_sync (BonoboUIEngine   *engine,
		       BonoboUISync     *sync,
		       BonoboUINode     *node,
		       GtkWidget        *parent,
		       GList           **widgets,
		       int              *pos)
{
	BonoboUINode *a;
	GList        *b, *nextb;

#ifdef WIDGET_SYNC_DEBUG
	printf ("In sync to pos %d with widgets:\n", *pos);
	for (b = *widgets; b; b = b->next) {
		BonoboUINode *node = bonobo_ui_engine_widget_get_node (b->data);

		if (node)
			printf ("\t'%s'\n", bonobo_ui_xml_make_path (node));
		else
			printf ("\tno node ptr\n");
	}
#endif

	b = *widgets;
	for (a = node; a; b = nextb) {
		gboolean same;

		nextb = b ? b->next : NULL;

		if (b && bonobo_ui_sync_ignore_widget (sync, b->data)) {
			(*pos)++;
			continue;
		}

		same = (b != NULL) &&
			(bonobo_ui_engine_widget_get_node (b->data) == a);

#ifdef WIDGET_SYNC_DEBUG
		printf ("Node '%s(%p)' Dirty '%d' same %d on b %d widget %p\n",
			bonobo_ui_xml_make_path (a), a,
			node_is_dirty (engine, a), same, b != NULL, b?b->data:NULL);
#endif

		if (node_is_dirty (engine, a)) {
			BonoboUISyncStateFn ss;
			BonoboUISyncBuildFn bw;
			BonoboUINode       *cmd_node;
			
			if (bonobo_ui_node_has_name (a, "placeholder")) {
				ss = bonobo_ui_sync_state_placeholder;
				bw = bonobo_ui_sync_build_placeholder;
			} else {
				ss = bonobo_ui_sync_state;
				bw = bonobo_ui_sync_build;
			}

			cmd_node = bonobo_ui_engine_get_cmd_node (engine, a);

			if (same) {
#ifdef WIDGET_SYNC_DEBUG
				printf ("-- just syncing state --\n");
#endif
				ss (sync, a, cmd_node, b->data, parent);
				(*pos)++;
			} else {
				NodeInfo   *info;
				GtkWidget  *widget;

				info = bonobo_ui_xml_get_data (
					engine->priv->tree, a);

#ifdef WIDGET_SYNC_DEBUG
				printf ("re-building widget\n");
#endif

				widget = bw (sync, a, cmd_node, pos, parent);

#ifdef WIDGET_SYNC_DEBUG
				printf ("Built item '%p' '%s' and inserted at '%d'\n",
					widget, bonobo_ui_node_get_name (a), *pos);
#endif

				info->widget = widget;
				if (widget) {
					bonobo_ui_engine_widget_set_node (
						sync->engine, widget, a);

					ss (sync, a, cmd_node, widget, parent);
				}
#ifdef WIDGET_SYNC_DEBUG
				else
					printf ("Failed to build widget\n");
#endif

				nextb = b; /* NB. don't advance 'b' */
			}

		} else {
			if (!same) {
				BonoboUINode *bn = b ? bonobo_ui_engine_widget_get_node (b->data) : NULL;
				NodeInfo   *info;

				info = bonobo_ui_xml_get_data (engine->priv->tree, a);

				if (!info->widget) {
					/*
					 *  A control that hasn't been filled out yet
					 * and thus has no widget in 'b' list but has
					 * a node in 'a' list, thus we want to stick
					 * on this 'b' node until a more favorable 'a'
					 */
					nextb = b;
					(*pos)--;
					g_assert (info->type | CUSTOM_WIDGET);
#ifdef WIDGET_SYNC_DEBUG
					printf ("not dirty & not same, but has no widget\n");
#endif
				} else {
					g_warning ("non dirty node, but widget mismatch "
						   "a: '%s:%s', b: '%s:%s' '%p'",
						   bonobo_ui_node_get_name (a),
						   bonobo_ui_node_get_attr (a, "name"),
						   bn ? bonobo_ui_node_get_name (bn) : "NULL",
						   bn ? bonobo_ui_node_get_attr (bn, "name") : "NULL",
						   info->widget);
				}
			}
#ifdef WIDGET_SYNC_DEBUG
			else
				printf ("not dirty & same: no change\n");
#endif
			(*pos)++;
		}

		if (bonobo_ui_node_has_name (a, "placeholder")) {
			bonobo_ui_engine_sync (
				engine, sync,
				bonobo_ui_node_children (a),
				parent, &nextb, pos);

			hide_placeholder_if_empty_or_hidden (engine, a);
		}

		a = bonobo_ui_node_next (a);
	}

	while (b && bonobo_ui_sync_ignore_widget (sync, b->data))
		b = b->next;

	*widgets = b;
}

static void
check_excess_widgets (BonoboUISync *sync, GList *wptr)
{
	if (wptr) {
		GList *b;
		int warned = 0;

		for (b = wptr; b; b = b->next) {
			BonoboUINode *node;

			if (bonobo_ui_sync_ignore_widget (sync, b->data))
				continue;
			
			if (!warned++)
				g_warning ("Excess widgets at the "
					   "end of the container; weird");

			node = bonobo_ui_engine_widget_get_node (b->data);
			g_message ("Widget type '%s' with node: '%s'",
				   gtk_type_name (GTK_OBJECT (b->data)->klass->type),
				   node ? bonobo_ui_xml_make_path (node) : "NULL");
		}
	}
}

static void
do_sync (BonoboUIEngine *engine,
	 BonoboUISync   *sync,
	 BonoboUINode   *node)
{
#ifdef WIDGET_SYNC_DEBUG
	fprintf (stderr, "Syncing ('%s') on node '%s'\n",
		 gtk_type_name (GTK_OBJECT (sync)->klass->type),
		 bonobo_ui_xml_make_path (node));
#endif
	if (bonobo_ui_node_parent (node) == engine->priv->tree->root)
		bonobo_ui_sync_update_root (sync, node);

	/* FIXME: it would be nice to sub-class again to get rid of this */
	if (bonobo_ui_sync_has_widgets (sync)) {
		int       pos;
		GList    *widgets, *wptr;

		wptr = widgets = bonobo_ui_sync_get_widgets (sync, node);

		pos = 0;
		bonobo_ui_engine_sync (
			engine, sync, bonobo_ui_node_children (node),
			bonobo_ui_engine_node_get_widget (engine, node),
			&wptr, &pos);
		
		check_excess_widgets (sync, wptr);
		
		g_list_free (widgets);
	}

	bonobo_ui_xml_clean (engine->priv->tree, node);
}

static void
seek_dirty (BonoboUIEngine *engine,
	    BonoboUISync   *sync,
	    BonoboUINode   *node)
{
	BonoboUIXmlData *info;

	if (!node)
		return;

	info = bonobo_ui_xml_get_data (engine->priv->tree, node);
	if (info->dirty) { /* Rebuild tree from here down */

		do_sync (engine, sync, node);

	} else {
		BonoboUINode *l;

		for (l = bonobo_ui_node_children (node); l;
		     l = bonobo_ui_node_next (l))
			seek_dirty (engine, sync, l);
	}
}

void
bonobo_ui_engine_update_node (BonoboUIEngine *engine,
			      BonoboUINode   *node)
{
	BonoboUISync *sync = find_sync_for_node (engine, node);

	if ((sync = find_sync_for_node (engine, node))) {
		if (bonobo_ui_sync_is_recursive (sync))
			seek_dirty (engine, sync, node);
		else 
			do_sync (engine, sync, node);
	}
#ifdef WIDGET_SYNC_DEBUG
	else if (!bonobo_ui_node_has_name (node, "commands"))
		g_warning ("No syncer for '%s'",
			   bonobo_ui_xml_make_path (node));
#endif
}

static void
dirty_by_cmd (BonoboUIEnginePrivate *priv,
	      BonoboUINode          *search,
	      const char            *search_id)
{
	BonoboUINode *l;
	char         *id = node_get_id (search);

	g_return_if_fail (search_id != NULL);

/*	printf ("Dirty node by cmd if %s == %s on node '%s'\n", search_id, id,
	bonobo_ui_xml_make_path (search));*/

	if (id && !strcmp (search_id, id)) /* Dirty it */
		bonobo_ui_xml_set_dirty (priv->tree, search);

	g_free (id);

	for (l = bonobo_ui_node_children (search); l;
             l = bonobo_ui_node_next (l))
		dirty_by_cmd (priv, l, search_id);
}

static void
move_dirt_cmd_to_widget (BonoboUIEngine *engine)
{
	BonoboUINode *cmds, *l;

	cmds = bonobo_ui_xml_get_path (engine->priv->tree, "/commands");

	if (!cmds)
		return;

	for (l = bonobo_ui_node_children (cmds); l;
             l = bonobo_ui_node_next (l)) {
		BonoboUIXmlData *data = bonobo_ui_xml_get_data (engine->priv->tree, l);

		if (data->dirty) {
			char *cmd_name;

			cmd_name = bonobo_ui_node_get_attr (l, "name");
			if (!cmd_name)
				g_warning ("Serious error, cmd without name");
			else
				dirty_by_cmd (engine->priv,
					      engine->priv->tree->root,
					      cmd_name);

			bonobo_ui_node_free_string (cmd_name);
		}
	}
}

static void
update_commands_state (BonoboUIEngine *engine)
{
	BonoboUINode *cmds, *l;

	cmds = bonobo_ui_xml_get_path (engine->priv->tree, "/commands");

/*	g_warning ("Update commands state!");
	bonobo_ui_engine_dump (priv->win, "before update");*/

	if (!cmds)
		return;

	for (l = bonobo_ui_node_children (cmds); l;
             l = bonobo_ui_node_next (l)) {
		BonoboUIXmlData *data = bonobo_ui_xml_get_data (
			engine->priv->tree, l);
		char *cmd_name;

		cmd_name = bonobo_ui_node_get_attr (l, "name");
		if (!cmd_name)
			g_warning ("Internal error; cmd with no id");

		else if (data->dirty)
			update_cmd_state (
				engine, engine->priv->tree->root, 
				l, cmd_name);

		data->dirty = FALSE;
		bonobo_ui_node_free_string (cmd_name);
	}
}

static void
process_state_updates (BonoboUIEngine *engine)
{
	while (engine->priv->state_updates) {
		StateUpdate *su = engine->priv->state_updates->data;

		engine->priv->state_updates = g_slist_remove (
			engine->priv->state_updates, su);

		bonobo_ui_sync_state_update (
			su->sync, su->widget, su->state);

		state_update_destroy (su);
	}
}

void
bonobo_ui_engine_update (BonoboUIEngine *engine)
{
	BonoboUINode *node;
	GSList *l;

	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if (engine->priv->frozen)
		return;

	for (l = engine->priv->syncs; l; l = l->next)
		bonobo_ui_sync_stamp_root (l->data);

	move_dirt_cmd_to_widget (engine);

/*	bonobo_ui_engine_dump (priv->win, "before update");*/

	for (node = bonobo_ui_node_children (engine->priv->tree->root);
	     node; node = bonobo_ui_node_next (node)) {

		if (!bonobo_ui_node_get_name (node))
			continue;

		bonobo_ui_engine_update_node (engine, node);
	}

	update_commands_state (engine);

	process_state_updates (engine);

/*	bonobo_ui_engine_dump (priv->win, "after update");*/
}

void
bonobo_ui_engine_queue_update (BonoboUIEngine   *engine,
			       GtkWidget        *widget,
			       BonoboUINode     *node,
			       BonoboUINode     *cmd_node)
{
	StateUpdate  *su;
	BonoboUISync *sync;
	
	g_return_if_fail (node != NULL);

	sync = find_sync_for_node (engine, node);
	g_return_if_fail (sync != NULL);

	su = state_update_new (
		sync, widget, 
		cmd_node != NULL ? cmd_node : node);

	if (su)
		engine->priv->state_updates = g_slist_prepend (
			engine->priv->state_updates, su);
}      

GtkWidget *
bonobo_ui_engine_build_control (BonoboUIEngine *engine,
				BonoboUINode   *node)
{
	GtkWidget *control = NULL;
	NodeInfo  *info = bonobo_ui_xml_get_data (
		engine->priv->tree, node);

/*	fprintf (stderr, "Control '%p', type '%d' object '%p'\n",
	info->widget, info->type, info->object);*/

	if (info->widget) {
		control = info->widget;
		g_assert (info->widget->parent == NULL);

	} else if (info->object != CORBA_OBJECT_NIL) {

		control = bonobo_widget_new_control_from_objref
			(bonobo_object_dup_ref (info->object, NULL),
			 CORBA_OBJECT_NIL);
		g_return_val_if_fail (control != NULL, NULL);
		
		info->type |= CUSTOM_WIDGET;
	}

	bonobo_ui_sync_do_show_hide (NULL, node, NULL, control);

/*	fprintf (stderr, "Type on '%s' '%s' is %d widget %p\n",
		 bonobo_ui_node_get_name (node),
		 bonobo_ui_node_get_attr (node, "name"),
		 info->type, info->widget);*/

	return control;
}

/* Node info accessors */

void
bonobo_ui_engine_stamp_custom (BonoboUIEngine *engine,
			       BonoboUINode   *node)
{
	NodeInfo *info;
	
	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	info->type |= CUSTOM_WIDGET;
}

CORBA_Object
bonobo_ui_engine_node_get_object (BonoboUIEngine   *engine,
				  BonoboUINode     *node)
{
	NodeInfo *info;
	
	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	return info->object;
}

gboolean
bonobo_ui_engine_node_is_dirty (BonoboUIEngine *engine,
				BonoboUINode   *node)
{
	BonoboUIXmlData *data;
	
	data = bonobo_ui_xml_get_data (engine->priv->tree, node);

	return data->dirty;
}

const char *
bonobo_ui_engine_node_get_id (BonoboUIEngine *engine,
			      BonoboUINode   *node)
{
	BonoboUIXmlData *data;
	
	data = bonobo_ui_xml_get_data (engine->priv->tree, node);

	return data->id;
}

void
bonobo_ui_engine_node_set_dirty (BonoboUIEngine *engine,
				 BonoboUINode   *node,
				 gboolean        dirty)
{
	BonoboUIXmlData *data;
	
	data = bonobo_ui_xml_get_data (engine->priv->tree, node);

	data->dirty = dirty;
}

GtkWidget *
bonobo_ui_engine_node_get_widget (BonoboUIEngine   *engine,
				  BonoboUINode     *node)
{
	NodeInfo *info;
	
	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	return info->widget;
}


/* Helpers */

char *
bonobo_ui_engine_get_attr (BonoboUINode *node,
			   BonoboUINode *cmd_node,
			   const char   *attr)
{
	char *txt;

	if ((txt = bonobo_ui_node_get_attr (node, attr)))
		return txt;

	if (cmd_node && (txt = bonobo_ui_node_get_attr (cmd_node, attr)))
		return txt;

	return NULL;
}

void
bonobo_ui_engine_add_hint (BonoboUIEngine   *engine,
			   const char       *str)
{
	gtk_signal_emit (GTK_OBJECT (engine),
			 signals [ADD_HINT], str);
}

void
bonobo_ui_engine_remove_hint (BonoboUIEngine *engine)
{
	gtk_signal_emit (GTK_OBJECT (engine),
			 signals [REMOVE_HINT]);
}

void
bonobo_ui_engine_emit_verb_on (BonoboUIEngine   *engine,
			       BonoboUINode     *node)
{
	gtk_signal_emit (GTK_OBJECT (engine),
			 signals [EMIT_VERB_ON], node);
}

void
bonobo_ui_engine_emit_event_on (BonoboUIEngine   *engine,
				BonoboUINode     *node,
				const char       *state)
{
	gtk_signal_emit (GTK_OBJECT (engine),
			 signals [EMIT_EVENT_ON],
			 node, state);
}

#define WIDGET_NODE_KEY "BonoboUIEngine:NodeKey"

BonoboUINode *
bonobo_ui_engine_widget_get_node (GtkWidget *widget)
{
	g_return_val_if_fail (widget != NULL, NULL);
	
	return gtk_object_get_data (GTK_OBJECT (widget),
				    WIDGET_NODE_KEY);
}

void
bonobo_ui_engine_widget_attach_node (GtkWidget    *widget,
				     BonoboUINode *node)
{
	if (widget)
		gtk_object_set_data (GTK_OBJECT (widget),
				     WIDGET_NODE_KEY, node);
}

void
bonobo_ui_engine_widget_set_node (BonoboUIEngine *engine,
				  GtkWidget      *widget,
				  BonoboUINode   *node)
{
	BonoboUISync *sync;

	if (!widget)
		return;

	sync = find_sync_for_node (engine, node);

	sync_widget_set_node (sync, widget, node);
}

BonoboUINode *
bonobo_ui_engine_get_cmd_node (BonoboUIEngine *engine,
			       BonoboUINode   *from_node)
{
	char         *path;
	BonoboUINode *ret;
	char         *cmd_name;

	g_return_val_if_fail (engine != NULL, NULL);

	if (!from_node)
		return NULL;

	if (!(cmd_name = node_get_id (from_node)))
		return NULL;

	path = g_strconcat ("/commands/", cmd_name, NULL);
	ret  = bonobo_ui_xml_get_path (engine->priv->tree, path);

	if (!ret) {
		BonoboUIXmlData *data_from;
		BonoboUINode *commands;
		BonoboUINode *node;

		commands = bonobo_ui_node_new ("commands");
		node     = bonobo_ui_node_new_child (commands, "cmd");

		bonobo_ui_node_set_attr (node, "name", cmd_name);

		data_from   = bonobo_ui_xml_get_data (
			engine->priv->tree, from_node);

		bonobo_ui_xml_merge (
			engine->priv->tree, "/",
			commands, data_from->id);
		
		ret = bonobo_ui_xml_get_path (engine->priv->tree, path);
		g_assert (ret != NULL);
	}

	g_free (path);
	g_free (cmd_name);

	return ret;
}

void
bonobo_ui_engine_emit_verb_on_w (BonoboUIEngine *engine,
				 GtkWidget      *widget)
{
	BonoboUINode *node = bonobo_ui_engine_widget_get_node (widget);

	gtk_signal_emit (GTK_OBJECT (engine),
			 signals [EMIT_VERB_ON], node);
}

void
bonobo_ui_engine_emit_event_on_w (BonoboUIEngine *engine,
				  GtkWidget      *widget,
				  const char     *state)
{
	BonoboUINode *node = bonobo_ui_engine_widget_get_node (widget);

	gtk_signal_emit (GTK_OBJECT (engine),
			 signals [EMIT_EVENT_ON], node, state);
}

void
bonobo_ui_engine_stamp_root (BonoboUIEngine *engine,
			     BonoboUINode   *node,
			     GtkWidget      *widget)
{
	NodeInfo *info;

	if (!node)
		return;

	info = bonobo_ui_xml_get_data (engine->priv->tree, node);

	info->widget = widget;
	info->type |= ROOT_WIDGET;

	bonobo_ui_engine_widget_attach_node (widget, node);
}

BonoboUINode *
bonobo_ui_engine_get_path (BonoboUIEngine *engine,
			   const char     *path)
{
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	return bonobo_ui_xml_get_path (engine->priv->tree, path);
}

void
bonobo_ui_engine_freeze (BonoboUIEngine *engine)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	engine->priv->frozen++;
}

void
bonobo_ui_engine_thaw (BonoboUIEngine *engine)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));
	
	if (--engine->priv->frozen <= 0) {
		bonobo_ui_engine_update (engine);
		engine->priv->frozen = 0;
	}
}

void
bonobo_ui_engine_dump (BonoboUIEngine *engine,
		       FILE           *out,
		       const char     *msg)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	fprintf (out, "Bonobo UI Engine : frozen '%d'\n",
		 engine->priv->frozen);

	sub_components_dump (engine, out);

	/* FIXME: propagate the FILE * */
	bonobo_ui_xml_dump (engine->priv->tree,
			    engine->priv->tree->root, msg);
}

void
bonobo_ui_engine_dirty_tree (BonoboUIEngine *engine,
			     BonoboUINode   *node)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

#ifdef WIDGET_SYNC_DEBUG
	fprintf (stderr, "Dirty tree '%s'",
		 bonobo_ui_xml_make_path (node));
#endif
	if (node) {
		bonobo_ui_xml_set_dirty (engine->priv->tree, node);

		bonobo_ui_engine_update (engine);
	}
}

void
bonobo_ui_engine_clean_tree (BonoboUIEngine *engine,
			     BonoboUINode   *node)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	if (node)
		bonobo_ui_xml_clean (engine->priv->tree, node);
}

void
bonobo_ui_engine_set_config_path (BonoboUIEngine *engine,
				  const char     *path)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (engine));

	g_free (engine->priv->config_path);
	engine->priv->config_path = g_strdup (path);
}

const char *
bonobo_ui_engine_get_config_path (BonoboUIEngine *engine)
{
	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);
	
	return engine->priv->config_path;
}