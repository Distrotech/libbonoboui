/*
 * bonobo-win.c: The Bonobo Window implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include "config.h"
#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-win.h>
#include <bonobo/bonobo-ui-item.h>
#include <bonobo/bonobo-ui-toolbar.h>

#undef NAUTILUS_LOOP

#define XML_FREE(a) ((a)?xmlFree(a):(a))

#define	BINDING_MOD_MASK()				\
	(gtk_accelerator_get_default_mod_mask () | GDK_RELEASE_MASK)

#define BONOBO_WIN_PRIV_KEY "BonoboWin::Priv"

GtkWindowClass *bonobo_win_parent_class = NULL;

struct _BonoboWinPrivate {
	BonoboWin     *app;

	int            frozen;

	GnomeDock     *dock;

	GnomeDockItem *menu_item;
	GtkMenuBar    *menu;

	BonoboUIXml   *tree;

	GtkAccelGroup *accel_group;

	char          *name;		/* Win name */
	char          *prefix;		/* Win prefix */

	GHashTable    *radio_groups;
	GHashTable    *keybindings;
	GSList        *components;
	GSList        *popups;

	GtkWidget     *main_vbox;

	GtkBox        *status;
	GtkStatusbar  *main_status;

	GtkWidget     *client_area;
};

typedef enum {
	ROOT_WIDGET   = 0x1,
	CUSTOM_WIDGET = 0x2
} NodeType;

#define NODE_IS_ROOT_WIDGET(n)   ((n->type & ROOT_WIDGET) != 0)
#define NODE_IS_CUSTOM_WIDGET(n) ((n->type & CUSTOM_WIDGET) != 0)

typedef struct {
	BonoboUIXmlData parent;

	int             type;
	GtkWidget      *widget;
	Bonobo_Unknown  object;
} NodeInfo;

static void
info_dump_fn (BonoboUIXmlData *a)
{
	NodeInfo *info = (NodeInfo *) a;

	fprintf (stderr, " '%s' widget %8p object %8p\n",
		 (char *)a->id, info->widget, info->object);
}

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

/*
 *   Placeholders have no widget, so we have to go above them
 * to fetch the real parent.
 */
static GtkWidget *
node_get_parent_widget (BonoboUIXml *tree, xmlNode *node)
{
	NodeInfo *info;

	if (!node)
		return NULL;

	do {
		info = bonobo_ui_xml_get_data (tree, node->parent);
		if (info && info->widget)
			return info->widget;
	} while ((node = node->parent) && (node->parent));

	return NULL;
}

static char *
node_get_id (xmlNode *node)
{
	xmlChar *txt;
	char    *ret;

	g_return_val_if_fail (node != NULL, NULL);

	if (!(txt = xmlGetProp (node, "id"))) {
		txt = xmlGetProp (node, "verb");
		if (txt && txt [0] == '\0') {
			xmlFree (txt);
			txt = xmlGetProp (node, "name");
		}
	}

	if (txt) {
		ret = g_strdup (txt);
		xmlFree (txt);
	} else
		ret = NULL;

	return ret;
}

static char *
node_get_id_or_path (xmlNode *node)
{
	char *txt;

	g_return_val_if_fail (node != NULL, NULL);

	if ((txt = node_get_id (node)))
		return txt;

	return bonobo_ui_xml_make_path (node);
}

/*
 *  This indirection is needed so we can serialize user settings
 * on a per component basis in future.
 */
typedef struct {
	char          *name;
	Bonobo_Unknown object;
} WinComponent;

static WinComponent *
win_component_get (BonoboWinPrivate *priv, const char *name)
{
	WinComponent *component;
	GSList       *l;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (priv != NULL, NULL);

	for (l = priv->components; l; l = l->next) {
		component = l->data;
		
		if (!strcmp (component->name, name))
			return component;
	}
	
	component = g_new (WinComponent, 1);
	component->name = g_strdup (name);
	component->object = CORBA_OBJECT_NIL;

	priv->components = g_slist_prepend (priv->components, component);

	return component;
}

static Bonobo_Unknown
win_component_objref (BonoboWinPrivate *priv, const char *name)
{
	WinComponent *component = win_component_get (priv, name);

	g_return_val_if_fail (component != NULL, CORBA_OBJECT_NIL);

	return component->object;
}

/*
 * Use the pointer identity instead of a costly compare
 */
static char *
win_component_cmp_name (BonoboWinPrivate *priv, const char *name)
{
	WinComponent *component;

	/*
	 * NB. For overriding if we get a NULL we just update the
	 * node without altering the id.
	 */
	if (!name || name [0] == '\0')
		return NULL;

	component = win_component_get (priv, name);
	g_return_val_if_fail (component != NULL, NULL);

	return component->name;
}

static void
win_component_destroy (BonoboWinPrivate *priv, WinComponent *component)
{
	priv->components = g_slist_remove (priv->components, component);

	if (component) {
		g_free (component->name);
		if (component->object != CORBA_OBJECT_NIL)
			bonobo_object_release_unref (component->object, NULL);
		g_free (component);
	}
}

typedef struct {
	GtkMenu          *menu;
	char             *path;
} WinPopup;

static void
popup_remove (BonoboWinPrivate *priv,
	      WinPopup         *popup)
{
	g_return_if_fail (priv != NULL);
	g_return_if_fail (popup != NULL);

	priv->popups = g_slist_remove (
		priv->popups, popup);
	
	g_free (popup->path);
	g_free (popup);
}

void
bonobo_win_remove_popup (BonoboWin     *win,
			 const char    *path)
{
	GSList *l, *next;

	g_return_if_fail (path != NULL);
	g_return_if_fail (BONOBO_IS_WIN (win));
	g_return_if_fail (win->priv != NULL);

	for (l = win->priv->popups; l; l = next) {
		WinPopup *popup = l->data;

		next = l->next;
		if (!strcmp (popup->path, path))
			popup_remove (win->priv, popup);
	}
}

static void
popup_destroy (GtkObject *menu, WinPopup *popup)
{
	BonoboWinPrivate *priv = gtk_object_get_data (
		GTK_OBJECT (menu), BONOBO_WIN_PRIV_KEY);

	g_return_if_fail (priv != NULL);
	bonobo_win_remove_popup (priv->app, popup->path);
}

void
bonobo_win_add_popup (BonoboWin     *win,
		      GtkMenu       *menu,
		      const char    *path)
{
	WinPopup *popup;

	g_return_if_fail (path != NULL);
	g_return_if_fail (GTK_IS_MENU (menu));
	g_return_if_fail (BONOBO_IS_WIN (win));

	bonobo_win_remove_popup (win, path);

	popup       = g_new (WinPopup, 1);
	popup->menu = menu;
	popup->path = g_strdup (path);

	win->priv->popups = g_slist_prepend (win->priv->popups, popup);

	gtk_object_set_data (GTK_OBJECT (menu),
			     BONOBO_WIN_PRIV_KEY,
			     win->priv);

	gtk_signal_connect (GTK_OBJECT (menu), "destroy",
			    (GtkSignalFunc) popup_destroy, popup);
}

void
bonobo_win_register_component (BonoboWin     *app,
			       const char    *name,
			       Bonobo_Unknown component)
{
	WinComponent *appcomp;

	g_return_if_fail (BONOBO_IS_WIN (app));

	if ((appcomp = win_component_get (app->priv, name))) {
		if (appcomp->object != CORBA_OBJECT_NIL)
			bonobo_object_release_unref (appcomp->object, NULL);
	}

	if (component != CORBA_OBJECT_NIL)
		appcomp->object = bonobo_object_dup_ref (component, NULL);
	else
		appcomp->object = CORBA_OBJECT_NIL;
}

void
bonobo_win_deregister_component (BonoboWin     *app,
				 const char    *name)
{
	WinComponent *component;

	g_return_if_fail (BONOBO_IS_WIN (app));

	if ((component = win_component_get (app->priv, name))) {
		bonobo_win_xml_rm (app, "/", component->name);
		win_component_destroy (app->priv, component);
	} else
		g_warning ("Attempting to deregister non-registered "
			   "component '%s'", name);
}

static xmlNode *
get_cmd_state (BonoboWinPrivate *priv, const char *cmd_name)
{
	char    *path;
	xmlNode *ret;

	g_return_val_if_fail (priv != NULL, NULL);

	if (!cmd_name)
		return NULL;

	path = g_strconcat ("/commands/", cmd_name, NULL);
	ret  = bonobo_ui_xml_get_path (priv->tree, path);

	if (!ret) {
		xmlNode *node = xmlNewNode (NULL, "cmd");
		xmlSetProp (node, "name", cmd_name);

		bonobo_ui_xml_merge (
			priv->tree, "/commands", node, NULL);
		
		ret = bonobo_ui_xml_get_path (priv->tree, path);
		g_assert (ret != NULL);
	}

	g_free (path);

	return ret;
}

static void
set_cmd_dirty (BonoboWinPrivate *priv, xmlNode *cmd_node)
{
	char *cmd_name;
	xmlNode *node;
	BonoboUIXmlData *data;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (cmd_node != NULL);

	if (!(cmd_name = node_get_id (cmd_node)))
		return;

	node = get_cmd_state (priv, cmd_name);

	data = bonobo_ui_xml_get_data (priv->tree, node);
	data->dirty = TRUE;

	g_free (cmd_name);
}

static void
widget_set_state (GtkWidget *widget, xmlNode *node)
{
	char *txt;

	if ((txt = xmlGetProp (node, "sensitive"))) {
		gtk_widget_set_sensitive (widget, atoi (txt));
		xmlFree (txt);
	}

	if ((txt = xmlGetProp (node, "state"))) {

		if (BONOBO_IS_UI_ITEM (widget))
			bonobo_ui_item_set_state (
				BONOBO_UI_ITEM (widget), txt);

		else if (GTK_IS_CHECK_MENU_ITEM (widget))
			gtk_check_menu_item_set_active (
				GTK_CHECK_MENU_ITEM (widget), 
				atoi (txt));
		else
			g_warning ("TESTME: strange, setting "
				   "state '%s' on wierd object", txt);
		xmlFree (txt);
	}
}

static void
update_cmd_state (BonoboWinPrivate *priv, xmlNode *search,
		  xmlNode *state, const char *search_id)
{
	xmlNode    *l;
	char *id = node_get_id (search);

	g_return_if_fail (search_id != NULL);

	if (id && !strcmp (search_id, id)) { /* Sync its state */
		NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, search);

		if (info->widget)
			widget_set_state (info->widget, state);
	}

	for (l = search->childs; l; l = l->next)
		update_cmd_state (priv, l, state, search_id);

	g_free (id);
}

static void
update_commands_state (BonoboWinPrivate *priv)
{
	xmlNode *cmds, *l;

	cmds = bonobo_ui_xml_get_path (priv->tree, "/commands");

	if (!cmds)
		return;

	for (l = cmds->childs; l; l = l->next) {
		BonoboUIXmlData *data = bonobo_ui_xml_get_data (priv->tree, l);
		char *cmd_name;

		cmd_name = xmlGetProp (l, "name");
		if (!cmd_name)
			g_warning ("Internal error; cmd with no id");

		else if (data->dirty)
			update_cmd_state (priv, priv->tree->root, l, cmd_name);

		data->dirty = FALSE;
		XML_FREE (cmd_name);
	}
}

/**
 * set_cmd_state:
 * @priv: environment data
 * @cmd_node: the node containing either the command name or a plain boring widget
 * @prop: the name of the property to set
 * @value: the value of the property
 * @immediate_update: whether to immediately re-sync all dependant nodes.
 * 
 *  Set the state of the XML node representing this command.
 **/
static void
set_cmd_state (BonoboWinPrivate *priv, xmlNode *cmd_node, const char *prop,
	       const char *value, gboolean immediate_update)
{
	xmlNode *node;
	char    *cmd_name;
	char    *old_value;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (prop != NULL);
	g_return_if_fail (value != NULL);
	g_return_if_fail (cmd_node != NULL);

	if (!(cmd_name = node_get_id (cmd_node))) { /* A non cmd widget */
		NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, cmd_node);

		widget_set_state (info->widget, cmd_node);
		xmlSetProp (cmd_node, prop, value);
		return;
	}

	node = get_cmd_state (priv, cmd_name);

	old_value = xmlGetProp (node, prop);

#ifdef NAUTILUS_LOOP
	fprintf (stderr, "Set '%s' : '%s' to '%s' (%d)",
		 cmd_name, prop, value, immediate_update);
#endif
	/* We set it to the same thing */
	if (old_value && !strcmp (old_value, value)) {
		g_free (cmd_name);
#ifdef NAUTILUS_LOOP
		fprintf (stderr, "same\n");
#endif
		return;
	}
#ifdef NAUTILUS_LOOP
	else
		fprintf (stderr, "different\n");
#endif

	xmlSetProp (node, prop, value);

	if (immediate_update)
		update_cmd_state (priv, priv->tree->root, node, cmd_name);
	else {
		BonoboUIXmlData *data =
			bonobo_ui_xml_get_data (priv->tree, node);

		data->dirty = TRUE;
	}
	g_free (cmd_name);
}

static void
real_emit_ui_event (BonoboWinPrivate *priv, const char *component_name,
		    const char *id, int type, const char *new_state)
{
	Bonobo_UIComponent component;

	g_return_if_fail (id != NULL);
	g_return_if_fail (new_state != NULL);

	if (!component_name) /* Auto-created entry, no-one can listen to it */
		return;

	gtk_object_ref (GTK_OBJECT (priv->app));

	component = win_component_objref (priv, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		Bonobo_UIComponent_ui_event (
			component, id, type, new_state, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			/* FIXME: so if it is a sys exception do we de-merge ? */
			g_warning ("Exception emitting state change to %d '%s' '%s'",
				   type, id, new_state);
		}
		
		CORBA_exception_free (&ev);
	} else
		g_warning ("NULL Corba handle of name '%s'", component_name);

	gtk_object_unref (GTK_OBJECT (priv->app));
}

static void
custom_widget_unparent (NodeInfo *info)
{
	GtkContainer *container;

	g_return_if_fail (info != NULL);

	if (!info->widget)
		return;

	g_return_if_fail (GTK_IS_WIDGET (info->widget));

	container = GTK_CONTAINER (info->widget->parent);
	g_return_if_fail (container != NULL);

	gtk_widget_ref (info->widget);
	gtk_container_remove (container, info->widget);
}

static void
replace_override_fn (GtkObject        *object,
		     xmlNode          *new,
		     xmlNode          *old,
		     BonoboWinPrivate *priv)
{
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, new);
	NodeInfo *old_info = bonobo_ui_xml_get_data (priv->tree, old);

	g_return_if_fail (info != NULL);
	g_return_if_fail (old_info != NULL);

/*	g_warning ("Replace override on '%s' '%s' widget '%p'",
		   old->name, xmlGetProp (old, "name"), old_info->widget);
	info_dump_fn (old_info);
	info_dump_fn (info);*/

	/* Copy useful stuff across */
	info->type = old_info->type;
	info->widget = old_info->widget;

	/* Steal object reference */
	info->object = old_info->object;
	old_info->object = CORBA_OBJECT_NIL;
}

static void
override_fn (GtkObject *object, xmlNode *node, BonoboWinPrivate *priv)
{
	char     *id = node_get_id_or_path (node);
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	/* To stop stale pointers floating in the overrides */
	if (info->widget && NODE_IS_CUSTOM_WIDGET (info)) {
		custom_widget_unparent (info);
		g_warning ("TESTME: untested code path overriding custom widgets");
	} else
		info->widget = NULL;

	real_emit_ui_event (priv, info->parent.id, id,
			    Bonobo_UIComponent_OVERRIDDEN, "");

/*	fprintf (stderr, "XOverride '%s'\n", id);*/

	g_free (id);
}

static void
reinstate_fn (GtkObject *object, xmlNode *node, BonoboWinPrivate *priv)
{
	char     *id = node_get_id_or_path (node);
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	if (!NODE_IS_CUSTOM_WIDGET (info))
		g_assert (info->widget == NULL);

	real_emit_ui_event (priv, info->parent.id, id,
			    Bonobo_UIComponent_REINSTATED, "");

/*	fprintf (stderr, "XReinstate '%s'\n", id);*/

	g_free (id);
}

static void
remove_fn (GtkObject *object, xmlNode *node, BonoboWinPrivate *priv)
{
	char     *id = node_get_id_or_path (node);
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	real_emit_ui_event (priv, info->parent.id, id,
			    Bonobo_UIComponent_REMOVED, "");

	if (info->widget && !NODE_IS_ROOT_WIDGET (info))
		gtk_widget_destroy (info->widget);

	info->widget = NULL;

/*	fprintf (stderr, "XRemove '%s'\n", id);*/

	g_free (id);
}

/*
 * Doesn't the GtkRadioMenuItem API suck badly !
 */
#define MAGIC_RADIO_GROUP_KEY "Bonobo::RadioGroupName"

static void
radio_group_remove (GtkRadioMenuItem *menuitem,
		    char             *group_name)
{
	GtkRadioMenuItem *master;
	char             *orig_key;
	GSList           *l;
	BonoboWinPrivate *priv =
		gtk_object_get_data (GTK_OBJECT (menuitem),
				     MAGIC_RADIO_GROUP_KEY);

	if (!g_hash_table_lookup_extended
	    (priv->radio_groups, group_name, (gpointer *)&orig_key,
	     (gpointer *)&master)) {
		g_warning ("Radio group hash inconsistancy");
		return;
	}
	
	l = master->group;
	while (l && l->data == menuitem)
		l = l->next;
	
	g_hash_table_remove (priv->radio_groups, group_name);
	g_free (orig_key);

	if (l) { /* Entries left in group */
		g_hash_table_insert (priv->radio_groups,
				     group_name, l->data);
	} else /* alloced in signal_connect; grim hey */
		g_free (group_name);
}

static void
radio_group_add (BonoboWinPrivate *priv,
		 GtkRadioMenuItem *menuitem,
		 const char       *group_name)
{
	GtkRadioMenuItem *master;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (menuitem != NULL);
	g_return_if_fail (group_name != NULL);

	if (!(master = g_hash_table_lookup (priv->radio_groups, group_name)))
		g_hash_table_insert (priv->radio_groups, g_strdup (group_name),
				     menuitem);
	else
		gtk_radio_menu_item_set_group (
			menuitem, gtk_radio_menu_item_group (master));

	gtk_object_set_data (GTK_OBJECT (menuitem),
			     MAGIC_RADIO_GROUP_KEY, priv);

	gtk_signal_connect (GTK_OBJECT (menuitem), "destroy",
			    (GtkSignalFunc) radio_group_remove,
			    g_strdup (group_name));
}

/**
 *  The widget / container sweep is neccessary since info->widget is a 'submenu' for
 * menu construction, and yet the GtkMenuItem needs to be destroyed which is
 * stored in the container.
 **/
static void
container_destroy_siblings (BonoboUIXml *tree, GtkWidget *widget, xmlNode *node)
{
	xmlNode   *l;

/*	if (node)
		fprintf (stderr, "Container destroy siblings on '%s' '%s'\n",
		node->name, xmlGetProp (node, "name"));*/

	for (l = node; l; l = l->next) {
		NodeInfo *info;

		info = bonobo_ui_xml_get_data (tree, l);

		if (!NODE_IS_CUSTOM_WIDGET (info))
			container_destroy_siblings (tree, info->widget, l->childs);

		if (info->widget) {
			if (NODE_IS_CUSTOM_WIDGET (info))
				custom_widget_unparent (info);
			else {
				gtk_widget_destroy (info->widget);
				info->widget = NULL;
			}
		} /* else freshly merged and no widget yet */
	}

	if (GTK_IS_CONTAINER (widget))
		gtk_container_foreach (GTK_CONTAINER (widget),
				       (GtkCallback) gtk_widget_destroy,
				       NULL);
}

static void
real_exec_verb (BonoboWinPrivate *priv,
		const char       *component_name,
		const char       *verb)
{
	Bonobo_UIComponent component;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (verb != NULL);
	g_return_if_fail (component_name != NULL);

	gtk_object_ref (GTK_OBJECT (priv->app));

	component = win_component_objref (priv, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		Bonobo_UIComponent_exec_verb (
			component, verb, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			/* FIXME: so if it is a sys exception do we de-merge ? */
			g_warning ("Exception executing verb '%s' on '%s'",
				   verb, component_name);
		}
		
		CORBA_exception_free (&ev);
	} else
		g_warning ("NULL Corba handle of name '%s'", component_name);

	gtk_object_unref (GTK_OBJECT (priv->app));
}

static gint
exec_verb_cb (GtkWidget *item, xmlNode *node)
{
	CORBA_char        *verb;
	BonoboUIXmlData   *data;
	BonoboWinPrivate  *priv;

	g_return_val_if_fail (node != NULL, FALSE);

	priv = gtk_object_get_data (GTK_OBJECT (item), BONOBO_WIN_PRIV_KEY);
	g_return_val_if_fail (priv != NULL, FALSE);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_val_if_fail (data != NULL, FALSE);

	verb = node_get_id (node);
	if (!verb) {
		g_warning ("No verb on '%s' '%s'",
			   node->name, xmlGetProp (node, "name"));
		return FALSE;
	}

	if (!data->id) {
		g_warning ("Wierd; no ID on verb '%s'", verb);
		xmlFree (verb);
		return FALSE;
	}

	real_exec_verb (priv, data->id, verb);

	g_free (verb);

	return FALSE;
}

static gint
menu_toggle_emit_ui_event (GtkCheckMenuItem *item, xmlNode *node)
{
	char             *id, *state;
	BonoboUIXmlData  *data;
	BonoboWinPrivate *priv;

	g_return_val_if_fail (node != NULL, FALSE);

	priv = gtk_object_get_data (GTK_OBJECT (item), BONOBO_WIN_PRIV_KEY);
	g_return_val_if_fail (priv != NULL, FALSE);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_val_if_fail (data != NULL, FALSE);

	if (item->active)
		state = "1";
	else
		state = "0";
	
	if ((id = node_get_id (node))) {
		set_cmd_state (priv, node, "state", state, TRUE);
	} else {
		id = bonobo_ui_xml_make_path (node);
		xmlSetProp (node, "state", state);
	}

	real_emit_ui_event (priv, data->id, id,
			    Bonobo_UIComponent_STATE_CHANGED,
			    state);

	g_free (id);

	return FALSE;
}

static gint
app_item_emit_ui_event (BonoboUIItem *item, const char *state, xmlNode *node)
{
	char             *id;
	BonoboUIXmlData  *data;
	BonoboWinPrivate *priv;

	g_return_val_if_fail (node != NULL, FALSE);

	priv = gtk_object_get_data (GTK_OBJECT (item), BONOBO_WIN_PRIV_KEY);
	g_return_val_if_fail (priv != NULL, FALSE);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_val_if_fail (data != NULL, FALSE);

	if ((id = node_get_id (node))) {
		set_cmd_state (priv, node, "state", state, TRUE);
	} else {
		id = bonobo_ui_xml_make_path (node);
		xmlSetProp (node, "state", state);
	}

	real_emit_ui_event (priv, data->id, id,
			    Bonobo_UIComponent_STATE_CHANGED,
			    state);
	g_free (id);

	return FALSE;
}

#define BONOBO_WIN_MENU_ITEM_KEY "BonoboWin::Priv"

static void
put_hint_in_statusbar (GtkWidget *menuitem, xmlNode *node)
{
	BonoboWinPrivate *priv;
	char *hint;

	priv = gtk_object_get_data (GTK_OBJECT (menuitem),
				    BONOBO_WIN_MENU_ITEM_KEY);

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	hint = xmlGetProp (node, "descr");
	g_return_if_fail (hint != NULL);

	if (priv->main_status) {
		guint id;

		id = gtk_statusbar_get_context_id (priv->main_status,
						  "BonoboWin:menu-hint");
		gtk_statusbar_push (priv->main_status, id, hint);
	}
	xmlFree (hint);
}

static void
remove_hint_from_statusbar (GtkWidget *menuitem, xmlNode *node)
{
	BonoboWinPrivate *priv;

	priv = gtk_object_get_data (GTK_OBJECT (menuitem),
				    BONOBO_WIN_MENU_ITEM_KEY);

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	if (priv->main_status) {
		guint id;

		id = gtk_statusbar_get_context_id (priv->main_status,
						  "BonoboWin:menu-hint");
		gtk_statusbar_pop (priv->main_status, id);
	}
}


static void
menu_item_set_label (BonoboWinPrivate *priv, xmlNode *node,
		     GtkWidget *parent, GtkWidget *menu_widget)
{
	char *label_text;

	if ((label_text = xmlGetProp (node, "label"))) {
		GtkWidget *label;
		guint      keyval;

		label = gtk_accel_label_new (label_text);

		/*
		 * Setup the widget.
		 */
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_widget_show (label);
		
		/*
		 * Insert it into the menu item widget and setup the
		 * accelerator.
		 */
		gtk_container_add (GTK_CONTAINER (menu_widget), label);
		gtk_accel_label_set_accel_widget (
			GTK_ACCEL_LABEL (label), menu_widget);
	
		keyval = gtk_label_parse_uline (GTK_LABEL (label), label_text);

		if (keyval != GDK_VoidSymbol) {
			if (GTK_IS_MENU (parent))
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					gtk_menu_ensure_uline_accel_group (
						GTK_MENU (parent)),
					keyval, 0, 0);

			else if (GTK_IS_MENU_BAR (parent) &&
				 priv->accel_group != NULL)
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					priv->accel_group,
					keyval, GDK_MOD1_MASK, 0);
			else
				g_warning ("Adding accelerator went bananas");
		}
		xmlFree (label_text);
	}
}

static void
menu_item_set_global_accels (BonoboWinPrivate *priv, xmlNode *node,
			     GtkWidget *menu_widget)
{
	char *text;

	if ((text = xmlGetProp (node, "accel"))) {
		guint           key;
		GdkModifierType mods;
		char           *signal;

/*		fprintf (stderr, "Accel name is afterwards '%s'\n", text); */
		bonobo_ui_util_accel_parse (text, &key, &mods);
		xmlFree (text);

		if (!key)
			return;

/*		if (GTK_IS_CHECK_MENU_ITEM (menu_widget))
			signal = "toggled";
			else*/
		signal = "activate";

		gtk_widget_add_accelerator (menu_widget,
					    signal,
					    priv->accel_group,
					    key, mods,
					    GTK_ACCEL_VISIBLE);
	}
}

static GtkWidget *
menu_item_create (BonoboWinPrivate *priv, GtkWidget *parent, xmlNode *node)
{
	GtkWidget *menu_widget;
	NodeInfo  *info;
	char      *type;
	char      *hint;

	info = bonobo_ui_xml_get_data (priv->tree, node);

	/* Create menu item */
	if ((type = xmlGetProp (node, "type"))) {
		char *state;
		
		if (!strcmp (type, "radio")) {
			char *group = xmlGetProp (node, "group");

			menu_widget = gtk_radio_menu_item_new (NULL);

			if (group)
				radio_group_add (
					priv,
					GTK_RADIO_MENU_ITEM (menu_widget),
					group);

			xmlFree (group);
		} else if (!strcmp (type, "toggle"))
			menu_widget = gtk_check_menu_item_new ();

		else {
			g_warning ("Unhandled type of menu '%s'", type);
			return NULL;
		}

		info->widget = menu_widget;
			
		gtk_check_menu_item_set_show_toggle (
			GTK_CHECK_MENU_ITEM (menu_widget), TRUE);

		if (!(state = xmlGetProp (node, "state")))
			state = "0";
		set_cmd_state (priv, node, "state", state, FALSE);
		
		gtk_object_set_data (GTK_OBJECT (menu_widget), BONOBO_WIN_PRIV_KEY, priv);
		gtk_signal_connect (GTK_OBJECT (menu_widget), "toggled",
				    (GtkSignalFunc) menu_toggle_emit_ui_event,
				    node);

		xmlFree (type);
	} else {
		char *txt;

		if ((txt = xmlGetProp (node, "pixtype"))) {
			GtkWidget *pixmap;

			menu_widget = gtk_pixmap_menu_item_new ();

			pixmap = bonobo_ui_util_xml_get_pixmap (menu_widget, node);
			
			if (pixmap) {
				gtk_widget_show (GTK_WIDGET (pixmap));
				gtk_pixmap_menu_item_set_pixmap (
					GTK_PIXMAP_MENU_ITEM (menu_widget),
					GTK_WIDGET (pixmap));
			}
			xmlFree (txt);
		} else
			menu_widget = gtk_menu_item_new ();

		info->widget = menu_widget;
	}

	if ((hint = xmlGetProp (node, "descr"))) {
		gtk_object_set_data (GTK_OBJECT (menu_widget),
				     BONOBO_WIN_MENU_ITEM_KEY, priv);

		gtk_signal_connect (GTK_OBJECT (menu_widget),
				    "select",
				    GTK_SIGNAL_FUNC (put_hint_in_statusbar),
				    node);
		
		gtk_signal_connect (GTK_OBJECT (menu_widget),
				    "deselect",
				    GTK_SIGNAL_FUNC (remove_hint_from_statusbar),
				    node);
	}

	menu_item_set_label (priv, node, parent, menu_widget);

	menu_item_set_global_accels (priv, node, menu_widget);

	return menu_widget;
}

/*
 * Insert slightly cleverly.
 *  NB. it is no use inserting into the default placeholder here
 *  since this will screw up path addressing and subsequent merging.
 */
static void
add_node_fn (xmlNode *parent, xmlNode *child)
{
	xmlNode *insert = parent;
	char    *pos;
/*	xmlNode *l;
	if (!xmlGetProp (child, "noplace"))
		for (l = parent->childs; l; l = l->next) {
			if (!strcmp (l->name, "placeholder") &&
			    !xmlGetProp (l, "name")) {
			    insert = l;
				g_warning ("Found default placeholder");
			}
		}*/

	if (insert->childs &&
	    (pos = xmlGetProp (child, "pos"))) {
		if (!strcmp (pos, "top")) {
			g_warning ("TESTME: unused code branch");
			xmlAddPrevSibling (insert->childs, child);
		} else /* FIXME: we could have 'middle'; is it useful ? */
			xmlAddChild (insert, child);
		xmlFree (pos);
	} else /* just add to bottom */
		xmlAddChild (insert, child);
}

static void build_menu_widget (BonoboWinPrivate *priv, xmlNode *node);

static void
build_menu_placeholder (BonoboWinPrivate *priv, xmlNode *node, GtkWidget *parent)
{
	xmlNode   *l;
	char      *delimit;
	gboolean   top = FALSE, bottom = FALSE;
	GtkWidget *sep;

	if ((delimit = xmlGetProp (node, "delimit"))) {
		if (!strcmp (delimit, "top") ||
		    !strcmp (delimit, "both"))
			top = (node->childs != NULL) && (node->prev != NULL);

		if (!strcmp (delimit, "bottom") ||
		    !strcmp (delimit, "both"))
			bottom = (node->childs != NULL) && (node->next != NULL);
		xmlFree (delimit);
	}

	if (top) {
		sep = gtk_menu_item_new ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		gtk_menu_shell_append (GTK_MENU_SHELL (parent), sep);
	}

/*	g_warning ("Building placeholder %d %d %p %p %p", top, bottom,
	node->childs, node->prev, node->next);*/
		
	for (l = node->childs; l; l = l->next)
		build_menu_widget (priv, l);

	if (bottom) {
		sep = gtk_menu_item_new ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		gtk_menu_shell_append (GTK_MENU_SHELL (parent), sep);
	}
}

static GtkWidget *
build_control (BonoboWinPrivate *priv,
	       xmlNode          *node,
	       GtkWidget        *parent)
{
	GtkWidget *control = NULL;
	NodeInfo  *info = bonobo_ui_xml_get_data (priv->tree, node);

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
		info->widget = control;
	}

/*	fprintf (stderr, "Type on '%s' '%s' is %d widget %p\n",
		 node->name, xmlGetProp (node, "name"),
		 info->type, info->widget);*/

	return control;
}

static void
build_menu_widget (BonoboWinPrivate *priv, xmlNode *node)
{
	NodeInfo  *info;
	GtkWidget *parent, *menu_widget;
	char      *verb, *sensitive;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);
	g_return_if_fail (node->name != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_parent_widget (priv->tree, node);

	if (!strcmp (node->name, "placeholder")) {
		build_menu_placeholder (priv, node, parent);
		return;
	}

	if (!strcmp (node->name, "submenu")) {
		xmlNode      *l;
		GtkMenuShell *shell;
		GtkMenu      *menu;
		GtkWidget    *tearoff;
		
		menu_widget = menu_item_create (priv, parent, node);
		if (!menu_widget)
			return;

		if (parent == NULL)
			shell = GTK_MENU_SHELL (priv->menu);
		else
			shell = GTK_MENU_SHELL (parent);

		/* Create the menu shell. */
		menu = GTK_MENU (gtk_menu_new ());

		gtk_menu_set_accel_group (menu, priv->accel_group);

		/*
		 * Create the tearoff item at the beginning of the menu shell,
		 * if appropriate.
		 */
		if (gnome_preferences_get_menus_have_tearoff ()) {
			tearoff = gtk_tearoff_menu_item_new ();
			gtk_widget_show (tearoff);
			gtk_menu_prepend (GTK_MENU (menu), tearoff);
		}

		/*
		 * Associate this menu shell with the menu item for
		 * this submenu.
		 */
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_widget),
					   GTK_WIDGET (menu));

		info->widget = GTK_WIDGET (menu);

		gtk_menu_shell_append (shell, menu_widget);
		
		for (l = node->childs; l; l = l->next)
			build_menu_widget (priv, l);

		gtk_widget_show (GTK_WIDGET (menu));
		gtk_widget_show (GTK_WIDGET (shell));

	} else if (!strcmp (node->name, "menuitem")) {
		g_return_if_fail (parent != NULL);

		menu_widget = menu_item_create (priv, parent, node);
		if (!menu_widget)
			return;

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), menu_widget);
	} else if (!strcmp (node->name, "control")) {
		GtkWidget *control;

		control = build_control (priv, node, parent);
		if (!control)
			return;

		menu_widget = gtk_menu_item_new ();
		gtk_container_add (GTK_CONTAINER (menu_widget), control);
		gtk_widget_show (control);

		g_return_if_fail (menu_widget != NULL);

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), menu_widget);
	} else {
		g_warning ("Unknown name type '%s'", node->name);
		return;
	}

	if ((sensitive = xmlGetProp (node, "sensitive"))) {
		set_cmd_state (priv, node, "sensitive", sensitive, FALSE);
		xmlFree (sensitive);
	}
	
	set_cmd_dirty (priv, node);

	gtk_widget_show (menu_widget);
	
	if ((verb = xmlGetProp (node, "verb"))) {
		gtk_object_set_data (GTK_OBJECT (menu_widget),
				     BONOBO_WIN_PRIV_KEY, priv);
		gtk_signal_connect (GTK_OBJECT (menu_widget), "activate",
				    (GtkSignalFunc) exec_verb_cb, node);
		xmlFree (verb);
	}
}

static void
update_menus (BonoboWinPrivate *priv, xmlNode *node)
{
	xmlNode  *l;
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	if (info->widget)
		gtk_widget_hide (GTK_WIDGET (info->widget));

	container_destroy_siblings (priv->tree, info->widget, node->childs);

	/*
	 * Create the tearoff item at the beginning of the menu shell,
	 * if appropriate.
	 */
	if (info->widget &&
	    node_get_parent_widget (priv->tree, node) &&
	    gnome_preferences_get_menus_have_tearoff ()) {
		GtkWidget *tearoff = gtk_tearoff_menu_item_new ();
		gtk_widget_show (tearoff);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (info->widget), tearoff);
	}

	for (l = node->childs; l; l = l->next)
		build_menu_widget (priv, l);

	if (info->widget)
		gtk_widget_show (GTK_WIDGET (info->widget));
}

static void build_toolbar_widget (BonoboWinPrivate *priv, xmlNode *node);

static void
build_toolbar_placeholder (BonoboWinPrivate *priv, xmlNode *node, GtkWidget *parent)
{
	xmlNode   *l;
	char      *delimit;
	gboolean   top = FALSE, bottom = FALSE;
	GtkWidget *sep;

	if ((delimit = xmlGetProp (node, "delimit"))) {
		if (!strcmp (delimit, "top") ||
		    !strcmp (delimit, "both"))
			top = (node->childs != NULL) && (node->prev != NULL);

		if (!strcmp (delimit, "bottom") ||
		    !strcmp (delimit, "both"))
			bottom = (node->childs != NULL) && (node->next != NULL);
		xmlFree (delimit);
	}

	if (top) {
		sep = bonobo_ui_item_new_separator ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		bonobo_ui_toolbar_add (BONOBO_UI_TOOLBAR (parent), sep);
	}

/*	g_warning ("Building placeholder %d %d %p %p %p", top, bottom,
	node->childs, node->prev, node->next);*/
		
	for (l = node->childs; l; l = l->next)
		build_toolbar_widget (priv, l);

	if (bottom) {
		sep = bonobo_ui_item_new_separator ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		bonobo_ui_toolbar_add (BONOBO_UI_TOOLBAR (parent), sep);
	}
}

/*
 * see menu_toplevel_item_create_widget.
 */
static void
build_toolbar_widget (BonoboWinPrivate *priv, xmlNode *node)
{
	NodeInfo   *info;
	GtkWidget  *parent;
	char *type, *verb, *sensitive, *state, *txt, *label;
	GtkWidget  *pixmap;
	GtkWidget  *item;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_parent_widget (priv->tree, node);

	if (!strcmp (node->name, "placeholder")) {
		build_toolbar_placeholder (priv, node, parent);
		return;
	}

	/* Create toolbar item */
	if ((type = xmlGetProp (node, "pixtype"))) {
		pixmap = bonobo_ui_util_xml_get_pixmap (parent, node);
		if (pixmap) 
			gtk_widget_show (GTK_WIDGET (pixmap));
		xmlFree (type);
	} else
		pixmap = NULL;

	type = xmlGetProp (node, "type");
	label = xmlGetProp (node, "label");

	if (!type || !strcmp (type, "std"))
		item = bonobo_ui_item_new_item (label, pixmap);
	
	else if (!strcmp (type, "toggle"))
		item = bonobo_ui_item_new_toggle (label, pixmap);
	
	else if (!strcmp (type, "separator"))
		item = bonobo_ui_item_new_separator ();
	
	else {
		/* FIXME: Implement radio-toolbars */
		g_warning ("Invalid type '%s'", type);
		return;
	}

	XML_FREE (type);
	XML_FREE (label);
	
	bonobo_ui_toolbar_add (BONOBO_UI_TOOLBAR (parent), item);

	bonobo_ui_item_set_tooltip (
		BONOBO_UI_ITEM (item),
		bonobo_ui_toolbar_get_tooltips (
			BONOBO_UI_TOOLBAR (parent)),
		(txt = xmlGetProp (node, "descr")));
	XML_FREE (txt);

	info->widget = GTK_WIDGET (item);
	gtk_widget_show (info->widget);

	if ((verb = xmlGetProp (node, "verb"))) {
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    (GtkSignalFunc) exec_verb_cb, node);
		xmlFree (verb);
	}

	gtk_object_set_data (GTK_OBJECT (item), BONOBO_WIN_PRIV_KEY, priv);
	gtk_signal_connect (GTK_OBJECT (item), "state_altered",
			    (GtkSignalFunc) app_item_emit_ui_event, node);

	if ((sensitive = xmlGetProp (node, "sensitive"))) {
		set_cmd_state (priv, node, "sensitive", sensitive, FALSE);
		xmlFree (sensitive);
	}

	if ((state = xmlGetProp (node, "state"))) {
		set_cmd_state (priv, node, "state", state, FALSE);
		xmlFree (state);
	}

	set_cmd_dirty (priv, node);
}


static void
build_toolbar_control (BonoboWinPrivate *priv, xmlNode *node)
{
	NodeInfo   *info;
	GtkWidget  *parent;
	GtkWidget  *item;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_parent_widget (priv->tree, node);

	item = build_control (priv, node, parent);
	if (!item)
		return;

	gtk_widget_show (GTK_WIDGET (item));

	bonobo_ui_toolbar_add (BONOBO_UI_TOOLBAR (parent), item);
}

static void
update_dockitem (BonoboWinPrivate *priv, xmlNode *node)
{
	xmlNode       *l;
	NodeInfo      *info = bonobo_ui_xml_get_data (priv->tree, node);
	char          *txt;
	guint          dummy;
	char          *dockname = xmlGetProp (node, "name");
	GnomeDockItem *item;
	BonoboUIToolbar *toolbar;
	gboolean         tooltips;

	item = gnome_dock_get_item_by_name (priv->dock,
					    dockname,
					    &dummy, &dummy,
					    &dummy, &dummy);
	
	if (!item) {
		GnomeDockItemBehavior beh = GNOME_DOCK_ITEM_BEH_EXCLUSIVE;
		
		if (!gnome_preferences_get_toolbar_detachable())
			beh |= GNOME_DOCK_ITEM_BEH_LOCKED;

		item = GNOME_DOCK_ITEM (gnome_dock_item_new (
			dockname, beh));

		gnome_dock_add_item (priv->dock, item,
				     GNOME_DOCK_TOP,
				     1, 0, 0, TRUE);
	}

	/* Re-generation is far faster if unmapped */
	gtk_widget_hide (GTK_WIDGET (item));

	container_destroy_siblings (priv->tree, GTK_WIDGET (item), node->childs);

	toolbar = BONOBO_UI_TOOLBAR (bonobo_ui_toolbar_new ());

	if ((txt = xmlGetProp (node, "homogeneous"))) {
		bonobo_ui_toolbar_set_homogeneous (
			toolbar, atoi (txt));
		xmlFree (txt);
	}

	info->widget = GTK_WIDGET (toolbar);

	gtk_container_add (GTK_CONTAINER (item),
			   info->widget);
	gtk_widget_show (info->widget);

	for (l = node->childs; l; l = l->next) {
		if (!strcmp (l->name, "toolitem"))
			build_toolbar_widget (priv, l);

		else if (!strcmp (l->name, "control"))
			build_toolbar_control (priv, l);
	}

	if ((txt = xmlGetProp (node, "look"))) {
		if (!strcmp (txt, "both"))
			bonobo_ui_toolbar_set_style (
				toolbar, GTK_TOOLBAR_BOTH);
		else
			bonobo_ui_toolbar_set_style (
				toolbar, GTK_TOOLBAR_ICONS);
		xmlFree (txt);
	}

	if ((txt = xmlGetProp (node, "relief"))) {

		if (!strcmp (txt, "normal"))
			bonobo_ui_toolbar_set_relief (
				toolbar, GTK_RELIEF_NORMAL);

		else if (!strcmp (txt, "half"))
			bonobo_ui_toolbar_set_relief (
				toolbar, GTK_RELIEF_HALF);
		else
			bonobo_ui_toolbar_set_relief (
				toolbar, GTK_RELIEF_NONE);
		xmlFree (txt);
	}

	tooltips = TRUE;
	if ((txt = xmlGetProp (node, "tips"))) {
		tooltips = atoi (txt);
		xmlFree (txt);
	}
	
	bonobo_ui_toolbar_set_tooltips (toolbar, tooltips);

	if ((txt = xmlGetProp (node, "hidden"))) {
		if (atoi (txt)) {
			gtk_widget_hide (GTK_WIDGET (item));
			return;
		} else
			gtk_widget_show (GTK_WIDGET (item));
	} else {
		gtk_widget_show (GTK_WIDGET (item));
	}

	xmlFree (dockname);
}

typedef struct {
	guint           key;
	GdkModifierType mods;
	xmlNode        *node;
} Binding;

static gboolean
keybindings_free (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	g_free (key);

	return TRUE;
}

/*
 * Shamelessly stolen from gtkbindings.c
 */
static guint
keybinding_hash_fn (gconstpointer  key)
{
  register const Binding *e = key;
  register guint h;

  h = e->key;
  h ^= e->mods;

  return h;
}

static gint
keybinding_compare_fn (gconstpointer a,
		       gconstpointer b)
{
	register const Binding *ba = a;
	register const Binding *bb = b;

	return (ba->key == bb->key && ba->mods == bb->mods);
}

static void
update_keybindings (BonoboWinPrivate *priv, xmlNode *node)
{
	xmlNode *l;

	g_hash_table_foreach_remove (priv->keybindings, keybindings_free, NULL);

	for (l = node->childs; l; l = l->next) {
		guint           key;
		GdkModifierType mods;
		char           *name;
		Binding        *binding;
		
		name = xmlGetProp (l, "name");
		if (!name)
			continue;
		
		bonobo_ui_util_accel_parse (name, &key, &mods);
		xmlFree (name);

		binding       = g_new0 (Binding, 1);
		binding->mods = mods & BINDING_MOD_MASK ();
		binding->key  = gdk_keyval_to_lower (key);
		binding->node = l;

		g_hash_table_insert (priv->keybindings, binding, binding);
	}
}

static void
update_status (BonoboWinPrivate *priv, xmlNode *node)
{
	xmlNode *l;
	char *txt;
	GtkWidget *item = GTK_WIDGET (priv->status);

	gtk_widget_hide (item);

	container_destroy_siblings (priv->tree, GTK_WIDGET (priv->status), node->childs);

	priv->main_status = NULL;

	for (l = node->childs; l; l = l->next) {
		char *name;
		GtkWidget *widget;
		
		name = xmlGetProp (l, "name");
		if (!name)
			continue;

		if (!strcmp (name, "main")) {
			char *txt;

			widget = gtk_statusbar_new ();
			priv->main_status = GTK_STATUSBAR (widget);
			/* insert a little padding so text isn't jammed against frame */
			gtk_misc_set_padding (
				GTK_MISC (GTK_STATUSBAR (widget)->label),
				GNOME_PAD, 0);
			gtk_widget_show (GTK_WIDGET (widget));
			gtk_box_pack_start (priv->status, widget, TRUE, TRUE, 0);
			
			if ((txt = xmlNodeGetContent (l))) {
				guint id;

				id = gtk_statusbar_get_context_id (priv->main_status,
								   "BonoboWin:data");

				gtk_statusbar_push (priv->main_status, id, txt);
				xmlFree (txt);
			}
		} else if (!strcmp (l->name, "control")) {
			NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, l);

			if (info->object == CORBA_OBJECT_NIL) {
				xmlFree (name);
				continue;
			}

			widget = build_control (
				priv, l, GTK_WIDGET (priv->status));
			if (!widget)
				return;

			gtk_widget_show (widget);
			gtk_box_pack_end (priv->status, widget, TRUE, TRUE, 0);
		}
		xmlFree (name);
	}

	if ((txt = xmlGetProp (node, "hidden"))) {
		if (atoi (txt)) {
			gtk_widget_hide (item);
			return;
		} else
			gtk_widget_show (item);
	} else {
		gtk_widget_show (item);
	}
}

typedef enum {
	UI_UPDATE_MENU,
	UI_UPDATE_DOCKITEM
} UIUpdateType;

static void
seek_dirty (BonoboWinPrivate *priv, xmlNode *node, UIUpdateType type)
{
	BonoboUIXmlData *info;

	if (!node)
		return;

	info = bonobo_ui_xml_get_data (priv->tree, node);
	if (info->dirty) { /* Rebuild tree from here down */
		
		bonobo_ui_xml_set_dirty (priv->tree, node, FALSE);

		switch (type) {
		case UI_UPDATE_MENU:
			update_menus (priv, node);
			break;
		case UI_UPDATE_DOCKITEM:
			update_dockitem (priv, node);
			break;
		default:
			g_warning ("Looking for unhandled super type");
			break;
		}
	} else {
		xmlNode *l;

		for (l = node->childs; l; l = l->next)
			seek_dirty (priv, l, type);
	}
}

static void
setup_root_widgets (BonoboWinPrivate *priv)
{
	xmlNode  *node;
	NodeInfo *info;
	GSList   *l;

	if ((node = bonobo_ui_xml_get_path (priv->tree,
					    "/menu"))) {
		info = bonobo_ui_xml_get_data (priv->tree, node);
		info->widget = GTK_WIDGET (priv->menu);
		info->type |= ROOT_WIDGET;
	}

	for (l = priv->popups; l; l = l->next) {
		WinPopup *popup = l->data;

		if ((node = bonobo_ui_xml_get_path (priv->tree,
						    popup->path))) {
			info = bonobo_ui_xml_get_data (priv->tree, node);
			info->widget = GTK_WIDGET (popup->menu);
			info->type |= ROOT_WIDGET;
		}
	}
}

static void
update_widgets (BonoboWinPrivate *priv)
{
	xmlNode *node;

	if (priv->frozen)
		return;

	setup_root_widgets (priv);

	for (node = priv->tree->root->childs; node; node = node->next) {
		if (!node->name)
			continue;

		if (!strcmp (node->name, "menu")) {
			seek_dirty (priv, node, UI_UPDATE_MENU);

		} else if (!strcmp (node->name, "popup")) {
			seek_dirty (priv, node, UI_UPDATE_MENU);

		} else if (!strcmp (node->name, "dockitem")) {
			seek_dirty (priv, node, UI_UPDATE_DOCKITEM);

		} else if (!strcmp (node->name, "keybindings")) {
			update_keybindings (priv, node);

		} else if (!strcmp (node->name, "status")) {
			update_status (priv, node);

		} /* else unknown */
	}

	update_commands_state (priv);
}

void
bonobo_win_set_contents (BonoboWin *app,
			 GtkWidget *contents)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->client_area != NULL);

	gtk_container_add (GTK_CONTAINER (app->priv->client_area), contents);
}

GtkWidget *
bonobo_win_get_contents (BonoboWin *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	g_return_val_if_fail (app->priv != NULL, NULL);
	g_return_val_if_fail (app->priv->dock != NULL, NULL);

	return GTK_BIN (app->priv->client_area)->child;
}

static gboolean
radio_group_destroy (gpointer	key,
		     gpointer	value,
		     gpointer	user_data)
{
	g_free (key);
	g_slist_free (value);

	return TRUE;
}

static void
destroy_priv (BonoboWinPrivate *priv)
{
	priv->app = NULL;

	gtk_object_unref (GTK_OBJECT (priv->tree));
	priv->tree = NULL;

	g_free (priv->name);
	priv->name = NULL;

	g_free (priv->prefix);
	priv->prefix = NULL;

	g_hash_table_foreach_remove (priv->radio_groups,
				     radio_group_destroy, NULL);
	g_hash_table_destroy (priv->radio_groups);
	priv->radio_groups = NULL;

	g_hash_table_foreach_remove (priv->keybindings,
				     keybindings_free, NULL);
	g_hash_table_destroy (priv->keybindings);
	priv->keybindings = NULL;

	while (priv->components)
		win_component_destroy (priv, priv->components->data);

	while (priv->popups)
		popup_remove (priv, priv->popups->data);
	
	g_free (priv);
}

static void
bonobo_win_finalize (GtkObject *object)
{
	BonoboWin *app = (BonoboWin *)object;
	
	if (app) {
		if (app->priv)
			destroy_priv (app->priv);
		app->priv = NULL;
	}
	GTK_OBJECT_CLASS (bonobo_win_parent_class)->finalize (object);
}

char *
bonobo_win_xml_get (BonoboWin  *app,
		    const char *path,
		    gboolean    node_only)
{
	xmlDoc     *doc;
	xmlChar    *mem = NULL;
	xmlNode    *node;
	int         size;
	CORBA_char *ret;

	g_return_val_if_fail (BONOBO_IS_WIN (app), NULL);

	doc = xmlNewDoc ("1.0");
	g_return_val_if_fail (doc != NULL, NULL);

	node = bonobo_ui_xml_get_path (app->priv->tree, path);
	if (!node)
		return NULL;

	doc->root = xmlCopyNode (node, TRUE);
	g_return_val_if_fail (doc->root != NULL, NULL);

	if (node_only && doc->root->childs) {
#warning FIXME: this only frees the head child node!
		xmlNode *tmp = doc->root->childs;
		xmlUnlinkNode (tmp);
		xmlFreeNode (tmp);
	}

	xmlDocDumpMemory (doc, &mem, &size);

	g_return_val_if_fail (mem != NULL, NULL);

	xmlFreeDoc (doc);

	ret = CORBA_string_dup (mem);
	xmlFree (mem);

	return ret;
}

gboolean
bonobo_win_xml_node_exists (BonoboWin  *app,
			    const char *path)
{
	xmlNode *node;
	gboolean wildcard;

	g_return_val_if_fail (BONOBO_IS_WIN (app), FALSE);

	node = bonobo_ui_xml_get_path_wildcard (
		app->priv->tree, path, &wildcard);

	if (!wildcard)
		return (node != NULL);
	else
		return (node != NULL && node->childs != NULL);
}

BonoboUIXmlError
bonobo_win_object_set (BonoboWin  *app,
		       const char *path,
		       Bonobo_Unknown object,
		       CORBA_Environment *ev)
{
	xmlNode   *node;
	NodeInfo  *info;

	g_return_val_if_fail (BONOBO_IS_WIN (app), BONOBO_UI_XML_BAD_PARAM);

	node = bonobo_ui_xml_get_path (app->priv->tree, path);
	if (!node)
		return BONOBO_UI_XML_INVALID_PATH;

	info = bonobo_ui_xml_get_data (app->priv->tree, node);

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

	return BONOBO_UI_XML_OK;
}

BonoboUIXmlError
bonobo_win_object_get (BonoboWin  *app,
		       const char *path,
		       Bonobo_Unknown *object,
		       CORBA_Environment *ev)
{
	xmlNode *node;
	NodeInfo *info;

	g_return_val_if_fail (object != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (BONOBO_IS_WIN (app), BONOBO_UI_XML_BAD_PARAM);

	*object = CORBA_OBJECT_NIL;

	node = bonobo_ui_xml_get_path (app->priv->tree, path);
	if (!node)
		return BONOBO_UI_XML_INVALID_PATH;

	info = bonobo_ui_xml_get_data (app->priv->tree, node);

	if (info->object != CORBA_OBJECT_NIL)
		*object = bonobo_object_dup_ref (info->object, ev);

	return BONOBO_UI_XML_OK;
}

BonoboUIXmlError
bonobo_win_xml_merge_tree (BonoboWin  *app,
			   const char *path,
			   xmlNode    *tree,
			   const char *component)
{
	BonoboUIXmlError err;
	
	g_return_val_if_fail (app != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (app->priv != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (app->priv->tree != NULL, BONOBO_UI_XML_BAD_PARAM);

	if (!tree || !tree->name)
		return BONOBO_UI_XML_OK;

	/*
	 *  Because peer to peer merging makes the code hard, and
	 * paths non-inituitive and since we want to merge root
	 * elements as peers to save lots of redundant CORBA calls
	 * we special case root.
	 */
	if (!strcmp (tree->name, "Root")) {
		bonobo_ui_xml_strip (tree);
		err = bonobo_ui_xml_merge (
			app->priv->tree, path, tree->childs,
			win_component_cmp_name (app->priv, component));
	} else
		err = bonobo_ui_xml_merge (
			app->priv->tree, path, tree,
			win_component_cmp_name (app->priv, component));

	update_widgets (app->priv);

	return err;
}

BonoboUIXmlError
bonobo_win_xml_merge (BonoboWin  *app,
		      const char *path,
		      const char *xml,
		      const char *component)
{
	xmlDoc  *doc;
	BonoboUIXmlError err;

	g_return_val_if_fail (app != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (xml != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (app->priv != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (app->priv->tree != NULL, BONOBO_UI_XML_BAD_PARAM);

	doc = xmlParseDoc ((char *)xml);
	if (!doc)
		return BONOBO_UI_XML_INVALID_XML;

	err = bonobo_win_xml_merge_tree (app, path, doc->root, component);
	doc->root = NULL;
	
	xmlFreeDoc (doc);

	return err;
}

BonoboUIXmlError
bonobo_win_xml_rm (BonoboWin  *app,
		   const char *path,
		   const char *by_component)
{
	BonoboUIXmlError err;

	g_return_val_if_fail (app != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (app->priv != NULL, BONOBO_UI_XML_BAD_PARAM);
	g_return_val_if_fail (app->priv->tree != NULL, BONOBO_UI_XML_BAD_PARAM);

	err = bonobo_ui_xml_rm (
		app->priv->tree, path,
		win_component_cmp_name (app->priv, by_component));

	update_widgets (app->priv);

	return err;
}

void
bonobo_win_freeze (BonoboWin *win)
{
	g_return_if_fail (BONOBO_IS_WIN (win));

	win->priv->frozen++;
}

void
bonobo_win_thaw (BonoboWin *win)
{
	g_return_if_fail (BONOBO_IS_WIN (win));
	
	if (--win->priv->frozen <= 0) {
		update_widgets (win->priv);
		win->priv->frozen = 0;
	}
}

void
bonobo_win_dump (BonoboWin  *app,
		 const char *msg)
{
	g_return_if_fail (BONOBO_IS_WIN (app));

	fprintf (stderr, "Bonobo Win '%s': frozen '%d'\n",
		 app->priv->name, app->priv->frozen);

	bonobo_ui_xml_dump (app->priv->tree, app->priv->tree->root, msg);
}

GtkAccelGroup *
bonobo_win_get_accel_group (BonoboWin *app)
{
	g_return_val_if_fail (BONOBO_IS_WIN (app), NULL);

	return app->priv->accel_group;
}


static gint
bonobo_win_binding_handle (GtkWidget        *widget,
			   GdkEventKey      *event,
			   BonoboWinPrivate *priv)
{
	Binding  lookup, *binding;

	lookup.key  = gdk_keyval_to_lower (event->keyval);
	lookup.mods = event->state & BINDING_MOD_MASK ();

	if (!(binding = g_hash_table_lookup (priv->keybindings, &lookup)))
		return FALSE;
	else {
		BonoboUIXmlData *data;
		char *verb;
		
		data = bonobo_ui_xml_get_data (priv->tree, binding->node);
		g_return_val_if_fail (data != NULL, FALSE);
		
		real_exec_verb (priv, data->id,
				(verb = xmlGetProp (binding->node, "verb")));
		XML_FREE (verb);
		
		return TRUE;
	}
	
	return FALSE;
}

static BonoboWinPrivate *
construct_priv (BonoboWin *app)
{
	BonoboWinPrivate *priv;
	GnomeDockItemBehavior behavior;

	priv = g_new0 (BonoboWinPrivate, 1);

	priv->app    = app;	

	/* Keybindings; the gtk_binding stuff is just too evil */
	gtk_signal_connect (GTK_OBJECT (app), "key_press_event",
			    (GtkSignalFunc) bonobo_win_binding_handle, priv);

	priv->dock   = GNOME_DOCK (gnome_dock_new ());
	gtk_container_add (GTK_CONTAINER (app),
			   GTK_WIDGET    (priv->dock));

	behavior = (GNOME_DOCK_ITEM_BEH_EXCLUSIVE
		    | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	if (!gnome_preferences_get_menubar_detachable ())
		behavior |= GNOME_DOCK_ITEM_BEH_LOCKED;

	priv->menu_item = GNOME_DOCK_ITEM (gnome_dock_item_new (
		"menu", behavior));
	priv->menu      = GTK_MENU_BAR (gtk_menu_bar_new ());
	gtk_container_add (GTK_CONTAINER (priv->menu_item),
			   GTK_WIDGET    (priv->menu));
	gnome_dock_add_item (priv->dock, priv->menu_item,
			     GNOME_DOCK_TOP, 0, 0, 0, TRUE);

	priv->main_vbox = gtk_vbox_new (FALSE, 0);
	gnome_dock_set_client_area (priv->dock, priv->main_vbox);

	priv->client_area = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->client_area, TRUE, TRUE, 0);

	priv->status = GTK_BOX (gtk_hbox_new (FALSE, 0));
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), GTK_WIDGET (priv->status), FALSE, FALSE, 0);

	priv->tree = bonobo_ui_xml_new (NULL,
					info_new_fn,
					info_free_fn,
					info_dump_fn,
					add_node_fn);

	bonobo_ui_util_build_skeleton (priv->tree);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "override",
			    (GtkSignalFunc) override_fn, priv);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "replace_override",
			    (GtkSignalFunc) replace_override_fn, priv);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "reinstate",
			    (GtkSignalFunc) reinstate_fn, priv);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "remove",
			    (GtkSignalFunc) remove_fn, priv);

	priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app),
				    priv->accel_group);

	gtk_widget_show_all (GTK_WIDGET (priv->dock));
	gtk_widget_hide (GTK_WIDGET (priv->status));

	priv->radio_groups = g_hash_table_new (
		g_str_hash, g_str_equal);

	priv->keybindings = g_hash_table_new (keybinding_hash_fn, 
					      keybinding_compare_fn);	

	return priv;
}

static void
bonobo_win_class_init (BonoboWinClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_win_parent_class =
		gtk_type_class (gtk_window_get_type ());

	object_class->finalize = bonobo_win_finalize;
}

static void
bonobo_win_init (BonoboWin *win)
{
	win->priv = construct_priv (win);
}

void
bonobo_win_set_name (BonoboWin  *win,
		     const char *win_name)
{
	BonoboWinPrivate *priv;

	g_return_if_fail (BONOBO_IS_WIN (win));

	priv = win->priv;

	g_free (priv->name);
	g_free (priv->prefix);

	if (win_name) {
		priv->name = g_strdup (win_name);
		priv->prefix = g_strconcat ("/", win_name, "/", NULL);
	} else {
		priv->name = NULL;
		priv->prefix = g_strdup ("/");
	}
}

char *
bonobo_win_get_name (BonoboWin *win)
{
	g_return_val_if_fail (BONOBO_IS_WIN (win), NULL);
	g_return_val_if_fail (win->priv != NULL, NULL);

	if (win->priv->name)
		return g_strdup (win->priv->name);
	else
		return NULL;
}

GtkWidget *
bonobo_win_construct (BonoboWin  *win,
		      const char *win_name,
		      const char *title)
{
	g_return_val_if_fail (BONOBO_IS_WIN (win), NULL);

	bonobo_win_set_name (win, win_name);

	if (title)
		gtk_window_set_title (GTK_WINDOW (win), title);

	return GTK_WIDGET (win);
}

GtkWidget *
bonobo_win_new (const char   *win_name,
		const char   *title)
{
	BonoboWin *win;

	win = gtk_type_new (BONOBO_WIN_TYPE);

	return bonobo_win_construct (win, win_name, title);
}

/**
 * bonobo_win_get_type:
 *
 * Returns: The GtkType for the BonoboWin class.
 */
GtkType
bonobo_win_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboWin",
			sizeof (BonoboWin),
			sizeof (BonoboWinClass),
			(GtkClassInitFunc) bonobo_win_class_init,
			(GtkObjectInitFunc) bonobo_win_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_window_get_type (), &info);
	}

	return type;
}
