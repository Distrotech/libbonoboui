/*
 * bonobo-ui-xml.c: A module for merging, overlaying and de-merging XML 
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include "config.h"
#include <bonobo/bonobo-ui-xml.h>

#undef UI_XML_DEBUG
#undef BONOBO_UI_XML_DUMP

#ifdef BONOBO_UI_XML_DUMP
#	define DUMP_XML(a,b,c) (bonobo_ui_xml_dump ((a), (b), (c)))
#else
#	define DUMP_XML(a,b,c)
#endif

#define XML_FREE(a) (a?xmlFree(a):a)

static GtkObjectClass *bonobo_ui_xml_parent_class;

enum {
	OVERRIDE,
	REINSTATE,
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

static void
do_strip (xmlNode *node)
{
	xmlNode *l, *next;
	xmlAttr *a;

	if (!node)
		return;

	if (node->type == XML_COMMENT_NODE) {
		xmlUnlinkNode (node);
		xmlFreeNode   (node);
		return;
	}

	node->ns = NULL;
	node->doc = NULL;
	node->nsDef = NULL;

	for (a = node->properties; a; a = a->next)
		a->ns = NULL;

	for (l = node->childs; l; l = next) {
		next = l->next;
		do_strip (l);
	}
}

void
bonobo_ui_xml_strip (xmlNode *node)
{
	for (; node; node = node->next)
		do_strip (node);
}

gpointer
bonobo_ui_xml_get_data (BonoboUIXml *tree, xmlNode *node)
{
	if (!node->_private) {
		if (tree && tree->data_new)
			node->_private = tree->data_new ();
		else
			node->_private = g_new0 (BonoboUIXmlData, 1);
	}

	return node->_private;
}

void
bonobo_ui_xml_set_dirty (BonoboUIXml *tree,
			 xmlNode     *node,
			 gboolean     dirty)
{
	BonoboUIXmlData *data;
	xmlNode         *l;

	data = bonobo_ui_xml_get_data (tree, node);
	data->dirty = dirty;

	for (l = node->childs; l; l = l->next)
		bonobo_ui_xml_set_dirty (tree, l, dirty);
}

static void node_free (BonoboUIXml *tree, xmlNode *node);

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
node_free (BonoboUIXml *tree, xmlNode *node)
{
	free_nodedata (tree, node->_private, FALSE);
	xmlUnlinkNode (node);
	xmlFreeNode   (node);
}

static void
free_nodedata_tree (BonoboUIXml *tree, xmlNode *node, gboolean do_overrides)
{
	xmlNode *l;

	if (node == NULL)
		return;

	free_nodedata (tree, node->_private, do_overrides);

	for (l = node->childs; l; l = l->next)
		free_nodedata_tree (tree, l, do_overrides);
}

static void
do_set_id (BonoboUIXml *tree, xmlNode *node, gpointer id)
{
	BonoboUIXmlData *data =
		bonobo_ui_xml_get_data (tree, node);

	data->id = id;

	/* Do some basic validation here ? */
	{
		char *p, *name;
		
		if ((name = xmlGetProp (node, "name"))) {
			/*
			 *  The consequences of this are so unthinkable
			 * an assertion is warrented.
			 */
			for (p = name; *p; p++)
				g_assert (*p != '/' && *p != '#');

			xmlFree (name);
		}
	}

	for (node = node->childs; node; node = node->next)
		do_set_id (tree, node, id);
}

static void
set_id (BonoboUIXml *tree, xmlNode *node, gpointer id)
{
	for (; node; node = node->next)
		do_set_id (tree, node, id);
}

static void
dump_internals (BonoboUIXml *tree, xmlNode *node)
{
	int i;
	char *txt;
	xmlNode *l;
	static int indent = -4;
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, node);

	indent += 2;

	for (i = 0; i < indent; i++)
		fprintf (stderr, " ");

	fprintf (stderr, "%s name=\"%s\" ", node->name,
		 (txt = xmlGetProp (node, "name")));
	if (txt)
		xmlFree (txt);
	fprintf (stderr, "%d len %d", data->dirty,
		 g_slist_length (data->overridden));
	if (tree->dump)
		tree->dump (data);
	else
		fprintf (stderr, "\n");

	if (data->overridden) {
		GSList *l;
		int     old_indent;
		old_indent = indent;
		for (l = data->overridden; l; l = l->next) {
			for (i = 0; i < indent; i++)
				fprintf (stderr, " ");
			fprintf (stderr, "`--->");
			dump_internals (tree, l->data);
			indent += 4;
		}
		indent = old_indent;
	}

	for (l = node->childs; l; l = l->next)
		dump_internals (tree, l);

	indent -= 2;
}

void
bonobo_ui_xml_dump (BonoboUIXml *tree, xmlNode *node, const char *descr)
{
	xmlDoc *doc;

	doc = xmlNewDoc ("1.0");
	doc->root = node;
	
	fprintf (stderr, "%s\n", descr);

	xmlDocDump (stderr, doc);

	doc->root = NULL;
	xmlFreeDoc (doc);
	fprintf (stderr, "--- Internals ---\n");
	dump_internals (tree, node);
	fprintf (stderr, "---\n");
}

/*
 * Re-parenting should be in libxml but isn't
 */
static void
move_children (xmlNode *from, xmlNode *to)
{
	xmlNode *l, *next;

	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);
	g_return_if_fail (to->childs == NULL);

	for (l = from->childs; l; l = next) {
		next = l->next;
		
		xmlUnlinkNode (l);
		xmlAddChild (to, l);
	}

	g_assert (from->childs == NULL);
}

static void merge (BonoboUIXml *tree, xmlNode *current, xmlNode **new);

static void
override_node_with (BonoboUIXml *tree, xmlNode *old, xmlNode *new)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, new);
	BonoboUIXmlData *old_data = bonobo_ui_xml_get_data (tree, old);
	gboolean         same;

	same = identical (tree, data->id, old_data->id);

	if (!data->id) {
		same = TRUE;
		data->id = old_data->id;
	}

	if (!same) {
		gtk_signal_emit (GTK_OBJECT (tree), signals [OVERRIDE], old);

		data->overridden = g_slist_prepend (old_data->overridden, old);
	} else {
		data->overridden = old_data->overridden;
/*		g_warning ("Replace override of '%s' '%s'",
		old->name, xmlGetProp (old, "name"));*/
	}

	old_data->overridden = NULL;

	if (new->childs)
		merge (tree, old, &new->childs);

	move_children (old, new);

	xmlReplaceNode (old, new);

	g_assert (old->childs == NULL);

	if (!new->properties) /* A path simplifying entry */
		new->properties = xmlCopyPropList (new, old->properties);

	data->dirty = TRUE;
	if (new->parent) {
		data = bonobo_ui_xml_get_data (tree, new->parent);
		data->dirty = TRUE;
	}

	if (same)
		node_free (tree, old);
}

static void
reinstate_old_node (BonoboUIXml *tree, xmlNode *node)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, node);
	xmlNode  *old;

	g_return_if_fail (data != NULL);

 	/* Mark tree as dirty */
	if (data->overridden) { /* Something to re-instate */
		BonoboUIXmlData *old_data;

		g_return_if_fail (data->overridden->data != NULL);
		
		/* Get Old node from overridden list */
		old = data->overridden->data;
		old_data = bonobo_ui_xml_get_data (tree, old);

		/* Update Overridden list */
		old_data->overridden = g_slist_next (data->overridden);
		g_slist_free_1 (data->overridden);
		data->overridden = NULL;
		
		/* Move children across */
		move_children (node, old);
		
		/* Switch node back into tree */
		xmlReplaceNode (node, old);

		/* Mark dirty */
		old_data->dirty = TRUE;
		if (old->parent) {
			data = bonobo_ui_xml_get_data (tree, old->parent);
			data->dirty = TRUE;
		}

		gtk_signal_emit (GTK_OBJECT (tree), signals [REINSTATE], old);
	} else if (node->childs) { /* We need to leave the node here */
		/* Re-tag the node */
		BonoboUIXmlData *child_data = 
			bonobo_ui_xml_get_data (tree, node->childs);
		
		data->id = child_data->id;
		return;
	} else {
/*		fprintf (stderr, "destroying node '%s' '%s'\n",
		node->name, xmlGetProp (node, "name"));*/

		/* Mark dirty */
		if (node->parent) {
			data = bonobo_ui_xml_get_data (tree, node->parent);
			data->dirty = TRUE;
		}

		gtk_signal_emit (GTK_OBJECT (tree), signals [REMOVE], node);
		xmlUnlinkNode (node);
	}

	if (node == tree->root) /* Ugly special case */
		tree->root = NULL;

	/* Destroy the old node */
	node_free (tree, node);
}

static xmlNode *
find_child (xmlNode *node, const char *name)
{
	xmlNode *l, *ret = NULL;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	for (l = node->childs; l && !ret; l = l->next) {
		char *txt;

		if ((txt = xmlGetProp (l, "name"))) {
			if (!strcmp (txt, name))
				ret = l;

			xmlFree (txt);
		}

		if (!ret && !strcmp (l->name, name))
			ret = l;
	}

	return ret;
}

/*
 *  This monumental waste of time chewed 2 hours of my life
 * and was to try and help Eazel not have to change their
 * code at all; These routines worked fine, the compat ones
 * were duff.
 */
/*
char *
bonobo_ui_xml_path_escape (const char *path)
{
	char *ret, *dest;
	int   len = 0;
	const char *p;

	for (p = path; p && *p; p++) {
		if (*p == '/')
			len++;
		len++;
	}
	
	dest = ret = g_malloc (len + 1);

	for (p = path; p && *p; p++) {
		if (*p == '/' ||
		    *p == '\\')
			*dest++ = '\\';
		*dest++ = *p;
	}
	dest [len] = '\0';

	return ret;
}

char *
bonobo_ui_xml_path_unescape (const char *path)
{
	char *ret, *dest;
	const char *p;
	
	dest = ret = g_malloc (strlen (path) + 1);

	for (p = path; p && *p; p++) {
		if (*p == '\\')
			p++;
		*dest++ = *p;
	}
	*dest = '\0';
	
	return ret;
}
*/

static char **
bonobo_ui_xml_path_split (const char *path)
{
	return g_strsplit (path, "/", -1);
/*	GSList *string_list = NULL, *l;
	char *chunk, **ret;
	int   i, chunks;
	const char *p;

	g_return_val_if_fail (path != NULL, NULL);

	chunk = g_malloc (strlen (path) + 2);
	p = path;

	string_list = g_slist_prepend (string_list, chunk);
	chunks = 1;
	for (i = 0; p && *p; i++) {

		if (*p == '\\') {
			p++;
			chunk [i] = *p++;
		} else if (*p == '/') {
			chunk [i] = '\0';
			p++;
			if (*p != '\0') {
				string_list = g_slist_prepend (
					string_list, &chunk [i] + 1);
				chunks++;
			}
		} else
			chunk [i] = *p++;
	}
	chunk [i] = '\0';
	g_assert (i < strlen (path) + 2);

	ret = g_new (char *, chunks + 1);
	ret [chunks] = NULL;

	l = string_list;
	for (i = chunks - 1; i >= 0; i--) {
		ret [i] = l->data;
		l = l->next;
	}
	g_assert (l == NULL);
	g_slist_free (string_list);
	g_assert (ret [0] == chunk);

	fprintf (stderr, "Split path '%s' to:\n", path);
	for (i = 0; ret [i]; i++)
		fprintf (stderr, "> %s\n", ret [i]);

		return ret;*/
}

static void
bonobo_ui_xml_path_freev (char **split)
{
	g_strfreev (split);

/*	if (split)
		g_free (*split);

		g_free (split);*/
}

static xmlNode *
xml_get_path (BonoboUIXml *tree, const char *path)
{
	xmlNode *ret;
	char   **names;
	int      i;
	
	g_return_val_if_fail (tree != NULL, NULL);

#ifdef UI_XML_DEBUG
	fprintf (stderr, "Find path '%s'\n", path);
#endif
	DUMP_XML (tree, tree->root, "Before find path");

	if (!path || path [0] == '\0')
		return tree->root;

	if (path [0] != '/')
		g_warning ("non-absolute path brokenness '%s'", path);

	names = bonobo_ui_xml_path_split (path);

	ret = tree->root;
	for (i = 0; names && names [i]; i++) {
		if (names [i] [0] == '\0')
			continue;

/*		g_warning ("Path element '%s'", names [i]);*/

		if (!(ret = find_child (ret, names [i]))) {
			bonobo_ui_xml_path_freev (names);
			return NULL;
		}
	}
		
	bonobo_ui_xml_path_freev (names);

	DUMP_XML (tree, tree->root, "After clean find path");

	return ret;
}

xmlNode *
bonobo_ui_xml_get_path (BonoboUIXml *tree, const char *path)
{
	return xml_get_path (tree, path);
}

char *
bonobo_ui_xml_make_path  (xmlNode *node)
{
	GString *path;
	char    *tmp;

	g_return_val_if_fail (node != NULL, NULL);

	path = g_string_new ("");
	while (node) {

		if ((tmp = xmlGetProp (node, "name"))) {
			g_string_prepend (path, tmp);
			g_string_prepend (path, "/");
			xmlFree (tmp);
		}

		node = node->parent;
	}

	tmp = path->str;
	g_string_free (path, FALSE);

/*	fprintf (stderr, "Make path: '%s'\n", tmp);*/
	return tmp;
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

static void
reinstate_node (BonoboUIXml *tree, xmlNode *node, gpointer id)
{
	BonoboUIXmlData *data;
	xmlNode *l, *next;
			
	for (l = node->childs; l; l = next) {
		next = l->next;
		reinstate_node (tree, l, id);
	}

	data = bonobo_ui_xml_get_data (tree, node);

	if (identical (tree, data->id, id))
		reinstate_old_node (tree, node);

	prune_overrides_by_id (tree, data, id);
}

/**
 * merge:
 * @current: the parent node
 * @new: an xml fragment
 * 
 * Merges two xml trees overriding where appropriate.
 * new and its siblings get merged into current's children
 **/
static void
merge (BonoboUIXml *tree, xmlNode *current, xmlNode **new)
{
	xmlNode *a, *b, *nexta, *nextb;

	for (a = current->childs; a; a = nexta) {
		xmlChar *a_name = NULL;
		xmlChar *b_name = NULL;
			
		nexta = a->next;
		nextb = NULL;

		for (b = *new; b; b = nextb) {
			nextb = b->next;

			XML_FREE (a_name);
			a_name = NULL;
			XML_FREE (b_name);
			b_name = NULL;

/*			printf ("'%s' '%s' with '%s' '%s'\n",
				a->name, xmlGetProp (a, "name"),
				b->name, xmlGetProp (b, "name"));*/
			
			a_name = xmlGetProp (a, "name");
			b_name = xmlGetProp (b, "name");

			if (!a_name && !b_name &&
			    !strcmp (a->name, b->name))
				break;

			if (!a_name || !b_name)
				continue;

			if (!strcmp (a_name, b_name))
				break;
		}
		XML_FREE (a_name);
		XML_FREE (b_name);

		if (b == *new)
			*new = nextb;

		if (b) /* Merger candidate */
			override_node_with (tree, a, b);
	}

	for (b = *new; b; b = nextb) {
		BonoboUIXmlData *data;

		nextb = b->next;
		
/*		fprintf (stderr, "Transfering '%s' '%s' into '%s' '%s'\n",
			 b->name, xmlGetProp (b, "name"),
			 current->name, xmlGetProp (current, "name"));*/

		xmlUnlinkNode (b);

		if (tree->add_node)
			tree->add_node (current, b);
		else
			xmlAddChild (current, b);

		data = bonobo_ui_xml_get_data (tree, b);
		data->dirty = TRUE;

		data = bonobo_ui_xml_get_data (tree, current);
		data->dirty = TRUE;

/*		DUMP_XML (tree, current, "After transfer");*/
	}

	*new = NULL;
/*	DUMP_XML (tree, current, "After all"); */
}

BonoboUIXmlError
bonobo_ui_xml_merge (BonoboUIXml *tree,
		     const char  *path,
		     xmlNode     *nodes,
		     gpointer     id)
{
	xmlNode *current;

	g_return_val_if_fail (BONOBO_IS_UI_XML (tree), BONOBO_UI_XML_BAD_PARAM);

	if (nodes == NULL)
		return BONOBO_UI_XML_OK;

	bonobo_ui_xml_strip (nodes);
	set_id (tree, nodes, id);

	current = bonobo_ui_xml_get_path (tree, path);
	if (!current) {
		/* FIXME: we leak nodes here */
		return BONOBO_UI_XML_INVALID_PATH;
	}

#ifdef UI_XML_DEBUG
	{
		char *txt;
		fprintf (stderr, "\n\n\nPATH: '%s' '%s\n", current->name,
			 (txt = xmlGetProp (current, "name")));
		XML_FREE (txt);
	}
#endif

	DUMP_XML (tree, tree->root, "Merging in");
	DUMP_XML (tree, nodes, "this load");

	/*
	 * FIXME: ok; so we need to create a peer to merge against here.
	 */

	merge (tree, current, &nodes);

#ifdef UI_XML_DEBUG
	bonobo_ui_xml_dump (tree, tree->root, "Merged to");
#endif

	return BONOBO_UI_XML_OK;
}

BonoboUIXmlError
bonobo_ui_xml_rm (BonoboUIXml *tree,
		  const char      *path,
		  gpointer         id)
{
	xmlNode *current;

	current = xml_get_path (tree, path);

	if (current)
		reinstate_node (tree, current, id);
	else
		return BONOBO_UI_XML_INVALID_PATH;

	DUMP_XML (tree, tree->root, "After remove");

	return BONOBO_UI_XML_OK;
}

static void
bonobo_ui_xml_destroy (GtkObject *object)
{
	BonoboUIXml *tree = BONOBO_UI_XML (object);

	if (tree) {
		if (tree->root) {
			free_nodedata_tree (tree, tree->root, TRUE);
			xmlFreeNode (tree->root);
			tree->root = NULL;
		}
	}
}

static void
bonobo_ui_xml_class_init (BonoboUIXmlClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;
	
	bonobo_ui_xml_parent_class = gtk_type_class (
		gtk_object_get_type ());

	object_class->destroy = bonobo_ui_xml_destroy;

	signals [OVERRIDE] = gtk_signal_new (
		"override", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboUIXmlClass, override),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	signals [REINSTATE] = gtk_signal_new (
		"reinstate", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboUIXmlClass, reinstate),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	signals [REMOVE] = gtk_signal_new (
		"remove", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboUIXmlClass, remove),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

/**
 * bonobo_cmd_model_get_type:
 *
 * Returns the GtkType for the BonoboCmdModel class.
 */
GtkType
bonobo_ui_xml_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboUIXml",
			sizeof (BonoboUIXml),
			sizeof (BonoboUIXmlClass),
			(GtkClassInitFunc) bonobo_ui_xml_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_object_get_type (), &info);
	}

	return type;
}

BonoboUIXml *
bonobo_ui_xml_new (BonoboUIXmlCompareFn   compare,
		   BonoboUIXmlDataNewFn   data_new,
		   BonoboUIXmlDataFreeFn  data_free,
		   BonoboUIXmlDumpFn      dump,
		   BonoboUIXmlAddNode     add_node)
{
	BonoboUIXml *tree;

	tree = gtk_type_new (BONOBO_UI_XML_TYPE);

	tree->compare = compare;
	tree->data_new = data_new;
	tree->data_free = data_free;
	tree->dump = dump;
	tree->add_node = add_node;

	tree->root = xmlNewNode (NULL, "Root");

	return tree;
}
