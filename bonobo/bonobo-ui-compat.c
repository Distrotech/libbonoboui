/*
 * bonobo-ui-compat.c: Compatibility functions for the old GnomeUI stuff,
 *                    and the old Bonobo UI handler API.
 *
 *  This module acts as an excercise in filthy coding habits
 * take a look around.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include <glib.h>
#include <unistd.h>
#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-container.h>
#include <bonobo/bonobo-ui-compat.h>

#undef COMPAT_DEBUG

#ifdef COMPAT_DEBUG

#define sloppy_check(c,a,ev)
#define sloppy_check_val(c,a,ev,val)

#else

#define sloppy_check(c,a,ev)							\
	G_STMT_START {								\
		if (!bonobo_ui_container_path_exists ((c), (a), (ev))) {	\
			g_free (a);						\
			return;							\
		}								\
	} G_STMT_END

#define sloppy_check_val(c,a,ev,val)						\
	G_STMT_START {								\
		if (!bonobo_ui_container_path_exists ((c), (a), (ev))) {	\
			g_free (a);						\
			return (val);						\
		}								\
	} G_STMT_END

#endif

typedef struct {
	BonoboUIComponent *component;
	BonoboWin         *application;

	char              *name;

	Bonobo_UIContainer container;
} BonoboUIHandlerPrivate;

static int busk_name = 0;

#define MAGIC_UI_HANDLER_KEY "Bonobo::CompatUIPrivKey"

static BonoboUIHandlerPrivate *
get_priv (BonoboUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);

	return gtk_object_get_data (GTK_OBJECT (uih), MAGIC_UI_HANDLER_KEY);
}

static void
compat_set (BonoboUIHandlerPrivate *priv,
	    const char *xml_path,
	    const char *xml)
{
#ifdef COMPAT_DEBUG
	fprintf (stderr, "Merge to '%s' : '%s'\n", xml_path, xml);
#endif
	if (priv && priv->component && priv->container != CORBA_OBJECT_NIL) {
		bonobo_ui_component_set (priv->component, priv->container,
					 xml_path, xml, NULL);
#ifdef COMPAT_DEBUG
		fprintf (stderr, "success\n");
	} else {
		fprintf (stderr, "failure\n");
#endif
	}
}

static void
compat_set_tree (BonoboUIHandlerPrivate *priv,
		 const char *xml_path,
		 xmlNode    *node)
{
#ifdef COMPAT_DEBUG
	{
		xmlDoc     *doc;
		xmlChar    *mem = NULL;
		int         size;

		doc = xmlNewDoc ("1.0");
		g_return_if_fail (doc != NULL);

		doc->root = node;

		xmlDocDumpMemory (doc, &mem, &size);

		g_return_if_fail (mem != NULL);

		doc->root = NULL;
		xmlFreeDoc (doc);

		fprintf (stderr, "Merge to '%s' : '%s'\n", xml_path, mem);

		xmlFree (mem);
	}
#endif

	if (priv && priv->component && priv->container != CORBA_OBJECT_NIL) {
		bonobo_ui_component_set_tree (priv->component, priv->container,
					      xml_path, node, NULL);
#ifdef COMPAT_DEBUG
		fprintf (stderr, "success\n");
	} else {
		fprintf (stderr, "failure\n");
#endif
	}
}

static void
compat_set_siblings (BonoboUIHandlerPrivate *priv,
		     const char *xml_path,
		     xmlNode    *node)
{
	xmlNode *l;

	if (priv && priv->component && priv->container != CORBA_OBJECT_NIL) {
		for (; node->prev; node = node->prev)
			;
		for (l = node; l; l = l->next) {
			xmlNode *copy = xmlCopyNode (l, TRUE);
			
			bonobo_ui_component_set_tree (
				priv->component, priv->container,
				xml_path, copy, NULL);
			
			xmlFreeNode (copy);
		}
	}
}

void
bonobo_ui_handler_set_container (BonoboUIHandler *uih,
				 Bonobo_Unknown   cont)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	priv->container = bonobo_object_dup_ref (cont, NULL);
}

void
bonobo_ui_handler_unset_container (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	if (priv->container != CORBA_OBJECT_NIL) {
		bonobo_ui_component_rm (priv->component, priv->container, "/", NULL);
		bonobo_object_release_unref (priv->container, NULL);
	}
	priv->container = CORBA_OBJECT_NIL;
}

void
bonobo_ui_handler_create_menubar (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	compat_set (priv, "/", "<menu/>");
}

void
bonobo_ui_handler_create_toolbar (BonoboUIHandler *uih, const char *name)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *xml;

	g_return_if_fail (priv != NULL);

	xml = g_strdup_printf ("<dockitem name=\"%s\"/>", name);
	compat_set (priv, "/", xml);
	g_free (xml);
}

static void
priv_destroy (GtkObject *object, BonoboUIHandlerPrivate *priv)
{
	if (priv->container)
		bonobo_ui_handler_unset_container ((BonoboUIHandler *)object);

	g_free (priv->name);
	g_free (priv);
}

static void
setup_priv (BonoboObject *object)
{
	BonoboUIHandlerPrivate *priv;

	priv = g_new0 (BonoboUIHandlerPrivate, 1);

	gtk_object_set_data (GTK_OBJECT (object), MAGIC_UI_HANDLER_KEY, priv);
	gtk_signal_connect  (GTK_OBJECT (object), "destroy",
			     (GtkSignalFunc) priv_destroy, priv);
}

/*
 * This constructs a new component client
 */
BonoboUIHandler *
bonobo_ui_handler_new (void)
{
	BonoboUIComponent *object;
	char              *name;
	BonoboUIHandlerPrivate *priv;

	name = g_strdup_printf ("Busk%d", (getpid () * 10) + busk_name++);
	object = bonobo_ui_component_new (name);

	g_free (name);

	setup_priv (BONOBO_OBJECT (object));

	priv = get_priv ((BonoboUIHandler *)object);
	g_return_val_if_fail (priv != NULL, NULL);

	priv->component = object;

	return (BonoboUIHandler *)object;
}

BonoboUIHandlerMenuItem *
bonobo_ui_compat_menu_item_new (GnomeUIInfo *uii, gpointer data,
				BonoboUICompatType type)
{
	BonoboUIHandlerMenuItem *item = g_new0 (BonoboUIHandlerMenuItem, 1);
	
	item->type = type;
	item->uii  = uii;
	item->data = data;

	return item;
}

/*
 * Lots of waste for bincompat
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_one (GnomeUIInfo *uii)
{
	return bonobo_ui_compat_menu_item_new (uii, NULL, BONOBO_UI_COMPAT_ONE);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_list (GnomeUIInfo *uii)
{
	return bonobo_ui_compat_menu_item_new (uii, NULL, BONOBO_UI_COMPAT_LIST);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_tree (GnomeUIInfo *uii)
{
	return bonobo_ui_compat_menu_item_new (uii, NULL, BONOBO_UI_COMPAT_TREE);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_one_with_data (GnomeUIInfo *uii, gpointer data)
{
	return bonobo_ui_compat_menu_item_new (uii, data, BONOBO_UI_COMPAT_ONE);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_list_with_data (GnomeUIInfo *uii, gpointer data)
{
	return bonobo_ui_compat_menu_item_new (uii, data, BONOBO_UI_COMPAT_LIST);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_tree_with_data (GnomeUIInfo *uii, gpointer data)
{
	return bonobo_ui_compat_menu_item_new (uii, data, BONOBO_UI_COMPAT_TREE);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_toolbar_parse_uiinfo_list (GnomeUIInfo *uii)
{
	return bonobo_ui_compat_menu_item_new (uii, NULL, BONOBO_UI_COMPAT_LIST);
}

BonoboUIHandlerMenuItem *
bonobo_ui_handler_toolbar_parse_uiinfo_list_with_data (GnomeUIInfo *uii, gpointer data)
{
	return bonobo_ui_compat_menu_item_new (uii, data, BONOBO_UI_COMPAT_LIST);
}

static void
add_accel (xmlNode *node,
	   guint key, GdkModifierType ac_mods)
{
	if (key) {
		char *name = bonobo_ui_util_accel_name (key, ac_mods);
		xmlSetProp (node, "accel", name);
		g_free (name);
	}
}		

typedef struct {
	BonoboUIHandlerCallback cb;
	gpointer                user_data;
	char                   *old_path;
	GDestroyNotify          destroy_fn;
} VerbClosure;

static void
verb_to_cb (BonoboUIComponent *component,
	    gpointer           user_data,
	    const char        *cname)
{
	VerbClosure *c = user_data;

	if (!c || !c->cb)
		return;

	/* Does anyone use the path field here ? */
	c->cb ((BonoboUIHandler *)component, c->user_data,
	       c->old_path);
}

static void
verb_free_closure (VerbClosure *c)
{
	if (c) {
		if (c->destroy_fn)
			c->destroy_fn (c->user_data);
		c->destroy_fn = NULL;
		g_free (c->old_path);
		g_free (c);
	}
}

static void
compat_add_verb (BonoboUIComponent *component, const char *verb,
		 BonoboUIHandlerCallback cb, gpointer user_data,
		 const char *old_path, GDestroyNotify notify_fn)
{
	VerbClosure *c = g_new (VerbClosure, 1);

	c->cb         = cb;
	c->user_data  = user_data;
	c->old_path   = g_strdup (old_path);
	c->destroy_fn = notify_fn;

	bonobo_ui_component_add_verb_full (
		component, verb, verb_to_cb, c,
		(GDestroyNotify) verb_free_closure);
}

static xmlNode *
compat_menu_parse_uiinfo_one_with_data (BonoboUIHandlerPrivate *priv, 
					 GnomeUIInfo            *uii,
					 void                   *data,
					 xmlNode                *parent)
{
	xmlNode *node;
	char    *verb;

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	switch (uii->type) {
	case GNOME_APP_UI_ENDOFINFO:
		return NULL;

	case GNOME_APP_UI_ITEM:
		node = xmlNewNode (NULL, "menuitem");
		break;

	case GNOME_APP_UI_TOGGLEITEM:
		node = xmlNewNode (NULL, "menuitem");
		xmlSetProp (node, "type", "toggle");
		break;

	case GNOME_APP_UI_RADIOITEMS:
	{
/*		GnomeUIInfo *info;
		for (info = uii->moreinfo; info && !info->type == GNOME_APP_UI_ENDOFINFO; info++) {
		}
 */
		g_warning ("FIXME: radioitems ...");

		return NULL;
	}

	case GNOME_APP_UI_SUBTREE:
	case GNOME_APP_UI_SUBTREE_STOCK:
		node = xmlNewNode (NULL, "submenu");
		break;

	case GNOME_APP_UI_SEPARATOR:
		node = xmlNewNode (NULL, "menuitem");
		break;

	case GNOME_APP_UI_HELP:
		g_error ("Help unimplemented."); /* FIXME */

	case GNOME_APP_UI_BUILDER_DATA:
		g_error ("Builder data - what to do?"); /* FIXME */

	case GNOME_APP_UI_ITEM_CONFIGURABLE:
		g_error ("Configurable item!");

	case BONOBO_APP_UI_PLACEHOLDER:
		node = xmlNewNode (NULL, "placeholder");
		break;

	default:
		g_warning ("Unknown UIInfo Type: %d", uii->type);
		return NULL;
	}

	xmlSetProp (node, "name", uii->label);

	if (uii->label)
		xmlSetProp (node, "label", L_(uii->label));

	if (uii->hint)
		xmlSetProp (node, "descr", L_(uii->hint));

	verb = uii->label;

	if (uii->type == GNOME_APP_UI_ITEM ||
/*	    uii->type == GNOME_APP_UI_RADIOITEM ||*/
	    uii->type == GNOME_APP_UI_TOGGLEITEM) {
		compat_add_verb (priv->component, verb, uii->moreinfo,
				 data ? data : uii->user_data, "DummyPath", NULL);
		xmlSetProp (node, "verb", verb);
	}

	if (uii->pixmap_info) {
		switch (uii->pixmap_type) {
		case GNOME_APP_PIXMAP_NONE:
			break;
		case GNOME_APP_PIXMAP_STOCK:
			bonobo_ui_util_xml_set_pix_stock (node, uii->pixmap_info);
			break;
		case GNOME_APP_PIXMAP_FILENAME:
			bonobo_ui_util_xml_set_pix_fname (node, uii->pixmap_info);
			break;
		case GNOME_APP_PIXMAP_DATA:
			bonobo_ui_util_xml_set_pix_xpm (node, (const char **)uii->pixmap_info);
			break;
		default:
			break;
		}
	}

	add_accel (node, uii->accelerator_key, uii->ac_mods);

	xmlAddChild (parent, node);

	return node;
}

static void
compat_menu_parse_uiinfo_tree_with_data (BonoboUIHandlerPrivate *priv, 
					 GnomeUIInfo            *uii,
					 void                   *data,
					 xmlNode                *parent);

static void
compat_menu_parse_uiinfo_list_with_data (BonoboUIHandlerPrivate *priv, 
					 GnomeUIInfo            *uii,
					 void                   *data,
					 xmlNode                *parent)
{
	GnomeUIInfo *curr_uii;

	g_return_if_fail (uii != NULL);

	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++) {
		compat_menu_parse_uiinfo_tree_with_data (
			priv, curr_uii, data, parent);
	}

	/* Parse the terminal entry. */
	compat_menu_parse_uiinfo_one_with_data (
		priv, curr_uii, data, parent);
}
	
static void
compat_menu_parse_uiinfo_tree_with_data (BonoboUIHandlerPrivate *priv, 
					 GnomeUIInfo            *uii,
					 void                   *data,
					 xmlNode                *parent)
{
	xmlNode *node;

	node = compat_menu_parse_uiinfo_one_with_data (
		priv, uii, data, parent);

	if (!node)
		return;

	if (uii->type == GNOME_APP_UI_SUBTREE ||
	    uii->type == GNOME_APP_UI_SUBTREE_STOCK) {
		compat_menu_parse_uiinfo_list_with_data (
			priv, uii->moreinfo, data, node);
	}
}

/*
 * Turns eg.
 *    /Wibble/Pie
 * into
 *    /menu/submenu/#Wibble
 * 
 * ie. we discard the explicit path since this cannot be
 * propagated to children with a tree add and inconsistancy
 * is bad.
 */
static char *
make_path (const char *root_at, 
	   const char *duff_path,
	   gboolean    strip_parent)
{
	char    *ret;
	GString *str = g_string_new (root_at);
	char   **strv;
	int      i;

	strv = g_strsplit (duff_path, "/", -1);

	for (i = 0; strv && strv [i]; i++) {
		if (strip_parent && !strv [i + 1])
			break;

		if (strv [i] [0] == '\0')
			continue;

		g_string_append (str, "/");
		g_string_append (str, strv [i]);
	}
	
	g_strfreev (strv);

	ret = str->str;
	g_string_free (str, FALSE);

#ifdef COMPAT_DEBUG
	g_warning ("Make path returns '%s' from '%s'", ret, duff_path);
#endif

	return ret;
}

gboolean
bonobo_ui_handler_menu_path_exists (BonoboUIHandler *uih, const char *path)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *xml_path;
	CORBA_Environment ev;
	gboolean ans;

	g_return_val_if_fail (priv != NULL, FALSE);

	xml_path = make_path ("/menu", path, FALSE);

	CORBA_exception_init (&ev);
	ans = Bonobo_UIContainer_node_exists (priv->container, xml_path, &ev);
	CORBA_exception_free (&ev);

	g_free (xml_path);

	return ans;
}

void
bonobo_ui_handler_menu_add_one (BonoboUIHandler *uih, const char *parent_path,
				BonoboUIHandlerMenuItem *item)
{
	char    *xml_path;
	xmlNode *parent = xmlNewNode (NULL, "dummy");
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", parent_path, FALSE);

	switch (item->type) {
	case BONOBO_UI_COMPAT_LIST:
		compat_menu_parse_uiinfo_list_with_data (
			priv, item->uii, item->data, parent);
		break;
	case BONOBO_UI_COMPAT_ONE:
		compat_menu_parse_uiinfo_one_with_data (
			priv, item->uii, item->data, parent);
		break;
	case BONOBO_UI_COMPAT_TREE:
		compat_menu_parse_uiinfo_tree_with_data (
			priv, item->uii, item->data, parent);
		break;
	}

	compat_set_siblings (priv, xml_path, parent->childs);
	xmlFreeNode (parent);

	g_free (xml_path);
}

void
bonobo_ui_handler_menu_add_list (BonoboUIHandler *uih, const char *parent_path,
				 BonoboUIHandlerMenuItem *item)
{
	bonobo_ui_handler_menu_add_one (uih, parent_path, item);
}

void
bonobo_ui_handler_menu_add_tree (BonoboUIHandler *uih, const char *parent_path,
				 BonoboUIHandlerMenuItem *item)
{
	bonobo_ui_handler_menu_add_one (uih, parent_path, item);
}

void
bonobo_ui_handler_menu_free_one (BonoboUIHandlerMenuItem *item)
{
	g_free (item);
}

void
bonobo_ui_handler_menu_free_list (BonoboUIHandlerMenuItem *item)
{
	g_free (item);
}

void
bonobo_ui_handler_menu_free_tree (BonoboUIHandlerMenuItem *item)
{
	g_free (item);
}

void
bonobo_ui_handler_toolbar_free_list (BonoboUIHandlerMenuItem *item)
{
	g_free (item);
}

/**
 * xmlHasProp:
 *  Only implemented in gnome-xml 2.0
 */
static xmlAttrPtr
xmlHasProp(xmlNodePtr node, const xmlChar *name) {
    xmlAttrPtr prop;

    if ((node == NULL) || (name == NULL)) return(NULL);
    /*
     * Check on the properties attached to the node
     */
    prop = node->properties;
    while (prop != NULL) {
        if (!xmlStrcmp(prop->name, name))  {
	    return(prop);
        }
	prop = prop->next;
    }

    return(NULL);
}

static void
deal_with_pixmap (BonoboUIHandlerPixmapType pixmap_type,
		  gpointer pixmap_data, xmlNode *node)
{
	xmlAttr *attr;

	/* Flush out pre-existing pixmap if any */
	if ((attr = xmlHasProp (node, "pixtype")))
		xmlRemoveProp (attr);

	if ((attr = xmlHasProp (node, "pixname")))
		xmlRemoveProp (attr);

	xmlNodeSetContent (node, NULL);

	switch (pixmap_type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		break;

	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		bonobo_ui_util_xml_set_pix_stock (node, pixmap_data);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
		bonobo_ui_util_xml_set_pix_fname (node, pixmap_data);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		bonobo_ui_util_xml_set_pix_xpm (node, pixmap_data);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA:
		bonobo_ui_util_xml_set_pixbuf (node, pixmap_data);
		break;
	}
}

void
bonobo_ui_handler_menu_new (BonoboUIHandler *uih, const char *path,
			    BonoboUIHandlerMenuItemType type,
			    const char *label, const char *hint,
			    int pos, BonoboUIHandlerPixmapType pixmap_type,
			    gpointer pixmap_data,
			    guint accelerator_key, GdkModifierType ac_mods,
			    BonoboUIHandlerCallback callback,
			    gpointer callback_data)
{
	xmlNode *node;
	char    *cname;
	char    *verb;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);
	cname = strrchr (path, '/');
	g_return_if_fail (cname != NULL);

	if (cname > path && *(cname - 1) == '\\')
		cname = strrchr (cname - 1, '/');
	g_return_if_fail (cname != NULL);

	cname++;

#ifdef COMPAT_DEBUG
	fprintf (stderr, "ui_handler_menu_new '%s'", path);
#endif

	switch (type) {
	case BONOBO_UI_HANDLER_MENU_PLACEHOLDER:
		node = xmlNewNode (NULL, "placeholder");
		xmlSetProp (node, "name", cname);
		goto add_menu_item;

	case BONOBO_UI_HANDLER_MENU_RADIOGROUP:
	case BONOBO_UI_HANDLER_MENU_END:
		return;
	default:
		break;
	}
	
	verb = cname;

	node = bonobo_ui_util_new_menu (type == BONOBO_UI_HANDLER_MENU_SUBTREE,
					cname, label, hint, NULL);
	deal_with_pixmap (pixmap_type, pixmap_data, node);

	switch (type) {
	case BONOBO_UI_HANDLER_MENU_RADIOITEM:
		xmlSetProp (node, "type", "radio");
		break;
	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		xmlSetProp (node, "type", "toggle");
		break;
	default:
		break;
	}

	switch (type) {
	case BONOBO_UI_HANDLER_MENU_ITEM:
	case BONOBO_UI_HANDLER_MENU_RADIOITEM:
	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		compat_add_verb (priv->component, verb,
				 callback, callback_data, path, NULL);
		add_accel (node, accelerator_key, ac_mods);
		xmlSetProp (node, "verb", cname);
		break;
	case BONOBO_UI_HANDLER_MENU_SEPARATOR:
	case BONOBO_UI_HANDLER_MENU_SUBTREE:
		break;
	default:
		g_warning ("Broken type for menu");
		return;
	}

	if (accelerator_key) {
		char *name = bonobo_ui_util_accel_name (accelerator_key,
							ac_mods);
		xmlSetProp (node, "accel", name);
		g_free (name);
	}

	{
		char *xml_path;

	add_menu_item:
		/*
		 * This will never work for evil like /wibble/radio\/ group/etc.
		 */
		if (type == BONOBO_UI_HANDLER_MENU_RADIOITEM) {
			char *real_path = g_strdup (path);
			char *p;
			p = strrchr (real_path, '/');
			g_return_if_fail (p != NULL);
			*p = '\0';
			p = strrchr (real_path, '/');
			g_return_if_fail (p != NULL);
			*p = '\0';
			xmlSetProp (node, "group", p + 1);

			xml_path = make_path ("/menu", real_path, FALSE);
			g_free (real_path);
		} else
			xml_path = make_path ("/menu", path, TRUE);

		compat_set_tree (priv, xml_path, node);
		xmlFreeNode (node);

		g_free (xml_path);
	}
}

void
bonobo_ui_handler_menu_new_item (BonoboUIHandler *uih, const char *path,
				const char *label, const char *hint, int pos,
				BonoboUIHandlerPixmapType pixmap_type,
				gpointer pixmap_data, guint accelerator_key,
				GdkModifierType ac_mods,
				BonoboUIHandlerCallback callback,
				gpointer callback_data)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_ITEM,
				   label, hint, pos, pixmap_type,
				   pixmap_data, accelerator_key, ac_mods,
				   callback, callback_data);
}

void
bonobo_ui_handler_menu_new_subtree (BonoboUIHandler *uih, const char *path,
				   const char *label, const char *hint, int pos,
				   BonoboUIHandlerPixmapType pixmap_type,
				   gpointer pixmap_data, guint accelerator_key,
				   GdkModifierType ac_mods)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_SUBTREE,
				   label, hint, pos, pixmap_type, pixmap_data,
				   accelerator_key, ac_mods, NULL, NULL);
}

void
bonobo_ui_handler_menu_new_separator (BonoboUIHandler *uih, const char *path, int pos)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_SEPARATOR,
				   NULL, NULL, pos, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

void
bonobo_ui_handler_menu_new_placeholder (BonoboUIHandler *uih, const char *path)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_PLACEHOLDER,
				   NULL, NULL, -1, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

void
bonobo_ui_handler_menu_new_radiogroup (BonoboUIHandler *uih, const char *path)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_RADIOGROUP,
				   NULL, NULL, -1, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

void
bonobo_ui_handler_menu_new_radioitem (BonoboUIHandler *uih, const char *path,
				     const char *label, const char *hint, int pos,
				     guint accelerator_key, GdkModifierType ac_mods,
				     BonoboUIHandlerCallback callback,
				     gpointer callback_data)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_RADIOITEM,
				   label, hint, pos, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, accelerator_key, ac_mods, callback, callback_data);
}

void
bonobo_ui_handler_menu_new_toggleitem (BonoboUIHandler *uih, const char *path,
				      const char *label, const char *hint, int pos,
				      guint accelerator_key, GdkModifierType ac_mods,
				      BonoboUIHandlerCallback callback,
				      gpointer callback_data)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_TOGGLEITEM,
				   label, hint, pos, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, accelerator_key, ac_mods, callback, callback_data);
}

int
bonobo_ui_handler_menu_get_pos (BonoboUIHandler *uih, const char *path)
{
	/* FIXME: this can not meaningfully be fixed */
	return -1;
}


static xmlNode *
compat_toolbar_parse_uiinfo_one_with_data (BonoboUIHandlerPrivate *priv, 
					   GnomeUIInfo            *uii,
					   void                   *data,
					   xmlNode                *parent)
{
	xmlNode *node;
	char    *verb;

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	switch (uii->type) {
	case GNOME_APP_UI_ENDOFINFO:
		return NULL;

	case GNOME_APP_UI_ITEM:
		node = xmlNewNode (NULL, "toolitem");
		break;

	case GNOME_APP_UI_TOGGLEITEM:
		node = xmlNewNode (NULL, "toolitem");
		xmlSetProp (node, "type", "toggle");
		break;

	case GNOME_APP_UI_RADIOITEMS:
		g_warning ("FIXME: radioitems ...");

		return NULL;
	default:
		g_warning ("Unknown UIInfo Type: %d", uii->type);
		return NULL;
	}

	xmlSetProp (node, "name", uii->label);
	verb = uii->label;

	if (uii->label)
		xmlSetProp (node, "label", L_(uii->label));

	if (uii->hint)
		xmlSetProp (node, "descr", L_(uii->hint));

	if (uii->type == GNOME_APP_UI_ITEM ||
/*	    uii->type == GNOME_APP_UI_RADIOITEM ||*/
	    uii->type == GNOME_APP_UI_TOGGLEITEM) {
		compat_add_verb (priv->component, verb, uii->moreinfo,
				 data ? data : uii->user_data, "DummyPath", NULL);
		xmlSetProp (node, "verb", verb);
	}

	if (uii->pixmap_info) {
		switch (uii->pixmap_type) {
		case GNOME_APP_PIXMAP_NONE:
			break;
		case GNOME_APP_PIXMAP_STOCK:
			bonobo_ui_util_xml_set_pix_stock (node, uii->pixmap_info);
			break;
		case GNOME_APP_PIXMAP_FILENAME:
			bonobo_ui_util_xml_set_pix_fname (node, uii->pixmap_info);
			break;
		case GNOME_APP_PIXMAP_DATA:
			bonobo_ui_util_xml_set_pix_xpm (node, (const char **)uii->pixmap_info);
			break;
		default:
			break;
		}
	}

	add_accel (node, uii->accelerator_key, uii->ac_mods);

	xmlAddChild (parent, node);

	return node;
}

static void
compat_toolbar_parse_uiinfo_list_with_data (BonoboUIHandlerPrivate *priv, 
					 GnomeUIInfo            *uii,
					 void                   *data,
					 xmlNode                *parent)
{
	GnomeUIInfo *curr_uii;

	g_return_if_fail (uii != NULL);

	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++) {
		compat_toolbar_parse_uiinfo_one_with_data (
			priv, curr_uii, data, parent);
	}

	/* Parse the terminal entry. */
	compat_toolbar_parse_uiinfo_one_with_data (
		priv, curr_uii, data, parent);
}

void
bonobo_ui_handler_toolbar_add_list (BonoboUIHandler *uih, const char *parent_path,
				    BonoboUIHandlerToolbarItem *item)
{
	char    *xml_path;
	xmlNode *parent = xmlNewNode (NULL, "dummy");
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("", parent_path, TRUE);

	compat_toolbar_parse_uiinfo_list_with_data (
		priv, item->uii, item->data, parent);

	compat_set_siblings (priv, xml_path, parent->childs);
	xmlFreeNode (parent);

	g_free (xml_path);
}

void
bonobo_ui_handler_toolbar_new (BonoboUIHandler *uih, const char *path,
			      BonoboUIHandlerToolbarItemType type,
			      const char *label, const char *hint,
			      int pos, const Bonobo_Control control,
			      BonoboUIHandlerPixmapType pixmap_type,
			      gpointer pixmap_data, guint accelerator_key,
			      GdkModifierType ac_mods,
			      BonoboUIHandlerCallback callback,
			      gpointer callback_data)
{
	xmlNode *node;
	char    *cname = strrchr (path, '/');
	char    *verb;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);
	g_return_if_fail (cname != NULL);
	cname++;

	verb = cname;

	switch (type) {
	case BONOBO_UI_HANDLER_MENU_SEPARATOR:
		node = bonobo_ui_util_new_std_toolbar (cname, NULL, NULL, NULL);
		break;
	case BONOBO_UI_HANDLER_MENU_ITEM:
		node = bonobo_ui_util_new_std_toolbar (cname, label?label[0]?label:NULL:NULL,
						       hint, verb);
		add_accel (node, accelerator_key, ac_mods);
		compat_add_verb (priv->component, verb, callback,
				 callback_data, path, NULL);
		break;
	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		node = bonobo_ui_util_new_toggle_toolbar (cname, label?label[0]?label:NULL:NULL,
							  hint, verb);
		compat_add_verb (priv->component, verb, callback,
				 callback_data, path, NULL);
		add_accel (node, accelerator_key, ac_mods);
		/* Add Verb & Callback */
		break;
	case BONOBO_UI_HANDLER_MENU_RADIOITEM:
	case BONOBO_UI_HANDLER_MENU_RADIOGROUP:
	case BONOBO_UI_HANDLER_MENU_PLACEHOLDER:
	case BONOBO_UI_HANDLER_MENU_SUBTREE:
		return;
	case BONOBO_UI_HANDLER_MENU_END:
	default:
		g_warning ("Broken type for toolbar");
		return;
	}

	deal_with_pixmap (pixmap_type, pixmap_data, node);

	{
		char *xml_path = make_path ("", path, TRUE);

		compat_set_tree (priv, xml_path, node);
		xmlFreeNode (node);

		g_free (xml_path);
	}
}

/**
 * bonobo_ui_handler_toolbar_new_item
 */
void
bonobo_ui_handler_toolbar_new_item (BonoboUIHandler *uih, const char *path,
				   const char *label, const char *hint, int pos,
				   BonoboUIHandlerPixmapType pixmap_type,
				   gpointer pixmap_data,
				   guint accelerator_key, GdkModifierType ac_mods,
				   BonoboUIHandlerCallback callback,
				   gpointer callback_data)
{
	bonobo_ui_handler_toolbar_new (uih, path,
				      BONOBO_UI_HANDLER_TOOLBAR_ITEM,
				      label, hint, pos, CORBA_OBJECT_NIL, pixmap_type,
				      pixmap_data, accelerator_key,
				      ac_mods, callback, callback_data);
}

/**
 * bonobo_ui_handler_toolbar_new_control:
 */
void
bonobo_ui_handler_toolbar_new_control (BonoboUIHandler *uih, const char *path,
				      int pos, Bonobo_Control control)
{
	bonobo_ui_handler_toolbar_new (uih, path,
				      BONOBO_UI_HANDLER_TOOLBAR_CONTROL,
				      NULL, NULL, pos, control, BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}


/**
 * bonobo_ui_handler_toolbar_new_separator:
 */
void
bonobo_ui_handler_toolbar_new_separator (BonoboUIHandler *uih, const char *path,
					int pos)
{
	bonobo_ui_handler_toolbar_new (uih, path,
				      BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR,
				      NULL, NULL, pos, CORBA_OBJECT_NIL,
				      BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}

/**
 * bonobo_ui_handler_toolbar_new_radiogroup:
 */
void
bonobo_ui_handler_toolbar_new_radiogroup (BonoboUIHandler *uih, const char *path)
{
	bonobo_ui_handler_toolbar_new (uih, path, BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP,
				      NULL, NULL, -1, CORBA_OBJECT_NIL,
				      BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}

/**
 * bonobo_ui_handler_toolbar_new_radioitem:
 */
void
bonobo_ui_handler_toolbar_new_radioitem (BonoboUIHandler *uih, const char *path,
					const char *label, const char *hint, int pos,
					BonoboUIHandlerPixmapType pixmap_type,
					gpointer pixmap_data,
					guint accelerator_key, GdkModifierType ac_mods,
					BonoboUIHandlerCallback callback,
					gpointer callback_data)
{
	g_warning ("Radioitems Unsupported");
/*	bonobo_ui_handler_toolbar_new (uih, path, BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM,
				      label, hint, pos, CORBA_OBJECT_NIL,
				      pixmap_type, pixmap_data,
				      accelerator_key, ac_mods, callback, callback_data);*/
}

/**
 * bonobo_ui_handler_toolbar_new_toggleitem:
 */
void
bonobo_ui_handler_toolbar_new_toggleitem (BonoboUIHandler *uih, const char *path,
					 const char *label, const char *hint, int pos,
					 BonoboUIHandlerPixmapType pixmap_type,
					 gpointer pixmap_data,
					 guint accelerator_key, GdkModifierType ac_mods,
					 BonoboUIHandlerCallback callback,
					 gpointer callback_data)
{
	bonobo_ui_handler_toolbar_new (uih, path, BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM,
				      label, hint, pos, CORBA_OBJECT_NIL,
				      pixmap_type, pixmap_data,
				      accelerator_key, ac_mods, callback,
				      callback_data);
}

static void
do_set_pixmap (BonoboUIHandlerPrivate *priv,
	       const char             *xml_path,
	       BonoboUIHandlerPixmapType type, gpointer data)
{
	char    *parent_path;
	xmlNode *node;
	
	parent_path = bonobo_ui_xml_get_parent_path (xml_path);

	node = bonobo_ui_container_get_tree (priv->container,
					     xml_path, FALSE, NULL);

	g_return_if_fail (node != NULL);

	deal_with_pixmap (type, data, node);

	bonobo_ui_component_set_tree (priv->component,
				      priv->container,
				      parent_path,
				      node, NULL);

	g_free (parent_path);
	xmlFreeNode (node);
}

void
bonobo_ui_handler_toolbar_item_set_pixmap (BonoboUIHandler *uih, const char *path,
					   BonoboUIHandlerPixmapType type,
					   gpointer data)
{
	char *xml_path;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);
	do_set_pixmap (priv, xml_path, type, data);
	g_free (xml_path);
}

void
bonobo_ui_handler_set_app (BonoboUIHandler *uih, BonoboWin *app)
{
	BonoboUIHandlerPrivate *priv;
	BonoboUIContainer *container;

	priv = get_priv (uih);
	g_return_if_fail (priv != NULL);

	priv->application = app;

	container = bonobo_ui_container_new ();
	bonobo_ui_container_set_app (container, app);
	bonobo_ui_handler_set_container (
		uih, bonobo_object_corba_objref (BONOBO_OBJECT (container)));
}

BonoboWin *
bonobo_ui_handler_get_app (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv;

	priv = get_priv (uih);
	g_return_val_if_fail (priv != NULL, NULL);

	return priv->application;
}

GtkType
bonobo_ui_handler_get_type (void)
{
	return GTK_TYPE_OBJECT;
}

void
bonobo_ui_handler_set_statusbar (BonoboUIHandler *uih, GtkWidget *statusbar)
{
	g_warning ("Not setting statusbar '%p'", statusbar);
}

GtkWidget *
bonobo_ui_handler_get_statusbar (BonoboUIHandler *uih)
{
	return NULL;
}

void
bonobo_ui_handler_menu_set_toggle_state	(BonoboUIHandler *uih, const char *path,
					 gboolean state)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	const char *txt;
	char *xml_path;

	g_return_if_fail (priv != NULL);

	if (state)
		txt = "1";
	else
		txt = "0";

	xml_path = make_path ("/menu", path, FALSE);

	sloppy_check (priv->container, xml_path, NULL);

	bonobo_ui_container_set_prop (priv->container, xml_path, "state", txt, NULL);
	g_free (xml_path);
}

gboolean
bonobo_ui_handler_menu_get_toggle_state	(BonoboUIHandler *uih, const char *path)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	gboolean ret;
	char *txt, *xml_path;

	g_return_val_if_fail (priv != NULL, FALSE);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check_val (priv->container, xml_path, NULL, FALSE);
	txt = bonobo_ui_container_get_prop (priv->container, xml_path, "state", NULL);
	ret = atoi (txt);
	g_free (txt);
	g_free (xml_path);

	return ret;
}

void
bonobo_ui_handler_menu_set_radio_state (BonoboUIHandler *uih, const char *path,
					gboolean state)
{
	bonobo_ui_handler_menu_set_toggle_state (uih, path, state);
}

gboolean
bonobo_ui_handler_menu_get_radio_state (BonoboUIHandler *uih, const char *path)
{
	return bonobo_ui_handler_menu_get_toggle_state (uih, path);
}

void
bonobo_ui_handler_menu_remove (BonoboUIHandler *uih, const char *path)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *xml_path;

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);

	bonobo_ui_component_rm (priv->component, priv->container, xml_path, NULL);

	g_free (xml_path);
}

void
bonobo_ui_handler_menu_set_sensitivity (BonoboUIHandler *uih, const char *path,
					gboolean sensitive)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *xml_path;

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);
	if (sensitive)
		bonobo_ui_container_set_prop (
			priv->container, xml_path, "sensitive", "1", NULL);
	else
		bonobo_ui_container_set_prop (
			priv->container, xml_path, "sensitive", "0", NULL);
	g_free (xml_path);
}

void
bonobo_ui_handler_menu_set_label (BonoboUIHandler *uih, const char *path,
				  const gchar *label)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *xml_path;

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);
	bonobo_ui_container_set_prop (priv->container, xml_path, "label", label, NULL);
	g_free (xml_path);
}

gchar *
bonobo_ui_handler_menu_get_label (BonoboUIHandler *uih, const char *path)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *label, *xml_path;

	g_return_val_if_fail (priv != NULL, FALSE);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check_val (priv->container, xml_path, NULL, NULL);
	label = bonobo_ui_container_get_prop (priv->container, xml_path, "label", NULL);
	g_free (xml_path);

	return label;
}

void
bonobo_ui_handler_menu_set_hint (BonoboUIHandler *uih, const char *path,
				 const gchar *hint)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *xml_path;

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);
	bonobo_ui_container_set_prop (priv->container, xml_path, "hint", hint, NULL);
	g_free (xml_path);
}

void
bonobo_ui_handler_menu_set_pixmap (BonoboUIHandler *uih, const char *path,
				   BonoboUIHandlerPixmapType type, gpointer data)
{
	char *xml_path;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);
	do_set_pixmap (priv, xml_path, type, data);
	g_free (xml_path);
}

void
bonobo_ui_handler_menu_set_callback (BonoboUIHandler *uih, const char *path,
				     BonoboUIHandlerCallback callback,
				     gpointer callback_data,
				     GDestroyNotify callback_data_destroy_notify)
{
	xmlNode *node;
	char *xml_path;
	char *verb;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", path, FALSE);
	sloppy_check (priv->container, xml_path, NULL);

	node = bonobo_ui_container_get_tree (priv->container,
					     xml_path, FALSE, NULL);

	g_return_if_fail (node != NULL);

	if ((verb = xmlGetProp (node, "verb"))) {
		compat_add_verb (priv->component, verb,
				 callback, callback_data, path,
				 callback_data_destroy_notify);
	} else
		g_warning ("No verb to add callback to");

	g_free (xml_path);
	xmlFreeNode (node);
}

void
bonobo_ui_handler_menu_get_callback (BonoboUIHandler *uih, const char *path,
				     BonoboUIHandlerCallback *callback,
				     gpointer *callback_data,
				     GDestroyNotify *callback_data_destroy_notify)
{
	fprintf (stderr, "bonobo_ui_handler_menu_get_callback unimplemented\n");
	*callback = NULL;
	*callback_data = NULL;
	*callback_data_destroy_notify = NULL;
}

void
bonobo_ui_handler_menu_remove_callback_no_notify (BonoboUIHandler *uih,
						  const char *path)
{
	fprintf (stderr, "bonobo_ui_handler_remove_callback_no_notify unimplemented\n");
}

gboolean
bonobo_ui_handler_dock_add (BonoboUIHandler       *uih,
			    const char            *name,
			    Bonobo_Control         control,
			    GnomeDockItemBehavior  behavior,
			    GnomeDockPlacement     placement,
			    gint                   band_num,
			    gint                   band_position,
			    gint                   offset)
{
	fprintf (stderr, "bonobo_ui_handler_dock_add unimplemented\n");
	return TRUE;
}

gboolean
bonobo_ui_handler_dock_remove (BonoboUIHandler       *uih,
			       const char            *name)
{
	fprintf (stderr, "bonobo_ui_handler_dock_remove unimplemented\n");
	return TRUE;
}

gboolean
bonobo_ui_handler_dock_set_sensitive (BonoboUIHandler       *uih,
				      const char            *name,
				      gboolean               sensitivity)
{
	fprintf (stderr, "bonobo_ui_handler_dock_set_sensitive unimplemented\n");
	return TRUE;
}

gboolean
bonobo_ui_handler_dock_get_sensitive (BonoboUIHandler       *uih,
				      const char            *name)
{
	fprintf (stderr, "bonobo_ui_handler_dock_get_sensitive unimplemented\n");
	return TRUE;
}

BonoboUIComponent *
bonobo_ui_compat_get_component (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_val_if_fail (priv != NULL, NULL);

	return priv->component;
}

Bonobo_UIContainer
bonobo_ui_compat_get_container (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_val_if_fail (priv != NULL, NULL);

	return priv->container;
}

static char *
path_escape_forward_slashes (const char *str)
{
	const char *p = str;
	char *new, *newp;

	new = g_malloc (strlen (str) * 2 + 1);
	newp = new;

	while (*p != '\0') {
		g_assert (*p != '/');
/*		if (*p == '/') {
			*newp++ = '\\';
			*newp++ = '/';
		} else if (*p == '\\') {
			*newp++ = '\\';
			*newp++ = '\\';
		} else {
			*newp++ = *p;
		}
*/
		p++;
	}

/*	*newp = '\0';

	final = g_strdup (new);
	g_free (new);*/
	return g_strdup (str);
}

/*
 * bonobo_ui_handler_build_path_v:
 * 
 * Builds a path from an optional base path and a list of individual pieces.
 * 
 * @base: Beginning of path, already including leading separator and 
 * separators between pieces. Pass NULL to construct the entire path
 * from individual pieces.
 * @va_list: List of individual path pieces, without any separators yet added.
 * 
 * Result: Path ready for use with bonobo_ui_handler calls that take paths.
 */
char *
bonobo_ui_handler_build_path_v (const char *base, va_list path_components)
{
	char *path;
	const char *path_component;

	/* Note that base is not escaped, because it already contains separators. */
	path = g_strdup (base);
	for (;;) {
		char *old_path;
		char *escaped_component;
	
		path_component = va_arg (path_components, const char *);
		if (path_component == NULL)
			break;
			
		old_path = path;
		escaped_component = path_escape_forward_slashes (path_component);

		if (path == NULL) {
			path = g_strconcat ("/", escaped_component, NULL);
		} else {
			path = g_strconcat (old_path, "/", escaped_component, NULL);
		}
		g_free (old_path);
	}
	
	return path;
}

/*
 * bonobo_ui_handler_build_path:
 * 
 * Builds a path from an optional base path and a NULL-terminated list of individual pieces.
 * 
 * @base: Beginning of path, already including leading separator and 
 * separators between pieces. Pass NULL to construct the entire path
 * from individual pieces.
 * @...: List of individual path pieces, without any separators yet added, ending with NULL.
 * 
 * Result: Path ready for use with bonobo_ui_handler calls that take paths.
 */
char *
bonobo_ui_handler_build_path (const char *base, ...)
{
	va_list args;
	char *path;

	va_start (args, base);
	path = bonobo_ui_handler_build_path_v (base, args);
	va_end (args);
	
	return path;
}

GList *
bonobo_ui_handler_menu_get_child_paths (BonoboUIHandler *uih, const char *parent_path)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	xmlNode *node, *l;
	GList *ret = NULL, *i;
	char *xml_path;

	g_return_val_if_fail (priv != NULL, NULL);
	g_return_val_if_fail (parent_path != NULL, NULL);
	g_return_val_if_fail (priv->container != CORBA_OBJECT_NIL, NULL);

	xml_path = make_path ("/menu", parent_path, FALSE);

	fprintf (stderr, "Get child paths from '%s' '%s'", parent_path, xml_path);
	node = bonobo_ui_container_get_tree (
		priv->container, xml_path, TRUE, NULL);

	g_free (xml_path);

	g_return_val_if_fail (node != NULL, NULL);

	for (l = node->childs; l; l = l->next) {
		xmlChar *txt;
		if ((txt = xmlGetProp (l, "name"))) {
			ret = g_list_prepend (ret, g_strdup (txt));
			xmlFree (txt);
		} else
			ret = g_list_prepend (ret, g_strdup (l->name));
	}

	xmlFreeNode (node);

	if (ret)
		for (i = ret; i; i = i->next)
			fprintf (stderr, "> '%s'\n", (char *)i->data);
	else
		fprintf (stderr, "No items\n");

	return g_list_reverse (ret);
}