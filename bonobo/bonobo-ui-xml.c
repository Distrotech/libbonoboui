/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-ui-xml.c: A module for merging, overlaying and de-merging XML 
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */
#include "config.h"
#include <string.h>
#include <glib-object.h>
#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-marshal.h>
#include <bonobo/bonobo-ui-node-private.h>

#undef UI_XML_DEBUG
#undef BONOBO_UI_XML_DUMP

#ifdef BONOBO_UI_XML_DUMP
#	define DUMP_XML(a,b,c) (bonobo_ui_xml_dump ((a), (b), (c)))
#else
#	define DUMP_XML(a,b,c)
#endif

static GQuark pos_id = 0;
static GQuark name_id = 0;
static GQuark placeholder_id = 0;

static void watch_update  (BonoboUIXml  *tree,
			   BonoboUINode *node);
static void watch_destroy (gpointer      data);

static GObjectClass *parent_class = NULL;

enum {
	OVERRIDE,
	REPLACE_OVERRIDE,
	REINSTATE,
	RENAME,
	REMOVE,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

inline static gboolean
identical (BonoboUIXml *tree, gpointer a, gpointer b)
{
	gboolean val;

	if (tree->compare)
		val = tree->compare (a, b);
	else
		val = (a == b);

/*	fprintf (stderr, "Identical ? '%p' '%p' : %d\n", a, b, val);*/

	return val;
}

/**
 * bonobo_ui_xml_get_data:
 * @tree: the tree
 * @node: the node
 * 
 * This function gets the data pointer associated with @node
 * and if there is no data pointer, constructs it using a user supplied
 * callback.
 * 
 * Return value: a valid Data pointer - never NULL.
 **/
gpointer
bonobo_ui_xml_get_data (BonoboUIXml *tree, BonoboUINode *node)
{
	if (!bonobo_ui_node_get_data (node)) {
		if (tree && tree->data_new)
			bonobo_ui_node_set_data (node, tree->data_new ());
		else {
			g_warning ("Error: No tree, and no data on node; leaking");
			bonobo_ui_node_set_data (node, g_new0 (BonoboUIXmlData, 1));
		}
	}

	return bonobo_ui_node_get_data (node);
}

/**
 * bonobo_ui_xml_clean:
 * @tree: the tree
 * @node: the node
 * 
 * This function marks the entire @tree from @node down
 * ( all its child nodes ) as being clean.
 **/
void
bonobo_ui_xml_clean (BonoboUIXml  *tree,
		     BonoboUINode *node)
{
	BonoboUIXmlData *data;
	BonoboUINode    *l;

	data = bonobo_ui_xml_get_data (tree, node);
	data->dirty = FALSE;

	for (l = bonobo_ui_node_children (node); l;
	     l = bonobo_ui_node_next (l))
		bonobo_ui_xml_clean (tree, l);
}

static void
set_children_dirty (BonoboUIXml *tree, BonoboUINode *node)
{
	BonoboUINode *l;

	if (!node)
		return;

	for (l = bonobo_ui_node_children (node); l;
	     l = bonobo_ui_node_next (l)) {
		BonoboUIXmlData *data;

		data = bonobo_ui_xml_get_data (tree, l);
		data->dirty = TRUE;
		
		set_children_dirty (tree, l);
	}
}

/**
 * bonobo_ui_xml_set_dirty:
 * @tree: the tree
 * @node: the node
 * 
 * This function sets a node as being dirty, along with all
 * its children. However more than this, it also sets its parent
 * dirty, and bubbles this up while the parent is a placeholder,
 * so as to allow a re-generate to be forced for its real visible
 * parent.
 **/
void
bonobo_ui_xml_set_dirty (BonoboUIXml *tree, BonoboUINode *node)
{
	int i;
	BonoboUINode *l;

	l = node;
	for (i = 0; (i < 2) && l; i++) {
		BonoboUIXmlData *data;

		/*
		 * FIXME: the placeholder functionality is broken and should
		 * live in bonobo-window.c for cleanliness and never in this
		 * more generic code.
		 */
		if (l->name_id == placeholder_id)
			i--;

		data = bonobo_ui_xml_get_data (tree, l);
		data->dirty = TRUE;

		l = bonobo_ui_node_parent (l);
	}

	/* Too conservative in some cases.*/
	set_children_dirty (tree, node);
}

/**
 * bonobo_ui_xml_get_parent_path:
 * @path: the path
 * 
 * This function lops one level off a path, much
 * like appending '..' to a Unix directory path.
 * 
 * Return value: the parent's path, use g_free to release it
 **/
char *
bonobo_ui_xml_get_parent_path (const char *path)
{
	const char *p;
	char *ret;

	if ((p = strrchr (path, '/')))
		ret = g_strndup (path, p - path);
	else
		ret = g_strdup (path);

	return ret;
}

static void node_free (BonoboUIXml *tree, BonoboUINode *node);

static void
free_nodedata (BonoboUIXml *tree, BonoboUIXmlData *data,
	       gboolean do_overrides)
{
	if (data) {
		if (data->overridden) {
			if (do_overrides) {
				GSList *l;

				for (l = data->overridden; l; l = l->next)
					node_free (tree, l->data);
				g_slist_free (data->overridden);
			} else 
				/*
				 *  This indicates a serious error in the
				 * overriding logic.
				 */
				g_warning ("Leaking overridden nodes");
		}

		if (tree->data_free)
			tree->data_free (data);
		else
			g_free (data);
	}
}

static void
free_nodedata_tree (BonoboUIXml *tree, BonoboUINode *node, gboolean do_overrides)
{
	BonoboUINode *l;

	if (node == NULL)
		return;

	free_nodedata (tree, bonobo_ui_node_get_data (node), do_overrides);
	bonobo_ui_node_set_data (node, NULL);

	for (l = bonobo_ui_node_children (node); l;
             l = bonobo_ui_node_next (l))
		free_nodedata_tree (tree, l, do_overrides);
}

static void
node_free (BonoboUIXml *tree, BonoboUINode *node)
{
	free_nodedata_tree (tree, node, TRUE);
	bonobo_ui_node_unlink (node);
	bonobo_ui_node_unref (node);
}

static void
do_set_id (BonoboUIXml *tree, BonoboUINode *node, gpointer id)
{
	BonoboUIXmlData *data;

	data = bonobo_ui_xml_get_data (tree, node);
	data->id = id;

#ifdef UI_XML_DEBUG
	/* Do some basic validation here ? */
	{
		const char *p, *name;
		
		if ((name = bonobo_ui_node_get_attr_by_id (node, name_id))) {
			for (p = name; *p; p++)
				g_assert (*p != '/' && *p != '#');
		}
	}
#endif

	for (node = node->children; node; node = node->next)
		do_set_id (tree, node, id);
}

static void
set_id (BonoboUIXml *tree, BonoboUINode *node, gpointer id)
{
	for (; node; node = node->next)
		do_set_id (tree, node, id);
}

static void
dump_internals (BonoboUIXml *tree, BonoboUINode *node)
{
	int i;
	BonoboUINode *l;
	const char *txt;
	static int indent = -4;
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, node);

	indent += 2;

	for (i = 0; i < indent; i++)
		fprintf (stderr, " ");

	fprintf (stderr, "%16s name=\"%10s\" ", bonobo_ui_node_get_name (node),
		 (txt = bonobo_ui_node_peek_attr (node, "name")) ? txt : "NULL");

	fprintf (stderr, "%d len %u", data->dirty,
		 g_slist_length (data->overridden));
	if (tree->dump)
		tree->dump (tree, node);
	else
		fprintf (stderr, "\n");

	if (data->overridden) {
		GSList *list;
		int     old_indent;
		old_indent = indent;
		for (list = data->overridden; list; list = list->next) {
			for (i = 0; i < indent; i++)
				fprintf (stderr, " ");
			fprintf (stderr, "`--->");
			dump_internals (tree, list->data);
			indent += 4;
		}
		indent = old_indent;
	}

	for (l = bonobo_ui_node_children (node); l; l = bonobo_ui_node_next (l))
		dump_internals (tree, l);

	indent -= 2;
}

/**
 * bonobo_ui_xml_dump:
 * @tree: the tree node
 * @bnode: the base node to start dumping from
 * @descr: a description string to print.
 * 
 * This debug function dumps the contents of a BonoboUIXml tree
 * to stderr, it is used by BonoboUIEngine to provide some of the
 * builtin BonoboUIDump verb functionality.
 **/
void
bonobo_ui_xml_dump (BonoboUIXml  *tree,
		    BonoboUINode *node,
		    const char   *descr)
{
	gchar *str;

	str = bonobo_ui_node_to_string (node, TRUE);
	fprintf (stderr, "Dump '%s':\n%s\n", descr, str);
	g_free (str);

	fprintf (stderr, "--- Internals ---\n");
	dump_internals (tree, node);
	fprintf (stderr, "---\n");
}

static void
prune_overrides_by_id (BonoboUIXml *tree, BonoboUIXmlData *data, gpointer id)
{
	GSList *l, *next;

	for (l = data->overridden; l; l = next) {
		BonoboUIXmlData *o_data;
				
		next = l->next;
		o_data = bonobo_ui_xml_get_data (tree, l->data);

		if (identical (tree, o_data->id, id)) {
			node_free (tree, l->data);

			data->overridden =
				g_slist_remove_link (data->overridden, l);
			g_slist_free_1 (l);
		}
	}
}

static void merge (BonoboUIXml *tree, BonoboUINode *current, BonoboUINode **new);

static void
override_node_with (BonoboUIXml *tree, BonoboUINode *old, BonoboUINode *new)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, new);
	BonoboUIXmlData *old_data = bonobo_ui_xml_get_data (tree, old);
	gboolean         same, transparent, override;

	/* Is it just a path / grouping simplifying entry with no content ? */
	transparent = bonobo_ui_node_transparent (new);

	same = identical (tree, data->id, old_data->id);

	g_assert (data->id);

	override = !same && !transparent;

	if (override) {

		g_signal_emit (tree, signals [OVERRIDE], 0, new, old);

		data->overridden = g_slist_prepend (old_data->overridden, old);
		prune_overrides_by_id (tree, data, data->id);
	} else {
		if (transparent)
			data->id = old_data->id;

		data->overridden = old_data->overridden;
		g_signal_emit (tree, signals [REPLACE_OVERRIDE], 0, new, old);

/*		fprintf (stderr, "Replace override of '%s' '%s' with '%s' '%s'",
			 old->name, bonobo_ui_node_peek_attr (old, "name"),
			 new->name, bonobo_ui_node_peek_attr (new, "name"));*/
	}

	old_data->overridden = NULL;

 	if (bonobo_ui_node_children (new))
 		merge (tree, old, &new->children);

	bonobo_ui_node_move_children (old, new);

	bonobo_ui_node_replace (old, new);

	g_assert (bonobo_ui_node_children (old) == NULL);

	if (transparent)
		bonobo_ui_node_copy_attrs (old, new);

	bonobo_ui_xml_set_dirty (tree, new);

	if (!override)
		node_free (tree, old);

	watch_update (tree, new);
}

static void
reinstate_old_node (BonoboUIXml *tree, BonoboUINode *node)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, node);
	BonoboUINode  *old;

	g_return_if_fail (data != NULL);

	if (data->overridden) { /* Something to re-instate */
		BonoboUIXmlData *old_data;

		g_return_if_fail (data->overridden->data != NULL);

		/* Get Old node from overridden list */
		old = data->overridden->data;
		old_data = bonobo_ui_xml_get_data (tree, old);

/*		fprintf (stderr, "Reinstating override '%s' '%s' with '%s' '%s'",
			 node->name, xmlGetProp (node, "name"),
			 old->name, xmlGetProp (old, "name"));*/
		
		/* Update Overridden list */
		old_data->overridden = g_slist_next (data->overridden);
		g_slist_free_1 (data->overridden);
		data->overridden = NULL;

		/* Fire remove while still in tree */
		g_signal_emit (tree, signals [REMOVE], 0, node);
		
		/* Move children across */
		bonobo_ui_node_move_children (node, old);

		/* Switch node back into tree */
		bonobo_ui_node_replace (node, old);

		/* Mark dirty */
		bonobo_ui_xml_set_dirty (tree, old);

		g_signal_emit (tree, signals [REINSTATE], 0, old);

		watch_update (tree, old);

	} else if (bonobo_ui_node_children (node)) { /* We need to leave the node here */
		/* Re-tag the node */
		BonoboUIXmlData *child_data = 
			bonobo_ui_xml_get_data (tree, bonobo_ui_node_children (node));
		data->id = child_data->id;

		/* Mark dirty */
		bonobo_ui_xml_set_dirty (tree, node);
		
		g_signal_emit (tree, signals [RENAME], 0, node);
		return;
	} else {
		/* Mark parent & up dirty */
		bonobo_ui_xml_set_dirty (tree, node);

		g_signal_emit (tree, signals [REMOVE], 0, node);
	}

/*		fprintf (stderr, "destroying node '%s' '%s'\n",
		node->name, bonobo_ui_node_peek_attr (node, "name"));*/
			
	bonobo_ui_node_unlink (node);
	
	if (node == tree->root) /* Ugly special case */
		tree->root = NULL;

	/* Destroy the old node */
	node_free (tree, node);
}

static BonoboUINode *
xml_get_path (BonoboUIXml *tree, const char *path,
	      gboolean allow_wild, gboolean *wildcard)
{
	BonoboUINode *ret;
	char   **names, *copy;
	int      i, j, slashes;
	
	g_return_val_if_fail (tree != NULL, NULL);
	g_return_val_if_fail (!allow_wild || wildcard != NULL, NULL);

#ifdef UI_XML_DEBUG
	fprintf (stderr, "Find path '%s'\n", path);
#endif
/*	DUMP_XML (tree, tree->root, "Before find path"); */

	if (allow_wild)
		*wildcard = FALSE;
	if (!path || path [0] == '\0')
		return tree->root;

	if (path [0] != '/')
		g_warning ("non-absolute path brokenness '%s'", path);

	for (i = slashes = 0; path [i]; i++)
		if (path [i] == '/')
			slashes++;

	names = g_newa (char *, slashes + 2);
	names [0] = copy = g_alloca (i + 1);

	for (i = j = 0; path [i]; i++) {
		if (path [i] == '/') {
			copy [i] = '\0';
			names [++j] = &copy [i + 1];
		} else
			copy [i] = path [i];
	}
	copy [i] = '\0';
	names [++j] = NULL;

/*	names = bonobo_ui_xml_path_split (path); */

	ret = tree->root;
	for (i = 0; names && names [i]; i++) {
		if (names [i] [0] == '\0')
			continue;

/*		g_warning ("Path element '%s'", names [i]); */

		if (allow_wild &&
		    names [i] [0] == '*' &&
		    names [i] [1] == '\0')
			*wildcard = TRUE;

		else if (!(ret = bonobo_ui_node_get_path_child (ret, names [i]))) {
/*			bonobo_ui_xml_path_freev (names); */
			return NULL;
		}
	}
		
/*	bonobo_ui_xml_path_freev (names); */

/*	DUMP_XML (tree, tree->root, "After clean find path"); */

	return ret;
}

/**
 * bonobo_ui_xml_get_path:
 * @tree: the tree
 * @path: the path
 * 
 * This function returns the node at path @path inside the
 * internal XML tree.
 * 
 * Return value: a pointer to the node at @path
 **/
BonoboUINode *
bonobo_ui_xml_get_path (BonoboUIXml *tree, const char *path)
{
	return xml_get_path (tree, path, FALSE, NULL);
}

/**
 * bonobo_ui_xml_get_path_wildcard:
 * @tree: the tree
 * @path: the path
 * @wildcard: whether to allow '*' as a wildcard
 * 
 * This does a wildcard path match, the only
 * wildcard character is '*'. This is only really
 * used by the _rm command and the _exists functionality.
 * 
 * Return value: TRUE if the path matches.
 **/
BonoboUINode *
bonobo_ui_xml_get_path_wildcard (BonoboUIXml *tree, const char *path,
				 gboolean    *wildcard)
{
	return xml_get_path (tree, path, TRUE, wildcard);
}

/**
 * bonobo_ui_xml_make_path:
 * @node: the node.
 * 
 * This generates a path name for a node in a tree.
 * 
 * Return value: the path name, use g_free to release.
 **/
char *
bonobo_ui_xml_make_path (BonoboUINode *node)
{
	GString    *path;
	char       *ret;

	g_return_val_if_fail (node != NULL, NULL);

	path = g_string_new (NULL);
	while (node && node->parent) {
		const char *tmp;

		if ((tmp = bonobo_ui_node_get_attr_by_id (node, name_id))) {
			g_string_prepend (path, tmp);
			g_string_prepend (path, "/");
		} else {
			g_string_prepend (path, bonobo_ui_node_get_name (node));
			g_string_prepend (path, "/");
		}

		node = node->parent;
	}

	ret = path->str;
	g_string_free (path, FALSE);

/*	fprintf (stderr, "Make path: '%s'\n", tmp);*/
	return ret;
}

static void
reinstate_node (BonoboUIXml *tree, BonoboUINode *node,
		gpointer id, gboolean nail_me)
{
	BonoboUINode *l, *next;
			
	for (l = bonobo_ui_node_children (node); l; l = next) {
		next = bonobo_ui_node_next (l);
		reinstate_node (tree, l, id, TRUE);
	}

	if (nail_me) {
		BonoboUIXmlData *data;

		data = bonobo_ui_xml_get_data (tree, node);

		if (identical (tree, data->id, id))
			reinstate_old_node (tree, node);
		else
			prune_overrides_by_id (tree, data, id);
	}
}

static void
do_insert (BonoboUINode *parent,
	   BonoboUINode *child,
	   BonoboUINode *insert)
{
	const char *pos;
	gboolean    added = FALSE;

	if ((pos = bonobo_ui_node_get_attr_by_id (child, pos_id))) {
		if (pos [0] == 't') {
			bonobo_ui_node_insert_before (
				bonobo_ui_node_children (parent),
				child);
			added = TRUE;
		}
	}

	if (!added) {
		if (insert)
			bonobo_ui_node_insert_before (
				insert, child);
		else
			bonobo_ui_node_add_child (parent, child);
	}
}

static void
merge (BonoboUIXml *tree, BonoboUINode *current, BonoboUINode **new)
{
	BonoboUINode *a, *b, *nexta, *nextb, *insert = NULL;

	for (a = current->children; a; a = nexta) {
		BonoboUINode *result;
		const xmlChar *a_name;
		const xmlChar *b_name = NULL;
			
		nexta = a->next;
		nextb = NULL;

		a_name = bonobo_ui_node_get_attr_by_id (a, name_id);

		for (b = *new; b; b = nextb) {
			nextb = b->next;

			b_name = NULL;

/*			printf ("'%s' '%s' with '%s' '%s'\n",
				a->name, bonobo_ui_node_peek_attr (a, "name"),
				b->name, bonobo_ui_node_peek_attr (b, "name"));*/
			
			b_name = bonobo_ui_node_get_attr_by_id (b, name_id);

			if (!a_name && !b_name && a->name_id == b->name_id)
				break;

			if (!a_name || !b_name)
				continue;

			if (!strcmp (a_name, b_name))
				break;
		}

		if (b == *new)
			*new = nextb;

		if (b) /* Merger candidate */
			override_node_with (tree, a, b);

		result = b ? b : a;

		if (!insert && !a_name && result->name_id == placeholder_id)
			insert = result;
	}

	for (b = *new; b; b = nextb) {
		BonoboUIXmlData *data;
		
		nextb = b->next;
		
/*		fprintf (stderr, "Transfering '%s' '%s' into '%s' '%s'\n",
			 b->name, bonobo_ui_node_peek_attr (b, "name"),
			 current->name, bonobo_ui_node_peek_attr (current, "name"));*/

		bonobo_ui_node_unlink (b);

		do_insert (current, b, insert);

		if (tree->add_node)
			tree->add_node (current, b, tree->user_data);
		
		bonobo_ui_xml_set_dirty (tree, b);

		/* FIXME: this looks redundant */
		data = bonobo_ui_xml_get_data (tree, current);
		data->dirty = TRUE;

		watch_update (tree, b);

/*		DUMP_XML (tree, current, "After transfer");*/
	}

	*new = NULL;
/*	DUMP_XML (tree, current, "After all"); */
}

/**
 * bonobo_ui_xml_merge:
 * @tree: the tree
 * @path: the path to merge into
 * @nodes: the nodes
 * @id: the id to merge with.
 * 
 * Merges new xml nodes into the internal tree, overriding
 * where appropriate. Merging is by id, ie. overriding only
 * occurs where there is an id mismatch.
 * 
 * Return value: an error flag.
 **/
BonoboUIError
bonobo_ui_xml_merge (BonoboUIXml  *tree,
		     const char   *path,
		     BonoboUINode *nodes,
		     gpointer      id)
{
	BonoboUINode *current;

	g_return_val_if_fail (BONOBO_IS_UI_XML (tree), BONOBO_UI_ERROR_BAD_PARAM);

	if (nodes == NULL)
		return BONOBO_UI_ERROR_OK;

	current = bonobo_ui_xml_get_path (tree, path);
	if (!current) {
		BonoboUINode *l, *next;

		for (l = nodes; l; l = next) {
			next = l->next;
			node_free (tree, l);
		}

		return BONOBO_UI_ERROR_INVALID_PATH;
	}

	set_id (tree, nodes, id);

#ifdef UI_XML_DEBUG
	fprintf (stderr, "\n\n\nPATH: '%s' '%s\n",
		 g_quark_to_string (current->name_id),
		 bonobo_ui_node_peek_attr (current, "name"));
#endif

	DUMP_XML (tree, tree->root, "Merging in");
	DUMP_XML (tree, nodes, "this load");

	merge (tree, current, &nodes);

#ifdef UI_XML_DEBUG
	bonobo_ui_xml_dump (tree, tree->root, "Merged to");
#endif

	return BONOBO_UI_ERROR_OK;
}

BonoboUIError
bonobo_ui_xml_rm (BonoboUIXml *tree,
		  const char  *path,
		  gpointer     id)
{
	BonoboUINode *current;
	gboolean      wildcard;

	current = bonobo_ui_xml_get_path_wildcard (
		tree, path, &wildcard);

/*	fprintf (stderr, "remove stuff from '%s' (%d) -> '%p'\n",
	path, wildcard, current); */

	if (current)
		reinstate_node (tree, current, id, !wildcard);
	else
		return BONOBO_UI_ERROR_INVALID_PATH;

	DUMP_XML (tree, tree->root, "After remove");

	return BONOBO_UI_ERROR_OK;
}

static void
bonobo_ui_xml_dispose (GObject *object)
{
	GSList      *l;
	BonoboUIXml *tree = (BonoboUIXml *) object;

	for (l = tree->watches; l; l = l->next)
		watch_destroy (l->data);
	g_slist_free (tree->watches);
	tree->watches = NULL;

	parent_class->dispose (object);
}

static void
bonobo_ui_xml_finalize (GObject *object)
{
	BonoboUIXml *tree = (BonoboUIXml *) object;
	
	if (tree->root) {
		free_nodedata_tree (tree, tree->root, TRUE);
		bonobo_ui_node_unref (tree->root);
		tree->root = NULL;
	}

	parent_class->finalize (object);
}

static void
bonobo_ui_xml_class_init (BonoboUIXmlClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	
	parent_class = g_type_class_peek_parent (klass);

	pos_id         = g_quark_from_static_string ("pos");
 	name_id        = g_quark_from_static_string ("name");
 	placeholder_id = g_quark_from_static_string ("placeholder");

	object_class->dispose  = bonobo_ui_xml_dispose;
	object_class->finalize = bonobo_ui_xml_finalize;

	signals [OVERRIDE] =
		g_signal_new ("override",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (BonoboUIXmlClass, override),
			      NULL, NULL,
			      bonobo_ui_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2,
			      G_TYPE_POINTER, G_TYPE_POINTER);
	signals [REPLACE_OVERRIDE] =
		g_signal_new ("replace_override",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (BonoboUIXmlClass, replace_override),
			      NULL, NULL,
			      bonobo_ui_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2,
			      G_TYPE_POINTER, G_TYPE_POINTER);
	signals [REINSTATE] =
		g_signal_new ("reinstate",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (BonoboUIXmlClass, reinstate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1,
			      G_TYPE_POINTER);
	signals [RENAME] =
		g_signal_new ("rename",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (BonoboUIXmlClass, rename),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1,
			      G_TYPE_POINTER);
	signals [REMOVE] =
		g_signal_new ("remove",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (BonoboUIXmlClass, remove),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1,
			      G_TYPE_POINTER);
}

/**
 * bonobo_cmd_model_get_type:
 *
 * Returns the GType for the BonoboCmdModel class.
 */
GType
bonobo_ui_xml_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (BonoboUIXmlClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) bonobo_ui_xml_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (BonoboUIXml),
			0, /* n_preallocs */
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT, "BonoboUIXml",
					       &info, 0);
	}

	return type;
}

BonoboUIXml *
bonobo_ui_xml_new (BonoboUIXmlCompareFn   compare,
		   BonoboUIXmlDataNewFn   data_new,
		   BonoboUIXmlDataFreeFn  data_free,
		   BonoboUIXmlDumpFn      dump,
		   BonoboUIXmlAddNode     add_node,
		   gpointer               user_data)
{
	BonoboUIXml *tree;

	tree = g_object_new (BONOBO_TYPE_UI_XML, NULL);

	tree->compare = compare;
	tree->data_new = data_new;
	tree->data_free = data_free;
	tree->dump = dump;
	tree->add_node = add_node;
	tree->user_data = user_data;

	tree->root = bonobo_ui_node_new ("Root");

	tree->watches = NULL;

	return tree;
}

typedef struct {
	char    *path;
	gpointer user_data;
} Watch;

static void
watch_update (BonoboUIXml *tree, BonoboUINode *node)
{
	GSList *l;
	char   *path;

	if (!tree->watch)
		return;

	/* FIXME: for speed we only check root nodes for now */
	if (bonobo_ui_node_parent (node) !=
	    tree->root)
		return;

	path = bonobo_ui_xml_make_path (node);

	for (l = tree->watches; l; l = l->next) {
		Watch *w = l->data;

		if (!strcmp (w->path, path)) {
/*			fprintf (stderr, "Found watch on '%s'", path);*/
			tree->watch (tree, path, node, w->user_data);
		}
	}
	g_free (path);
}

void
bonobo_ui_xml_add_watch (BonoboUIXml  *tree,
			 const char   *path,
			 gpointer      user_data)
{
	Watch *w = g_new0 (Watch, 1);

	g_return_if_fail (BONOBO_IS_UI_XML (tree));

	w->path = g_strdup (path);
	w->user_data = user_data;

	tree->watches = g_slist_append (
		tree->watches, w);
}

void
bonobo_ui_xml_remove_watch_by_data (BonoboUIXml *tree,
				    gpointer     user_data)
{
	GSList *l;
	GSList *next;

	g_return_if_fail (BONOBO_IS_UI_XML (tree));

	for (l = tree->watches; l; l = next) {
		Watch *w = l->data;

		next = l->next;

		if (w->user_data == user_data) {
			tree->watches = g_slist_remove (
				tree->watches, w);
			watch_destroy (w);
		}
	}
}

static void
watch_destroy (gpointer data)
{
	Watch *w = data;
	
	if (w) {
		g_free (w->path);
		g_free (w);
	}
}

void
bonobo_ui_xml_set_watch_fn (BonoboUIXml       *tree,
			    BonoboUIXmlWatchFn watch)
{
	g_return_if_fail (BONOBO_IS_UI_XML (tree));

	tree->watch = watch;
}
