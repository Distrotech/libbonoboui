/*
 *  This module acts as an excercise in filthy coding habits
 * take a look around.
 */

#include "bonobo.h"
#include "bonobo-ui-compat.h"

#define COMPAT_DEBUG

typedef struct {
	BonoboUIComponent *component;
	BonoboApp         *app;

	char              *name;

	BonoboUIXml       *ui;

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
compat_sync (BonoboUIHandlerPrivate *priv, const char *parent_path,
	     xmlNode *single_node)
{
	gboolean do_siblings;
	xmlNode *node;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (priv->ui != NULL);
	g_return_if_fail (priv->ui->root != NULL);

#ifdef COMPAT_DEBUG
	g_warning ("Sync '%s' %d", parent_path, (single_node != NULL));
#endif

	if (single_node) {
		do_siblings = FALSE;
		node = single_node;
	} else {
		xmlNode *parent;

		if (!strcmp (parent_path, "/")) { /* Just blat root over; special case */
			do_siblings = FALSE;
			node = priv->ui->root;
		}
		
		parent = bonobo_ui_xml_get_path (priv->ui, parent_path);
		if (!parent || !parent->childs)
			return;

		do_siblings = TRUE;
		node = parent->childs;
	}

	if (!node)
		return;

	if (do_siblings) {
		xmlNode *l;

/*		g_warning ("TESTME: do siblings");*/
		for (; node->prev; node = node->prev)
			;
		for (l = node; l; l = l->next) {
			xmlNode *copy = xmlCopyNode (node, TRUE);
			
			bonobo_ui_component_set_tree (
				priv->component, priv->container,
				parent_path, copy, NULL);
			
			xmlFreeNode (copy);
		}
	} else {
		xmlNode *copy;
		
		if (node->prev || node->next)
			copy = xmlCopyNode (node, TRUE);
		else
			copy = node;
		
/*		g_warning ("TESTME: do normal set");*/

		bonobo_ui_component_set_tree (
			priv->component, priv->container,
			parent_path, copy, NULL);
			
		if (node->prev || node->next)
			xmlFreeNode (copy);
	}
}

void
bonobo_ui_handler_set_container (BonoboUIHandler *uih,
				 Bonobo_Unknown   cont)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	priv->container = bonobo_object_dup_ref (cont, NULL);

	compat_sync (priv, "/", NULL);
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

#warning FIXME; we currently always have a menu bar; hmmm...
	bonobo_ui_xml_get_path (priv->ui, "/menu");
}

void
bonobo_ui_handler_create_toolbar (BonoboUIHandler *uih, const char *name)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	char *path;

	g_return_if_fail (priv != NULL);

	path = g_strdup_printf ("/dockitem/#%s", name);
	bonobo_ui_xml_get_path (priv->ui, path);
	g_free (path);
	compat_sync (priv, "/", NULL);
}

static void
priv_destroy (GtkObject *object, BonoboUIHandlerPrivate *priv)
{
	if (priv->container)
		bonobo_ui_handler_unset_container ((BonoboUIHandler *)object);

	gtk_object_unref (GTK_OBJECT (priv->ui));

	g_free (priv->name);
	g_free (priv);
}

static void
setup_priv (BonoboObject *object)
{
	BonoboUIHandlerPrivate *priv;

	priv = g_new0 (BonoboUIHandlerPrivate, 1);

	priv->ui = bonobo_ui_xml_new (NULL, NULL, NULL, NULL, NULL);

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
	char              *name = g_strdup_printf ("Busk%d", busk_name++);
	BonoboUIHandlerPrivate *priv;

	object = bonobo_ui_component_new (name);

	g_free (name);

	setup_priv (BONOBO_OBJECT (object));

	priv = get_priv ((BonoboUIHandler *)object);
	g_return_val_if_fail (priv != NULL, NULL);

	priv->component = object;

	return (BonoboUIHandler *)object;
}

/*
 * This constructs a new container
 */
BonoboUIHandler *
bonobo_ui_handler_new_for_app (BonoboApp *app)
{
	BonoboUIHandlerPrivate *priv;

	setup_priv (BONOBO_OBJECT (app));

	priv = get_priv ((BonoboUIHandler *)app);
	g_return_val_if_fail (priv != NULL, NULL);

	priv->app = app;
	priv->component = bonobo_ui_component_new ("toplevel");

	/* Festering lice */
	bonobo_ui_handler_set_container (
		(BonoboUIHandler *) app,
		bonobo_object_corba_objref (BONOBO_OBJECT (app)));

	return (BonoboUIHandler *)app;
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
add_accel_verb (BonoboUIHandlerPrivate *priv,
		guint key, GdkModifierType ac_mods,
		const char *verb)
{
	xmlNode *parent, *accel;

	if (!verb || key == 0)
		return;

	accel = bonobo_ui_util_build_accel (key, ac_mods, verb);
	parent = bonobo_ui_xml_get_path (priv->ui, "/keybindings");

	xmlAddChild (parent, accel);
}		

typedef struct {
	BonoboUIHandlerCallback cb;
	gpointer                user_data;
	char                   *old_path;
} VerbClosure;

static void
verb_to_cb (BonoboUIComponent *component,
	    const char        *cname,
	    gpointer           user_data)
{
	VerbClosure *c = user_data;

	if (!c || !c->cb)
		return;

	/* Does anyone use the path field here ? */
	c->cb ((BonoboUIHandler *)component, c->user_data,
	       c->old_path);
}

static void
free_closure (gpointer object, gpointer closure)
{
	g_free (((VerbClosure *)closure)->old_path);
	g_free (closure);
}

static void
compat_add_verb (BonoboUIComponent *component, const char *verb,
		 BonoboUIHandlerCallback cb, gpointer user_data,
		 const char *old_path)
{
	VerbClosure *c = g_new (VerbClosure, 1);

	c->cb        = cb;
	c->user_data = user_data;
	c->old_path  = g_strdup (old_path);

	bonobo_ui_component_add_verb (component, verb,
				      verb_to_cb, c);
	gtk_signal_connect (GTK_OBJECT (component),
			    "destroy", (GtkSignalFunc) free_closure, c);
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
				 data ? data : uii->user_data, "DummyPath");
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

	add_accel_verb (priv, uii->accelerator_key, uii->ac_mods, verb);
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
make_path (const char *root_at, const char *subtype,
	   const char *duff_path, gboolean strip_parent)
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
		g_string_append (str, subtype);
		g_string_append (str, "/#");
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

	xml_path = make_path ("/menu", "submenu", path, FALSE);

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
	xmlNode *parent;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("/menu", "submenu", parent_path, TRUE);
	parent = bonobo_ui_xml_get_path (priv->ui, xml_path);

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

	compat_sync (priv, xml_path, NULL);

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
	xmlNode *node, *parent;
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

	switch (type) {
	case BONOBO_UI_HANDLER_MENU_PLACEHOLDER:
		g_warning ("CONVERTME: Placeholders not handled");
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
	case BONOBO_UI_HANDLER_MENU_ITEM:
	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		compat_add_verb (priv->component, verb,
				 callback, callback_data, path);
		add_accel_verb (priv, accelerator_key, ac_mods, verb);
		xmlSetProp (node, "verb", cname);
		break;
	case BONOBO_UI_HANDLER_MENU_SEPARATOR:
	case BONOBO_UI_HANDLER_MENU_SUBTREE:
		break;
	default:
		g_warning ("Broken type for menu");
		return;
	}

	{
		char *xml_path;

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

			xml_path = make_path ("/menu", "submenu", real_path, FALSE);
			g_free (real_path);
		} else
			xml_path = make_path ("/menu", "submenu", path, TRUE);

		parent = bonobo_ui_xml_get_path (priv->ui, xml_path);

		xmlAddChild (parent, node);
		compat_sync (priv, xml_path, node);

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
				 data ? data : uii->user_data, "DummyPath");
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

	add_accel_verb (priv, uii->accelerator_key, uii->ac_mods, verb);
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
	xmlNode *parent;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("", "dockitem", parent_path, TRUE);
	parent = bonobo_ui_xml_get_path (priv->ui, xml_path);

	compat_toolbar_parse_uiinfo_list_with_data (
		priv, item->uii, item->data, parent);

	compat_sync (priv, xml_path, NULL);

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
	xmlNode *node, *parent;
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
		add_accel_verb (priv, accelerator_key, ac_mods, verb);
		compat_add_verb (priv->component, verb, callback,
				 callback_data, path);
		break;
	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		node = bonobo_ui_util_new_toggle_toolbar (cname, label?label[0]?label:NULL:NULL,
							  hint, verb);
		compat_add_verb (priv->component, verb, callback,
				 callback_data, path);
		add_accel_verb (priv, accelerator_key, ac_mods, verb);
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
		char *xml_path = make_path ("", "dockitem", path, TRUE);
		parent = bonobo_ui_xml_get_path (priv->ui, xml_path);

		xmlAddChild (parent, node);
		compat_sync (priv, xml_path, node);

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

void
bonobo_ui_handler_toolbar_item_set_pixmap (BonoboUIHandler *uih, const char *path,
					   BonoboUIHandlerPixmapType type,
					   gpointer data)
{
	xmlNode *node;
	char *xml_path;
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	g_return_if_fail (priv != NULL);

	xml_path = make_path ("", "dockitem", path, TRUE);

	node = bonobo_ui_container_get_tree (priv->container,
					     xml_path, FALSE, NULL);

	g_return_if_fail (node != NULL);

	deal_with_pixmap (type, data, node);

	bonobo_ui_component_set_tree (priv->component,
				      priv->container,
				      xml_path,
				      node, NULL);

	g_free (xml_path);
	xmlFreeNode (node);
}

void
bonobo_ui_handler_set_app (BonoboUIHandler *uih, GnomeApp *app)
{
	g_warning ("Deprecated function; you need to use bonobo_app");
}

GnomeApp *
bonobo_ui_handler_get_app (BonoboUIHandler *uih)
{
	return NULL;
}

GtkType
bonobo_ui_handler_get_type (void)
{
	return GTK_TYPE_OBJECT;
}
