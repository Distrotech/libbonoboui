#include "config.h"
#include "bonobo-ui-xml.h"

#undef BONOBO_UI_XML_DUMP

static GtkObjectClass *bonobo_ui_xml_parent_class;

enum {
	OVERRIDE,
	REINSTATE,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

gpointer
bonobo_ui_xml_get_data (BonoboUIXml *tree, xmlNode *node)
{
	if (!node->_private) {
		if (tree->data_new)
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

static void
free_nodedata (BonoboUIXml *tree, BonoboUIXmlData *data)
{
	if (data) {
		if (data->overridden)
			/*
			 *  This indicates a serious error in the
			 * overriding logic.
			 */
			g_warning ("Leaking overridden nodes");

		if (tree->data_free)
			tree->data_free (data);
		else
			g_free (data);
	}
}

static void
node_free (BonoboUIXml *tree, xmlNode *node)
{
	free_nodedata (tree, node->_private);
	xmlUnlinkNode (node);
	xmlFreeNode   (node);
}

static void
free_nodedata_tree (BonoboUIXml *tree, xmlNode *node)
{
	xmlNode *l;

	if (node == NULL)
		return;

	free_nodedata (tree, node->_private);

	for (l = node->childs; l; l = l->next)
		free_nodedata_tree (tree, l);
}

static void
set_id (BonoboUIXml *tree, xmlNode *node, gpointer id)
{
	BonoboUIXmlData *data =
		bonobo_ui_xml_get_data (tree, node);

	data->id = id;

	for (node = node->childs; node; node = node->next)
		set_id (tree, node, id);
}

void
bonobo_ui_xml_dump (xmlNode *node, const char *descr)
{
#ifdef BONOBO_UI_XML_DUMP	
	xmlDoc *doc;

	doc = xmlNewDoc ("1.0");
	doc->root = node;
	
	fprintf (stderr, "%s\n", descr);

	xmlDocDump (stderr, doc);

	doc->root = NULL;
	xmlFreeDoc (doc);
	fprintf (stderr, "---\n");
#endif
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
		/* FIXME: are we reversing node order here ? */
		xmlAddChild (to, l);
	}
}

static void merge (BonoboUIXml *tree, xmlNode *current, xmlNode *new);

static void
override_node_with (BonoboUIXml *tree, xmlNode *old, xmlNode *new)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, new);
	BonoboUIXmlData *old_data = bonobo_ui_xml_get_data (tree, old);

	gtk_signal_emit (GTK_OBJECT (tree), signals [OVERRIDE], old);

	data->overridden = g_slist_prepend (
		old_data->overridden, old);
	old_data->overridden = NULL;

	if (new->childs) {
		merge (tree, old, new->childs);
		move_children (old, new);
	}

	xmlReplaceNode (old, new);

	g_assert (old->childs == NULL);

	if (new->parent) {
		data = bonobo_ui_xml_get_data (tree, new->parent);
		data->dirty = TRUE;
	} else
		data->dirty = TRUE;
}

static void
reinstate_old_node (BonoboUIXml *tree, xmlNode *node)
{
	BonoboUIXmlData *data = bonobo_ui_xml_get_data (tree, node);
	BonoboUIXmlData *old_data;
	xmlNode  *old;

	g_return_if_fail (data != NULL);

 	/* Mark tree as dirty */
	if (node->parent) {
		BonoboUIXmlData *parent_data;

		parent_data = bonobo_ui_xml_get_data (tree, node->parent);
		parent_data->dirty = TRUE;
	} else
		data->dirty = TRUE;

	if (data->overridden) { /* Something to re-instate */
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

		gtk_signal_emit (GTK_OBJECT (tree), signals [REINSTATE], old);
	} else {
/*		fprintf (stderr, "destroying node '%s' '%s'\n",
		node->name, xmlGetProp (node, "name"));*/

		/* Do we want to do this ? */
		xmlUnlinkNode (node);
	}

	/* Destroy the old node */
	free_nodedata_tree (tree, node);
	xmlFreeNode (node);
}

static xmlNode *
find_child (xmlNode *node, const char *name)
{
	xmlNode *l;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	for (l = node->childs; l; l = l->next)
		if (!strcmp (l->name, name))
			break;

	return l;
}

static xmlNode *
find_sibling (xmlNode *node, const char *name)
{
	xmlNode *l;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	for (l = node; l; l = l->next)
		if (xmlGetProp (l, "name") &&
		    !strcmp (xmlGetProp (l, "name"), name))
			break;
	
	return l;
}

static xmlNode *
xml_get_path (BonoboUIXml *tree, const char *path)
{
	xmlNode *ret;

	g_return_val_if_fail (tree != NULL, NULL);

/*	fprintf (stderr, "Find path '%s'\n", path);*/

	if (!path || path [0] == '\0')
		ret = tree->root;
	else {
		char   **names;
		int      i;
		xmlNode *next;

		if (path [0] != '/')
			g_warning ("non-absolute path brokenness '%s'", path);

		names = g_strsplit (path, "/", -1);

		ret = tree->root;
		for (i = 0; names && names [i]; i++) {
			if (names [i] [0] == '\0')
				continue;
			
			if (names [i] [0] == '#') {
				if ((next = find_sibling (ret, &names [i] [1])))
					ret = next;
				else
					xmlSetProp (ret, "name", &names [i] [1]);
			} else {
				if ((next = find_child (ret, names [i])))
					ret = next;
				else /* Create the node */
					ret = xmlNewChild (ret, NULL, names [i], NULL);
			}
		}
		
		g_strfreev (names);
	}

	return ret;
}

char *
bonobo_ui_xml_get_path  (xmlNode *node)
{
	GString *path;
	char    *tmp;

	g_return_val_if_fail (node != NULL, NULL);

	path = g_string_new ("");
	while (node) {

		if ((tmp = xmlGetProp (node, "name"))) {
			g_string_prepend (path, tmp);
			g_string_prepend (path, "/#");
		}

		if (node->parent) {
			g_string_prepend   (path, node->name);
			g_string_prepend_c (path, '/');
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
		gboolean         same;
				
		next = l->next;
		o_data = bonobo_ui_xml_get_data (tree, l->data);

		if (tree->compare)
			same = tree->compare (o_data->id, id);
		else
			same = (o_data->id == id);

		if (same) {
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

	if (data->id == id)
		reinstate_old_node (tree, node);
	else
		prune_overrides_by_id (tree, data, id);
}

/**
 * merge:
 * @current: the parent node
 * @new: an xml fragment
 * 
 * Merges two xml trees overriding where treeropriate.
 * new and its siblings get merged into current's children
 **/
static void
merge (BonoboUIXml *tree, xmlNode *current, xmlNode *new)
{
	xmlNode *a, *b, *nexta, *nextb;

	for (a = current->childs; a; a = nexta) {
		nexta = a->next;
		nextb = NULL;

		for (b = new; b; b = nextb) {
			xmlChar *a_name, *b_name;

			nextb = b->next;

/*			printf ("'%s' '%s' with '%s' '%s'\n",
				a->name, xmlGetProp (a, "name"),
				b->name, xmlGetProp (b, "name"));*/
			
			if (strcmp (a->name, b->name))
				continue;

			a_name = xmlGetProp (a, "name");
			b_name = xmlGetProp (b, "name");
			if (!a_name && !b_name)
				break;

			if (!a_name || !b_name)
				continue;

			if (!strcmp (a_name, b_name))
				break;
		}

		if (b == new)
			new = nextb;

		if (b) /* Merger candidate */
			override_node_with (tree, a, b);
	}

	for (b = new; b; b = nextb) {
		BonoboUIXmlData *data;

		nextb = b->next;
		
		fprintf (stderr, "Transfering '%s' '%s' into '%s' '%s'\n",
			 b->name, xmlGetProp (b, "name"),
			 current->name, xmlGetProp (current, "name"));

		xmlUnlinkNode (b);
		xmlAddChild (current, b);

		bonobo_ui_xml_dump (current, "After transfer");

		data = bonobo_ui_xml_get_data (tree, current);
		data->dirty = TRUE;
	}

	bonobo_ui_xml_dump (current, "After all");
}

void
bonobo_ui_xml_merge (BonoboUIXml *tree,
		     const char      *path,
		     const char      *xml,
		     gpointer         id)
{
	xmlNode *current;
	xmlDoc  *doc;

	g_return_if_fail (xml != NULL);

	doc = xmlParseDoc ((char *)xml);

	set_id (tree, doc->root, id);

	current = xml_get_path (tree, path);

/*	fprintf (stderr, "PATH: '%s' '%s\n", current->name,
	xmlGetProp (current, "name"));*/

	merge (tree, current, doc->root);

	doc->root = NULL;
	xmlFreeDoc (doc);
	
/*	bonobo_ui_xml_dump (tree->root, "Merged to");*/
}

void
bonobo_ui_xml_rm (BonoboUIXml *tree,
		  const char      *path,
		  gpointer         id)
{
	xmlNode *current;

	current = xml_get_path (tree, path);

	if (current)
		reinstate_node (tree, current, id);
}

static void
bonobo_ui_xml_destroy (GtkObject *object)
{
	BonoboUIXml *tree = BONOBO_UI_XML (object);

	if (tree) {
		tree->compare = NULL;

		if (tree->root)
			xmlFreeNode (tree->root);
		g_warning ("Leaking the Data elements in the tree");
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
bonobo_ui_xml_new (BonoboUIXmlCompareFn  compare,
		   BonoboUIXmlDataNewFn  data_new,
		   BonoboUIXmlDataFreeFn data_free)
{
	BonoboUIXml *tree;

	tree= gtk_type_new (BONOBO_UI_XML_TYPE);

	tree->compare = compare;
	tree->data_new = data_new;
	tree->data_free = data_free;

	tree->root = xmlNewNode (NULL, "Root");

	return tree;
}
